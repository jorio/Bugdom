/* global FS, IDBFS, ENV, addRunDependency, removeRunDependency */
'use strict';
const canvas = document.getElementById('canvas');
const statusText = document.getElementById('status');
const startButton = document.getElementById('start');
const notice = document.getElementById('notice');
const actions = ['up', 'down', 'left', 'right', 'jump', 'kick', 'roll', 'buddy',
  'pause', 'confirm', 'cameraLeft', 'cameraRight', 'zoomIn', 'zoomOut', 'pickup', 'jet', 'nextWeapon', 'prevWeapon', 'cameraMode'];
// Game identity and action bindings live outside this reusable runtime.
document.title = `${gameConfig.title} · Web`;
document.getElementById('title').textContent = gameConfig.title;
document.getElementById('tagline').textContent = gameConfig.tagline;
document.getElementById('control-hint').textContent = gameConfig.hint;
document.getElementById('credits').textContent = gameConfig.credits;
const sourceLink = document.getElementById('source-link');
sourceLink.href = gameConfig.source;
sourceLink.textContent = 'Source & license';
canvas.setAttribute('aria-label', `${gameConfig.title} game. Tap menu items to select them.`);
document.getElementById('touch-controls').dataset.buttons = gameConfig.buttons.length;
for (const [index, action] of gameConfig.buttons.entries()) {
  const button = document.createElement('button');
  button.dataset.action = action.action;
  button.setAttribute('aria-label', action.label);
  if (!index) button.className = 'primary';
  const icon = document.createElement('span');
  icon.setAttribute('aria-hidden', 'true'); icon.textContent = action.icon;
  const label = document.createElement('small'); label.textContent = action.label.toUpperCase();
  button.append(icon, label);
  document.getElementById('actions').append(button);
}
const sources = new Map();
let started = false, playing = false, paused = false;
let runtimeStopped = false;
let touchEnabled = matchMedia('(pointer: coarse)').matches;
let storageReady = false, syncing = false, syncAgain = false;
let soundMuted = false;
try { soundMuted = localStorage.getItem(`${gameConfig.id}.muted`) === 'true'; } catch { /* Session-only preference. */ }
function updateMuteButton() {
  const button = document.getElementById('mute');
  const label = soundMuted ? 'Unmute sound' : 'Mute sound';
  button.setAttribute('aria-label', label);
  button.setAttribute('title', label);
  button.setAttribute('aria-pressed', String(soundMuted));
}
updateMuteButton();
const touch = new PangeaTouchControls(document.getElementById('touch-controls'), {
  setAnalogMove(x, y) { if (started) Module._WebSetStick(x, y); },
  applyLookDelta(dx, dy) { if (started) Module._WebAddCamera(dx * .006, dy * .004, 0); },
  zoomBy(delta) { if (started) Module._WebAddCamera(0, 0, delta * 2); }
});

function setAction(action, source, down) {
  if (!started) return;
  const index = actions.indexOf(action);
  if (index < 0) return;
  if (!sources.has(action)) sources.set(action, new Set());
  const owners = sources.get(action);
  if (down) owners.add(source); else owners.delete(source);
  Module._WebSetButton(index, owners.size ? 1 : 0);
  document.querySelectorAll(`[data-action="${action}"]`).forEach(button => {
    button.classList.toggle('pressed', owners.size > 0);
  });
}

function releaseAll() {
  sources.clear();
  touch.reset();
  if (started) Module._WebResetInput();
  document.querySelectorAll('.pressed').forEach(button => button.classList.remove('pressed'));
}

function stopGame(message) {
  // Never call Wasm exports after abort/exit. The shell still has live browser
  // listeners, so turn off their bridge before clearing gesture ownership.
  runtimeStopped = true;
  started = false;
  playing = paused = false;
  releaseAll();
  updateControls();
  document.getElementById('welcome').hidden = false;
  document.getElementById('toolbar').hidden = true;
  statusText.textContent = message;
  startButton.disabled = false;
  startButton.textContent = 'Reload game';
  startButton.onclick = () => location.reload();
}

function updateControls() {
  touch.setActive(started && touchEnabled && playing && !paused);
  document.getElementById('confirm').hidden = !started || (playing && !paused);
  document.getElementById('toggle-touch').setAttribute('aria-pressed', String(touchEnabled));
}

function syncSaves() {
  if (!storageReady) return;
  if (syncing) { syncAgain = true; return; }
  syncing = true;
  FS.syncfs(false, error => {
    syncing = false;
    if (error) notice.textContent = 'Could not save to browser storage.';
    if (syncAgain) { syncAgain = false; syncSaves(); }
  });
}

var Module = {
  canvas,
  noInitialRun: true,
  preRun: [function () {
    const prefsRoot = gameConfig.prefsRoot;
    ENV.XDG_CONFIG_HOME = prefsRoot;
    FS.mkdirTree(prefsRoot);
    FS.mount(IDBFS, {}, prefsRoot);
    addRunDependency('load-saves');
    FS.syncfs(true, error => {
      storageReady = !error;
      if (error) notice.textContent = 'Browser storage unavailable; saves will last for this session.';
      removeRunDependency('load-saves');
    });
  }],
  onRuntimeInitialized() {
    if (runtimeStopped) return;
    Module._WebSetMuted(soundMuted ? 1 : 0);
    statusText.textContent = 'Ready to explore.';
    startButton.textContent = `Play ${gameConfig.title}`;
    startButton.disabled = false;
  },
  setStatus(text) { if (text && !started && !runtimeStopped) statusText.textContent = text; },
  print: (...args) => console.log(...args),
  printErr: (...args) => console.error(...args),
  onAbort(reason) {
    notice.textContent = 'The game stopped. Reload to try again.';
    stopGame(`The game stopped: ${reason}`);
  },
  onExit() {
    syncSaves();
    stopGame('Thanks for playing. Reload to start again.');
  },
  syncSaves,
  updateGameState(nextPlaying, nextPaused) {
    if (runtimeStopped) return;
    if (playing !== nextPlaying || paused !== nextPaused) releaseAll();
    playing = nextPlaying; paused = nextPaused;
    updateControls();
  }
};

