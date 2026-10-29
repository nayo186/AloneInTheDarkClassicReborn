///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Custom backgrounds: export and import of camera backgrounds and full-screen
// pictures (title, book pages, character choice...)
///////////////////////////////////////////////////////////////////////////////

#include "customBackgrounds.h"
#include "settings.h"
#include <bx/timer.h>
#include "common.h"  // after bx: windows.h min/max macros
#include "consoleLog.h"
#include "userFolder.h"
#include "gameFolder.h"
#include "coreHooks.h"
#include "platformHooks.h"
#include "pak.h"
#include "AITD1.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image_write.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image.h"

#include "jobSystem.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <fstream>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" { extern char homePath[512]; }

namespace fs = std::filesystem;

// Exports and the player's pictures: the working directory (the copy's folder,
// or the user folder for a GOG installation; never the game data)
static std::string folder(const char* name)
{
    return name;
}

// "CAMERA00_000.png": the PAK name and the index of the picture in it
static std::string pictureName(const char* pakName, int index)
{
    char name[64];
    snprintf(name, sizeof(name), "%s_%03d.png", pakName, index);
    return name;
}

// AITD1 full-screen pictures of ITD_RESS (AITD1.h): armadillo, letter, book,
// notebook, character choice, frog, game over, title, intro background
static const int kAitd1Screens[] = { AITD1_TATOU_MCG, AITD1_LETTRE, AITD1_LIVRE, AITD1_CARNET, AITD1_PERSO_CHOICE,
    AITD1_GRENOUILLE, AITD1_DEAD_END, AITD1_TITRE, AITD1_FOND_INTRO };

// Pixels of a full-screen picture: 64000 bytes, or 770 bytes of header first
// (title, armadillo: core/AITD1.cpp, AITD1_Tatou.cpp)
static int pictureOffset(int size)
{
    return (size == 64000 + 770) ? 770 : 0;
}

// ---- One picture: the original and the player's replacement -----------------

struct Picture
{
    char pakName[16] = "";
    int index = -1;
    unsigned char originalPixels[320 * 200];   // palette indexes as loaded
    bgfx::TextureHandle original = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle custom = BGFX_INVALID_HANDLE;
    int customWidth = 0;
    int customHeight = 0;
    bool customOriginalShape = false;  // 16:10 file like 320x200: all of it is the play area
    fs::file_time_type customTime;
    bool customChecked = false;        // the file was looked for

    bool known() const { return index >= 0; }
};

// The camera background of the current camera, and the full-screen picture
// shown last (it is used only while the screen still shows it)
static Picture s_camera;
static Picture s_screen;
static bool s_screenOnScreen = false;

// ---- Pictures decoded in the background ------------------------------------------
// Decoding an HD picture takes ~130 ms (2880x2160): done by the job threads,
// ahead for the other cameras of the room, so a camera change finds it ready.
// When it is not ready the game waits for it with the last image on screen.

struct DecodedPicture
{
    fs::file_time_type time;            // of the file decoded
    unsigned char* pixels = nullptr;    // RGBA, stbi's (freed with it)
    int width = 0;
    int height = 0;
    std::string error;                  // why it could not be read
    bool ready = false;
    std::mutex mutex;
    std::condition_variable done;

    ~DecodedPicture() { if (pixels) stbi_image_free(pixels); }
};

// By file path; at most kMaxDecoded kept (~190 MB at 2880x2160), oldest dropped
static const size_t kMaxDecoded = 8;
static std::map<std::string, std::shared_ptr<DecodedPicture>> s_decoded;
static std::deque<std::string> s_decodedOrder;

static void decodePicture(const std::string& path, DecodedPicture& decoded)
{
    int width, height, channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    std::lock_guard<std::mutex> lock(decoded.mutex);
    decoded.pixels = pixels;
    decoded.width = width;
    decoded.height = height;
    if (!pixels)
        decoded.error = stbi_failure_reason() ? stbi_failure_reason() : "unknown";
    decoded.ready = true;
}

