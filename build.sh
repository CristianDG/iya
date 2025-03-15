set -euo pipefail

if [ -f ./.env ]; then
  source ./.env
else
  echo ".env not found"
fi

linker_flags=""
linker_flags+=" -Wl,--no-entry -Wl,--export=__heap_base -Wl,--import-symbols"
linker_flags+=" -Wl,--export=main"
linker_flags+=" -Wl,--export=fds"

flags=""
flags+=" -g -fno-builtin --no-standard-libraries"
flags+=" --target=wasm32-freestanding "
flags+=" $linker_flags"

$CC -o dist/out.wasm src/platform_wasm.c $flags

