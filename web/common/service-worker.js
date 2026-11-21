/* Generated as sw.js at the app root. No game-specific code belongs here. */
'use strict';
const PREFIX = `pangea:${self.registration.scope}:`;
const CACHE = `${PREFIX}__BUILD_VERSION__`;
const ASSETS = __PRECACHE_ASSETS__;
const URLS = new Set(ASSETS.map(path => new URL(path, self.registration.scope).href));
self.addEventListener('install', event => {
  event.waitUntil((async () => {
    try {
      const cache = await caches.open(CACHE);
      await cache.addAll(ASSETS.map(path => new Request(new URL(path, self.registration.scope), {cache: 'reload'})));
    } catch (error) {
      await caches.delete(CACHE);
      throw error;
    }
    // Do not skipWaiting: a running game must keep its matching JS/Wasm/data set.
  })());
});
self.addEventListener('activate', event => {
  event.waitUntil((async () => {
    for (const key of await caches.keys()) {
      if (key.startsWith(PREFIX) && key !== CACHE) await caches.delete(key);
    }
    await self.clients.claim();
  })());
});
self.addEventListener('fetch', event => {
  if (event.request.method !== 'GET') return;
  const url = new URL(event.request.url);
  const home = new URL(self.registration.scope);
  const navigation = event.request.mode === 'navigate' &&
    (url.pathname === home.pathname || url.pathname === new URL('index.html', home).pathname);
  const canonical = navigation ? new URL('index.html', home).href : url.href;
  if (!URLS.has(canonical)) return;
  event.respondWith((async () => {
    const cache = await caches.open(CACHE);
    return await cache.match(canonical) || fetch(event.request);
  })());
});
