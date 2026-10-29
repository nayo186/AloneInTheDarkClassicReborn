///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Texts of the advanced options, shared by the launcher window and the
// in-game F1 window (UTF-8: compile the files that include it with /utf-8)
///////////////////////////////////////////////////////////////////////////////

#pragma once

enum class AdvancedText
{
    Title,
    FrameLimit,
    FrameLimitNone,
    FrameLimitHelp,
    VSync,
    VSyncHelp,
    AspectRatio,
    AspectRatioHelp,
    Aspect4x3,
    Aspect16x10,
    AspectStretched,
    Fullscreen,
    FullscreenHelp,
    Resolution,
    ResolutionHelp,
    ResolutionAuto,
    CustomBackgrounds,
    CustomBackgroundsHelp,
    EnhancedText,
    EnhancedTextHelp,
    ExportBackgrounds,
    ExportBackgroundsHelp,
    ExportWorking,
    ExportDone,
    ExportFailed,
    ExportMasks,
    ExportMasksHelp,
    ExportMasksDone,
    Effects,
    FilmGrain,
    FilmGrainHelp,
    Bloom,
    BloomHelp,
    Ssao,
    SsaoHelp,
    Antialiasing,
    AntialiasingHelp,
    AntialiasingOff,
    AnimationSmoothing,
    AnimationSmoothingHelp,
    Close,
    Controls,
    ControlsHelp,
    ControlsPressKey,
    ControlsPressPad,
    ControlsKeyboard,
    ControlsController,
    ControlsAdd,
    ControlsRemove,
    ControlsDefaults,
    ModernStick,
    ModernStickHelp,
    ActionUp,
    ActionDown,
    ActionLeft,
    ActionRight,
    ActionAction,
    ActionConfirm,
    ActionCancel,
    ActionRun,
    ActionOptions,
    Audio,
    MusicVolume,
    MusicVolumeHelp,
    SoundVolume,
    SoundVolumeHelp,
    Voices,
    VoicesHelp,
    VoicesEnglish,
    VoicesFrench,
    VoicesSpanish,
    VoicesItalian,
    VoicesGerman,
    Cheats,
    CheatsEnabled,
    CheatsEnabledHelp,
    InfiniteLife,
    InfiniteLifeHelp,
    Count
};

// Languages in the launcher's order: 0 English, 1 Spanish, 2 French,
// 3 Italian, 4 German
static const int kAdvancedTextLanguages = 5;

// Widths of the resolution option, always 4:3 (height = width * 3 / 4); 0 = automatic
static const int kAdvancedResolutions[] = { 0, 640, 1024, 1280, 1600, 1920 };
static const int kAdvancedResolutionCount = 6;

inline int advancedResolutionHeight(int width)
{
    return width * 3 / 4;
}