// The picture of that file, decoded or on its way (a changed file is decoded
// again, so a picture saved while playing appears)
static std::shared_ptr<DecodedPicture> requestDecode(const fs::path& path, fs::file_time_type time)
{
    const std::string key = path.string();
    auto found = s_decoded.find(key);
    if (found != s_decoded.end() && found->second->time == time)
        return found->second;

    auto decoded = std::make_shared<DecodedPicture>();
    decoded->time = time;
    if (found == s_decoded.end())
        s_decodedOrder.push_back(key);
    s_decoded[key] = decoded;
    while (s_decodedOrder.size() > kMaxDecoded)
    {
        s_decoded.erase(s_decodedOrder.front());   // a job still running keeps its own
        s_decodedOrder.pop_front();
    }

    if (!JobSystem::instance().isInitialized())
    {
        decodePicture(key, *decoded);
        return decoded;
    }
    JobSystem::instance().scheduleJob(JobSystem::JobType::LOAD_HD_BACKGROUND, JobSystem::Priority::HIGH,
        [key, decoded]() {
            decodePicture(key, *decoded);
            decoded->done.notify_all();
        });
    return decoded;
}

static void dropDecoded()
{
    s_decoded.clear();
    s_decodedOrder.clear();
}

// Reads ahead the HD pictures of the other cameras of the current room
static void prefetchRoomCameras()
{
    if (!g_settings.graphics.customBackgrounds || currentRoom < 0 || currentRoom >= (int)roomDataTable.size())
        return;
    char pakName[16];
    snprintf(pakName, sizeof(pakName), "CAMERA%02d", g_currentFloor);
    for (const u16 camera : roomDataTable[currentRoom].cameraIdxTable)
    {
        const fs::path path = fs::u8path(folder(customPicturesFolder()) + pictureName(pakName, camera));
        std::error_code error;
        const fs::file_time_type time = fs::last_write_time(path, error);
        if (!error)
            requestDecode(path, time);
    }
}

static void dropCustom(Picture& picture)
{
    if (bgfx::isValid(picture.custom))
        bgfx::destroy(picture.custom);
    picture.custom = BGFX_INVALID_HANDLE;
    picture.customWidth = picture.customHeight = 0;
}

static void loadCustom(Picture& picture)
{
    dropCustom(picture);
    picture.customChecked = true;
    if (!picture.known())
        return;

    const fs::path path = fs::u8path(folder(customPicturesFolder()) + pictureName(picture.pakName, picture.index));
    std::error_code error;
    if (!fs::exists(path, error))
        return;
    picture.customTime = fs::last_write_time(path, error);

    // Usually decoded already (prefetchRoomCameras); otherwise the game waits
    // here with the last image on screen (the window keeps answering)
    const std::shared_ptr<DecodedPicture> decoded = requestDecode(path, picture.customTime);
    const double waitStart = (double)bx::getHPCounter() / (double)bx::getHPFrequency();
    bool wasReady;
    {
        std::unique_lock<std::mutex> lock(decoded->mutex);
        wasReady = decoded->ready;
        decoded->done.wait(lock, [&] { return decoded->ready; });
    }
    const int waitedMs = (int)(((double)bx::getHPCounter() / (double)bx::getHPFrequency() - waitStart) * 1000.0);
    if (!decoded->pixels)
    {
        printf(MAIN_WARN "Custom background %s could not be read (%s)\n", path.string().c_str(), decoded->error.c_str());
        return;
    }
    const int width = decoded->width;
    const int height = decoded->height;
    const float ratio = (float)width / (float)height;
    if (ratio < 4.0f / 3.0f - 0.01f)
    {
        printf(MAIN_WARN "Custom background %s is narrower than 4:3 (%dx%d): not used\n", path.string().c_str(), width, height);
        return;
    }
    // Uploaded straight from the decoded pixels (no 24 MB copy): bgfx lets go
    // of them once uploaded, and only then this reference
    auto* keep = new std::shared_ptr<DecodedPicture>(decoded);
    picture.custom = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        bgfx::makeRef(decoded->pixels, (uint32_t)width * height * 4,
            [](void*, void* userData) { delete (std::shared_ptr<DecodedPicture>*)userData; }, keep));
    picture.customWidth = width;
    picture.customHeight = height;
    // The shape of the stored original (320x200 and its enlargements): the
    // whole picture is the play area, shown in 4:3 like the original pixels
    picture.customOriginalShape = (ratio > 1.6f - 0.01f && ratio < 1.6f + 0.01f);
    if (wasReady)
        printf(MAIN_TAG "Custom background %s (%dx%d, decoded ahead)\n", pictureName(picture.pakName, picture.index).c_str(), width, height);
    else
        printf(MAIN_TAG "Custom background %s (%dx%d, waited %d ms)\n", pictureName(picture.pakName, picture.index).c_str(), width, height, waitedMs);
}

