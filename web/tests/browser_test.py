"""Integration tests against the compiled game; see web/README.md for setup."""
from functools import partial
from http.server import ThreadingHTTPServer
from pathlib import Path
import sys
import threading
import time
import unittest

from playwright.sync_api import sync_playwright

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from serve import DevHandler


class QuietHandler(DevHandler):
    def log_message(self, *_):
        pass


class BrowserTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        build = Path(__file__).resolve().parents[2] / 'build-web'
        if not (build / 'index.wasm').exists():
            raise RuntimeError('Build the web target first: bash web/build.sh')
        cls.server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(build)))
        cls.server_thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.server_thread.start()
        cls.url = f'http://127.0.0.1:{cls.server.server_port}'
        cls.pw = sync_playwright().start()
        cls.browser = cls.pw.chromium.launch()

    @classmethod
    def tearDownClass(cls):
        cls.browser.close()
        cls.pw.stop()
        cls.server.shutdown()
        cls.server.server_close()
        cls.server_thread.join()

    def setUp(self):
        self.context = self.browser.new_context(viewport={'width': 844, 'height': 390},
                                               has_touch=True, is_mobile=True, device_scale_factor=2)
        self.page = self.context.new_page()
        self.errors = []
        self.page.on('pageerror', lambda error: self.errors.append(str(error)))
        self.page.on('dialog', lambda dialog: (self.errors.append(dialog.message), dialog.dismiss()))
        self.page.on('console', lambda message: self.errors.append(message.text)
                     if 'Game Fatal Alert' in message.text else None)

    def tearDown(self):
        self.context.close()
        self.assertEqual(self.errors, [])

    def ready(self):
        self.page.goto(self.url)
        self.page.wait_for_function('!document.querySelector("#start").disabled', timeout=120000)

    def test_mute_and_saved_files_survive_reload(self):
        self.ready()
        p = self.page
        p.locator('#start').tap()
        p.locator('#mute').tap()
        self.assertEqual(p.evaluate('Module._WebIsMuted()'), 1)
        p.evaluate('''async () => {
          Module.FS.writeFile(gameConfig.prefsRoot + '/regression-save', 'saved game data');
          Module.syncSaves();
          await new Promise(resolve => {
            const timer = setInterval(() => { if (!syncing) { clearInterval(timer); resolve(); } }, 20);
          });
        }''')
        self.ready()
        self.assertEqual(p.evaluate('Module.FS.readFile(gameConfig.prefsRoot + "/regression-save", {encoding:"utf8"})'), 'saved game data')
        self.assertEqual(p.locator('#mute').get_attribute('aria-pressed'), 'true')
        p.locator('#start').tap()
        p.wait_for_timeout(500)
        self.assertEqual(p.evaluate('Module._WebIsMuted()'), 1)
        p.locator('#mute').tap()
        self.assertEqual(p.evaluate('Module._WebIsMuted()'), 0)

    def test_unavailable_storage_still_starts(self):
        self.context.add_init_script("Object.defineProperty(window, 'indexedDB', {get() {throw new Error('blocked storage');}})")
        self.ready()
        self.assertIn('session', self.page.locator('#notice').inner_text())
        self.page.locator('#start').tap()
        self.page.wait_for_timeout(500)
        self.assertTrue(self.page.evaluate('started'))

    def test_pwa_installs_and_reloads_offline(self):
        self.ready()
        p = self.page
        manifest = p.evaluate("async () => (await fetch('manifest.webmanifest')).json()")
        self.assertEqual(manifest['scope'], './')
        self.assertEqual(manifest['start_url'], './')
        self.assertEqual(manifest['display'], 'fullscreen')
        self.assertEqual({icon['sizes'] for icon in manifest['icons']}, {'192x192', '512x512'})
        p.wait_for_function('navigator.serviceWorker.controller !== null', timeout=180000)
        # The worker only activates after every HTML/JS/Wasm/data/icon asset is cached.
        self.context.set_offline(True)
        p.reload()
        p.wait_for_function('!document.querySelector("#start").disabled', timeout=120000)
        p.locator('#start').tap()
        p.wait_for_timeout(500)
        self.assertTrue(p.evaluate('started && !runtimeStopped'))

    def test_stopped_runtime_disables_input_bridge(self):
        self.ready()
        p = self.page
        # Stop before callMain, then make any accidental bridge call fail loudly.
        p.evaluate('''() => {
          Module.onAbort('test abort');
          for (const key of Object.keys(Module)) if (key.startsWith('_Web'))
            Module[key] = () => { throw Error('called stopped runtime: ' + key); };
          Module.onRuntimeInitialized();
          Module.updateGameState(true, false);
          window.dispatchEvent(new Event('blur'));
          document.querySelector('#mute').click();
        }''')
        self.assertEqual(p.locator('#start').inner_text(), 'Reload game')
        self.assertTrue(p.locator('#toolbar').is_hidden())
        self.assertTrue(p.locator('#touch-controls').is_hidden())

    def test_level_one_controls_resize_and_pause(self):
        self.ready()
        p = self.page
        p.locator('#start').tap()
        # Existing game shortcut: hold 1 while dismissing the legal screen.
        shortcut = p.evaluate('gameConfig.id') == 'bugdom'
        if shortcut: p.keyboard.down('1')
        deadline = time.monotonic() + 90
        while not p.evaluate('playing') and time.monotonic() < deadline:
            if p.locator('#confirm').is_visible():
                p.locator('#confirm').tap()
            p.wait_for_timeout(500)
        if shortcut: p.keyboard.up('1')
        self.assertTrue(p.evaluate('playing'), 'Level 1 did not start')
        p.wait_for_timeout(500)
        for width, height in [(844, 390), (667, 320), (390, 844)]:
            p.set_viewport_size({'width': width, 'height': height})
            p.wait_for_function('([w,h]) => canvas.width === w && canvas.height === h && canvas.width === canvas.clientWidth && canvas.height === canvas.clientHeight', arg=[width, height])
            boxes = p.locator('#actions button').evaluate_all('els => els.map(e => {const r=e.getBoundingClientRect(); return {action:e.dataset.action,x:r.x,y:r.y,w:r.width,h:r.height};})')
            self.assertEqual(boxes[0]['action'], 'kick')
            for i, box in enumerate(boxes):
                self.assertGreaterEqual(box['y'], 0)
                self.assertLessEqual(box['y'] + box['h'], height)
                self.assertLessEqual(box['x'] + box['w'], width)
                if i:
                    self.assertGreater(box['y'], boxes[i-1]['y'] + boxes[i-1]['h'])
        p.keyboard.down('k')
        p.evaluate("setAction('kick', 99, true); setAction('kick', 99, false)")
        self.assertTrue(p.locator('[data-action="kick"]').evaluate("e => e.classList.contains('pressed')"))
        p.keyboard.up('k')
        self.assertFalse(p.locator('[data-action="kick"]').evaluate("e => e.classList.contains('pressed')"))
        # visualViewport can resize without a window resize when browser bars move.
        p.keyboard.down('k')
        p.evaluate("Module.webViewport[1] -= 20; window.visualViewport.dispatchEvent(new Event('resize'))")
        self.assertFalse(p.locator('[data-action="kick"]').evaluate("e => e.classList.contains('pressed')"))
        p.keyboard.up('k')
        p.locator('[data-action="pause"]').tap()
        p.wait_for_function('paused')
        self.assertTrue(p.locator('#touch-controls').is_hidden())


if __name__ == '__main__':
    unittest.main(verbosity=2)
