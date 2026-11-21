#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_dir"
for tool in emcmake cmake; do
  command -v "$tool" >/dev/null || { echo "Missing $tool. See web/README.md." >&2; exit 1; }
done
test -f extern/Pomme/CMakeLists.txt || { echo 'Run git submodule update --init --recursive first.' >&2; exit 1; }
test -f extern/SDL/CMakeLists.txt || { echo 'Download SDL 3.2.8 into extern/SDL (see web/README.md).' >&2; exit 1; }
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DSDL_SOURCE_DIR="$repo_dir/extern/SDL"
cmake --build build-web --parallel
echo 'Built build-web/index.html. Serve with: python3 web/serve.py'