// Sets the picture shown; returns false when it was already this one
static bool setPicture(Picture& picture, const char* pakName, int index, const unsigned char* pixels)
{
    const bool same = (picture.index == index && strcmp(picture.pakName, pakName) == 0);
    strncpy(picture.pakName, pakName, sizeof(picture.pakName) - 1);
    picture.pakName[sizeof(picture.pakName) - 1] = '\0';
    picture.index = index;

    memcpy(picture.originalPixels, pixels, sizeof(picture.originalPixels));
    if (!bgfx::isValid(picture.original))
        picture.original = bgfx::createTexture2D(320, 200, false, 1, bgfx::TextureFormat::R8U);
    bgfx::updateTexture2D(picture.original, 0, 0, 0, 0, 320, 200, bgfx::copy(pixels, 320 * 200));

    if (same)
        return false;
    picture.customChecked = false;
    if (g_settings.graphics.customBackgrounds)
        loadCustom(picture);
    else
        dropCustom(picture);
    return true;
}

// A picture saved, added or removed while playing
static void pollCustom(Picture& picture)
{
    if (!picture.known())
        return;
    const fs::path path = fs::u8path(folder(customPicturesFolder()) + pictureName(picture.pakName, picture.index));
    std::error_code error;
    if (!fs::exists(path, error))
    {
        if (bgfx::isValid(picture.custom))
            dropCustom(picture);
    }
    else if (!bgfx::isValid(picture.custom) || fs::last_write_time(path, error) != picture.customTime)
    {
        loadCustom(picture);
    }
}

// ---- Masks of the current camera ---------------------------------------------

// Foreground masks of the current camera and the player's replacements
struct CameraMask
{
    int roomId;
    int maskId;
    std::vector<unsigned char> shape;  // 320x200, 0 or 0xFF
    bgfx::TextureHandle custom = BGFX_INVALID_HANDLE;
    fs::file_time_type customTime;
    bool customPresent = false;
    bool customOriginalShape = false;  // 16:10 like the 320x200 scene (e.g. enlarged by an AI)
};
static std::vector<CameraMask> s_masks;

static std::string maskName(const char* pakName, int index, int roomId, int maskId)
{
    char name[96];
    snprintf(name, sizeof(name), "%s_%03d_mask_%d_%d.png", pakName, index, roomId, maskId);
    return name;
}

static void dropMask(CameraMask& mask)
{
    if (bgfx::isValid(mask.custom))
        bgfx::destroy(mask.custom);
    mask.custom = BGFX_INVALID_HANDLE;
    mask.customPresent = false;
}

static void dropMasks()
{
    for (CameraMask& mask : s_masks)
        dropMask(mask);
    s_masks.clear();
}

// The player's mask: any size (the size of the custom picture), white covers
static void loadMask(CameraMask& mask)
{
    dropMask(mask);
    const fs::path path = fs::u8path(folder(customMasksFolder()) + maskName(s_camera.pakName, s_camera.index, mask.roomId, mask.maskId));
    std::error_code error;
    if (!fs::exists(path, error))
        return;
    mask.customPresent = true;
    mask.customTime = fs::last_write_time(path, error);

    int width, height, channels;
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 1);
    if (!pixels)
    {
        printf(MAIN_WARN "Custom mask %s could not be read (%s)\n", path.string().c_str(), stbi_failure_reason());
        return;
    }
    mask.custom = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::R8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, bgfx::copy(pixels, (uint32_t)width * height));
    stbi_image_free(pixels);
    const float shape = (float)width / (float)height;
    mask.customOriginalShape = (shape > 1.57f && shape < 1.63f);
    printf(MAIN_TAG "Custom mask %s (%dx%d)\n", maskName(s_camera.pakName, s_camera.index, mask.roomId, mask.maskId).c_str(), width, height);
}

// ---- Core hooks --------------------------------------------------------------

// A camera background was loaded into aux (core/camera.cpp)
static void onCameraBackgroundReady(const char* pakName, int index, const unsigned char* pixels)
{
    // Back to a camera: the full-screen picture is over
    s_screenOnScreen = false;

    // Closing a menu loads the same camera picture again without creating its
    // masks: keep them (they were dropped and "Export masks" found none)
    const bool sameCamera = (s_camera.index == index && strcmp(s_camera.pakName, pakName) == 0);
    if (!sameCamera)
        dropMasks();  // the masks of this camera are created next (osystem_createMask)
    setPicture(s_camera, pakName, index, pixels);
    prefetchRoomCameras();
}

