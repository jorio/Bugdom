# PWA and Trusted Web Activity hosting

Every web build generates `manifest.webmanifest` and `sw.js`, and copies the
192px/512px install icons derived from the game's existing artwork. Deploy the
**entire contents of `build-web/` together**. Each game can live on its own
origin or in a separate subdirectory; its manifest ID, start URL, scope and
service-worker cache are scoped to that directory. Serve `.wasm` as
`application/wasm`, `.webmanifest` as `application/manifest+json`, and `.js` as
JavaScript. Use HTTPS in production. `localhost` also supports service workers;
an ordinary HTTP LAN address does not.

The service worker caches the complete matching HTML/JS/Wasm/data/icon set.
The welcome screen reports when offline preparation finishes. The first visit
needs a network connection and enough browser storage for the game's assets.
Then the installed game can launch offline. Saved games remain in IndexedDB,
separate from the service-worker caches. Browsers may evict storage; installation
is not a substitute for backups. Nanosaur retains its original high-score and
preference persistence; this port does not invent mid-game save slots.

Updates are content-versioned. A new worker installs a complete cache before
it can activate, and waits until all windows using the old version close.
There is no forced reload during play. Close all game windows and reopen to
apply an update. Deploy atomically and avoid long-lived HTTP caching of `sw.js`.
The worker only intercepts this game's known assets and entry page, so it does
not take over other sites or other game subdirectories on the same origin.

For install testing on a phone, use a trusted HTTPS development endpoint or
provide a certificate trusted by the device:

```sh
python3 web/serve.py --port 8000 --cert /path/to/cert.pem --key /path/to/key.pem
```

Chromium exposes an **Install game** button when it offers installation. On iOS,
use Safari's Add to Home Screen. Installed games request fullscreen landscape;
the browser version also supports portrait and browser chrome resizing.

## Android TWA packaging

The web build is suitable for a wrapper produced by Bubblewrap or another TWA
packager. Packaging requires the eventual HTTPS manifest URL, an Android
application ID, and the signing certificate for that Android application. No
package, signing key, or domain is invented or embedded in these repositories.
A TWA uses the browser's PWA runtime and offline cache, rather than embedding
the game data in the APK.

After choosing those values, initialize the Android wrapper against the hosted
manifest using the [official TWA guide](https://developer.android.com/develop/ui/views/layout/webapps/guide-trusted-web-activities-version2).
Generate the website association using the certificate SHA-256 fingerprint:

```sh
python3 web/common/assetlinks.py \
  --package com.yourcompany.yourgame \
  --sha256 AA:BB:...:FF \
  --output build-web/.well-known/assetlinks.json
```

Replace the abbreviated fingerprint with all 32 bytes. The helper validates
it, preserves other apps already in the file, and supports repeated `--sha256`
arguments. Serve this file at **the origin root**
`https://your-domain/.well-known/assetlinks.json`, even if the game is hosted
in a subdirectory. It must be public JSON without redirects or authentication.
Use the Play **app-signing** certificate when Play signs distributed builds;
its fingerprint can differ from your upload key. Never commit private keys.

The Android wrapper must declare the reciprocal website association. Without
successful verification the browser opens a Custom Tab with browser UI instead
of a TWA. Verification and a signed Android package can only be tested after
those deployment values exist. See [Digital Asset Links configuration](https://developer.android.com/training/app-links/configure-assetlinks)
and [web app manifest requirements](https://web.dev/articles/install-criteria).

## Verification

`python web/tests/browser_test.py` exercises manifest fields and a real offline
reload of the compiled game, in addition to gameplay, input, storage and mute.
The shared-code drift check is independent of the browser:

```sh
python3 web/common/check_sync.py ../Bugdom ../Bugdom2 ../Nanosaur ../Nanosaur2
```
