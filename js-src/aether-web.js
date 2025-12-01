let _aether_eval = null;
let _aether_eval_compiled = null;

function aether_init() {
  Module = {
    onRuntimeInitialized: async function() {
      const response = await fetch('app.abc');
      const content = await response.text();

      FS.writeFile('dest/app.abc', content);

      const aether_create = Module.cwrap('emscripten_create', 'number', ['string']);
      aether_create('dest/app.abc');

      _aether_eval = Module.cwrap('emscripten_eval', 'string', ['string', 'string']);
      _aether_eval_compiled = Module.cwrap('emscripten_eval_compiled', 'string', ['string']);

      aether_eval('(use "std/core.ae")');
      aether_eval('(use "std/base.ae")');
    },
  };

  const script = document.createElement('script');
  script.src = 'aether.js';
  document.head.appendChild(script);
}

function aether_eval(code) {
  return _aether_eval(code, 'aether-web.js');
}

function aether_eval_compiled(bytecode) {
  return _aether_eval_compiled(bytecode);
}
