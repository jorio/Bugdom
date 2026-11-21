# Shared Pangea browser runtime

This directory is kept byte-for-byte identical in Bugdom, Bugdom2, Nanosaur and
Nanosaur2. It is intentionally vendored: each upstream repository can build on
its own, without depending on another game or a new published package. The
owner can extract this directory into a library later without untangling game
logic. Keep fixes here synchronized across the ports.

- `WebBuild.cmake` owns Emscripten flags, preloaded assets and shell copying.
- `platform.c` / `platform.h` provide latched actions, pointer and analog input,
  viewport sizing, master mute, save synchronization and the Asyncify yield.
- `gl_compat.c` / `gl_compat.h` adapt the subset of desktop OpenGL used by these
  games to Emscripten's legacy client-array emulation (tested with 4.0.20).
- `WebTexture.c` / `webtexture.h` perform independently testable pixel conversion.
- `controls.js`, `touch.js`, `style.css`, and `shell.html` implement the browser UI.

Game identity, labels, keyboard aliases and action buttons belong in
`web/game.js`. Native action mappings, camera behavior and gameplay state belong
in the game's `System/Web.c` and engine hooks. The stable `WEB_*` action IDs in
`platform.h` match the `actions` array in `controls.js`; append IDs rather than
reordering them. Each adapter implements `WebGameResize` and `WebEndFrame`.

Desktop builds do not compile this runtime. The web GL wrappers are enabled
only by `__EMSCRIPTEN__`. Keep the upstream renderer unchanged where a common
wrapper can handle the difference. Real-device and full-level visual testing
remain necessary; legacy GL emulation is not a complete desktop GL replacement.