// A full-screen picture is shown (title, book page, character choice...):
// the core loads and draws it as always, this only learns which one it is
static void onScreenPictureShown(const char* pakName, int index)
{
    const int size = getPakSize(pakName, index);
    if (size < 64000)
        return;
    char* data = loadPak(pakName, index);
    if (!data)
        return;
    setPicture(s_screen, pakName, index, (const unsigned char*)data + pictureOffset(size));
    free(data);
    // Used once the screen shows it (customBackgroundsScreenUploaded)
    s_screenOnScreen = false;
}

// The player's folders exist from the start (the launcher also creates the
// portable ones the first time it opens)
static void prepareCustomFolder()
{
    std::error_code error;
    fs::create_directories(fs::u8path(folder(customPicturesFolder())), error);
    if (usesPortableFolders())
    {
        fs::create_directories(fs::u8path(folder(customMasksFolder())), error);
        fs::create_directories(fs::u8path(folder(exportsFolder())), error);
    }
}

static void screenUploaded(const unsigned char* screen)
{
    if (!s_screen.known())
        return;
    // The full-screen picture is on screen while most of the screen is still
    // it (texts and frames drawn over it change only a part); another screen
    // (a camera, a menu, black) changes most of it
    int different = 0;
    for (int i = 0; i < 320 * 200; i++)
        different += (screen[i] != s_screen.originalPixels[i]);
    s_screenOnScreen = (different < 320 * 200 / 2);
}

static void update()
{
    if (!g_settings.graphics.customBackgrounds)
    {
        dropCustom(s_camera);
        dropCustom(s_screen);
        dropDecoded();
        s_camera.customChecked = s_screen.customChecked = false;
        for (CameraMask& mask : s_masks)
            dropMask(mask);
        return;
    }
    if (s_camera.known() && !s_camera.customChecked)
    {
        loadCustom(s_camera);
        for (CameraMask& mask : s_masks)
            loadMask(mask);
    }
    if (s_screen.known() && !s_screen.customChecked)
        loadCustom(s_screen);

    // Twice a second: pictures and masks saved, added or removed while playing
    static double s_lastCheck = 0.0;
    const double now = (double)bx::getHPCounter() / (double)bx::getHPFrequency();
    if (now - s_lastCheck < 0.5)
        return;
    s_lastCheck = now;

    pollCustom(s_camera);
    pollCustom(s_screen);

    std::error_code error;
    for (CameraMask& mask : s_masks)
    {
        const fs::path maskPath = fs::u8path(folder(customMasksFolder()) + maskName(s_camera.pakName, s_camera.index, mask.roomId, mask.maskId));
        if (!fs::exists(maskPath, error))
        {
            if (mask.customPresent)
                dropMask(mask);
        }
        else if (!mask.customPresent || fs::last_write_time(maskPath, error) != mask.customTime)
        {
            loadMask(mask);
        }
    }
}

// The picture replaced right now: the full-screen picture while it is on
// screen, otherwise the camera background during gameplay
static const Picture* activePicture()
{
    if (!g_settings.graphics.customBackgrounds)
        return nullptr;
    if (s_screenOnScreen && bgfx::isValid(s_screen.custom) && bgfx::isValid(s_screen.original))
        return &s_screen;
    if (bgfx::isValid(s_camera.custom) && bgfx::isValid(s_camera.original) && g_playWorldActive && !g_menuActive)
        return &s_camera;
    return nullptr;
}

// The picture on show, with the 4:3 part of a wider image
static bool backgroundPicture(BackgroundPicture* out)
{
    const Picture* picture = activePicture();
    if (!picture)
        return false;
    out->picture = picture->custom;
    out->original = picture->original;
    out->u0 = 0.0f;
    out->u1 = 1.0f;
    if (picture->customWidth <= 0 || picture->customHeight <= 0 || picture->customOriginalShape)
        return true;
    const float centreWidth = (float)picture->customHeight * 4.0f / 3.0f;
    if (centreWidth >= (float)picture->customWidth)
        return true;
    out->u0 = ((float)picture->customWidth - centreWidth) * 0.5f / (float)picture->customWidth;
    out->u1 = 1.0f - out->u0;
    return true;
}

static void maskCreated(int roomId, int maskId, const unsigned char* shape, int x1, int y1, int x2, int y2)
{
    (void)x1; (void)y1; (void)x2; (void)y2;
    for (CameraMask& mask : s_masks)
    {
        if (mask.roomId == roomId && mask.maskId == maskId)
        {
            dropMask(mask);
            mask.shape.assign(shape, shape + 320 * 200);
            if (g_settings.graphics.customBackgrounds)
                loadMask(mask);
            return;
        }
    }
    CameraMask mask;
    mask.roomId = roomId;
    mask.maskId = maskId;
    mask.shape.assign(shape, shape + 320 * 200);
    s_masks.push_back(mask);
    if (g_settings.graphics.customBackgrounds)
        loadMask(s_masks.back());
}