// Browser chrome can change the visual viewport without firing window.resize.
// Keep both the page and the SDL drawing buffer in step with the visible area.
function updateViewport() {
  const viewport = window.visualViewport;
  const style = document.documentElement.style;
  style.setProperty('--viewport-width', `${viewport?.width ?? window.innerWidth}px`);
  style.setProperty('--viewport-height', `${viewport?.height ?? window.innerHeight}px`);
  style.setProperty('--viewport-left', `${viewport?.offsetLeft ?? 0}px`);
  style.setProperty('--viewport-top', `${viewport?.offsetTop ?? 0}px`);
  const safe = getComputedStyle(document.getElementById('safe-area'));
  const nextViewport = [canvas.clientWidth, canvas.clientHeight,
    ...['Top', 'Right', 'Bottom', 'Left'].map(edge => Math.round(parseFloat(safe[`padding${edge}`]) || 0))];
  if (Module.webViewport && nextViewport.some((value, i) => value !== Module.webViewport[i])) releaseAll();
  Module.webViewport = nextViewport;
}
new ResizeObserver(updateViewport).observe(document.getElementById('game'));
window.addEventListener('resize', updateViewport);
window.visualViewport?.addEventListener('resize', updateViewport);
window.visualViewport?.addEventListener('scroll', updateViewport);
updateViewport();

startButton.onclick = () => {
  if (started || runtimeStopped) return;
  started = true;
  document.getElementById('welcome').hidden = true;
  document.getElementById('toolbar').hidden = false;
  canvas.focus();
  updateControls();
  Module.callMain([]);
};

// Pointer capture lets a held action survive drifting off its button. Multiple
// owners prevent one finger/key release from cancelling another held input.
document.querySelectorAll('[data-action]').forEach(button => {
  button.addEventListener('pointerdown', event => {
    event.preventDefault();
    event.stopPropagation();
    canvas.focus();
    button.setPointerCapture(event.pointerId);
    setAction(button.dataset.action, event.pointerId, true);
  });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
    button.addEventListener(type, event => {
      event.stopPropagation();
      setAction(button.dataset.action, event.pointerId, false);
    });
  }
});

const keyboard = gameConfig.keyboard;
for (const type of ['keydown', 'keyup']) {
  window.addEventListener(type, event => {
    if (!started) return;
    if (keyboard[event.code]) {
      event.preventDefault();
      setAction(keyboard[event.code], event.code, type === 'keydown');
    }
    if (['Space', 'Tab', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(event.code)) event.preventDefault();
  });
}
window.addEventListener('blur', releaseAll);
window.addEventListener('resize', releaseAll);
document.addEventListener('visibilitychange', () => {
  if (document.hidden) {
    releaseAll();
    if (started && playing && !paused) {
      Module._WebRequestPause();
    }
    syncSaves();
  }
});
window.addEventListener('pagehide', syncSaves);
canvas.addEventListener('contextmenu', event => event.preventDefault());
function canvasPointer(event, down) {
  if (!started || event.pointerType === 'mouse') return;
  event.preventDefault();
  const bounds = canvas.getBoundingClientRect();
  Module._WebSetPointer((event.clientX - bounds.left) / bounds.width,
    (event.clientY - bounds.top) / bounds.height, down ? 1 : 0);
}
canvas.addEventListener('pointerdown', event => {
  if (event.pointerType !== 'mouse') canvas.setPointerCapture(event.pointerId);
  canvasPointer(event, true);
});
for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
  canvas.addEventListener(type, event => canvasPointer(event, false));
}
canvas.addEventListener('pointermove', event => {
  if (started && event.pointerType === 'mouse') Module._WebSetPointer(-1, -1, 0);
});
canvas.addEventListener('webglcontextlost', event => {
  event.preventDefault(); releaseAll();
  notice.textContent = 'Graphics interrupted. Reload the page to resume from your last save.';
});
document.getElementById('toggle-touch').onclick = () => {
  releaseAll(); touchEnabled = !touchEnabled; updateControls(); canvas.focus();
};
document.getElementById('mute').onclick = () => {
  if (runtimeStopped) return;
  soundMuted = !soundMuted;
  Module._WebSetMuted(soundMuted ? 1 : 0);
  updateMuteButton();
  try { localStorage.setItem(`${gameConfig.id}.muted`, String(soundMuted)); } catch { /* Session-only preference. */ }
  canvas.focus();
};
document.getElementById('fullscreen').onclick = async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else if (document.getElementById('game').requestFullscreen) {
      await document.getElementById('game').requestFullscreen();
      try { await screen.orientation?.lock('landscape'); } catch { /* Optional on mobile browsers. */ }
    }
    else notice.textContent = 'Use your browser’s Add to Home Screen option for a larger view.';
  } catch { notice.textContent = 'Full screen is unavailable in this browser.'; }
  canvas.focus();
};
