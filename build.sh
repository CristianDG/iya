set -euo pipefail

: ${CC=clang}
: ${LINKER="wasm-ld"}

flags=""
flags+=" -DDG_PLATFORM_WASM"
flags+=" -Ithird_party"
flags+=" -g -fno-builtin --no-standard-libraries"
flags+=" -gdwarf-5 -gpubnames"
# flags+=" -gsplit-dwarf"
flags+=" --target=wasm32-freestanding "

linker_flags=""
linker_flags+=" -Wl,--no-entry -Wl,--export=__heap_base"
linker_flags+=" -Wl,--import-undefined"
linker_flags+=" -Wl,--export-all"


$CC -c -o dist/out.o src/platform_wasm.c $flags

$LINKER -o dist/out.wasm dist/out.o ${linker_flags//-Wl,/}

rm dist/out.o
