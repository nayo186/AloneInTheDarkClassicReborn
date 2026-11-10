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
    About,
    AboutText,
    AboutAuthor,
    AboutCredits,
    AboutThanks,
    AboutGithub,
    ImportGame,
    ImportCopying,
    ImportDone,
    ImportFailed,
    UpdateConfirm,
    ImportLanguageDone,
    ImportNarratorDone,
    ImportPicturesDone,
    ImportNothing,
    InstallDisc,
    FilesUnknown,
    FilesUnknownTitle,
    FilesUnknownText,
    PlayAnyway,
    Cancel,
    FilesChecking,
    FilesMissing,
    FilesMissingTitle,
    FilesMissingText,
    FilesDontWarn,
    OptionsNotSavedTitle,
    OptionsNotSavedText,
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
    // About
    { "About", "Acerca de", "À propos", "Informazioni", "Über" },
    // AboutText
    { "A source port of the Alone in the Dark trilogy and Jack in the Dark for modern PCs, with fixes and new options.",
      "Un port del motor de la trilogía Alone in the Dark y de Jack in the Dark para PCs modernos, con arreglos y opciones nuevas.",
      "Un portage du moteur de la trilogie Alone in the Dark et de Jack in the Dark pour les PC modernes, avec des corrections et de nouvelles options.",
      "Un port del motore della trilogia Alone in the Dark e di Jack in the Dark per PC moderni, con correzioni e nuove opzioni.",
      "Eine Portierung der Engine der Alone-in-the-Dark-Trilogie und von Jack in the Dark für moderne PCs, mit Korrekturen und neuen Optionen." },
    // AboutAuthor (%s: the author's alias)
    { "Created by %s", "Creado por %s", "Créé par %s", "Creato da %s", "Erstellt von %s" },
    // AboutCredits
    { "Built on FITD by yaz0r, the reimplementation of the original engine that makes this project possible, with parts of Alone in the Dark Re-Haunted. Free software under the GPL v2. The games belong to their owners: you need your own copy.",
      "Basado en FITD de yaz0r, la reimplementación del motor original que hace posible este proyecto, con partes de Alone in the Dark Re-Haunted. Software libre con licencia GPL v2. Los juegos son de sus dueños: necesitas tu propia copia.",
      "Basé sur FITD de yaz0r, la réimplémentation du moteur original qui rend ce projet possible, avec des parties d'Alone in the Dark Re-Haunted. Logiciel libre sous licence GPL v2. Les jeux appartiennent à leurs propriétaires : il vous faut votre propre copie.",
      "Basato su FITD di yaz0r, la reimplementazione del motore originale che rende possibile questo progetto, con parti di Alone in the Dark Re-Haunted. Software libero con licenza GPL v2. I giochi appartengono ai loro proprietari: serve una copia propria.",
      "Basiert auf FITD von yaz0r, der Neuimplementierung der Original-Engine, die dieses Projekt möglich macht, mit Teilen von Alone in the Dark Re-Haunted. Freie Software unter der GPL v2. Die Spiele gehören ihren Eigentümern: Sie brauchen eine eigene Kopie." },
    // AboutThanks
    { "Thank you to everyone who tests it and reports problems: every version is better thanks to you.",
      "Gracias a todos los que lo prueban y reportan errores: cada versión mejora gracias a vosotros.",
      "Merci à tous ceux qui le testent et signalent des problèmes : chaque version s'améliore grâce à vous.",
      "Grazie a tutti quelli che lo provano e segnalano problemi: ogni versione migliora grazie a voi.",
      "Danke an alle, die es testen und Fehler melden: Jede Version wird dank euch besser." },
    // AboutGithub
    { "Open GitHub", "Abrir GitHub", "Ouvrir GitHub", "Apri GitHub", "GitHub öffnen" },
    // ImportGame (Android: instead of Browse; also languages, narrators and pictures)
    { "Import...", "Importar...", "Importer...", "Importa...", "Importieren..." },
    // ImportCopying (%d: percent)
    { "Copying the game... %d%%", "Copiando el juego... %d%%", "Copie du jeu... %d%%", "Copia del gioco... %d%%",
      "Spiel wird kopiert... %d%%" },
    // ImportDone
    { "Game imported.", "Juego importado.", "Jeu importé.", "Gioco importato.", "Spiel importiert." },
    // ImportFailed
    { "The game could not be copied. See launcher.log.", "No se pudo copiar el juego. Ver launcher.log.",
      "Le jeu n'a pas pu être copié. Voir launcher.log.", "Impossibile copiare il gioco. Vedi launcher.log.",
      "Das Spiel konnte nicht kopiert werden. Siehe launcher.log." },
    // UpdateConfirm (Android; %s: the new version)
    { "Android will ask you to update the app to %s: accept it. Your games, saves and settings are kept.",
      "Android te pedirá actualizar la app a la %s: acéptalo. Tus juegos, partidas y opciones se conservan.",
      "Android va vous demander de mettre à jour l'application vers la %s : acceptez. Vos jeux, sauvegardes et options sont conservés.",
      "Android ti chiederà di aggiornare l'app alla %s: accetta. Giochi, salvataggi e opzioni restano.",
      "Android wird Sie bitten, die App auf %s zu aktualisieren: Bestätigen Sie. Spiele, Spielstände und Einstellungen bleiben erhalten." },
    // ImportLanguageDone
    { "Language imported: choose it in Game language (Alone in the Dark 3).",
      "Idioma importado: elígelo en Idioma del juego (Alone in the Dark 3).",
      "Langue importée : choisissez-la dans Langue du jeu (Alone in the Dark 3).",
      "Lingua importata: sceglila in Lingua del gioco (Alone in the Dark 3).",
      "Sprache importiert: Wählen Sie sie unter Spielsprache (Alone in the Dark 3)." },
    // ImportNarratorDone
    { "Narrator imported: choose it in the game's options (F1, Alone in the Dark).",
      "Narrador importado: elígelo en las opciones del juego (F1, Alone in the Dark).",
      "Narrateur importé : choisissez-le dans les options du jeu (F1, Alone in the Dark).",
      "Narratore importato: sceglilo nelle opzioni del gioco (F1, Alone in the Dark).",
      "Erzähler importiert: Wählen Sie ihn in den Spieloptionen (F1, Alone in the Dark)." },
    // ImportPicturesDone
    { "Backgrounds and masks imported.", "Fondos y máscaras importados.", "Décors et masques importés.",
      "Sfondi e maschere importati.", "Hintergründe und Masken importiert." },
    // ImportNothing
    { "Nothing to import in that folder: choose the game's folder (GOG or Steam), a language, a narrator, or Backgrounds or Masks.",
      "No hay nada que importar en esa carpeta: elige la del juego (GOG o Steam), un idioma, un narrador, o Backgrounds o Masks.",
      "Rien à importer dans ce dossier : choisissez celui du jeu (GOG ou Steam), une langue, un narrateur, ou Backgrounds ou Masks.",
      "Niente da importare in quella cartella: scegli quella del gioco (GOG o Steam), una lingua, un narratore, o Backgrounds o Masks.",
      "In diesem Ordner gibt es nichts zu importieren: Wählen Sie den des Spiels (GOG oder Steam), eine Sprache, einen Erzähler oder Backgrounds oder Masks." },
    // InstallDisc
    { "Your CD (image)", "Tu CD (imagen)", "Votre CD (image)", "Il tuo CD (immagine)", "Ihre CD (Abbild)" },
    // FilesUnknown
    { "Some game files are in no edition we know (see launcher.log)",
      "Hay archivos del juego que no son de ninguna edición conocida (ver launcher.log)",
      "Des fichiers du jeu ne sont d'aucune édition connue (voir launcher.log)",
      "Alcuni file del gioco non sono di nessuna edizione conosciuta (vedi launcher.log)",
      "Einige Spieldateien gehören zu keiner bekannten Ausgabe (siehe launcher.log)" },
    // FilesUnknownTitle
    { "Game files we do not know", "Archivos del juego que no conocemos", "Des fichiers du jeu inconnus",
      "File del gioco che non conosciamo", "Spieldateien, die wir nicht kennen" },
    // FilesUnknownText
    { "Some game files are different from the editions we know (GOG, Steam and the CDs we have checked): they may be from another edition, changed or damaged. The game may not work well, freeze or close. If it is your original edition and something goes wrong, please tell us on GitHub which one you have (language, year) and send launcher.log and tatou.log: they list the files that differ, so we can make it work.",
      "Algunos archivos del juego son distintos de los de las ediciones que conocemos (GOG, Steam y los CD que hemos comprobado): pueden ser de otra edición, estar modificados o dañados. El juego podría no funcionar bien, quedarse trabado o cerrarse. Si es tu edición original y algo falla, cuéntanos en GitHub cuál tienes (idioma, año) y envía launcher.log y tatou.log: dicen qué archivos son distintos, para poder hacerla funcionar.",
      "Certains fichiers du jeu diffèrent de ceux des éditions que nous connaissons (GOG, Steam et les CD que nous avons vérifiés) : ils peuvent venir d'une autre édition, être modifiés ou endommagés. Le jeu peut mal fonctionner, se bloquer ou se fermer. Si c'est votre édition d'origine et qu'il y a un problème, dites-nous sur GitHub laquelle vous avez (langue, année) et envoyez launcher.log et tatou.log : ils indiquent les fichiers différents, pour que nous puissions la faire fonctionner.",
      "Alcuni file del gioco sono diversi da quelli delle edizioni che conosciamo (GOG, Steam e i CD che abbiamo verificato): possono essere di un'altra edizione, modificati o danneggiati. Il gioco potrebbe non funzionare bene, bloccarsi o chiudersi. Se è la tua edizione originale e qualcosa va storto, dicci su GitHub quale hai (lingua, anno) e invia launcher.log e tatou.log: elencano i file diversi, così potremo farla funzionare.",
      "Einige Spieldateien unterscheiden sich von den Ausgaben, die wir kennen (GOG, Steam und die CDs, die wir geprüft haben): Sie können aus einer anderen Ausgabe stammen, verändert oder beschädigt sein. Das Spiel funktioniert vielleicht nicht richtig, hängt sich auf oder schließt sich. Wenn es Ihre Originalausgabe ist und etwas schiefgeht, sagen Sie uns auf GitHub, welche Sie haben (Sprache, Jahr), und senden Sie launcher.log und tatou.log: Sie nennen die abweichenden Dateien, damit wir sie zum Laufen bringen können." },
    // PlayAnyway
    { "Play anyway", "Jugar igualmente", "Jouer quand même", "Gioca comunque", "Trotzdem spielen" },
    // Cancel
    { "Cancel", "Cancelar", "Annuler", "Annulla", "Abbrechen" },
    // FilesChecking
    { "Checking the game files...", "Comprobando los archivos del juego...", "Vérification des fichiers du jeu...",
      "Controllo dei file del gioco...", "Spieldateien werden geprüft..." },
    // FilesMissing (%d: how many)
    { "%d game files are missing (see launcher.log)", "Faltan %d archivos del juego (ver launcher.log)",
      "Il manque %d fichiers du jeu (voir launcher.log)", "Mancano %d file del gioco (vedi launcher.log)",
      "%d Spieldateien fehlen (siehe launcher.log)" },
    // FilesMissingTitle
    { "Game files are missing", "Faltan archivos del juego", "Il manque des fichiers du jeu",
      "Mancano file del gioco", "Es fehlen Spieldateien" },
    // FilesMissingText
    { "Files the game needs are not in this folder. When the game needs them it will close or freeze. Copy the game's folder again from your installation or your CD.",
      "En esta carpeta faltan archivos que el juego necesita. Cuando el juego los necesite se cerrará o se quedará trabado. Vuelve a copiar la carpeta del juego desde tu instalación o tu CD.",
      "Des fichiers dont le jeu a besoin manquent dans ce dossier. Quand le jeu en aura besoin, il se fermera ou se bloquera. Copiez à nouveau le dossier du jeu depuis votre installation ou votre CD.",
      "In questa cartella mancano file di cui il gioco ha bisogno. Quando il gioco ne avrà bisogno si chiuderà o si bloccherà. Copia di nuovo la cartella del gioco dalla tua installazione o dal tuo CD.",
      "In diesem Ordner fehlen Dateien, die das Spiel braucht. Wenn das Spiel sie braucht, schließt es sich oder hängt sich auf. Kopieren Sie den Ordner des Spiels erneut aus Ihrer Installation oder von Ihrer CD." },
    // FilesDontWarn
    { "Do not warn again for these files", "No volver a avisar para estos archivos", "Ne plus avertir pour ces fichiers",
      "Non avvisare più per questi file", "Für diese Dateien nicht mehr warnen" },
    // OptionsNotSavedTitle
    { "Your options could not be saved", "No se pudieron guardar tus opciones", "Vos options n'ont pas pu être enregistrées",
      "Non è stato possibile salvare le tue opzioni", "Ihre Optionen konnten nicht gespeichert werden" },
    // OptionsNotSavedText
    { "The launcher could not write the file below. Its folder may be read-only or protected, for example by an antivirus or by Windows' controlled folder access. The game will start with the options it had before, and your saves may not be kept either. To fix it, move the game's folder to another place (for example C:\\Games) or allow it in the antivirus.",
      "El lanzador no pudo escribir el archivo de abajo. Su carpeta puede ser de solo lectura o estar protegida, por ejemplo por un antivirus o por el acceso controlado a carpetas de Windows. El juego empezará con las opciones que tenía, y puede que tus partidas tampoco se guarden. Para arreglarlo, mueve la carpeta del juego a otro sitio (por ejemplo C:\\Juegos) o permítela en el antivirus.",
      "Le lanceur n'a pas pu écrire le fichier ci-dessous. Son dossier est peut-être en lecture seule ou protégé, par exemple par un antivirus ou par l'accès contrôlé aux dossiers de Windows. Le jeu démarrera avec les options qu'il avait, et vos sauvegardes pourraient ne pas être gardées non plus. Pour corriger cela, déplacez le dossier du jeu ailleurs (par exemple C:\\Jeux) ou autorisez-le dans l'antivirus.",
      "L'avvio non è riuscito a scrivere il file qui sotto. La sua cartella potrebbe essere di sola lettura o protetta, per esempio da un antivirus o dall'accesso controllato alle cartelle di Windows. Il gioco partirà con le opzioni che aveva, e anche i salvataggi potrebbero non essere conservati. Per risolvere, sposta la cartella del gioco in un altro posto (per esempio C:\\Giochi) o consentila nell'antivirus.",
      "Der Starter konnte die Datei unten nicht schreiben. Ihr Ordner ist vielleicht schreibgeschützt oder geschützt, zum Beispiel durch ein Antivirenprogramm oder den überwachten Ordnerzugriff von Windows. Das Spiel startet mit den bisherigen Optionen, und auch Ihre Spielstände werden vielleicht nicht gespeichert. Verschieben Sie den Ordner des Spiels an einen anderen Ort (zum Beispiel C:\\Spiele) oder erlauben Sie ihn im Antivirenprogramm." },
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
