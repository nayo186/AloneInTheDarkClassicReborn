///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Texts of the advanced options, shared by the launcher window and the
// in-game F1 window (UTF-8: compile the files that include it with /utf-8)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdio>

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
    ResolutionOriginal,
    IntegerScale,
    IntegerScaleHelp,
    IntegerScaleOff,
    IntegerScaleAuto,
    IntegerSize,
    IntegerResolution,
    IntegerResolutionHelp,
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
    ExportSequences,
    ExportSequencesHelp,
    ExportSequencesDone,
    Effects,
    FilmGrain,
    FilmGrainHelp,
    Scanlines,
    ScanlinesHelp,
    ScanlineLevel,
    ScanlineLevelHelp,
    Bloom,
    BloomHelp,
    Ssao,
    SsaoHelp,
    Antialiasing,
    AntialiasingHelp,
    AntialiasingOff,
    Renderer,
    RendererHelp,
    RendererRestart,
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
    Vibration,
    VibrationHelp,
    ActionUp,
    ActionDown,
    ActionLeft,
    ActionRight,
    ActionAction,
    ActionConfirm,
    ActionCancel,
    ActionRun,
    ActionMap,
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
    MusicSource,
    MusicSourceHelp,
    MusicSourceCD,
    MusicSourceAdlib,
    Cheats,
    CheatsEnabled,
    CheatsEnabledHelp,
    InfiniteLife,
    InfiniteLifeHelp,
    // The controls window: its tabs and the controller drawing
    KeyboardTab,
    ControllerTab,
    PadHelp,
    PadNone,
    PadMove,
    PadAction,
    PadConfirm,
    PadCancel,
    PadOptions,
    PadLeftStick,
    PadLeftStickPress,
    PadRightStick,
    PadRightStickPress,
    PadDpad,
    PadView,
    PadMenu,
    PadCross,
    PadCircle,
    PadSquare,
    PadTriangle,
    Count
};

// Languages in the launcher's order: 0 English, 1 Spanish, 2 French,
// 3 Italian, 4 German
static const int kAdvancedTextLanguages = 5;

// Widths of the resolution option, 4:3 (height = width * 3 / 4); 0 = automatic.
// 320 is the games' own 320 x 200 (VGA mode 13h), stretched to the screen as
// in 1992: the 3D models on the same pixels as the backgrounds. 1440 is 4:3
// at 1080 lines, the height of a Full HD screen
static const int kAdvancedOriginalResolution = 320;
static const int kAdvancedResolutions[] = { 0, 320, 640, 1024, 1280, 1440, 1600, 1920 };
static const int kAdvancedResolutionCount = (int)(sizeof(kAdvancedResolutions) / sizeof(kAdvancedResolutions[0]));

inline int advancedResolutionHeight(int width)
{
    return width == kAdvancedOriginalResolution ? 200 : width * 3 / 4;
}

// Integer scaling (graphics.integerScale): every pixel of the game's
// 320 x 200 the same number of screen pixels wide and tall, black borders
// around. 0 off, -1 automatic (the largest that fits), N = the 200 lines N
// times (the largest that fits if it is too big). Aspect 16:10 (and
// Stretched) has square pixels: N x N. 4:3 has pixels 1.2 times taller than
// wide: the width is the nearest whole number, N / 1.2 (5 lines: 4, 1280 x
// 1000; 6: 5, 1600 x 1200). With it the normal resolution is not used: the
// 3D models follow graphics.integerResolution (integerResolutionImage).
static const int kIntegerScaleAuto = -1;
static const int kIntegerScaleMax = 21;   // 4320 lines, an 8K screen

inline void integerScaleSize(int aspect, int tall, int* width, int* height)
{
    const int wide = (aspect == 0) ? (tall * 5 + 3) / 6 : tall;
    *width = 320 * (wide < 1 ? 1 : wide);
    *height = 200 * tall;
}

// In 4:3, the sizes within 5% of its shape (from 5 lines up). The smaller
// ones (640 x 400, 960 x 600...) have the shape of 16:10, so the 4:3 list
// leaves them out (issue #7: they looked right and the 4:3 ones stretched);
// only Automatic in a small window falls back on them
inline bool integerScaleTrueShape(int aspect, int tall)
{
    if (aspect != 0)
        return true;
    const int wide = (tall * 5 + 3) / 6;
    const int error = 6 * wide - 5 * tall;
    return (error < 0 ? -error : error) * 4 <= tall;
}

// The lists offer only the sizes of the aspect ratio's shape that fit the
// screen (size unknown, 0: all that have its shape)
inline bool integerScaleOffered(int aspect, int tall, int screenWidth, int screenHeight)
{
    if (!integerScaleTrueShape(aspect, tall))
        return false;
    int width, height;
    integerScaleSize(aspect, tall, &width, &height);
    return screenWidth <= 0 || screenHeight <= 0 || (width <= screenWidth && height <= screenHeight);
}

