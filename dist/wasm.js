
class WasmInterface {
  memory = undefined;
  get mem() {
    return this.memory;
  }
}

async function runWasm(wasmPath, extraForeignImports, memoryDependentImports) {

  const wasmInterface = new WasmInterface()
  let imports = {}
  let exports = {}

  if (extraForeignImports !== undefined) {
    imports = {
      ...imports,
      ...Object.fromEntries(
        Object.entries(extraForeignImports)
          .map(([name, foreignImport]) => [name, foreignImport(wasmInterface)])),
    }
  }

  const response = await fetch(wasmPath)
  const file = await response.arrayBuffer()
  const wasm = await WebAssembly.instantiate(file, imports)

  wasmInterface.memory = wasm.instance.exports.memory

  return wasm
}

