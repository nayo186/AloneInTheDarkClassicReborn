// Alone in the Dark: Classic Reborn for Android: "Return to the launcher"
// (BUGS.md C94). The game ends its process when the player leaves it; this
// activity runs in a process of its own (AndroidManifest.xml), makes sure the
// game's has ended and starts the app again, which opens on the launcher's
// screen (Fitd/fitd.cpp). Started by ClassicRebornActivity.restartOnLauncher.
package com.classicreborn.game;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public class RestartActivity extends Activity {
    static final String EXTRA_GAME_PROCESS = "gameProcess";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        int gameProcess = getIntent().getIntExtra(EXTRA_GAME_PROCESS, -1);
        if (gameProcess > 0)
            android.os.Process.killProcess(gameProcess);
        Intent app = new Intent(this, ClassicRebornActivity.class);
        app.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(app);
        finish();
        Runtime.getRuntime().exit(0);
    }
}
