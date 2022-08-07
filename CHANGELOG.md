# Bugdom-vita changelog

- **1.3.2-vita.2 (August 7, 2022)**
    - Get rid of BGRA-RGBA pixel conversion code in Bugdom and Pomme, moving it to a vitaGL fork. Future maintenance should be much easier.
    - Fix 1-byte offset bug making textures look grainy.

- **1.3.2-vita.1 (July 16, 2022)**
    - Initial Vita port.

# Bugdom changelog

- **1.3.2 (June 27, 2022)**
    - Seamless terrain texturing
    - Mouse-driven menus can now be navigated with a controller
    - Increase joystick deadzone to prevent Rollie from drifting continuously with a badly-calibrated controller
    - Smoother mouse control for waterbugs and dragonflies
    - Add option to constrain the viewport to a 4:3 aspect ratio
    - Default window dimensions adapt to screen size
    - Bump SDL to 2.0.22
    - To make cool screenshots, you can now swivel the camera with `<` `>` while the game is paused
    - To toggle fullscreen mode, use Alt+Enter instead of F11
    - Minor bugfixes, including:
        - Fix rare occurrences of garbled screens after fading out
        - Fix rare crash in between levels during a long game session, especially on macOS
        - Fix minor fence collision bugs
        - Fix boss health bar sometimes showing incorrect value at start of level

- **1.3.1 (August 21, 2021)**
    - Full rewrite of the 3D renderer, replacing legacy QuickDraw 3D code. The game should work better on systems that had trouble running the previous version.
    - Cosmetic touchups on 3DMF model textures. Some textures could use UV clamping, and some had incorrect alpha (spider teeth, etc.).
    - Native Apple Silicon support in macOS build.
    - Extra settings: hide/show bottom bar, high/low level of detail.
    - Command-line options to control MSAA and screen resolution.
    - Bump SDL to 2.0.16.
    - Minor gameplay & presentation bugfixes.

- **1.3.0 (December 29, 2020)**
    - Major update so the game works on modern systems.
    - Support for arbitrary screen resolutions beyond 640x480.
    - Presentation enhancements (built-in save & settings dialogs, widescreen touchups, etc.).
    - Many bugfixes.
    - *Note: this is the last version of Bugdom that uses Quesa for rendering, i.e. the last version containing "true" QuickDraw 3D code.*

---

- **1.2.1 (June 2003)** *Last known version for Mac OS 8-9*

- **1.2 (Sept. 2002)** Maintenance update. Fixed / changed cheat codes so that the tilde key is not required. New automatic version checking feature.

- **1.1.5 (Feb. 2001)** Fixed a problem where the Save Game dialog would not appear for some people.

- **1.1.4 (Sept. 2000)** OS X helped to discover a memory thrashing bug which was probably causing problems for some people. Also recompiled with Codewarrior 6 which should fix some problems.

- **1.1.3 (June 2000)** Fixed a really bad memory corruption problem & tweaked fence collision code.. again.

- **1.1.2** Better volume control.

- **1.1.1 (March 2000)** Nothing worth mentioning, just a recompile.

- **1.1 (Jan. 2000)** Major internal modifications to the way memory is used. This should help work around some crashing and freezing problems which were caused by problems with OS 9 on some of the new iMacs. Fixed a problem with the keyboard controls being too sensitive on really fast Macs. Keyboard sensitivity should now be constant on all machines regardless of speed. Fixed problem where FireFlies could be killed with Buddy Bug while Rollie is being carried – this would result in player being dropped off in void areas. Also fixed problem where Rollie could get stuck inside of walls while riding the DragonFly.

- **1.0.4 (Nov. 1999)** Now uses Navigation Services for Saving and Opening saved game files. This version should also fix a crashing problem on the iBook when trying to open a saved game with Virtual Memory turned on.

- **1.0.3 (Nov. 1999)** Fixed some problems with Rollie going thru solid walls. Also, decreased sensitivity of +/- keys for changing volume. Powerbooks and new AV iMacs seemed to go from audio-off to audio-loud in one step, so the volume keys should now have less effect each time they are hit for finer volume control on those Macs.

- **1.0.2 (Oct. 1999)** The game is now more tolerant of non-ATI 3D cards. Any card which supports RAVE 1.6 and can do simultaneous 2D and 3D should work without incident.

- **1.0.1 (July/Aug? 1999)** Fixed targeting problem with Fire Flies on level 8. Better error reporting if QD3D is not installed properly. Nuts on level 4 don’t regenerate infinitely. Improved VRAM usage for Macs with only 2MB of VRAM. Fixed problem with score on Bonus screen not displaying all of the digits properly in some cases.

- **1.0 (July? 1999)** We shipped it!
