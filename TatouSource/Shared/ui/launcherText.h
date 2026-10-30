///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Texts of the launcher screen in English, Spanish, French, Italian and
// German (the language of the menus, not of the game). Header-only and in
// Shared/ so the game can show the same screen where there is no separate
// launcher (Android). UTF-8: compile with /utf-8.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>

enum class UiText
{
    WindowTitle,
    LaunchMenu,
    PlayAitd1,
    PlayAitd2,
    PlayAitd3,
    PlayJack,
    ComingSoon,
    Settings,
    Defaults,
    DefaultsHelp,
    DefaultsChanged,
    GameLanguage,
    NoLanguageFound,
    DataFolder,
    Browse,
    DataFolderMissing,
    DataFolderInvalid,
    Exit,
    InterfaceLanguage,
    TatouMissing,
    LaunchFailed,
    Subtitle,
    JackMissing,
    InstallGog,
    InstallCopy,
    DebugMode,
    Update,
    UpdateTitle,
    UpdateChecking,
    UpdateLatest,
    UpdateCheckFailed,
    UpdateAvailable,
    UpdateInstall,
    UpdateLater,
    UpdateInstalling,
    UpdateDone,
    UpdateFailed,
    UpdateAccept,
    UpdateClose,
    Count
};

enum class UiLanguage
{
    English,
    Spanish,
    French,
    Italian,
    German,
    Count
};

