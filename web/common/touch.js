/* Floating stick, drag-look and pinch gestures, following BlockWorld's touch HUD.
 * The target supplies setAnalogMove(x,y), applyLookDelta(dx,dy), zoomBy(delta).
 * Pointer ownership stays here; game input stays behind that small interface. */
'use strict';
class PangeaTouchControls {
  constructor(root, target) {
    this.root = root;
    this.target = target;
    this.ring = root.querySelector('[data-touch-stick]');
    this.knob = this.ring.firstElementChild;
    this.stick = null;
    this.look = new Map();
    this.first = null;
    this.pinchDistance = 0;
    this.active = false;
    root.addEventListener('pointerdown', event => this.down(event));
    root.addEventListener('pointermove', event => this.move(event));
    for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
      root.addEventListener(type, event => this.up(event));
    }
    root.addEventListener('contextmenu', event => event.preventDefault());
  }

  setActive(active) {
    if (this.active && !active) this.reset();
    this.active = active;
    this.root.hidden = !active;
  }

  reset() {
    const ids = [...this.look.keys()];
    if (this.stick !== null) ids.push(this.stick);
    this.releaseStick();
    this.look.clear();
    this.first = null;
    this.pinchDistance = 0;
    for (const id of ids) {
      if (this.root.hasPointerCapture(id)) this.root.releasePointerCapture(id);
    }
  }

  releaseStick() {
    this.stick = null;
    this.ring.style.opacity = '0';
    this.knob.style.transform = '';
    this.target.setAnalogMove(0, 0);
  }

  steer(x, y) {
    const dx = x - this.center.x, dy = y - this.center.y;
    const length = Math.hypot(dx, dy);
    const reach = this.radius * .6;
    const shown = Math.min(length, reach) / (length || 1);
    this.knob.style.transform = `translate(${dx * shown}px, ${dy * shown}px)`;
    if (length <= this.radius * .12) this.target.setAnalogMove(0, 0);
    else {
      const scale = Math.min(1, length / reach) / length;
      this.target.setAnalogMove(dx * scale, dy * scale);
    }
  }

  spread() {
    const [a, b] = this.look.values();
    return Math.hypot(a.x - b.x, a.y - b.y);
  }

  down(event) {
    if (!this.active || event.pointerType === 'mouse' || event.target.closest('button, a, input')) return;
    // A new primary touch also recovers from a lost OS cancellation event.
    if (event.isPrimary) this.reset();
    if (this.look.size >= 2) return;
    event.preventDefault();
    this.root.setPointerCapture(event.pointerId);
    this.root.querySelector('.gesture-hint').hidden = true;
    const now = performance.now();
    const lone = (this.stick !== null ? 1 : 0) + this.look.size === 1;
    // Two fingers placed together are a pinch, even across the screen midpoint.
    // Once the first thumb is moving, a second finger remains independent look.
    if (lone && this.first && now - this.first.at < 400 && this.first.travel < 24) {
      const id = this.stick ?? this.look.keys().next().value;
      const position = { x: this.first.x, y: this.first.y };
      this.releaseStick();
      this.look.clear();
      this.look.set(id, position);
      this.look.set(event.pointerId, { x: event.clientX, y: event.clientY });
      this.pinchDistance = this.spread();
      this.first = null;
      return;
    }
    const bounds = this.root.getBoundingClientRect();
    // Only a fresh lone finger can become the first half of a pinch. Replacing
    // a camera finger while moving must not steal the existing movement thumb.
    this.first = this.stick === null && this.look.size === 0
      ? { id: event.pointerId, at: now, ox: event.clientX, oy: event.clientY, x: event.clientX, y: event.clientY, travel: 0 }
      : null;
    if (this.stick === null && this.look.size < 2 && event.clientX < bounds.left + bounds.width / 2) {
      this.stick = event.pointerId;
      this.radius = this.ring.offsetWidth / 2;
      this.center = { x: event.clientX, y: event.clientY };
      this.ring.style.left = `${event.clientX - bounds.left - this.radius}px`;
      this.ring.style.top = `${event.clientY - bounds.top - this.radius}px`;
      this.ring.style.opacity = '.55';
      this.steer(event.clientX, event.clientY);
    } else if (this.look.size < 2) {
      this.look.set(event.pointerId, { x: event.clientX, y: event.clientY });
      if (this.look.size === 2) this.pinchDistance = this.spread();
    }
  }

  move(event) {
    if (!this.active) return;
    if (this.first?.id === event.pointerId) {
      this.first.x = event.clientX; this.first.y = event.clientY;
      this.first.travel = Math.max(this.first.travel, Math.hypot(event.clientX - this.first.ox, event.clientY - this.first.oy));
    }
    if (this.stick === event.pointerId) {
      this.steer(event.clientX, event.clientY);
      event.preventDefault();
      return;
    }
    const position = this.look.get(event.pointerId);
    if (!position) return;
    const dx = event.clientX - position.x, dy = event.clientY - position.y;
    position.x = event.clientX; position.y = event.clientY;
    if (this.look.size === 2) {
      const distance = this.spread();
      this.target.zoomBy((this.pinchDistance - distance) * 1.2);
      this.pinchDistance = distance;
    } else this.target.applyLookDelta(dx, dy);
    event.preventDefault();
  }

  up(event) {
    if (this.stick === event.pointerId) this.releaseStick();
    this.look.delete(event.pointerId);
    if (this.first?.id === event.pointerId || (this.stick === null && !this.look.size)) this.first = null;
  }
}
