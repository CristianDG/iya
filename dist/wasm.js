async function runWasm(wasmPath, extraForeignImports) {

  let imports = {}
  let exports = {}

  if (extraForeignImports !== undefined) {
    imports = {
      ...imports,
      ...Object.fromEntries(
        Object.entries(extraForeignImports)
          .map(([name, foreignImport]) => [name, foreignImport])),
    }
  }

  const response = await fetch(wasmPath)
  const file = await response.arrayBuffer()
  const wasm = await WebAssembly.instantiate(file, imports)

  return wasm
}
