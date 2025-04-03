set -euo pipefail

: ${CC=clang}
: ${LINKER="wasm-ld"}

flags=""
flags+=" -std=c11"

flags+=" -Wall -Werror"
flags+=" -Wno-unused-variable"

flags+=" -DDG_PLATFORM_WASM"
flags+=" -Ithird_party"
flags+=" -g -fno-builtin --no-standard-libraries"
flags+=" -gdwarf-5 -gpubnames"
flags+=" --target=wasm32-freestanding "
# flags+=" -gsplit-dwarf"

linker_flags=""
linker_flags+=" -Wl,--no-entry -Wl,--export=__heap_base -Wl,--export=__heap_end"
linker_flags+=" -Wl,--import-undefined"
linker_flags+=" -Wl,--export-all"


$CC -c -o dist/out.o src/platform_wasm.c $flags

$LINKER -o dist/out.wasm dist/out.o ${linker_flags//-Wl,/}

rm dist/out.o