// The size of the scene in a screen or window (area). Automatic: the largest
// that fits (Stretched fills the width with its own factor; in 4:3 the sizes
// of its true shape first, with none that fits any); a chosen size: that one
// or, if too big (a window), the largest smaller one that fits. False if off
// or not even 320 x 200 fits.
inline bool integerScaleResolve(int value, int aspect, int areaWidth, int areaHeight, int size[2])
{
    if (value == 0)
        return false;
    int tallest = areaHeight / 200;
    if (value > 0 && value < tallest)
        tallest = value;
    for (int pass = (value > 0) ? 1 : 0; pass < 2; pass++)
    {
        for (int tall = tallest; tall >= 1; tall--)
        {
            if (pass == 0 && !integerScaleTrueShape(aspect, tall))
                continue;
            integerScaleSize(aspect, tall, &size[0], &size[1]);
            if (value < 0 && aspect == 2)
                size[0] = 320 * (areaWidth / 320);
            if (size[0] >= 320 && size[0] <= areaWidth)
                return true;
        }
    }
    return false;
}

// The 3D image with integer scaling (graphics.integerResolution): 0 the whole
// size of the scene (the sharpest); N the 320 x 200 N times, enlarged by whole
// pixels (1 the original). N must divide both factors of the scene (in 4:3 the
// width and the height are enlarged differently); if not, the nearest lower
// one that does.
inline void integerResolutionImage(int value, int sceneWidth, int sceneHeight, int size[2])
{
    size[0] = sceneWidth;
    size[1] = sceneHeight;
    if (value <= 0)
        return;
    const int wide = sceneWidth / 320;
    const int tall = sceneHeight / 200;
    int lines = (wide < 1 || tall < 1) ? 1 : value;
    while (lines > 1 && (wide % lines != 0 || tall % lines != 0))
        lines--;
    size[0] = 320 * lines;
    size[1] = 200 * lines;
}

