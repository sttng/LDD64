# preferences.ini

Every setting LDD reads from or writes to `preferences.ini`, as found in the restored code (the
C++ sources and the Lua layouts of the assets). The settings under "LDD64 additions" are new in
LDD64; all others behave as in LDD 4.3.

## Where the values come from

LDD looks a key up in four places and takes the first value it finds:

1. the command line: `LDD64.exe -Key value` (a trailing `-Key` gives an empty value);
2. the user file, `%APPDATA%\LEGO Company\LEGO Digital Designer\preferences.ini` (Mac:
   `~/Library/Application Support/LEGO Company/LEGO Digital Designer/preferences.ini`; Linux:
   `~/.local/share/LEGO Company/LEGO Digital Designer/preferences.ini`);
3. the shipped file, `preferences.ini` in the program's resource folder (next to `LDD64.exe` on
   Windows, `ldd64` on Linux, in the bundle's `Contents/Resources` on macOS);
4. the built-in defaults (`SetDefaultPreferences`, LDDMain.cpp).

LDD writes only to the user file: the Preferences dialog, the window geometry, the recent models
and the bookkeeping keys below. A file is a list of `Key=value` lines without sections.

- Keys are case-insensitive: `PLAYSOUND` and `PlaySound` are the same setting. (The code spells
  several keys in two ways; both are listed once below.)
- Yes/no values accept `1` / `0`, `yes` / `no` and `true` / `false`, in any case. LDD writes
  `yes` / `no`.
- *dialog* marks a setting of Edit > Preferences; *restart* one that takes effect at the next
  start.

## Start-up and online services

LEGO's servers no longer answer, so the online features below cannot work any more.

| Key | Default | Values | What it does |
|---|---|---|---|
| `DoServerCall` | `1` | yes/no | Contact LEGO's server at start (version check, brick and palette downloads). With `no` LDD starts offline and skips the "Internet access failed" message. |
| `DoStartupChecks` | `1` | yes/no | Run the start screen (splash, brick installation, server calls) and the version and clean-up checks. With `no` the editor opens at once; the brick database must already be installed. |
| `IgnorePatching` | `0` | yes/no | Do not offer a newer version that the server reports. |
| `ForcePatching` | `0` | yes/no | Download the newest version at the next online start; LDD sets it back to `no` right away. |
| `initialiseDesignerURL` | `http://ldd.lego.com/service/integration.asmx/initialiseDesigner` | URL | The start-up web service. |
| `BrickPricesFileURL` | (none) | URL | The brick price list of the online shop features. |
| `ShowStartHelp` | `1` | yes/no | Show the start help over the editor when it opens. |
| `LoadMostRecentModel` | `0` | yes/no | Open the most recently used model at start (instead of the start help). |
| `Open` | (none) | file | Windows: a model to open once LDD has started, normally given on the command line. |
| `AllowMultipleLDDInstances` | `0` | yes/no | Windows: a second start normally brings the running LDD to the front and quits; `yes` starts another LDD. |
| `UnpackBricks` | `0` | yes/no | Install the brick database as files in the user folder instead of reading it from the `db.lif` archive in memory. |
| `LoadAssemblies` | `1` | yes/no | Load the assemblies (bricks made of several parts) of the brick database. |
| `language` | `en` | language code | Language of the user interface. *dialog* (Windows: Language). |

## Editor and user interface

| Key | Default | Values | What it does |
|---|---|---|---|
| `SHOWINFOFIELD` | `1` | yes/no | Show the info bar of the editor. *dialog* |
| `SHOWTOOLTIPS` | `1` | yes/no | Show tooltips. *dialog* |
| `PlaySound` | `1` | yes/no | Play the interface sounds. *dialog* (when sound is available) |
| `ShowBrickReorientation` | `1` | yes/no | Show the brick reorientation help while carrying bricks. *dialog* |
| `repeatPaste` | `0` | yes/no | After pasted or cloned bricks are placed, take up another copy at once. *dialog* |
| `SHOWBRICKCOUNTS` | `1` | yes/no | Show the brick counts in the palette and the colour lists. *dialog* |
| `INVERTEDCAMERAX` | `0` | yes/no | Turn the camera the other way horizontally. *dialog* |
| `INVERTEDCAMERAY` | `0` | yes/no | Turn the camera the other way vertically. *dialog* |
| `ZoomToSelection` | `0` | yes/no | When the camera centres on a brick, also zoom to fit it. |
| `SortByIdOnly` | `0` | yes/no | Brick palette: sort by design number only, without the main groups. |
| `ShowHideSlideDuration` | `500` | milliseconds | Duration of the panels' slide-in and slide-out. |
| `BrickSizeScale`, `TemplateSizeScale`, `GroupSizeScale` | (none) | number | Icon size sliders of the palette tabs (written by the sliders). |
| `CameraControlRect` | (none) | `x y right bottom` | Place and size of the camera control panel (written by LDD). |

## Graphics

