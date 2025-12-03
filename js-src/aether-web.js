let _aetherEvalCompiled = null;
let _aetherEval = null;

function aether_init(aetherJSPath, initCallback) {
  Module = {
    onRuntimeInitialized: async function() {
      const aetherCreate = Module.cwrap('emscripten_create', 'null', []);
      _aetherEvalCompiled =
        Module.cwrap('emscripten_eval_compiled', 'string', ['array', 'number']);
      _aetherEval = Module.cwrap('emscripten_eval', 'string', ['string', 'string']);

      aetherCreate();

      aetherEval('(use "std/core.ae")');
      aetherEval('(use "std/base.ae")');

      const response = await fetch('app.abc');
      const content = await response.blob();
      const array = new Uint8Array(await content.arrayBuffer());

      aetherEvalCompiled(array);

      initCallback();
    },
  };

  const script = document.createElement('script');
  script.src = aetherJSPath;
  document.head.appendChild(script);
}

function aetherEvalCompiled(bytecode) {
  return _aetherEvalCompiled(bytecode, bytecode.length);
}

function aetherEval(code) {
  return _aetherEval(code, 'aether-web.js');
}