// One row per UiText, one column per UiLanguage (EN, ES, FR, IT, DE)
static const char* const kLauncherTexts[(int)UiText::Count][(int)UiLanguage::Count] =
{
    // WindowTitle
    { "Alone in the Dark: Classic Reborn - Launcher", "Alone in the Dark: Classic Reborn - Lanzador",
      "Alone in the Dark: Classic Reborn - Lanceur", "Alone in the Dark: Classic Reborn - Avvio",
      "Alone in the Dark: Classic Reborn - Starter" },
    // LaunchMenu
    { "Launch Menu", "Menú de inicio", "Menu de lancement", "Menu di avvio", "Startmenü" },
    // PlayAitd1
    { "PLAY ALONE IN THE DARK", "JUGAR ALONE IN THE DARK", "JOUER À ALONE IN THE DARK",
      "GIOCA AD ALONE IN THE DARK", "ALONE IN THE DARK SPIELEN" },
    // PlayAitd2
    { "PLAY ALONE IN THE DARK 2", "JUGAR ALONE IN THE DARK 2", "JOUER À ALONE IN THE DARK 2",
      "GIOCA AD ALONE IN THE DARK 2", "ALONE IN THE DARK 2 SPIELEN" },
    // PlayAitd3
    { "PLAY ALONE IN THE DARK 3", "JUGAR ALONE IN THE DARK 3", "JOUER À ALONE IN THE DARK 3",
      "GIOCA AD ALONE IN THE DARK 3", "ALONE IN THE DARK 3 SPIELEN" },
    // PlayJack
    { "PLAY JACK IN THE DARK", "JUGAR JACK IN THE DARK", "JOUER À JACK IN THE DARK",
      "GIOCA A JACK IN THE DARK", "JACK IN THE DARK SPIELEN" },
    // ComingSoon
    { "Coming soon", "Próximamente", "Bientôt disponible", "Prossimamente", "Demnächst" },
    // Settings
    { "Settings", "Configuración", "Configuration", "Configurazione", "Einstellungen" },
    // Defaults
    { "DEFAULT SETTINGS", "VALORES POR DEFECTO", "VALEURS PAR DÉFAUT", "VALORI PREDEFINITI", "STANDARDWERTE" },
    // DefaultsHelp
    { "The 1992 game with fixes, no extras", "El juego de 1992 con arreglos, sin añadidos",
      "Le jeu de 1992 corrigé, sans ajouts", "Il gioco del 1992 corretto, senza aggiunte",
      "Das Spiel von 1992 mit Korrekturen, ohne Extras" },
    // DefaultsChanged
    { "Options changed: click to restore", "Opciones cambiadas: pulsa para restaurar",
      "Options modifiées : cliquez pour rétablir", "Opzioni modificate: clic per ripristinare",
      "Optionen geändert: klicken zum Zurücksetzen" },
    // GameLanguage
    { "Game language", "Idioma del juego", "Langue du jeu", "Lingua del gioco", "Spielsprache" },
    // NoLanguageFound
    { "No language files found", "No se encontraron idiomas", "Aucune langue trouvée",
      "Nessuna lingua trovata", "Keine Sprachdateien gefunden" },
    // DataFolder
    { "Game data folder", "Carpeta de datos del juego", "Dossier des données du jeu",
      "Cartella dei dati del gioco", "Ordner der Spieldaten" },
    // Browse
    { "Browse...", "Examinar...", "Parcourir...", "Sfoglia...", "Durchsuchen..." },
    // DataFolderMissing
    { "Choose the folder that contains the game data",
      "Elige la carpeta que contiene los datos del juego",
      "Choisissez le dossier qui contient les données du jeu",
      "Scegli la cartella che contiene i dati del gioco",
      "Wähle den Ordner mit den Spieldaten" },
    // DataFolderInvalid
    { "Choose the GOG or Steam installation, or a copy of the data",
      "Elige la instalación de GOG o Steam, o una copia de los datos",
      "Choisissez l'installation GOG ou Steam, ou une copie des données",
      "Scegli l'installazione GOG o Steam, o una copia dei dati",
      "Wähle die GOG- oder Steam-Installation oder eine Datenkopie" },
    // Exit
    { "EXIT", "SALIR", "QUITTER", "ESCI", "BEENDEN" },
    // InterfaceLanguage
    { "Interface", "Interfaz", "Interface", "Interfaccia", "Oberfläche" },
    // TatouMissing
    { "Tatou.exe was not found next to the launcher", "No se encontró Tatou.exe junto al lanzador",
      "Tatou.exe est introuvable à côté du lanceur", "Tatou.exe non trovato accanto all'avvio",
      "Tatou.exe wurde neben dem Starter nicht gefunden" },
    // LaunchFailed
    { "The game could not be started (see launcher.log)", "No se pudo abrir el juego (ver launcher.log)",
      "Impossible de lancer le jeu (voir launcher.log)", "Impossibile avviare il gioco (vedi launcher.log)",
      "Das Spiel konnte nicht gestartet werden (siehe launcher.log)" },
    // Subtitle
    { "Survival Horror Classic", "Clásico del survival horror", "Classique du survival horror",
      "Classico del survival horror", "Survival-Horror-Klassiker" },
    // JackMissing
    { "Jack in the Dark not found (JACK folder)",
      "No se encontró Jack in the Dark (carpeta JACK)",
      "Jack in the Dark introuvable (dossier JACK)",
      "Jack in the Dark non trovato (cartella JACK)",
      "Jack in the Dark nicht gefunden (Ordner JACK)" },
    // InstallGog
    { "GOG/Steam installation",
      "Instalación de GOG/Steam",
      "Installation GOG/Steam",
      "Installazione GOG/Steam",
      "GOG/Steam-Installation" },
    // InstallCopy
    { "Copy of the game data", "Copia de los datos del juego", "Copie des données du jeu",
      "Copia dei dati del gioco", "Kopie der Spieldaten" },
    // DebugMode
    { "Debug mode", "Modo depuración", "Mode débogage", "Modalità debug", "Debug-Modus" },
    // Update
    { "Update", "Actualizar", "Mettre à jour", "Aggiorna", "Aktualisieren" },
    // UpdateTitle
    { "Updates", "Actualizaciones", "Mises à jour", "Aggiornamenti", "Updates" },
    // UpdateChecking
    { "Looking for a new version...", "Buscando una versión nueva...", "Recherche d'une nouvelle version...",
      "Ricerca di una nuova versione...", "Suche nach einer neuen Version..." },
    // UpdateLatest (%s: this version)
    { "You have the latest version (%s).", "Ya tienes la última versión (%s).", "Vous avez la dernière version (%s).",
      "Hai già l'ultima versione (%s).", "Sie haben die neueste Version (%s)." },
    // UpdateCheckFailed
    { "Could not look for updates (no connection?). See launcher.log.",
      "No se pudo buscar actualizaciones (¿sin conexión?). Ver launcher.log.",
      "Impossible de rechercher des mises à jour (pas de connexion ?). Voir launcher.log.",
      "Impossibile cercare aggiornamenti (nessuna connessione?). Vedi launcher.log.",
      "Die Suche nach Updates ist fehlgeschlagen (keine Verbindung?). Siehe launcher.log." },
    // UpdateAvailable (%s: the new version, %s: this one)
    { "A new version is available: %s (you have %s). Install it now? Your settings, saves and backgrounds are kept.",
      "Hay una versión nueva: %s (tienes la %s). ¿Instalarla ahora? Tus opciones, partidas y fondos se conservan.",
      "Une nouvelle version est disponible : %s (vous avez la %s). L'installer maintenant ? Vos options, sauvegardes et décors sont conservés.",
      "È disponibile una nuova versione: %s (hai la %s). Installarla ora? Opzioni, salvataggi e sfondi restano.",
      "Eine neue Version ist verfügbar: %s (Sie haben %s). Jetzt installieren? Einstellungen, Spielstände und Hintergründe bleiben erhalten." },
    // UpdateInstall
    { "Install", "Instalar", "Installer", "Installa", "Installieren" },
    // UpdateLater
    { "Not now", "Ahora no", "Plus tard", "Non ora", "Nicht jetzt" },
    // UpdateInstalling (%s: the new version)
    { "Downloading and installing %s...", "Descargando e instalando la %s...", "Téléchargement et installation de la %s...",
      "Download e installazione della %s...", "%s wird heruntergeladen und installiert..." },
    // UpdateDone (%s: the new version)
    { "Updated to %s. The launcher will close: open it again.",
      "Actualizado a la %s. El lanzador se cerrará: vuelve a abrirlo.",
      "Mis à jour vers la %s. Le lanceur va se fermer : ouvrez-le à nouveau.",
      "Aggiornato alla %s. Il programma di avvio si chiuderà: riaprilo.",
      "Auf %s aktualisiert. Der Starter wird geschlossen: öffnen Sie ihn erneut." },
    // UpdateFailed
    { "The update could not be installed; nothing was changed. See launcher.log.",
      "No se pudo instalar la actualización; no se cambió nada. Ver launcher.log.",
      "La mise à jour n'a pas pu être installée ; rien n'a été modifié. Voir launcher.log.",
      "Impossibile installare l'aggiornamento; non è stato cambiato nulla. Vedi launcher.log.",
      "Das Update konnte nicht installiert werden; nichts wurde geändert. Siehe launcher.log." },
    // UpdateAccept
    { "Accept", "Aceptar", "Accepter", "Accetta", "OK" },
    // UpdateClose
    { "Close", "Cerrar", "Fermer", "Chiudi", "Schließen" },
};

inline UiLanguage& currentUiLanguage()
{
    static UiLanguage language = UiLanguage::English;
    return language;
}

// Text in the current interface language (UTF-8)
inline const char* tr(UiText text)
{
    return kLauncherTexts[(int)text][(int)currentUiLanguage()];
}

inline UiLanguage uiLanguage()
{
    return currentUiLanguage();
}

inline void setUiLanguage(UiLanguage language)
{
    currentUiLanguage() = language;
}

// Native name of a language ("English", "Español"...) and its config code
inline const char* uiLanguageName(UiLanguage language)
{
    static const char* const names[(int)UiLanguage::Count] = { "English", "Español", "Français", "Italiano", "Deutsch" };
    return names[(int)language];
}

inline const char* uiLanguageCode(UiLanguage language)
{
    static const char* const codes[(int)UiLanguage::Count] = { "en", "es", "fr", "it", "de" };
    return codes[(int)language];
}

inline UiLanguage uiLanguageFromCode(const char* code)
{
    for (int i = 0; i < (int)UiLanguage::Count; i++)
        if (code && strcmp(code, uiLanguageCode((UiLanguage)i)) == 0)
            return (UiLanguage)i;
    return UiLanguage::English;
}
