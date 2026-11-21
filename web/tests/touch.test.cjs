const { test } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const Context = vm.createContext({ performance });
vm.runInContext(readFileSync(path.join(__dirname, '../touch.js'), 'utf8'), Context);
const TouchControls = vm.runInContext('BugdomTouchControls', Context);

function fixture() {
  const events = [];
  const captured = new Set();
  const knob = { style: {} };
  const ring = { style: {}, offsetWidth: 140, firstElementChild: knob };
  const root = {
    hidden: true, addEventListener() {},
    querySelector: selector => selector === '[data-touch-stick]' ? ring : {},
    getBoundingClientRect: () => ({ left: 0, top: 0, width: 844 }),
    setPointerCapture: id => captured.add(id),
    hasPointerCapture: id => captured.has(id),
    releasePointerCapture: id => captured.delete(id),
  };
  const target = {
    setAnalogMove: (x, y) => events.push(['move', x, y]),
    applyLookDelta: (x, y) => events.push(['look', x, y]),
    zoomBy: delta => events.push(['zoom', delta]),
  };
  const controls = new TouchControls(root, target);
  controls.setActive(true);
  function send(type, id, x, y, options = {}) {
    controls[type]({ pointerId: id, clientX: x, clientY: y,
      pointerType: 'touch', isPrimary: false, target: { closest: () => false },
      preventDefault() {}, ...options });
  }
  return { controls, events, root, ring, captured, send };
}

test('floating stick has a dead zone and caps diagonal magnitude', () => {
  const { send, events, ring } = fixture();
  send('down', 1, 100, 200);
  send('move', 1, 103, 202);
  assert.deepEqual(events.at(-1), ['move', 0, 0]);
  send('move', 1, 180, 120);
  const [, x, y] = events.at(-1);
  assert.ok(x > .7 && y < -.7);
  assert.ok(Math.abs(Math.hypot(x, y) - 1) < 1e-6);
  send('up', 1, 180, 120);
  assert.deepEqual(events.at(-1), ['move', 0, 0]);
  assert.equal(ring.style.opacity, '0');
});

test('moving thumb and independent camera finger can coexist', () => {
  const { send, controls, events } = fixture();
  send('down', 1, 100, 200);
  send('move', 1, 140, 160);
  send('down', 2, 600, 150);
  send('move', 2, 620, 160);
  assert.equal(controls.stick, 1);
  assert.deepEqual(events.at(-1), ['look', 20, 10]);
});

test('two fingers placed together pinch across the screen', () => {
  const { send, controls, events } = fixture();
  send('down', 1, 250, 200);
  send('down', 2, 550, 200);
  send('move', 2, 580, 200);
  assert.equal(controls.stick, null);
  assert.deepEqual(events.at(-1), ['zoom', -36]);
});

test('lifting a look finger does not turn a moving thumb into the next pinch', () => {
  const { send, controls, events } = fixture();
  send('down', 1, 100, 200);
  send('move', 1, 140, 160);
  send('down', 2, 600, 150);
  send('up', 2, 600, 150);
  send('down', 3, 610, 180);
  send('move', 3, 620, 190);
  assert.equal(controls.stick, 1);
  assert.deepEqual(events.at(-1), ['look', 10, 10]);
});

test('extra fingers and interactive elements do not acquire gesture ownership', () => {
  const { send, captured } = fixture();
  send('down', 1, 500, 200);
  send('down', 2, 600, 200);
  send('down', 3, 700, 200);
  send('down', 4, 100, 200, { target: { closest: () => true } });
  assert.deepEqual([...captured], [1, 2]);
});

test('pause/reset releases movement, camera pointers and pointer captures', () => {
  const { send, controls, events, captured, root } = fixture();
  send('down', 1, 100, 200);
  send('move', 1, 140, 160);
  send('down', 2, 600, 150);
  controls.setActive(false);
  assert.equal(controls.stick, null);
  assert.equal(controls.look.size, 0);
  assert.equal(captured.size, 0);
  assert.equal(root.hidden, true);
  assert.deepEqual(events.at(-1), ['move', 0, 0]);
  const count = events.length;
  send('move', 2, 650, 200);
  assert.equal(events.length, count);
});
