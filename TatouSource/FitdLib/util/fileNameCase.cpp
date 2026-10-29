///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// File names in any case (see fileNameCase.h)
///////////////////////////////////////////////////////////////////////////////

#include "fileNameCase.h"

#ifdef _WIN32

std::string matchFileCase(const std::string& path)
{
    return path;   // Windows names ignore case already
}

#else

#include <filesystem>
#include <strings.h>

namespace fs = std::filesystem;

std::string matchFileCase(const std::string& path)
{
    std::error_code error;
    if (path.empty() || fs::exists(path, error))
        return path;

    // Folder by folder from the start: the part found so far is searched for
    // the next name in any case
    const fs::path wanted(path);
    fs::path found = wanted.root_path();
    for (const fs::path& part : wanted.relative_path())
    {
        const fs::path exact = found / part;
        if (fs::exists(exact, error))
        {
            found = exact;
            continue;
        }
        const fs::path folder = found.empty() ? fs::path(".") : found;
        bool matched = false;
        for (const fs::directory_entry& entry : fs::directory_iterator(folder, error))
        {
            const std::string name = entry.path().filename().string();
            if (strcasecmp(name.c_str(), part.string().c_str()) == 0)
            {
                found /= name;
                matched = true;
                break;
            }
        }
        if (!matched)
            return path;
    }
    return found.string();
}

#endif
