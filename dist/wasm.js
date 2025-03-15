async function runWasm(wasmPath, consoleElement, extraForeignImports, wasmMemoryInterface, intSize = 4) {

  let imports = {}
  let exports = {}

  if (extraForeignImports !== undefined) {
    imports = {
      ...imports,
      ...Object.fromEntries(
        Object.entries(extraForeignImports)
          .map(([name, foreignImport]) => [name, foreignImport(wasmMemoryInterface)])),
    }
  }

  const response = await fetch(wasmPath)
  const file = await response.arrayBuffer()
  const wasm = await WebAssembly.instantiate(file, imports)
  exports = wasm.instance.exports

  return wasm
}
