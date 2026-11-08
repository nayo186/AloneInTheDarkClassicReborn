// Alone in the Dark: Classic Reborn for Android: SDL's activity, starting
// the engine (libmain.so) with the arguments given to the app, for example
//   adb shell am start -n com.classicreborn.game/.ClassicRebornActivity --es args "--game aitd3"
// It also imports into the app's own folder (UserData), which other apps and
// the phone's file managers cannot reach: Android's folder picker (a folder
// copied to the phone, a memory card...), then what the chosen folder holds,
// recognised by its contents (importFolder), all in one import: a game with
// the languages, narrators, backgrounds, masks and saves its folder has (as
// the PC's portable UserData/<game>), the whole UserData of the PC, or only
// a language, a narrator or backgrounds and masks. The launcher
// (Launcher/androidJava.cpp) starts it and follows its progress. And the launcher's updates (Launcher/updater.cpp):
// downloads (Android has no curl) and the new app handed to Android's
// installer, which asks the player to confirm.
package com.classicreborn.game;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.os.Build;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class ClassicRebornActivity extends SDLActivity {
    private static final String TAG = "ClassicReborn";
    private static final int REQUEST_IMPORT = 4100;
    private static final String ACTION_INSTALLED = "com.classicreborn.game.INSTALL_STATUS";

    // What a GOG or Steam installation holds that the games use; anything
    // else (manuals, DOSBox, the store's files) is left out
    private static final String[] GAME_ITEMS = {
        "GAME.GOG", "GAME.INS", "GAME.INST", "INDARK", "INDARK2", "INDARK3", "MUSIC", "JACK"
    };

    // Import progress, read by the launcher (importStatus)
    private static final int IMPORT_IDLE = 0, IMPORT_PICKING = 1, IMPORT_COPYING = 2,
        IMPORT_DONE = 3, IMPORT_FAILED = 4, IMPORT_CANCELLED = 5;
    private static volatile int sImportState = IMPORT_IDLE;
    private static volatile long sImportCopied = 0, sImportTotal = 0;
    // What was imported (importStatus): the IMPORT_KIND_ values
    private static final int IMPORT_KIND_NONE = 0, IMPORT_KIND_GAME = 1, IMPORT_KIND_LANGUAGE = 2,
        IMPORT_KIND_NARRATOR = 3, IMPORT_KIND_PICTURES = 4;
    private static volatile int sImportKind = IMPORT_KIND_NONE;
    private static String sImportUserData = "";
    private static int sImportGame = 0;   // the launcher's tab: 0 AITD1, 1 AITD2, 2 AITD3
    private static final String[] GAME_FOLDERS = { "AITD1", "AITD2", "AITD3" };
    // The language PAKs of AITD3 and their folder in Languages (Shared/languageFolder.h)
    private static final String[][] LANGUAGES = {
        { "FRANCAIS.PAK", "French" }, { "ITALIANO.PAK", "Italian" },
        { "ESPAGNOL.PAK", "Spanish" }, { "DEUTSCH.PAK", "German" }, { "ENGLISH.PAK", "English" }
    };
    private static ClassicRebornActivity sActivity;

    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }

    @Override
    protected String[] getArguments() {
        String args = getIntent() != null ? getIntent().getStringExtra("args") : null;
        if (args == null || args.trim().isEmpty())
            return new String[0];
        return args.trim().split(" +");
    }

    @Override
    protected void onCreate(android.os.Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sActivity = this;
    }

    // From the launcher: opens the folder picker; what it holds goes into
    // userData (the app's UserData folder), for the game of the launcher's tab
    public static void startImport(String userData, int game) {
        final ClassicRebornActivity activity = sActivity;
        if (activity == null || sImportState == IMPORT_PICKING || sImportState == IMPORT_COPYING)
            return;
        sImportUserData = userData;
        sImportGame = Math.max(0, Math.min(2, game));
        sImportKind = IMPORT_KIND_NONE;
        sImportCopied = 0;
        sImportTotal = 0;
        sImportState = IMPORT_PICKING;
        activity.runOnUiThread(() -> {
            try {
                activity.startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), REQUEST_IMPORT);
            } catch (Exception e) {
                Log.e(TAG, "No folder picker", e);
                sImportState = IMPORT_FAILED;
            }
        });
    }

    // For the launcher: "state copied total kind" (the IMPORT_ and
    // IMPORT_KIND_ values; failed with kind none: nothing was recognised)
    public static String importStatus() {
        return sImportState + " " + sImportCopied + " " + sImportTotal + " " + sImportKind;
    }

    // ── Updates ─────────────────────────────────────────────────────────────

    // From the updater's thread: url into path; false if it failed
    public static boolean download(String url, String path, int seconds) {
        HttpURLConnection connection = null;
        try {
            connection = (HttpURLConnection) new URL(url).openConnection();
            connection.setInstanceFollowRedirects(true);
            connection.setConnectTimeout(30000);
            connection.setReadTimeout(seconds * 1000);
            connection.setRequestProperty("User-Agent", "ClassicReborn-Launcher");   // GitHub's API asks for one
            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                Log.e(TAG, "Download of " + url + ": HTTP " + connection.getResponseCode());
                return false;
            }
            File file = new File(path);
            file.getParentFile().mkdirs();
            byte[] buffer = new byte[1 << 16];
            try (InputStream in = connection.getInputStream(); OutputStream out = new FileOutputStream(file)) {
                int read;
                while ((read = in.read(buffer)) > 0)
                    out.write(buffer, 0, read);
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Download of " + url + " failed", e);
            return false;
        } finally {
            if (connection != null)
                connection.disconnect();
        }
    }

    // From the updater's thread: the new app to Android's installer, which
    // asks the player (onNewIntent); false if it could not be handed over
    public static boolean installPackage(String path) {
        final ClassicRebornActivity activity = sActivity;
        if (activity == null)
            return false;
        try {
            File file = new File(path);
            PackageInstaller installer = activity.getPackageManager().getPackageInstaller();
            PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
            int sessionId = installer.createSession(params);
            try (PackageInstaller.Session session = installer.openSession(sessionId)) {
                try (InputStream in = new FileInputStream(file);
                     OutputStream out = session.openWrite("app", 0, file.length())) {
                    byte[] buffer = new byte[1 << 16];
                    int read;
                    while ((read = in.read(buffer)) > 0)
                        out.write(buffer, 0, read);
                    session.fsync(out);
                }
                Intent status = new Intent(activity, ClassicRebornActivity.class).setAction(ACTION_INSTALLED);
                int flags = PendingIntent.FLAG_UPDATE_CURRENT | (Build.VERSION.SDK_INT >= 31 ? PendingIntent.FLAG_MUTABLE : 0);
                PendingIntent pending = PendingIntent.getActivity(activity, 0, status, flags);
                session.commit(pending.getIntentSender());
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Install of " + path + " failed", e);
            return false;
        }
    }

    // Android's installer answers: it needs the player's confirmation
    @Override
    protected void onNewIntent(Intent intent) {
        if (intent != null && ACTION_INSTALLED.equals(intent.getAction())) {
            int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
            Log.i(TAG, "Install status " + status + ": " + intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE));
            if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
                Intent confirm = intent.getParcelableExtra(Intent.EXTRA_INTENT);
                if (confirm != null)
                    startActivity(confirm);
            }
            return;
        }
        super.onNewIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_IMPORT) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            sImportState = IMPORT_CANCELLED;
            return;
        }
        final Uri tree = data.getData();
        sImportState = IMPORT_COPYING;
        new Thread(() -> {
            try {
                sImportState = importFolder(tree) ? IMPORT_DONE : IMPORT_FAILED;
            } catch (Exception e) {
                Log.e(TAG, "Import failed", e);
                sImportState = IMPORT_FAILED;
            }
        }, "ImportGame").start();
    }

    // An entry of a folder chosen in the picker
    private static class Entry {
        String id, name;
        boolean folder;
        long size;
    }

    private List<Entry> children(Uri tree, String folderId) {
        List<Entry> list = new ArrayList<>();
        Uri uri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, folderId);
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE, DocumentsContract.Document.COLUMN_SIZE
        };
        try (Cursor cursor = getContentResolver().query(uri, columns, null, null, null)) {
            while (cursor != null && cursor.moveToNext()) {
                Entry entry = new Entry();
                entry.id = cursor.getString(0);
                entry.name = cursor.getString(1);
                entry.folder = DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2));
                entry.size = cursor.isNull(3) ? 0 : cursor.getLong(3);
                list.add(entry);
            }
        }
        return list;
    }

    private static boolean isGameItem(String name) {
        for (String item : GAME_ITEMS)
            if (item.equalsIgnoreCase(name))
                return true;
        return false;
    }

    private long sizeOf(Uri tree, Entry entry) {
        if (!entry.folder)
            return entry.size;
        long total = 0;
        for (Entry child : children(tree, entry.id))
            total += sizeOf(tree, child);
        return total;
    }

    private static Entry find(List<Entry> entries, String name) {
        for (Entry entry : entries)
            if (entry.name.equalsIgnoreCase(name))
                return entry;
        return null;
    }

    private static boolean hasExtension(List<Entry> entries, String... extensions) {
        for (Entry entry : entries) {
            String lower = entry.name.toLowerCase(Locale.ROOT);
            for (String extension : extensions)
                if (!entry.folder && lower.endsWith(extension))
                    return true;
        }
        return false;
    }

    // The name of the chosen folder itself
    private String folderName(Uri tree) {
        String id = DocumentsContract.getTreeDocumentId(tree);
        Uri uri = DocumentsContract.buildDocumentUriUsingTree(tree, id);
        try (Cursor cursor = getContentResolver().query(uri,
                new String[] { DocumentsContract.Document.COLUMN_DISPLAY_NAME }, null, null, null)) {
            if (cursor != null && cursor.moveToFirst())
                return cursor.getString(0);
        }
        String tail = id.substring(id.lastIndexOf('/') + 1);
        return tail.substring(tail.lastIndexOf(':') + 1);
    }

    // A planned copy: an entry of the chosen folder into a folder of the app
    private static class Copy {
        final Entry entry;
        final File into;
        Copy(Entry entry, File into) { this.entry = entry; this.into = into; }
    }

    // The folders of a game's portable folder (Shared/userFolder.h) besides
    // GAME, copied as they are: the player's languages (AITD3), narrators
    // (AITD1), backgrounds, masks and saves
    private static final String[] EXTRA_FOLDERS = { "Languages", "Voices", "Backgrounds", "Masks", "Saves" };
    private static final String[] USERDATA_GAMES = { "AITD1", "AITD2", "AITD3", "JACK" };

    private static boolean isExtraFolder(Entry entry) {
        for (String extra : EXTRA_FOLDERS)
            if (entry.folder && extra.equalsIgnoreCase(entry.name))
                return true;
        return false;
    }

    private static boolean isUserDataGame(Entry entry) {
        for (String game : USERDATA_GAMES)
            if (entry.folder && game.equalsIgnoreCase(entry.name))
                return true;
        return false;
    }

    // The game's files among entries into gameData (its GAME folder): an
    // installation's items (GAME_ITEMS), or loose data (a copy: every file
    // but manuals); returns false if there are none
    private boolean planGameData(List<Entry> entries, File gameData, List<Copy> plan) {
        boolean found = false;
        for (Entry entry : entries)
            if (isGameItem(entry.name)) {
                plan.add(new Copy(entry, gameData));
                found = true;
            }
        if (found || !hasExtension(entries, ".pak"))
            return found;
        for (Entry entry : entries)
            if (!entry.folder && !entry.name.toLowerCase(Locale.ROOT).endsWith(".pdf"))
                plan.add(new Copy(entry, gameData));
        return true;
    }

    // A game's folder, as the portable one of the PC (UserData/AITD3: GAME,
    // Languages, Backgrounds...) or an installation with those folders inside:
    // everything into gameFolder; returns false if it holds none of it
    private boolean planGameFolder(Uri tree, List<Entry> entries, File gameFolder, List<Copy> plan) {
        boolean found = planGameData(entries, new File(gameFolder, "GAME"), plan);
        for (Entry entry : entries) {
            if (entry.folder && entry.name.equalsIgnoreCase("GAME"))
                found |= planGameData(children(tree, entry.id), new File(gameFolder, "GAME"), plan);
            else if (isExtraFolder(entry)) {
                plan.add(new Copy(entry, gameFolder));
                found = true;
            }
        }
        return found;
    }

    // What the chosen folder holds, and where it goes, all in one import:
    // - the whole UserData of the PC (AITD1, AITD2, AITD3, JACK inside): each
    //   game's folder into its own;
    // - a game's folder (the PC's portable UserData/<game>, or a GOG or Steam
    //   installation, or a copy of the data), with its GAME and the Languages,
    //   Voices, Backgrounds, Masks and Saves it has: into the folder of the
    //   game named so (AITD1...), otherwise of the launcher's tab;
    // - only a language of AITD3 (its PAK without the game's floors, or a CD
    //   image on the AITD3 tab): AITD3/Languages/<language>;
    // - only a narrator of AITD1 (its first voice 000000, or a CD image on the
    //   AITD1 tab): AITD1/Voices/<folder>;
    // - only a Backgrounds or Masks folder: into the tab's game.
    // Manuals (PDF) are left out. False if nothing was recognised (nothing is
    // copied).
    private boolean importFolder(Uri tree) throws Exception {
        List<Entry> top = children(tree, DocumentsContract.getTreeDocumentId(tree));
        String name = folderName(tree);
        File userData = new File(sImportUserData);
        File tabGame = new File(userData, GAME_FOLDERS[sImportGame]);
        List<Copy> plan = new ArrayList<>();
        boolean image = hasExtension(top, ".cue", ".bin", ".img", ".iso");
        String language = null;
        for (String[] pair : LANGUAGES)
            if (language == null && find(top, pair[0]) != null)
                language = pair[1];
        boolean floors = find(top, "ETAGE00.PAK") != null;

        // UserData has AITD1, AITD2 or AITD3 (a GOG installation of AITD1 has
        // a JACK folder too, Jack's data: it is not one)
        boolean userDataLevel = find(top, "AITD1") != null || find(top, "AITD2") != null || find(top, "AITD3") != null;
        if (userDataLevel)
            for (Entry entry : top)
                if (isUserDataGame(entry))
                    planGameFolder(tree, children(tree, entry.id), new File(userData, entry.name.toUpperCase(Locale.ROOT)), plan);
        if (userDataLevel) {
            sImportKind = IMPORT_KIND_GAME;
        } else if (language != null && !floors) {
            sImportKind = IMPORT_KIND_LANGUAGE;
            addAll(plan, top, new File(new File(userData, "AITD3/Languages"), language));
        } else if (find(top, "000000.VOC") != null || find(top, "000000.WAV") != null || find(top, "000000.OGG") != null
                || (image && sImportGame == 0)) {
            sImportKind = IMPORT_KIND_NARRATOR;
            addAll(plan, top, new File(new File(userData, "AITD1/Voices"), name));
        } else if (image && sImportGame == 2) {
            sImportKind = IMPORT_KIND_LANGUAGE;
            addAll(plan, top, new File(new File(userData, "AITD3/Languages"), name));
        } else if (name.equalsIgnoreCase("Backgrounds") || name.equalsIgnoreCase("Masks")) {
            sImportKind = IMPORT_KIND_PICTURES;
            addAll(plan, top, new File(tabGame, name.equalsIgnoreCase("Masks") ? "Masks" : "Backgrounds"));
        } else {
            // A game's folder: the one of its name (UserData/AITD3...) or the tab's
            File gameFolder = tabGame;
            for (String game : USERDATA_GAMES)
                if (game.equalsIgnoreCase(name))
                    gameFolder = new File(userData, game);
            if (!planGameFolder(tree, top, gameFolder, plan)) {
                Log.i(TAG, "Nothing to import in " + name);
                return false;
            }
            boolean gameData = false;
            for (Copy copy : plan)
                gameData |= copy.into.getName().equals("GAME");
            sImportKind = gameData ? IMPORT_KIND_GAME : IMPORT_KIND_PICTURES;
        }

        long total = 0;
        for (Copy copy : plan)
            total += sizeOf(tree, copy.entry);
        sImportTotal = total;
        Log.i(TAG, "Importing " + name + " (kind " + sImportKind + "): " + plan.size() + " items, " + total + " bytes");
        byte[] buffer = new byte[1 << 20];
        for (Copy copy : plan) {
            copy.into.mkdirs();
            copyEntry(tree, copy.entry, copy.into, buffer);
        }
        return true;
    }

    // Every entry but manuals into one folder
    private static void addAll(List<Copy> plan, List<Entry> entries, File into) {
        for (Entry entry : entries)
            if (entry.folder || !entry.name.toLowerCase(Locale.ROOT).endsWith(".pdf"))
                plan.add(new Copy(entry, into));
    }

    private void copyEntry(Uri tree, Entry entry, File folder, byte[] buffer) throws Exception {
        File target = new File(folder, entry.name);
        if (entry.folder) {
            target.mkdirs();
            for (Entry child : children(tree, entry.id))
                copyEntry(tree, child, target, buffer);
            return;
        }
        Uri uri = DocumentsContract.buildDocumentUriUsingTree(tree, entry.id);
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(target)) {
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
                sImportCopied += read;
            }
        }
    }
}