static bgfx::TextureHandle maskPicture(int roomId, int maskId, bool* originalShape)
{
    *originalShape = false;
    // Masks belong to the camera background only
    if (activePicture() != &s_camera)
        return BGFX_INVALID_HANDLE;
    for (const CameraMask& mask : s_masks)
    {
        if (mask.roomId == roomId && mask.maskId == maskId)
        {
            *originalShape = mask.customOriginalShape;
            return mask.custom;
        }
    }
    return BGFX_INVALID_HANDLE;
}

void customBackgroundsRegister()
{
    g_onCameraBackgroundReady = onCameraBackgroundReady;
    g_onScreenPictureShown = onScreenPictureShown;
    g_onFrameStart = update;
    g_getBackgroundPicture = backgroundPicture;
    g_getMaskPicture = maskPicture;
    g_onScreenUploaded = screenUploaded;
    g_onMaskCreated = maskCreated;
    prepareCustomFolder();
}

// ---- Export ------------------------------------------------------------------

// The game palette from the game files (ITD_RESS entry 3, loadPalette in
// core/startup.cpp), not the one on screen at the moment: title screens, menus
// and dark rooms change that one, and exports came out with wrong colours
static bool gamePalette(palette_t& palette)
{
    char* paletteData = loadPak("ITD_RESS", 3);
    if (!paletteData)
        return false;
    copyPalette(paletteData, palette);
    convertPaletteIfRequired(palette);
    free(paletteData);
    return true;
}

static std::atomic<int> s_exportState(0);
static std::atomic<int> s_exportCount(0);

static bool exportPicture(const std::string& outFolder, const char* pakName, int index, const palette_t& palette, int gameId)
{
    const int size = getPakSize(pakName, index);
    if (size < 64000)
        return false;
    char* data = loadPak(pakName, index);
    if (!data)
        return false;
    const unsigned char* pixels = (const unsigned char*)data + pictureOffset(size);

    // From Jack in the Dark on, each camera picture carries its own palette
    palette_t pictureColours = palette;
    if (gameId >= JACK && size >= 64000 + 256 * 3)
    {
        copyPalette(data + 64000, pictureColours);
        convertPaletteIfRequired(pictureColours);
    }

    // The original pixels as stored, 320x200 (the game shows them in 4:3)
    std::vector<unsigned char> rgb(320 * 200 * 3);
    for (int i = 0; i < 320 * 200; i++)
    {
        const unsigned char index8 = pixels[i];
        rgb[i * 3 + 0] = pictureColours[index8][0];
        rgb[i * 3 + 1] = pictureColours[index8][1];
        rgb[i * 3 + 2] = pictureColours[index8][2];
    }
    free(data);

    const std::string path = outFolder + pictureName(pakName, index);
    return stbi_write_png(path.c_str(), 320, 200, 3, rgb.data(), 320 * 3) != 0;
}

