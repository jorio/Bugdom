#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
node --test web/tests/*.test.cjs
test_binary=$(mktemp "${TMPDIR:-/tmp}/bugdom-texture-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT
"${CC:-cc}" -std=c17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iweb/common web/common/WebTexture.c web/tests/texture_test.c -o "$test_binary"
"$test_binary"

python3 web/tests/pwa_test.py
