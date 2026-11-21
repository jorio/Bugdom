'use strict';
(() => {
  const status = document.getElementById('pwa-status');
  const install = document.getElementById('install');
  let prompt;
  window.addEventListener('beforeinstallprompt', event => {
    event.preventDefault(); prompt = event; install.hidden = false;
  });
  install.onclick = async () => {
    if (!prompt) return;
    await prompt.prompt();
    await prompt.userChoice;
    prompt = null; install.hidden = true;
  };
  window.addEventListener('appinstalled', () => { install.hidden = true; });
  if (!window.isSecureContext || !('serviceWorker' in navigator)) {
    status.textContent = 'Install and offline play are available over HTTPS or localhost.';
    return;
  }
  navigator.serviceWorker.register('sw.js', {scope: './', updateViaCache: 'none'}).then(registration => {
    status.textContent = registration.active ? 'Available offline.' : 'Preparing offline play…';
    const watch = worker => {
      worker.addEventListener('statechange', () => {
        if (worker.state === 'installed') status.textContent = navigator.serviceWorker.controller
          ? 'Update ready. Close all game windows and reopen to update.' : 'Available offline.';
        if (worker.state === 'redundant') status.textContent = 'Offline download did not finish. Online play is still available.';
      });
    };
    if (registration.installing) watch(registration.installing);
    registration.addEventListener('updatefound', () => { if (registration.installing) watch(registration.installing); });
  }).catch(() => { status.textContent = 'Offline storage is unavailable. Online play is still available.'; });
})();