static void exportAll(int gameId)
{
    palette_t palette = {};
    if (!gamePalette(palette))
    {
        printf(MAIN_ERR "Background export: the game palette (ITD_RESS 3) could not be read\n");
        s_exportState = 3;
        return;
    }

    const std::string outFolder = folder(exportsFolder());
    std::error_code error;
    fs::create_directories(fs::u8path(outFolder), error);
    if (error)
    {
        printf(MAIN_ERR "Background export: cannot create %s\n", outFolder.c_str());
        s_exportState = 3;
        return;
    }

    for (int floor = 0; floor < 32; floor++)
    {
        char pakName[16];
        snprintf(pakName, sizeof(pakName), "CAMERA%02d", floor);
        const unsigned int count = PAK_getNumFiles(pakName);
        if (count == 0 || count > 4096)
            continue;
        for (unsigned int index = 0; index < count; index++)
        {
            if (exportPicture(outFolder, pakName, (int)index, palette, gameId))
                s_exportCount++;
        }
    }

    if (gameId == AITD1)
    {
        // Cameras with a special picture after the sorcerer dies (core/camera.cpp)
        const int specials[] = { AITD1_CAM07000, AITD1_CAM07001, AITD1_CAM06000, AITD1_CAM06005, AITD1_CAM06008 };
        for (int index : specials)
        {
            if (exportPicture(outFolder, "ITD_RESS", index, palette, gameId))
                s_exportCount++;
        }

        // Full-screen pictures; the armadillo has its own palette (ITD_RESS 1)
        palette_t tatouPalette = palette;
        if (char* tatouData = loadPak("ITD_RESS", AITD1_TATOU_PAL))
        {
            copyPalette(tatouData, tatouPalette);
            free(tatouData);
        }
        for (int index : kAitd1Screens)
        {
            const palette_t& colours = (index == AITD1_TATOU_MCG) ? tatouPalette : palette;
            if (exportPicture(outFolder, "ITD_RESS", index, colours, gameId))
                s_exportCount++;
        }
    }
    else if (gameId == JACK)
    {
        // Full-screen pictures (book, outside view...): every ITD_RESS entry
        // the size of a picture (exportPicture skips the rest)
        const unsigned int count = PAK_getNumFiles("ITD_RESS");
        for (unsigned int index = 0; index < count && index < 256; index++)
            if (exportPicture(outFolder, "ITD_RESS", (int)index, palette, gameId))
                s_exportCount++;
    }

    printf(MAIN_OK "Background export: %d pictures in %s\n", s_exportCount.load(), outFolder.c_str());
    s_exportState = 2;
}

void customBackgroundsExportStart()
{
    if (s_exportState == 1)
        return;
    s_exportCount = 0;
    s_exportState = 1;
    std::thread(exportAll, (int)g_gameId).detach();
}

int customBackgroundsExportState()
{
    return s_exportState;
}

int customBackgroundsExportCount()
{
    return s_exportCount;
}

// ---- Export of every mask -----------------------------------------------------

// The core's polygon filler (core/polys.cpp) draws into polyBackBuffer
// (core/lines.cpp). It is not reentrant: the game stays paused while the
// export runs (customBackgroundsExportBusy, advancedOptions.cpp)
extern unsigned char* polyBackBuffer;
void fillpoly(s16* datas, int n, unsigned char c);

static std::atomic<int> s_maskExportState(0);
static std::atomic<int> s_maskExportCount(0);

// One mask and its guide (the camera picture with the mask in red)
static bool writeMask(const std::string& maskFolder, const std::string& guideFolder, const char* pakName, int index,
                      int roomId, int maskId, const std::vector<unsigned char>& shape,
                      const unsigned char* pixels, const palette_t& palette)
{
    const std::string name = maskName(pakName, index, roomId, maskId);
    if (!stbi_write_png((maskFolder + name).c_str(), 320, 200, 1, shape.data(), 320))
        return false;
    if (pixels)
    {
        std::vector<unsigned char> guide(320 * 200 * 3);
        for (int i = 0; i < 320 * 200; i++)
        {
            int r = palette[pixels[i]][0], g = palette[pixels[i]][1], b = palette[pixels[i]][2];
            if (shape[i])
            {
                r = (r + 255 * 2) / 3;
                g = g / 3;
                b = b / 3;
            }
            guide[i * 3 + 0] = (unsigned char)r;
            guide[i * 3 + 1] = (unsigned char)g;
            guide[i * 3 + 2] = (unsigned char)b;
        }
        std::string guideName = name;
        guideName.insert(guideName.size() - 4, "_guide");
        stbi_write_png((guideFolder + guideName).c_str(), 320, 200, 3, guide.data(), 320 * 3);
    }
    return true;
}

// A camera picture (pixels of the 320x200 image); empty if it cannot be read
static std::vector<unsigned char> loadCameraPixels(const char* pakName, int index)
{
    std::vector<unsigned char> pixels;
    const int size = getPakSize(pakName, index);
    if (size < 64000)
        return pixels;
    if (char* data = loadPak(pakName, index))
    {
        pixels.assign((unsigned char*)data + pictureOffset(size), (unsigned char*)data + pictureOffset(size) + 64000);
        free(data);
    }
    return pixels;
}