| Key | Default | Values | What it does |
|---|---|---|---|
| `UseShadersInScene` | `1` | yes/no | Shaders in the 3D view. *dialog*, *restart* |
| `UseShadersInPalette` | `1` | yes/no | Shaders in the palette. *dialog*, *restart* |
| `UseRoundEdges` | `0` | yes/no | Rounded edges (needs both shader settings). *dialog*, *restart* |
| `SelectedPostProcessingEffect` | `0` | number | The post-processing effect of the 3D view, by its number among the installed effects. *dialog* |
| `CompatibilityModeLevel` | `0` | 0 to 100, steps of 20 | Graphics compatibility ("safe mode"). Above 0 the start screen shows a warning. LDD sets it to 100 itself after a graphics failure. *dialog*, *restart* |
| `ShowCompatibilityModeWarningAtStartup` | `1` | yes/no | Show the safe-mode warning at start (its "don't show again" box sets `no`). |
| `TerrainHeightMap`, `TerrainTexture` | (none) | file names | A terrain under the model: height map and texture images in the assets folder (both needed). |

### LDD64 additions

| Key | Default | Values | What it does |
|---|---|---|---|
| `ViewDistance` | (none) = 1500 | 200 to 1,000,000 | Far clipping distance of the 3D view, in LDD units (1 = 1 cm); the zoom-out limit stays 100 below it. Other values are ignored. |
| `FastNavigation` | (none) = no | yes/no | While the camera moves over a model that draws slowly, show a box per mesh; still frames are unchanged. |
| `FastNavigationThreshold` | (none) = 50 | milliseconds | Draw time of a full frame from which `FastNavigation` takes over. |
| `ScaledWheelZoom` | (none) = no | yes/no | A mouse wheel notch moves the camera by 5 % of its distance to the target (at least the original 5 units). |
| `LevelOfDetail` | `0` | yes/no | Draw distant bricks without studs and tubes. *dialog* |
| `LevelOfDetailDistance` | `500` | 100 to 2000 | Distance from which bricks are simplified. *dialog* |
| `LevelOfDetailBoxes` | `0` | yes/no | Draw bricks beyond twice the distance as boxes. *dialog* |

## Files and folders

| Key | Default | Values | What it does |
|---|---|---|---|
| `UserModelDirectory` | `Documents/LEGO Creations/Models` | folder | Folder of the open and save dialogs (LDD updates it). |
| `UserBIDirectory` | `Documents/LEGO Creations/Building Instructions` | folder | Folder of the HTML building instructions (LDD updates it). |
| `userBrickDBDirName` | `db` | folder name | Folder of the installed brick database in the user data folder. |
| `userPalettesDirName` | `Palettes` | folder name | Folder of the downloaded palettes in the user data folder. |
| `PalettePath` | (empty) | folder | Load the palettes from this folder instead (and without the extended palette). |
| `UserPalettesPath` | `UserPalettes` | folder | Folder of the user's own palettes (in the user data folder, or a full path). |
| `LDrawMapFile` | `ldraw.xml` | file name | Part and colour map for LDraw import and export (LDD64 ships a current one). |
| `RecentModels` | (none) | list | The recently opened models (LDD keeps this list). |
| `mainWindowSize` | `1024 768` | `width height` | Size of the main window (LDD updates it). |
| `mainWindowOrigin` | `0 0` | `x y` | Position of the main window (LDD updates it). |
| `mainWindowMinSize` | `1024 768` | `width height` | Smallest size of the main window. |

## Developer and diagnostics

| Key | Default | Values | What it does |
|---|---|---|---|
| `DeveloperMode` | `0` | yes/no | Developer features: the Developer menu, LXFML and LDraw in the file dialogs, load statistics in the trace, and the brick tooltip settings in the dialog. |
| `QAMode` | `0` | yes/no | Shows the brick tooltip settings in the dialog. |
| `BRICKTOOLTIP` | `0` | yes/no | A tooltip with brick data in the 3D view. *dialog* (developer or QA mode) |
| `ShowExtendedBrickToolTip` | `0` | yes/no | Longer palette tooltips with brick data. *dialog* (developer or QA mode) |
| `verbose` | `0` | yes/no | Timing reports and verbose trace. It must be a valid yes/no value, or LDD does not start. |
| `VerboseTrace` | (none) | yes/no | Verbose trace output. |
| `TraceName` | (none) | name | Name of the trace output. |
| `dontshowassertbox` | (none) | yes/no | Failed assertions show no message box. |
| `dontshowassertboxcontinue` | (none) | yes/no | As above, and LDD carries on. |
| `CreateLargeStackDump` | (none) | yes/no | Windows: crash dumps with more of the memory. |

## Kept by LDD (not for editing)

| Key | What it is |
|---|---|
| `ApplicationID` | An ID made at the first start, sent with the server calls. |
| `ApplicationVersion` | The version that last ran; older files' `ApplicationVersionMajor`, `…Minor`, `…Revision` and `…Build` are replaced by it. |
| `PaletteNames` | The installed palettes and their versions. |
| `Cleanup` | Set while the start-up checks run and when the brick database turned out broken: the bricks are installed again at the next start. |
| `ImproperShutDown`, `UserBrickVersion` | Removed by LDD when found. |

## Defaults without a reader

`ThemeAccess` (empty) and `SubstitutionTransparency` (`0.5`) are among the built-in defaults, but
no code of LDD 4.3 reads them.
