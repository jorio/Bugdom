# Experimental mobile web port

The reusable browser layer lives in [`common/`](common/README.md), with game-specific configuration in `game.js` and the C engine adapter.

This builds the original C/C++ game as WebAssembly with SDL3 and WebGL 1.
Desktop builds retain their existing renderer and input path.

## Build

Install CMake 3.21+ and [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).
The compatibility layer is tested with **Emscripten 4.0.20** and **SDL 3.2.8**.

```sh
git submodule update --init --recursive
git clone --depth 1 --branch release-3.2.8 https://github.com/libsdl-org/SDL.git extern/SDL
# In your emsdk checkout:
./emsdk install 4.0.20
./emsdk activate 4.0.20
source ./emsdk_env.sh
# Back in the Bugdom checkout:
bash web/build.sh
python3 web/serve.py
```

Open <http://localhost:8000>. For a phone on the same Wi-Fi, replace `localhost`
with the development computer’s LAN IP (on a Mac, `ipconfig getifaddr en0`).
The Android emulator normally reaches the host at <http://10.0.2.2:8000>.
The iOS Simulator can use `localhost`. Allow incoming connections if the host
firewall prompts. The server serves `build-web` only, with caching disabled.
Rebuild after C/C++ changes, then reload the browser. Rerunning the build also
copies changes to the CSS and controls script. This is not a hot-reloading server.

## Controls

- Tap **Play Bugdom** to start (also supplies a user gesture for browser audio).
- Tap the original game's menu icons. **Continue** skips title/intro screens.
- The BlockWorld-style floating thumbstick appears wherever your left thumb
  lands. It supports analog movement, diagonals and a small center dead zone.
  Drag the right side to orbit/tilt the camera; pinch with two fingers to zoom.
  A pair placed together can pinch across both halves; once the left thumb is
  moving, the right thumb independently controls the camera.
- Round **Kick**, **Jump**, **Roll**, and **Buddy** buttons form a right-edge
  column, with Kick at the top. Movement, camera and held actions can be used together. The top-right
  round buttons toggle touch controls, all sound, fullscreen, and pause. Mute
  remembers its setting in this browser. Controls appear
  during gameplay and release on pause, tab switching, resize or cancellation.
- Keyboard: WASD/arrows to move, J to jump, K to kick, Space to roll, Tab for buddy,
  comma/period to rotate camera, 1/2 to zoom, Esc to pause. Original key bindings
  and connected SDL gamepads are also available.
- Landscape is recommended. Browser fullscreen is optional; on iOS, use the
  browser's Add to Home Screen feature where fullscreen is unavailable.

## PWA and TWA

All web builds include an install manifest, app icons and a versioned offline service worker. See [PWA/TWA hosting and signing](common/PWA.md) for HTTPS, offline updates, and Android Digital Asset Links setup.

## Storage and hosting

Preferences, high scores and the game's save slots are persisted in IndexedDB
after writes. This preserves the game's existing save opportunities; it does not
add autosave. Storage belongs to the URL's origin, so `localhost`, a LAN IP and a
production hostname have separate saves. Clearing site data removes them. If
storage is unavailable, a message explains that only session saves are available.

Deploy `index.html`, `index.js`, `index.wasm`, `index.data`, `manifest.webmanifest`, `sw.js` and `web/` from
`build-web/` together on any static host. Serve `.wasm` as `application/wasm` and
enable compression for production. Assets are about 65 MB before transfer
compression. No backend, cross-origin isolation or SharedArrayBuffer is required.
Keep the original credits and CC BY-NC-SA 4.0 license; see [LICENSE.md](../LICENSE.md).

## Port notes / limitations

- Asyncify yields from the common presentation path so the original nested game,
  menu and fade loops can run without blocking the browser. SDL's implicit yields
  are disabled to avoid multiple sleeps per frame.
- Fixed-function rendering uses Emscripten's legacy GL compatibility layer.
  SDL3 needs an explicit initialization bridge to that layer. Packed Mac textures
  are converted to RGBA, strided updates are repacked, and mesh indices use 16 bits.
- Rendering uses CSS pixel resolution rather than device pixel resolution to
  avoid multiplying the GPU workload on high-density phones.
- The canvas tracks the visual viewport as mobile browser bars expand/collapse.
  HUD artwork stays inside safe areas, with a compact 20-pixel bottom strip and
  a top bar clear of the utility buttons. The scene fills the viewport behind it.
- This is an experimental compatibility port. Real iOS/Android hardware, all ten
  levels, controller mappings, graphics fidelity and long sessions need further
  testing. Context loss currently requires a reload.

## Regression tests

Run the gesture and texture-conversion tests with Node.js 18+ and a C compiler
that supports AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
bash web/test.sh
```

After building the web target, run the browser integration tests in Chromium:

```sh
python3 -m venv /tmp/bugdom-test-env
/tmp/bugdom-test-env/bin/pip install playwright==1.65.2
/tmp/bugdom-test-env/bin/playwright install chromium
/tmp/bugdom-test-env/bin/python web/tests/browser_test.py
```

The browser suite starts its own local server on a free port and uses isolated
browser profiles. It checks level-one startup, pause, action ownership, phone
layouts, viewport resizing, mute and saved-file persistence, blocked storage,
and recovery UI after a stopped runtime. The gesture tests cover independent
movement/camera fingers, pinch ownership, cancellation, and extra fingers. The
C tests cover packed pixel formats, opacity, strided HUD updates, and unaligned
input. These checks complement real-device playtesting; they do not verify all
levels or Safari behavior.

References: [SDL Emscripten support](https://wiki.libsdl.org/SDL3/README-emscripten),
[Asyncify](https://emscripten.org/docs/porting/asyncify.html),
[OpenGL support](https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html).