// The masks of one AITD1 camera, from the floor's camera data, built as
// createAITD1Mask (core/camera.cpp) builds them on entering the camera.
// names: the picture names the masks are saved under (the floor camera, and
// the picture shown instead after the sorcerer dies)
static int exportCameraMasks(char* cameraData, const std::vector<std::pair<std::string, int>>& names,
                             const std::vector<std::vector<unsigned char>>& pictures, const palette_t& palette,
                             const std::string& maskFolder, const std::string& guideFolder)
{
    int count = 0;
    std::vector<unsigned char> shape(320 * 200);
    const int numViewedRooms = READ_LE_U16(cameraData + 0x12);
    for (int room = 0; room < numViewedRooms && room < 64; room++)
    {
        char* entry = cameraData + 0x14 + room * 12;   // AITD1 viewed-room entry
        char* data2 = cameraData + READ_LE_U16(entry + 2);
        char* data = data2 + 2;
        const int numMask = READ_LE_U16(data2);
        if (numMask < 0 || numMask > 256)
            continue;

        for (int maskIdx = 0; maskIdx < numMask; maskIdx++)
        {
            std::fill(shape.begin(), shape.end(), (unsigned char)0);
            polyBackBuffer = shape.data();
            char* src = data2 + READ_LE_U16(data + 2);
            const int numPolys = READ_LE_U16(src);
            src += 2;
            bool valid = (numPolys <= 256);
            for (int poly = 0; valid && poly < numPolys; poly++)
            {
                const int numPoints = READ_LE_U16(src);
                src += 2;
                if (numPoints > 64)
                {
                    valid = false;
                    break;
                }
                fillpoly((s16*)src, numPoints, 0xFF);
                src += numPoints * 4;
            }
            polyBackBuffer = nullptr;

            if (valid)
            {
                for (size_t n = 0; n < names.size(); n++)
                {
                    const unsigned char* pixels = pictures[n].empty() ? nullptr : pictures[n].data();
                    if (writeMask(maskFolder, guideFolder, names[n].first.c_str(), names[n].second, room, maskIdx,
                                  shape, pixels, palette))
                        count++;
                }
            }

            const int numOverlay = READ_LE_U16(data);
            data += 2;
            data += ((numOverlay * 4) + 1) * 2;
        }
    }
    return count;
}

// Picture shown instead of a floor camera after the sorcerer dies (core/camera.cpp)
static int specialCameraPicture(int floor, int camera)
{
    if (floor == 6 && camera == 0) return AITD1_CAM06000;
    if (floor == 6 && camera == 5) return AITD1_CAM06005;
    if (floor == 6 && camera == 8) return AITD1_CAM06008;
    if (floor == 7 && camera == 0) return AITD1_CAM07000;
    if (floor == 7 && camera == 1) return AITD1_CAM07001;
    return -1;
}

// The masks of one Jack in the Dark camera (MASKnn entry), built as loadMask
// (core/camera.cpp) builds them: per viewed room, per mask, a box and its
// rows as runs of skipped and covered pixels. The entry describes itself
// (each table starts with the offset of its first item).
static int exportJackCameraMasks(unsigned char* data, int size, const char* pakName, int index,
                                 const std::vector<unsigned char>& picture, const palette_t& palette,
                                 const std::string& maskFolder, const std::string& guideFolder)
{
    auto inside = [size](unsigned int offset, unsigned int length) { return offset + length <= (unsigned int)size; };
    if (!inside(0, 4))
        return 0;
    int count = 0;
    std::vector<unsigned char> shape(320 * 200);
    const unsigned int numRooms = READ_LE_U32(data) / 4;
    for (unsigned int room = 0; room < numRooms && room < 64 && inside(room * 4, 4); room++)
    {
        const unsigned int roomOffset = READ_LE_U32(data + room * 4);
        if (!inside(roomOffset, 4))
            continue;
        unsigned char* roomData = data + roomOffset;
        const unsigned int numMasks = READ_LE_U32(roomData) / 4;
        for (unsigned int maskIdx = 0; maskIdx < numMasks && maskIdx < 256 && inside(roomOffset + maskIdx * 4, 4); maskIdx++)
        {
            const unsigned int maskOffset = roomOffset + READ_LE_U32(roomData + maskIdx * 4);
            if (!inside(maskOffset, 12))
                continue;
            unsigned char* src = data + maskOffset;
            const int x1 = READ_LE_U16(src), y1 = READ_LE_U16(src + 2);
            const int rows = READ_LE_U16(src + 10);
            unsigned int position = maskOffset + 12;
            std::fill(shape.begin(), shape.end(), (unsigned char)0);
            bool valid = (rows >= 0 && rows <= 200);
            for (int row = 0; valid && row < rows; row++)
            {
                if (!inside(position, 2)) { valid = false; break; }
                const int runs = READ_LE_U16(data + position);
                position += 2;
                int offset = x1 + (y1 + row) * 320;
                for (int run = 0; run < runs; run++)
                {
                    if (!inside(position, 2)) { valid = false; break; }
                    offset += data[position];
                    const int covered = data[position + 1];
                    position += 2;
                    for (int m = 0; m < covered; m++, offset++)
                        if (offset >= 0 && offset < 320 * 200)
                            shape[offset] = 0xFF;
                }
            }
            if (valid && writeMask(maskFolder, guideFolder, pakName, index, (int)room, (int)maskIdx, shape,
                                   picture.empty() ? nullptr : picture.data(), palette))
                count++;
        }
    }
    return count;
}

