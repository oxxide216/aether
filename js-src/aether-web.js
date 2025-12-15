let _aetherEvalCompiled = null;
let _aetherEvalMacros = null;
let _aetherEval = null;

let aetherI = 0;

async function fetchBinaryFile(path) {
  const response = await fetch(path);
  const content = await response.blob();
  return new Uint8Array(await content.arrayBuffer());
}

async function aetherInit(dataPrefix, initCallback) {
  const appArray = await fetchBinaryFile(dataPrefix + '/app.abc');

  Module = {
    onRuntimeInitialized: () => {
      const aetherCreate = Module.cwrap('emscripten_create', 'null', []);
      _aetherEvalCompiled =
        Module.cwrap('emscripten_eval_compiled', 'string', ['array', 'number']);
      _aetherEvalMacros =
        Module.cwrap('emscripten_eval_macros', 'null', ['string', 'number']);
      _aetherEval = Module.cwrap('emscripten_eval', 'string', ['string', 'string']);

      aetherCreate();
      aetherEvalCompiled(appArray);

      initCallback();
    },
  };

  const script = document.createElement('script');
  script.src = dataPrefix + '/aether.js';
  document.head.appendChild(script);
}

function aetherEvalCompiled(bytecode) {
  return _aetherEvalCompiled(bytecode, bytecode.length);
}

function aetherEvalMacros(macro_bytecode) {
  return _aetherEvalMacros(macro_bytecode, macro_bytecode.length);
}

function aetherEval(code) {
  return _aetherEval(code, 'aether-web:' + aetherI++);
}
