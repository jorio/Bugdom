#!/usr/bin/env python3
"""Generate a scoped, versioned PWA bundle after Emscripten links the game."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil


def write_changed(path, content):
    data = content.encode()
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--web', required=True, type=Path)
    args = parser.parse_args()
    config = json.loads((args.web / 'game.js').read_text().removeprefix('const gameConfig = ').strip().removesuffix(';'))
    build = args.build
    icons = build / 'web/icons'
    icons.mkdir(parents=True, exist_ok=True)
    for size in [192, 512]:
        shutil.copy2(args.web / f'icons/icon-{size}.png', icons / f'icon-{size}.png')
    manifest = dict(id='./', name=config['title'], short_name=config['title'],
                    description=config['tagline'], start_url='./', scope='./',
                    display='fullscreen', orientation='landscape',
                    background_color='#0b120f', theme_color='#14251d',
                    lang='en', categories=['games'], prefer_related_applications=False,
                    icons=[dict(src=f'web/icons/icon-{size}.png', sizes=f'{size}x{size}',
                                type='image/png', purpose='any') for size in [192, 512]])
    write_changed(build / 'manifest.webmanifest', json.dumps(manifest, indent=2)+'\n')
    assets = ['index.html', 'index.js', 'index.wasm', 'index.data', 'manifest.webmanifest',
              'web/game.js', 'web/common/controls.js', 'web/common/touch.js',
              'web/common/style.css', 'web/common/pwa.js', 'web/common/legacy_gl.js',
              'web/icons/icon-192.png', 'web/icons/icon-512.png']
    digest = hashlib.sha256()
    for asset in assets:
        digest.update(asset.encode())
        with (build / asset).open('rb') as f:
            for block in iter(lambda: f.read(1024*1024), b''): digest.update(block)
    template = (args.web / 'common/service-worker.js').read_text()
    digest.update(template.encode())
    worker = template.replace('__BUILD_VERSION__', digest.hexdigest()[:20]).replace('__PRECACHE_ASSETS__', json.dumps(assets))
    write_changed(build / 'sw.js', worker)
    print(f'PWA: {config["title"]}, {len(assets)} offline assets, version {digest.hexdigest()[:20]}')


if __name__ == '__main__':
    main()