// A choice of the 3D resolution list for that scene: those that fit exactly,
// without repeating the whole size (image: the 3D image it gives)
inline bool integerResolutionOffered(int value, int sceneWidth, int sceneHeight, int image[2])
{
    integerResolutionImage(value, sceneWidth, sceneHeight, image);
    if (value <= 0)
        return true;
    return image[0] == 320 * value && image[1] == 200 * value &&
        !(image[0] == sceneWidth && image[1] == sceneHeight);
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
        { "4:3 is the shape it was drawn for. 16:10: square pixels, as in GOG/DOSBox. Stretched: fullscreen only.",
          "4:3 es la forma para la que se dibujó. 16:10: píxeles cuadrados, como en GOG/DOSBox. Estirada: solo en pantalla completa.",
          "4:3 est le format pour lequel il a été dessiné. 16:10 : pixels carrés, comme dans GOG/DOSBox. Étirée : plein écran seulement.",
          "4:3 è la forma per cui è stato disegnato. 16:10: pixel quadrati, come in GOG/DOSBox. Allungata: solo a schermo intero.",
          "4:3 ist das Format, für das es gezeichnet wurde. 16:10: quadratische Pixel, wie in GOG/DOSBox. Gestreckt: nur im Vollbild." },
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
        // ResolutionOriginal
        { "original", "original", "originale", "originale", "Original" },
        // IntegerScale
        { "Integer scaling", "Integer scaling", "Integer scaling", "Integer scaling", "Integer scaling" },
        // IntegerScaleHelp
        { "Every pixel the same size, black borders around.",
          "Todos los píxeles iguales, con bordes negros.",
          "Tous les pixels identiques, bords noirs autour.",
          "Tutti i pixel uguali, bordi neri intorno.",
          "Alle Pixel gleich groß, schwarze Ränder." },
        // IntegerScaleOff
        { "Off", "Desactivado", "Désactivé", "Disattivato", "Aus" },
        // IntegerScaleAuto
        { "Automatic", "Automático", "Automatique", "Automatico", "Automatisch" },
        // IntegerSize
        { "Size", "Tamaño", "Taille", "Dimensione", "Größe" },
        // IntegerResolution
        { "3D resolution", "Resolución 3D", "Résolution 3D", "Risoluzione 3D", "3D-Auflösung" },
        // IntegerResolutionHelp
        { "Sharpness of the 3D models; only those that keep every pixel equal.",
          "Nitidez de los modelos 3D; solo las que dejan todos los píxeles iguales.",
          "Netteté des modèles 3D ; seulement celles qui gardent les pixels égaux.",
          "Nitidezza dei modelli 3D; solo quelle che lasciano i pixel uguali.",
          "Schärfe der 3D-Modelle; nur solche mit gleich großen Pixeln." },
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
        // ExportSequences
        { "Export sequences", "Exportar secuencias", "Exporter les séquences", "Esporta sequenze", "Sequenzen exportieren" },
        // ExportSequencesHelp
        { "Saves every frame of the animated scenes as a PNG, to remake them.",
          "Guarda cada imagen de las escenas animadas en PNG, para rehacerlas.",
          "Enregistre chaque image des scènes animées en PNG, pour les refaire.",
          "Salva ogni immagine delle scene animate in PNG, per rifarle.",
          "Speichert jedes Bild der animierten Szenen als PNG zum Neugestalten." },
        // ExportSequencesDone
        { "frames exported",
          "imágenes exportadas",
          "images exportées",
          "immagini esportate",
          "Bilder exportiert" },
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
        // Scanlines
        { "Scanlines", "Scanlines", "Lignes de balayage", "Linee di scansione", "Scanlines" },
        // ScanlinesHelp
        { "Thin dark lines over the image, like an old CRT screen.",
          "Líneas finas y oscuras sobre la imagen, como una pantalla CRT antigua.",
          "Fines lignes sombres sur l'image, comme un vieil écran cathodique.",
          "Sottili linee scure sull'immagine, come un vecchio schermo a tubo catodico.",
          "Feine dunkle Linien über dem Bild, wie bei einem alten Röhrenbildschirm." },
        // ScanlineLevel
        { "Intensity", "Intensidad", "Intensité", "Intensità", "Stärke" },
        // ScanlineLevelHelp
        { "How dark the scanlines are, from 1 (soft) to 4 (strong).",
          "Lo oscuras que son las scanlines, de 1 (suaves) a 4 (marcadas).",
          "L'intensité des lignes, de 1 (douces) à 4 (marquées).",
          "Quanto sono scure le linee, da 1 (leggere) a 4 (marcate).",
          "Wie dunkel die Linien sind, von 1 (sanft) bis 4 (kräftig)." },
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
        // Renderer
        { "Graphics engine", "Motor gráfico", "Moteur graphique", "Motore grafico", "Grafik-Engine" },
        // RendererHelp
        { "How the game draws: change it if the picture looks wrong.",
          "Cómo dibuja el juego: cámbialo si la imagen se ve mal.",
          "Comment le jeu dessine : changez-le si l'image s'affiche mal.",
          "Come disegna il gioco: cambialo se l'immagine non si vede bene.",
          "Wie das Spiel zeichnet: ändern, wenn das Bild falsch aussieht." },
        // RendererRestart
        { "Applied when the game starts again.", "Se aplica al volver a abrir el juego.",
          "Appliqué au prochain lancement du jeu.", "Si applica al prossimo avvio del gioco.",
          "Gilt ab dem nächsten Spielstart." },
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
        // Vibration
        { "Controller vibration", "Vibración del mando", "Vibration de la manette", "Vibrazione del controller",
          "Controller-Vibration" },
        // VibrationHelp
        { "The controller vibrates a little when you are hit or shoot, and more when you die.",
          "El mando vibra un poco al recibir un golpe o al disparar, y más al morir.",
          "La manette vibre un peu quand vous êtes touché ou tirez, et plus quand vous mourez.",
          "Il controller vibra un po' quando vieni colpito o spari, e di più quando muori.",
          "Der Controller vibriert leicht, wenn du getroffen wirst oder schießt, und stärker, wenn du stirbst." },
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
        // ActionMap
        { "Map (Alone 3)", "Mapa (Alone 3)", "Plan (Alone 3)", "Mappa (Alone 3)", "Karte (Alone 3)" },
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
        // MusicSource
        { "Music type", "Tipo de música", "Type de musique", "Tipo di musica", "Musikart" },
        // MusicSourceHelp
        { "CD: recorded music. AdLib: the synthesized one of 1992; some places are silent, as on floppy disks.",
          "CD: música grabada. AdLib: la sintetizada de 1992; algunos lugares sin música, como en disquetes.",
          "CD : musique enregistrée. AdLib : celle synthétisée de 1992 ; certains lieux sans musique, comme sur disquettes.",
          "CD: musica registrata. AdLib: quella sintetizzata del 1992; alcuni luoghi senza musica, come su dischetti.",
          "CD: aufgenommene Musik. AdLib: die synthetisierte von 1992; manche Orte ohne Musik, wie auf Disketten." },
        // MusicSourceCD
        { "CD", "CD", "CD", "CD", "CD" },
        // MusicSourceAdlib
        { "AdLib", "AdLib", "AdLib", "AdLib", "AdLib" },
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
        // KeyboardTab
        { "Keyboard", "Teclado", "Clavier", "Tastiera", "Tastatur" },
        // ControllerTab
        { "Controller", "Mando", "Manette", "Controller", "Controller" },
        // PadHelp
        { "Click a button (or its text) to change what it does.", "Pulsa un botón (o su texto) para cambiar lo que hace.", "Cliquez sur un bouton (ou son texte) pour changer ce qu'il fait.", "Fai clic su un pulsante (o sul suo testo) per cambiare cosa fa.", "Klicke auf eine Taste (oder ihren Text), um zu ändern, was sie tut." },
        // PadNone
        { "Not used", "Sin uso", "Non utilisé", "Non usato", "Nicht belegt" },
        // PadMove
        { "Move", "Moverse", "Se déplacer", "Muoversi", "Bewegen" },
        // PadAction
        { "Action: use, fight, take", "Acción: usar, luchar, coger", "Action : utiliser, combattre, prendre", "Azione: usare, combattere, prendere", "Aktion: benutzen, kämpfen, nehmen" },
        // PadConfirm
        { "Inventory · accept", "Inventario · aceptar", "Inventaire · valider", "Inventario · conferma", "Inventar · bestätigen" },
        // PadCancel
        { "Back · pause menu", "Volver · menú de pausa", "Retour · menu pause", "Indietro · menu di pausa", "Zurück · Pausemenü" },
        // PadOptions
        { "Options (F1)", "Opciones (F1)", "Options (F1)", "Opzioni (F1)", "Optionen (F1)" },
        // PadLeftStick
        { "Left stick", "Stick izquierdo", "Stick gauche", "Stick sinistro", "Linker Stick" },
        // PadLeftStickPress
        { "Left stick (press)", "Pulsar stick izquierdo", "Stick gauche (appui)", "Stick sinistro (pressione)", "Linker Stick (drücken)" },
        // PadRightStick
        { "Right stick", "Stick derecho", "Stick droit", "Stick destro", "Rechter Stick" },
        // PadRightStickPress
        { "Right stick (press)", "Pulsar stick derecho", "Stick droit (appui)", "Stick destro (pressione)", "Rechter Stick (drücken)" },
        // PadDpad
        { "D-pad", "Cruceta", "Croix directionnelle", "Croce direzionale", "Steuerkreuz" },
        // PadView
        { "View", "Vista", "Affichage", "Visualizza", "Ansicht" },
        // PadMenu
        { "Menu", "Menú", "Menu", "Menu", "Menü" },
        // PadCross
        { "Cross", "Cruz", "Croix", "Croce", "Kreuz" },
        // PadCircle
        { "Circle", "Círculo", "Rond", "Cerchio", "Kreis" },
        // PadSquare
        { "Square", "Cuadrado", "Carré", "Quadrato", "Quadrat" },
        // PadTriangle
        { "Triangle", "Triángulo", "Triangle", "Triangolo", "Dreieck" },
    };
    if (language < 0 || language >= kAdvancedTextLanguages)
        language = 0;
    return texts[(int)text][language];
}

