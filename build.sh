set -euo pipefail

if [ -f ./.env ]; then
  source ./.env
else
  echo ".env not found"
fi

: ${CC=clang}

linker_flags=""
linker_flags+=" -Wl,--no-entry -Wl,--export=__heap_base"
if [[ $CC == "zig cc" ]]; then
  linker_flags+=" -Wl,--import-symbols"
else
  linker_flags+=" -Wl,--import-undefined"
fi

linker_flags+=" -Wl,--export=main"
linker_flags+=" -Wl,--export=fds"

flags=""
flags+=" -g -fno-builtin --no-standard-libraries"
flags+=" --target=wasm32-freestanding "
flags+=" $linker_flags"

$CC -o dist/out.wasm src/platform_wasm.c $flags

