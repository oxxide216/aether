let _aether_eval = null;

function aether_init() {
  Module = {
    onRuntimeInitialized: async function() {
      const response = await fetch('app.abc');
      const content = await response.text();

      FS.writeFile('dest/app.abc', content);

      const aether_create = Module.cwrap('emscripten_create', 'number', ['string']);
      aether_create('dest/app.abc');

      _aether_eval = Module.cwrap('emscripten_eval', 'null', ['string', 'string']);

      aether_eval('(use "std/core.ae")');
      aether_eval('(use "std/base.ae")');
    },
  };

  const script = document.createElement('script');
  script.src = 'aether.js';
  document.head.appendChild(script);
}

function aether_eval(code) {
  _aether_eval(code, 'aether-web.js');
}