inline const char* advancedText(AdvancedText text, int language)
{
    static const char* const texts[(int)AdvancedText::Count][kAdvancedTextLanguages] =
    {
        // Title
        { "ADVANCED OPTIONS", "OPCIONES AVANZADAS", "OPTIONS AVANCÉES", "OPZIONI AVANZATE", "ERWEITERTE OPTIONEN" },
        // FrameLimit
        { "FPS", "FPS", "FPS", "FPS", "FPS" },
        // FrameLimitNone
        { "No limit", "Sin límite", "Sans limite", "Nessun limite", "Kein Limit" },
        // FrameLimitHelp
        { "Frames per second. The game speed does not change.",
          "Imágenes por segundo. La velocidad del juego no cambia.",
          "Images par seconde. La vitesse du jeu ne change pas.",
          "Fotogrammi al secondo. La velocità del gioco non cambia.",
          "Bilder pro Sekunde. Die Spielgeschwindigkeit ändert sich nicht." },
        // VSync
        { "VSync", "VSync", "VSync", "VSync", "VSync" },
        // VSyncHelp
        { "Avoids tearing.",
          "Evita cortes en la imagen.",
          "Évite les déchirures.",
          "Evita lo strappo dell'immagine.",
          "Vermeidet Tearing." },
        // AspectRatio
        { "Aspect ratio", "Relación de aspecto", "Format d'image", "Rapporto d'aspetto", "Seitenverhältnis" },
        // AspectRatioHelp
        { "4:3 is the original shape. Stretched: fullscreen only.",
          "4:3 es la forma original. Estirada: solo en pantalla completa.",
          "4:3 est le format d'origine. Étirée : plein écran seulement.",
          "4:3 è la forma originale. Allungata: solo a schermo intero.",
          "4:3 ist das Originalformat. Gestreckt: nur im Vollbild." },
        // Aspect4x3
        { "4:3 (original)", "4:3 (original)", "4:3 (original)", "4:3 (originale)", "4:3 (Original)" },
        // Aspect16x10
        { "16:10", "16:10", "16:10", "16:10", "16:10" },
        // AspectStretched
        { "Stretched", "Estirada", "Étirée", "Allungata", "Gestreckt" },
        // Fullscreen
        { "Fullscreen", "Pantalla completa", "Plein écran", "Schermo intero", "Vollbild" },
        // FullscreenHelp
        { "Also Alt+Enter or F11.",
          "También Alt+Intro o F11.",
          "Aussi Alt+Entrée ou F11.",
          "Anche Alt+Invio o F11.",
          "Auch Alt+Eingabe oder F11." },
        // Resolution
        { "Resolution", "Resolución", "Résolution", "Risoluzione", "Auflösung" },
        // ResolutionHelp
        { "Sharpness of the 3D models. Automatic uses your screen.",
          "Nitidez de los modelos 3D. Automática usa tu pantalla.",
          "Netteté des modèles 3D. Automatique utilise votre écran.",
          "Nitidezza dei modelli 3D. Automatica usa il tuo schermo.",
          "Schärfe der 3D-Modelle. Automatisch nutzt deinen Bildschirm." },
        // ResolutionAuto
        { "Automatic", "Automática", "Automatique", "Automatica", "Automatisch" },
        // CustomBackgrounds
        { "Custom backgrounds", "Fondos personalizados", "Décors personnalisés", "Sfondi personalizzati", "Eigene Hintergründe" },
        // CustomBackgroundsHelp
        { "Uses your own backgrounds and masks.",
          "Usa tus propios fondos y máscaras.",
          "Utilise vos propres décors et masques.",
          "Usa i tuoi sfondi e maschere.",
          "Nutzt eigene Hintergründe und Masken." },
        // EnhancedText
        { "Enhanced text", "Texto mejorado", "Texte amélioré", "Testo migliorato", "Verbesserter Text" },
        // EnhancedTextHelp
        { "Smoother, more legible letters.",
          "Letras más suaves y legibles.",
          "Lettres plus lisses et lisibles.",
          "Lettere più morbide e leggibili.",
          "Glattere, besser lesbare Buchstaben." },
        // ExportBackgrounds
        { "Export backgrounds", "Exportar fondos", "Exporter les décors", "Esporta sfondi", "Hintergründe exportieren" },
        // ExportBackgroundsHelp
        { "Saves every background as a PNG, to edit or enlarge.",
          "Guarda todos los fondos en PNG, para editarlos o ampliarlos.",
          "Enregistre tous les décors en PNG, à modifier ou agrandir.",
          "Salva tutti gli sfondi in PNG, da modificare o ampliare.",
          "Speichert alle Hintergründe als PNG zum Bearbeiten oder Vergrößern." },
        // ExportWorking
        { "Exporting backgrounds...", "Exportando fondos...", "Export des décors...", "Esportazione sfondi...", "Hintergründe werden exportiert..." },
        // ExportDone
        { "Backgrounds exported",
          "Fondos exportados",
          "Décors exportés",
          "Sfondi esportati",
          "Hintergründe exportiert" },
        // ExportFailed
        { "The backgrounds could not be exported (see tatou.log)", "No se pudieron exportar los fondos (ver tatou.log)",
          "Impossible d'exporter les décors (voir tatou.log)", "Impossibile esportare gli sfondi (vedi tatou.log)",
          "Hintergründe konnten nicht exportiert werden (siehe tatou.log)" },
        // ExportMasks
        { "Export masks", "Exportar máscaras", "Exporter les masques", "Esporta maschere", "Masken exportieren" },
        // ExportMasksHelp
        { "Saves every mask (the parts of the background that cover the characters) as a PNG.",
          "Guarda todas las máscaras (partes del fondo que tapan a los personajes) en PNG.",
          "Enregistre tous les masques (parties du décor qui cachent les personnages) en PNG.",
          "Salva tutte le maschere (parti dello sfondo che coprono i personaggi) in PNG.",
          "Speichert alle Masken (Teile des Hintergrunds, die die Figuren verdecken) als PNG." },
        // ExportMasksDone
        { "masks exported",
          "máscaras exportadas",
          "masques exportés",
          "maschere esportate",
          "Masken exportiert" },
        // Effects
        { "Effects", "Efectos", "Effets", "Effetti", "Effekte" },
        // FilmGrain
        { "Film Grain", "Grano de película", "Grain de film", "Grana della pellicola", "Filmkörnung" },
        // FilmGrainHelp
        { "Fine moving noise over the image, like an old film.",
          "Ruido fino y animado sobre la imagen, como una película antigua.",
          "Bruit fin et animé sur l'image, comme un vieux film.",
          "Rumore fine e animato sull'immagine, come una vecchia pellicola.",
          "Feines, bewegtes Rauschen über dem Bild, wie bei einem alten Film." },
        // Bloom
        { "Bloom", "Bloom", "Bloom", "Bloom", "Bloom" },
        // BloomHelp
        { "Bright areas glow with a soft halo.",
          "Las zonas claras brillan con un halo suave.",
          "Les zones claires brillent avec un halo doux.",
          "Le zone chiare brillano con un alone morbido.",
          "Helle Bereiche leuchten mit einem weichen Schein." },
        // Ssao
        { "Ambient Occlusion (SSAO)", "Oclusión ambiental (SSAO)", "Occlusion ambiante (SSAO)",
          "Occlusione ambientale (SSAO)", "Umgebungsverdeckung (SSAO)" },
        // SsaoHelp
        { "Shades the folds and corners of the 3D characters and objects.",
          "Sombrea los pliegues y rincones de los personajes y objetos 3D.",
          "Ombre les plis et les recoins des personnages et objets 3D.",
          "Ombreggia pieghe e angoli dei personaggi e degli oggetti 3D.",
          "Schattiert Falten und Ecken der 3D-Figuren und -Objekte." },
        // Antialiasing
        { "Antialiasing", "Suavizado (antialiasing)", "Anticrénelage", "Antialiasing", "Kantenglättung" },
        // AntialiasingHelp
        { "Smooths the edges of the 3D models.",
          "Suaviza los bordes de los modelos 3D.",
          "Adoucit les bords des modèles 3D.",
          "Ammorbidisce i bordi dei modelli 3D.",
          "Glättet die Kanten der 3D-Modelle." },
        // AntialiasingOff
        { "Off", "Desactivado", "Désactivé", "Disattivato", "Aus" },
        // AnimationSmoothing
        { "Animation Smoothing", "Suavizado de animaciones", "Lissage des animations",
          "Animazioni fluide", "Animationsglättung" },
        // AnimationSmoothingHelp
        { "Smoother animations.",
          "Animaciones más fluidas.",
          "Animations plus fluides.",
          "Animazioni più fluide.",
          "Flüssigere Animationen." },
        // Close
        { "CLOSE", "CERRAR", "FERMER", "CHIUDI", "SCHLIESSEN" },
        // Controls
        { "CONTROLS", "CONTROLES", "COMMANDES", "COMANDI", "STEUERUNG" },
        // ControlsHelp
        { "Click an entry to remove it.",
          "Pulsa una entrada para quitarla.",
          "Cliquez sur une entrée pour la retirer.",
          "Clicca su una voce per rimuoverla.",
          "Klicke einen Eintrag an, um ihn zu entfernen." },
        // ControlsPressKey
        { "Press a key for: %s  (Esc: cancel)",
          "Pulsa una tecla para: %s  (Esc: cancelar)",
          "Appuyez sur une touche pour : %s  (Échap : annuler)",
          "Premi un tasto per: %s  (Esc: annulla)",
          "Drücke eine Taste für: %s  (Esc: abbrechen)" },
        // ControlsPressPad
        { "Press a button, trigger or stick of the controller for: %s  (Esc: cancel)",
          "Pulsa un botón, gatillo o stick del mando para: %s  (Esc: cancelar)",
          "Appuyez sur un bouton, une gâchette ou un stick de la manette pour : %s  (Échap : annuler)",
          "Premi un pulsante, grilletto o stick del controller per: %s  (Esc: annulla)",
          "Drücke eine Taste, einen Trigger oder Stick des Controllers für: %s  (Esc: abbrechen)" },
        // ControlsKeyboard
        { "KEYBOARD", "TECLADO", "CLAVIER", "TASTIERA", "TASTATUR" },
        // ControlsController
        { "CONTROLLER", "MANDO", "MANETTE", "CONTROLLER", "CONTROLLER" },
        // ControlsAdd
        { "+ Add", "+ Añadir", "+ Ajouter", "+ Aggiungi", "+ Hinzufügen" },
        // ControlsRemove
        { "Remove", "Quitar", "Retirer", "Rimuovi", "Entfernen" },
        // ControlsDefaults
        { "RESTORE DEFAULT CONTROLS", "RESTAURAR CONTROLES POR DEFECTO", "RÉTABLIR LES COMMANDES PAR DÉFAUT",
          "RIPRISTINA COMANDI PREDEFINITI", "STANDARDSTEUERUNG WIEDERHERSTELLEN" },
        // ModernStick
        { "Modern left stick", "Stick izquierdo moderno", "Stick gauche moderne", "Levetta sinistra moderna",
          "Moderner linker Stick" },
        // ModernStickHelp
        { "The character walks where you push the stick, seen from the camera. The d-pad and the keys stay tank controls.",
          "El personaje camina hacia donde empujas el stick, según la cámara. La cruceta y el teclado siguen tipo tanque.",
          "Le personnage marche là où vous poussez le stick, vu de la caméra. La croix et le clavier restent en mode tank.",
          "Il personaggio cammina dove spingi la levetta, rispetto alla telecamera. Croce e tastiera restano a carro armato.",
          "Die Figur läuft dorthin, wohin du den Stick drückst, von der Kamera aus gesehen. Steuerkreuz und Tastatur bleiben Panzersteuerung." },
        // ActionUp
        { "Forward", "Avanzar", "Avancer", "Avanti", "Vorwärts" },
        // ActionDown
        { "Back", "Retroceder", "Reculer", "Indietro", "Rückwärts" },
        // ActionLeft
        { "Turn left", "Girar a la izquierda", "Tourner à gauche", "Girare a sinistra", "Links drehen" },
        // ActionRight
        { "Turn right", "Girar a la derecha", "Tourner à droite", "Girare a destra", "Rechts drehen" },
        // ActionAction
        { "Action", "Acción", "Action", "Azione", "Aktion" },
        // ActionConfirm
        { "Confirm / inventory", "Confirmar / inventario", "Valider / inventaire", "Conferma / inventario", "Bestätigen / Inventar" },
        // ActionCancel
        { "Cancel / pause", "Cancelar / pausa", "Annuler / pause", "Annulla / pausa", "Abbrechen / Pause" },
        // ActionRun
        { "Run", "Correr", "Courir", "Correre", "Rennen" },
        // ActionOptions
        { "Options window", "Ventana de opciones", "Fenêtre des options", "Finestra delle opzioni", "Optionsfenster" },
        // Audio
        { "AUDIO", "AUDIO", "AUDIO", "AUDIO", "AUDIO" },
        // MusicVolume
        { "Music", "Música", "Musique", "Musica", "Musik" },
        // MusicVolumeHelp
        { "CD or AdLib music.",
          "Música del CD o AdLib.",
          "Musique du CD ou AdLib.",
          "Musica del CD o AdLib.",
          "CD- oder AdLib-Musik." },
        // SoundVolume
        { "Voices and effects", "Voces y efectos", "Voix et effets", "Voci ed effetti", "Stimmen und Effekte" },
        // SoundVolumeHelp
        { "Narrator and sound effects.",
          "Narrador y efectos de sonido.",
          "Narrateur et effets sonores.",
          "Narratore ed effetti sonori.",
          "Erzähler und Soundeffekte." },
        // Voices
        { "Narrator", "Narrador", "Narrateur", "Narratore", "Erzähler" },
        // VoicesHelp
        { "Voice that reads the books. Others: folders in UserData\\AITD1\\Voices (see the README).",
          "Voz que lee los libros. Otras: carpetas en UserData\\AITD1\\Voices (ver el README).",
          "Voix qui lit les livres. Autres : dossiers dans UserData\\AITD1\\Voices (voir le README).",
          "Voce che legge i libri. Altre: cartelle in UserData\\AITD1\\Voices (vedi il README).",
          "Stimme, die die Bücher liest. Weitere: Ordner in UserData\\AITD1\\Voices (siehe README)." },
        // VoicesEnglish
        { "English", "Inglés", "Anglais", "Inglese", "Englisch" },
        // VoicesFrench
        { "French", "Francés", "Français", "Francese", "Französisch" },
        // VoicesSpanish
        { "Spanish", "Español", "Espagnol", "Spagnolo", "Spanisch" },
        // VoicesItalian
        { "Italian", "Italiano", "Italien", "Italiano", "Italienisch" },
        // VoicesGerman
        { "German", "Alemán", "Allemand", "Tedesco", "Deutsch" },
        // Cheats
        { "CHEATS", "TRUCOS", "ASTUCES", "TRUCCHI", "CHEATS" },
        // CheatsEnabled
        { "Enable cheats", "Activar trucos", "Activer les astuces", "Attiva trucchi", "Cheats aktivieren" },
        // CheatsEnabledHelp
        { "Turns every cheat on or off.",
          "Enciende o apaga todos los trucos.",
          "Active ou désactive toutes les astuces.",
          "Attiva o disattiva tutti i trucchi.",
          "Schaltet alle Cheats ein oder aus." },
        // InfiniteLife
        { "Infinite life", "Vida infinita", "Vie infinie", "Vita infinita", "Unendliches Leben" },
        // InfiniteLifeHelp
        { "Damage never kills you; the game's traps still do.",
          "El daño nunca te mata; las trampas del juego sí.",
          "Les dégâts ne vous tuent jamais ; les pièges du jeu, si.",
          "I danni non ti uccidono mai; le trappole del gioco sì.",
          "Schaden tötet dich nie, die Fallen des Spiels schon." },
    };
    if (language < 0 || language >= kAdvancedTextLanguages)
        language = 0;
    return texts[(int)text][language];
}