// The name of a resolution in the list: "Automatic", "320 × 200 (original)",
// "1280 × 960"... (language: 0 English, 1 Spanish, 2 French, 3 Italian, 4 German)
inline void advancedResolutionLabel(int width, int language, char* label, size_t size)
{
    if (width <= 0)
        snprintf(label, size, "%s", advancedText(AdvancedText::ResolutionAuto, language));
    else if (width == kAdvancedOriginalResolution)
        snprintf(label, size, "%d × %d (%s)", width, advancedResolutionHeight(width), advancedText(AdvancedText::ResolutionOriginal, language));
    else
        snprintf(label, size, "%d × %d", width, advancedResolutionHeight(width));
}

// The name of a 3D image of the list: "1280 × 1000", "640 × 400",
// "320 × 200 (original)"; "Automatic" while the scene size is not known
inline void integerResolutionLabel(const int image[2], int language, char* label, size_t size)
{
    if (image[0] <= 0 || image[1] <= 0)
        snprintf(label, size, "%s", advancedText(AdvancedText::ResolutionAuto, language));
    else if (image[0] == kAdvancedOriginalResolution && image[1] == 200)
        advancedResolutionLabel(kAdvancedOriginalResolution, language, label, size);
    else
        snprintf(label, size, "%d × %d", image[0], image[1]);
}

// The name of a choice of the list: "Off", "Automatic", "1280 × 1000"...
inline void integerScaleLabel(int value, int aspect, int language, char* label, size_t size)
{
    if (value == 0)
        snprintf(label, size, "%s", advancedText(AdvancedText::IntegerScaleOff, language));
    else if (value < 0)
        snprintf(label, size, "%s", advancedText(AdvancedText::IntegerScaleAuto, language));
    else
    {
        int width, height;
        integerScaleSize(aspect, value, &width, &height);
        snprintf(label, size, "%d × %d", width, height);
    }
}