static void exportAllMasks()
{
    palette_t palette = {};
    gamePalette(palette);

    const std::string maskFolder = folder(exportsFolder()) + "masks/";
    const std::string guideFolder = folder(exportsFolder()) + "masks_guides/";
    std::error_code error;
    fs::create_directories(fs::u8path(maskFolder), error);
    fs::create_directories(fs::u8path(guideFolder), error);
    if (error)
    {
        printf(MAIN_ERR "Mask export: cannot create %s\n", maskFolder.c_str());
        s_maskExportState = 3;
        return;
    }

    // Jack in the Dark: its masks are in MASKnn, one entry per camera
    for (int floor = 0; g_gameId == JACK && floor < 32; floor++)
    {
        char maskPak[16], cameraPak[16];
        snprintf(maskPak, sizeof(maskPak), "MASK%02d", floor);
        snprintf(cameraPak, sizeof(cameraPak), "CAMERA%02d", floor);
        const unsigned int cameras = PAK_getNumFiles(maskPak);
        for (unsigned int camera = 0; camera < cameras && camera < 4096; camera++)
        {
            const int size = getPakSize(maskPak, (int)camera);
            char* data = (size > 0) ? loadPak(maskPak, (int)camera) : nullptr;
            if (!data)
                continue;
            s_maskExportCount += exportJackCameraMasks((unsigned char*)data, size, cameraPak, (int)camera,
                loadCameraPixels(cameraPak, (int)camera), palette, maskFolder, guideFolder);
            free(data);
        }
    }

    // Alone in the Dark: the masks are polygons in the floor's camera data
    for (int floor = 0; g_gameId == AITD1 && floor < 32; floor++)
    {
        char floorPak[16], cameraPak[16];
        snprintf(floorPak, sizeof(floorPak), "ETAGE%02d", floor);
        snprintf(cameraPak, sizeof(cameraPak), "CAMERA%02d", floor);
        if (PAK_getNumFiles(floorPak) < 2)
            continue;
        const int dataSize = getPakSize(floorPak, 1);
        char* cameraData = loadPak(floorPak, 1);
        if (!cameraData)
            continue;

        const unsigned int cameras = PAK_getNumFiles(cameraPak);
        for (unsigned int camera = 0; camera < cameras && camera < 4096; camera++)
        {
            const unsigned int offset = READ_LE_U32(cameraData + camera * 4);
            if ((int)offset + 0x14 > dataSize)
                continue;

            std::vector<std::pair<std::string, int>> names = { { cameraPak, (int)camera } };
            std::vector<std::vector<unsigned char>> pictures = { loadCameraPixels(cameraPak, (int)camera) };
            const int special = specialCameraPicture(floor, (int)camera);
            if (special >= 0)
            {
                names.push_back({ "ITD_RESS", special });
                pictures.push_back(loadCameraPixels("ITD_RESS", special));
            }
            s_maskExportCount += exportCameraMasks(cameraData + offset, names, pictures, palette, maskFolder, guideFolder);
        }
        free(cameraData);
    }

    printf(MAIN_OK "Mask export: %d masks in %s (guides in %s)\n", (int)s_maskExportCount, maskFolder.c_str(), guideFolder.c_str());
    s_maskExportState = 2;
}

void customBackgroundsExportMasksStart()
{
    if (s_maskExportState == 1)
        return;
    if (g_gameId != AITD1 && g_gameId != JACK)
    {
        printf(MAIN_WARN "Mask export: only Alone in the Dark 1 and Jack in the Dark\n");
        s_maskExportState = 3;
        return;
    }
    s_maskExportCount = 0;
    s_maskExportState = 1;
    std::thread(exportAllMasks).detach();
}

int customBackgroundsExportMasksState()
{
    return s_maskExportState;
}

int customBackgroundsExportMasksCount()
{
    return s_maskExportCount;
}

bool customBackgroundsExportBusy()
{
    return s_maskExportState == 1 || customBackgroundsExportState() == 1;
}
