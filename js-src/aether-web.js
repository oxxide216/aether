let _aether_eval_compiled = null;
let _aether_eval = null;

function aether_init(init_callback) {
  Module = {
    onRuntimeInitialized: async function() {
      const response = await fetch('app.abc');
      const content = await response.blob();
      const array = new Uint8Array(await content.arrayBuffer());

      const aether_create = Module.cwrap('emscripten_create', 'null', []);
      aether_create();

      _aether_eval_compiled =
        Module.cwrap('emscripten_eval_compiled', 'string', ['array', 'number']);
      _aether_eval = Module.cwrap('emscripten_eval', 'string', ['string', 'string']);

      aether_eval('(use "std/core.ae")');
      aether_eval('(use "std/base.ae")');
      aether_eval_compiled(array);

      init_callback();
    },
  };

  const script = document.createElement('script');
  script.src = 'aether.js';
  document.head.appendChild(script);
}

function aether_eval_compiled(bytecode) {
  return _aether_eval_compiled(bytecode, bytecode.length);
}

function aether_eval(code) {
  return _aether_eval(code, 'aether-web.js');
}
