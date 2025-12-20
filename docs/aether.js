// include: shell.js
// include: minimum_runtime_check.js
(function() {
  // "30.0.0" -> 300000
  function humanReadableVersionToPacked(str) {
    str = str.split('-')[0]; // Remove any trailing part from e.g. "12.53.3-alpha"
    var vers = str.split('.').slice(0, 3);
    while(vers.length < 3) vers.push('00');
    vers = vers.map((n, i, arr) => n.padStart(2, '0'));
    return vers.join('');
  }
  // 300000 -> "30.0.0"
  var packedVersionToHumanReadable = n => [n / 10000 | 0, (n / 100 | 0) % 100, n % 100].join('.');

  var TARGET_NOT_SUPPORTED = 2147483647;

  // Note: We use a typeof check here instead of optional chaining using
  // globalThis because older browsers might not have globalThis defined.
  var currentNodeVersion = typeof process !== 'undefined' && process.versions?.node ? humanReadableVersionToPacked(process.versions.node) : TARGET_NOT_SUPPORTED;
  if (currentNodeVersion < TARGET_NOT_SUPPORTED) {
    throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');
  }
  if (currentNodeVersion < 2147483647) {
    throw new Error(`This emscripten-generated code requires node v${ packedVersionToHumanReadable(2147483647) } (detected v${packedVersionToHumanReadable(currentNodeVersion)})`);
  }

  var userAgent = typeof navigator !== 'undefined' && navigator.userAgent;
  if (!userAgent) {
    return;
  }

  var currentSafariVersion = userAgent.includes("Safari/") && userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/) ? humanReadableVersionToPacked(userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentSafariVersion < TARGET_NOT_SUPPORTED) {
    throw new Error(`This page was compiled without support for Safari browser. Pass -sMIN_SAFARI_VERSION=${currentSafariVersion} or lower to enable support for this browser.`);
  }
  if (currentSafariVersion < 2147483647) {
    throw new Error(`This emscripten-generated code requires Safari v${ packedVersionToHumanReadable(2147483647) } (detected v${currentSafariVersion})`);
  }

  var currentFirefoxVersion = userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentFirefoxVersion < 129) {
    throw new Error(`This emscripten-generated code requires Firefox v129 (detected v${currentFirefoxVersion})`);
  }

  var currentChromeVersion = userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentChromeVersion < 128) {
    throw new Error(`This emscripten-generated code requires Chrome v128 (detected v${currentChromeVersion})`);
  }
})();

// end include: minimum_runtime_check.js
// The Module object: Our interface to the outside world. We import
// and export values on it. There are various ways Module can be used:
// 1. Not defined. We create it here
// 2. A function parameter, function(moduleArg) => Promise<Module>
// 3. pre-run appended it, var Module = {}; ..generated code..
// 4. External script tag defines var Module.
// We need to check if Module already exists (e.g. case 3 above).
// Substitution will be replaced with actual code on later stage of the build,
// this way Closure Compiler will not mangle it (e.g. case 4. above).
// Note that if you want to run closure, and also to use Module
// after the generated code, you will need to define   var Module = {};
// before the code. Then that object will be used in the code, and you
// can continue to use Module afterwards as well.
var Module = typeof Module != 'undefined' ? Module : {};

// Determine the runtime environment we are in. You can customize this by
// setting the ENVIRONMENT setting at compile time (see settings.js).

var ENVIRONMENT_IS_WEB = true;
var ENVIRONMENT_IS_WORKER = false;
var ENVIRONMENT_IS_NODE = false;
var ENVIRONMENT_IS_SHELL = false;

// --pre-jses are emitted after the Module integration code, so that they can
// refer to Module (if they choose; they can also define Module)


var arguments_ = [];
var thisProgram = './this.program';
var quit_ = (status, toThrow) => {
  throw toThrow;
};

// In MODULARIZE mode _scriptName needs to be captured already at the very top of the page immediately when the page is parsed, so it is generated there
// before the page load. In non-MODULARIZE modes generate it here.
var _scriptName = globalThis.document?.currentScript?.src;

// `/` should be present at the end if `scriptDirectory` is not empty
var scriptDirectory = '';
function locateFile(path) {
  if (Module['locateFile']) {
    return Module['locateFile'](path, scriptDirectory);
  }
  return scriptDirectory + path;
}

// Hooks that are implemented differently in different runtime environments.
var readAsync, readBinary;

if (ENVIRONMENT_IS_SHELL) {

} else

// Note that this includes Node.js workers when relevant (pthreads is enabled).
// Node.js workers are detected as a combination of ENVIRONMENT_IS_WORKER and
// ENVIRONMENT_IS_NODE.
if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  try {
    scriptDirectory = new URL('.', _scriptName).href; // includes trailing slash
  } catch {
    // Must be a `blob:` or `data:` URL (e.g. `blob:http://site.com/etc/etc`), we cannot
    // infer anything from them.
  }

  if (!(globalThis.window || globalThis.WorkerGlobalScope)) throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');

  {
// include: web_or_worker_shell_read.js
readAsync = async (url) => {
    assert(!isFileURI(url), "readAsync does not work with file:// URLs");
    var response = await fetch(url, { credentials: 'same-origin' });
    if (response.ok) {
      return response.arrayBuffer();
    }
    throw new Error(response.status + ' : ' + response.url);
  };
// end include: web_or_worker_shell_read.js
  }
} else
{
  throw new Error('environment detection error');
}

var out = console.log.bind(console);
var err = console.error.bind(console);

var IDBFS = 'IDBFS is no longer included by default; build with -lidbfs.js';
var PROXYFS = 'PROXYFS is no longer included by default; build with -lproxyfs.js';
var WORKERFS = 'WORKERFS is no longer included by default; build with -lworkerfs.js';
var FETCHFS = 'FETCHFS is no longer included by default; build with -lfetchfs.js';
var ICASEFS = 'ICASEFS is no longer included by default; build with -licasefs.js';
var JSFILEFS = 'JSFILEFS is no longer included by default; build with -ljsfilefs.js';
var OPFS = 'OPFS is no longer included by default; build with -lopfs.js';

var NODEFS = 'NODEFS is no longer included by default; build with -lnodefs.js';

// perform assertions in shell.js after we set up out() and err(), as otherwise
// if an assertion fails it cannot print the message

assert(!ENVIRONMENT_IS_WORKER, 'worker environment detected but not enabled at build time.  Add `worker` to `-sENVIRONMENT` to enable.');

assert(!ENVIRONMENT_IS_NODE, 'node environment detected but not enabled at build time.  Add `node` to `-sENVIRONMENT` to enable.');

assert(!ENVIRONMENT_IS_SHELL, 'shell environment detected but not enabled at build time.  Add `shell` to `-sENVIRONMENT` to enable.');

// end include: shell.js

// include: preamble.js
// === Preamble library stuff ===

// Documentation for the public APIs defined in this file must be updated in:
//    site/source/docs/api_reference/preamble.js.rst
// A prebuilt local version of the documentation is available at:
//    site/build/text/docs/api_reference/preamble.js.txt
// You can also build docs locally as HTML or other formats in site/
// An online HTML version (which may be of a different version of Emscripten)
//    is up at http://kripken.github.io/emscripten-site/docs/api_reference/preamble.js.html

var wasmBinary;

if (!globalThis.WebAssembly) {
  err('no native wasm support detected');
}

// Wasm globals

//========================================
// Runtime essentials
//========================================

// whether we are quitting the application. no code should run after this.
// set in exit() and abort()
var ABORT = false;

// set by exit() and abort().  Passed to 'onExit' handler.
// NOTE: This is also used as the process return code code in shell environments
// but only when noExitRuntime is false.
var EXITSTATUS;

// In STRICT mode, we only define assert() when ASSERTIONS is set.  i.e. we
// don't define it at all in release modes.  This matches the behaviour of
// MINIMAL_RUNTIME.
// TODO(sbc): Make this the default even without STRICT enabled.
/** @type {function(*, string=)} */
function assert(condition, text) {
  if (!condition) {
    abort('Assertion failed' + (text ? ': ' + text : ''));
  }
}

// We used to include malloc/free by default in the past. Show a helpful error in
// builds with assertions.

/**
 * Indicates whether filename is delivered via file protocol (as opposed to http/https)
 * @noinline
 */
var isFileURI = (filename) => filename.startsWith('file://');

// include: runtime_common.js
// include: runtime_stack_check.js
// Initializes the stack cookie. Called at the startup of main and at the startup of each thread in pthreads mode.
function writeStackCookie() {
  var max = _emscripten_stack_get_end();
  assert((max & 3) == 0);
  // If the stack ends at address zero we write our cookies 4 bytes into the
  // stack.  This prevents interference with SAFE_HEAP and ASAN which also
  // monitor writes to address zero.
  if (max == 0) {
    max += 4;
  }
  // The stack grow downwards towards _emscripten_stack_get_end.
  // We write cookies to the final two words in the stack and detect if they are
  // ever overwritten.
  HEAPU32[((max)/4)] = 0x02135467;
  HEAPU32[(((max)+(4))/4)] = 0x89BACDFE;
  // Also test the global address 0 for integrity.
  HEAPU32[((0)/4)] = 1668509029;
}

function checkStackCookie() {
  if (ABORT) return;
  var max = _emscripten_stack_get_end();
  // See writeStackCookie().
  if (max == 0) {
    max += 4;
  }
  var cookie1 = HEAPU32[((max)/4)];
  var cookie2 = HEAPU32[(((max)+(4))/4)];
  if (cookie1 != 0x02135467 || cookie2 != 0x89BACDFE) {
    abort(`Stack overflow! Stack cookie has been overwritten at ${ptrToString(max)}, expected hex dwords 0x89BACDFE and 0x2135467, but received ${ptrToString(cookie2)} ${ptrToString(cookie1)}`);
  }
  // Also test the global address 0 for integrity.
  if (HEAPU32[((0)/4)] != 0x63736d65 /* 'emsc' */) {
    abort('Runtime error: The application has corrupted its heap memory area (address zero)!');
  }
}
// end include: runtime_stack_check.js
// include: runtime_exceptions.js
// end include: runtime_exceptions.js
// include: runtime_debug.js
var runtimeDebug = true; // Switch to false at runtime to disable logging at the right times

// Used by XXXXX_DEBUG settings to output debug messages.
function dbg(...args) {
  if (!runtimeDebug && typeof runtimeDebug != 'undefined') return;
  // TODO(sbc): Make this configurable somehow.  Its not always convenient for
  // logging to show up as warnings.
  console.warn(...args);
}

// Endianness check
(() => {
  var h16 = new Int16Array(1);
  var h8 = new Int8Array(h16.buffer);
  h16[0] = 0x6373;
  if (h8[0] !== 0x73 || h8[1] !== 0x63) abort('Runtime error: expected the system to be little-endian! (Run with -sSUPPORT_BIG_ENDIAN to bypass)');
})();

function consumedModuleProp(prop) {
  if (!Object.getOwnPropertyDescriptor(Module, prop)) {
    Object.defineProperty(Module, prop, {
      configurable: true,
      set() {
        abort(`Attempt to set \`Module.${prop}\` after it has already been processed.  This can happen, for example, when code is injected via '--post-js' rather than '--pre-js'`);

      }
    });
  }
}

function makeInvalidEarlyAccess(name) {
  return () => assert(false, `call to '${name}' via reference taken before Wasm module initialization`);

}

function ignoredModuleProp(prop) {
  if (Object.getOwnPropertyDescriptor(Module, prop)) {
    abort(`\`Module.${prop}\` was supplied but \`${prop}\` not included in INCOMING_MODULE_JS_API`);
  }
}

// forcing the filesystem exports a few things by default
function isExportedByForceFilesystem(name) {
  return name === 'FS_createPath' ||
         name === 'FS_createDataFile' ||
         name === 'FS_createPreloadedFile' ||
         name === 'FS_preloadFile' ||
         name === 'FS_unlink' ||
         name === 'addRunDependency' ||
         // The old FS has some functionality that WasmFS lacks.
         name === 'FS_createLazyFile' ||
         name === 'FS_createDevice' ||
         name === 'removeRunDependency';
}

/**
 * Intercept access to a symbols in the global symbol.  This enables us to give
 * informative warnings/errors when folks attempt to use symbols they did not
 * include in their build, or no symbols that no longer exist.
 *
 * We don't define this in MODULARIZE mode since in that mode emscripten symbols
 * are never placed in the global scope.
 */
function hookGlobalSymbolAccess(sym, func) {
  if (!Object.getOwnPropertyDescriptor(globalThis, sym)) {
    Object.defineProperty(globalThis, sym, {
      configurable: true,
      get() {
        func();
        return undefined;
      }
    });
  }
}

function missingGlobal(sym, msg) {
  hookGlobalSymbolAccess(sym, () => {
    warnOnce(`\`${sym}\` is no longer defined by emscripten. ${msg}`);
  });
}

missingGlobal('buffer', 'Please use HEAP8.buffer or wasmMemory.buffer');
missingGlobal('asm', 'Please use wasmExports instead');

function missingLibrarySymbol(sym) {
  hookGlobalSymbolAccess(sym, () => {
    // Can't `abort()` here because it would break code that does runtime
    // checks.  e.g. `if (typeof SDL === 'undefined')`.
    var msg = `\`${sym}\` is a library symbol and not included by default; add it to your library.js __deps or to DEFAULT_LIBRARY_FUNCS_TO_INCLUDE on the command line`;
    // DEFAULT_LIBRARY_FUNCS_TO_INCLUDE requires the name as it appears in
    // library.js, which means $name for a JS name with no prefix, or name
    // for a JS name like _name.
    var librarySymbol = sym;
    if (!librarySymbol.startsWith('_')) {
      librarySymbol = '$' + sym;
    }
    msg += ` (e.g. -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='${librarySymbol}')`;
    if (isExportedByForceFilesystem(sym)) {
      msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
    }
    warnOnce(msg);
  });

  // Any symbol that is not included from the JS library is also (by definition)
  // not exported on the Module object.
  unexportedRuntimeSymbol(sym);
}

function unexportedRuntimeSymbol(sym) {
  if (!Object.getOwnPropertyDescriptor(Module, sym)) {
    Object.defineProperty(Module, sym, {
      configurable: true,
      get() {
        var msg = `'${sym}' was not exported. add it to EXPORTED_RUNTIME_METHODS (see the Emscripten FAQ)`;
        if (isExportedByForceFilesystem(sym)) {
          msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
        }
        abort(msg);
      },
    });
  }
}

// end include: runtime_debug.js
// include: binaryDecode.js
// Prevent Closure from minifying the binaryDecode() function, or otherwise
// Closure may analyze through the WASM_BINARY_DATA placeholder string into this
// function, leading into incorrect results.
/** @noinline */
function binaryDecode(bin) {
  for (var i = 0, l = bin.length, o = new Uint8Array(l), c; i < l; ++i) {
    c = bin.charCodeAt(i);
    o[i] = ~c >> 8 & c; // Recover the null byte in a manner that is compatible with https://crbug.com/453961758
  }
  return o;
}
// end include: binaryDecode.js
// Memory management
var
/** @type {!Int8Array} */
  HEAP8,
/** @type {!Uint8Array} */
  HEAPU8,
/** @type {!Int16Array} */
  HEAP16,
/** @type {!Uint16Array} */
  HEAPU16,
/** @type {!Int32Array} */
  HEAP32,
/** @type {!Uint32Array} */
  HEAPU32,
/** @type {!Float32Array} */
  HEAPF32,
/** @type {!Float64Array} */
  HEAPF64;

// BigInt64Array type is not correctly defined in closure
var
/** not-@type {!BigInt64Array} */
  HEAP64,
/* BigUint64Array type is not correctly defined in closure
/** not-@type {!BigUint64Array} */
  HEAPU64;

var runtimeInitialized = false;



function updateMemoryViews() {
  var b = wasmMemory.buffer;
  HEAP8 = new Int8Array(b);
  HEAP16 = new Int16Array(b);
  Module['HEAPU8'] = HEAPU8 = new Uint8Array(b);
  HEAPU16 = new Uint16Array(b);
  HEAP32 = new Int32Array(b);
  HEAPU32 = new Uint32Array(b);
  HEAPF32 = new Float32Array(b);
  HEAPF64 = new Float64Array(b);
  HEAP64 = new BigInt64Array(b);
  HEAPU64 = new BigUint64Array(b);
}

// include: memoryprofiler.js
// end include: memoryprofiler.js
// end include: runtime_common.js
assert(globalThis.Int32Array && globalThis.Float64Array && Int32Array.prototype.subarray && Int32Array.prototype.set,
       'JS engine does not provide full typed array support');

function preRun() {
  if (Module['preRun']) {
    if (typeof Module['preRun'] == 'function') Module['preRun'] = [Module['preRun']];
    while (Module['preRun'].length) {
      addOnPreRun(Module['preRun'].shift());
    }
  }
  consumedModuleProp('preRun');
  // Begin ATPRERUNS hooks
  callRuntimeCallbacks(onPreRuns);
  // End ATPRERUNS hooks
}

function initRuntime() {
  assert(!runtimeInitialized);
  runtimeInitialized = true;

  checkStackCookie();

  // Begin ATINITS hooks
  SOCKFS.root = FS.mount(SOCKFS, {}, null);
if (!Module['noFSInit'] && !FS.initialized) FS.init();
TTY.init();
  // End ATINITS hooks

  wasmExports['__wasm_call_ctors']();

  // Begin ATPOSTCTORS hooks
  FS.ignorePermissions = false;
  // End ATPOSTCTORS hooks
}

function postRun() {
  checkStackCookie();
   // PThreads reuse the runtime from the main thread.

  if (Module['postRun']) {
    if (typeof Module['postRun'] == 'function') Module['postRun'] = [Module['postRun']];
    while (Module['postRun'].length) {
      addOnPostRun(Module['postRun'].shift());
    }
  }
  consumedModuleProp('postRun');

  // Begin ATPOSTRUNS hooks
  callRuntimeCallbacks(onPostRuns);
  // End ATPOSTRUNS hooks
}

/** @param {string|number=} what */
function abort(what) {
  Module['onAbort']?.(what);

  what = 'Aborted(' + what + ')';
  // TODO(sbc): Should we remove printing and leave it up to whoever
  // catches the exception?
  err(what);

  ABORT = true;

  // Use a wasm runtime error, because a JS error might be seen as a foreign
  // exception, which means we'd run destructors on it. We need the error to
  // simply make the program stop.
  // FIXME This approach does not work in Wasm EH because it currently does not assume
  // all RuntimeErrors are from traps; it decides whether a RuntimeError is from
  // a trap or not based on a hidden field within the object. So at the moment
  // we don't have a way of throwing a wasm trap from JS. TODO Make a JS API that
  // allows this in the wasm spec.

  // Suppress closure compiler warning here. Closure compiler's builtin extern
  // definition for WebAssembly.RuntimeError claims it takes no arguments even
  // though it can.
  // TODO(https://github.com/google/closure-compiler/pull/3913): Remove if/when upstream closure gets fixed.
  /** @suppress {checkTypes} */
  var e = new WebAssembly.RuntimeError(what);

  // Throw the error whether or not MODULARIZE is set because abort is used
  // in code paths apart from instantiation where an exception is expected
  // to be thrown when abort is called.
  throw e;
}

function createExportWrapper(name, nargs) {
  return (...args) => {
    assert(runtimeInitialized, `native function \`${name}\` called before runtime initialization`);
    var f = wasmExports[name];
    assert(f, `exported native function \`${name}\` not found`);
    // Only assert for too many arguments. Too few can be valid since the missing arguments will be zero filled.
    assert(args.length <= nargs, `native function \`${name}\` called with ${args.length} args but expects ${nargs}`);
    return f(...args);
  };
}

var wasmBinaryFile;

function findWasmBinary() {
  return binaryDecode(' asm   ¹N`~~~`~`~~~~`~~~`~~~`~|`~~ ` `~~~`~~~~`~ `~~~~`~``~`~~`~~~`~~`~`~~`~`~~`~`  `~~``~~~~`~~`~|`~ `~| `~~`~~~~ `~~~~ `~~~~~ `~~~~`~~~~~~ `~~~ `~~~ `|~~`~~`~~~~~`~~~~`~~~ `~~ `	~~~~~~ `~~~~~`\n~~~~~~ `~~~~ `~~~ ` ~`~~~~~~ `~ `~~`~`~~~`~~~~~`~~`~~`~`~~`~~~` `~~`||`|~|`~~~~~`~~~~~~~`~ `|~`~~~ `~~|``~~~`~~~~~`~~~~`~`ñ\n.envexit envemscripten_asm_const_int envgetaddrinfo 	envemscripten_asm_const_ptr envemscripten_console_log \nenvemscripten_console_warn \nenvemscripten_console_error \nenv*emscripten_set_keypress_callback_on_thread env)emscripten_set_keydown_callback_on_thread env\'emscripten_set_keyup_callback_on_thread env\'emscripten_set_click_callback_on_thread env+emscripten_set_mousedown_callback_on_thread env)emscripten_set_mouseup_callback_on_thread env*emscripten_set_dblclick_callback_on_thread env+emscripten_set_mousemove_callback_on_thread env,emscripten_set_mouseenter_callback_on_thread env,emscripten_set_mouseleave_callback_on_thread env__syscall_faccessat env__syscall_chdir wasi_snapshot_preview1fd_close \renv__syscall_fcntl64 env__syscall_openat env__syscall_ioctl wasi_snapshot_preview1fd_write wasi_snapshot_preview1fd_read env__syscall_getcwd wasi_snapshot_preview1fd_seek env__syscall_fstat64 env__syscall_stat64 env__syscall_newfstatat env__syscall_lstat64 env__syscall_poll envemscripten_err \nenv__syscall_getdents64 env__syscall_readlinkat env__syscall_unlinkat env__syscall_rmdir env	_abort_js envemscripten_resize_heap env__syscall_accept4 env__syscall_bind env__syscall_connect env__syscall_listen env__syscall_recvfrom env__syscall_sendto env__syscall_socket ïí  !"!" #"$%!$#\n\n  &   \'(   \n\n\n)*\n\n ""+&,&&-.&/&0&!!&!1\n23&&4 	\n                                                                                                             5"	26\r\r\n\n7( 89998 \r\r:\n\n;<=259>?>2 @	  7 ABC&+  DE	\r76\n   2"222FFG\nHIJIKLM\n2(;\r\r\r\rpzz ~B~B ~B ~ B°~ BÔmemory __wasm_call_ctors .malloc öemscripten_create :emscripten_eval_compiled ;free øemscripten_eval_macros =emscripten_eval >emscripten_destroy ?__indirect_function_table htons ³fflush htonl ntohs emscripten_stack_get_end emscripten_stack_get_base strerror emscripten_stack_init emscripten_stack_get_free _emscripten_stack_restore _emscripten_stack_alloc emscripten_stack_get_current __start_em_asm\r__stop_em_asm	ø By ¡£¤¥¦¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÝÞßàáèâäåæçéêëìíîïðñòóôõöýþ÷ùúûüÿ ÕÖíîñ\nµí	í ËO~# B}! $   7   )7    )Ü §6  B|A 6  B|$ È~# B}!@@  ( (GAqE\r  A Aq:  A 6@@ (  (HAqE\r  )  (¬|-  !A!  t u! )  (¬|-  !A!@   t uGAqE\r  A Aq:   (Aj6  AAq:  - Aqÿ~~# B }! B 7@@  (\r  B 7   ) ,  A-F: @ - AqE\r     ) B|7     (Aj6 A 6@@ (  (HAqE\r  )B\n~7  )  (¬|-  !A!   t uA0k¬ )|7  (Aj6 @ - AqE\r  )! B  }7  )7 )Ã~\n# B }! B 7 A 6   ) ,  A-F: @ - AqE\r     ) B|7     (Aj6@ (  (H!A ! Aq! !@ E\r   )  (¬|-  !A!  t uA.G!@ AqE\r   +D      $@¢9  )  (¬|-  !A!	   	t 	uA0k· + 9  (Aj6@ (  (HAqE\r   (Aj6 D      ð?9 @@ (  (HAqE\r  + D      $@¢9   )  (¬|-  !\nA!  \n t uA0k· + £ + 9  (Aj6 @@ - AqE\r   +9  +9 +~~# B}! $    7  6@ ( )( )(kKAqE\r @@ )(E\r @@ ( )( )(kKAqE\r )!  (At6  ))  )(Aj­ù ! ) 7  (! )!   (j6 )(Aj­ö ! ) 7  B|$ !    ) 7    (6  B|A 6 x~~# B}! $    7  :  )A³  - ! )) ! )! (!  Aj6  ­| :   B|$ j~# B0}! $    7(  7  )(! ) ! B| ¯   )7  )7   ·  B0|$ ~~# B}! $    7 ) (³  ))  )(­|! ) ! (­!@ P\r    ü\n   (! )!   (j6 B|$ ~~# B0}! $    7(  7   ) 7 A6@ )B SAqE\r   )B~7  (Aj6@@ )B\nYAqE\r  )B\n7  (Aj6  )( (³  )()  )((­|! (Aj­!  ) 7   Bã  Ó  (! )(!   (j6 B0|$ ¾~~# B0}! $    7(  9   + 9 A6@@ +D      $@fAqE\r  +D      $@£9  (Aj6   (Aj6@@ + +ü¹¡B ¹dAqE\r  +D      $@¢9  (Aj6  )( (³  )()  )((­|! (Aj­!  + 9   Bö  Ó  (! )(!   (j6 B0|$ ¥~~# B }!   $   A6B !   )ø 7   )ð 7B !   7x   7p  (!  B|!      Bð |ì BÈ   Bð ü\n    B |$ ö~~# BÀ }! $    78  64B !  )° 7(  )¨ 7  B 7 )8! (4! B|   B|BÈ Bà |À  BÈ  B|AAqå 7  )(!B !  7°   ) 7¨  ) ¼ ! BÀ |$  ¦	~~~~~# B }! $    7B !  7  7 )! B|!A !A !BÈ !    Aq    (Aj­ö 7  ) ! )!	 (­!\n@ \nP\r   	 \nü\n   )  (­|A :  @B (Ä B (À MAqE\r @@B (Ä E\r @@B (Ä B (À MAqE\rB (Ä At!B  6Ä  B )¸ B (Ä ­Bù !B  7¸ A!\rB  \r6Ä Bö !B  7¸  ) !B )¸ B (À ­B| 7 B (À Aj!B  6À  )ø  ) ! B |$  ~~# B }! $    7  6 )! (!   B B Å @B (¤ B (   (jIAqE\r B (   (j!B  6¤ @@B (  \r B (¤ ­Bö !B  7 B ) B (¤ ­Bù !B  7 B ) B (  ­B|! ) !	 (­B!\n@ \nP\r   	 \nü\n   (B (  j!B  6   B |$ ü	~~~~~# Bà }! $    7X  7P B AÍ 7H )PÜ §! )H 6 )H(!B  Í ! )H 7  )H) ! )P! )H(­!@ P\r    ü\n    )X7(  )XÜ §60 B(|B|A 6  )H!	 B8|  )07  )(7 B !\nB¨ !B !A !\r B8|  	   \n \r  )H! B8|!B !B !A !B !A ! Aq!A!  t u!A!          t uñ B !  )° 7   )¨ 7 )H! )!B !  7°   ) 7¨  BÈ  B8|AAqå 7 ) !B !  7°   )7¨  )¼ ! Bà |$  ¥~# B0}!   $ @B )à B RAqE\r B )à ø   A 6,@@  (,B (ø IAqE\rB )ð   (,­B|B8|Ï     (,Aj6, @B )ð B RAqE\r B )ð ø B ) ø B Ï B ) ø   B 7  A 6   A 6$  ) !B !  7     )7 B )¨ ø   B 7  A 6  A 6  )!B !  7°    )7¨ BÈ Þ   A 6@@  (B (À IAqE\rB )¸   (­B|) ø     (Aj6 B )¸ ø   B0|$ Ö~~# B°}! $   7¨  6¤  7  7B !   7   7 @ (¤­BTAqE\r B )  ! Aï6  B  £ B )  Bõ B £ A    )¨7 A6 B|B|A 6  B 7p A6x Bð |B|A 6   )7P  )7H  )x7@  )p78@ BÈ | B8|° Aq\r B )  ! Aô60 B  B0|£ B )  BÌ B £ A    )¨(6l@ (¤ (lGAqE\r B )  !	 Aû6 	B  B|£ B )  !\n (l!  (¤6$  6  \nB¯  B |£ A   A6hB !  7`  7X )¨!\r )! BØ | \r Bè | Á  )! )X!  )7  ) 7  )¨! )!    Bè | BØ | Â  B°|$ ©~~# B0}! $    7(  7   7  7 )  )( ­|( ! )( 6 )((! )( 6 )!  ( ­B|§6  ) )((­B~§Í ! )( 7  A 6@@ ( )((IAqE\r )( !	 )()  (­B~| 	6 )()  (­B~| )  ) )Ã   (Aj6  B0|$ ¨~~# B0}! $    7(  7   7  7  7 )  )( ­|( ! )( 6 )!  ( ­B|§6  ) )((­B§Í ! )( 7  A 6@@ ( )((IAqE\r )AÒ Í !	 )()  (­B| 	7  )()  (­B|)  )  ) ) )Ä   (Aj6  B0|$ ~~~# B0}! $    7(  7   7  7 )  )( ­|( ! )( 6 )!  ( ­B|§6  ) )((Í ! )( 7  A 6@@ ( )((IAqE\r )  )( ­|-  ! )()  (­| :   )!	 	 	( Aj6   (Aj6  B0|$ ­~~~~~|~~~~\n~~~# Bð }! $    7h  7`  7X  7P  7H )` )X5 |-  ! )h :   )X!  5 B|>  )h1  !@@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  )hB| )` )X )P )HÂ  )HAÒ Í !	 )h 	7  )h)  )` )X )P )HÄ  )hB|B| )` )X )P )HÂ  )HAÒ Í !\n )h \n7  )hB| )` )X )HÃ  )h)  )` )X )P )HÄ  )HAÒ Í ! )h 7  )h)  )` )X )P )HÄ  )hB|B| )` )X )P )HÂ  )` )X( ­|( ! )h 6 " )X!\r \r \r( ­B|§6  )H )h( "­B~§Í ! )h 7  A 6D@@ (D )h( "IAqE\rB !  78  70  7(  )HAÒ Í 7( )( )` )X )P )HÄ  B(|B| )` )X )P )HÂ  )h)  (D­B~|!  )87  )07  )(7   (DAj6D  )` )X5 |-  A G! )h : * )X!  5 B|> @ )h- *AqE\r  )hB|B0| )` )X )P )HÂ  )HAÒ Í ! )h 7  )h)  )` )X )P )HÄ  )hB|B| )` )X )P )HÂ  )HAÒ Í ! )h 7  )hB| )` )X )HÃ  )h)  )` )X )P )HÄ  )HAÒ Í ! )h 7  )HAÒ Í ! )h 7 \n )h)  )` )X )P )HÄ  )h) \n )` )X )P )HÄ \r )HAÒ Í ! )h 7  )HAÒ Í ! )h 7  )hB| )` )X )HÃ  )h)  )` )X )P )HÄ  )h)  )` )X )P )HÄ  )` )X5 |-  A G! )h :  )X!  5 B|> @ )h- AqE\r  )HAÒ Í ! )h 7 \n )h) \n )` )X )P )HÄ  )hB| )` )X )P )HÂ \n )hB| )` )X )HÃ 	 )hB| )` )X )HÃ  )` )X( ­|) ! )h 7  )X!  ( ­B|§6  )` )X( ­|+ ! )h 9  )X!  ( ­B|§6  )` )X( ­|-  ! A !!  Aÿq !AÿqG!" )h "Aq:  )X!# # #( ­B|§6   )hB|7  )` )X( ­|( !$ )  $6 )X!% % %( ­B|§6  )H ) (­B§Í !& )  &7  A 6@@ ( ) (IAqE\r ) )  (­B| )` )X )HÃ   (Aj6  )hB|B| )` )X )P )HÂ  )hB|B | )` )X )HÃ  )` )X( ­|( !\' )h \'6 \n )X!( ( (( ­B|§6  )H )h( \n­B§Í !) )h )7  A 6@@ ( )h( \nIAqE\r )HAÒ Í !* )h)  (­B| *7  )HAÒ Í !+ )h)  (­B| +7 )h)  (­B|)  )` )X )P )HÄ  )h)  (­B|) )` )X )P )HÄ   (Aj6  )HAÒ Í !, )h ,7  )h)  )` )X )P )HÄ  )` )X( ­|( !- )h -6  )X!. . .( ­B|§6  )H )h( ­B§Í !/ )h /7 \n A 6@@ ( )h( IAqE\r )HAÒ Í !0 )h) \n (­B| 07  )HAÒ Í !1 )h) \n (­B| 17 )h) \n (­B|)  )` )X )P )HÄ  )h) \n (­B|) )` )X )P )HÄ   (Aj6 B )  !2 A¿6  2B  £ B )  Bä B £ A    )` )X( ­|( 6 )X!3 3 3( ­B|§6  )HAÍ !4 )h 47 B )h) B!5 )`!6 )H!7 5 6 B| 7Ã  )` )X( ­|( !8 )h 8; J )X!9 9 9( ­B|§6  )` )X( ­|( !: )h :; L )X!; ; ;( ­B|§6  Bð |$ ¶~~~~~~# Bà}! $   7Ø  6Ô  7È  7ÀB !   7   7 @ (Ô­BTAqE\r B )  ! A6  B  £ B )  Bõ B £ A    )Ø7° A6¸ B°|B|A 6  B 7  A6¨ B |B|A 6   )¸7X  )°7P  )¨7H  ) 7@@ BÐ | BÀ |° Aq\r B )  ! A60 B  B0|£ B )  BÌ B £ A    )Ø(6@ (Ô (GAqE\r B )  !	 A6 	B  B|£ B )  !\n (!  (Ô6$  6  \nB¯  B |£ A   A6B !  7  7 )Ø!\r )À! B| \r B| Á @ )ÈB RAqE\r @ )È( )È( (jIAqE\r  )È( (j! )È 6@@ )È(\r  )È(­Bö ! )È 7  )È)  )È(­Bù ! )È 7  A 6@@ ( (IAqE\r  )ÀAÍ 7x ) (­B~|(! )x 6 )À )x(Í ! )x 7  )x) ! ) (­B~|) ! )x(­!@ P\r    ü\n   )x! )È) ! )È! (!  Aj6  ­B| 7   (Aj6    )Ø (­|( 6    (6  (­B|§6   )À  (­B§Í 7  A 6t@@ (t  (IAqE\r   )  (t­B|7h )h! )Ø! )À!   B| Ã  )Ø (­|( ! )h 6  (­B|§6 )À )h(­B§Í ! )h 7 A 6d@@ (d )h(IAqE\r )h) (d­B|!  )Ø!! )À!"   ! B| "Ã   (dAj6d  )hB |!# )Ø!$ )À!% # $ B| B| %Â  )Ø (­|-  !&A !\' &Aÿq \'AÿqG!( )h (Aq: 0  (­B|§6  (tAj6t  Bà|$ Ð~~~# BÀ }! $    78  70  7(  Aq: \' )0!A!  6   6   5 ö 7@ - \'AqE\r  )8 B !  7  7 )0! )(!	 B| B |  	 B|Ç  )8!\n )0! )() ) ! \n B| B |  B| È @ )B RAqE\r  )ø B (  !\r ) \r6  )0( ! ) 6 )! BÀ |$  à~~# Bà }! $    7X  7P  7H  7@  78 )X! )P! )H!A   É  )@(!	 )X)  )H( ­| 	6  )H!\n \n \n( ­B|§6  A 64@@ (4 )@(IAqE\r B|! )@)  (4­B|) !  )7  ) 7   )H( 6( B|B|A 6 @ )8( )8(MAqE\r @@ )8(E\r @@ )8( )8(MAqE\r )8!\r \r \r(At6  )8)  )8(­B~ù ! )8 7  )8A6Bö ! )8 7  )8)  )8(­B~|!  )(7  ) 7  )7  )8!  (Aj6 )@)  (4­B|) ! )X! )P! )H!  )7  ) 7 B|   Ê   (4Aj64  Bà |$ â~~# BÀ }! $    78  70  7(  7   7  7  ) ( 6 )0! )(! ) !	A   	É  )0)  (­|A 6  ) !\n \n \n( ­B|§6  A 6@@ ( )8(IAqE\r )8)  (­B|) - !A !@ Aÿq AÿqGAq\r  )8)  (­B|)  )0 )( )  ) )Ë  )0)  (­|!\r \r \r( Aj6   (Aj6  BÀ |$ Ê~~# B0}! $    6,  7   7  7  )( 6@@ )(  (,j (KAqE\r  (At6 @ ( )( GAqE\r  (! ) 6  ) )  )( ­ù ! )  7  B0|$ þ~~~# B }! $   7  7  7  (­B|§ ) ) )É   (! ))  )( ­| 6  )!  ( ­B|§6  A 6@@ (  (IAqE\r  )  (­|-  ! ))  )( ­| :   )!  ( ­B|§6   (Aj6  B |$ à~	~~~~|~~~~~~~~~~# B}! $    7ø  7ð  7è  7à  7Ø  7Ð )ð! )è! )à!	A   	É  )ø-  !\n )ð)  )à5 | \n:   )à!  5 B|>  )ø1  !@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  )øB| )ð )è )à )Ø )ÐÈ  )ø)  )ð )è )à )Ø )ÐË  )øB|B| )ð )è )à )Ø )ÐÈ  )øB|!\r )ð! )è! )à!  \r) 7È  \r)  7À  )È7h  )À7` Bà |   Ê  )ø)  )ð )è )à )Ø )ÐË  )ø)  )ð )è )à )Ø )ÐË  )øB|B| )ð )è )à )Ø )ÐÈ  )ð! )è! )à!A   É  )ø( "! )ð)  )à( ­| 6  )à!  ( ­B|§6  A 6¼@@ (¼ )ø( "IAqE\r )ø)  (¼­B~|)  )ð )è )à )Ø )ÐË  )ø)  (¼­B~|B| )ð )è )à )Ø )ÐÈ   (¼Aj6¼  )ð! )è! )à!A!    É   )ø- *q! )ð)  )à5 | :   )à!  5 B|> @ )ø- *AqE\r  )øB|B0| )ð )è )à )Ø )ÐÈ  )ø)  )ð )è )à )Ø )ÐË  )øB|B| )ð )è )à )Ø )ÐÈ  )øB|! )ð! )è! )à!  ) 7°  )  7¨  )°7x  )¨7p Bð |   Ê  )ø)  )ð )è )à )Ø )ÐË \r )ø)  )ð )è )à )Ø )ÐË  )ø) \n )ð )è )à )Ø )ÐË  )øB|!  )ð!! )è!" )à!#   ) 7    )  7  ) 7  )7 B| ! " #Ê  )ø)  )ð )è )à )Ø )ÐË  )ø)  )ð )è )à )Ø )ÐË  )ð!$ )è!% )à!&A!\' \' $ % &É  \' )ø- q!( )ð)  )à5 | (:   )à!) ) )5 B|> @ )ø- AqE\r  )ø) \n )ð )è )à )Ø )ÐË \n )øB| )ð )è )à )Ø )ÐÈ 	 )øB|!* )ð!+ )è!, )à!-  *) 7  *)  7  )7  )7 B| + , -Ê  )øB|!. )ð!/ )è!0 )à!1  .) 7  .)  7ø  )7¨  )ø7  B | / 0 1Ê  )ð!2 )è!3 )à!4A 2 3 4É  )ø) !5 )ð)  )à( ­| 57  )à!6 6 6( ­B|§6  )ð!7 )è!8 )à!9A 7 8 9É  )ø+ !: )ð)  )à( ­| :9  )à!; ; ;( ­B|§6  )ð!< )è!= )à!>A < = >É  )ø- Aq!? )ð)  )à( ­| ?:   )à!@ @ @( ­B|§6  )ð!A )è!B )à!CA A B CÉ  )ø( \n!D )ð)  )à( ­| D6  )à!E E E( ­B|§6  A 6ô@@ (ô )ø( \nIAqE\r )ø)  (ô­B|!F )ð!G )è!H )à!I  F)7¸  F) 7° B°| G H IÊ   (ôAj6ô  )øB|B| )ð )è )à )Ø )ÐÈ  )øB|B |!J )ð!K )è!L )à!M  J) 7è  J)  7à  )è7È  )à7À BÀ| K L MÊ  )ð!N )è!O )à!PA N O PÉ  )ø( \n!Q )ð)  )à( ­| Q6  )à!R R R( ­B|§6  A 6Ü@@ (Ü )ø( \nIAqE\r )ø)  (Ü­B|)  )ð )è )à )Ø )ÐË  )ø)  (Ü­B|) )ð )è )à )Ø )ÐË   (ÜAj6Ü  )ø)  )ð )è )à )Ø )ÐË  )ð!S )è!T )à!UA S T UÉ  )ø( !V )ð)  )à( ­| V6  )à!W W W( ­B|§6  A 6Ø@@ (Ø )ø( IAqE\r )ø) \n (Ø­B|)  )ð )è )à )Ø )ÐË  )ø) \n (Ø­B|) )ð )è )à )Ø )ÐË   (ØAj6Ø  A : × A 6Ð@@ (Ð )Ø(IAqE\r )Ø)  (Ð­B~|!X )ø) B!Y  X)7X  X) 7P  Y)7H  Y) 7@@ BÐ | BÀ |° AqE\r  )ð!Z )è![ )à!\\A Z [ \\É  )Ø)  (Ð­B~|(!] )ð)  )à( ­| ]6  )à!^ ^ ^( ­B|§6  A: ×  (ÐAj6Ð @ - ×Aq\r B )  !_ AÖ6  _B£  £ B )  !` )Ð(!a )Ð) !b )ø/ JAÿÿqAj!c )ø/ LAÿÿqAj!d )ø) B(!e )ø) B) !f B0| f7  B(| e6  B$| d6  B | c6   b7  a6 `Bø  B|£ A   )ð!g )è!h )à!iA g h iÉ  )ø/ JAÿÿq!j )ð)  )à( ­| j6  )à!k k k( ­B|§6  )ø/ LAÿÿq!l )ð)  )à( ­| l6  )à!m m m( ­B|§6  B|$ Å	~~~~~# Bð }! $    7h  7`  7X  : W )`A6  A6P  (P­ö 7HB !  7@  78 )`! )X! BÈ | BÐ |   B8|Ç  )`!A BÈ | BÐ | É  )h(!	 )H )`( ­| 	6  )`!\n \n \n( ­B|§6  A 64@@ (4 )h(IAqE\r  )h)  (4­B|7( )(! )`!  )7  ) 7 B| BÈ | BÐ | Ê  )`!\rA BÈ | BÐ | \rÉ  )((! )H )`( ­| 6  )`!  ( ­B|§6  A 6$@@ ($ )((IAqE\r )() ($­B|! )`!  )7  ) 7   BÈ | BÐ | Ê   ($Aj6$ @ - WAqE\r  )(B |  )(B |! )`! )X) ) !  BÈ | BÐ |  B8| È  )`!A BÈ | BÐ | É  )(- 0Aq! )H )`( ­| :   )`!  ( ­B|§6   (4Aj64 B (  ! )H 6  )`( ! )H 6@ )8B RAqE\r  )8ø  )H! Bð |$  À~~~~# BÀ }! $    70  6,  )0) 7   )07@@@ ) B RAqE\r@ ) ( (,j ) (MAqE\r   ) )  ) (­|7 (,! ) !   (j6  )78  ) B|7  ) )7   A 6@ ( (,IAqE\r   (,6 (­B|ö ! ) 7  )) B|! ))  7  (,! ))  6 (! ))  6 )) B 7 )) ) !	 )) (­!\nA !@ \nP\r  	  \nü   )) ) 78 )8! BÀ |$  }~# B}!   7  )) 7 @@ ) B RAqE\r ) A 6 ) ) ! ) (­!A !@ P\r    ü   ) )7  y~# B }! $    7  )) 7@@ )B RAqE\r  ))7 )ø   )7  )B 7  B |$ ì~# B0}! $    7   7@@ ) B RAq\r  B 7( B 7  B|7  ) 7 @@ ) B RAqE\r )B|AÍ ! ) 7  ) )  )Ñ ! ))  7   )) B|7  ) )7    )7( )(! B0|$  ~~# B0}! $    7   7@@@ ) ( E\r  )B RAq\r  ) 7(  )Ò 7 )! ) !  ) 7   )7  )7  )7  ) 7  )! ) 7 )A6 @@ ) ( AFAqE\r  )B|AÍ ! ) 7 ) )) )Ð ! )) 7@@ ) ( AFAqE\r  ) (! ) 6 )B| )(Í !	 ) 	7 ))!\n ) )! )(­!@ P\r  \n  ü\n  @@ ) ( AFAqE\r  )B|!\r ) B|! )!   Ó  \r )7 \r ) 7 @@ ) ( AFAqE\r  ))!  (HAj6H@ ) ) )RAqE\r  )B|AÐ Í ! ) 7 )) ) )BÐ ü\n  @ ) ( AFAqE\r  ))!  (Aj6@ ) ) )RAqE\r  )B|AÍ ! ) 7 )) ) )Bü\n    )7( )(! B0|$  ¡~# B}! $    7  )B|A(Í 7 @ )( )(MAqE\r @@ )(E\r @@ )( )(MAqE\r )!  (At6  ))  )(­Bù ! ) 7  )A6Bö ! ) 7  ) ! ))  )(­B| 7  )!  (Aj6 ) ! B|$  ~# B }! $   7  7   )B| )(­B§Í 7    )(6   )(6 A 6@@ (  (IAqE\r ))  (­B|)  )Ñ !  )  (­B| 7  ))  (­B|) )Ñ !  )  (­B| 7  (Aj6  B |$ "~# B}!   7B¸ æ~~# BÀ }! $    78  70  )0Ò 7( )(! A6  B|A 6  B|!  )87 B|B 7   )07 A 6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  )(! BÀ |$  è~~# BÀ }! $   78  )8Ò 70 )0! A6 B|B|A 6  B|B|!   )7   ) 7   )87  A 6( A : , B|B%|!A !  :   ;    )(7   ) 7  )7  )7  )7  )0! BÀ |$  æ~~# BÀ }! $    78  70  )0Ò 7( )(! A6  B|A 6  B|!  )87 B|B 7   )07 A 6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  )(! BÀ |$  æ~~# BÀ }! $    98  70  )0Ò 7( )(! A6  B|A 6  B|!  +89 B|B 7   )07 A 6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  )(! BÀ |$  ø~~# BÀ }! $    Aq: ?  70  )0Ò 7( )(! A6  A 6 B|!  - ?Aq:  B|!B !  7   7    )07 A 6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  )(!	 BÀ |$  	è~~# BÀ }! $   78  )8Ò 70 )0! A6 B|B|A 6  B|B|!   )7   ) 7   )87  A 6( A : , B|B%|!A !  :   ;    )(7   ) 7  )7  )7  )7  )0! BÀ |$  ~~# BÀ }! $   78  )8Ò 70  )8B|AÐ Í 7( )(  BÐ ü\n   )(!  (HAj6H )0! A6  B|A 6  B|!  )(7 B|B 7   )87 A 6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  )0! BÀ |$  ã~~~# Bà}! $   7Ø  )ØÒ 7Ð  )ØB|AÍ 7È )È!B!A ! B0|  ü B!  B0| ü\n   )ÈB |  Bð ü\n   )È!  (Aj6 )Ð! A6 B|B|A 6  B|B|!	  )È7 	B|B 7   )Ø7  A 6( A : , B|B%|!\nA ! \n :  \n ;    )(7   ) 7  )7  )7  )7  )Ð! Bà|$  Ò~~~# B }! $    7@@ )( AFAqE\r   )))7@ )B R!A ! Aq! !@ E\r  )- As!@ AqE\r   ))7 )) Ý   )7@@ )( AFAqE\r  A 6@@ ( )(IAqE\r )) (­B|) Ý  )) (­B|)Ý   (Aj6 @@ )( AFAqE\r  ))! (HAj!  6H@ \r @ )))0- )AqE\r  )))0A : ) ))A 6( A 6 @@ (  )))0(IAqE\r )))0)  ( ­B|) Ý   ( Aj6   )))0A 6 )))0B|Î  )))0A 6 @ )( AFAqE\r  ))! (Aj!	  	6@ 	\r @ ))) B RAqE\r  ))) ø @ )))B RAqE\r  )))ø  ))B |Þ  B |$ ø~# B }! $    7@ )) B RAqE\r  )) ø @ ))B RAqE\r  ))ø   )) 7@@ )B RAqE\r  ))07 )ß   )7   ))87@@ )B RAqE\r  ))07  )ß   ) 7  B |$ æ~# B}! $    7 A 6@@ ( )(IAqE\r ))  (­B|) Ý   (Aj6 @ )) B RAqE\r  )) ø  )A 6 )B|Ï @ ))B RAqE\r  ))ø  )A 6  )ø  B|$ ~~# Bð }! $    7`  7X@@ )`(  )X( GAqE\r  A Aq: o )`5 !@ BV\r @@@@@@@@ §   AAq: o  )`))7P  )X))7H@ )PB R!A ! Aq! !@ E\r  )HB R!@ AqE\r @ )P)  )H) à Aq\r  A Aq: o\n  )P)7P  )H)7H )PB Q!A !	 Aq!\n 	!@ \nE\r  )HB Q!  Aq: o )`B|! )XB|!\r  )7  ) 7  \r)7  \r) 7   B| ° Aq: o  )`) )X)QAq: o  )`+ )X+aAq: o  )`- Aq )X- AqFAq: o@ )`( )X(GAqE\r  A Aq: o A 6D@@ (D )`(IAqE\r@@ )`) (D­B|)  )X) (D­B|) à AqE\r  )`) (D­B|) )X) (D­B|)à Aq\r A Aq: o  (DAj6D  AAq: o@ )`)(@A KAqE\r  )`)B8|! )X)B8|!  )78  ) 70  )7(  ) 7   B0| B |° Aq: o A Aq: o A Aq: o - oAq! Bð |$  ¹~# B }!   6  7  7 A 6@@@ ( (IAqE\r@ ) (­B|) (  ) (­B|( GAqE\r  ) (­B|( E\r  A Aq:   (Aj6  AAq:  - Aq«~~~~~~~~~# Bà}! $    7Ð  7È  7À  7¸  : ·@@ )À(@A KAqE\r  )Ð! )ÀB8|! )À(! )È!	  )7  ) 7   B|  	ã 7¨@ )¨B RAq\r B !\n  \n7   \n7 )ÀB8|! B|  )7x  ) 7p B| Bð |·  B|BÉ ¶  A 6@@ ( )À(IAqE\r@ (A KAqE\r  B|!A !\rA!  \r t uµ  )È (­B|) ! )Ð! B| A AAq  @ )Ð(HE\r  B 7Ø  (Aj6  B|!AÝ !A!   t uµ  B|  ) 7h  )7` B| Bà |´ @@ )¸B RAqE\r B )  ! Aë6  BÓ  £ B )  ! )¸) (! )¸) ) ! )¸/AÿÿqAj! )¸/\nAÿÿqAj! (! )! B0| 7  B(| 6  B$| 6  B | 6   7  6 B°  B|£ B )  ! Aî6@ BÓ  BÀ |£ B )  ! (!  )7X  6P B¼  BÐ |£  )ø  )ÐA6H )ÐB7P  )Ð)0Ô 7Ø )¨)@!  )Ð )È   7ø@ )Ð(HAFAqE\r  )ÐA 6H  )ø7Ø )Ðä   )Ð)07ð@ )ð($ )À( )À((jIAqE\r  )À( )À((j!  )ð  6$@@ )ð( \r  )ð($­Bö !! )ð !7 )ð) )ð($­Bù !" )ð "7 )À( )À((j!# )ð #6  A 6ì@@ (ì )À(IAqE\r BÈ|!$ )À)  (ì­B|!% $ %)7 $ %) 7   )È (ì­B|) 7Ø A 6à BÈ|B|A 6  )ð) (ì­B|!& & )à7 & )Ø7 & )Ð7 & )È7   (ìAj6ì  A 6Ä@@ (Ä )À((IAqE\r B |!\' )À)  (Ä­B~|!( \' ()7 \' () 7   )À)  (Ä­B~|)7° A6¸ B |B|A 6  )ð) (Ä )À(j­B|!) ) )¸7 ) )°7 ) )¨7 ) ) 7   (ÄAj6Ä   )Ð )ÀB| - ·Aqå 7@ )Ð(HAFAqE\r  )ÐA 6H B 7@ )Ð(H\r @@ - ·AqE\r   ) )ð)8Ñ 7  )ð)8Ô 7 )Ðæ   )7Ø )Ø!* Bà|$  *~# BÐ }! $    7@  6<  70 A 6,@@@ (, )@(IAqE\r  )@) (,­BÈ ~|7  ) !  )7  ) 7  )7  ) 7 @ B| ° AqE\r  ) ( (<FAqE\r  (< )0 ) B|á AqE\r   ) 7H  (,Aj6,  B 7H )H! BÐ |$  Ó~# BÐ }! $    7H@ )H)0)0B QAqE\r BÀ ö ! )H)( 70 )H)()0!B !  7@  78  70  7(  7   7  7  7  )@78  )870  )07(  )(7   ) 7  )7  )7  )7  )H)(! )H)()0 78 )H)()0! )H 7( )H)(! )H)0 70 )H)0)0! )H 70 BÐ |$ Ø~# B0}! $    7   7  :  A 6@@@ (Aj )(IAqE\r  )  ))  (­B|) A Aqç 7@ ) (HE\r   )7(  (Aj6  B 7 @@ )(A KAqE\r  ) ! )!   )  (Aj­B|)  - Aqç 7 @ ) (HE\r   ) 7(@ - AqE\r   ) )0Ô 7   ) 7( )(! B0|$  Ò~# B }! $    7  ))07 A 6@@ ( )(IAqE\r ))  (­B|) Ý   (Aj6  )A 6 )B|Î  )A 6 @ ))0)8B RAqE\r  ))0)8! ) 70 B |$ ÿR7~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~# BÀ\r}! $    7°\r  7¨\r  Aq: §\r B 7\r )¨\r1  !@@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r   )°\r )¨\rB| - §\rAqå 7\r@ )°\r(HE\r   )\r7¸\r  )°\r )¨\r) AAqç 7\r@ )°\r(HE\r   )\r7¸\r@ )\r( AGAqE\r B !  7\r  7\r )\r! )°\r! B\r| A AAq  B )  ! AÅ6  BÓ  £ B )  !	 )¨\r) B(!\n )¨\r) B) ! )¨\r/ JAÿÿqAj! )¨\r/ LAÿÿqAj!\r Bð|  )\r7  )\r7 Bð| B|´  (ø! Bà|  )\r7(  )\r7  Bà| B |´  )à! BÐ | 7  BÈ | 6  BÄ | \r6  BÀ | 6   78  \n60 	B  B0|£  )°\rA6H )°\rB7P )\rø   )°\r)0Ô 7¸\r@ )¨\r(  )\r)(GAqE\r B )  ! AÐ6` BÓ  Bà |£ B )  ! )¨\r) B(! )¨\r) B) ! )¨\r/ JAÿÿqAj! )¨\r/ LAÿÿqAj! )¨\r( ! )\r)(! B| 6  B| 6  B| 6  B| 6   7x  6p B÷  Bð |£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r  )°\r)0B| )¨\r( ­B§Í 7Ø A 6Ô@@ (Ô )¨\r( IAqE\r )°\r )¨\r) \n (Ô­B|) AAqç ! )Ø (Ô­B| 7 @ )°\r(HE\r   )°\r)0Ô 7¸\r  (ÔAj6Ô   )°\r)X7È )\r)! )°\r 7X  )°\r )Ø )\r) )¨\rBÂ | - §\rAqâ 7\r )È! )°\r 7X@ )°\r(HAFAqE\r  )°\r)PB RAqE\r B !  )  7À  ) 7¸@ )¨\r) -  AÿqA	FAqE\r  )¨\r) B|!  ) 7À  )  7¸ Aï6BÓ  B|Æ  )¨\r) B(! )¨\r) B) ! (À! )¸!  )¨\r/ JAÿÿqAj!! BÀ| !6  B¸|  7  B°| 6   7¨  6 BÛ  B |Æ   )°\r)0Ô 7¸\r  )°\r )¨\r) AAqç 7°@ )°\r(HE\r   )°7¸\r@@ )°) )°\r)0QAqE\r  )°!" " "( Aj6   )° )°\r)0Ñ 7°B !#  #7¨  #7   #7  #7 B|!$ )¨\rB|!% $ %) 7  $ %)  7    )°7  )°\r)XB R!& A A &Aq6¨@@ (¨AFAqE\r @ )°\r( )°\r(MAqE\r @@ )°\r(E\r @@ )°\r( )°\r(MAqE\r )°\r!\' \' \'(At6  )°\r)  )°\r(­Bù !( )°\r (7  )°\rA6B ö !) )°\r )7  )°\r)  )°\r(­B|!* * )¨7 * ) 7 * )7 * )7  )°\r!+ + +(Aj6@ )°\r)0($ )°\r)0( MAqE\r @@ )°\r)0($E\r @@ )°\r)0($ )°\r)0( MAqE\r )°\r)0!, , ,($At6$  )°\r)0) )°\r)0($­Bù !- )°\r)0 -7 )°\r)0A6$B ö !. )°\r)0 .7 )°\r)0) )°\r)0( ­B|!/ / )¨7 / ) 7 / )7 / )7  )°\r)0!0 0 0( Aj6   )°\r )¨\r) AAqç 7@ )°\r(HE\r   )7¸\r@ )ÿ AqE\r   )°\r )¨\rB\n| - §\rAqå 7\r@ )°\r(HE\r   )\r7¸\r  )\r7¸\r A 6@@ ( )¨\r( "IAqE\r  )°\r )¨\r)  (­B~|) AAqç 7@ )°\r(HE\r   )7¸\r@ )ÿ AqE\r   )°\r )¨\r)  5B~|B| - §\rAqå 7\r@ )°\r(HE\r   )\r7¸\r  )\r7¸\r  (Aj6 @ )¨\r- *AqE\r   )°\r )¨\rB2| - §\rAqå 7\r@ )°\r(HE\r   )\r7¸\r )°\rä  )°\r)0A: ( A 6@  )°\r )¨\r) AAqç 7ø@ )°\r(HE\r   )ø7¸\r@@ )øÿ Aq\r   )°\r )¨\rB|B|A Aqå 7ð@ )°\r(HE\r   )ð7¸\r (!1  1Aj6@ 1Aä FAqE\r  )°\ræ  )°\rä  A 6 )°\r)0A : ( )°\ræ  )°\r!2 )¨\rB|!3  3) 7à  3)  7Ø  )à7  )Ø7  2 B|è 7è@ )èB RAq\r B )  !4 A¼6Ð 4BÓ  BÐ|£ B )  !5 )¨\r) B(!6 )¨\r) B) !7 )¨\r/ JAÿÿqAj!8 )¨\r/ LAÿÿqAj!9 )¨\r( \n!: )¨\r) !; B| ;7  Bø| :6  Bô| 96  Bð| 86   77è  66à 5Bº  Bà|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r  )°\r )¨\r) AAqç 7Ð@ )°\r(HE\r   )Ð7¸\r@ )è) )ÐQAqE\r  )è)!< < <( Aj6 @@ )Ð) )è))QAqE\r  )Ð!= = =( Aj6   )Ð )è))Ñ 7Ð )Ð!> )è >7\r@ - §\rAq\r \r  )°\r )¨\r) AAqç 7È@ )°\r(HE\r   )È7¸\r  )°\r )¨\r) \nAAqç 7À@ )°\r(HE\r   )À7¸\r@@ )È( AFAqE\r @ )À( AGAqE\r B )  !? Aß6  ?BÓ  B |£ B )  !@ )¨\r) B(!A )¨\r) B) !B )¨\r/ JAÿÿqAj!C )¨\r/ LAÿÿqAj!D BÄ| D6  BÀ| C6   B7¸  A6° @Bí  B°|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r  )È))7¸ A 6´@ )¸B R!EA !F EAq!G F!H@ GE\r  (´ )À)§I!H@ HAqE\r   )¸)7¸  (´Aj6´@@ )¸B RAqE\r   )¸) 7\r  )°\r)0Ô 7\r@@ )È( AFAqE\r @@ )À)§ )È(IAqE\r  )ÈB|!I  I)7¨  I) 7   )À) ) |7  A6¨ )°\r)0!J  )¨7Ø  ) 7Ð  BÐ| JÖ 7\r  )°\r)0Ô 7\r@@ )È( AFAqE\r  A :  A 6@@ ( )È(IAqE\r@ )È) (­B|)  )Àà AqE\r   )È) (­B|)7\r A:   (Aj6 @ - Aq\r   )°\r)0Ô 7\rB !K  K7  K7 )È!L )°\r!M B| LA AAq M B )  !N A6à NBÓ  Bà|£ B )  !O )¨\r) B(!P )¨\r) B) !Q )¨\r/ JAÿÿqAj!R )¨\r/ LAÿÿqAj!S Bø\n|  )7ø  )7ð Bø\n| Bð|´  (!T Bè\n|  )7  )7 Bè\n| B|´  )è\n!U B°| U7  B¨| T6  B¤| S6  B | R6   Q7  P6 OB  B|£  )°\rA6H )°\rB7P )ø   )°\r)0Ô 7¸\r )°\r!V )¨\rB|!W  W) 7Ø\n  W)  7Ð\n  )Ø\n7È  )Ð\n7À  V BÀ|è 7à\n@ )à\nB RAq\r B )  !X A6 XBÓ  B|£ B )  !Y )¨\r) B(!Z )¨\r) B) ![ )¨\r/ JAÿÿqAj!\\ )¨\r/ LAÿÿqAj!] )¨\r( \n!^ )¨\r) !_ B°| _7  B¨| ^6  B¤| ]6  B | \\6   [7  Z6 YBº  B|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r\r@ )à\n)( AKAqE\r  )à\n)- $Aq\r  )à\n) )à\n))Ñ !` )à\n `7  )°\r )¨\r) AAqç 7È\n@ )°\r(HE\r   )È\n7¸\r\r  )°\r )¨\r) AAqç 7À\n@ )°\r(HE\r   )À\n7¸\r\r@@ )À\n) )à\n))QAqE\r  )À\n!a a a( Aj6   )À\n )à\n))Ñ 7À\n@@ )à\n)( AFAqE\r @ )È\n( AGAqE\r B )  !b A³6À bBÓ  BÀ|£ B )  !c )¨\r) B(!d )¨\r) B) !e )¨\r/ JAÿÿqAj!f )¨\r/ LAÿÿqAj!g Bä| g6  Bà| f6   e7Ø  d6Ð cB  BÐ|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r  )à\n)))7¸\n A 6´\n@ )¸\nB R!hA !i hAq!j i!k@ jE\r  (´\n )È\n)§I!k@ kAqE\r   )¸\n)7¸\n  (´\nAj6´\n@ )¸\nB RAq\r B )  !l AÃ6ð lBÓ  Bð|£ B )  !m )¨\r) B(!n )¨\r) B) !o )¨\r/ JAÿÿqAj!p )¨\r/ LAÿÿqAj!q B| q6  B| p6   o7  n6 mBê  B|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r )¸\n) !r r r( Aj6  )À\n!s )¸\n s7 @@ )à\n)( AFAqE\r  A : ³\n A 6¬\n@@ (¬\n )à\n)(IAqE\r@ )à\n)) (¬\n­B|)  )È\nà AqE\r  )à\n)) (¬\n­B|)!t t t( Aj6  )À\n!u )à\n)) (¬\n­B| u7 A: ³\n  (¬\nAj6¬\n @ - ³\nAq\r @@ )È\n) )à\n))QAqE\r  )È\n!v v v( Aj6   )È\n )à\n))Ñ 7È\n  )È\n7\n  )À\n7 \n@ )à\n)( )à\n)(FAqE\r @@ )à\n)(\r  )à\n)A6 )à\n)!w w w(At6  )°\r)0B| )à\n)(­B§Í 7\n )\n!x )à\n))!y )à\n)(­B!z@ zP\r  x y zü\n   )\n!{ )à\n) {7 )à\n))!| )à\n)!} }(!~ } ~Aj6 | ~­B|!  ) \n7  )\n7 B !  7\n  7\n )à\n)! )°\r! B\n| A AAq  B )  ! Aö6  BÓ  B |£ B )  ! )¨\r) B(! )¨\r) B) ! )¨\r/ JAÿÿqAj! )¨\r/ LAÿÿqAj! Bð	|  )\n7¸  )\n7° Bð	| B°|´  (ø	! Bà	|  )\n7È  )\n7À Bà	| BÀ|´  )à	! Bð| 7  Bè| 6  Bä| 6  Bà| 6   7Ø  6Ð B¸  BÐ|£  )°\rA6H )°\rB7P )\nø   )°\r)0Ô 7¸\r@ - §\rAqE\r   )°\r)0Ô 7\r@ )¨\r- AqE\r   )°\r )¨\r) \nAAqç 7\r@ )°\r(HE\r   )\r7¸\r\r )°\rA6H\n@ - §\rAq\r \n  )°\r)0B|AÍ 7Ø	  )Ø	7Ð	 A 6Ì	@@ (Ì	 )¨\r( \nIAqE\r  )°\r)0B|AÍ 7À	 )°\r )¨\r)  (Ì	­B|) AAqç ! )À	 7 @ )°\r(HE\r   )À	) 7¸\r\r )À	B 7@@ )Ð	B RAqE\r  )À	! )Ð	 7  )À	7Ð	  )À	7Ø	  )À	7Ð	  (Ì	Aj6Ì	   )Ø	 )°\r)0Õ 7\r	@ - §\rAq\r 	 )°\r! )¨\rB|!  ) 7°	  )  7¨	  )°	7  )¨	7   B|è 7¸	@ )¸	B RAq\r B )  ! A©6Ð BÓ  BÐ|£ B )  ! )¨\r) B(! )¨\r) B) ! )¨\r/ JAÿÿqAj! )¨\r/ LAÿÿqAj! )¨\r( \n! )¨\r) ! B| 7  Bø| 6  Bô| 6  Bð| 6   7è  6à Bº  Bà|£  )°\rA6H )°\rB7P  )°\r)0Ô 7¸\r\n  )¸	)7\r@ - §\rAqE\r  )¨\rB|! )°\r)0!  ) 7 	  )  7	  ) 	7¨  )	7   B | Ö 7\r@ - §\rAqE\r   )¨\r)  )°\r)0× 7\r@ - §\rAqE\r   )¨\r+  )°\r)0Ø 7\r@ - §\rAqE\r  )¨\r- ! )°\r)0!  Aq Ù 7\r@ - §\rAq\r B !  7	  7	  )°\r)8é 7	  )¨\r( \n6ø  (ø6ü  )	B| (ø­B§Í 7ð )ð! )¨\r) ! )¨\r( \n­B!@ P\r    ü\n   )°\r! )	!  )¨\rB|B|!¡  Bð| B	|   ¡ê  B |!¢ )¨\rB|!£ ¢ £) 7  ¢ £)  7   B |B|!¤ )¨\rB|B|!¥ ¤ ¥) 7  ¤ ¥)  7   B |B |!¦ ¦ )	7 ¦ )	7   )	7Ð B |B8|!§ )¨\rB|B |!¨ § ¨) 7  § ¨)  7   A6è B |BÌ |A 6  )°\r)0!©BÐ !ª B°| B | ªü\n    B°| ©Û 7\r@ - §\rAq\r B !«  «7  «7  )¨\r( \n6  (6  )°\r)0B| (­B§Í 7 A 6@@ ( (IAqE\rB !¬  ¬7  ¬7ø  )°\r )¨\r)  (­B|) AAqç 7ø@ )°\r(HE\r   )ø7¸\r  )°\r )¨\r)  (­B|)AAqç 7@ )°\r(HE\r   )7¸\r ) (­B|!­ ­ )7 ­ )ø7   (Aj6  )°\r)0!®  )7  )7  B| ®Ú 7\r  )°\r )¨\r) AAqç 7ð@ )°\r(HE\r   )ð7¸\r A 6ì@@ (ì )¨\r( IAqE\r  )°\r )¨\r) \n (ì­B|) AAqç 7à@ )°\r(HE\r   )à7¸\r@ )à )ðà AqE\r   )°\r )¨\r) \n 5ìB|) - §\rAqç 7\r@ )°\r(HE\r   )\r7¸\r  (ìAj6ì @ - §\rAqE\r  )°\r)X!¯ )°\r)0!°BÐ !± B| ¯ ±ü\n    B| °Û 7\r@ - §\rAqE\r  )\rB RAq\r   )°\r)0Ô 7¸\r  )\r7¸\r )¸\r!² BÀ\r|$  ²Æ~# Bà }! $    7P  )P)07H@@@ )HB RAqE\r  )H( 6D@@ (DA KAqE\r )H) (DAk­B|!  )7  ) 7  )7  ) 7 @ B| ° AqE\r   )H) (D­B|B`|7X  (DAj6D @ )H- (Aq\r   )H)87H   )P(6@@@ (@A KAqE\r )P)  (@Ak­B|!  )78  ) 70  )7(  ) 7 @ B0| B |° AqE\r   )P)  (@­B|B`|7X  (@Aj6@  B 7X )X! Bà |$  ¿~~# Bà }! $    7X  )X7P@ )PB R!A ! Aq! !@ E\r  )P)0B R!A ! Aq! ! E\r  )P- )!@ AqE\r   )P)07P@ )P- )AqE\r   )P)07HBÀ ö !	 )P 	70 )P)0!\nB !  7@  78  70  7(  7   7  7  7 \n )@78 \n )870 \n )07( \n )(7  \n ) 7 \n )7 \n )7 \n )7   )P)07P@ )HB RAqE\r  )H! )P 70 )P!\r )H \r78 )PA: ) )P! Bà |$  µ~# B0}! $    7(  7   7  7  7 A 6@@ ( )(IAqE\r )( )  ) ) ))  (­B|) ë @ )((HE\r   (Aj6  B0|$ å		~~~~~# BÐ}! $    7È  7À  7¸  7°  7¨ )¨1  !@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  )È )À )¸ )° )¨B|ê @ )È(HE\r  )È )À )¸ )° )¨) ë @ )È(HE\r  )È )À )¸ )° )¨B|B|ê @ )È(HE\r  )È )À )¸ )° )¨) ë @ )È(HE\r @ )À( )À(FAqE\r @@ )À(\r  )ÀA6 )À!  (At6  )°B| )À(­B§Í 7  ) ! )À) !	 )À(­B!\n@ \nP\r   	 \nü\n   ) ! )À 7  )À) ! )À!\r \r(! \r Aj6  ­B|! )¨B|!  ) 7   )  7   )È )À )¸ )° )¨) ë @ )È(HE\r  )È )À )¸ )° )¨B|B|ê @ )È(HE\r  A 6@@ ( )¨( "IAqE\r )È )À )¸ )° )¨)  (­B~|B|ê @ )È(HE\r   (Aj6 @ )¨- *AqE\r  )È )À )¸ )° )¨B|B0|ê @ )È(HE\r  )È )À )¸ )° )¨B|B|ê @ )È(HE\r  )È )À )¸ )° )¨) ë @ )È(HE\r \r )È )À )¸ )° )¨) ë @ )È(HE\r \r )È )À )¸ )° )¨) \në @ )È(HE\r \r )È )À )¸ )° )¨) ë @ )È(HE\r  )È )À )¸ )° )¨) ë @ )È(HE\r @ )¨- AqE\r  )È )À )¸ )° )¨) \në @ )È(HE\r \n )È )À )¸ )° )¨B|ê @ )È(HE\r \n	 A 6@@ ( )À(IAqE\r )¨B|! )À)  (­B|!  ) 7  )  7  )7   )7  )7  ) 7@ B| B|° AqE\r   (Aj6  )È! )¨B|!  ) 7x  )  7p  )x70  )p7(   B(|è 7@ )B RAqE\r  )(AGAqE\r  BØ |! )!  )7  ) 7   )) )°Ñ 7h@ )¸( )¸(FAqE\r @@ )¸(\r  )¸A6 )¸!  (At6  )°B| )¸(­B~§Í 7P )P! )¸) ! )¸(­B~!@ P\r    ü\n   )P! )¸ 7  )¸) ! )¸! (!  Aj6  ­B~|!  )h7  )`7  )X7 @ )À( )À( )¨( \njIAqE\r  )À( )¨( \nj!  )À  6  )°B| )À(­B§Í 7H )H!! )À) !" )À(­B!#@ #P\r  ! " #ü\n   )H!$ )À $7  A 6D@@ (D )¨( \nIAqE\r )À) !% )À!& &(!\' & \'Aj6 % \'­B|!( )¨)  (D­B|!) ( ))7 ( )) 7   (DAj6D  )È )À )¸ )° )¨B|B|ê @ )È(HE\r  A 6@@@ (@ )¨( \nIAqE\r )È )À )¸ )° )¨)  (@­B|) ë @ )È(HE\r  )È )À )¸ )° )¨)  (@­B|)ë @ )È(HE\r   (@Aj6@  )È )À )¸ )° )¨) ë @ )È(HE\r  A 6<@@ (< )¨( IAqE\r )È )À )¸ )° )¨) \n (<­B|) ë @ )È(HE\r  )È )À )¸ )° )¨) \n (<­B|)ë @ )È(HE\r   (<Aj6<  BÐ|$ \r~~# Bð}! $   6ì  7à  7ØBð !  A  ü   BÀ ö 7   ) !B !  7Ð  7È  7À  7¸  7°  7¨  7   7  )Ð78  )È70  )À7(  )¸7   )°7  )¨7  ) 7  )7     ) 7(    ) 70  BÀ ö 78  )8!B !	  	7  	7  	7  	7x  	7p  	7h  	7`  	7X  )78  )70  )7(  )x7   )p7  )h7  )`7  )X7    )0B|AÍ 7P  )P7H A 6D@@ (D (ìIAqE\r  )à (D­B|) Ü §6@   )0B| (@Í 78 )8!\n )à (D­B|) ! (@­!@ P\r  \n  ü\n     )0B|AÍ 70  )0B|A(Í !\r )0 \r7  )0) ! A6 B|B|A 6  B|B|!  )87  (@6 B|A 6    )07  A6( A : , B|B%|!A !  :   ;    )(7   ) 7  )7  )7  )7  )0A:  )0! )H 7  )07H  (DAj6D    )P )Øí  Bð|$ Ö\n~~~~~~~~~~~# B}! $    7  7  7x )x!B (ð ! BÄ  î  )x!B (àõ ! Bð  î  )x!B (ü !	 Bðõ  	î  )x!\nB ( ý ! \nBü  î  )x!B (à !\r B°ý  \rî  )x!B (È ! Bð  î  )x!B (è ! Bð  î  )x!B (¨ ! BÐ  î  )x!B (Ð ! BÐ  î  )x!B (À ! B°  î  )B|! )x!  )7  ) 7  )! ) 7@  ))0Ô 7p BÐ |! BÇ 7P A6X B|A 6   )p7` A6h BÐ |B|A 6 @ )( )(MAqE\r @@ )(E\r @@ )( )(MAqE\r )!  (At6  ))  )(­Bù ! ) 7  )A6B ö ! ) 7  ))  )(­B|!  )h7  )`7  )X7  )P7  )!     (Aj6 B 78 A6@ B8|B|A 6  ))0!!  )@7  )87  B| !Ö 7H B|!" B 7 A6  "B|A 6   )H7( A60 B|B|A 6 @ )( )(MAqE\r @@ )(E\r @@ )( )(MAqE\r )!# # #(At6  ))  )(­Bù !$ ) $7  )A6B ö !% ) %7  ))  )(­B|!& & )07 & )(7 & ) 7 & )7  )!\' \' \'(Aj6 B|$ Ú~~~# B }! $    7  7  6 (! )!   (j6 ))  )(­BÈ ~ù ! ) 7  ))  )(­BÈ ~|! )! (­BÈ ~!	@ 	P\r    	ü\n   (!\n )!  \n (j6 B |$ ¹\n~# B }! $    7  7  7@@ )) B RAq\r  )A6 )AÍ ! ) 7 @ )( )(MAqE\r  )!  (At6  ) )(­B§Í 7  ) ! )) ! )(­B!@ P\r    ü\n   ) !	 ) 	7  )!\n )) ! )! (!\r  \rAj6  \r­B| \n7  B |$ ~~~# B0}! $   7( A 6$@@@ ($B (è IAqE\rB )à  ($­B|!  )7  ) 7  )7  ) 7 @ B| ° AqE\r B )à  ($­B|!   )7   ) 7   ($Aj6$    (6   )( (Í 7   ) ! ) !  (­!@ P\r    ü\n  @B (ì B (è MAqE\r @@B (ì E\r @@B (ì B (è MAqE\rB (ì At!	B  	6ì  B )à B (ì ­Bù !\nB  \n7à A!B  6ì Bö !B  7à B )à B (è ­B|!\r \r  )7 \r  ) 7 B (è Aj!B  6è  B0|$ ³\n~~~~~# B}!	 	$  	  7 	 7 	 7x 	 7p 	 : o 	 7` 	 7X 	 ;V 	 ;TB !\n 	 \n7H 	 \n7@B ! 	 78 	 70 	A 6,@@ 	(, 	)(IAqE\r 	 	))  	5,B|) 7  	)x! 	)p!\r 	)`! 	- oAq! 	 	B |  \r 	BÀ |  ò : @ 	- Aq\r  	)x! 	)`! 	B |  ó  	) ! 	)`! 	BÀ |  ï  	 	(,6@@ 	( 	(HIAqE\r@ 	(< 	(8MAqE\r @@ 	(<E\r @@ 	(< 	(8MAqE\r 	 	(<At6<  	 	)0 	(<­B ù 70 	A6< 	Bö 70 	- ! 	)0 	(8­| Aq:   	 	(8Aj68 	 	(Aj6  	 	(,Aj6,  	A 6@@ 	( 	(HIAqE\r 	 	)0 	5|-  Aq: @@ 	- AqE\r A ! 	/V!A!  t u! 	 ;@@ 	- AqE\r A ! 	/T!A!  t u! 	 ; 	)@ 	5B|) ! 	)! 	)x! 	)p! 	- o! 	)`!  	)X!! 	/!" 	/!# 	- !$ Aq!%A!& " &t &u!\'A!(     %   ! \' # (t (u $Aqô  	 	(Aj6  	)@!) 	) )7  	(H!* 	) *6@ 	)0B RAqE\r  	)0ø  	B|$ ~~~# Bð }! $    7`  7X  7P  7H  : G  78@@@ )XB RAqE\r  )PB RAq\r A Aq: o@ )`) -  AÿqAFAqE\r  )`) B| )X )Põ  A Aq: o@ )`) -  AÿqAFAqE\r  )`) B| )X )Põ  A Aq: o@ )`) -  AÿqAFAqE\r  )`) B| )X )Põ  A Aq: o@ )`) -  AÿqAFAqE\r  A 64@@ (4 )`) ( \nIAqE\r )`) )  (4­B| )X )Põ   (4Aj64  A Aq: o@ )`) -  AÿqA	FAqE\r  )`) B|! )X!  ) 7(  )  7   )(7  ) 7  B| ö 60@ (0AGAqE\r @@ )HB RAqE\r  (0!	 )X!\n )P! )H! - G!\r )8! 	 \n   \rAq ÷   )P)  (0­B|) 7 )X! )8! B|  ó  )! )` 7  AAq: o A Aq: o - oAq! Bð |$  á	~# BÀ }! $    78  70  7(@@@ )0B RAqE\r  )8)  )0ø Aq\r  )(AÒ Í 7  )  )8) BÒ ü\n   ) ! )8 7  ) 1  ! BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  ) B| )0 )(ù  ) B| )0 )(ó  ) B|B| )0 )(ù  ) B|B| )0 )(ó  ) B| )0 )(ó  ) B|B| )0 )(ù  A 6@@ ( ) ( "IAqE\r ) )  (­B~| )0 )(ó  ) )  (­B~|B| )0 )(ù   (Aj6 @ ) - *AqE\r  ) B|B0| )0 )(ù  ) B| )0 )(ó  ) B|B| )0 )(ù  ) B|B| )0 )(ó \r ) B| )0 )(ó  ) B|B| )0 )(ó  ) B|B| )0 )(ó  ) B|B| )0 )(ó  ) B| )0 )(ù \n	  )( ) ( \n­B§Í 7 )! ) ) ! ) ( \n­B!@ P\r    ü\n   )!	 )  	7  ) B|B| )0 )(ù  A 6@@ ( ) ( \nIAqE\r ) )  (­B| )0 )(ó  ) )  (­B|B| )0 )(ó   (Aj6 @ ) - AqE\r  ) B|B| )0 )(ó  ) B| )0 )(ó  A 6@@ ( ) ( IAqE\r ) ) \n (­B| )0 )(ó  ) ) \n (­B|B| )0 )(ó   (Aj6  BÀ |$ ­C~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~# Bð}!\n \n$  \n  7è \n 7à \n 7Ø \n 7Ð \n : Ï \n 7À \n 7¸ \n ;¶ \n ;´ \n 	: ³@ \n)ØB RAqE\r  \n)ÐB RAqE\r  \n- ³Aq\r  \n)¸! \n)è 7 B \n)è1  !@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  \n)èB|!\r \n)à! \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! Aq!A!  t u!A! \r         t uñ  \n)èB|! \n)Ø! \n)Ð! \n)À! \n- ÏAq! \n   B   ò : ²@ \n- ²AqE\r  \nA ;¶ \nA ;´ \n)è) ! \n)à!  \n)Ø!! \n)Ð!" \n- Ï!# \n)À!$ \n)¸!% \n/¶!& \n/´!\' \n- ³Aq \n- ²AqqA G!( #Aq!)A!* & *t *u!+A!,    ! " ) $ % + \' ,t ,u (Aqô  \n)èB\n|!- \n)à!. \n)Ø!/ \n)Ð!0 \n- Ï!1 \n)À!2 \n)¸!3 \n/¶!4 \n/´!5 1Aq!6A!7 4 7t 7u!8A!9 - . / 0 6 2 3 8 5 9t 9uñ @ \n)è) -  AÿqA	FAqE\r  \n)è) B|!: \n :) 7¨ \n :)  7  \n)à!; \n)è( !< \n \n)¨7P \n \n) 7H \n ; \nBÈ | <ú 7@ \n)B RAqE\r  \n \n)è) \n7 \n \n)(6 \n \n)(6@ \n)- 0AqE\r  \n \n(Aj6 \n \n)è(  \n(k6 \n \n(6 \n \n)À \n(­B§Í 7ø \nA 6ô@@ \n(ô \n(IAqE\r \n)è) \n \n( \n(ôj­B|) != \n)ø \n(ô­B| =7  \n \n(ôAj6ô  \n \n)ÀAÒ Í 7è \n)èA:   \n)ø!> \n)è >7  \n(!? \n)è ?6 \n@ \n( \n(MAqE\r @@ \n(E\r @@ \n( \n(MAqE\r \n \n(At6  \n \n) \n(­Bù 7 \nA6 \nBö 7 \n)è!@ \n) \n(­B| @7  \n \n(Aj6B !A \n A7à \n A7ØB !B \n B7Ð \n B7È \n)!C \nBÈ| \n C)7@ \n C) 78 \nBÈ| \nB8|·  \nBÈ|!DAÀ !EA!F D E Ft Fuµ  \n \n(Ô6Ä \nA 6À@@ \n(À \n)(IAqE\r \n)) \n(À­B|!G \nBÈ| \n G)7 \n G) 7 \nBÈ| \nB|·  \nB | \n \n)Ð7  \n \n)È7 \nB | \nB|´  \n)À!H \nB°| \n \n)¨70 \n \n) 7( \nB°| \nB(| Hð @ \n(ä \n(àMAqE\r @@ \n(äE\r @@ \n(ä \n(àMAqE\r \n \n(äAt6ä  \n \n)Ø \n(ä­Bù 7Ø \nA6ä \nBö 7Ø \n)Ø \n(à­B|!I I \n)¸7 I \n)°7  \n \n(àAj6à \n \n(Ä6Ô \n \n(ÀAj6À  \n)Èø  \n)è!JA !K J K:   \n)è!L \n)!M L M) (7 \n L M)  7  \n \n)À \n)è( \nAtÍ 7 \n)!N \n)è!O O) !P O5 \nB!Q@ QP\r  N P Qü\n   \n)!R \n)è R7  \n \n)Ø7 \n \n(à6B!S S \nB|| K6  \n \n)7x \n \n(6 S \nBø || K6  \n)è!TB!U T U|!V \n)B|!W \n)À!X V W \nB| Xû  U \n)è|!Y \n)à!Z \n)- 0![ \n)À!\\ \n)¸!] \n)è/ JAÿÿq \n)(4k!^ \n)è/ LAÿÿq \n)(8k!_ \nB|!` \nBø |!a [Aq!bA!c ^ ct cu!dA!e Y Z ` a b \\ ] d _ et euñ @ \n)ØB RAqE\r  \n)Øø  \n)èB|!f \n)Ø!g \n)Ð!h \n)À!i \n- ÏAq!j \n f g hB  j iò : w@ \n- wAqE\r  \nA ;¶ \nA ;´ \n)è) !k \n)à!l \n)Ø!m \n)Ð!n \n- Ï!o \n)À!p \n)¸!q \n/¶!r \n/´!s \n- ³Aq \n- wAqqA G!t oAq!uA!v r vt vu!wA!x k l m n u p q w s xt xu tAqô  \n)èB|!y \n)Ø!z \n)Ð!{ \n)À!| \n- ÏAq!} \n y z {B  } |ò : v@ \n- vAqE\r  \nA ;¶ \nA ;´ \n)è) !~ \n)à! \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! \n- ³Aq \n- vAqqA G! Aq!A!  t u!A! ~         t u Aqô  \n)èB\n|! \n)à! \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! Aq!A!  t u!A!          t uñ  \nA 6p@@ \n(p \n)è( "IAqE\r \n)è)  \n5pB~|! \n)Ø! \n)Ð! \n)À! \n- ÏAq! \n   B   ò : o@ \n- oAqE\r  \nA ;¶ \nA ;´ \n)è)  \n5pB~|) ! \n)à! \n)Ø!  \n)Ð!¡ \n- Ï!¢ \n)À!£ \n)¸!¤ \n/¶!¥ \n/´!¦ \n- ³Aq \n- oAqqA G!§ ¢Aq!¨A!© ¥ ©t ©u!ªA!«     ¡ ¨ £ ¤ ª ¦ «t «u §Aqô  \n)è)  \n5pB~|B|!¬ \n)à!­ \n)Ø!® \n)Ð!¯ \n- Ï!° \n)À!± \n)¸!² \n/¶!³ \n/´!´ °Aq!µA!¶ ³ ¶t ¶u!·A!¸ ¬ ­ ® ¯ µ ± ² · ´ ¸t ¸uñ  \n \n(pAj6p @ \n)è- *AqE\r  \n)èB2|!¹ \n)à!º \n)Ø!» \n)Ð!¼ \n- Ï!½ \n)À!¾ \n)¸!¿ \n/¶!À \n/´!Á ½Aq!ÂA!Ã À Ãt Ãu!ÄA!Å ¹ º » ¼ Â ¾ ¿ Ä Á Åt Åuñ  \n)èB|!Æ \n)Ø!Ç \n)Ð!È \n)À!É \n- ÏAq!Ê \n Æ Ç ÈB  Ê Éò : n@ \n- nAqE\r  \nA ;¶ \nA ;´ \n)è) !Ë \n)à!Ì \n)Ø!Í \n)Ð!Î \n- Ï!Ï \n)À!Ð \n)¸!Ñ \n/¶!Ò \n/´!Ó \n- ³Aq \n- nAqqA G!Ô ÏAq!ÕA!Ö Ò Öt Öu!×A!Ø Ë Ì Í Î Õ Ð Ñ × Ó Øt Øu ÔAqô  \n)èB\n|!Ù \n)à!Ú \n)Ø!Û \n)Ð!Ü \n- Ï!Ý \n)À!Þ \n)¸!ß \n/¶!à \n/´!á ÝAq!âA!ã à ãt ãu!äA!å Ù Ú Û Ü â Þ ß ä á åt åuñ  \n)èB|!æ \n)Ø!ç \n)Ð!è \n)À!é \n- ÏAq!ê \n æ ç èB  ê éò : m@ \n- mAqE\r  \nA ;¶ \nA ;´ \n)è) !ë \n)à!ì \n)Ø!í \n)Ð!î \n- Ï!ï \n)À!ð \n)¸!ñ \n/¶!ò \n/´!ó \n- ³Aq \n- mAqqA G!ô ïAq!õA!ö ò öt öu!÷A!ø ë ì í î õ ð ñ ÷ ó øt øu ôAqô \r \n)èB|!ù \n)Ø!ú \n)Ð!û \n)À!ü \n- ÏAq!ý \n ù ú ûB  ý üò : l@ \n- lAqE\r  \nA ;¶ \nA ;´ \n)è) !þ \n)à!ÿ \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! \n- ³Aq \n- lAqqA G! Aq!A!  t u!A! þ ÿ        t u Aqô  \n)èB\n|! \n)Ø! \n)Ð! \n)À! \n- ÏAq! \n   B   ò : k@ \n- kAqE\r  \nA ;¶ \nA ;´ \n)è) \n! \n)à! \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! \n- ³Aq \n- kAqqA G! Aq!A!  t u!A!          t u Aqô  \n)èB|! \n)Ø!  \n)Ð!¡ \n)À!¢ \n- ÏAq!£ \n    ¡B  £ ¢ò : j@ \n- jAqE\r  \nA ;¶ \nA ;´ \n)è) !¤ \n)à!¥ \n)Ø!¦ \n)Ð!§ \n- Ï!¨ \n)À!© \n)¸!ª \n/¶!« \n/´!¬ \n- ³Aq \n- jAqqA G!­ ¨Aq!®A!¯ « ¯t ¯u!°A!± ¤ ¥ ¦ § ® © ª ° ¬ ±t ±u ­Aqô  \n)èB|!² \n)Ø!³ \n)Ð!´ \n)À!µ \n- ÏAq!¶ \n ² ³ ´B  ¶ µò : i@ \n- iAqE\r  \nA ;¶ \nA ;´ \n)è) !· \n)à!¸ \n)Ø!¹ \n)Ð!º \n- Ï!» \n)À!¼ \n)¸!½ \n/¶!¾ \n/´!¿ \n- ³Aq \n- iAqqA G!À »Aq!ÁA!Â ¾ Ât Âu!ÃA!Ä · ¸ ¹ º Á ¼ ½ Ã ¿ Ät Äu ÀAqô  \n)èB|!Å \n)à!Æ \n)Ø!Ç \n)Ð!È \n- Ï!É \n)À!Ê \n)¸!Ë \n/¶!Ì \n/´!Í ÉAq!ÎA!Ï Ì Ït Ïu!ÐA!Ñ Å Æ Ç È Î Ê Ë Ð Í Ñt Ñuñ \n	 \n)èB|!Ò \n)à!Ó \n)Ø!Ô \n)Ð!Õ \n- Ï!Ö \n)À!× \n)¸!Ø \n/¶!Ù \n/´!Ú ÖAq!ÛA!Ü Ù Üt Üu!ÝA!Þ Ò Ó Ô Õ Û × Ø Ý Ú Þt Þuñ  \nA 6d@@ \n(d \n)è( \nIAqE\r \n)è)  \n5dB|!ß \n)Ø!à \n)Ð!á \n)À!â \n- ÏAq!ã \n ß à áB  ã âò : c@ \n- cAqE\r  \nA ;¶ \nA ;´ \n)è)  \n5dB|) !ä \n)à!å \n)Ø!æ \n)Ð!ç \n- Ï!è \n)À!é \n)¸!ê \n/¶!ë \n/´!ì \n- ³Aq \n- cAqqA G!í èAq!îA!ï ë ït ïu!ðA!ñ ä å æ ç î é ê ð ì ñt ñu íAqô  \n)è)  \n5dB|B|!ò \n)Ø!ó \n)Ð!ô \n)À!õ \n- ÏAq!ö \n ò ó ôB  ö õò : b@ \n- bAqE\r  \nA ;¶ \nA ;´ \n)è)  \n5dB|)!÷ \n)à!ø \n)Ø!ù \n)Ð!ú \n- Ï!û \n)À!ü \n)¸!ý \n/¶!þ \n/´!ÿ \n- ³Aq \n- bAqqA G! ûAq!A! þ t u!A! ÷ ø ù ú  ü ý  ÿ t u Aqô  \n \n(dAj6d @ \n)è- AqE\r  \n)èB\n|! \n)Ø! \n)Ð! \n)À! \n- ÏAq! \n   B   ò : a@ \n- aAqE\r  \nA ;¶ \nA ;´ \n)è) \n! \n)à! \n)Ø! \n)Ð! \n- Ï! \n)À! \n)¸! \n/¶! \n/´! \n- ³Aq \n- aAqqA G! Aq!A!  t u!A!          t u Aqô  \n)èB|! \n)Ø! \n)Ð! \n)À! \n- ÏAq! \n   B   ò : `@ \n- `AqE\r  \nA ;¶ \nA ;´ \n)è) ! \n)à! \n)Ø! \n)Ð!  \n- Ï!¡ \n)À!¢ \n)¸!£ \n/¶!¤ \n/´!¥ \n- ³Aq \n- `AqqA G!¦ ¡Aq!§A!¨ ¤ ¨t ¨u!©A!ª      § ¢ £ © ¥ ªt ªu ¦Aqô  \nA 6\\@@ \n(\\ \n)è( IAqE\r \n)è) \n \n5\\B|!« \n)Ø!¬ \n)Ð!­ \n)À!® \n- ÏAq!¯ \n « ¬ ­B  ¯ ®ò : [@ \n- [AqE\r  \nA ;¶ \nA ;´ \n)è) \n \n5\\B|) !° \n)à!± \n)Ø!² \n)Ð!³ \n- Ï!´ \n)À!µ \n)¸!¶ \n/¶!· \n/´!¸ \n- ³Aq \n- [AqqA G!¹ ´Aq!ºA!» · »t »u!¼A!½ ° ± ² ³ º µ ¶ ¼ ¸ ½t ½u ¹Aqô  \n)è) \n \n5\\B|B|!¾ \n)Ø!¿ \n)Ð!À \n)À!Á \n- ÏAq!Â \n ¾ ¿ ÀB  Â Áò : Z@ \n- ZAqE\r  \nA ;¶ \nA ;´ \n)è) \n \n5\\B|)!Ã \n)à!Ä \n)Ø!Å \n)Ð!Æ \n- Ï!Ç \n)À!È \n)¸!É \n/¶!Ê \n/´!Ë \n- ³Aq \n- ZAqqA G!Ì ÇAq!ÍA!Î Ê Ît Îu!ÏA!Ð Ã Ä Å Æ Í È É Ï Ë Ðt Ðu ÌAqô  \n \n(\\Aj6\\  \n)è/ J!ÑA!Ò Ñ Òt Òu!Ó \n/¶!ÔA!Õ Ó Ô Õt Õuj!Ö \n)è Ö; J \n)è/ L!×A!Ø × Øt Øu!Ù \n/´!ÚA!Û Ù Ú Ût Ûuj!Ü \n)è Ü; L \nBð|$ Ô~# B0}! $    7(  7   7 )(! ) !  )7  ) 7    ö 6@ (AGAqE\r  ))  (­B|) -  AÿqA	FAqE\r  )(! ))  (­B|) B|!  ) 7   )  7   B0|$ Ò~# BÀ }! $   70 A 6,@@@ (, )0(IAqE\r )0)  (,­B|!  )7   ) 7   )7   ) 7@ B| B|° AqE\r   (,6<  (,Aj6,  A6< (<! BÀ |$  ¦~~# BÐ }! $    6L  7@  78  70  Aq: /  7   )8)  5LB|) 7@@ - /AqE\r  (LAj )@(FAqE\r   )8)  )8(Ak­B|) B|7 A 6@@ ( )(IAqE\r  ))  5B|) 7  )@! )8! )0!	 - /!\n ) !@    	 \nAq ò Aq\r  )@! ) !\r   \ró  )0 )  ) ï   (Aj6  )@! ) ! B|  ó  )0 ) ) ï  BÐ |$ ~~# BÀ }! $    70  7( )0-  Awj! A	K@@@@@@ \n  )0B|! )(!  ) 7   )  7  ) 7  )7  B| ö AGAq: ? A Aq: ?  )0- Aq: ? A Aq: ? AAq: ? - ?Aq! BÀ |$  ~# B0}! $    7(  7   7B !  7  7  )((6  ) (­B§Í 7 )! )() ! (­B!@ P\r    ü\n   A 6@@ ( (IAqE\r ) (­B| )  )ó   (Aj6  )(!  )7  )7  B0|$ ~# BÀ }! $    70  6, A 6(@@@ (( )0(IAqE\r  )0)  ((­B|7  ) !  )7  ) 7  )7  ) 7 @ B| ° AqE\r @ ) ( (,FAq\r  ) ( (,IAqE\r ) - 0AqE\r  ) 78  ((Aj6(  B 78 )8! BÀ |$  ~# B0}! $    7(  7   7  7 A 6@@ ( )((IAqE\r )()  (­B|)  )  ) )ü   (Aj6  B0|$ Æ~# B°}! $    7¨  7   7  7 )¨1  !@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  )¨B| )  ) )û  )¨)  )  ) )ü  )¨B|B| )  ) )û  )¨)  )  ) )ü  A 6@@ ( ) (IAqE\r )¨B|! ) )  (­B|!  ) 7  )  7ø  )7  )ø7  )7  ) 7 @ B| ° AqE\r  )¨B|! ))  (­B|!	  	) 7   	)  7    (Aj6  )¨)  )  ) )ü  )¨B|B| )  ) )û  A 6ô@@ (ô )¨( "IAqE\r )¨)  (ô­B~|)  )  ) )ü  )¨)  (ô­B~|B| )  ) )û   (ôAj6ô @ )¨- *AqE\r  )¨B|B0| )  ) )û  )¨)  )  ) )ü  )¨B|B| )  ) )û  )¨)  )  ) )ü  A 6ð@@ (ð ) (IAqE\r )¨B|!\n ) )  (ð­B|!  \n) 7è  \n)  7à  )è78  )à70  )7(  ) 7 @ B0| B |° AqE\r  )¨B|! ))  (ð­B|!\r  \r) 7   \r)  7    (ðAj6ð \r )¨)  )  ) )ü  )¨) \n )  ) )ü  )¨)  )  ) )ü  )¨)  )  ) )ü  A 6Ü@@ (Ü ) (IAqE\r )¨B|! ) )  (Ü­B|!  ) 7Ð  )  7È  )Ð7X  )È7P  )7H  ) 7@@ BÐ | BÀ |° AqE\r  )¨B|! ))  (Ü­B|!  ) 7   )  7    (ÜAj6Ü  )¨B| )  ) )û \n A 6Ä@@ (Ä ) (IAqE\r )¨B|! ) )  (Ä­B|!  ) 7¸  )  7°  )¸7x  )°7p  )7h  ) 7`@ Bð | Bà |° AqE\r  )¨B|! ))  (Ä­B|!  ) 7   )  7    (ÄAj6Ä 	 A 6¬@@ (¬ )¨( \nIAqE\r A 6¨@@ (¨ ) (IAqE\r )¨)  (¬­B|! ) )  (¨­B|!  )7  ) 7  )7  ) 7@ B| B|° AqE\r  )¨)  (¬­B|! ))  (¨­B|!  )7  ) 7   (¨Aj6¨   (¬Aj6¬  )¨B|B| )  ) )û  A 6¤@@ (¤ )¨( \nIAqE\r )¨)  (¤­B|)  )  ) )ü  )¨)  (¤­B|) )  ) )ü   (¤Aj6¤ @ )¨- AqE\r  )¨) \n )  ) )ü  )¨)  )  ) )ü  A 6 @@ (  )¨( IAqE\r )¨) \n ( ­B|)  )  ) )ü  )¨) \n ( ­B|) )  ) )ü   ( Aj6   B°|$ ~# B0}! $   7(  7   )(BÝ ¢ 7@@ )B RAq\r   B 7   A6  B|A 6  )B A«   )® §6  )  (Í 7 )B A «  )! (­! )! B  ¨  )    )7   )7  B0|$ ­~# B }! $    7  )Bþ ¢ 7@@ )B RAq\r  A Aq:  ) ! (­! )! B  ±  )  AAq:  - Aq! B |$  ~# B}!   7 @@ ) ( \r  A Aq: @ ) ( AFAqE\r   ) ))B RAq: @ ) ( AFAqE\r   ) (A GAq: @ ) ( AFAqE\r   ) )B RAq: @ ) ( AFAqE\r   ) +B ¹bAq: @ ) ( AFAqE\r   ) - Aq:  AAq:  - Aqò	~~~# Bà }! $    7X  7P  7H@ )P( )P(FAqE\r @@ )P(\r  )PA6 )P!  (At6  )XB| )P(­B§Í 7@ )@! )P) ! )P(­B!@ P\r    ü\n   )@!	 )P 	7   )XÒ 78 )8!\n A6 B|B|A 6  B|B|!  )7  ) 7   )X7( A60 A : 4 B|B%|!A !\r  \r:   \r;   \n )07  \n )(7 \n ) 7 \n )7 \n )7   )87   )H7 )P) ! )P! (!  Aj6  ­B|!  )7  ) 7  Bà |$ \r~~~~~~~~~~~~~# Bð }! $    7h  7`  6\\  Aq: [  7P )`5 !@@ BV\r @@@@@@@@@ §	   )hBÇ ¶ 	 )h!AÛ !A!	   	t 	uµ   )`))7H@@ )HB RAqE\r@ )H )`))RAqE\r  )h!\nA !A! \n  t uµ @ )H) ( AFAqE\r  )h!\rA\'!A! \r  t uµ  )h! )H) ! (\\! - [! )P!    Aq  @ )H) ( AFAqE\r  )h!A\'!A!   t uµ   )H)7H  )h!AÝ !A!   t uµ @@ - [AqE\r  )hB ¶  )h! )`B|!  )7   ) 7  B|· @@ - [AqE\r  )hB ¶  )h )`)¸ @@ - [AqE\r  )hBõ ¶  )h )`+¹ @@ - [AqE\r  )hB¯ ¶ @@ )`- AqE\r  )hBÄ ¶  )hBØ ¶  )h!AÛ !A!   t uµ  A 6D@@ (D )`)(IAqE\r@ (DA KAqE\r  )h! A !!A!"   ! "t "uµ  )h!# )`))  (D­B|!$  $)70  $) 7( # B(|·   (DAj6D  )hB© ¶  )hB ¶  A 6@@@ (@ )`(IAqE\r A 6<@@ (< (\\AjIAqE\r )hB ¶   (<Aj6<  )h!% )`)!& 5@!\'B!( & \' (|) !) (\\!*A!+ * +j!, )P!- % ) , + - [q -  )hB ¶  )h!. )`) 5@ (|)!/ + (\\j!0 - [!1 )P!2 . / 0 1Aq 2  )h!3A\n!4A!5 3 4 5t 5uµ   (@Aj6@  A 68@@ (8 (\\IAqE\r )hB ¶   (8Aj68  )h!6Aý !7A!8 6 7 8t 8uµ  )hB ¶ B )  !9 A6  9Bù  £ B )  !:  )`( 6 :BÍ  B|£  )PA6H )PB7P Bð |$ Û~# Bð }! $    7h  7` )h!  -  AF:  )h1  !@ BV\r @@@@@@@@@@@@@@@@@@@ § 	\n\r  )hB| )`  )h)  )`  )hB|B| )`  BÈ |! )hB|!  ) 7   )  7    )h7X@ )`( )`(MAqE\r @@ )`(E\r @@ )`( )`(MAqE\r )`!  (At6  )`)  )`(­B~ù ! )` 7  )`A6Bö !	 )` 	7  )`)  )`(­B~|!\n \n )X7 \n )P7 \n )H7  )`!  (Aj6 )h)  )`  )hB|B| )`  A 6D@@ (D )h( "IAqE\r )h)  (D­B~|)  )`  )h)  (D­B~|B| )`   (DAj6D  )hB|B0| )`  )h)  )`  )hB|B| )`  )h)  )` \r )h)  )`  )h) \n )`  )h)  )`  )h)  )`  )hB| )` \n  )`(6@@@ (@A KAqE\r  )`)  (@­B~|Bh|78 )8! )hB|!\r  \r) 70  \r)  7(  )7  ) 7  )07  )(7 @ B| ° AqE\r  )8)- !A !@ Aÿq AÿqGAqE\r  )8)A : @@ )8)) -  AÿqAFAqE\r  )8)) B|B| )`  )8))  )`   (@Aj6@ 	 )hB|B| )`  A 6$@@ ($ )h( \nIAqE\r )h)  ($­B|)  )`  )h)  ($­B|) )`   ($Aj6$ @ )h- AqE\r  )h) \n )`  )h)  )`  A 6 @@ (  )h( IAqE\r )h) \n ( ­B|)  )`  )h) \n ( ­B|) )`   ( Aj6   Bð |$ ~# B }! $    7  7 A 6@@ ( )(IAqE\r ))  (­B|)  )   (Aj6  B |$ h~# B }! $    7B !  7  7 ) B| @ )B RAqE\r  )ø  B |$  BÂ ¹~~~~~~~~~~# B°}! $   7¨  7   7  7  :  A 6@@@ (B (ø IAqE\r B )ð  (­B|7 )) ! )¨!	  )7  ) 7  	)7  	) 7 @ B| ° AqE\r @ ) ( ) ( )( jIAqE\r  ) ( )( j!\n )  \n6 ) )  ) (­Bù ! )  7  ) )  ) (­B|! ))!\r )( ­B!@ P\r   \r ü\n   )( ! ) !   (j6@ )( )( )(0jIAqE\r  )( )(0j! ) 6 ))  )(­Bù ! ) 7  ))  )(­B|! ))(! )(0­B!@ P\r    ü\n   )(0! )!   (j6 ) )B8|) 7  )B|!   )7   ) 7   (Aj6   )AÍ 7ø )¨(! )ø 6 ) )ø(Í ! )ø 7  )ø) ! )¨! ) ! 5!@ P\r    ü\n   ) )ø Bè !A !  B|   ü   )7  ) 7  7¨  ) 7À  )ø7È  )7Ð  )7Ø  - Aq: à B|BØ |!! B| B|B   ! )7 ! )7   ) (6x  (x6|  ) (|­B§Í 7p )p!" ) ) !# (x­B!$@ $P\r  " # $ü\n    )(6h  (h6l  ) (l­B§Í 7` )`!% )) !& (h­B!\'@ \'P\r  % & \'ü\n    )ø7  B |B|!( B|BØ |!) ( ))7 ( )) 7  B |B|!* * )x7 * )p7  B |B(|!+ + )h7 + )`7  B |B8| )) 7 @B (ü B (ø MAqE\r @@B (ü E\r @@B (ü B (ø MAqE\rB (ü At!,B  ,6ü  B )ð B (ü ­Bù !-B  -7ð A!.B  .6ü BÀ ö !/B  /7ð B )ð B (ø ­B|!0 0 )X78 0 )P70 0 )H7( 0 )@7  0 )87 0 )07 0 )(7 0 ) 7 B (ø Aj!1B  16ø  )°ø  B|BØ |!2   2)7   2) 7  B°|$  ~# BÀ }! $    78  70 A 6,@@@ (, )8(IAqE\r )8)  (,­B|) ! )0!  )7   ) 7  )7  ) 7@ B| B|° AqE\r   (,Aj6, @ )8( )8(MAqE\r @@ )8(E\r @@ )8( )8(MAqE\r )8!  (At6  )8)  )8(­Bù ! )8 7  )8A6Bö ! )8 7  )0! )8)  )8(­B| 7  )8!	 	 	(Aj6 BÀ |$ ¶~~# Bð }! $   7h  7`B !  7X  7P )h! B0|  @ - L!A ! Aq! !	@ \r  )0§!\nA \nt¬ )`B RAs!	@ 	AqE\r   )hA Aq 7( )(! )h)H! BÐ |  ï  )h!\r B| \r   ) 7H  )7@  )78  )70   )P7    (X6  B|A 6  Bð |$ ~# BÀ }! $   78 )8!  )(70  ) 7(  )7   )7  )7  ) 7   )8  )8 B|  BÀ |$ ¾4#~~~	~|~~~~~~~~~~~~~~# BÐ}! $    7ÀA!   q: ¿  )À)HAÒ Í 7° )°!BÒ !A ! BÞ|  ü   BÞ| ü\n   )À!BªA! B¸|    )À)8!	 )° 	7 B /Ð!\n )° \n; J /Ò! )° ; L  : · )¸Bo|!@@ BV\r @@@@@@@@ §  )°A\n:   )°B|!\r  )ÀB|7  (ÈAk6 B|B|A 6  )À)H! B |  )7Ø  )7Ð B | BÐ| ð  \r )¨7  \r ) 7   )°A	:   )°B|! B¸|B|! )À)H! B|  )7è  ) 7à B| Bà| ð   )7   )7   )°A:   B¸|B|!  )7ø  ) 7ð Bð|± ! )° 7  )°A:   B¸|B|!  )7  ) 7 B|² ! )° 9  )°A\r:   B¸|B|! BÄ 7ð A6ø Bð|B|A 6   )7¨  ) 7   )ø7  )ð7 B | B|° ! )° Aq:  )À!  )(7è  ) 7à  )7Ø  )7Ð  )7È  ) 7À )À!B! B°|    )À! B|    )À! Bð|   - !A ! Aq! ! @ \r  )À!! BÐ| !  )ÐBQ! @@  AqE\r  )À BÀ|  )°A:   )°B|!" )À!# B | #  " )È7 ( " )À7   " )¸7  " )°7  " )¨7  " ) 7   )°A:   )°B|!$ $ )¸7  $ )°7   )°A:   )°B|!% )À!& B| &  % )7  % )7   )°A:   A : ·@@ - ·Aq\r  )À!\' Bð| \'   )7Ð  )7È  )ø7À  )ð7¸ )¸B}|!(@@ (BV\r @@@@@@@@@ (§ 			  )À!) BÐ| )  )À!* B°| *Bx  )°A:   )°B|!+ B°|B|!, )À)H!- B |  ,)7  ,) 7  B |  -ð  + )¨7  + ) 7   )ÀA Aq !. )° .7  )À!/ B| /B 	 )À!0 Bà\r| 0  )°A:   )ÀA Aq !1 )° 17  )°B|B|!2 )À!3 BÐ\r| 3Bà  2 )Ø\r7  2 )Ð\r7   )À!4 B°\r| 4Bà B !5  57¨\r  57 \r@@ )°\rBQAqE\r  )ÀA Aq 7\r B\r|B|!6 )À!7 Bø| 7Bà  6 )\r7 6 )ø7 @ (¬\r (¨\rMAqE\r @@ (¬\rE\r @@ (¬\r (¨\rMAqE\r  (¬\rAt6¬\r   ) \r (¬\r­B~ù 7 \r A6¬\r Bö 7 \r ) \r (¨\r­B~|!8 8 )\r7 8 )\r7 8 )\r7   (¨\rAj6¨\r )À!9 BØ| 9Bà   )ð7È\r  )è7À\r  )à7¸\r  )Ø7°\r  (¨\r!: )° :6 " )À)H )°( "AlÍ !; )° ;7  )°!< <) != ) \r!> <5 "B~!?@ ?P\r  = > ?ü\n   ) \rø  )°\rBQ!@ )° @: *@ )°- *AqE\r  )°B|B0|!A )À!B BÈ| BB  A )Ð7  A )È7   )À!C B¨| CB  )À!D B| D  )À )°/ JAÿÿq )°/ LAÿÿq   )°7È	 )À!E Bè| E  )°A:   )ÀA Aq !F )° F7  )°B|B|!G )À!H BØ| HB  G )à7  G )Ø7   )À!I B¸| IB  )À!J B| J  )À!K Bè\n| KB  Bè\n|B|!L  L)7  L) 7  )B|7  (Ak6 )À!M BÈ\n| MB  )À)8!N B¸\n|  N)7  N) 7 B¸\n| B|  )À)H!O  )À\n7¨  )¸\n7   B | O 7°\nB !P  P)À 7 \n  P)¸ 7\n  P)° 7\n  )°\n7\nB !Q  Q7\n  Q7\nB !R  R)Ð 7ø	  R)È 7ð	B !S  S7è	  S7à	 A 6Ü	@@ (Ü	­BTAqE\r 5Ü	B B\n||) !T B\n| T¶   )7  )7 B\n| B|· @ )À- PAqE\r  BØ 7È	 A6Ð	 BÈ	|B|A 6  B\n|  )Ð	7ø  )È	7ð B\n| Bð|·  )\n!U )À)H!V B¸	| U Vý   )À	7ø	  )¸	7ð	@ (ø	AGAqE\r   (\nAj6\n B	|  )\n7¨  )\n7  B	| B |´  )À)H!W B¨	|  ) 	7¸  )	7° B¨	| B°| Wð   )°	7è	  )¨	7à	@ )À- PAqE\r   (\nAk6\n BÞ 7	 A6	 B	|B|A 6  B\n|  )	7è  )	7à B\n| Bà|·  )\n!X )À)H!Y Bø| X Yý   )	7ø	  )ø7ð	@ (ø	AGAqE\r   (\nAj6\n BØ|  )\n7È  )\n7À BØ| BÀ|´  )À)H!Z Bè|  )à7Ø  )Ø7Ð Bè| BÐ| Zð   )ð7è	  )è7à	 A 6\n  (Ü	Aj6Ü	 @ )\nB RAqE\r  )\nø @ (ø	AFAqE\r B )  ![ A6 [B½  B|£ B )  !\\ )À)8(!] )À)8) !^ /ÐAÿÿqAj!_ /ÒAÿÿqAj!` (!a )!b BÀ | b7  B8| a6  B4| `6  B0| _6   ^7(  ]6  \\BÎ  B |£ A   A : × A 6Ð@@ (Ð )À)@(IAqE\r )À)@)  (Ð­B|) !c  c)7  c) 7  )è	7  )à	7@ B| B|° AqE\r  A: ×  (ÐAj6Ð @ - ×AqE\r   )ð	7À A6È BÀ|B|A 6   )À)HAÍ 7¸ )¸!d d )è	7 d )à	7  Bã 7¨ A6° B¨|B|A 6   )È7x  )À7p  )°7h  )¨7`@@ Bð | Bà |° AqE\r  )À)@ )¸  )ð	!e (ø	!f )À)@!g )À)H!h B| e f g hÅ @ )À)0( )À)0( ( jIAqE\r  )À)0( ( j!i )À)0 i6@@ )À)0(\r  )À)0(­Bö !j )À)0 j7  )À)0)  )À)0(­Bù !k )À)0 k7  )À)0)  )À)0(­B|!l )!m ( ­B!n@ nP\r  l m nü\n   ( !o )À)0!p p o p(j6 B 7 )°B|!q )¸!r )À!s s)0!t s)@!u s- P!v B|  )ø	7X  )ð	7P vAq!w B| BÐ | r t u B| w  q )7  q )7   )À!x Bà| x  )À!y B°| yBx  B°|B|!z  z)7Ø  z) 7Ð )À!{ B| {   )¨7Ð  ) 7È  )7À  )7¸@@ )¸BQAqE\r  )À!| Bð| |  )°A:   )°B|!} )À)H!~ Bà|  )Ø7¸  )Ð7° Bà| B°| ~ð  } )è7  } )à7   )ÀA Aq ! )° 7  )ÀA Aq ! )° 7  )°A:   )°B|! )À)H! BÐ|  )Ø7È  )Ð7À BÐ| BÀ| ð   )Ø7   )Ð7   )ÀA Aq ! )° 7  )À! B°| B  )À! B|   )°A:   )À! Bð|   )ðBR! )° : @ )°- AqE\r  )ÀA Aq ! )° 7 \n )À! BÐ| B  )À! B°|   )°A:   )°B|! )À! B|    )¨7   ) 7   )7   )À! Bø|   )°A :   )°B|! )À! Bè| B   )ð7   )è7   )À! BÈ| B  )°A:   )ÀA Aq ! )° 7  )°B|B|! )À! B¸| B   )À7   )¸7   )À! B| B @ - ¿Aq\r @ )À! BØ|    )ð7Ð  )è7È  )à7À  )Ø7¸  )Ð7  )È7  )À7  )¸7ø - !A ! Aq! !@ \r  )¸BQ!@ AqE\r  )À! B¸|    )À)HAÒ Í 7° )°A:   )°! )° 7  )ÀAAq ! )° 7 \n )À)8! )° 7 B /Ð! )° ; J /Ò! )° ; L  )°7°  )°7È )È!  BÐ|$   }~# B}! $   7@@ )   ))8 ))H !  6 AFAqE\r @ (AFAqE\r   A:  B|$ j~# B}!   7  7  )! ) !  )7  ) 7  ) (! ) 6 ) (! ) 6¹~~~~# Bð }! $   7h  7`   )h @  - AqE\r B )  ! AÐ6  B½  £ B )  ! )h)8(!  )h)8) 7  6 BÜ  B|£  )` B )  BÚ B £ A    ) §!@A t¬ )`B RAqE\r  Bð |$ B )  ! AÛ60 B½  B0|£ B )  !	 )h)8(!\n )h)8) !  /AÿÿqAj!  /AÿÿqAj!\r BÔ | \r6  BÐ | 6   7H  \n6@ 	B¾  BÀ |£  )` B )  !  (!   )7(  6  B¢  B |£ A  ~~# B }! $   7B !   7(   7    7   7   7   7 B !  7  7 )! Bè| Bx @@ )èBRAqE\r Bè|B|! ))H! BØ|  )7  ) 7 BØ| B| ð @ ( (MAqE\r @@ (E\r @@ ( (MAqE\r  (At6   ) (­Bù 7 A6 Bö 7 ) (­B|!  )à7  )Ø7   (Aj6 )!	 B¸| 	Bx   )Ð7  )È7ø  )À7ð  )¸7è  )!\nB! B| \n     (6   ))H  (AtÍ 7   ) ! )!\r 5B!@ P\r   \r ü\n   )ø  )! Bø|  @@ - Aq\r  )øBQAqE\r  )! BØ|   )! B¸| B   )ÀB|7¨  (ÈAk6° B¨|B|A 6   B |! ))H! B|  )°7   )¨7 B| B| ð   ) 7  )7  )!B ! B|      )7   )7 )! Bè |   - !A ! Aq! !@ \r  )! BÈ |   )HBQ!@ AqE\r  )! B(|   B |$ \n~# B }! $   7B !   7   7 B !  7  7@@ )! Bè |   )hBRAqE\r  )A Aq 7` )! BÀ | B   )A Aq 78  )`7(  )870@ ( (MAqE\r @@ (E\r @@ ( (MAqE\r  (At6   ) (­Bù 7 A6 Bö 7 ) (­B|!  )07  )(7   (Aj6    (6   ))H  (­B§Í 7   ) ! )!	  (­B!\n@ \nP\r   	 \nü\n   )ø  )! B| BÀ   B |$ ª\r	~~# Bð}! $    7è  6ä  6àB !  7Ø  7Ð  7È  7À  7¸  7°  7¨  7  )è! B| Bx  B |! B|B|! )è)H! Bð|  )7(  ) 7  Bð| B | ð   )ø7  )ð7   (ä6Ô  (à6Ø )è!	 BÐ| 	B B !\n  \n7È  \n7À )è! B |  @ - ¼!A !\r Aq! \r!@ \r  ) BR!@ AqE\r  )è! B| Bx @ )BQAqE\r  A: Ð )è! Bà| Bx   )ø7  )ð7  )è7  )à7 B|B|! )è)H! BÐ|  )7  ) 7  BÐ|  ð @ (Ì (ÈMAqE\r @@ (ÌE\r @@ (Ì (ÈMAqE\r  (ÌAt6Ì   )À (Ì­Bù 7À A6Ì Bö 7À )À (È­B|!  )Ø7  )Ð7   (ÈAj6È B|B|! )è)H! BÀ|  )7  ) 7 BÀ| B| ð @ (Ì (ÈMAqE\r @@ (ÌE\r @@ (Ì (ÈMAqE\r  (ÌAt6Ì   )À (Ì­Bù 7À A6Ì Bö 7À )À (È­B|!  )È7  )À7   (ÈAj6È )è! B |    )¸7¸  )°7°  )¨7¨  ) 7   (È6¸  )è)H (¸­B§Í 7° )°! )À! (¸­B!@ P\r    ü\n   )Àø  )è! B| B  )è! Bà | B  B |B |! )è! BÐ | B   )X7  )P7  )è!  B0|  B @ )è)0( )è)0(MAqE\r @@ )è)0(E\r @@ )è)0( )è)0(MAqE\r )è)0!! ! !(At6  )è)0)  )è)0(­Bù !" )è)0 "7  )è)0A6BÀ ö !# )è)0 #7  )è)0)  )è)0(­B|!$ $ )Ø78 $ )Ð70 $ )È7( $ )À7  $ )¸7 $ )°7 $ )¨7 $ ) 7  )è)0!% % %(Aj6 Bð|$ ¦~# B}!  (6@@@ (A KAqE\r )  (Ak­|-  !A!@  t uA/FAqE\r    ) 7    (6  B|A 6   (Aj6   B 7   A 6  B|A 6 ~# B}! $   7  )  (Aj­B §Í 7  ) !  ) !  (­B !@ P\r    ü\n   )   (­|A :   ) ! B|$  ®~~# Bà}! $   7ØB !   7   7   7    )ØA Aq 7 B !  7Ð  7È@ )Ø! Bè |    )7À  )x7¸  )p7°  )h7¨  )À7   )¸7  )°7  )¨7 - ¤!A ! Aq! !	@ \r  )¨BR!	@ 	AqE\r   )ØA Aq 7` )Ø!\n BÀ | \nB   )ØA Aq 78  )`7(  )870@ (Ô (ÐMAqE\r @@ (ÔE\r @@ (Ô (ÐMAqE\r  (ÔAt6Ô   )È (Ô­Bù 7È A6Ô Bö 7È )È (Ð­B|!  )07  )(7   (ÐAj6Ð   (Ð6   )Ø)H  (­B§Í 7  )! )È!\r  (­B!@ P\r   \r ü\n   )Èø  )Ø! B| B  Bà|$ ¼ ~~~~~~~~~~~~~~~~# BÐ}! $    7À  7¸  7°  7¨@@ )À(A KAqE\r  B 7  )À)! )À! B|   B | B|   )À(;  )À(;@ ) BQAqE\r  )À!  (Aj6 )ÀA 6 A6Ì@ ) BQAqE\r @ )À!  )7  ) 7  A  B| !	  	6üA !\n@ 	E\r  (üA\nG!\n@ \nAqE\r  (! )À!  )  ­|7  (!\r )À!  ( \rk6 A6Ì@ ) B QAqE\r  (! )À!   (j6 A6Ì@ ) BQAqE\r  )À!  )7H  ) 7@A !  BÀ |  Bø| 6ôB )  ! Aß6 B½  B|£ B )  ! )°(! )°) ! )À(Aj! )À(Aj! (ô! B8| 6  B4| 6  B0| 6   7(  6  B´  B |£ A  @@ ) BQAqE\r  )ÀB |! )À) B|-  !A!   t uµ  A : ó@ )À(A K!A ! Aq! ! @ E\r  )À) -  !!A!" ! "t "u!# )À) -  !$A!% # $ %t %uG!&A!\' &Aq!( \'!)@ (\r  - ó!) )! @  AqE\r  )À!*  *)7X  *) 7PA !+  BÐ | + Bì| 6è@@ - óAq\r  (èAÜ GAqE\r@@ - óAqE\r  )ÀB |!, )À )ÀB| !-A!. , - .t .uµ  A 6ä@@ (ä (ìIAqE\r )ÀB |!/ )À)  (ä­|-  !0A!1 / 0 1t 1uµ   (äAj6ä @@ - óAqE\r  A : ó@ (èAÜ FAqE\r  A: ó (ì!2 )À!3 3 3)  2­|7  (ì!4 )À!5 5 5( 4k6 )À!6 6 6(Aj6@ )À(\r B )  !7 A6` 7B½  Bà |£ B )  !8 )°(!9 )°) !: /AÿÿqAj!; /AÿÿqAj!< B| <6  B| ;6   :7x  96p 8B  Bð |£ A   )ÀB |!= )À) -  !>A!? = > ?t ?uµ  )À!@ @ @) B|7  )À!A A A(Aj6 )À!B B B(Aj6  )À) 7À  )À(,6È BÀ|B|A 6  )¨!C BÐ|  )È7  )À7 BÐ| B| Cð   )Ø7  )Ð7 )ÀA 6, (!D )À!E E D E(j6 )¸!F  ) 7  B |B|!G G )7 G )7   /;¸  /;º A : ¼ B |B|!HA !I H I:  H I;   F )¸7 F )°7 F )¨7 F ) 7  A 6Ì A6Ì (Ì!J BÐ|$  J©\n~~8~	~~# B }!   7  7  )) -  :  , APj! AÈ K@@@@@@@@@@@@@ I	 \n A\n:  A\r: \n A	: 	 A:  A:  A:  A :  AÜ :  A :  )!  ) B|7  )!  (Aj6 )!  ( Aj6 @ )(A K!A ! Aq!	 !\n@ 	E\r  )) -  !A!@@  t uA0NAqE\r  )) -  !\rA! \r t uA9L!A! Aq! ! \r )) -  !A!@  t uAá NAqE\r  )) -  !A!  t uAæ L!A! Aq! ! \r )) -  !A!  t uAÁ N!A ! Aq! !@ E\r  )) -  ! A!!   !t !uAÆ L! ! !\n@ \nAqE\r  - !"A!#  " #t #uAt:  )) -  !$A!%@@ $ %t %uA0NAqE\r  )) -  !&A!\' & \'t \'uA9LAqE\r  )) -  !(A!) ( )t )uA0k!* - !+A!,  * + ,t ,uj:  )) -  !-A!.@@ - .t .uAá NAqE\r  )) -  !/A!0 / 0t 0uAæ LAqE\r  )) -  !1A!2 1 2t 2uAá kA\nj!3 - !4A!5  3 4 5t 5uj:  )) -  !6A!7@ 6 7t 7uAÁ NAqE\r  )) -  !8A!9 8 9t 9uAÆ LAqE\r  )) -  !:A!; : ;t ;uAÁ kA\nj!< - !=A!>  < = >t >uj:  )!? ? ?) B|7  )!@ @ @(Aj6 )!A A A) B|7  )!B B B(Aj6 )!C C C( Aj6   - :  A :  )!D D D) B|7  )!E E E(Aj6 )!F F F( Aj6 @ )(A K!GA !H GAq!I H!J@ IE\r  )) -  !KA!L K Lt LuA0N!MA !N MAq!O N!J OE\r  )) -  !PA!Q P Qt QuA9L!J@ JAqE\r  - !RA!S  R St SuA\nl:  )) -  !TA!U@ T Ut UuA0NAqE\r  )) -  !VA!W V Wt WuA9LAqE\r  )) -  !XA!Y X Yt YuA0k!Z - ![A!\\  Z [ \\t \\uj:  )!] ] ]) B|7  )!^ ^ ^(Aj6 )!_ _ _( Aj6  )!` ` `) B|7  )!a a a(Aj6 )!b b b( Aj6   - :  A :  )!c c c) B|7  )!d d d(Aj6 )!e e e( Aj6 @ )(A K!fA !g fAq!h g!i@ hE\r  )) -  !jA!k j kt kuA0N!lA !m lAq!n m!i nE\r  )) -  !oA!p o pt puA7L!i@ iAqE\r  - !qA!r  q rt ruAt:  )) -  !sA!t@ s tt tuA0NAqE\r  )) -  !uA!v u vt vuA7LAqE\r  )) -  !wA!x w xt xuA0k!y - !zA!{  y z {t {uj:  )!| | |) B|7  )!} } }(Aj6 )!~ ~ ~( Aj6  )!  ) B|7  )!  (Aj6 )!  ( Aj6   - :   - :  - !A!  t uº~~# B0}! $    7( A 6$ B 7@@ )B TAqE\r )§!@A t¬ )(B RAqE\r   ($Aj6$  )B|7  B 7 B 7@ )BÀ T!A ! Aq! !@ E\r  ) ($­T!@ AqE\r  )§!@A t¬ )(B RAqE\r @ )B VAqE\r @@ )B| ($­QAqE\r B )  !B¹  ¤ B )  !	B  	¤  )!\nBÂ  \nB|) B )  ¤   )B|7  )B|7 B0|$ ~# B }! $    7  7  )) 7 @@ ) ))B RAq\r   ))0Ô 7  ) ))) 7 )! B |$  ¿~# B0}! $    7   7  )) 7@@ )))B RAq\r   ) )0Ô 7(  ) )0B|AÍ 7 ))))! ) 7  ) ) )0Õ 7( )(! B0|$  á~~# B0}! $    7   7  )) 7@@ )))B RAq\r   ) )0Ô 7(  )))7@ )B R!A ! Aq! !@ E\r  ))B R!@ AqE\r   ))7  )) 7( )(! B0|$  þ~# Bð }! $    7`  7X  )X) 7P  )X)7H@@@ )P( AFAqE\r   )P))7@ A 6<@@ )@B RAqE\r@ )@)  )Hà AqE\r   (<­ )`)0× 7h  )@)7@  (<Aj6< @ )P( AFAqE\r @ )H( )P(MAqE\r  A 68@@ (8 )P( )H(kIAqE\r  )P) (8­|7(  )H(60 B(|B|A 6  )HB|!  )07   )(7  )7  ) 7@ B| B|° AqE\r   (8­ )`)0× 7h  (8Aj68   )`)0Ô 7h )h! Bð |$  õ~~# BÐ }! $    7@  78  )8) 70@@ )0( AFAqE\r   )0))7( A 6$@@ )(B RAqE\r  )()7(  ($Aj6$   ($­ )@)0× 7H@ )0( AFAqE\r  A 6  A 6@@ )0B|! (!  )7  ) 7 B|  B| E\r  ( Aj6   ( (j6   ( ­ )@)0× 7H  )@)0Ô 7H )H! BÐ |$  ¸~# B}! $    7p  7h  )h) 7`  )h)7X  )h)7P  )p Bà | 7H@ )X)B SAqE\r  )XB 7@ )X) )H)UAqE\r  )H)! )X 7@ )P) )H)UAqE\r  )H)! )P 7@ )P) )X)SAqE\r  )X)! )P 7@@ )`( AFAqE\r   )`))7@  )p)0B|AÍ 78  )870 A 6,@@ (, )X)§IAqE\r  )@)7@  (,Aj6,  A 6(@@ ((­ )P)§­ )X)}SAqE\r )p)0B|AÍ ! )0 7 )@) ! )0) 7   )@)7@  )0)70  ((Aj6(   )8 )p)0Õ 7x  )`) )X)|7  )P) )X)}§6  B|B|A 6  )p)0!  ) 7  )7  B| Ö 7x )x!	 B|$  	²~# BÀ }! $    78  70  )0) 7(  )0)7   )8)0B|AÍ 7  )B|7  )()7@@ ) ) )SAqE\r )8)0B|AÍ ! ) 7  )8)0Ò ! ))  7  )) ) A6  )! )) )  7  )) B|7  )B|7  ) )8)0Õ ! BÀ |$  Ê~# BÐ }! $    7@  78  )8) 70  )8)7(B !  7   7@ )(- $Aq\r   )@)0B|AÍ 7   ) B|7  )())7@@ )B RAqE\r  )) 7  )@ B| )0)B AAqâ 7 @ )@(HE\r @@ )(- $AqE\r  )) !  ( Aj6  ) ! ) 7  )@)0B|AÍ ! ) 7  ) ! ))  7   )) B|7  ))7 @@ )(- $AqE\r   )(7H  )  )@)0Õ 7H )H! BÐ |$  ~# Bà }! $    7P  7H  )H) 7@  )H)78B !  70  7(@ )8- $Aq\r   )P)0B|AÍ 70  )0B|7(  )8)7   )8))7@@@ )B RAqE\r  )) 7  )P B| )@)B AAqâ 7@ )P(HE\r @ )( AGAqE\r B )  ! AÝ6  Bå  £ B )  BÝ B £   )P)0Ô 7X@@ )- AqE\r @ )8- $Aq\r  )P)0B|AÍ ! )( 7  )) ! )()  7   )() B|7(  ) )7 @ )8- $AqE\r  ) )) !  ( Aj6  ))! )  7  ))7 @ )8- $AqE\r   )87X  )0 )P)0Õ 7X )X!	 Bà |$  	õ~# Bà }! $    7X  7P  )P) 7H  )P)7@  )P)78  )@70  )8))7(@@ )(B RAqE\r  )07  )() 7  )X B| )H)B AAqâ 7@ )X(HE\r   )70  )()7(  )0! Bà |$  È~~~# BÐ }! $    7H  7@  )@) 78  )@)70B !  7(  7 @ )8- $Aq\r   )H)0B|AÍ 7(  )(B|7   )8))7  )0))7@ )B R!A ! Aq! !@ E\r  )B R!@ AqE\r  )H)0!B!	  	|!\nA! \n Í ! )  7   	 )H)0| Í 7 	 )H)0| Í !\r ) \r7 )) ! )) 7  	 )H)0| Í ! )) 7 )) ! ))) 7 @@ )8- $AqE\r  ) )H)0Õ ! ) 7  ) )H)0Õ ! ) )  7   ) ) B|7   ))7  ))7 )( )H)0Õ ! BÐ |$  £~\n# B0}!   7   7@@ ) (  )( GAqE\r  A Aq: / ) 5 !@ BV\r @@@@@ §	 @@ ) ( )(IAqE\r  ) ! )!  7 A 6@@ ( )(IAqE\r ) ) (­|-  !A!  t u! )) (­|-  !A!	@   	t 	uJAqE\r  AAq: / ) ) (­|-  !\nA! \n t u! )) (­|-  !\rA!@  \r t uHAqE\r  A Aq: /  (Aj6   ) ( )(KAq: /  ) ) ))UAq: /  ) + )+dAq: /  ) - Aq )- AqJAq: / A Aq: / A Aq: / - /Aqà~~~~~~# B }! $    7  7  )) 7  ) ) 7x  )x)Bö 7p  )))7h A 6d@@ )hB RAqE\r )h) ! )p (d­B| 7   )h)7h  (dAj6d B !  ) 7X  ) 7P  )ø 7H  )ð 7@ A 6<@@ (<­BTAqE\r (<­!  BÀ | B|( 68@@ (8 )x)§IAqE\r  )p (8­B|) 70  (86,@ (,! (<­!  BÀ | B|( O!A !	 Aq!\n 	!@ \nE\r  )p! (,!\r (<­!  \r BÀ | B|( k­B|)  )0¢ !@ AqE\r  )p! (,! (<­!   BÀ | B|( k­B|) ! )p (,­B| 7  (<­! BÀ | B|( !  (, k6, )0! )p (,­B| 7   (8Aj68   (<Aj6< @@ )- $AqE\r   )))7  A 6@@ ( )x)§IAqE\r )p (­B|) ! )  7   ) )7   (Aj6  )pø   )7  ))0B|AÍ 7  )B|7 A 6@@ ( )x)§IAqE\r ))0B|AÍ ! ) 7  )p (­B|)  ))0Ñ ! ))  7   )) B|7  (Aj6  )pø   ) ))0Õ 7 )! B |$  Ó~\r~# BÐ}! $    7È  7À  )À) 7¸  )À)7°@@ )¸( AFAqE\r   )¸))7¨@@ )¨B RAqE\r )È )¨ )°)B A Aqâ @ )È(HE\r   )¨)7¨ @@ )¸( AFAqE\r  B 7 A6 B|B|A 6  )È)0!  )7  )7  B| Ö 7  A 6@@ ( )¸(IAqE\r )¸) (­|-  ! ) ) :   )È! )°)!  B | B A Aqâ @ )È(HE\r   (Aj6 @ )¸( AFAqE\r B !  7  7x )È)0! BÍ 7h A6p Bè |B|A 6  Bø |  )p7   )h7B !	  Bø | B| 	  )È)0!\n BÉ 7X A6` BØ |B|A 6  Bø |  )`70  )X7(B ! \n Bø | B(|   )È)0!  )7@  )x78  B8| Ú 7P A 6L@@ (L )¸(IAqE\r )¸) (L­B|) !\r )P) \r7 )¸) (L­B|)! )P) 7 )È! )°)!  BÐ | B A Aqâ @ )È(HE\r   (LAj6L  )È)0Ô ! BÐ|$  	~# BÐ }! $    7@  78  )8) 70B !  7(  7  )0! )@! B | A A Aq  @@ )@(HE\r  B 7H  (,6  )@)0B| (,Í 7 )! ) ! (­!@ P\r    ü\n   ) ø  )@)0!	  )7  )7    	Ö 7H )H!\n BÐ |$  \nJ~# B}! $    7  7  ) ) ) A§ ! B|$  ~~~# BÀ }! $    78  70  6,  (,6   )8)0B| ( Í 7 (,Aj! AK@@@@@   )0)! ) 7  )0)§! ) 6  )0)§! ) ;  )0)§! ) :   )8)0!	  ) 7  )7 B| 	Ö !\n BÀ |$  \nJ~# B}! $    7  7  ) ) ) A§ ! B|$  J~# B}! $    7  7  ) ) ) A§ ! B|$  J~# B}! $    7  7  ) ) ) A§ ! B|$  ~# B0}! $    7   7  )) 7@@ )( AFAqE\r  )B|!  )7  ) 7   ±  ) )0× 7(@ )( AFAqE\r   )- Aq­ ) )0× 7(@ )( AFAqE\r   )+ü ) )0× 7(  ) )0Ô 7( )(! B0|$  Þ~# B0}! $    7   7  )) 7@@ )( AFAqE\r   ))¹ ) )0Ø 7(@ )( AFAqE\r  )B|!  )7  ) 7   ²  ) )0Ø 7(  ) )0Ô 7( )(! B0|$  j~~# B }! $    7  7  )) 7 )ÿ ! ))0! Aq Ù ! B |$  ¸	~~~# B }! $    7  7  )) 7  ))7x@@ )( AFAqE\r  )x( AFAqE\r   )) )x)| ))0× 7@ )( AFAqE\r  )x( AFAqE\r   )+ )x+  ))0Ø 7@ )( AFAqE\r  )x( AFAqE\r B !  7p  7h )B|! Bè |  )7  ) 7  Bè | ·  )xB|! Bè |  )7  ) 7 Bè | B|·   (t6`  ))0B| (`Í 7X )X! )h! (`­!@ P\r    ü\n   )hø  ))0!	  )`7(  )X7   B | 	Ö 7@ )( AFAqE\r  )x( AFAqE\r   ))7P@ )- $Aq\r   ))0B|AÍ 7P ))) ))0Ð !\n )P \n7  )P7H@ )HB R!A ! Aq!\r !@ \rE\r  )H)B R!@ AqE\r   )H)7H@@ )- $AqE\r  )x)) ))Ð ! )H 7 )x)) ))0Ð ! )H 7@ )- $AqE\r   )7  )P ))0Õ 7@ )( AFAqE\r   ))7@@ )- $Aq\r   ))0B|AÍ 7@ ))) ))0Ð ! )@ 7  )@78@ )8B R!A ! Aq! !@ E\r  )8)B R!@ AqE\r   )8)78@@ )- $AqE\r  ))B|AÍ ! )8 7 ))0B|AÍ ! )8 7@@ )- $AqE\r  )x) ))RAqE\r  )x ))Ñ ! )8) 7  )x! )8) 7  )8)B 7@ )- $AqE\r   )7  )@ ))0Õ 7@ )x( AFAqE\r   ))0B|AÍ 70 ))0B|AÍ ! )0 7 )! )0) 7  )x)) ))0Ð ! )0) 7  )0 ))0Õ 7  ))0Ô 7 )! B |$  Þ~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r   )) ))} ) )0× 7(@ )( AFAqE\r   )+ )+¡ ) )0Ø 7(  ) )0Ô 7( )(! B0|$  ÿ~# Bð }! $    7`  7X  )X) 7P  )X)7H@@ )P( AFAqE\r   )P) )H)~ )`)0× 7h@ )P( AFAqE\r   )P+ )H+¢ )`)0Ø 7h@ )P( AFAqE\r B !  7@  78 A 64@@ (4 )H)§IAqE\r )PB|! B8|  )7  ) 7  B8| ·   (4Aj64   )`)0B| (DÍ 7   (D6( B |B|A 6  ) ! )8! ((­!@ P\r    ü\n   )8ø  )`)0!  )(7  ) 7  B| Ö 7h  )`)0Ô 7h )h!	 Bð |$  	Þ~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r   )) )) ) )0× 7(@ )( AFAqE\r   )+ )+£ ) )0Ø 7(  ) )0Ô 7( )(! B0|$  n~# B }! $    7  7  )) 7  ))7  )) ) ) ))0× ! B |$  |~~# B }! $    7  7  )) 7  ))7  ) ) à ! ))0! Aq Ù ! B |$  ~~# B }! $    7  7  )) 7  ))7  ) ) à As! ))0! Aq Ù ! B |$  Ó~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))S! ) )0!  Aq Ù 7( )+ )+c! ) )0!  Aq Ù 7( )(! B0|$  ü~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))W! ) )0!  Aq Ù 7(@ )( AFAqE\r  )+ )+e! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  ü~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))U! ) )0!  Aq Ù 7(@ )( AFAqE\r  )+ )+d! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  ü~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))Y! ) )0!  Aq Ù 7(@ )( AFAqE\r  )+ )+f! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )( AFAqE\r  )) ))B R! ) )0!  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqqA G! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))B R! ) )0!  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqrA G! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )( AFAqE\r  )) ))B R! ) )0!  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqsA G! ) )0!  Aq Ù 7(  ) )0Ô 7( )(! B0|$  c~~# B}! $    7  7  ) ) ÿ As! ))0! Aq Ù ! B|$  Ñ~# BÐ}! $    7À  7¸ )¸) 5 !@@ BV\r @@@@@@@@@ §	   BÇ 7¨ A6° B¨|B|A 6  )À)0!  )°7   )¨7  B| Ö 7È	 B¬ 7 A6  B|B|A 6  )À)0!  ) 70  )7(  B(| Ö 7È BÍ 7 A6 B|B|A 6  )À)0!  )7@  )78  B8| Ö 7È B 7ø A6 Bø|B|A 6  )À)0!  )7P  )ø7H  BÈ | Ö 7È Bõ 7è A6ð Bè|B|A 6  )À)0!  )ð7`  )è7X  BØ | Ö 7È B¯ 7Ø A6à BØ|B|A 6  )À)0!	  )à7p  )Ø7h  Bè | 	Ö 7È B 7È A6Ð BÈ|B|A 6  )À)0!\n  )Ð7  )È7x  Bø | \nÖ 7È Bä 7¸ A6À B¸|B|A 6  )À)0!  )À7  )¸7  B| Ö 7È B 7¨ A6° B¨|B|A 6  )À)0!  )°7   )¨7  B| Ö 7ÈB )  !\r A¤6  \rBå  £ B )  !  )¸) ( 6 B³  B|£   )À)0Ô 7È )È! BÐ|$  `~~# B}! $    7  7  ) ) ( A F! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  `~~# B}! $    7  7  ) ) ( AF! ))0! Aq Ù ! B|$  æ~# B }! $    7  7  )) 7  )))7ø@@@ )øB RAqE\r@ )ø) ( AGAqE\r B )  ! AÔ6  Bå  £ B )  B B £   ))0Ô 7  )ø)7ø B !  7ð  7è Bø |A B  Bè|ì  ))0!Bð ! B| Bø | ü\n    B| Ü 7 )! B |$  Ë\r~~~~~\n~# Bð}! $    7à  7Ø  )Ø) 7Ð  )Ø)7È  )Ø)7À  )Ø)7¸  )Ø) 7°  )È)7  A6¨ B |B|A 6  B 7 A6 B|B|A 6   )¨7¨  ) 7   )7  )7@@@ B | B|° Aq\r  B 7 A6 B|B|A 6   )¨7  ) 7  )7x  )7p B| Bð |° AqE\r  )È7è  )Ð)(6üB !  7ð  7è  7à )È! )À!B!  |! )Ð)!  )7h  )7`A!	 BÐ| Bà |   Bà| Bð| 	  )Ð)!\n  )À|!A ! BÐ| \n    Bð|   ñ  )à)0!\rB! \r |!A!   Í 7È  )à)0| Í ! )È 7  )à)0| Í ! )È) 7  7À  7¸  B¸||! 	 )°- q!  BÐ|  Bà| Æ 7¸   )à)0| (ÀÍ 7° )°! )¸! 5À!@ P\r    ü\n   )¸ø   )°7¸  7¨  7  )à)0!  )À7X  )¸7P  BÐ | Ö 7 )à)0! Bì 7 A6 B| 6  )!  )7H  )7@  B | BÀ |  @ )¸- AqE\r  )Ð)( (üKAqE\r   )Ð))  5üB|7ø  )Ð)( (ük6  )Ð)( (ük6B !  7ð  7è Bð|! )°- !  Bø|  Bà| AqÌ 7è  )à)0B| (ðÍ 7à )à! )è! (ð­! @  P\r     ü\n   )èø   )à7è )à)0!!  )ð7(  )è7   B | !Ö 7Ø )à)0!" B  7È A6Ð BÈ|B|A 6  )Ø!# B |  )Ð78  )È70 " B | B0| # @ (¨AFAqE\r   )à)0Ô 7À )à)0!$ B  7° A6¸ B°|B|A 6  )À!% B |  )¸7  )°7 $ B | B| %  )àø  )à)0!&  )¨7  ) 7    &Ú 7è )è!\' Bð|$  \'~~# BÀ }! $    78  70  )0) 7(  )0)7  B 7 ) )! ) (! )()B |Bà |! B|   B| À   )()B | B|AAqå 7 @ )()(hE\r  )()A 6h@ )())pB RAqE\r  )()B 7p )  )8)0Ñ ! BÀ |$  ~~~~# BÀ }! $    78  70  )0) 7(  )0)7  B 7 ) )! ) (! )()B|! B|    B|Å @ )()( )()( (jIAqE\r  )()( (j! )() 6 )())  )()(­Bù ! )() 7  )())  )()(­B|! )!	 (­B!\n@ \nP\r   	 \nü\n   (! )()!   (j6 )ø  )8)0Ô !\r BÀ |$  \r~~~~# Bð }! $    7h  7`  )`) 7X  )`)7P  )`)7H B 7@B !  78  70 )PB|! )HB|! )X)! B |  )7  ) 7A ! B | B|   B0| BÀ |   )X)! )HB|!	 B |!\nB !A ! BÀ |!\rA ! Aq!A!  t u!A! \n     \r 	   t uñ  )X)B |Bà |! )HB|!  )7  ) 7   )X)B | B |AAqå 7@ )0B RAqE\r  )0ø @ )X)(hE\r  )X)A 6h@ )X))pB RAqE\r  )X)B 7p ) )h)0Ñ ! Bð |$  :~# B }!   7  7  )) 7 )A: $ )n~# B }! $    7  7  )) 7 ))! ) 7P )A6H ))0Ô ! B |$  ê~# B }! $    7  7  )) 7 @@ ) ( AFAqE\r  ) )B SAqE\r  ) )! B  } ))0× 7@ ) ( AFAqE\r  ) +B ¹cAqE\r   ) + ))0Ø 7  ))0Ô 7 )! B |$  ª~|~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r @@ )) ))WAqE\r  ))! ))!   ) )0× 7(@ )( AFAqE\r @@ )+ )+eAqE\r  )+! )+!   ) )0Ø 7(  ) )0Ô 7( )(! B0|$  «~|~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r @@ )) ))YAqE\r  ))! ))!  ¹ ) )0Ø 7(@ )( AFAqE\r @@ )+ )+fAqE\r  )+! )+!   ) )0Ø 7(  ) )0Ô 7( )(! B0|$  å~# BÐ }! $    7@  78  )8) 70  )8)7(@@ )0( AFAqE\r  B7  A 6@@ ( )()§IAqE\r  )0) ) ~7   (Aj6   )  )@)0× 7H@ )0( AFAqE\r  D      ð?9 A 6@@ ( )()§IAqE\r  )0+ +¢9  (Aj6   + )@)0Ø 7H  )@)0Ô 7H )H! BÐ |$  Y~# B }! $    7  7  )) 7 )+ ))0Ø ! B |$  ^~# B }! $    7  7  )) 7 )+Ò  ))0Ø ! B |$  ñ~# BÐ }! $    7H  7@  )@) 78  )@)70  )@)7(  )8( )((j6   )H)0B| ( Í 7 )! )8)! )0)!@ P\r    ü\n   ) )0)|! )()! )((­!@ P\r    ü\n   ) )0)| )((­|!	 )8) )0)|!\n )8(­ )0)}!@ P\r  	 \n ü\n   )H)0!  ) 7  )7 B| Ö !\r BÐ |$  \r¾	~# BÐ }! $    7H  7@  )@) 78  )@)70  )@)7(  )8(­ )()}§6   )H)0B| ( Í 7 )! )8)! )0)!@ P\r    ü\n   ) )0)|! )8) )0)|B|! )8(­ )0)} )()}!@ P\r    ü\n   )H)0!	  ) 7  )7 B| 	Ö !\n BÐ |$  \n½~# BÐ }! $    7H  7@  )@) 78  )@)70  )@)7(  )8(6$@ ($­ )0) )((­|SAqE\r   )0) )((­|§6$  ($6  )H)0B| (Í 7 )! )8)! )0)!@ P\r    ü\n   ) )0)|! )()! )((­!@ P\r    ü\n   ) )0)| )((­|!	 )8) )0)| )((­|!\n )8(­ )0)} )((­}!@ P\r  	 \n ü\n   )H)0!  )7  )7   Ö !\r BÐ |$  \rà~	~~~# B°}! $    7¨  7   ) ) 7  ) )7  )¨)0B|AÍ 7  )7 A 6| A 6x@@ (x )(IAqE\r A6t A 6p@ (p (xj )(I!A ! Aq! !@ E\r  (p )(I!@ AqE\r  )) (p (xj­|-  !A!  t u!	 )) (p­|-  !\nA!@ 	 \n t uGAqE\r  A 6t  (pAj6p@ (tE\r  )¨)0B|AÍ ! ) 7 )¨)0Ò !\r )) \r7   (x (|k6h  )¨)0B| (hÍ 7` )`! )) (|­|! (h­!@ P\r    ü\n   ))) ! A68 B8|B|A 6  B8|B|!  )h7  )`7   )¨)07P A6X A : \\ B8|B%|!A !  :   ;    )X7   )P7  )H7  )@7  )87   (xAj6|  ))7  (xAj6x @ (xA KAqE\r  )¨)0B|AÍ ! ) 7 )¨)0Ò ! )) 7   (x (|k60  )¨)0B| (0Í 7( )(! )) (|­|! (0­!@ P\r    ü\n   ))) ! A6  B|A 6  B|!  )07  )(7   )¨)07 A6  A : $ B%|!A !  :   ;    ) 7   )7  )7  )7  ) 7  ) )¨)0Õ ! B°|$  ~# BÐ }! $    7@  78  )8) 70  )8)7(  )8)7 @@@ )() ) )YAq\r  ) )§ )0(KAqE\r  )@)0Ô 7H  )0) )()|7  ) ) )()}§6 B|B|A 6  )@)0!  )7  )7    Ö 7H )H! BÐ |$  À\n~# B}! $    7  7x  )x) 7p  )x)7hB !  7`  7X  )p))7P@@@ )PB RAqE\r@ )P )p))RAqE\r  )hB|! BØ |  )7(  ) 7  BØ | B |· @ )P) ( AGAqE\r B )  ! A6  Bô  £ B )  B B £   ))0Ô 7 )P) B|! BØ |  )7  ) 7 BØ | B|·   )P)7P   ))0B| (dÍ 7@  (d6H BÀ |B|A 6  )@! )X! (d­!	@ 	P\r    	ü\n   )Xø  ))0!\n  )H78  )@70  B0| \nÖ 7 )! B|$  	~~~~~# BÐ}! $    7À  7¸  )¸) 7°  )¸)7¨@@ )°( )¨(IAqE\r  )À)0! A Aq Ù 7È  )°)7  )¨(6  B¤|!A !  6  )¨!  ) 7   )7  )7  )7  B| B|° :  )À)0!B!  |!	A!\n  	 \nÍ 7  )À)0| \nÍ ! ) 7 )À)0Ò ! )) 7  ))) !\r A6`  6d Bè |!  - Aq: h B|!B !  7   7    )À)07x A6 A :  Bà |B%|!A !  :   ;   \r )7  \r )x7 \r )p7 \r )h7 \r )`7  )À)0B|AÍ ! )) 7  )°) )¨(­|7P  )°( )¨(k6X BÐ |B|A 6  )À)0Ò ! ))) 7  )))) ! A6( B(|B|A 6  B(|B|!  )X7  )P7   )À)07@ A6H A : L B(|B%|!A !  :   ;    )H7   )@7  )87  )07  )(7   ) )À)0Õ 7È )È! BÐ|$  G~# B}! $    7  7  ) ) AÜ ! B|$  ï~~~~# B }! $    7  7  6  )) 7x@@ )x( (IAqE\r   ))0Ô 7 B 7p (Aj! AK@@@@@    )x)) 7p  )x)( ¬7p )x)/ !A!   t u¬7p )x)-  !A!   t u¬7p  ))0B|AÍ 7h ))0B|AÍ !	 )h 	7 ))0Ò !\n )h) \n7  )h)) ! A6@ BÀ |B|A 6  BÀ |B|!  )p7H B|B 7   ))07X A6` A : d BÀ |B%|!\rA ! \r :  \r ;    )`7   )X7  )P7  )H7  )@7  ))0B|AÍ ! )h) 7  )x) (­|70  )x( (k68 B0|B|A 6  ))0Ò ! )h)) 7  )h))) ! A6 B|B|A 6  B|B|!  )87  )07   ))07  A6( A : , B|B%|!A !  :   ;    )(7   ) 7  )7  )7  )7   )h ))0Õ 7 )! B |$  G~# B}! $    7  7  ) ) AÜ ! B|$  G~# B}! $    7  7  ) ) AÜ ! B|$  G~# B}! $    7  7  ) ) AÜ ! B|$  ä~~~# BÀ }! $    70  7(  )() 7 A !B  6   ) ))7@@@ )B RAqE\r )) ! )0!B  A A Aq  @ )0(HE\r  B 78  ))7 B !A !A!   t uµ  B° 7 )!	 Að :  A :  B|!\n B ) 7  	 \n    )0)0Ô 78 )8! BÀ |$  K~# B}! $    7  7  ))@ ))0Õ ! B|$  æ~~# B }! $    7  7  )) 7 )B|!  )7@  ) 78  B8|ã 7ø@@ )øA  E\r  )øø   ))0Ô 7B !  7ð  7è  )øÄ 7à  ))0Ò 7Ø )ØA6  )àB R!A! Aq! !@ \r  ( A6G! !	 )Ø 	Aq:  ))0!\n B 7È A6Ð BÈ|B|A 6  )Ø! Bè|  )Ð70  )È7( \n Bè| B(|  @ )àB RAqE\r  )à @ )ø Bà |Ô A HAqE\r  )øø   ))0Ô 7  ))0Ò 7X )XA6  )! )X 7 ))0!\r B 7H A6P BÈ |B|A 6  )X! Bè|  )P7  )H7 \r Bè| B|   )øø  ))0!  )ð7   )è7  B| Ú 7 )! B |$  y~# B}! $    (Aj­ö 7 )!  ) !  (­!@ P\r    ü\n   )  (­|A :   )! B|$  þ~# Bà }! $    7P  7H  )H) 7@ )@B|!  )7   ) 7  B|ã 78 )8! )P)0B|! B(|  ý  )8ø @@ (0AFAqE\r   )P)0Ô 7X )P)0!  )07  )(7  B| Ö 7X )X! Bà |$  Ë~# BÐ }! $    7H  7@  )@) 78  )@)70 )8B|!  )7  ) 7  B|ã 7( )(! )0B|!  )7   ) 7  B|þ  )(ø  )H)0Ô ! BÐ |$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ã 7 )Ñ  )ø  )()0Ô ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ã 7 )BÉ AÀ A	¾  )()0Ô ! B0|$  P~# B }! $    7  7  6  7  )Ñ ! B |$  \n~# Bð }! $    7`  7X  )X) 7P  )`)0B|AÍ 7H  )H7@ )PB|!  )7  ) 7  B|ã 78  )8Ä 70@@@ )0B RAqE\r @@ )0Ì !  7( B RAqE\r  )(B|Ü §6   )`)0B| ( Í 7 )! )(B|! ( ­!@ P\r    ü\n   )`)0B|AÍ ! )@ 7  )@)7@ )`)0Ò !	 )@ 	7  )@) A6  )@) B|!\n \n ) 7 \n )7   )0  )8ø   )`)0Ô 7h )8ø   )H )`)0Õ 7h )h! Bð |$  £~~# BÀ }! $    70  7(  )() 7  AAA  6@@ (A HAqE\r   )0)0Ô 78 (! A6  A   A6 (AA B|A  (AA B|A B !  7  7 A; A 6  ) )§Aÿÿq³ ;\n@ ( B|A A HAqE\r  (   )0)0Ô 78@ (A A HAqE\r  (   )0)0Ô 78  (¬ )0)0× 78 )8! BÀ |$  ¿~~# B}! $    7  7x  )x) 7p  )x)7h  )p(Aj­ö 7` )`! )p)! )p(­!@ P\r    ü\n   )` )p(­|A :  B !  7X  7P )h)! BÐ | ¸  BÐ |!A !	A!\n  	 \nt \nuµ   )P7HB !  7@  78  70  7(  7   7 A6 A6 @@ )` )H B| B| A HAqE\r  )`ø  )Hø   ))0Ô 7  )( )( )( 6@ (A HAqE\r  )`ø  )Hø   ))0Ô 7 A6 (AA B|A   ( )) )( 6@ (A HAqE\r  )`ø  )Hø  )   ))0Ô 7 )`ø  )Hø  )   (¬ ))0× 7 )! B|$  ~# BÐ }! $    7@  78  )8) 70  )8)7( A; A 6  )()§Aÿÿq³ ; A6  )0)§ B| B| 6@@ (A HAqE\r   )@)0Ô 7H A6 (AA B|A   (¬ )@)0× 7H )H! BÐ |$  `~# B }! $    7  7  )) 7 ))§  ))0Ô ! B |$  ~# B }! $    7  7  )) 7  ))7  ))§ ) ) ) (­A   ))0Ô ! B |$  ¾~~# BÐ }! $    7@  78  )8) 70  )8)7(  )@)0B| )()§Í 7 A 6  B|B|A 6   )0)§6 A; B|BA\nÅ  /!A!@  t uE\r   )0)§ ) )()A  §6 @@ ( \r   )@)0Ô 7H )@)0!  ) 7  )7    Ö 7H )H! BÐ |$  ~~# Bà }! $    7P  7H  )H) 7@ AÀ 6<  )P)0B| (<Í 7( A 60 B(|B|A 6   )@)§6  A;$ A 6@@@ B |BA\nÅ  /&!A!@  t u\r   )@)§ )( (0­| (< (0k­A  §6@ (\r @ (A HAqE\r   )P)0Ô 7X  ( (0j60@ (0 (<OAqE\r   )(7  (<AÀ j6<  )P)0B| (<Í 7( )(! )! (0­!@ P\r    ü\n   @ (0\r   )P)0Ô 7X )P)0!  )07  )(7    Ö 7X )X!	 Bà |$  	²~# BÀ }! $    78  70  )8)0B|AÀ Í 7( )(BÀ ²   )(7  )(Ü §6  B|B|A 6  )8)0!  ) 7  )7 B| Ö ! BÀ |$  À~# B }! $    7  7  )) 7  )(Aj­ö 7  ) ! ))! )(­!@ P\r    ü\n   )  )(­|A :   )   ) ø  ))0Ô ! B |$  ¥~# BÐ }! $    7H  7@  )@) 78  )8(Aj­ö 70 )0! )8)! )8(­!@ P\r    ü\n   )0 )8(­|A :    )H)0B|AÀ Í 7( )0 )(Ï  )0ø   )(7  )(Ü §6  B|B|A 6  )H)0!  ) 7  )7 B| Ö ! BÐ |$  ~~~~# Bà}! $    7Ø  7Ð  BÈ|7 A¨!A   µ B !  7À  7¸  )Ø)0Ò 7° )°! A6 B|B|A 6  B|B|!  /ÈAÿÿq­7 B|B 7   )Ø)07  A6¨ A : ¬ B|B%|!A !  :   ;    )¨7   ) 7  )7  )7  )7  )Ø)0!	 B 7x A6 Bø |B|A 6  )°!\n B¸|  )7  )x7 	 B¸| B| \n   )Ø)0Ò 7p )p! A6H BÈ |B|A 6  BÈ |B|!  /ÊAÿÿq­7P B|B 7   )Ø)07` A6h A : l BÈ |B%|!\rA ! \r :  \r ;    )h7   )`7  )X7  )P7  )H7  )Ø)0! B° 78 A6@ B8|B|A 6  )p! B¸|  )@7   )87  B¸| B|   )Ø)0!  )À70  )¸7( B(| Ú ! Bà|$  Å~~~# BÐ }! $    7H  7@@B -  Aq\r A B à A!B  :  B !  (Ì 68  )Ä 70  )¼ 7(  )´ 7   )¬ 7  )¤ 7  ) 7  ) 7   (Auq6A !   á  )H)0Ô ! BÐ |$  x~~# B}! $    7  7 @B -  AqE\r A !  B á  ))0Ô ! B|$  Ï~# BÐ }! $    7H  7@  )@) 78 )8B|!  )7  ) 7   ø 70 BÓ 7( )(! Að : & A : \' B&|!  )07   B|  )0ø  )H)0Ô ! BÐ |$  y~# B}! $    (Aj­ö 7 )!  ) !  (­!@ P\r    ü\n   )  (­|A :   )! B|$  «~# Bð }! $    7h  7`  )`) 7X  )`)7P )XB|!  )7  ) 7   ø 7H )PB|!  )7  ) 7  B|ø 7@ Bð 78 )8! Að : 5 Að : 6 A : 7 B5|! )H!  )@7(  7    B |  )Hø  )@ø  )h)0Ô ! Bð |$  «~# Bð }! $    7h  7`  )`) 7X  )`)7P )XB|!  )7  ) 7   ø 7H )PB|!  )7  ) 7  B|ø 7@ BÔ 78 )8! Að : 5 Að : 6 A : 7 B5|! )H!  )@7(  7    B |  )Hø  )@ø  )h)0Ô ! Bð |$  ñ	~# B}! $    7x  7p  )p) 7h )hB|!  )7  ) 7   ø 7` Bº 7P )P! Að : N A : O BÎ |!  )`7    B| 7X  )XÜ §6H  )x)0B| (HÍ 7@ )@! )X! (H­!@ P\r    ü\n   )`ø  )Xø   )@70  (H68 B0|B|A 6  )x)0!	  )87(  )07  B | 	Ö !\n B|$  \nñ	~# B}! $    7x  7p  )p) 7h )hB|!  )7  ) 7   ø 7` B£ 7P )P! Að : N A : O BÎ |!  )`7    B| 7X  )XÜ §6H  )x)0B| (HÍ 7@ )@! )X! (H­!@ P\r    ü\n   )`ø  )Xø   )@70  (H68 B0|B|A 6  )x)0!	  )87(  )07  B | 	Ö !\n B|$  \nâ\n\n~	~~~~# B}! $    6ü  7ð  7è  )è7àB !  7Ø  7Ð )ð!B !   |Ü >Ì   )ð|7¸  (Ì6ÀB!  B¸||!A !	  	6  )ð!\nBÀ !  \n |Ü >´   )ð|7   (´6¨  B || 	6  )à) )0! BÍ 7 A6  B|| 	6  )à) )0!\r  )À7x  )¸7p Bð | \rÖ !  )7h  )7`  BÐ| Bà |   )à) )0! B¯ 7 A6  B|| 	6  )à) )0!  )¨7X  ) 7P BÐ | Ö !  )7H  )7@  BÐ| BÀ |   )à) )0! B¿ 7ðA!  6ø  Bð|| 	6  )ð! )à) )0! - !A!  q Ù !  )ø78  )ð70  BÐ| B0|   )à) )0! Bµ 7à A	6è  Bà|| 	6  )ð! )à) )0!  - \rq Ù !  )è7(  )à7   BÐ| B |   )à) )0! B­ 7Ð A6Ø  BÐ|| 	6  )ð! )à) )0!  - q Ù !   )Ø7  )Ð7  BÐ| B|    )à) )0!! BÈ 7À  6È  BÀ|| 	6  )ð!" )à) )0!#  "- q #Ù !$  )È7  )À7  ! BÐ|  $  )à) )0!% Bû 7° A6¸  B°|| 	6  )ð- !& )à) )0!\' &Aq \'Ù !( BÐ|  )¸7  )°7 % BÐ| B| (  )à) )0!)  )Ø7  )Ð7  B| )Ú 7¨  )¨7  )à)  B | )à)B A Aqâ AAq!* B|$  *Ó	~# B°}! $    6¬  7   7  )7B !  7  7 )) )0! Bú 7p A6x Bð |B|A 6  ) (¬ )) )0× ! B|  )x7  )p7   B|    )) )0! BÏ 7` A6h Bà |B|A 6  ) (¬ )) )0× ! B|  )h7  )`7  B| B|   )) )0!	 B¾ 7P A6X BÐ |B|A 6  ) /Aÿÿq­ )) )0× !\n B|  )X7(  )P7  	 B| B | \n  )) )0!  )78  )70  B0| Ú 7H  )H7@ ))  BÀ | ))B A Aqâ AAq! B°|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ø 7 )  )ø  )()0Ô ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ø 7 )  )ø  )()0Ô ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ø 7 )  )ø  )()0Ô ! B0|$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÝ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÝ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÝ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7   ø 7  )8) B|AÍ 7 )8! ) 7  ) )! ) 7 )! )!A !BÞ !	B!\n   Aq 	 \n  )ø  )8)0Ô ! BÀ |$  Í~# B }!  6  7@@  ( (MAqE\r  A 6   )  (­|-  :   - Aÿq6@@ (Aq\r  )A6 @@ (AàqAÀFAqE\r  )A6   (Aq6@@ (AðqAàFAqE\r  )A6   (Aq6@@ (AøqAðFAqE\r  )A6   (Aq6 )A6  A 6@  ( (k )( IAqE\r  A 6 A6@@ ( )( IAqE\r   )  ( (j­|-  : @ - AÿqAÀqAGAqE\r  (! ) 6  A 6  (At6  - AÿqA?q (r6  (Aj6 @ )( AFAqE\r  (AIAqE\r  A 6@ )( AFAqE\r  (AIAqE\r  A 6@ )( AFAqE\r  (AIAqE\r  A 6@ (A°OAqE\r  (Aÿ¿MAqE\r  A 6@ (AÿÿÃ KAqE\r  A 6  (6 (º~~~~# BÐ }! $   7H  7@  78  70   )@) 7   A 6  B|A 6  B7( )0A 6  A 6$@@ ($ )H(IAqE\rA !  6   6 )H)  5$B|! )@!  )7  ) 7   B| B | B| : @ - AqE\r  ( )0( KAqE\r    ( 6 (!	 )0 	6   ($­7(  ($Aj6$ @ )(BRAqE\r   (!\n )@!  )  \n­|7   (! )@!\r \r \r( k6@ )8B RAqE\r  )(! )8 7  BÐ |$ ï~# Bà }! $    7P  7H  7@ A6< A 68 A 64@@ (8!  )7  ) 7  B|  B0| 6, A : + A 6$@@ ($ )P(IAqE\r  )P)  ($­B|7@@ )(  (<GAqE\r @ )(AGAqE\r @ (,E\r  (, )(IAq\r  (, )(KAqE\r@ )(AGAqE\r   (0 (8j68  (4Aj64 A: +  )(6<@ (<\r  (8! )H 6  (4! )@ 6  AAq: _  ($Aj6$ @@@ - +AqE\r  (,\r A Aq: _ - _Aq! Bà |$  \r BÔ  A   A  ¬ß §    ¬ß §     A     "   AFò   ( !  ø   A  Ï~@@  (A N\r A!   E!   !    )  !@ \r    @  -  Aq\r    À !  )p!@  )h"P\r   7p@ P\r   7h@ )   R\r   7 Á   )¨ø   ø   rù~# B}"$ B !@@@ Aj  A	F\r  B|Bx"B|7x ) !@@ AK\r @@A tAàq\r  AF\r A	G\r  Bø |70@  A B0| "AdG\r   7   A	 B | !@ E\r  ¬ß §!A  (|"k  (xAF!  7p    Bð | ¬ß §!  7  A B| ¬ß §!@ AF\r   B  AF7      ¬ß §!  7`@@  A Bà | "AdF\r  ¬! B 7P@  A BÐ | "AdF\r Bd! A H\r    7@  A  BÀ | ¬! ß §! B|$  ¯~@  B R\r @@B )  PE\r A !B )   !@B )° P\r B )°   r!@À ) " P\r @@@  (A N\r A!   E!@  )(  )8Q\r     r!@ \r      )p" B R\r Á  @@  (A N\r A!   E!@@@  )(  )8Q\r   B B   )H    )(B R\r A! E\r@  )"  )"Q\r     }A  )P    B 78  B 7   B 7(  B 7  B 7A ! \r    A!@  A+× B R\r   -  Aò G!  Ar  Aø × P" A r  Aå × P" AÀ r  -  "Aò F"Ar  A÷ F"Ar  Aá Fð~@ P\r    :     |"B| :   BT\r    :    :  B}| :   B~| :   BT\r    :  B|| :   B	T\r   B   }B"|" AÿqAl"6    }B|"|"B|| 6  B	T\r   6  6 Bx| 6  Bt| 6  BT\r   6  6  6  6 Bp| 6  Bl| 6  Bh| 6  Bd| 6   BB"}"B T\r  ­B~!  |!@  7  7  7  7  B |! B`|"BV\r      (x  ¶ ~~# B0}"$    )8"7  )(!  7(  7    }"7  |! B|!A!@@@@@  (x B|B B| ò E\r  !@  )"Q\r@ BU\r  ! BB   )"V"	|" )   B  	}"|7  BB 	|" )  }7   }! !  (x   	k"¬ B| ò E\r  BR\r    )X"78   7(     )`|7  !B !  B 78  B 7   B 7(    ( A r6  AF\r   )}! B0|$  ú~# B0}"$   7B !    )`"B R­}7  )X!  7(  7 A !@@@  (x B|B B| ò \r  )"B U\rAA  P!    (  r6  !  )"X\r     )X"7     }|7@  )`P\r    B|7  |B| -  :   ! B0|$     (x  ò §~# B }"$ B !@@B  ,  × B R\r  A6 Bð	ö "P\r  A Bè @ A+× B R\r  AA -  Aò F6 @@ -  Aá F\r  ( !@  AB  "Aq\r   Ar¬7  A B|   ( Ar"6  A6 B7`   6x  Bð|7X@ Aq\r   B|7   A¨  \r  A\n6 Bñ 7P Bò 7H Bó 7@ Bô 7@B - Ù \r  A6 Â ! B |$  ©~# B}"$ B !@@B  ,  × B R\r  A6   ! B¶7 A   Ar  ¬ß §"A H\r   ¡ "B R\r   B ! B|$  9~# B}"$   7    ì ! B|$  $~  Ü !AA    B  ± R  §@    ü\n    ~@ BT\r     ¥    |!@@   BB R\r @@  BPE\r   !@ PE\r   !  !@  -  :   B|! B|"BP\r  T\r  B|!@ BÀ T\r   B@|"V\r @  ( 6   (6  (6  (6  (6  (6  (6  (6  ( 6   ($6$  ((6(  (,6,  (060  (464  (868  (<6< BÀ |! BÀ |" X\r   Z\r@  ( 6  B|! B|" T\r @ BZ\r   !@ BZ\r   ! B||!  !@  -  :    - :   - :   - :  B|! B|" X\r @  Z\r @  -  :   B|! B|" R\r   ~    ("Aj r6@  )(  )8Q\r   B B   )H    B 78  B 7   B 7(@  ( "AqE\r    A r6 A    )X  )`|"7   7 AtAu~~@@ (A N\r A!  E!  ~!  ("Aj r6@@ )" )"R\r  !     }"   T"¦   ) |7  }!   |! @ P\r @@@ § \r      )@  "B R\r@ \r     }    |!   }"B R\r B   P! @ \r     ¾~@@ AI\r  A6 @ AG\r   )"P\r   }  )|!@  )(  )8Q\r   B B   )H    )(P\r  B 78  B 7   B 7(      )P  B S\r   B 7  B 7    ( Aoq6 A AI@  (AJ\r     ©    !    © !@ E\r          ª ~~  )P!A!@  -  AqE\r AA  )(  )8Q!@  B     "B S\r @@  )"B Q\r B!  )8"P\rB(!  }   |) |! C~@  (AJ\r   ¬    !  ¬ !@ E\r     \n   ­ g~    ("Aj r6@  ( "AqE\r    A r6 A  B 7  B 7    )X"78   7(     )`|7 A ê~B !@@ ) "B R\r  ¯ \r ) !@   )("}X\r      )H  @@ (A H\r  P\r  !@@   |"B|-  A\nF\r B|"P\r      )H  " T\r  }! )(!  !B !   ¦   )( |7(  |! k~  ~!@@ (AJ\r     ° !   !    ° !  E\r   @   R\r B   P   ½~~# "!B ! B B  P"}"$  !@@ \r B ! !  ! B R\r  A6 B !   ¬ß " B S\r @@  P\r  -  A/F\r A,6 @  Q\r  ! Û ! $  \n   ´    At  AvrAÿÿqT~# B}"$   B|Bx"B|7  ) 7      ¬ß ! B|$  §K~# B}"$     Aÿq B| ò ! )! B|$ B   A   ²@@@@  A H\r  A G\r  -  \r    ! @@  AF\r  -  !@ \r  AÿqA/F\r AG\r AÿqA/G\r AF\r \r   !       !    !   ¬ß § A   Aº . @  AJ\r Bxß §  Bè  A º »~# Bð }"$ @@   B|¼ A N\r B !B !@  AB  AqE\r  A6 @ (AàqAF\r  A66 BBü "P\r  A6   A     6 ! Bð |$  ~# B }"$ @@ AN\r A !@  Ü "B T\r  A%6 A!    B|¦ A B |·     B ¿ ! ( B ·  B |$  é~~~~# B}"$ @@  Ü "P\r    B|"|-  A/F\r ! B 7 A 6(@@@@@@@@@@@@ Aq"E\r    B(|» E\r !	   B(|Ô AJ\r  "	( "\nA,G\r   B(|» \rA!A !\nA !\n@ (,Aàq"AÀF\r @ AF\r A!AA Aq!A!\nAA ! ((!@ AqE\r  P\r   (G\r  6  7  )7  6  )7  7 B R\r 	( !\n \nAG\r  ((6  )7  7A!A !\n P\r (!  §Aj6$  Aj"6   6  (6 A ! A 6  A 6  §Aj6$@ P\r  !	@@   	|-  A/F\r @@@   	|B|-  A/G\r  	!B ! 	B|"	B R\r  §! 	B|"	PE\r   6 @ \nE\r   A B Ã !A  ( "\rAF  A H! \r   @ Aq"\r    B(|     "\n\r@ P\r  )!	 ((!\n@@ ( \nG\r  ) 	Q\r ) "B R\r @ E\r  AqAG\r A!\n@ AJ\r   \r6  ½ "	P\r@ 	Ì "P\r  Aj!B  }!   |"B|!@@@ - A.G\r  - "\nE\r \nA.G\r  - E\r@ B|"Ü  T\r  A%6  	  A/:    Ú       B|¿ "\nE\r  	  	Ì "PE\r  	    |A :   E\r    B(|     "\n\rA !\n  A!\n B|$  \n BÀ ¸ BÈ  BÀ ¹ 4~  À ") "7p@ P\r    7h   7 Á   z~# B}"$ @@ AÀ q\r B ! AqAG\r  B|7 5 !  7 A   Ar  ¬ß ! B|$  §P~B !@  A$B Ã "A H\r @BBü "B R\r   B   6 !     §  ¬ß §B~# B}"$   7B¸    ì ! B|$   A* BØ   AN Ç \r BÐ B B 7ø É ! B B B }7° B B 7¨ B   6 B B 5Ä 7¸ ~@@  ("  (H\r A !@  (  B|B¡ "A J\r B !  ATF\r E\r A  k6 B    6      ¬|"B(|/ j6   B |) 7  B|!   ~@   Q\r @    |"}B  B}V\r     ¦    B!@@@   Z\r @ B Q\r   !@  BB R\r   !  !@ P\r  -  :   B|! B|! B|"BP\r @ B R\r @ BB Q\r @ P\r   B|"|"  |-  :   BPE\r  BX\r @   Bx|"|  |) 7  BV\r  P\r@   B|"|  |-  :   B R\r  BX\r @  ) 7  B|! B|! Bx|"BV\r  P\r @  -  :   B|! B|! B|"PE\r   b~# B}"$ A   B|  P" B BV¢ "Au q   B|Q¬ß ! B|$  »	~~~# BÀ }"$ B !@@  B R\r  A6 @@  B Þ "B R\r  A,6 @ BÿV\r  B |B  }"|   B|¦ B ! B !B !A !@@@ B | |"-  A/G\r B!  B | B|"|-  ! A/:  A !B ! A/G\r B|-  A/F\r A/: B! @@@@ A/Ø  }"	B R"\n\r  E\r@ 	BR\r  -  A.G\r  B|!  P"\r   |B|-  A/F\r P\r B | B|"|A/:   	B|!   |A :  @ -  A/F\r  B |B ² P\rB !	 B |Ü !@ P\r @B !@ BT\r @B! B | |B|-  A/F\r B|"BV\r B!B ! 	B| 	B|"	 	  T!	  |! B|"B R\r    	}!@   	Q\r  B | |" B|-  A/F\r   A/:   B|!  |" B`|B`T\r  |  	| B|Í   B | ¦ @ P\r     B|¦ ! Û ! 	!   |"BÿV\r   | B | | ¦   |A :    |!A!@@@@@@ 	BR\r  B | |"B~|-  A.G\r  B|-  A.G\r @   B~V\r  B|! ! A ! E\r  B | Î " Q\r@ B R\r  A,6 	 BU\r ( AG\r \r \r@@   |B|-  A/F\r  B|" P\r A !  BR\rB!     \n!  B | |,  !@ B|"B(R\r  A 6  !	@ B | |B|-  A/G\r @ "	B|! 	 B ||-  A/F\r  B | 	 }"| B | Í   B|" B -  AÿqA/G  BR! A !B !  B | |Ð  |!  A%6 B ! BÀ |$  #~  !@ "B|! -  A/F\r    }/@A  A £ "AaG\r   ¤ ! ¬ß §¯~|@  ½"B4§Aÿq"A²K\r @ AýK\r   D        ¢@@  " D      0C D      0Ã   ¡"D      à?dE\r     D      ð¿ !     !  D      à¿eE\r   D      ð? !      B S!   ;~# B}"$   7     ð ! B|$   A   A º  A  B     Ø " B   -   AÿqF·~@@@@ Aÿq"E\r @  BP\r  Aÿq!@  -  "E\r  F\r  B|" BB R\r B À  ) "} B ÀB ÀR\r ­B À~!@B À  "} B ÀB ÀR\r  )!  B|"!  B À }B ÀB ÀQ\r     Ü |  ! Aÿq!@ " -  "E\r  B|!  G\r   ~@@@   BP\r  -  !@ BB Q\r @   -  ":   E\r  B|!  B|"BPE\r @B À ) "} B ÀB ÀR\r @   7   B|!  "B|!B À )"} B ÀB ÀQ\r  §!   :   AÿqE\r @   - ":   B|!  B|! \r       Ù   /~@  Ü B|"ö "PE\r B     ¦ ~  !@@  BP\r @  -  \r     }  !@ B|"BB Q\r -  \r @ "B|!B À ) "} B ÀB ÀQ\r @ "B|! -  \r    }~~B ! B R!@@@  BP\r  P\r  Aÿq!@  -   F\r B|"B R!  B|" BP\r B R\r  E\r@  -   AÿqF\r  BT\r  Aÿq­B À~!@B À  )  "} B ÀB ÀR\r  B|!  Bx|"BV\r  P\r Aÿq!@@  -   G\r     B|! B ! B|"B R\r  ~   A  Ý "  } P" @  B`T\r  A   §k6 B!   >~# B}"$   7   A¨ µ !  B|$ AA   Z~# B}"$ @@ AI\r  A6 A!  7    A¨j µ ! B|$  ~@  ½"B4§Aÿq"AÿF\r @ \r @@  D        b\r A !  D      ðC¢ â !  ( A@j!  6     Axj6  BÿÿÿÿÿÿÿBð?¿!   «~# Bà}"$   7Ø B |A B(ü   )Ø7Ð@@B   BÐ| BÐ | B |  ä A N\r A!@@  (A N\r A!   E!    ( "A_q6 B !@@@@  )`B R\r   BÐ 7`  B 78  B 7   B 7(  )X!   7X  ) B R\rA!  ¯ \r    BÐ| BÐ | B |  ä ! A q!@ P\r   B B   )H    B 7`   7X  B 78  B 7   )(!  B 7(A  P!    ( "	 r6 A  	A q! \r     Bà|$  ~~	~# BÀ }"$   78 B\'|! B(|!	A !\nA !@@@@@A !@ !\r  AÿÿÿÿsJ\r  j! \r!@@@@@@@ \r-  "E\r @@@@ Aÿq"\r  ! A%G\r !@@ - A%F\r  ! B|! - ! B|"! A%F\r   \r}" Aÿÿÿÿs"­U\r §!@  P"\r    \r Äå  \r	  78 B|!A!@ , APj"A	K\r  - A$G\r  B|!A!\n !  78A !@@ ,  "A`j"AM\r  !A ! !A t"AÑqE\r @  B|"78  r! , "A`j"A O\r !A t"AÑq\r @@ A*G\r @@ , APj"A	K\r  - A$G\r  ­!@@  B R\r   B|A\n6 A !  B|( ! B|!A!\n \n\r B|!@  B R\r   78A !\nA !  ) "B|7  ( !A !\n  78 AJ\rA  k! AÀ r! B8|æ "A H\r )8!A !A!@@ -  A.F\r A !@ - A*G\r @@ , APj"A	K\r  - A$G\r  ­!@@  B R\r   B|A\n6 A !  B|( ! B|! \n\r B|!@ E\r A !  ) "B|7  ( !  78 AJ!  B|78A! B8|æ ! )8!@ !A! ",  "AjAFI\r\r B|! ­B:~ ¬|Bï |-  "AjAÿqAI\r   78@@ AF\r  E\r@ A H\r  ­!@  B R\r   B| 6    B|) 70 \r\n B0|   ç  AJ\r\rA ! \r\n  -  A q\r\r Aÿÿ{q"  AÀ q!A !Bß ! 	!@@@@@@@@@@@@@@@@@ -  "À"ASq  AqAF  "A¨j!	\n  	!@ A¿j  AÓ F\rA !Bß ! )0!A !@@@@@@@   )0 6  )0 ¬7  )0 ¬7  )0 ;  )0 :   )0 ¬7  )0 ¬7  A AK! Ar!Aø !A !Bß ! )0" 	 A qè !\r P\r AqE\r Av­Bß |!A!A !Bß ! )0" 	é !\r AqE\r 	 \r}" ¬S\r §Aj!@ )0"BU\r  B  }"70A!Bß !@ AqE\r A!Bà !Bá Bß  Aq"!  	ê !\r  A Hq\r Aÿÿ{q  !@ B R\r  \r A ! 	! 	!\r 	 \r} P­|" ¬"  U§! - 0!B²  )0" P!\r \r \r Aÿÿÿÿ AÿÿÿÿI­Þ "|!@ AJ\r  -  \r §! )0"PE\rA !	@ E\r  ¬!\r )0!A !  A  A  ë  A 6  >  B|70 B|!B!\rB !@@ ( "E\r B| ô "A H\r \r } ­"T\r B|!  |" \rT\r A=! BÿÿÿÿV\r  A   §" ë @ PE\r A !B !\r )0!@ ( "E\r \r B| ô ¬"|"\r V\r   B| å  B|! \r T\r   A    AÀ së     J!\n  A Hq\rA=!   +0       "A N\r	 - ! B|!   B R\r \nE\rB!@@  B|( "E\r  B|   ç  B|"B\nR\r A!A! B\nZ\r@  B|( \r B|"B\nQ\r A!  : \'A! 	! !\r ! 	! Aÿÿÿÿs  \r}" ¬"  U§"H\rA=!   j"  J" K\r  A    ë     ­å   A0   Asë   A0  §A ë    \r å   A    AÀ së  )8!A !A=!  6 A! BÀ |$   @  -  A q\r     ° ~~A !@  ) ",  APj"A	M\r A @A!@ AÌ³æ K\r A  A\nl"j  AÿÿÿÿsK!   B|"7  , ! ! ! APj"A\nI\r  â @@@@@@@@@@@@@@@@@@@ Awj 	\n\r  ) B|Bx"B|7    ) 7   ) "B|7    4 7   ) "B|7    5 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) "B|7    2 7   ) "B|7    3 7   ) "B|7    0  7   ) "B|7    1  7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    + 9       9 @  P\r @ B|"  B-   r:    B" B R\r  . @  P\r @ B|"  §AqA0r:    B" B R\r  =~@  P\r @ B|"    B\n"B\n~}§A0r:    B	V! !  \r  ~# B}"$ @  L\r  AÀq\r     k"A AI"­ @ \r @   Bå  A~j"AÿK\r     ­å  B|$ $     B÷ Bø ã ~~~~~|~# B°}"$ A ! A 6,@@ ï "BU\r A!	Bé !\n "ï !@ AqE\r A!	Bì !\nBï Bê  Aq"	!\n 	E!@@ Bøÿ Bøÿ R\r   A   	Aj" Aÿÿ{që    \n 	­å   B BÌ  A q"BÛ BÐ    bBå   A    AÀ së     J! B|!\r@@@@  B,|â "  "D        a\r   (,"Aj6, A r"Aá G\r A r"Aá F\rA  A H! (,!  Acj"6,A  A H! D      °A¢! B0|B B  A H|"!@  ü"6  B|!  ¸¡D    eÍÍA¢"D        b\r @@ AN\r  ! ! !@ A AI!@ B||" T\r  ­!B !@  5   |" BëÜ"BëÜ~}>  B||" Z\r  BëÜT\r  B||" > @@ " X\r B||"( E\r   (, k"6, ! A J\r @ AJ\r  AjA	nAj­! Aæ F!@A  k"A	 A	I!@@  T\r B B ( !AëÜ v!A tAs!A ! !@  ( " v j6   q l! B|" T\r B B ( ! E\r   6  B|!  (, j"6,   |" " B|   }B U! A H\r A !@  Z\r   }B§A	l!A\n! ( "A\nI\r @ Aj!  A\nl"O\r @  }BB	~Bw| A   Aæ Fk A G Aç Fqk"¬W\r   AÈ j"A	m"¬B|"B`|!A\n!@  A	lk"AJ\r @ A\nl! Aj"AG\r  B`|!@@ ( "  n" lk"\r   Q\r@@ Aq\r D      @C! AëÜG\r  X\r B`|-  AqE\rD     @C!D      à?D      ð?D      ø?  QD      ø?  Av"F  I!@ \r  \n-  A-G\r  ! !   k"6     a\r    j"6 @ AëÜI\r @ A 6 @ B||" Z\r  B||"A 6   ( Aj"6  AÿëÜK\r   }B§A	l!A\n! ( "A\nI\r @ Aj!  A\nl"O\r  B|"   V!@@ " X"\r B||"( E\r @@ Aç F\r  Aq! AsA A " J A{Jq" j!AA~  j! Aq"\r B	!@ \r B	! B||( "E\r A\n!B ! A\np\r A !@ Aj!  A\nl"pE\r  ­!  }BB	~! ¬!@@ A_qAÆ G\r   }Bw|"B  B U"   S§! ¬ | }Bw|"B  B U"   S§!A !A! AýÿÿÿAþÿÿÿ  r"J\r  A GjAj!@@ A_q"AÆ G\r   AÿÿÿÿsJ\r A  A J!@ \r  Au"s k­ \rê "}BU\r @ B|"A0:   \r }BS\r  B~|" :   B|A-A+ A H:   \r }" Aÿÿÿÿs­U\r §!  j" 	AÿÿÿÿsJ\r  A    	j" ë    \n 	­å   A0   Asë @@@@ AÆ G\r  B|B	!    V"!@ 5  ê !@@  Q\r   B|X\r@ B|"A0:    B|V\r   R\r  B|"A0:       }å  B|" X\r @ E\r   B° Bå   Z\r AH\r@@ 5  ê " B|X\r @ B|"A0:    B|V\r     A	 A	H­å  Awj! B|" Z\r A	J! ! \r @ A H\r   B|  V! B|B	! !@@ 5  ê " R\r  B|"A0:  @@  Q\r   B|X\r@ B|"A0:    B|V\r    Bå  B|!  rE\r   B° Bå      }" ­"  Så   §k! B|" Z\r AJ\r   A0 AjAA ë     \r }å  !  A0 A	jA	A ë   A    AÀ së     J! \nB	B  A q"|!@ AK\r A k!D      0@!@ D      0@¢! Aj"\r @ -  A-G\r    ¡ !    ¡!@ (," Au"s k­ \rê " \rR\r  B|"A0:   (,! 	Ar! B~|" Aj:   B|A-A+ A H:   AH AqEq! B|!@ " ü"¬B |-   r:    ·¡D      0@¢!@ B|" B|}BR\r  D        a q\r  A.:  B|! D        b\r A!Býÿÿÿ \r }" ­"|} ¬"S\r   A    §jAj  B|} |§"  B|}"B~| S  " j" ë     å   A0   Asë    B| å   A0   |§kA A ë     å   A    AÀ së     J! B°|$  .~  ) B|Bx"B|7    )  ) 9    ½¥~# B}"$   Bþ|   P" 7è B  B|"  V7ð A Bèü  A6 Bù 7H A6  Bÿ|7X  Bè|7  A :     ì ! B|$  ·~  )") !@ )"  )(  )8"}"  T"P\r    ¦   )  |"7   ) }"7@    T"P\r    ¦   )  |"7   ) }7 A :      )X"78   7(  @  \r A    6 A°~B!@@  P\r  Aÿ M\r@@Ê )¨) B R\r  AqA¿F\r A6 @ AÿK\r    A?qAr:    AvAÀr:  B@@ A°I\r  A@qAÀG\r   A?qAr:    AvAàr:     AvA?qAr: B@ A|jAÿÿ?K\r    A?qAr:    AvAðr:     AvA?qAr:    AvA?qAr: B A6 B!    :  B @  PE\r A    B ó §	 ¥  .~~~# B}"$ @@@@@  BðV\r @B (È§ "B   B|Bø  BT"B§"v"AqE\r @@ AsAq j"At­B" B¨ |"  ) ¨ ")" R\r B  A~ wq6È§   B )à§ T\r  ) R\r   7   7 B|!   At­"B7  |" )B7 B )Ð§ "X\r@ E\r @@  tA t"A  krqh"At­B" B¨ |"  ) ¨ ")" R\r B  A~ wq"6È§   B )à§ T\r  ) R\r   7   7 B|! @ At­" }"	BV\r   B7  |" )B7  B7  |"\n 	B7  | 	7 @ P\r  BBðÿÿÿÿ B¨ |!B )è§ !@@ A B§t"q\r B   r6È§  ! )"B )à§ T\r  7  7  7  7B  \n7è§ B  	7Ð§ B (Ì§ "E\r h­B) ¬ ")Bx }! !	@@@ ) " B R\r  )(" P\r  )Bx }"   T"!   	 !	  !  	B )à§ "T\r 	)0!@@ 	)"  	Q\r  	)" T\r ) 	R\r  ) 	R\r   7   7@@ 	)("B Q\r  	B(|!\n@ 	) "PE\r B !  	B |!\n@ \n!\r " B(|!\n  )("B R\r   B |!\n  ) "B R\r  \r T\r \rB 7 @ P\r @@ 	 	(8"­B") ¬ R\r  B ¬ |  7   B R\rB  A~ wq6Ì§   T\r@@ )  	R\r    7    7(  P\r   T\r   70@ 	) "P\r   T\r   7    70 	)("P\r   T\r   7(   70@@ BV\r  	  |" B7 	  |"   )B7 	 B7 	 |" B7  | 7 @ P\r  BBðÿÿÿÿ B¨ |!B )è§ ! @@A B§t" q\r B   r6È§  !\n )"\n T\r   7 \n  7   7   \n7B  7è§ B  7Ð§  	B|! B!  Bÿ~V\r   B|" Bx!B (Ì§ "E\r @@  B§"\r A !@ AÿÿM\r A! A& g"k­§Aq AtrA>s!B  }!@@@@ ­B) ¬ "PE\r B ! B !	 B B? AvAj­} AF!\nB ! B !	@@ )Bx }"\r Z\r  \r! !	 \rPE\r B ! !	 !      )("\r \r  \nB<B|) "Q \rP!  \nB!\n ! B R\r @   	B R\r A t"A  kr q"E\r h­B) ¬ ! B !	  P\r@  )Bx }"\n T!@  ) "B R\r   )(! \n  !   	 !	 !  B R\r  	P\r  B )Ð§  }Z\r  	B )à§ "T\r 	)0!@@ 	)"  	Q\r  	)" T\r ) 	R\r  ) 	R\r   7   7@@ 	)("B Q\r  	B(|!\n@ 	) "PE\r B !  	B |!\n@ \n!\r " B(|!\n  )("B R\r   B |!\n  ) "B R\r  \r T\r \rB 7 @ P\r @@ 	 	(8"­B") ¬ R\r  B ¬ |  7   B R\rB  A~ wq"6Ì§   T\r@@ )  	R\r    7    7(  P\r   T\r   70@ 	) "P\r   T\r   7    70 	)("P\r   T\r   7(   70@@ BV\r  	  |" B7 	  |"   )B7 	 B7 	 |"\n B7 \n | 7 @ BÿV\r  B"BB¨ |! @@B (È§ "A §t"q\r B   r6È§   !  )" T\r   \n7  \n7 \n  7 \n 7@@ B§"\r A !@ AÿÿM\r A! A& g"k­§Aq AtrA>s! \nB 7( \n 68 \nB 7  ­BB ¬ |!@@@ A t"q\r B   r6Ì§   \n7  \n 70 B B? AvAj­} AF!  ) !@ ")Bx Q\r  B<!  B!   B|"\r) "B R\r  \rB |"  T\r   \n7  \n 70 \n \n7 \n \n7  T\r )"  T\r   \n7  \n7 \nB 70 \n 7 \n  7 	B|! @B )Ð§ "  T\r B )è§ !@@   }"B T\r   |"	 B7   | 7   B7   B7   |"   )B7B !B !	B  7Ð§ B  	7è§  B|! @B )Ø§ "	 X\r B  	 }"7Ø§ B B )ð§ "  |"7ð§   B7   B7  B|! @@B )ð® P\r B )¯ !B !B B 7¯ B A 6¯ B B7¯ B B7¯ B B 7ø® B A 6¸® B  B|BpBØªÕª7ð® B !   BÏ |"|"\rB  }""\n X\rB ! @B )°® "P\r B ) ® " \n|" X\r  V\r@@@B - ¸® Aq\r @@@@@B )ð§ "P\r BÀ® ! @@   ) "T\r     )|T\r  )" B R\r B þ "	BQ\r \n!\r@B )ø® " B|" 	P\r  \n 	}  	|B   }|!\r \r X\r@B )°® " P\r B ) ® " \r|" X\r   V\r \rþ "  	R\r \r 	} "\rþ "	  )   )|Q\r 	!   BQ\r@ \r BÐ |T\r   !	  \r}B )¯ "|B  }"þ BQ\r  \r|!\r  !	 	BR\rB B (¸® Ar6¸®  \nþ !	B þ !  	BQ\r  BQ\r 	  Z\r   	}"\r BÈ |X\rB B ) ®  \r|" 7 ® @  B )¨® X\r B   7¨® @@@@B )ð§ "B Q\r BÀ® ! @ 	  ) "  )"\n|Q\r  )" PE\r @@B )à§ " P\r  	  Z\rB  	7à§ B ! B A 6Ø® B  \r7È® B  	7À® B B7¨ B B )ð® 7¨ @  B" B¨ |"7 ¨   7¨¨   B|" B R\r B  \rB¸|" Bp 	}B"}"7Ø§ B  	 |"7ð§   B7 	  |BÈ 7B B )¯ 7ø§   	Z\r   T\r   (Aq\r    \n \r|7B  Bp }B" |"7ð§ B B )Ø§  \r|"	  }" 7Ø§    B7  	|BÈ 7B B )¯ 7ø§ @ 	B )à§ Z\r B  	7à§  	 \r|!BÀ® ! @@@  ) "\n Q\r  )" PE\r   - AqE\rBÀ® ! @@@   ) "T\r     )|"T\r  )!  B  \rB¸|" Bp 	}B"\n}"7Ø§ B  	 \n|"\n7ð§  \n B7 	  |BÈ 7B B )¯ 7ø§   B? }B|B±|"    B |T"\nB+7 \nB )Ø® 7( \nB )Ð® 7  \nB )È® 7 \nB )À® 7B  \r7È® B  \nB|7Ð® B A 6Ø® B  	7À®  \nB(|! @  B7  B|!	  B|!  	 T\r  \n Q\r  \n \n)B~7  \n }"\rB7 \n \r7 @@ \rBÿV\r  \rB"BB¨ |! @@B (È§ "A §t"q\r B   r6È§   !  )"B )à§ T\r   7  7B!	B!\n@@ \rB§"\r A !@ AÿÿM\r A! \rA& g"k­§Aq AtrA>s! B 7(  68 B 7  ­BB ¬ |!@@@B (Ì§ "A t"q\r B   r6Ì§   7   70 \rB B? AvAj­} AF!  ) !	@ 	")Bx \rQ\r  B<!	  B!   	B|"\n) "	B R\r  \nB |" B )à§ T\r   7   70B!	B!\n ! !  B )à§ "	T\r )"  	T\r   7  7   7B ! B0!	B!\n  \n| 7   	|  7 B )Ø§ "  X\r B    }"7Ø§ B B )ð§ "  |"7ð§   B7   B7  B|!  A06 B ! õ     	7     ) \r|7 	 \n ÷ !  B|$   Ý~  Bp  }B|" B7 Bp }B|"  |"}!@@@ B )ð§ R\r B  7ð§ B B )Ø§  |"7Ø§   B7@ B )è§ R\r B  7è§ B B )Ð§  |"7Ð§   B7  | 7 @ )"BBR\r @@ BÿV\r  )!@ )"  B"BB¨ |"Q\r   B )à§ T\r  ) R\r@   R\r B B (È§ A~ §wq6È§ @  Q\r  B )à§ T\r ) R\r   7   7 )0!	@@ )" Q\r  )" B )à§ T\r  ) R\r ) R\r   7   7@@ )(" B Q\r  B(|!@ ) " PE\r B ! B |!@ !  "B(|! )(" B R\r  B |! ) " B R\r  B )à§ T\r B 7  	P\r @@  (8"\n­B" ) ¬ R\r   B ¬ | 7  B R\rB B (Ì§ A~ \nwq6Ì§  	B )à§ T\r@@ 	)  R\r  	 7  	 7( P\r B )à§ "T\r  	70@ ) " P\r    T\r   7    70 )(" P\r    T\r   7(   70 Bx" |!  |")!  B~7  B7  | 7 @ BÿV\r  B" BB¨ |!@@B (È§ "\nA  §t"q\r B  \n r6È§  !  )" B )à§ T\r  7   7  7   7@@ B§"\n\r A !\n@ \nAÿÿM\r A!\n A& \ng"\nk­§Aq \nAtrA>s!\n B 7(  \n68 B 7  \n­BB ¬ |! @@@B (Ì§ "A \nt"q\r B   r6Ì§    7    70 B B? \nAvAj­} \nAF!  ) !@ " )Bx Q\r B<! B!   B|") "B R\r  B |"B )à§ T\r  7    70  7  7  B )à§ "T\r  )" T\r  7   7 B 70   7  7 B|õ  ×~~@@  P\r   Bp|"B )à§ "T\r  Bx|) "BBQ\r  Bx" |!@ §Aq\r  BP\r  ) "}" T\r   |! @ B )è§ Q\r @ BÿV\r  )!@ )" B"BB¨ |"Q\r   T\r ) R\r@  R\r B B (È§ A~ §wq6È§ @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r@@  (8"	­B") ¬ R\r  B ¬ | 7  B R\rB B (Ì§ A~ 	wq6Ì§   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r  T\r  7(  70 )"BBR\r B   7Ð§   B~7   B7   7   Z\r )"BP\r@@ BB R\r @ B )ð§ R\r B  7ð§ B B )Ø§   |" 7Ø§    B7 B )è§ R\rB B 7Ð§ B B 7è§ @ B )è§ "\nR\r B  7è§ B B )Ð§   |" 7Ð§    B7   |  7 @@ BÿV\r  )!@ )" B"BB¨ |"Q\r   T\r ) R\r@  R\r B B (È§ A~ §wq6È§ @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r @@  (8"	­B") ¬ R\r  B ¬ | 7  B R\rB B (Ì§ A~ 	wq6Ì§   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70  Bx  |" B7   |  7   \nR\rB   7Ð§   B~7   B7   |  7 @  BÿV\r   B"BB¨ |! @@B (È§ "	A §t"q\r B  	 r6È§   !  )" T\r   7  7   7  7@@  B§"	\r A !	@ 	AÿÿM\r A!	  A& 	g"	k­§Aq 	AtrA>s!	 B 7(  	68 B 7  	­BB ¬ |!@@@@B (Ì§ "A 	t"\rq\r B   \rr6Ì§   7 B! B0!  B B? 	AvAj­} 	AF! ) !@ ")Bx  Q\r B<! B!  B|") "B R\r  B |"  T\r   7 B! B0! ! ! !  T\r )" T\r  7  7B !B0! B!  | 7   7   | 7 B BB )¨ B|" P7¨ õ  ¥~@  B R\r  ö @ BT\r  A06 B @  Bp|B  B|Bx BTú "P\r  B|@ ö "PE\r B    BpBx  Bx|) "BP Bx|"   T¦   ø  \n	~@@  B )à§ "T\r   )"B"BQ\r  Bx"P\r    |")"BP\r B !@ B R\r  BT\r@  B|T\r   !  }B )¯ BX\rB !@  T\r @  }"B T\r     BB7   |" B7  )B7  û   B !@ B )ð§ R\r B )Ø§  |" X\r    BB7   |"  }"B7B  7Ø§ B  7ð§   @ B )è§ R\r B !B )Ð§  |" T\r@@  }"B T\r     BB7   |" B7   |" 7   )B~7   B B7   |" )B7B !B !B  7è§ B  7Ð§   B ! BB R\r Bx |"	 T\r@@ BÿV\r  )!@ )" B"BB¨ |"Q\r   T\r ) R\r@  R\r B B (È§ A~ §wq6È§ @  Q\r   T\r ) R\r  7  7 )0!\n@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  \nP\r @@  (8"­B") ¬ R\r  B ¬ | 7  B R\rB B (Ì§ A~ wq6Ì§  \n T\r@@ \n)  R\r  \n 7  \n 7( P\r  T\r  \n70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70@ 	 }"BV\r    B 	B7   	|" )B7      BB7   |" B7   	|" )B7  û   õ   ~~   |!@@@@  )"BP\r B )à§ ! BP\r    ) "}" B )à§ "T\r  |!@  B )è§ Q\r @ BÿV\r   )!@  )" B"BB¨ |"Q\r   T\r )  R\r@  R\r B B (È§ A~ §wq6È§ @  Q\r   T\r )  R\r  7  7  )0!@@  )"  Q\r   )" T\r )  R\r )  R\r  7  7@@  )("B Q\r   B(|!@  ) "PE\r B !  B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r@@    (8"	­B") ¬ R\r  B ¬ | 7  B R\rB B (Ì§ A~ 	wq6Ì§   T\r@@ )   R\r   7   7( P\r  T\r  70@  ) "P\r   T\r  7   70  )("P\r  T\r  7(  70 )"BBR\r B  7Ð§   B~7   B7  7   T\r@@ )"BB R\r @ B )ð§ R\r B   7ð§ B B )Ø§  |"7Ø§    B7  B )è§ R\rB B 7Ð§ B B 7è§ @ B )è§ "\nR\r B   7è§ B B )Ð§  |"7Ð§    B7   | 7 @@ BÿV\r  )!@ )" B"BB¨ |"Q\r   T\r ) R\r@  R\r B B (È§ A~ §wq6È§ @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r @@  (8"	­B") ¬ R\r  B ¬ | 7  B R\rB B (Ì§ A~ 	wq6Ì§   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70   Bx |"B7   | 7    \nR\rB  7Ð§   B~7   B7   | 7 @ BÿV\r  B"BB¨ |!@@B (È§ "	A §t"q\r B  	 r6È§  ! )" T\r   7   7   7   7@@ B§"	\r A !	@ 	AÿÿM\r A!	 A& 	g"	k­§Aq 	AtrA>s!	  B 7(   	68  B 7  	­BB ¬ |!@@@B (Ì§ "A 	t"\rq\r B   \rr6Ì§    7    70 B B? 	AvAj­} 	AF! ) !@ ")Bx Q\r B<! B!  B|") "B R\r  B |" T\r   7    70    7    7  T\r )" T\r   7   7  B 70   7   7õ  ~# B}"$ @@  PE\r B !   B  B ÿ  ) !   BT\r B  )B R!@ ö " P\r   Bx|-  AqE\r   A    B|$    ? B~~@@  B S\r   B|Bx! B B   }Bøÿÿÿÿÿÿÿÿ }! @B )¨ "  |" ý X\r   ¦ \r  A06 BB   7¨  u~    ~  ~| B " B "~| Bÿÿÿÿ" Bÿÿÿÿ"~"B   ~|"B | Bÿÿÿÿ  ~|"B |7   B  Bÿÿÿÿ7 * B $ B B|Bp$  # # } #  # S~@@ AÀ qE\r   A@j­!B ! E\r  AÀ  k­  ­"!  !   7    7S~@@ AÀ qE\r   A@j­!B ! E\r  AÀ  k­  ­"!  !   7    7§~# B }"$  Bÿÿÿÿÿÿ?!@@ B0Bÿÿ"§"AÿjAýK\r   B< B! Aj­!@@  Bÿÿÿÿÿÿÿÿ" BT\r  B|!  BR\r  B |!B   BÿÿÿÿÿÿÿV"!  ­ |!@   P\r  BÿÿR\r   B< BB! Bÿ!@ AþM\r Bÿ!B ! @Aø Aø  P"" k"Að L\r B ! B !  BÀ  !A !@  F\r  B|   A k  ) )B R!       ) "B< )B! @@ Bÿÿÿÿÿÿÿÿ ­"BT\r   B|!  BR\r   B  |!   B    BÿÿÿÿÿÿÿV"!  ­! B |$  B4 B  ¿     A A A § ¬ß §     ­A A A ¨ ¬ß §     ­A A A © ¬ß §   )ø   ø     A A A A ª ¬ß §      B B          « ¬ß       B A          ­¬ ¬ß        ­A È ¬ß §Ã~~# B }"$ @@    A A A ­ "AdF\r  A¾G\r A qE\r    Aÿï_q A A A ­ "A H\r @ A qE\r  B7 A B|  AqE\r  B7  A   ¬ß ! B |$  §\n   $ ~#   }Bp"$   # \\~B !@  AK\r   ­B/ !@  E\r  AÿÿqE\r ­BÿÿBÄ |! ~    \n       At  AvrAÿÿq\n       AÿüqAx  AxAÿüqr§ B°-list-directory is-directory delete-directory alt-key shift-key ctrl-key meta-key get-index max -+   0X0x -0X+0X 0X-0x+0x 0x pow is-env make-env div get-text update-text is-list last sqrt sort import str-insert alert warning: unsupported syscall: __syscall_setsockopt not is-int to-int environment comment create-client exit is-unit split gt set ret let is-dict is-float to-float repeat rows on-key-press eval-macros compiled-macros cols get-args abs eat-str byte-8-to-str byte-16-to-str byte-64-to-str byte-32-to-str sub-str console-error Unknown error create-server on-mouse-enter filter identifier aether eq on-key-up on-mouse-up zip map macro get-file-info do on-key-down on-mouse-down console-warn button accept-connection close-connection term/raw-mode-on join min len nan current-platform atom mul is-bool to-bool get-html update-html tail eval string literal on-click on-double-click set-current-path get-current-path get-absolute-path match for-each console-log is-string printf inf elif term/raw-mode-off %f term/get-size receive-size str-remove on-mouse-move receive on-mouse-leave true value use else false type new line compile while write-file delete-file read-file get-range gen-range code whitespace str-replace mod round send and fold %ld eval-compiled add head is-func sub web rwa `}` `{` `]` `[` `<>` `->` `<->` `:` `::` `...` `)` `(`  [ NAN INF <lambda> eat-byte-8 eat-byte-16 eat-byte-64 eat-byte-32 /usr/include/aether/ ae-src/ ] -> ... (null)  or  %.*s:%u:%u: [ERROR] Expected  %.*s: [ERROR] Expected  src/std/str.c:%d:  src/lib/deserializer.c:%d:  src/lib/serializer.c:%d:  src/lib/parser.c:%d:  src/lib/vm.c:%d:  src/std/core.c:%d:  src/lib/misc.c:%d:  ,     {\n %.*s:%u:%u: [ERROR] set: only integer can be used as an array index\n [INFO] Trace: %.*s:%.*s:%u\n %.*s:%u:%u: [ERROR] Wrong arguments count: %u, expected %u\n [ERROR] Unknown type: %u\n [ERROR] Unknown value kind: %u\n %.*s:%u:%u: [ERROR] get: lists can only be indexed with integers\n [ERROR] Corrupted bytecode: expected %u, but got %u bytes\n %.*s:%u:%u: [ERROR] set: index out of bounds\n [ERROR] join: wrong part kinds\n %.*s:%u:%u: [ERROR] set: destination should be list or dictionary, but got %.*s\n %.*s:%u:%u: [ERROR] get: source should be list, string or dictionary, but got %.*s\n [ERROR] filter: predicate should return bool\n [ERROR] make-env: every program argument should be of type string\n %.*s:%u:%u: [ERROR] Could not import `%.*s` module\n %.*s:%u:%u: [ERROR] Value of kind %.*s is not callable\n %.*s:%u:%u: [ERROR] Symbol %.*s was not defined before usage\n %.*s:%u:%u: [ERROR] File offset for %.*s was not found\n %.*s:%u:%u: [ERROR] Intrinsic `%.*s` was not found\n [ERROR] Corrupted bytecode: unknown expression kind\n %.*s:%u:%u: [ERROR] String literal was not closed\n [ERROR] Corrupted bytecode: wrong magic\n [ERROR] Corrupted bytecode: not enough data\n , but got `%.*s`\n %.*s:%u:%u: [ERROR] Unexpected `%lc`\n , but got EOF\n        X             ABC  ABM  ABC  ABM      T                            ¡                  ÿÿÿÿ    .abm  .ae  ABM          ½  -     9      \n         ABC  ABM        HI                         	             \n\n\n  	  	                               \r \r   	   	                                               	                                                  	                                                   	                                              	                                                      	                                                   	         0123456789ABCDEF   N ë§~ uú ¹,ý·z¼ Ì¢ =I×  *_·úXÙýÊ½áÍÜ@x }gaì å\nÔ Ì>Ov¯  D ® ®` úw!ë+ `A ©£nN                                                        *                    \'9H                                  8R`S  Ê        »Ûë+;PSuccess Illegal byte sequence Domain error Result not representable Not a tty Permission denied Operation not permitted No such file or directory No such process File exists Value too large for defined data type No space left on device Out of memory Resource busy Interrupted system call Resource temporarily unavailable Invalid seek Cross-device link Read-only file system Directory not empty Connection reset by peer Operation timed out Connection refused Host is down Host is unreachable Address in use Broken pipe I/O error No such device or address Block device required No such device Not a directory Is a directory Text file busy Exec format error Invalid argument Argument list too long Symbolic link loop Filename too long Too many open files in system No file descriptors available Bad file descriptor No child process Bad address File too large Too many links No locks available Resource deadlock would occur State not recoverable Owner died Operation canceled Function not implemented No message of desired type Identifier removed Device not a stream No data available Device timeout Out of streams resources Link has been severed Protocol error Bad message File descriptor in bad state Not a socket Destination address required Message too large Protocol wrong type for socket Protocol not available Protocol not supported Socket type not supported Not supported Protocol family not supported Address family not supported by protocol Address not available Network is down Network unreachable Connection reset by network Connection aborted No buffer space available Socket is connected Socket not connected Cannot send after socket shutdown Operation already in progress Operation in progress Stale file handle Remote I/O error Quota exceeded No medium found Wrong medium type Multihop attempted Required key not available Key has expired Key has been revoked Key was rejected by service  B°­i                 	   	         \r   \r         ÿÿÿÿÿÿÿÿ       \n   \n          ;   ;          l   l         e   e         t   t          i   i         f   f          e   e         l   l         i   i         f   f          e   e         l   l         s   s         e   e          m   m         a   a         c   c         r   r         o   o          w   w         h   h         i   i         l   l         e   e          s   s         e   e         t   t          u   u         s   s         e   e          r   r         e   e         t   t          i   i         m   m         p   p         o   o         r   r         t   t          m   m         a   a         t   t         c   c         h   h          d   d         o   o          (   (          )   )          [   [          ]   ]          {   {          }   }          "   "          \'   \'          .   .         .   .         .   .          -   -         >   >          :   :          :   :         :   :          <   <         >   >          <   <         -   -         >   >          -   -         ÿÿÿÿÿÿÿÿ      0   9         0   9         ÿÿÿÿÿÿÿÿ       -   -         ÿÿÿÿÿÿÿÿ      0   9         0   9         ÿÿÿÿÿÿÿÿ      .   .         0   9         0   9         ÿÿÿÿÿÿÿÿ       t   t         r   r         u   u         e   e          f   f         a   a         l   l         s   s         e   e          a   z         A   Z         _   _         -   -         !   !         ?   ?         #   #         $   $         %   %         ^   ^         &   &         *   *         +   +         /   /         =   =         <   <         >   >         |   |         a   z         A   Z         _   _         -   -         !   !         ?   ?         #   #         $   $         %   %         ^   ^         &   &         *   *         +   +         /   /         =   =         <   <         >   >         |   |         0   9         ÿÿÿÿÿÿÿÿ    °            ð                                     @            `                         à            0                        °            à                        p            À            à            ð                                                  0            @            `                        °            À            à                         0                 	            	             &                     ´     c     )     ]     á     ß     S          t     U     O     Y     À      ¯          E     A                         S     ;     \'     2     6     "     ,          u     /     M     ù                                                                I                                                                ±                                                                 Q      	                                                           Q      	                                                                                                                                                                                               	                                                              	                                                         ¥     	                                                          |                                                               F                                                        	       Þ                                                        \n       x                                                               »                                                                 µ                                                         \r       µ                                                         \r       µ                                                         \r       þ                                                                 ç                                                                ö                                                                Ø                                                                Ê     \r                                                                                                                                                                                                                                                           r                                                                r                                                                ,                                                                 õ                                                               õ                                                               õ                                                               õ                                                               õ                                                                õ                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              Ë                                                               Ë                                                               _                                                                 i                                                                 ²                                                                ²                                                                                                                                                                                                R                                                                R                                                                ¬                                                                 ¬                                                                 Ú                                                        !       Ú                                                        !       &                                                        "       &                                                        "       Ú                                                        #       Ú                                                        #                                                                 $       ^                                                          %       D                                                          &       ©                                                           \'       Ê     	                                                     (                                                                 )       i                                                          *       $                                                          +       þ                                                          ,       a                                                          -                                                                  .                                                                 /       l                                                     0       ç     \r                                                   1                                                                2       N                                                       3                                                                 4       ?                                                          5       N               ¾                                                         6       ¾                                                         6       þ                                                        7       þ                                                        7       [                                                         8       [                                                         8       |                                                         9       |                                                         9       ¶                                                          :       Ï                                                         ;       \n               Ç      \n                                                  <            \n                                                  =       ¿                                                       >       L                                                        ?                                                              @       ù                                                        A       Â                                                        B       t                                                         C                                                                D       h                                                         E       ]     \n                                                    F              Ô                                                          G       µ                                                           H                           \r                                                    J            	                                                    K       z     \n                                                    L                                                                 M                                                                  N                                                                  O                      )     \r                                                    P       1     \r                                                   Q       Å                                                        R       ×                                                          S       Õ                                                         T                                                               U       -                                                         V                                                                         W       {                                                          X                                                                Y              ù     \r                                                      Z       è                                                            [       ä                                                            \\              Ò                                                           _       =                                                         `                                                                 a       4                                                         b                                                                 c                                                                d                                                                e       b     	                                                    f       b                                                         g       £     \r                                                    h       l                                                         i       k                                                         j            \r                                                    k       7                                                         l       5                                                         m       ¾                                                          n       ±                                                          o       \r     \r                                                     p                                     t                                               r       q       ÀO                                                ÿÿÿÿÿÿÿÿ                                                                                    HI                            u                                               r       v       ÈO                                               ÿÿÿÿ\n                                                                                       8J      W      B°Þ{ console.log(UTF8ToString($0)); } { alert(UTF8ToString($0)); } { const element = document.querySelector(UTF8ToString($0)); element.innerHTML = UTF8ToString($1); } { const element = document.querySelector(UTF8ToString($0)); element.textContent = UTF8ToString($1); } { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.innerHTML); } { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.textContent); }  target_features	+bulk-memory+bulk-memory-opt+call-indirect-overlong+memory64+\nmultivalue+mutable-globals+nontrapping-fptoint+reference-types+sign-ext');
}

function getBinarySync(file) {
  return file;
}

async function getWasmBinary(binaryFile) {

  // Otherwise, getBinarySync should be able to get it synchronously
  return getBinarySync(binaryFile);
}

async function instantiateArrayBuffer(binaryFile, imports) {
  try {
    var binary = await getWasmBinary(binaryFile);
    var instance = await WebAssembly.instantiate(binary, imports);
    return instance;
  } catch (reason) {
    err(`failed to asynchronously prepare wasm: ${reason}`);

    // Warn on some common problems.
    if (isFileURI(binaryFile)) {
      err(`warning: Loading from a file URI (${binaryFile}) is not supported in most browsers. See https://emscripten.org/docs/getting_started/FAQ.html#how-do-i-run-a-local-webserver-for-testing-why-does-my-program-stall-in-downloading-or-preparing`);
    }
    abort(reason);
  }
}

async function instantiateAsync(binary, binaryFile, imports) {
  return instantiateArrayBuffer(binaryFile, imports);
}

function getWasmImports() {
  // prepare imports
  var imports = {
    'env': wasmImports,
    'wasi_snapshot_preview1': wasmImports,
  };
  return imports;
}

// Create the wasm instance.
// Receives the wasm imports, returns the exports.
async function createWasm() {
  // Load the wasm module and create an instance of using native support in the JS engine.
  // handle a generated wasm instance, receiving its exports and
  // performing other necessary setup
  /** @param {WebAssembly.Module=} module*/
  function receiveInstance(instance, module) {
    wasmExports = instance.exports;

    wasmExports = applySignatureConversions(wasmExports);

    assignWasmExports(wasmExports);

    updateMemoryViews();

    removeRunDependency('wasm-instantiate');
    return wasmExports;
  }
  addRunDependency('wasm-instantiate');

  // Prefer streaming instantiation if available.
  // Async compilation can be confusing when an error on the page overwrites Module
  // (for example, if the order of elements is wrong, and the one defining Module is
  // later), so we save Module and check it later.
  var trueModule = Module;
  function receiveInstantiationResult(result) {
    // 'result' is a ResultObject object which has both the module and instance.
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    assert(Module === trueModule, 'the Module object should not be replaced during async compilation - perhaps the order of HTML elements is wrong?');
    trueModule = null;
    // TODO: Due to Closure regression https://github.com/google/closure-compiler/issues/3193, the above line no longer optimizes out down to the following line.
    // When the regression is fixed, can restore the above PTHREADS-enabled path.
    return receiveInstance(result['instance']);
  }

  var info = getWasmImports();

  // User shell pages can write their own Module.instantiateWasm = function(imports, successCallback) callback
  // to manually instantiate the Wasm module themselves. This allows pages to
  // run the instantiation parallel to any other async startup actions they are
  // performing.
  // Also pthreads and wasm workers initialize the wasm instance through this
  // path.
  if (Module['instantiateWasm']) {
    return new Promise((resolve, reject) => {
      try {
        Module['instantiateWasm'](info, (inst, mod) => {
          resolve(receiveInstance(inst, mod));
        });
      } catch(e) {
        err(`Module.instantiateWasm callback failed with error: ${e}`);
        reject(e);
      }
    });
  }

  wasmBinaryFile ??= findWasmBinary();
  var result = await instantiateAsync(wasmBinary, wasmBinaryFile, info);
  var exports = receiveInstantiationResult(result);
  return exports;
}

// end include: preamble.js

// Begin JS library code


  class ExitStatus {
      name = 'ExitStatus';
      constructor(status) {
        this.message = `Program terminated with exit(${status})`;
        this.status = status;
      }
    }

  var callRuntimeCallbacks = (callbacks) => {
      while (callbacks.length > 0) {
        // Pass the module as the first argument.
        callbacks.shift()(Module);
      }
    };
  var onPostRuns = [];
  var addOnPostRun = (cb) => onPostRuns.push(cb);

  var onPreRuns = [];
  var addOnPreRun = (cb) => onPreRuns.push(cb);

  var runDependencies = 0;
  
  
  var dependenciesFulfilled = null;
  
  var runDependencyTracking = {
  };
  
  var runDependencyWatcher = null;
  var removeRunDependency = (id) => {
      runDependencies--;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'removeRunDependency requires an ID');
      assert(runDependencyTracking[id]);
      delete runDependencyTracking[id];
      if (runDependencies == 0) {
        if (runDependencyWatcher !== null) {
          clearInterval(runDependencyWatcher);
          runDependencyWatcher = null;
        }
        if (dependenciesFulfilled) {
          var callback = dependenciesFulfilled;
          dependenciesFulfilled = null;
          callback(); // can add another dependenciesFulfilled
        }
      }
    };
  
  
  var addRunDependency = (id) => {
      runDependencies++;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'addRunDependency requires an ID')
      assert(!runDependencyTracking[id]);
      runDependencyTracking[id] = 1;
      if (runDependencyWatcher === null && globalThis.setInterval) {
        // Check for missing dependencies every few seconds
        runDependencyWatcher = setInterval(() => {
          if (ABORT) {
            clearInterval(runDependencyWatcher);
            runDependencyWatcher = null;
            return;
          }
          var shown = false;
          for (var dep in runDependencyTracking) {
            if (!shown) {
              shown = true;
              err('still waiting on run dependencies:');
            }
            err(`dependency: ${dep}`);
          }
          if (shown) {
            err('(end of list)');
          }
        }, 10000);
      }
    };

  /** @noinline */
  var base64Decode = (b64) => {
  
      assert(b64.length % 4 == 0);
      var b1, b2, i = 0, j = 0, bLength = b64.length;
      var output = new Uint8Array((bLength*3>>2) - (b64[bLength-2] == '=') - (b64[bLength-1] == '='));
      for (; i < bLength; i += 4, j += 3) {
        b1 = base64ReverseLookup[b64.charCodeAt(i+1)];
        b2 = base64ReverseLookup[b64.charCodeAt(i+2)];
        output[j] = base64ReverseLookup[b64.charCodeAt(i)] << 2 | b1 >> 4;
        output[j+1] = b1 << 4 | b2 >> 2;
        output[j+2] = b2 << 6 | base64ReverseLookup[b64.charCodeAt(i+3)];
      }
      return output;
    };


  
    /**
     * @param {number} ptr
     * @param {string} type
     */
  function getValue(ptr, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': return HEAP8[ptr];
      case 'i8': return HEAP8[ptr];
      case 'i16': return HEAP16[((ptr)/2)];
      case 'i32': return HEAP32[((ptr)/4)];
      case 'i64': return HEAP64[((ptr)/8)];
      case 'float': return HEAPF32[((ptr)/4)];
      case 'double': return HEAPF64[((ptr)/8)];
      case '*': return Number(HEAPU64[((ptr)/8)]);
      default: abort(`invalid type for getValue: ${type}`);
    }
  }

  var noExitRuntime = true;

  var ptrToString = (ptr) => {
      assert(typeof ptr === 'number', `ptrToString expects a number, got ${typeof ptr}`);
      // Convert to 64-bit unsigned value.  We need to use BigInt here since
      // Number cannot represent the full 64-bit range.
      if (ptr < 0) ptr = 2n**64n + BigInt(ptr);
      return '0x' + ptr.toString(16).padStart(16, '0');
    };


  
    /**
     * @param {number} ptr
     * @param {number} value
     * @param {string} type
     */
  function setValue(ptr, value, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': HEAP8[ptr] = value; break;
      case 'i8': HEAP8[ptr] = value; break;
      case 'i16': HEAP16[((ptr)/2)] = value; break;
      case 'i32': HEAP32[((ptr)/4)] = value; break;
      case 'i64': HEAP64[((ptr)/8)] = BigInt(value); break;
      case 'float': HEAPF32[((ptr)/4)] = value; break;
      case 'double': HEAPF64[((ptr)/8)] = value; break;
      case '*': HEAPU64[((ptr)/8)] = BigInt(value); break;
      default: abort(`invalid type for setValue: ${type}`);
    }
  }

  var stackRestore = (val) => __emscripten_stack_restore(val);

  var stackSave = () => _emscripten_stack_get_current();

  var lengthBytesUTF8 = (str) => {
      var len = 0;
      for (var i = 0; i < str.length; ++i) {
        // Gotcha: charCodeAt returns a 16-bit word that is a UTF-16 encoded code
        // unit, not a Unicode code point of the character! So decode
        // UTF16->UTF32->UTF8.
        // See http://unicode.org/faq/utf_bom.html#utf16-3
        var c = str.charCodeAt(i); // possibly a lead surrogate
        if (c <= 0x7F) {
          len++;
        } else if (c <= 0x7FF) {
          len += 2;
        } else if (c >= 0xD800 && c <= 0xDFFF) {
          len += 4; ++i;
        } else {
          len += 3;
        }
      }
      return len;
    };
  
  var warnOnce = (text) => {
      warnOnce.shown ||= {};
      if (!warnOnce.shown[text]) {
        warnOnce.shown[text] = 1;
        err(text);
      }
    };
  var stringToUTF8Array = (str, heap, outIdx, maxBytesToWrite) => {
      assert(typeof str === 'string', `stringToUTF8Array expects a string (got ${typeof str})`);
      // Parameter maxBytesToWrite is not optional. Negative values, 0, null,
      // undefined and false each don't write out any bytes.
      if (!(maxBytesToWrite > 0))
        return 0;
  
      var startIdx = outIdx;
      var endIdx = outIdx + maxBytesToWrite - 1; // -1 for string null terminator.
      for (var i = 0; i < str.length; ++i) {
        // For UTF8 byte structure, see http://en.wikipedia.org/wiki/UTF-8#Description
        // and https://www.ietf.org/rfc/rfc2279.txt
        // and https://tools.ietf.org/html/rfc3629
        var u = str.codePointAt(i);
        if (u <= 0x7F) {
          if (outIdx >= endIdx) break;
          heap[outIdx++] = u;
        } else if (u <= 0x7FF) {
          if (outIdx + 1 >= endIdx) break;
          heap[outIdx++] = 0xC0 | (u >> 6);
          heap[outIdx++] = 0x80 | (u & 63);
        } else if (u <= 0xFFFF) {
          if (outIdx + 2 >= endIdx) break;
          heap[outIdx++] = 0xE0 | (u >> 12);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
        } else {
          if (outIdx + 3 >= endIdx) break;
          if (u > 0x10FFFF) warnOnce('Invalid Unicode code point ' + ptrToString(u) + ' encountered when serializing a JS string to a UTF-8 string in wasm memory! (Valid unicode code points should be in range 0-0x10FFFF).');
          heap[outIdx++] = 0xF0 | (u >> 18);
          heap[outIdx++] = 0x80 | ((u >> 12) & 63);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
          // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
          // We need to manually skip over the second code unit for correct iteration.
          i++;
        }
      }
      // Null-terminate the pointer to the buffer.
      heap[outIdx] = 0;
      return outIdx - startIdx;
    };
  var stringToUTF8 = (str, outPtr, maxBytesToWrite) => {
      assert(typeof maxBytesToWrite == 'number', 'stringToUTF8(str, outPtr, maxBytesToWrite) is missing the third parameter that specifies the length of the output buffer!');
      return stringToUTF8Array(str, HEAPU8, outPtr, maxBytesToWrite);
    };
  
  var stringToNewUTF8 = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = _malloc(size);
      if (ret) stringToUTF8(str, ret, size);
      return ret;
    };


  

  var initRandomFill = () => {
  
      return (view) => crypto.getRandomValues(view);
    };
  var randomFill = (view) => {
      // Lazily init on the first invocation.
      (randomFill = initRandomFill())(view);
    };
  
  var PATH = {
  isAbs:(path) => path.charAt(0) === '/',
  splitPath:(filename) => {
        var splitPathRe = /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;
        return splitPathRe.exec(filename).slice(1);
      },
  normalizeArray:(parts, allowAboveRoot) => {
        // if the path tries to go above the root, `up` ends up > 0
        var up = 0;
        for (var i = parts.length - 1; i >= 0; i--) {
          var last = parts[i];
          if (last === '.') {
            parts.splice(i, 1);
          } else if (last === '..') {
            parts.splice(i, 1);
            up++;
          } else if (up) {
            parts.splice(i, 1);
            up--;
          }
        }
        // if the path is allowed to go above the root, restore leading ..s
        if (allowAboveRoot) {
          for (; up; up--) {
            parts.unshift('..');
          }
        }
        return parts;
      },
  normalize:(path) => {
        var isAbsolute = PATH.isAbs(path),
            trailingSlash = path.slice(-1) === '/';
        // Normalize the path
        path = PATH.normalizeArray(path.split('/').filter((p) => !!p), !isAbsolute).join('/');
        if (!path && !isAbsolute) {
          path = '.';
        }
        if (path && trailingSlash) {
          path += '/';
        }
        return (isAbsolute ? '/' : '') + path;
      },
  dirname:(path) => {
        var result = PATH.splitPath(path),
            root = result[0],
            dir = result[1];
        if (!root && !dir) {
          // No dirname whatsoever
          return '.';
        }
        if (dir) {
          // It has a dirname, strip trailing slash
          dir = dir.slice(0, -1);
        }
        return root + dir;
      },
  basename:(path) => path && path.match(/([^\/]+|\/)\/*$/)[1],
  join:(...paths) => PATH.normalize(paths.join('/')),
  join2:(l, r) => PATH.normalize(l + '/' + r),
  };
  
  
  var PATH_FS = {
  resolve:(...args) => {
        var resolvedPath = '',
          resolvedAbsolute = false;
        for (var i = args.length - 1; i >= -1 && !resolvedAbsolute; i--) {
          var path = (i >= 0) ? args[i] : FS.cwd();
          // Skip empty and invalid entries
          if (typeof path != 'string') {
            throw new TypeError('Arguments to path.resolve must be strings');
          } else if (!path) {
            return ''; // an invalid portion invalidates the whole thing
          }
          resolvedPath = path + '/' + resolvedPath;
          resolvedAbsolute = PATH.isAbs(path);
        }
        // At this point the path should be resolved to a full absolute path, but
        // handle relative paths to be safe (might happen when process.cwd() fails)
        resolvedPath = PATH.normalizeArray(resolvedPath.split('/').filter((p) => !!p), !resolvedAbsolute).join('/');
        return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
      },
  relative:(from, to) => {
        from = PATH_FS.resolve(from).slice(1);
        to = PATH_FS.resolve(to).slice(1);
        function trim(arr) {
          var start = 0;
          for (; start < arr.length; start++) {
            if (arr[start] !== '') break;
          }
          var end = arr.length - 1;
          for (; end >= 0; end--) {
            if (arr[end] !== '') break;
          }
          if (start > end) return [];
          return arr.slice(start, end - start + 1);
        }
        var fromParts = trim(from.split('/'));
        var toParts = trim(to.split('/'));
        var length = Math.min(fromParts.length, toParts.length);
        var samePartsLength = length;
        for (var i = 0; i < length; i++) {
          if (fromParts[i] !== toParts[i]) {
            samePartsLength = i;
            break;
          }
        }
        var outputParts = [];
        for (var i = samePartsLength; i < fromParts.length; i++) {
          outputParts.push('..');
        }
        outputParts = outputParts.concat(toParts.slice(samePartsLength));
        return outputParts.join('/');
      },
  };
  
  
  var UTF8Decoder = globalThis.TextDecoder && new TextDecoder();
  
  var findStringEnd = (heapOrArray, idx, maxBytesToRead, ignoreNul) => {
      var maxIdx = idx + maxBytesToRead;
      if (ignoreNul) return maxIdx;
      // TextDecoder needs to know the byte length in advance, it doesn't stop on
      // null terminator by itself.
      // As a tiny code save trick, compare idx against maxIdx using a negation,
      // so that maxBytesToRead=undefined/NaN means Infinity.
      while (heapOrArray[idx] && !(idx >= maxIdx)) ++idx;
      return idx;
    };
  
  
    /**
     * Given a pointer 'idx' to a null-terminated UTF8-encoded string in the given
     * array that contains uint8 values, returns a copy of that string as a
     * Javascript String object.
     * heapOrArray is either a regular array, or a JavaScript typed array view.
     * @param {number=} idx
     * @param {number=} maxBytesToRead
     * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
     * @return {string}
     */
  var UTF8ArrayToString = (heapOrArray, idx = 0, maxBytesToRead, ignoreNul) => {
  
      var endPtr = findStringEnd(heapOrArray, idx, maxBytesToRead, ignoreNul);
  
      // When using conditional TextDecoder, skip it for short strings as the overhead of the native call is not worth it.
      if (endPtr - idx > 16 && heapOrArray.buffer && UTF8Decoder) {
        return UTF8Decoder.decode(heapOrArray.subarray(idx, endPtr));
      }
      var str = '';
      while (idx < endPtr) {
        // For UTF8 byte structure, see:
        // http://en.wikipedia.org/wiki/UTF-8#Description
        // https://www.ietf.org/rfc/rfc2279.txt
        // https://tools.ietf.org/html/rfc3629
        var u0 = heapOrArray[idx++];
        if (!(u0 & 0x80)) { str += String.fromCharCode(u0); continue; }
        var u1 = heapOrArray[idx++] & 63;
        if ((u0 & 0xE0) == 0xC0) { str += String.fromCharCode(((u0 & 31) << 6) | u1); continue; }
        var u2 = heapOrArray[idx++] & 63;
        if ((u0 & 0xF0) == 0xE0) {
          u0 = ((u0 & 15) << 12) | (u1 << 6) | u2;
        } else {
          if ((u0 & 0xF8) != 0xF0) warnOnce('Invalid UTF-8 leading byte ' + ptrToString(u0) + ' encountered when deserializing a UTF-8 string in wasm memory to a JS string!');
          u0 = ((u0 & 7) << 18) | (u1 << 12) | (u2 << 6) | (heapOrArray[idx++] & 63);
        }
  
        if (u0 < 0x10000) {
          str += String.fromCharCode(u0);
        } else {
          var ch = u0 - 0x10000;
          str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
        }
      }
      return str;
    };
  
  var FS_stdin_getChar_buffer = [];
  
  
  /** @type {function(string, boolean=, number=)} */
  var intArrayFromString = (stringy, dontAddNull, length) => {
      var len = length > 0 ? length : lengthBytesUTF8(stringy)+1;
      var u8array = new Array(len);
      var numBytesWritten = stringToUTF8Array(stringy, u8array, 0, u8array.length);
      if (dontAddNull) u8array.length = numBytesWritten;
      return u8array;
    };
  var FS_stdin_getChar = () => {
      if (!FS_stdin_getChar_buffer.length) {
        var result = null;
        if (globalThis.window?.prompt) {
          // Browser.
          result = window.prompt('Input: ');  // returns null on cancel
          if (result !== null) {
            result += '\n';
          }
        } else
        {}
        if (!result) {
          return null;
        }
        FS_stdin_getChar_buffer = intArrayFromString(result, true);
      }
      return FS_stdin_getChar_buffer.shift();
    };
  var TTY = {
  ttys:[],
  init() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // currently, FS.init does not distinguish if process.stdin is a file or TTY
        //   // device, it always assumes it's a TTY device. because of this, we're forcing
        //   // process.stdin to UTF8 encoding to at least make stdin reading compatible
        //   // with text files until FS.init can be refactored.
        //   process.stdin.setEncoding('utf8');
        // }
      },
  shutdown() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // inolen: any idea as to why node -e 'process.stdin.read()' wouldn't exit immediately (with process.stdin being a tty)?
        //   // isaacs: because now it's reading from the stream, you've expressed interest in it, so that read() kicks off a _read() which creates a ReadReq operation
        //   // inolen: I thought read() in that case was a synchronous operation that just grabbed some amount of buffered data if it exists?
        //   // isaacs: it is. but it also triggers a _read() call, which calls readStart() on the handle
        //   // isaacs: do process.stdin.pause() and i'd think it'd probably close the pending call
        //   process.stdin.pause();
        // }
      },
  register(dev, ops) {
        TTY.ttys[dev] = { input: [], output: [], ops: ops };
        FS.registerDevice(dev, TTY.stream_ops);
      },
  stream_ops:{
  open(stream) {
          var tty = TTY.ttys[stream.node.rdev];
          if (!tty) {
            throw new FS.ErrnoError(43);
          }
          stream.tty = tty;
          stream.seekable = false;
        },
  close(stream) {
          // flush any pending line data
          stream.tty.ops.fsync(stream.tty);
        },
  fsync(stream) {
          stream.tty.ops.fsync(stream.tty);
        },
  read(stream, buffer, offset, length, pos /* ignored */) {
          if (!stream.tty || !stream.tty.ops.get_char) {
            throw new FS.ErrnoError(60);
          }
          var bytesRead = 0;
          for (var i = 0; i < length; i++) {
            var result;
            try {
              result = stream.tty.ops.get_char(stream.tty);
            } catch (e) {
              throw new FS.ErrnoError(29);
            }
            if (result === undefined && bytesRead === 0) {
              throw new FS.ErrnoError(6);
            }
            if (result === null || result === undefined) break;
            bytesRead++;
            buffer[offset+i] = result;
          }
          if (bytesRead) {
            stream.node.atime = Date.now();
          }
          return bytesRead;
        },
  write(stream, buffer, offset, length, pos) {
          if (!stream.tty || !stream.tty.ops.put_char) {
            throw new FS.ErrnoError(60);
          }
          try {
            for (var i = 0; i < length; i++) {
              stream.tty.ops.put_char(stream.tty, buffer[offset+i]);
            }
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
          if (length) {
            stream.node.mtime = stream.node.ctime = Date.now();
          }
          return i;
        },
  },
  default_tty_ops:{
  get_char(tty) {
          return FS_stdin_getChar();
        },
  put_char(tty, val) {
          if (val === null || val === 10) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val); // val == 0 would cut text output off in the middle.
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  ioctl_tcgets(tty) {
          // typical setting
          return {
            c_iflag: 25856,
            c_oflag: 5,
            c_cflag: 191,
            c_lflag: 35387,
            c_cc: [
              0x03, 0x1c, 0x7f, 0x15, 0x04, 0x00, 0x01, 0x00, 0x11, 0x13, 0x1a, 0x00,
              0x12, 0x0f, 0x17, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ]
          };
        },
  ioctl_tcsets(tty, optional_actions, data) {
          // currently just ignore
          return 0;
        },
  ioctl_tiocgwinsz(tty) {
          return [24, 80];
        },
  },
  default_tty1_ops:{
  put_char(tty, val) {
          if (val === null || val === 10) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val);
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  },
  };
  
  
  var mmapAlloc = (size) => {
      abort('internal error: mmapAlloc called but `emscripten_builtin_memalign` native symbol not exported');
    };
  var MEMFS = {
  ops_table:null,
  mount(mount) {
        return MEMFS.createNode(null, '/', 16895, 0);
      },
  createNode(parent, name, mode, dev) {
        if (FS.isBlkdev(mode) || FS.isFIFO(mode)) {
          // no supported
          throw new FS.ErrnoError(63);
        }
        MEMFS.ops_table ||= {
          dir: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              lookup: MEMFS.node_ops.lookup,
              mknod: MEMFS.node_ops.mknod,
              rename: MEMFS.node_ops.rename,
              unlink: MEMFS.node_ops.unlink,
              rmdir: MEMFS.node_ops.rmdir,
              readdir: MEMFS.node_ops.readdir,
              symlink: MEMFS.node_ops.symlink
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek
            }
          },
          file: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek,
              read: MEMFS.stream_ops.read,
              write: MEMFS.stream_ops.write,
              mmap: MEMFS.stream_ops.mmap,
              msync: MEMFS.stream_ops.msync
            }
          },
          link: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              readlink: MEMFS.node_ops.readlink
            },
            stream: {}
          },
          chrdev: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: FS.chrdev_stream_ops
          }
        };
        var node = FS.createNode(parent, name, mode, dev);
        if (FS.isDir(node.mode)) {
          node.node_ops = MEMFS.ops_table.dir.node;
          node.stream_ops = MEMFS.ops_table.dir.stream;
          node.contents = {};
        } else if (FS.isFile(node.mode)) {
          node.node_ops = MEMFS.ops_table.file.node;
          node.stream_ops = MEMFS.ops_table.file.stream;
          node.usedBytes = 0; // The actual number of bytes used in the typed array, as opposed to contents.length which gives the whole capacity.
          // When the byte data of the file is populated, this will point to either a typed array, or a normal JS array. Typed arrays are preferred
          // for performance, and used by default. However, typed arrays are not resizable like normal JS arrays are, so there is a small disk size
          // penalty involved for appending file writes that continuously grow a file similar to std::vector capacity vs used -scheme.
          node.contents = null; 
        } else if (FS.isLink(node.mode)) {
          node.node_ops = MEMFS.ops_table.link.node;
          node.stream_ops = MEMFS.ops_table.link.stream;
        } else if (FS.isChrdev(node.mode)) {
          node.node_ops = MEMFS.ops_table.chrdev.node;
          node.stream_ops = MEMFS.ops_table.chrdev.stream;
        }
        node.atime = node.mtime = node.ctime = Date.now();
        // add the new node to the parent
        if (parent) {
          parent.contents[name] = node;
          parent.atime = parent.mtime = parent.ctime = node.atime;
        }
        return node;
      },
  getFileDataAsTypedArray(node) {
        if (!node.contents) return new Uint8Array(0);
        if (node.contents.subarray) return node.contents.subarray(0, node.usedBytes); // Make sure to not return excess unused bytes.
        return new Uint8Array(node.contents);
      },
  expandFileStorage(node, newCapacity) {
        var prevCapacity = node.contents ? node.contents.length : 0;
        if (prevCapacity >= newCapacity) return; // No need to expand, the storage was already large enough.
        // Don't expand strictly to the given requested limit if it's only a very small increase, but instead geometrically grow capacity.
        // For small filesizes (<1MB), perform size*2 geometric increase, but for large sizes, do a much more conservative size*1.125 increase to
        // avoid overshooting the allocation cap by a very large margin.
        var CAPACITY_DOUBLING_MAX = 1024 * 1024;
        newCapacity = Math.max(newCapacity, (prevCapacity * (prevCapacity < CAPACITY_DOUBLING_MAX ? 2.0 : 1.125)) >>> 0);
        if (prevCapacity != 0) newCapacity = Math.max(newCapacity, 256); // At minimum allocate 256b for each file when expanding.
        var oldContents = node.contents;
        node.contents = new Uint8Array(newCapacity); // Allocate new storage.
        if (node.usedBytes > 0) node.contents.set(oldContents.subarray(0, node.usedBytes), 0); // Copy old data over to the new storage.
      },
  resizeFileStorage(node, newSize) {
        if (node.usedBytes == newSize) return;
        if (newSize == 0) {
          node.contents = null; // Fully decommit when requesting a resize to zero.
          node.usedBytes = 0;
        } else {
          var oldContents = node.contents;
          node.contents = new Uint8Array(newSize); // Allocate new storage.
          if (oldContents) {
            node.contents.set(oldContents.subarray(0, Math.min(newSize, node.usedBytes))); // Copy old data over to the new storage.
          }
          node.usedBytes = newSize;
        }
      },
  node_ops:{
  getattr(node) {
          var attr = {};
          // device numbers reuse inode numbers.
          attr.dev = FS.isChrdev(node.mode) ? node.id : 1;
          attr.ino = node.id;
          attr.mode = node.mode;
          attr.nlink = 1;
          attr.uid = 0;
          attr.gid = 0;
          attr.rdev = node.rdev;
          if (FS.isDir(node.mode)) {
            attr.size = 4096;
          } else if (FS.isFile(node.mode)) {
            attr.size = node.usedBytes;
          } else if (FS.isLink(node.mode)) {
            attr.size = node.link.length;
          } else {
            attr.size = 0;
          }
          attr.atime = new Date(node.atime);
          attr.mtime = new Date(node.mtime);
          attr.ctime = new Date(node.ctime);
          // NOTE: In our implementation, st_blocks = Math.ceil(st_size/st_blksize),
          //       but this is not required by the standard.
          attr.blksize = 4096;
          attr.blocks = Math.ceil(attr.size / attr.blksize);
          return attr;
        },
  setattr(node, attr) {
          for (const key of ["mode", "atime", "mtime", "ctime"]) {
            if (attr[key] != null) {
              node[key] = attr[key];
            }
          }
          if (attr.size !== undefined) {
            MEMFS.resizeFileStorage(node, attr.size);
          }
        },
  lookup(parent, name) {
          throw new FS.ErrnoError(44);
        },
  mknod(parent, name, mode, dev) {
          return MEMFS.createNode(parent, name, mode, dev);
        },
  rename(old_node, new_dir, new_name) {
          var new_node;
          try {
            new_node = FS.lookupNode(new_dir, new_name);
          } catch (e) {}
          if (new_node) {
            if (FS.isDir(old_node.mode)) {
              // if we're overwriting a directory at new_name, make sure it's empty.
              for (var i in new_node.contents) {
                throw new FS.ErrnoError(55);
              }
            }
            FS.hashRemoveNode(new_node);
          }
          // do the internal rewiring
          delete old_node.parent.contents[old_node.name];
          new_dir.contents[new_name] = old_node;
          old_node.name = new_name;
          new_dir.ctime = new_dir.mtime = old_node.parent.ctime = old_node.parent.mtime = Date.now();
        },
  unlink(parent, name) {
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  rmdir(parent, name) {
          var node = FS.lookupNode(parent, name);
          for (var i in node.contents) {
            throw new FS.ErrnoError(55);
          }
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  readdir(node) {
          return ['.', '..', ...Object.keys(node.contents)];
        },
  symlink(parent, newname, oldpath) {
          var node = MEMFS.createNode(parent, newname, 0o777 | 40960, 0);
          node.link = oldpath;
          return node;
        },
  readlink(node) {
          if (!FS.isLink(node.mode)) {
            throw new FS.ErrnoError(28);
          }
          return node.link;
        },
  },
  stream_ops:{
  read(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= stream.node.usedBytes) return 0;
          var size = Math.min(stream.node.usedBytes - position, length);
          assert(size >= 0);
          if (size > 8 && contents.subarray) { // non-trivial, and typed array
            buffer.set(contents.subarray(position, position + size), offset);
          } else {
            for (var i = 0; i < size; i++) buffer[offset + i] = contents[position + i];
          }
          return size;
        },
  write(stream, buffer, offset, length, position, canOwn) {
          // The data buffer should be a typed array view
          assert(!(buffer instanceof ArrayBuffer));
  
          if (!length) return 0;
          var node = stream.node;
          node.mtime = node.ctime = Date.now();
  
          if (buffer.subarray && (!node.contents || node.contents.subarray)) { // This write is from a typed array to a typed array?
            if (canOwn) {
              assert(position === 0, 'canOwn must imply no weird position inside the file');
              node.contents = buffer.subarray(offset, offset + length);
              node.usedBytes = length;
              return length;
            } else if (node.usedBytes === 0 && position === 0) { // If this is a simple first write to an empty file, do a fast set since we don't need to care about old data.
              node.contents = buffer.slice(offset, offset + length);
              node.usedBytes = length;
              return length;
            } else if (position + length <= node.usedBytes) { // Writing to an already allocated and used subrange of the file?
              node.contents.set(buffer.subarray(offset, offset + length), position);
              return length;
            }
          }
  
          // Appending to an existing file and we need to reallocate, or source data did not come as a typed array.
          MEMFS.expandFileStorage(node, position+length);
          if (node.contents.subarray && buffer.subarray) {
            // Use typed array write which is available.
            node.contents.set(buffer.subarray(offset, offset + length), position);
          } else {
            for (var i = 0; i < length; i++) {
             node.contents[position + i] = buffer[offset + i]; // Or fall back to manual write if not.
            }
          }
          node.usedBytes = Math.max(node.usedBytes, position + length);
          return length;
        },
  llseek(stream, offset, whence) {
          var position = offset;
          if (whence === 1) {
            position += stream.position;
          } else if (whence === 2) {
            if (FS.isFile(stream.node.mode)) {
              position += stream.node.usedBytes;
            }
          }
          if (position < 0) {
            throw new FS.ErrnoError(28);
          }
          return position;
        },
  mmap(stream, length, position, prot, flags) {
          if (!FS.isFile(stream.node.mode)) {
            throw new FS.ErrnoError(43);
          }
          var ptr;
          var allocated;
          var contents = stream.node.contents;
          // Only make a new copy when MAP_PRIVATE is specified.
          if (!(flags & 2) && contents && contents.buffer === HEAP8.buffer) {
            // We can't emulate MAP_SHARED when the file is not backed by the
            // buffer we're mapping to (e.g. the HEAP buffer).
            allocated = false;
            ptr = contents.byteOffset;
          } else {
            allocated = true;
            ptr = mmapAlloc(length);
            if (!ptr) {
              throw new FS.ErrnoError(48);
            }
            if (contents) {
              // Try to avoid unnecessary slices.
              if (position > 0 || position + length < contents.length) {
                if (contents.subarray) {
                  contents = contents.subarray(position, position + length);
                } else {
                  contents = Array.prototype.slice.call(contents, position, position + length);
                }
              }
              HEAP8.set(contents, ptr);
            }
          }
          return { ptr, allocated };
        },
  msync(stream, buffer, offset, length, mmapFlags) {
          MEMFS.stream_ops.write(stream, buffer, 0, length, offset, false);
          // should we check if bytesWritten and length are the same?
          return 0;
        },
  },
  };
  
  var FS_modeStringToFlags = (str) => {
      var flagModes = {
        'r': 0,
        'r+': 2,
        'w': 512 | 64 | 1,
        'w+': 512 | 64 | 2,
        'a': 1024 | 64 | 1,
        'a+': 1024 | 64 | 2,
      };
      var flags = flagModes[str];
      if (typeof flags == 'undefined') {
        throw new Error(`Unknown file open mode: ${str}`);
      }
      return flags;
    };
  
  var FS_getMode = (canRead, canWrite) => {
      var mode = 0;
      if (canRead) mode |= 292 | 73;
      if (canWrite) mode |= 146;
      return mode;
    };
  
  
  
  
    /**
     * Given a pointer 'ptr' to a null-terminated UTF8-encoded string in the
     * emscripten HEAP, returns a copy of that string as a Javascript String object.
     *
     * @param {number} ptr
     * @param {number=} maxBytesToRead - An optional length that specifies the
     *   maximum number of bytes to read. You can omit this parameter to scan the
     *   string until the first 0 byte. If maxBytesToRead is passed, and the string
     *   at [ptr, ptr+maxBytesToReadr[ contains a null byte in the middle, then the
     *   string will cut short at that byte index.
     * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
     * @return {string}
     */
  var UTF8ToString = (ptr, maxBytesToRead, ignoreNul) => {
      assert(typeof ptr == 'number', `UTF8ToString expects a number (got ${typeof ptr})`);
      return ptr ? UTF8ArrayToString(HEAPU8, ptr, maxBytesToRead, ignoreNul) : '';
    };
  
  var strError = (errno) => UTF8ToString(_strerror(errno));
  
  var ERRNO_CODES = {
      'EPERM': 63,
      'ENOENT': 44,
      'ESRCH': 71,
      'EINTR': 27,
      'EIO': 29,
      'ENXIO': 60,
      'E2BIG': 1,
      'ENOEXEC': 45,
      'EBADF': 8,
      'ECHILD': 12,
      'EAGAIN': 6,
      'EWOULDBLOCK': 6,
      'ENOMEM': 48,
      'EACCES': 2,
      'EFAULT': 21,
      'ENOTBLK': 105,
      'EBUSY': 10,
      'EEXIST': 20,
      'EXDEV': 75,
      'ENODEV': 43,
      'ENOTDIR': 54,
      'EISDIR': 31,
      'EINVAL': 28,
      'ENFILE': 41,
      'EMFILE': 33,
      'ENOTTY': 59,
      'ETXTBSY': 74,
      'EFBIG': 22,
      'ENOSPC': 51,
      'ESPIPE': 70,
      'EROFS': 69,
      'EMLINK': 34,
      'EPIPE': 64,
      'EDOM': 18,
      'ERANGE': 68,
      'ENOMSG': 49,
      'EIDRM': 24,
      'ECHRNG': 106,
      'EL2NSYNC': 156,
      'EL3HLT': 107,
      'EL3RST': 108,
      'ELNRNG': 109,
      'EUNATCH': 110,
      'ENOCSI': 111,
      'EL2HLT': 112,
      'EDEADLK': 16,
      'ENOLCK': 46,
      'EBADE': 113,
      'EBADR': 114,
      'EXFULL': 115,
      'ENOANO': 104,
      'EBADRQC': 103,
      'EBADSLT': 102,
      'EDEADLOCK': 16,
      'EBFONT': 101,
      'ENOSTR': 100,
      'ENODATA': 116,
      'ETIME': 117,
      'ENOSR': 118,
      'ENONET': 119,
      'ENOPKG': 120,
      'EREMOTE': 121,
      'ENOLINK': 47,
      'EADV': 122,
      'ESRMNT': 123,
      'ECOMM': 124,
      'EPROTO': 65,
      'EMULTIHOP': 36,
      'EDOTDOT': 125,
      'EBADMSG': 9,
      'ENOTUNIQ': 126,
      'EBADFD': 127,
      'EREMCHG': 128,
      'ELIBACC': 129,
      'ELIBBAD': 130,
      'ELIBSCN': 131,
      'ELIBMAX': 132,
      'ELIBEXEC': 133,
      'ENOSYS': 52,
      'ENOTEMPTY': 55,
      'ENAMETOOLONG': 37,
      'ELOOP': 32,
      'EOPNOTSUPP': 138,
      'EPFNOSUPPORT': 139,
      'ECONNRESET': 15,
      'ENOBUFS': 42,
      'EAFNOSUPPORT': 5,
      'EPROTOTYPE': 67,
      'ENOTSOCK': 57,
      'ENOPROTOOPT': 50,
      'ESHUTDOWN': 140,
      'ECONNREFUSED': 14,
      'EADDRINUSE': 3,
      'ECONNABORTED': 13,
      'ENETUNREACH': 40,
      'ENETDOWN': 38,
      'ETIMEDOUT': 73,
      'EHOSTDOWN': 142,
      'EHOSTUNREACH': 23,
      'EINPROGRESS': 26,
      'EALREADY': 7,
      'EDESTADDRREQ': 17,
      'EMSGSIZE': 35,
      'EPROTONOSUPPORT': 66,
      'ESOCKTNOSUPPORT': 137,
      'EADDRNOTAVAIL': 4,
      'ENETRESET': 39,
      'EISCONN': 30,
      'ENOTCONN': 53,
      'ETOOMANYREFS': 141,
      'EUSERS': 136,
      'EDQUOT': 19,
      'ESTALE': 72,
      'ENOTSUP': 138,
      'ENOMEDIUM': 148,
      'EILSEQ': 25,
      'EOVERFLOW': 61,
      'ECANCELED': 11,
      'ENOTRECOVERABLE': 56,
      'EOWNERDEAD': 62,
      'ESTRPIPE': 135,
    };
  
  var asyncLoad = async (url) => {
      var arrayBuffer = await readAsync(url);
      assert(arrayBuffer, `Loading data file "${url}" failed (no arrayBuffer).`);
      return new Uint8Array(arrayBuffer);
    };
  
  
  var FS_createDataFile = (...args) => FS.createDataFile(...args);
  
  var getUniqueRunDependency = (id) => {
      var orig = id;
      while (1) {
        if (!runDependencyTracking[id]) return id;
        id = orig + Math.random();
      }
    };
  
  
  
  var preloadPlugins = [];
  var FS_handledByPreloadPlugin = async (byteArray, fullname) => {
      // Ensure plugins are ready.
      if (typeof Browser != 'undefined') Browser.init();
  
      for (var plugin of preloadPlugins) {
        if (plugin['canHandle'](fullname)) {
          assert(plugin['handle'].constructor.name === 'AsyncFunction', 'Filesystem plugin handlers must be async functions (See #24914)')
          return plugin['handle'](byteArray, fullname);
        }
      }
      // In no plugin handled this file then return the original/unmodified
      // byteArray.
      return byteArray;
    };
  var FS_preloadFile = async (parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish) => {
      // TODO we should allow people to just pass in a complete filename instead
      // of parent and name being that we just join them anyways
      var fullname = name ? PATH_FS.resolve(PATH.join2(parent, name)) : parent;
      var dep = getUniqueRunDependency(`cp ${fullname}`); // might have several active requests for the same fullname
      addRunDependency(dep);
  
      try {
        var byteArray = url;
        if (typeof url == 'string') {
          byteArray = await asyncLoad(url);
        }
  
        byteArray = await FS_handledByPreloadPlugin(byteArray, fullname);
        preFinish?.();
        if (!dontCreateFile) {
          FS_createDataFile(parent, name, byteArray, canRead, canWrite, canOwn);
        }
      } finally {
        removeRunDependency(dep);
      }
    };
  var FS_createPreloadedFile = (parent, name, url, canRead, canWrite, onload, onerror, dontCreateFile, canOwn, preFinish) => {
      FS_preloadFile(parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish).then(onload).catch(onerror);
    };
  var FS = {
  root:null,
  mounts:[],
  devices:{
  },
  streams:[],
  nextInode:1,
  nameTable:null,
  currentPath:"/",
  initialized:false,
  ignorePermissions:true,
  filesystems:null,
  syncFSRequests:0,
  readFiles:{
  },
  ErrnoError:class extends Error {
        name = 'ErrnoError';
        // We set the `name` property to be able to identify `FS.ErrnoError`
        // - the `name` is a standard ECMA-262 property of error objects. Kind of good to have it anyway.
        // - when using PROXYFS, an error can come from an underlying FS
        // as different FS objects have their own FS.ErrnoError each,
        // the test `err instanceof FS.ErrnoError` won't detect an error coming from another filesystem, causing bugs.
        // we'll use the reliable test `err.name == "ErrnoError"` instead
        constructor(errno) {
          super(runtimeInitialized ? strError(errno) : '');
          this.errno = errno;
          for (var key in ERRNO_CODES) {
            if (ERRNO_CODES[key] === errno) {
              this.code = key;
              break;
            }
          }
        }
      },
  FSStream:class {
        shared = {};
        get object() {
          return this.node;
        }
        set object(val) {
          this.node = val;
        }
        get isRead() {
          return (this.flags & 2097155) !== 1;
        }
        get isWrite() {
          return (this.flags & 2097155) !== 0;
        }
        get isAppend() {
          return (this.flags & 1024);
        }
        get flags() {
          return this.shared.flags;
        }
        set flags(val) {
          this.shared.flags = val;
        }
        get position() {
          return this.shared.position;
        }
        set position(val) {
          this.shared.position = val;
        }
      },
  FSNode:class {
        node_ops = {};
        stream_ops = {};
        readMode = 292 | 73;
        writeMode = 146;
        mounted = null;
        constructor(parent, name, mode, rdev) {
          if (!parent) {
            parent = this;  // root node sets parent to itself
          }
          this.parent = parent;
          this.mount = parent.mount;
          this.id = FS.nextInode++;
          this.name = name;
          this.mode = mode;
          this.rdev = rdev;
          this.atime = this.mtime = this.ctime = Date.now();
        }
        get read() {
          return (this.mode & this.readMode) === this.readMode;
        }
        set read(val) {
          val ? this.mode |= this.readMode : this.mode &= ~this.readMode;
        }
        get write() {
          return (this.mode & this.writeMode) === this.writeMode;
        }
        set write(val) {
          val ? this.mode |= this.writeMode : this.mode &= ~this.writeMode;
        }
        get isFolder() {
          return FS.isDir(this.mode);
        }
        get isDevice() {
          return FS.isChrdev(this.mode);
        }
      },
  lookupPath(path, opts = {}) {
        if (!path) {
          throw new FS.ErrnoError(44);
        }
        opts.follow_mount ??= true
  
        if (!PATH.isAbs(path)) {
          path = FS.cwd() + '/' + path;
        }
  
        // limit max consecutive symlinks to 40 (SYMLOOP_MAX).
        linkloop: for (var nlinks = 0; nlinks < 40; nlinks++) {
          // split the absolute path
          var parts = path.split('/').filter((p) => !!p);
  
          // start at the root
          var current = FS.root;
          var current_path = '/';
  
          for (var i = 0; i < parts.length; i++) {
            var islast = (i === parts.length-1);
            if (islast && opts.parent) {
              // stop resolving
              break;
            }
  
            if (parts[i] === '.') {
              continue;
            }
  
            if (parts[i] === '..') {
              current_path = PATH.dirname(current_path);
              if (FS.isRoot(current)) {
                path = current_path + '/' + parts.slice(i + 1).join('/');
                // We're making progress here, don't let many consecutive ..'s
                // lead to ELOOP
                nlinks--;
                continue linkloop;
              } else {
                current = current.parent;
              }
              continue;
            }
  
            current_path = PATH.join2(current_path, parts[i]);
            try {
              current = FS.lookupNode(current, parts[i]);
            } catch (e) {
              // if noent_okay is true, suppress a ENOENT in the last component
              // and return an object with an undefined node. This is needed for
              // resolving symlinks in the path when creating a file.
              if ((e?.errno === 44) && islast && opts.noent_okay) {
                return { path: current_path };
              }
              throw e;
            }
  
            // jump to the mount's root node if this is a mountpoint
            if (FS.isMountpoint(current) && (!islast || opts.follow_mount)) {
              current = current.mounted.root;
            }
  
            // by default, lookupPath will not follow a symlink if it is the final path component.
            // setting opts.follow = true will override this behavior.
            if (FS.isLink(current.mode) && (!islast || opts.follow)) {
              if (!current.node_ops.readlink) {
                throw new FS.ErrnoError(52);
              }
              var link = current.node_ops.readlink(current);
              if (!PATH.isAbs(link)) {
                link = PATH.dirname(current_path) + '/' + link;
              }
              path = link + '/' + parts.slice(i + 1).join('/');
              continue linkloop;
            }
          }
          return { path: current_path, node: current };
        }
        throw new FS.ErrnoError(32);
      },
  getPath(node) {
        var path;
        while (true) {
          if (FS.isRoot(node)) {
            var mount = node.mount.mountpoint;
            if (!path) return mount;
            return mount[mount.length-1] !== '/' ? `${mount}/${path}` : mount + path;
          }
          path = path ? `${node.name}/${path}` : node.name;
          node = node.parent;
        }
      },
  hashName(parentid, name) {
        var hash = 0;
  
        for (var i = 0; i < name.length; i++) {
          hash = ((hash << 5) - hash + name.charCodeAt(i)) | 0;
        }
        return ((parentid + hash) >>> 0) % FS.nameTable.length;
      },
  hashAddNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        node.name_next = FS.nameTable[hash];
        FS.nameTable[hash] = node;
      },
  hashRemoveNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        if (FS.nameTable[hash] === node) {
          FS.nameTable[hash] = node.name_next;
        } else {
          var current = FS.nameTable[hash];
          while (current) {
            if (current.name_next === node) {
              current.name_next = node.name_next;
              break;
            }
            current = current.name_next;
          }
        }
      },
  lookupNode(parent, name) {
        var errCode = FS.mayLookup(parent);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        var hash = FS.hashName(parent.id, name);
        for (var node = FS.nameTable[hash]; node; node = node.name_next) {
          var nodeName = node.name;
          if (node.parent.id === parent.id && nodeName === name) {
            return node;
          }
        }
        // if we failed to find it in the cache, call into the VFS
        return FS.lookup(parent, name);
      },
  createNode(parent, name, mode, rdev) {
        assert(typeof parent == 'object')
        var node = new FS.FSNode(parent, name, mode, rdev);
  
        FS.hashAddNode(node);
  
        return node;
      },
  destroyNode(node) {
        FS.hashRemoveNode(node);
      },
  isRoot(node) {
        return node === node.parent;
      },
  isMountpoint(node) {
        return !!node.mounted;
      },
  isFile(mode) {
        return (mode & 61440) === 32768;
      },
  isDir(mode) {
        return (mode & 61440) === 16384;
      },
  isLink(mode) {
        return (mode & 61440) === 40960;
      },
  isChrdev(mode) {
        return (mode & 61440) === 8192;
      },
  isBlkdev(mode) {
        return (mode & 61440) === 24576;
      },
  isFIFO(mode) {
        return (mode & 61440) === 4096;
      },
  isSocket(mode) {
        return (mode & 49152) === 49152;
      },
  flagsToPermissionString(flag) {
        var perms = ['r', 'w', 'rw'][flag & 3];
        if ((flag & 512)) {
          perms += 'w';
        }
        return perms;
      },
  nodePermissions(node, perms) {
        if (FS.ignorePermissions) {
          return 0;
        }
        // return 0 if any user, group or owner bits are set.
        if (perms.includes('r') && !(node.mode & 292)) {
          return 2;
        } else if (perms.includes('w') && !(node.mode & 146)) {
          return 2;
        } else if (perms.includes('x') && !(node.mode & 73)) {
          return 2;
        }
        return 0;
      },
  mayLookup(dir) {
        if (!FS.isDir(dir.mode)) return 54;
        var errCode = FS.nodePermissions(dir, 'x');
        if (errCode) return errCode;
        if (!dir.node_ops.lookup) return 2;
        return 0;
      },
  mayCreate(dir, name) {
        if (!FS.isDir(dir.mode)) {
          return 54;
        }
        try {
          var node = FS.lookupNode(dir, name);
          return 20;
        } catch (e) {
        }
        return FS.nodePermissions(dir, 'wx');
      },
  mayDelete(dir, name, isdir) {
        var node;
        try {
          node = FS.lookupNode(dir, name);
        } catch (e) {
          return e.errno;
        }
        var errCode = FS.nodePermissions(dir, 'wx');
        if (errCode) {
          return errCode;
        }
        if (isdir) {
          if (!FS.isDir(node.mode)) {
            return 54;
          }
          if (FS.isRoot(node) || FS.getPath(node) === FS.cwd()) {
            return 10;
          }
        } else {
          if (FS.isDir(node.mode)) {
            return 31;
          }
        }
        return 0;
      },
  mayOpen(node, flags) {
        if (!node) {
          return 44;
        }
        if (FS.isLink(node.mode)) {
          return 32;
        } else if (FS.isDir(node.mode)) {
          if (FS.flagsToPermissionString(flags) !== 'r' // opening for write
              || (flags & (512 | 64))) { // TODO: check for O_SEARCH? (== search for dir only)
            return 31;
          }
        }
        return FS.nodePermissions(node, FS.flagsToPermissionString(flags));
      },
  checkOpExists(op, err) {
        if (!op) {
          throw new FS.ErrnoError(err);
        }
        return op;
      },
  MAX_OPEN_FDS:4096,
  nextfd() {
        for (var fd = 0; fd <= FS.MAX_OPEN_FDS; fd++) {
          if (!FS.streams[fd]) {
            return fd;
          }
        }
        throw new FS.ErrnoError(33);
      },
  getStreamChecked(fd) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(8);
        }
        return stream;
      },
  getStream:(fd) => FS.streams[fd],
  createStream(stream, fd = -1) {
        assert(fd >= -1);
  
        // clone it, so we can return an instance of FSStream
        stream = Object.assign(new FS.FSStream(), stream);
        if (fd == -1) {
          fd = FS.nextfd();
        }
        stream.fd = fd;
        FS.streams[fd] = stream;
        return stream;
      },
  closeStream(fd) {
        FS.streams[fd] = null;
      },
  dupStream(origStream, fd = -1) {
        var stream = FS.createStream(origStream, fd);
        stream.stream_ops?.dup?.(stream);
        return stream;
      },
  doSetAttr(stream, node, attr) {
        var setattr = stream?.stream_ops.setattr;
        var arg = setattr ? stream : node;
        setattr ??= node.node_ops.setattr;
        FS.checkOpExists(setattr, 63)
        setattr(arg, attr);
      },
  chrdev_stream_ops:{
  open(stream) {
          var device = FS.getDevice(stream.node.rdev);
          // override node's stream ops with the device's
          stream.stream_ops = device.stream_ops;
          // forward the open call
          stream.stream_ops.open?.(stream);
        },
  llseek() {
          throw new FS.ErrnoError(70);
        },
  },
  major:(dev) => ((dev) >> 8),
  minor:(dev) => ((dev) & 0xff),
  makedev:(ma, mi) => ((ma) << 8 | (mi)),
  registerDevice(dev, ops) {
        FS.devices[dev] = { stream_ops: ops };
      },
  getDevice:(dev) => FS.devices[dev],
  getMounts(mount) {
        var mounts = [];
        var check = [mount];
  
        while (check.length) {
          var m = check.pop();
  
          mounts.push(m);
  
          check.push(...m.mounts);
        }
  
        return mounts;
      },
  syncfs(populate, callback) {
        if (typeof populate == 'function') {
          callback = populate;
          populate = false;
        }
  
        FS.syncFSRequests++;
  
        if (FS.syncFSRequests > 1) {
          err(`warning: ${FS.syncFSRequests} FS.syncfs operations in flight at once, probably just doing extra work`);
        }
  
        var mounts = FS.getMounts(FS.root.mount);
        var completed = 0;
  
        function doCallback(errCode) {
          assert(FS.syncFSRequests > 0);
          FS.syncFSRequests--;
          return callback(errCode);
        }
  
        function done(errCode) {
          if (errCode) {
            if (!done.errored) {
              done.errored = true;
              return doCallback(errCode);
            }
            return;
          }
          if (++completed >= mounts.length) {
            doCallback(null);
          }
        };
  
        // sync all mounts
        for (var mount of mounts) {
          if (mount.type.syncfs) {
            mount.type.syncfs(mount, populate, done);
          } else {
            done(null);
          }
        }
      },
  mount(type, opts, mountpoint) {
        if (typeof type == 'string') {
          // The filesystem was not included, and instead we have an error
          // message stored in the variable.
          throw type;
        }
        var root = mountpoint === '/';
        var pseudo = !mountpoint;
        var node;
  
        if (root && FS.root) {
          throw new FS.ErrnoError(10);
        } else if (!root && !pseudo) {
          var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
          mountpoint = lookup.path;  // use the absolute path
          node = lookup.node;
  
          if (FS.isMountpoint(node)) {
            throw new FS.ErrnoError(10);
          }
  
          if (!FS.isDir(node.mode)) {
            throw new FS.ErrnoError(54);
          }
        }
  
        var mount = {
          type,
          opts,
          mountpoint,
          mounts: []
        };
  
        // create a root node for the fs
        var mountRoot = type.mount(mount);
        mountRoot.mount = mount;
        mount.root = mountRoot;
  
        if (root) {
          FS.root = mountRoot;
        } else if (node) {
          // set as a mountpoint
          node.mounted = mount;
  
          // add the new mount to the current mount's children
          if (node.mount) {
            node.mount.mounts.push(mount);
          }
        }
  
        return mountRoot;
      },
  unmount(mountpoint) {
        var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
        if (!FS.isMountpoint(lookup.node)) {
          throw new FS.ErrnoError(28);
        }
  
        // destroy the nodes for this mount, and all its child mounts
        var node = lookup.node;
        var mount = node.mounted;
        var mounts = FS.getMounts(mount);
  
        for (var [hash, current] of Object.entries(FS.nameTable)) {
          while (current) {
            var next = current.name_next;
  
            if (mounts.includes(current.mount)) {
              FS.destroyNode(current);
            }
  
            current = next;
          }
        }
  
        // no longer a mountpoint
        node.mounted = null;
  
        // remove this mount from the child mounts
        var idx = node.mount.mounts.indexOf(mount);
        assert(idx !== -1);
        node.mount.mounts.splice(idx, 1);
      },
  lookup(parent, name) {
        return parent.node_ops.lookup(parent, name);
      },
  mknod(path, mode, dev) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        if (!name) {
          throw new FS.ErrnoError(28);
        }
        if (name === '.' || name === '..') {
          throw new FS.ErrnoError(20);
        }
        var errCode = FS.mayCreate(parent, name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.mknod) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.mknod(parent, name, mode, dev);
      },
  statfs(path) {
        return FS.statfsNode(FS.lookupPath(path, {follow: true}).node);
      },
  statfsStream(stream) {
        // We keep a separate statfsStream function because noderawfs overrides
        // it. In noderawfs, stream.node is sometimes null. Instead, we need to
        // look at stream.path.
        return FS.statfsNode(stream.node);
      },
  statfsNode(node) {
        // NOTE: None of the defaults here are true. We're just returning safe and
        //       sane values. Currently nodefs and rawfs replace these defaults,
        //       other file systems leave them alone.
        var rtn = {
          bsize: 4096,
          frsize: 4096,
          blocks: 1e6,
          bfree: 5e5,
          bavail: 5e5,
          files: FS.nextInode,
          ffree: FS.nextInode - 1,
          fsid: 42,
          flags: 2,
          namelen: 255,
        };
  
        if (node.node_ops.statfs) {
          Object.assign(rtn, node.node_ops.statfs(node.mount.opts.root));
        }
        return rtn;
      },
  create(path, mode = 0o666) {
        mode &= 4095;
        mode |= 32768;
        return FS.mknod(path, mode, 0);
      },
  mkdir(path, mode = 0o777) {
        mode &= 511 | 512;
        mode |= 16384;
        return FS.mknod(path, mode, 0);
      },
  mkdirTree(path, mode) {
        var dirs = path.split('/');
        var d = '';
        for (var dir of dirs) {
          if (!dir) continue;
          if (d || PATH.isAbs(path)) d += '/';
          d += dir;
          try {
            FS.mkdir(d, mode);
          } catch(e) {
            if (e.errno != 20) throw e;
          }
        }
      },
  mkdev(path, mode, dev) {
        if (typeof dev == 'undefined') {
          dev = mode;
          mode = 0o666;
        }
        mode |= 8192;
        return FS.mknod(path, mode, dev);
      },
  symlink(oldpath, newpath) {
        if (!PATH_FS.resolve(oldpath)) {
          throw new FS.ErrnoError(44);
        }
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var newname = PATH.basename(newpath);
        var errCode = FS.mayCreate(parent, newname);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.symlink) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.symlink(parent, newname, oldpath);
      },
  rename(old_path, new_path) {
        var old_dirname = PATH.dirname(old_path);
        var new_dirname = PATH.dirname(new_path);
        var old_name = PATH.basename(old_path);
        var new_name = PATH.basename(new_path);
        // parents must exist
        var lookup, old_dir, new_dir;
  
        // let the errors from non existent directories percolate up
        lookup = FS.lookupPath(old_path, { parent: true });
        old_dir = lookup.node;
        lookup = FS.lookupPath(new_path, { parent: true });
        new_dir = lookup.node;
  
        if (!old_dir || !new_dir) throw new FS.ErrnoError(44);
        // need to be part of the same mount
        if (old_dir.mount !== new_dir.mount) {
          throw new FS.ErrnoError(75);
        }
        // source must exist
        var old_node = FS.lookupNode(old_dir, old_name);
        // old path should not be an ancestor of the new path
        var relative = PATH_FS.relative(old_path, new_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(28);
        }
        // new path should not be an ancestor of the old path
        relative = PATH_FS.relative(new_path, old_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(55);
        }
        // see if the new path already exists
        var new_node;
        try {
          new_node = FS.lookupNode(new_dir, new_name);
        } catch (e) {
          // not fatal
        }
        // early out if nothing needs to change
        if (old_node === new_node) {
          return;
        }
        // we'll need to delete the old entry
        var isdir = FS.isDir(old_node.mode);
        var errCode = FS.mayDelete(old_dir, old_name, isdir);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        // need delete permissions if we'll be overwriting.
        // need create permissions if new doesn't already exist.
        errCode = new_node ?
          FS.mayDelete(new_dir, new_name, isdir) :
          FS.mayCreate(new_dir, new_name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!old_dir.node_ops.rename) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(old_node) || (new_node && FS.isMountpoint(new_node))) {
          throw new FS.ErrnoError(10);
        }
        // if we are going to change the parent, check write permissions
        if (new_dir !== old_dir) {
          errCode = FS.nodePermissions(old_dir, 'w');
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // remove the node from the lookup hash
        FS.hashRemoveNode(old_node);
        // do the underlying fs rename
        try {
          old_dir.node_ops.rename(old_node, new_dir, new_name);
          // update old node (we do this here to avoid each backend
          // needing to)
          old_node.parent = new_dir;
        } catch (e) {
          throw e;
        } finally {
          // add the node back to the hash (in case node_ops.rename
          // changed its name)
          FS.hashAddNode(old_node);
        }
      },
  rmdir(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, true);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.rmdir) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.rmdir(parent, name);
        FS.destroyNode(node);
      },
  readdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        var readdir = FS.checkOpExists(node.node_ops.readdir, 54);
        return readdir(node);
      },
  unlink(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, false);
        if (errCode) {
          // According to POSIX, we should map EISDIR to EPERM, but
          // we instead do what Linux does (and we must, as we use
          // the musl linux libc).
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.unlink) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.unlink(parent, name);
        FS.destroyNode(node);
      },
  readlink(path) {
        var lookup = FS.lookupPath(path);
        var link = lookup.node;
        if (!link) {
          throw new FS.ErrnoError(44);
        }
        if (!link.node_ops.readlink) {
          throw new FS.ErrnoError(28);
        }
        return link.node_ops.readlink(link);
      },
  stat(path, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        var node = lookup.node;
        var getattr = FS.checkOpExists(node.node_ops.getattr, 63);
        return getattr(node);
      },
  fstat(fd) {
        var stream = FS.getStreamChecked(fd);
        var node = stream.node;
        var getattr = stream.stream_ops.getattr;
        var arg = getattr ? stream : node;
        getattr ??= node.node_ops.getattr;
        FS.checkOpExists(getattr, 63)
        return getattr(arg);
      },
  lstat(path) {
        return FS.stat(path, true);
      },
  doChmod(stream, node, mode, dontFollow) {
        FS.doSetAttr(stream, node, {
          mode: (mode & 4095) | (node.mode & ~4095),
          ctime: Date.now(),
          dontFollow
        });
      },
  chmod(path, mode, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChmod(null, node, mode, dontFollow);
      },
  lchmod(path, mode) {
        FS.chmod(path, mode, true);
      },
  fchmod(fd, mode) {
        var stream = FS.getStreamChecked(fd);
        FS.doChmod(stream, stream.node, mode, false);
      },
  doChown(stream, node, dontFollow) {
        FS.doSetAttr(stream, node, {
          timestamp: Date.now(),
          dontFollow
          // we ignore the uid / gid for now
        });
      },
  chown(path, uid, gid, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChown(null, node, dontFollow);
      },
  lchown(path, uid, gid) {
        FS.chown(path, uid, gid, true);
      },
  fchown(fd, uid, gid) {
        var stream = FS.getStreamChecked(fd);
        FS.doChown(stream, stream.node, false);
      },
  doTruncate(stream, node, len) {
        if (FS.isDir(node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!FS.isFile(node.mode)) {
          throw new FS.ErrnoError(28);
        }
        var errCode = FS.nodePermissions(node, 'w');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.doSetAttr(stream, node, {
          size: len,
          timestamp: Date.now()
        });
      },
  truncate(path, len) {
        if (len < 0) {
          throw new FS.ErrnoError(28);
        }
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: true });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doTruncate(null, node, len);
      },
  ftruncate(fd, len) {
        var stream = FS.getStreamChecked(fd);
        if (len < 0 || (stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(28);
        }
        FS.doTruncate(stream, stream.node, len);
      },
  utime(path, atime, mtime) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        var setattr = FS.checkOpExists(node.node_ops.setattr, 63);
        setattr(node, {
          atime: atime,
          mtime: mtime
        });
      },
  open(path, flags, mode = 0o666) {
        if (path === "") {
          throw new FS.ErrnoError(44);
        }
        flags = typeof flags == 'string' ? FS_modeStringToFlags(flags) : flags;
        if ((flags & 64)) {
          mode = (mode & 4095) | 32768;
        } else {
          mode = 0;
        }
        var node;
        var isDirPath;
        if (typeof path == 'object') {
          node = path;
        } else {
          isDirPath = path.endsWith("/");
          // noent_okay makes it so that if the final component of the path
          // doesn't exist, lookupPath returns `node: undefined`. `path` will be
          // updated to point to the target of all symlinks.
          var lookup = FS.lookupPath(path, {
            follow: !(flags & 131072),
            noent_okay: true
          });
          node = lookup.node;
          path = lookup.path;
        }
        // perhaps we need to create the node
        var created = false;
        if ((flags & 64)) {
          if (node) {
            // if O_CREAT and O_EXCL are set, error out if the node already exists
            if ((flags & 128)) {
              throw new FS.ErrnoError(20);
            }
          } else if (isDirPath) {
            throw new FS.ErrnoError(31);
          } else {
            // node doesn't exist, try to create it
            // Ignore the permission bits here to ensure we can `open` this new
            // file below. We use chmod below the apply the permissions once the
            // file is open.
            node = FS.mknod(path, mode | 0o777, 0);
            created = true;
          }
        }
        if (!node) {
          throw new FS.ErrnoError(44);
        }
        // can't truncate a device
        if (FS.isChrdev(node.mode)) {
          flags &= ~512;
        }
        // if asked only for a directory, then this must be one
        if ((flags & 65536) && !FS.isDir(node.mode)) {
          throw new FS.ErrnoError(54);
        }
        // check permissions, if this is not a file we just created now (it is ok to
        // create and write to a file with read-only permissions; it is read-only
        // for later use)
        if (!created) {
          var errCode = FS.mayOpen(node, flags);
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // do truncation if necessary
        if ((flags & 512) && !created) {
          FS.truncate(node, 0);
        }
        // we've already handled these, don't pass down to the underlying vfs
        flags &= ~(128 | 512 | 131072);
  
        // register the stream with the filesystem
        var stream = FS.createStream({
          node,
          path: FS.getPath(node),  // we want the absolute path to the node
          flags,
          seekable: true,
          position: 0,
          stream_ops: node.stream_ops,
          // used by the file family libc calls (fopen, fwrite, ferror, etc.)
          ungotten: [],
          error: false
        });
        // call the new stream's open function
        if (stream.stream_ops.open) {
          stream.stream_ops.open(stream);
        }
        if (created) {
          FS.chmod(node, mode & 0o777);
        }
        if (Module['logReadFiles'] && !(flags & 1)) {
          if (!(path in FS.readFiles)) {
            FS.readFiles[path] = 1;
          }
        }
        return stream;
      },
  close(stream) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (stream.getdents) stream.getdents = null; // free readdir state
        try {
          if (stream.stream_ops.close) {
            stream.stream_ops.close(stream);
          }
        } catch (e) {
          throw e;
        } finally {
          FS.closeStream(stream.fd);
        }
        stream.fd = null;
      },
  isClosed(stream) {
        return stream.fd === null;
      },
  llseek(stream, offset, whence) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (!stream.seekable || !stream.stream_ops.llseek) {
          throw new FS.ErrnoError(70);
        }
        if (whence != 0 && whence != 1 && whence != 2) {
          throw new FS.ErrnoError(28);
        }
        stream.position = stream.stream_ops.llseek(stream, offset, whence);
        stream.ungotten = [];
        return stream.position;
      },
  read(stream, buffer, offset, length, position) {
        assert(offset >= 0);
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.read) {
          throw new FS.ErrnoError(28);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesRead = stream.stream_ops.read(stream, buffer, offset, length, position);
        if (!seeking) stream.position += bytesRead;
        return bytesRead;
      },
  write(stream, buffer, offset, length, position, canOwn) {
        assert(offset >= 0);
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.write) {
          throw new FS.ErrnoError(28);
        }
        if (stream.seekable && stream.flags & 1024) {
          // seek to the end before writing in append mode
          FS.llseek(stream, 0, 2);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesWritten = stream.stream_ops.write(stream, buffer, offset, length, position, canOwn);
        if (!seeking) stream.position += bytesWritten;
        return bytesWritten;
      },
  mmap(stream, length, position, prot, flags) {
        // User requests writing to file (prot & PROT_WRITE != 0).
        // Checking if we have permissions to write to the file unless
        // MAP_PRIVATE flag is set. According to POSIX spec it is possible
        // to write to file opened in read-only mode with MAP_PRIVATE flag,
        // as all modifications will be visible only in the memory of
        // the current process.
        if ((prot & 2) !== 0
            && (flags & 2) === 0
            && (stream.flags & 2097155) !== 2) {
          throw new FS.ErrnoError(2);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(2);
        }
        if (!stream.stream_ops.mmap) {
          throw new FS.ErrnoError(43);
        }
        if (!length) {
          throw new FS.ErrnoError(28);
        }
        return stream.stream_ops.mmap(stream, length, position, prot, flags);
      },
  msync(stream, buffer, offset, length, mmapFlags) {
        assert(offset >= 0);
        if (!stream.stream_ops.msync) {
          return 0;
        }
        return stream.stream_ops.msync(stream, buffer, offset, length, mmapFlags);
      },
  ioctl(stream, cmd, arg) {
        if (!stream.stream_ops.ioctl) {
          throw new FS.ErrnoError(59);
        }
        return stream.stream_ops.ioctl(stream, cmd, arg);
      },
  readFile(path, opts = {}) {
        opts.flags = opts.flags || 0;
        opts.encoding = opts.encoding || 'binary';
        if (opts.encoding !== 'utf8' && opts.encoding !== 'binary') {
          abort(`Invalid encoding type "${opts.encoding}"`);
        }
        var stream = FS.open(path, opts.flags);
        var stat = FS.stat(path);
        var length = stat.size;
        var buf = new Uint8Array(length);
        FS.read(stream, buf, 0, length, 0);
        if (opts.encoding === 'utf8') {
          buf = UTF8ArrayToString(buf);
        }
        FS.close(stream);
        return buf;
      },
  writeFile(path, data, opts = {}) {
        opts.flags = opts.flags || 577;
        var stream = FS.open(path, opts.flags, opts.mode);
        if (typeof data == 'string') {
          data = new Uint8Array(intArrayFromString(data, true));
        }
        if (ArrayBuffer.isView(data)) {
          FS.write(stream, data, 0, data.byteLength, undefined, opts.canOwn);
        } else {
          abort('Unsupported data type');
        }
        FS.close(stream);
      },
  cwd:() => FS.currentPath,
  chdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        if (lookup.node === null) {
          throw new FS.ErrnoError(44);
        }
        if (!FS.isDir(lookup.node.mode)) {
          throw new FS.ErrnoError(54);
        }
        var errCode = FS.nodePermissions(lookup.node, 'x');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.currentPath = lookup.path;
      },
  createDefaultDirectories() {
        FS.mkdir('/tmp');
        FS.mkdir('/home');
        FS.mkdir('/home/web_user');
      },
  createDefaultDevices() {
        // create /dev
        FS.mkdir('/dev');
        // setup /dev/null
        FS.registerDevice(FS.makedev(1, 3), {
          read: () => 0,
          write: (stream, buffer, offset, length, pos) => length,
          llseek: () => 0,
        });
        FS.mkdev('/dev/null', FS.makedev(1, 3));
        // setup /dev/tty and /dev/tty1
        // stderr needs to print output using err() rather than out()
        // so we register a second tty just for it.
        TTY.register(FS.makedev(5, 0), TTY.default_tty_ops);
        TTY.register(FS.makedev(6, 0), TTY.default_tty1_ops);
        FS.mkdev('/dev/tty', FS.makedev(5, 0));
        FS.mkdev('/dev/tty1', FS.makedev(6, 0));
        // setup /dev/[u]random
        // use a buffer to avoid overhead of individual crypto calls per byte
        var randomBuffer = new Uint8Array(1024), randomLeft = 0;
        var randomByte = () => {
          if (randomLeft === 0) {
            randomFill(randomBuffer);
            randomLeft = randomBuffer.byteLength;
          }
          return randomBuffer[--randomLeft];
        };
        FS.createDevice('/dev', 'random', randomByte);
        FS.createDevice('/dev', 'urandom', randomByte);
        // we're not going to emulate the actual shm device,
        // just create the tmp dirs that reside in it commonly
        FS.mkdir('/dev/shm');
        FS.mkdir('/dev/shm/tmp');
      },
  createSpecialDirectories() {
        // create /proc/self/fd which allows /proc/self/fd/6 => readlink gives the
        // name of the stream for fd 6 (see test_unistd_ttyname)
        FS.mkdir('/proc');
        var proc_self = FS.mkdir('/proc/self');
        FS.mkdir('/proc/self/fd');
        FS.mount({
          mount() {
            var node = FS.createNode(proc_self, 'fd', 16895, 73);
            node.stream_ops = {
              llseek: MEMFS.stream_ops.llseek,
            };
            node.node_ops = {
              lookup(parent, name) {
                var fd = +name;
                var stream = FS.getStreamChecked(fd);
                var ret = {
                  parent: null,
                  mount: { mountpoint: 'fake' },
                  node_ops: { readlink: () => stream.path },
                  id: fd + 1,
                };
                ret.parent = ret; // make it look like a simple root node
                return ret;
              },
              readdir() {
                return Array.from(FS.streams.entries())
                  .filter(([k, v]) => v)
                  .map(([k, v]) => k.toString());
              }
            };
            return node;
          }
        }, {}, '/proc/self/fd');
      },
  createStandardStreams(input, output, error) {
        // TODO deprecate the old functionality of a single
        // input / output callback and that utilizes FS.createDevice
        // and instead require a unique set of stream ops
  
        // by default, we symlink the standard streams to the
        // default tty devices. however, if the standard streams
        // have been overwritten we create a unique device for
        // them instead.
        if (input) {
          FS.createDevice('/dev', 'stdin', input);
        } else {
          FS.symlink('/dev/tty', '/dev/stdin');
        }
        if (output) {
          FS.createDevice('/dev', 'stdout', null, output);
        } else {
          FS.symlink('/dev/tty', '/dev/stdout');
        }
        if (error) {
          FS.createDevice('/dev', 'stderr', null, error);
        } else {
          FS.symlink('/dev/tty1', '/dev/stderr');
        }
  
        // open default streams for the stdin, stdout and stderr devices
        var stdin = FS.open('/dev/stdin', 0);
        var stdout = FS.open('/dev/stdout', 1);
        var stderr = FS.open('/dev/stderr', 1);
        assert(stdin.fd === 0, `invalid handle for stdin (${stdin.fd})`);
        assert(stdout.fd === 1, `invalid handle for stdout (${stdout.fd})`);
        assert(stderr.fd === 2, `invalid handle for stderr (${stderr.fd})`);
      },
  staticInit() {
        FS.nameTable = new Array(4096);
  
        FS.mount(MEMFS, {}, '/');
  
        FS.createDefaultDirectories();
        FS.createDefaultDevices();
        FS.createSpecialDirectories();
  
        FS.filesystems = {
          'MEMFS': MEMFS,
        };
      },
  init(input, output, error) {
        assert(!FS.initialized, 'FS.init was previously called. If you want to initialize later with custom parameters, remove any earlier calls (note that one is automatically added to the generated code)');
        FS.initialized = true;
  
        // Allow Module.stdin etc. to provide defaults, if none explicitly passed to us here
        input ??= Module['stdin'];
        output ??= Module['stdout'];
        error ??= Module['stderr'];
  
        FS.createStandardStreams(input, output, error);
      },
  quit() {
        FS.initialized = false;
        // force-flush all streams, so we get musl std streams printed out
        _fflush(0);
        // close all of our streams
        for (var stream of FS.streams) {
          if (stream) {
            FS.close(stream);
          }
        }
      },
  findObject(path, dontResolveLastLink) {
        var ret = FS.analyzePath(path, dontResolveLastLink);
        if (!ret.exists) {
          return null;
        }
        return ret.object;
      },
  analyzePath(path, dontResolveLastLink) {
        // operate from within the context of the symlink's target
        try {
          var lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          path = lookup.path;
        } catch (e) {
        }
        var ret = {
          isRoot: false, exists: false, error: 0, name: null, path: null, object: null,
          parentExists: false, parentPath: null, parentObject: null
        };
        try {
          var lookup = FS.lookupPath(path, { parent: true });
          ret.parentExists = true;
          ret.parentPath = lookup.path;
          ret.parentObject = lookup.node;
          ret.name = PATH.basename(path);
          lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          ret.exists = true;
          ret.path = lookup.path;
          ret.object = lookup.node;
          ret.name = lookup.node.name;
          ret.isRoot = lookup.path === '/';
        } catch (e) {
          ret.error = e.errno;
        };
        return ret;
      },
  createPath(parent, path, canRead, canWrite) {
        parent = typeof parent == 'string' ? parent : FS.getPath(parent);
        var parts = path.split('/').reverse();
        while (parts.length) {
          var part = parts.pop();
          if (!part) continue;
          var current = PATH.join2(parent, part);
          try {
            FS.mkdir(current);
          } catch (e) {
            if (e.errno != 20) throw e;
          }
          parent = current;
        }
        return current;
      },
  createFile(parent, name, properties, canRead, canWrite) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(canRead, canWrite);
        return FS.create(path, mode);
      },
  createDataFile(parent, name, data, canRead, canWrite, canOwn) {
        var path = name;
        if (parent) {
          parent = typeof parent == 'string' ? parent : FS.getPath(parent);
          path = name ? PATH.join2(parent, name) : parent;
        }
        var mode = FS_getMode(canRead, canWrite);
        var node = FS.create(path, mode);
        if (data) {
          if (typeof data == 'string') {
            var arr = new Array(data.length);
            for (var i = 0, len = data.length; i < len; ++i) arr[i] = data.charCodeAt(i);
            data = arr;
          }
          // make sure we can write to the file
          FS.chmod(node, mode | 146);
          var stream = FS.open(node, 577);
          FS.write(stream, data, 0, data.length, 0, canOwn);
          FS.close(stream);
          FS.chmod(node, mode);
        }
      },
  createDevice(parent, name, input, output) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(!!input, !!output);
        FS.createDevice.major ??= 64;
        var dev = FS.makedev(FS.createDevice.major++, 0);
        // Create a fake device that a set of stream ops to emulate
        // the old behavior.
        FS.registerDevice(dev, {
          open(stream) {
            stream.seekable = false;
          },
          close(stream) {
            // flush any pending line data
            if (output?.buffer?.length) {
              output(10);
            }
          },
          read(stream, buffer, offset, length, pos /* ignored */) {
            var bytesRead = 0;
            for (var i = 0; i < length; i++) {
              var result;
              try {
                result = input();
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
              if (result === undefined && bytesRead === 0) {
                throw new FS.ErrnoError(6);
              }
              if (result === null || result === undefined) break;
              bytesRead++;
              buffer[offset+i] = result;
            }
            if (bytesRead) {
              stream.node.atime = Date.now();
            }
            return bytesRead;
          },
          write(stream, buffer, offset, length, pos) {
            for (var i = 0; i < length; i++) {
              try {
                output(buffer[offset+i]);
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
            }
            if (length) {
              stream.node.mtime = stream.node.ctime = Date.now();
            }
            return i;
          }
        });
        return FS.mkdev(path, mode, dev);
      },
  forceLoadFile(obj) {
        if (obj.isDevice || obj.isFolder || obj.link || obj.contents) return true;
        if (globalThis.XMLHttpRequest) {
          abort("Lazy loading should have been performed (contents set) in createLazyFile, but it was not. Lazy loading only works in web workers. Use --embed-file or --preload-file in emcc on the main thread.");
        } else { // Command-line.
          try {
            obj.contents = readBinary(obj.url);
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
        }
      },
  createLazyFile(parent, name, url, canRead, canWrite) {
        // Lazy chunked Uint8Array (implements get and length from Uint8Array).
        // Actual getting is abstracted away for eventual reuse.
        class LazyUint8Array {
          lengthKnown = false;
          chunks = []; // Loaded chunks. Index is the chunk number
          get(idx) {
            if (idx > this.length-1 || idx < 0) {
              return undefined;
            }
            var chunkOffset = idx % this.chunkSize;
            var chunkNum = (idx / this.chunkSize)|0;
            return this.getter(chunkNum)[chunkOffset];
          }
          setDataGetter(getter) {
            this.getter = getter;
          }
          cacheLength() {
            // Find length
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);
            xhr.send(null);
            if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort("Couldn't load " + url + ". Status: " + xhr.status);
            var datalength = Number(xhr.getResponseHeader("Content-length"));
            var header;
            var hasByteServing = (header = xhr.getResponseHeader("Accept-Ranges")) && header === "bytes";
            var usesGzip = (header = xhr.getResponseHeader("Content-Encoding")) && header === "gzip";
  
            var chunkSize = 1024*1024; // Chunk size in bytes
  
            if (!hasByteServing) chunkSize = datalength;
  
            // Function to get a range from the remote URL.
            var doXHR = (from, to) => {
              if (from > to) abort("invalid range (" + from + ", " + to + ") or no bytes requested!");
              if (to > datalength-1) abort("only " + datalength + " bytes available! programmer error!");
  
              // TODO: Use mozResponseArrayBuffer, responseStream, etc. if available.
              var xhr = new XMLHttpRequest();
              xhr.open('GET', url, false);
              if (datalength !== chunkSize) xhr.setRequestHeader("Range", "bytes=" + from + "-" + to);
  
              // Some hints to the browser that we want binary data.
              xhr.responseType = 'arraybuffer';
              if (xhr.overrideMimeType) {
                xhr.overrideMimeType('text/plain; charset=x-user-defined');
              }
  
              xhr.send(null);
              if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort("Couldn't load " + url + ". Status: " + xhr.status);
              if (xhr.response !== undefined) {
                return new Uint8Array(/** @type{Array<number>} */(xhr.response || []));
              }
              return intArrayFromString(xhr.responseText || '', true);
            };
            var lazyArray = this;
            lazyArray.setDataGetter((chunkNum) => {
              var start = chunkNum * chunkSize;
              var end = (chunkNum+1) * chunkSize - 1; // including this byte
              end = Math.min(end, datalength-1); // if datalength-1 is selected, this is the last block
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') {
                lazyArray.chunks[chunkNum] = doXHR(start, end);
              }
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') abort('doXHR failed!');
              return lazyArray.chunks[chunkNum];
            });
  
            if (usesGzip || !datalength) {
              // if the server uses gzip or doesn't supply the length, we have to download the whole file to get the (uncompressed) length
              chunkSize = datalength = 1; // this will force getter(0)/doXHR do download the whole file
              datalength = this.getter(0).length;
              chunkSize = datalength;
              out("LazyFiles on gzip forces download of the whole file when length is accessed");
            }
  
            this._length = datalength;
            this._chunkSize = chunkSize;
            this.lengthKnown = true;
          }
          get length() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._length;
          }
          get chunkSize() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._chunkSize;
          }
        }
  
        if (globalThis.XMLHttpRequest) {
          if (!ENVIRONMENT_IS_WORKER) abort('Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc');
          var lazyArray = new LazyUint8Array();
          var properties = { isDevice: false, contents: lazyArray };
        } else {
          var properties = { isDevice: false, url: url };
        }
  
        var node = FS.createFile(parent, name, properties, canRead, canWrite);
        // This is a total hack, but I want to get this lazy file code out of the
        // core of MEMFS. If we want to keep this lazy file concept I feel it should
        // be its own thin LAZYFS proxying calls to MEMFS.
        if (properties.contents) {
          node.contents = properties.contents;
        } else if (properties.url) {
          node.contents = null;
          node.url = properties.url;
        }
        // Add a function that defers querying the file size until it is asked the first time.
        Object.defineProperties(node, {
          usedBytes: {
            get: function() { return this.contents.length; }
          }
        });
        // override each stream op with one that tries to force load the lazy file first
        var stream_ops = {};
        for (const [key, fn] of Object.entries(node.stream_ops)) {
          stream_ops[key] = (...args) => {
            FS.forceLoadFile(node);
            return fn(...args);
          };
        }
        function writeChunks(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= contents.length)
            return 0;
          var size = Math.min(contents.length - position, length);
          assert(size >= 0);
          if (contents.slice) { // normal array
            for (var i = 0; i < size; i++) {
              buffer[offset + i] = contents[position + i];
            }
          } else {
            for (var i = 0; i < size; i++) { // LazyUint8Array from sync binary XHR
              buffer[offset + i] = contents.get(position + i);
            }
          }
          return size;
        }
        // use a custom read function
        stream_ops.read = (stream, buffer, offset, length, position) => {
          FS.forceLoadFile(node);
          return writeChunks(stream, buffer, offset, length, position)
        };
        // use a custom mmap function
        stream_ops.mmap = (stream, length, position, prot, flags) => {
          FS.forceLoadFile(node);
          var ptr = mmapAlloc(length);
          if (!ptr) {
            throw new FS.ErrnoError(48);
          }
          writeChunks(stream, HEAP8, ptr, length, position);
          return { ptr, allocated: true };
        };
        node.stream_ops = stream_ops;
        return node;
      },
  absolutePath() {
        abort('FS.absolutePath has been removed; use PATH_FS.resolve instead');
      },
  createFolder() {
        abort('FS.createFolder has been removed; use FS.mkdir instead');
      },
  createLink() {
        abort('FS.createLink has been removed; use FS.symlink instead');
      },
  joinPath() {
        abort('FS.joinPath has been removed; use PATH.join instead');
      },
  mmapAlloc() {
        abort('FS.mmapAlloc has been replaced by the top level function mmapAlloc');
      },
  standardizePath() {
        abort('FS.standardizePath has been removed; use PATH.normalize instead');
      },
  };
  var SOCKFS = {
  websocketArgs:{
  },
  callbacks:{
  },
  on(event, callback) {
        SOCKFS.callbacks[event] = callback;
      },
  emit(event, param) {
        SOCKFS.callbacks[event]?.(param);
      },
  mount(mount) {
        // The incomming Module['websocket'] can be used for configuring 
        // configuring subprotocol/url, etc
        SOCKFS.websocketArgs = Module['websocket'] || {};
        // Add the Event registration mechanism to the exported websocket configuration
        // object so we can register network callbacks from native JavaScript too.
        // For more documentation see system/include/emscripten/emscripten.h
        (Module['websocket'] ??= {})['on'] = SOCKFS.on;
  
        return FS.createNode(null, '/', 16895, 0);
      },
  createSocket(family, type, protocol) {
        // Emscripten only supports AF_INET
        if (family != 2) {
          throw new FS.ErrnoError(5);
        }
        type &= ~526336; // Some applications may pass it; it makes no sense for a single process.
        // Emscripten only supports SOCK_STREAM and SOCK_DGRAM
        if (type != 1 && type != 2) {
          throw new FS.ErrnoError(28);
        }
        var streaming = type == 1;
        if (streaming && protocol && protocol != 6) {
          throw new FS.ErrnoError(66); // if SOCK_STREAM, must be tcp or 0.
        }
  
        // create our internal socket structure
        var sock = {
          family,
          type,
          protocol,
          server: null,
          error: null, // Used in getsockopt for SOL_SOCKET/SO_ERROR test
          peers: {},
          pending: [],
          recv_queue: [],
          sock_ops: SOCKFS.websocket_sock_ops
        };
  
        // create the filesystem node to store the socket structure
        var name = SOCKFS.nextname();
        var node = FS.createNode(SOCKFS.root, name, 49152, 0);
        node.sock = sock;
  
        // and the wrapping stream that enables library functions such
        // as read and write to indirectly interact with the socket
        var stream = FS.createStream({
          path: name,
          node,
          flags: 2,
          seekable: false,
          stream_ops: SOCKFS.stream_ops
        });
  
        // map the new stream to the socket structure (sockets have a 1:1
        // relationship with a stream)
        sock.stream = stream;
  
        return sock;
      },
  getSocket(fd) {
        var stream = FS.getStream(fd);
        if (!stream || !FS.isSocket(stream.node.mode)) {
          return null;
        }
        return stream.node.sock;
      },
  stream_ops:{
  poll(stream) {
          var sock = stream.node.sock;
          return sock.sock_ops.poll(sock);
        },
  ioctl(stream, request, varargs) {
          var sock = stream.node.sock;
          return sock.sock_ops.ioctl(sock, request, varargs);
        },
  read(stream, buffer, offset, length, position /* ignored */) {
          var sock = stream.node.sock;
          var msg = sock.sock_ops.recvmsg(sock, length);
          if (!msg) {
            // socket is closed
            return 0;
          }
          buffer.set(msg.buffer, offset);
          return msg.buffer.length;
        },
  write(stream, buffer, offset, length, position /* ignored */) {
          var sock = stream.node.sock;
          return sock.sock_ops.sendmsg(sock, buffer, offset, length);
        },
  close(stream) {
          var sock = stream.node.sock;
          sock.sock_ops.close(sock);
        },
  },
  nextname() {
        if (!SOCKFS.nextname.current) {
          SOCKFS.nextname.current = 0;
        }
        return `socket[${SOCKFS.nextname.current++}]`;
      },
  websocket_sock_ops:{
  createPeer(sock, addr, port) {
          var ws;
  
          if (typeof addr == 'object') {
            ws = addr;
            addr = null;
            port = null;
          }
  
          if (ws) {
            // for sockets that've already connected (e.g. we're the server)
            // we can inspect the _socket property for the address
            if (ws._socket) {
              addr = ws._socket.remoteAddress;
              port = ws._socket.remotePort;
            }
            // if we're just now initializing a connection to the remote,
            // inspect the url property
            else {
              var result = /ws[s]?:\/\/([^:]+):(\d+)/.exec(ws.url);
              if (!result) {
                throw new Error('WebSocket URL must be in the format ws(s)://address:port');
              }
              addr = result[1];
              port = parseInt(result[2], 10);
            }
          } else {
            // create the actual websocket object and connect
            try {
              // The default value is 'ws://' the replace is needed because the compiler replaces '//' comments with '#'
              // comments without checking context, so we'd end up with ws:#, the replace swaps the '#' for '//' again.
              var url = 'ws://'.replace('#', '//');
              // Make the WebSocket subprotocol (Sec-WebSocket-Protocol) default to binary if no configuration is set.
              var subProtocols = 'binary'; // The default value is 'binary'
              // The default WebSocket options
              var opts = undefined;
  
              // Fetch runtime WebSocket URL config.
              if (SOCKFS.websocketArgs['url']) {
                url = SOCKFS.websocketArgs['url'];
              }
              // Fetch runtime WebSocket subprotocol config.
              if (SOCKFS.websocketArgs['subprotocol']) {
                subProtocols = SOCKFS.websocketArgs['subprotocol'];
              } else if (SOCKFS.websocketArgs['subprotocol'] === null) {
                subProtocols = 'null'
              }
  
              if (url === 'ws://' || url === 'wss://') { // Is the supplied URL config just a prefix, if so complete it.
                var parts = addr.split('/');
                url = url + parts[0] + ":" + port + "/" + parts.slice(1).join('/');
              }
  
              if (subProtocols !== 'null') {
                // The regex trims the string (removes spaces at the beginning and end, then splits the string by
                // <any space>,<any space> into an Array. Whitespace removal is important for Websockify and ws.
                subProtocols = subProtocols.replace(/^ +| +$/g,"").split(/ *, */);
  
                opts = subProtocols;
              }
  
              // If node we use the ws library.
              var WebSocketConstructor;
              {
                WebSocketConstructor = WebSocket;
              }
              ws = new WebSocketConstructor(url, opts);
              ws.binaryType = 'arraybuffer';
            } catch (e) {
              throw new FS.ErrnoError(23);
            }
          }
  
          var peer = {
            addr,
            port,
            socket: ws,
            msg_send_queue: []
          };
  
          SOCKFS.websocket_sock_ops.addPeer(sock, peer);
          SOCKFS.websocket_sock_ops.handlePeerEvents(sock, peer);
  
          // if this is a bound dgram socket, send the port number first to allow
          // us to override the ephemeral port reported to us by remotePort on the
          // remote end.
          if (sock.type === 2 && typeof sock.sport != 'undefined') {
            peer.msg_send_queue.push(new Uint8Array([
                255, 255, 255, 255,
                'p'.charCodeAt(0), 'o'.charCodeAt(0), 'r'.charCodeAt(0), 't'.charCodeAt(0),
                ((sock.sport & 0xff00) >> 8) , (sock.sport & 0xff)
            ]));
          }
  
          return peer;
        },
  getPeer(sock, addr, port) {
          return sock.peers[addr + ':' + port];
        },
  addPeer(sock, peer) {
          sock.peers[peer.addr + ':' + peer.port] = peer;
        },
  removePeer(sock, peer) {
          delete sock.peers[peer.addr + ':' + peer.port];
        },
  handlePeerEvents(sock, peer) {
          var first = true;
  
          var handleOpen = function () {
  
            sock.connecting = false;
            SOCKFS.emit('open', sock.stream.fd);
  
            try {
              var queued = peer.msg_send_queue.shift();
              while (queued) {
                peer.socket.send(queued);
                queued = peer.msg_send_queue.shift();
              }
            } catch (e) {
              // not much we can do here in the way of proper error handling as we've already
              // lied and said this data was sent. shut it down.
              peer.socket.close();
            }
          };
  
          function handleMessage(data) {
            if (typeof data == 'string') {
              var encoder = new TextEncoder(); // should be utf-8
              data = encoder.encode(data); // make a typed array from the string
            } else {
              assert(data.byteLength !== undefined); // must receive an ArrayBuffer
              if (data.byteLength == 0) {
                // An empty ArrayBuffer will emit a pseudo disconnect event
                // as recv/recvmsg will return zero which indicates that a socket
                // has performed a shutdown although the connection has not been disconnected yet.
                return;
              }
              data = new Uint8Array(data); // make a typed array view on the array buffer
            }
  
            // if this is the port message, override the peer's port with it
            var wasfirst = first;
            first = false;
            if (wasfirst &&
                data.length === 10 &&
                data[0] === 255 && data[1] === 255 && data[2] === 255 && data[3] === 255 &&
                data[4] === 'p'.charCodeAt(0) && data[5] === 'o'.charCodeAt(0) && data[6] === 'r'.charCodeAt(0) && data[7] === 't'.charCodeAt(0)) {
              // update the peer's port and it's key in the peer map
              var newport = ((data[8] << 8) | data[9]);
              SOCKFS.websocket_sock_ops.removePeer(sock, peer);
              peer.port = newport;
              SOCKFS.websocket_sock_ops.addPeer(sock, peer);
              return;
            }
  
            sock.recv_queue.push({ addr: peer.addr, port: peer.port, data: data });
            SOCKFS.emit('message', sock.stream.fd);
          };
  
          if (ENVIRONMENT_IS_NODE) {
            peer.socket.on('open', handleOpen);
            peer.socket.on('message', function(data, isBinary) {
              if (!isBinary) {
                return;
              }
              handleMessage((new Uint8Array(data)).buffer); // copy from node Buffer -> ArrayBuffer
            });
            peer.socket.on('close', function() {
              SOCKFS.emit('close', sock.stream.fd);
            });
            peer.socket.on('error', function(error) {
              // Although the ws library may pass errors that may be more descriptive than
              // ECONNREFUSED they are not necessarily the expected error code e.g.
              // ENOTFOUND on getaddrinfo seems to be node.js specific, so using ECONNREFUSED
              // is still probably the most useful thing to do.
              sock.error = 14; // Used in getsockopt for SOL_SOCKET/SO_ERROR test.
              SOCKFS.emit('error', [sock.stream.fd, sock.error, 'ECONNREFUSED: Connection refused']);
              // don't throw
            });
          } else {
            peer.socket.onopen = handleOpen;
            peer.socket.onclose = function() {
              SOCKFS.emit('close', sock.stream.fd);
            };
            peer.socket.onmessage = function peer_socket_onmessage(event) {
              handleMessage(event.data);
            };
            peer.socket.onerror = function(error) {
              // The WebSocket spec only allows a 'simple event' to be thrown on error,
              // so we only really know as much as ECONNREFUSED.
              sock.error = 14; // Used in getsockopt for SOL_SOCKET/SO_ERROR test.
              SOCKFS.emit('error', [sock.stream.fd, sock.error, 'ECONNREFUSED: Connection refused']);
            };
          }
        },
  poll(sock) {
          if (sock.type === 1 && sock.server) {
            // listen sockets should only say they're available for reading
            // if there are pending clients.
            return sock.pending.length ? (64 | 1) : 0;
          }
  
          var mask = 0;
          var dest = sock.type === 1 ?  // we only care about the socket state for connection-based sockets
            SOCKFS.websocket_sock_ops.getPeer(sock, sock.daddr, sock.dport) :
            null;
  
          if (sock.recv_queue.length ||
              !dest ||  // connection-less sockets are always ready to read
              (dest && dest.socket.readyState === dest.socket.CLOSING) ||
              (dest && dest.socket.readyState === dest.socket.CLOSED)) {  // let recv return 0 once closed
            mask |= (64 | 1);
          }
  
          if (!dest ||  // connection-less sockets are always ready to write
              (dest && dest.socket.readyState === dest.socket.OPEN)) {
            mask |= 4;
          }
  
          if ((dest && dest.socket.readyState === dest.socket.CLOSING) ||
              (dest && dest.socket.readyState === dest.socket.CLOSED)) {
            // When an non-blocking connect fails mark the socket as writable.
            // Its up to the calling code to then use getsockopt with SO_ERROR to
            // retrieve the error.
            // See https://man7.org/linux/man-pages/man2/connect.2.html
            if (sock.connecting) {
              mask |= 4;
            } else  {
              mask |= 16;
            }
          }
  
          return mask;
        },
  ioctl(sock, request, arg) {
          switch (request) {
            case 21531:
              var bytes = 0;
              if (sock.recv_queue.length) {
                bytes = sock.recv_queue[0].data.length;
              }
              HEAP32[((arg)/4)] = bytes;
              return 0;
            case 21537:
              var on = HEAP32[((arg)/4)];
              if (on) {
                sock.stream.flags |= 2048;
              } else {
                sock.stream.flags &= ~2048;
              }
              return 0;
            default:
              return 28;
          }
        },
  close(sock) {
          // if we've spawned a listen server, close it
          if (sock.server) {
            try {
              sock.server.close();
            } catch (e) {
            }
            sock.server = null;
          }
          // close any peer connections
          for (var peer of Object.values(sock.peers)) {
            try {
              peer.socket.close();
            } catch (e) {
            }
            SOCKFS.websocket_sock_ops.removePeer(sock, peer);
          }
          return 0;
        },
  bind(sock, addr, port) {
          if (typeof sock.saddr != 'undefined' || typeof sock.sport != 'undefined') {
            throw new FS.ErrnoError(28);  // already bound
          }
          sock.saddr = addr;
          sock.sport = port;
          // in order to emulate dgram sockets, we need to launch a listen server when
          // binding on a connection-less socket
          // note: this is only required on the server side
          if (sock.type === 2) {
            // close the existing server if it exists
            if (sock.server) {
              sock.server.close();
              sock.server = null;
            }
            // swallow error operation not supported error that occurs when binding in the
            // browser where this isn't supported
            try {
              sock.sock_ops.listen(sock, 0);
            } catch (e) {
              if (!(e.name === 'ErrnoError')) throw e;
              if (e.errno !== 138) throw e;
            }
          }
        },
  connect(sock, addr, port) {
          if (sock.server) {
            throw new FS.ErrnoError(138);
          }
  
          // TODO autobind
          // if (!sock.addr && sock.type == 2) {
          // }
  
          // early out if we're already connected / in the middle of connecting
          if (typeof sock.daddr != 'undefined' && typeof sock.dport != 'undefined') {
            var dest = SOCKFS.websocket_sock_ops.getPeer(sock, sock.daddr, sock.dport);
            if (dest) {
              if (dest.socket.readyState === dest.socket.CONNECTING) {
                throw new FS.ErrnoError(7);
              } else {
                throw new FS.ErrnoError(30);
              }
            }
          }
  
          // add the socket to our peer list and set our
          // destination address / port to match
          var peer = SOCKFS.websocket_sock_ops.createPeer(sock, addr, port);
          sock.daddr = peer.addr;
          sock.dport = peer.port;
  
          // because we cannot synchronously block to wait for the WebSocket
          // connection to complete, we return here pretending that the connection
          // was a success.
          sock.connecting = true;
        },
  listen(sock, backlog) {
          if (!ENVIRONMENT_IS_NODE) {
            throw new FS.ErrnoError(138);
          }
        },
  accept(listensock) {
          if (!listensock.server || !listensock.pending.length) {
            throw new FS.ErrnoError(28);
          }
          var newsock = listensock.pending.shift();
          newsock.stream.flags = listensock.stream.flags;
          return newsock;
        },
  getname(sock, peer) {
          var addr, port;
          if (peer) {
            if (sock.daddr === undefined || sock.dport === undefined) {
              throw new FS.ErrnoError(53);
            }
            addr = sock.daddr;
            port = sock.dport;
          } else {
            // TODO saddr and sport will be set for bind()'d UDP sockets, but what
            // should we be returning for TCP sockets that've been connect()'d?
            addr = sock.saddr || 0;
            port = sock.sport || 0;
          }
          return { addr, port };
        },
  sendmsg(sock, buffer, offset, length, addr, port) {
          if (sock.type === 2) {
            // connection-less sockets will honor the message address,
            // and otherwise fall back to the bound destination address
            if (addr === undefined || port === undefined) {
              addr = sock.daddr;
              port = sock.dport;
            }
            // if there was no address to fall back to, error out
            if (addr === undefined || port === undefined) {
              throw new FS.ErrnoError(17);
            }
          } else {
            // connection-based sockets will only use the bound
            addr = sock.daddr;
            port = sock.dport;
          }
  
          // find the peer for the destination address
          var dest = SOCKFS.websocket_sock_ops.getPeer(sock, addr, port);
  
          // early out if not connected with a connection-based socket
          if (sock.type === 1) {
            if (!dest || dest.socket.readyState === dest.socket.CLOSING || dest.socket.readyState === dest.socket.CLOSED) {
              throw new FS.ErrnoError(53);
            }
          }
  
          // create a copy of the incoming data to send, as the WebSocket API
          // doesn't work entirely with an ArrayBufferView, it'll just send
          // the entire underlying buffer
          if (ArrayBuffer.isView(buffer)) {
            offset += buffer.byteOffset;
            buffer = buffer.buffer;
          }
  
          var data = buffer.slice(offset, offset + length);
  
          // if we don't have a cached connectionless UDP datagram connection, or
          // the TCP socket is still connecting, queue the message to be sent upon
          // connect, and lie, saying the data was sent now.
          if (!dest || dest.socket.readyState !== dest.socket.OPEN) {
            // if we're not connected, open a new connection
            if (sock.type === 2) {
              if (!dest || dest.socket.readyState === dest.socket.CLOSING || dest.socket.readyState === dest.socket.CLOSED) {
                dest = SOCKFS.websocket_sock_ops.createPeer(sock, addr, port);
              }
            }
            dest.msg_send_queue.push(data);
            return length;
          }
  
          try {
            // send the actual data
            dest.socket.send(data);
            return length;
          } catch (e) {
            throw new FS.ErrnoError(28);
          }
        },
  recvmsg(sock, length) {
          // http://pubs.opengroup.org/onlinepubs/7908799/xns/recvmsg.html
          if (sock.type === 1 && sock.server) {
            // tcp servers should not be recv()'ing on the listen socket
            throw new FS.ErrnoError(53);
          }
  
          var queued = sock.recv_queue.shift();
          if (!queued) {
            if (sock.type === 1) {
              var dest = SOCKFS.websocket_sock_ops.getPeer(sock, sock.daddr, sock.dport);
  
              if (!dest) {
                // if we have a destination address but are not connected, error out
                throw new FS.ErrnoError(53);
              }
              if (dest.socket.readyState === dest.socket.CLOSING || dest.socket.readyState === dest.socket.CLOSED) {
                // return null if the socket has closed
                return null;
              }
              // else, our socket is in a valid state but truly has nothing available
              throw new FS.ErrnoError(6);
            }
            throw new FS.ErrnoError(6);
          }
  
          // queued.data will be an ArrayBuffer if it's unadulterated, but if it's
          // requeued TCP data it'll be an ArrayBufferView
          var queuedLength = queued.data.byteLength || queued.data.length;
          var queuedOffset = queued.data.byteOffset || 0;
          var queuedBuffer = queued.data.buffer || queued.data;
          var bytesRead = Math.min(length, queuedLength);
          var res = {
            buffer: new Uint8Array(queuedBuffer, queuedOffset, bytesRead),
            addr: queued.addr,
            port: queued.port
          };
  
          // push back any unread data for TCP connections
          if (sock.type === 1 && bytesRead < queuedLength) {
            var bytesRemaining = queuedLength - bytesRead;
            queued.data = new Uint8Array(queuedBuffer, queuedOffset + bytesRead, bytesRemaining);
            sock.recv_queue.unshift(queued);
          }
  
          return res;
        },
  },
  };
  
  var getSocketFromFD = (fd) => {
      var socket = SOCKFS.getSocket(fd);
      if (!socket) throw new FS.ErrnoError(8);
      return socket;
    };
  
  var inetPton4 = (str) => {
      var b = str.split('.');
      for (var i = 0; i < 4; i++) {
        var tmp = Number(b[i]);
        if (isNaN(tmp)) return null;
        b[i] = tmp;
      }
      return (b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)) >>> 0;
    };
  
  var inetPton6 = (str) => {
      var words;
      var w, offset, z, i;
      /* http://home.deds.nl/~aeron/regex/ */
      var valid6regx = /^((?=.*::)(?!.*::.+::)(::)?([\dA-F]{1,4}:(:|\b)|){5}|([\dA-F]{1,4}:){6})((([\dA-F]{1,4}((?!\3)::|:\b|$))|(?!\2\3)){2}|(((2[0-4]|1\d|[1-9])?\d|25[0-5])\.?\b){4})$/i
      var parts = [];
      if (!valid6regx.test(str)) {
        return null;
      }
      if (str === "::") {
        return [0, 0, 0, 0, 0, 0, 0, 0];
      }
      // Z placeholder to keep track of zeros when splitting the string on ":"
      if (str.startsWith("::")) {
        str = str.replace("::", "Z:"); // leading zeros case
      } else {
        str = str.replace("::", ":Z:");
      }
  
      if (str.indexOf(".") > 0) {
        // parse IPv4 embedded stress
        str = str.replace(new RegExp('[.]', 'g'), ":");
        words = str.split(":");
        words[words.length-4] = Number(words[words.length-4]) + Number(words[words.length-3])*256;
        words[words.length-3] = Number(words[words.length-2]) + Number(words[words.length-1])*256;
        words = words.slice(0, words.length-2);
      } else {
        words = str.split(":");
      }
  
      offset = 0; z = 0;
      for (w=0; w < words.length; w++) {
        if (typeof words[w] == 'string') {
          if (words[w] === 'Z') {
            // compressed zeros - write appropriate number of zero words
            for (z = 0; z < (8 - words.length+1); z++) {
              parts[w+z] = 0;
            }
            offset = z-1;
          } else {
            // parse hex to field to 16-bit value and write it in network byte-order
            parts[w+offset] = _htons(parseInt(words[w],16));
          }
        } else {
          // parsed IPv4 words
          parts[w+offset] = words[w];
        }
      }
      return [
        (parts[1] << 16) | parts[0],
        (parts[3] << 16) | parts[2],
        (parts[5] << 16) | parts[4],
        (parts[7] << 16) | parts[6]
      ];
    };
  
  var zeroMemory = (ptr, size) => HEAPU8.fill(0, ptr, ptr + size);
  
  /** @param {number=} addrlen */
  var writeSockaddr = (sa, family, addr, port, addrlen) => {
      switch (family) {
        case 2:
          addr = inetPton4(addr);
          zeroMemory(sa, 16);
          if (addrlen) {
            HEAP32[((addrlen)/4)] = 16;
          }
          HEAP16[((sa)/2)] = family;
          HEAP32[(((sa)+(4))/4)] = addr;
          HEAP16[(((sa)+(2))/2)] = _htons(port);
          break;
        case 10:
          addr = inetPton6(addr);
          zeroMemory(sa, 28);
          if (addrlen) {
            HEAP32[((addrlen)/4)] = 28;
          }
          HEAP32[((sa)/4)] = family;
          HEAP32[(((sa)+(8))/4)] = addr[0];
          HEAP32[(((sa)+(12))/4)] = addr[1];
          HEAP32[(((sa)+(16))/4)] = addr[2];
          HEAP32[(((sa)+(20))/4)] = addr[3];
          HEAP16[(((sa)+(2))/2)] = _htons(port);
          break;
        default:
          return 5;
      }
      return 0;
    };
  
  
  var DNS = {
  address_map:{
  id:1,
  addrs:{
  },
  names:{
  },
  },
  lookup_name(name) {
        // If the name is already a valid ipv4 / ipv6 address, don't generate a fake one.
        var res = inetPton4(name);
        if (res !== null) {
          return name;
        }
        res = inetPton6(name);
        if (res !== null) {
          return name;
        }
  
        // See if this name is already mapped.
        var addr;
  
        if (DNS.address_map.addrs[name]) {
          addr = DNS.address_map.addrs[name];
        } else {
          var id = DNS.address_map.id++;
          assert(id < 65535, 'exceeded max address mappings of 65535');
  
          addr = '172.29.' + (id & 0xff) + '.' + (id & 0xff00);
  
          DNS.address_map.names[addr] = name;
          DNS.address_map.addrs[name] = addr;
        }
  
        return addr;
      },
  lookup_addr(addr) {
        if (DNS.address_map.names[addr]) {
          return DNS.address_map.names[addr];
        }
  
        return null;
      },
  };
  
  var INT53_MAX = 9007199254740992;
  
  var INT53_MIN = -9007199254740992;
  var bigintToI53Checked = (num) => (num < INT53_MIN || num > INT53_MAX) ? NaN : Number(num);
  function ___syscall_accept4(fd, addr, addrlen, flags, d1, d2) {
    addr = bigintToI53Checked(addr);
    addrlen = bigintToI53Checked(addrlen);
  
  
  try {
  
      var sock = getSocketFromFD(fd);
      var newsock = sock.sock_ops.accept(sock);
      if (addr) {
        var errno = writeSockaddr(addr, newsock.family, DNS.lookup_name(newsock.daddr), newsock.dport, addrlen);
        assert(!errno);
      }
      return newsock.stream.fd;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  var inetNtop4 = (addr) =>
      (addr & 0xff) + '.' + ((addr >> 8) & 0xff) + '.' + ((addr >> 16) & 0xff) + '.' + ((addr >> 24) & 0xff);
  
  
  var inetNtop6 = (ints) => {
      //  ref:  http://www.ietf.org/rfc/rfc2373.txt - section 2.5.4
      //  Format for IPv4 compatible and mapped  128-bit IPv6 Addresses
      //  128-bits are split into eight 16-bit words
      //  stored in network byte order (big-endian)
      //  |                80 bits               | 16 |      32 bits        |
      //  +-----------------------------------------------------------------+
      //  |               10 bytes               |  2 |      4 bytes        |
      //  +--------------------------------------+--------------------------+
      //  +               5 words                |  1 |      2 words        |
      //  +--------------------------------------+--------------------------+
      //  |0000..............................0000|0000|    IPv4 ADDRESS     | (compatible)
      //  +--------------------------------------+----+---------------------+
      //  |0000..............................0000|FFFF|    IPv4 ADDRESS     | (mapped)
      //  +--------------------------------------+----+---------------------+
      var str = "";
      var word = 0;
      var longest = 0;
      var lastzero = 0;
      var zstart = 0;
      var len = 0;
      var i = 0;
      var parts = [
        ints[0] & 0xffff,
        (ints[0] >> 16),
        ints[1] & 0xffff,
        (ints[1] >> 16),
        ints[2] & 0xffff,
        (ints[2] >> 16),
        ints[3] & 0xffff,
        (ints[3] >> 16)
      ];
  
      // Handle IPv4-compatible, IPv4-mapped, loopback and any/unspecified addresses
  
      var hasipv4 = true;
      var v4part = "";
      // check if the 10 high-order bytes are all zeros (first 5 words)
      for (i = 0; i < 5; i++) {
        if (parts[i] !== 0) { hasipv4 = false; break; }
      }
  
      if (hasipv4) {
        // low-order 32-bits store an IPv4 address (bytes 13 to 16) (last 2 words)
        v4part = inetNtop4(parts[6] | (parts[7] << 16));
        // IPv4-mapped IPv6 address if 16-bit value (bytes 11 and 12) == 0xFFFF (6th word)
        if (parts[5] === -1) {
          str = "::ffff:";
          str += v4part;
          return str;
        }
        // IPv4-compatible IPv6 address if 16-bit value (bytes 11 and 12) == 0x0000 (6th word)
        if (parts[5] === 0) {
          str = "::";
          //special case IPv6 addresses
          if (v4part === "0.0.0.0") v4part = ""; // any/unspecified address
          if (v4part === "0.0.0.1") v4part = "1";// loopback address
          str += v4part;
          return str;
        }
      }
  
      // Handle all other IPv6 addresses
  
      // first run to find the longest contiguous zero words
      for (word = 0; word < 8; word++) {
        if (parts[word] === 0) {
          if (word - lastzero > 1) {
            len = 0;
          }
          lastzero = word;
          len++;
        }
        if (len > longest) {
          longest = len;
          zstart = word - longest + 1;
        }
      }
  
      for (word = 0; word < 8; word++) {
        if (longest > 1) {
          // compress contiguous zeros - to produce "::"
          if (parts[word] === 0 && word >= zstart && word < (zstart + longest) ) {
            if (word === zstart) {
              str += ":";
              if (zstart === 0) str += ":"; //leading zeros case
            }
            continue;
          }
        }
        // converts 16-bit words from big-endian to little-endian before converting to hex string
        str += Number(_ntohs(parts[word] & 0xffff)).toString(16);
        str += word < 7 ? ":" : "";
      }
      return str;
    };
  
  var readSockaddr = (sa, salen) => {
      // family / port offsets are common to both sockaddr_in and sockaddr_in6
      var family = HEAP16[((sa)/2)];
      var port = _ntohs(HEAPU16[(((sa)+(2))/2)]);
      var addr;
  
      switch (family) {
        case 2:
          if (salen !== 16) {
            return { errno: 28 };
          }
          addr = HEAP32[(((sa)+(4))/4)];
          addr = inetNtop4(addr);
          break;
        case 10:
          if (salen !== 28) {
            return { errno: 28 };
          }
          addr = [
            HEAP32[(((sa)+(8))/4)],
            HEAP32[(((sa)+(12))/4)],
            HEAP32[(((sa)+(16))/4)],
            HEAP32[(((sa)+(20))/4)]
          ];
          addr = inetNtop6(addr);
          break;
        default:
          return { errno: 5 };
      }
  
      return { family: family, addr: addr, port: port };
    };
  
  
  var getSocketAddress = (addrp, addrlen) => {
      var info = readSockaddr(addrp, addrlen);
      if (info.errno) throw new FS.ErrnoError(info.errno);
      info.addr = DNS.lookup_addr(info.addr) || info.addr;
      return info;
    };
  
  function ___syscall_bind(fd, addr, addrlen, d1, d2, d3) {
    addr = bigintToI53Checked(addr);
    addrlen = bigintToI53Checked(addrlen);
  
  
  try {
  
      var sock = getSocketFromFD(fd);
      var info = getSocketAddress(addr, addrlen);
      sock.sock_ops.bind(sock, info.addr, info.port);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  var SYSCALLS = {
  DEFAULT_POLLMASK:5,
  calculateAt(dirfd, path, allowEmpty) {
        if (PATH.isAbs(path)) {
          return path;
        }
        // relative path
        var dir;
        if (dirfd === -100) {
          dir = FS.cwd();
        } else {
          var dirstream = SYSCALLS.getStreamFromFD(dirfd);
          dir = dirstream.path;
        }
        if (path.length == 0) {
          if (!allowEmpty) {
            throw new FS.ErrnoError(44);;
          }
          return dir;
        }
        return dir + '/' + path;
      },
  writeStat(buf, stat) {
        HEAPU32[((buf)/4)] = stat.dev;
        HEAPU32[(((buf)+(4))/4)] = stat.mode;
        HEAPU64[(((buf)+(8))/8)] = BigInt(stat.nlink);
        HEAPU32[(((buf)+(16))/4)] = stat.uid;
        HEAPU32[(((buf)+(20))/4)] = stat.gid;
        HEAPU32[(((buf)+(24))/4)] = stat.rdev;
        HEAP64[(((buf)+(32))/8)] = BigInt(stat.size);
        HEAP32[(((buf)+(40))/4)] = 4096;
        HEAP32[(((buf)+(44))/4)] = stat.blocks;
        var atime = stat.atime.getTime();
        var mtime = stat.mtime.getTime();
        var ctime = stat.ctime.getTime();
        HEAP64[(((buf)+(48))/8)] = BigInt(Math.floor(atime / 1000));
        HEAPU64[(((buf)+(56))/8)] = BigInt((atime % 1000) * 1000 * 1000);
        HEAP64[(((buf)+(64))/8)] = BigInt(Math.floor(mtime / 1000));
        HEAPU64[(((buf)+(72))/8)] = BigInt((mtime % 1000) * 1000 * 1000);
        HEAP64[(((buf)+(80))/8)] = BigInt(Math.floor(ctime / 1000));
        HEAPU64[(((buf)+(88))/8)] = BigInt((ctime % 1000) * 1000 * 1000);
        HEAP64[(((buf)+(96))/8)] = BigInt(stat.ino);
        return 0;
      },
  writeStatFs(buf, stats) {
        HEAPU32[(((buf)+(8))/4)] = stats.bsize;
        HEAPU32[(((buf)+(72))/4)] = stats.bsize;
        HEAP64[(((buf)+(16))/8)] = BigInt(stats.blocks);
        HEAP64[(((buf)+(24))/8)] = BigInt(stats.bfree);
        HEAP64[(((buf)+(32))/8)] = BigInt(stats.bavail);
        HEAP64[(((buf)+(40))/8)] = BigInt(stats.files);
        HEAP64[(((buf)+(48))/8)] = BigInt(stats.ffree);
        HEAPU32[(((buf)+(56))/4)] = stats.fsid;
        HEAPU32[(((buf)+(80))/4)] = stats.flags;  // ST_NOSUID
        HEAPU32[(((buf)+(64))/4)] = stats.namelen;
      },
  doMsync(addr, stream, len, flags, offset) {
        if (!FS.isFile(stream.node.mode)) {
          throw new FS.ErrnoError(43);
        }
        if (flags & 2) {
          // MAP_PRIVATE calls need not to be synced back to underlying fs
          return 0;
        }
        var buffer = HEAPU8.slice(addr, addr + len);
        FS.msync(stream, buffer, offset, len, flags);
      },
  getStreamFromFD(fd) {
        var stream = FS.getStreamChecked(fd);
        return stream;
      },
  varargs:undefined,
  getStr(ptr) {
        var ret = UTF8ToString(ptr);
        return ret;
      },
  };
  
  function ___syscall_chdir(path) {
    path = bigintToI53Checked(path);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      FS.chdir(path);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_connect(fd, addr, addrlen, d1, d2, d3) {
    addr = bigintToI53Checked(addr);
    addrlen = bigintToI53Checked(addrlen);
  
  
  try {
  
      var sock = getSocketFromFD(fd);
      var info = getSocketAddress(addr, addrlen);
      sock.sock_ops.connect(sock, info.addr, info.port);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_faccessat(dirfd, path, amode, flags) {
    path = bigintToI53Checked(path);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      assert(!flags || flags == 512);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (amode & ~7) {
        // need a valid mode
        return -28;
      }
      var lookup = FS.lookupPath(path, { follow: true });
      var node = lookup.node;
      if (!node) {
        return -44;
      }
      var perms = '';
      if (amode & 4) perms += 'r';
      if (amode & 2) perms += 'w';
      if (amode & 1) perms += 'x';
      if (perms /* otherwise, they've just passed F_OK */ && FS.nodePermissions(node, perms)) {
        return -2;
      }
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  var syscallGetVarargP = () => {
      assert(SYSCALLS.varargs != undefined);
      var ret = Number(HEAPU64[((SYSCALLS.varargs)/8)]);
      SYSCALLS.varargs += 8;
      return ret;
    };
  
  var syscallGetVarargI = () => {
      assert(SYSCALLS.varargs != undefined);
      // the `+` prepended here is necessary to convince the JSCompiler that varargs is indeed a number.
      var ret = HEAP32[((+SYSCALLS.varargs)/4)];
      SYSCALLS.varargs += 4;
      return ret;
    };
  
  
  function ___syscall_fcntl64(fd, cmd, varargs) {
    varargs = bigintToI53Checked(varargs);
  
  
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (cmd) {
        case 0: {
          var arg = syscallGetVarargI();
          if (arg < 0) {
            return -28;
          }
          while (FS.streams[arg]) {
            arg++;
          }
          var newStream;
          newStream = FS.dupStream(stream, arg);
          return newStream.fd;
        }
        case 1:
        case 2:
          return 0;  // FD_CLOEXEC makes no sense for a single process.
        case 3:
          return stream.flags;
        case 4: {
          var arg = syscallGetVarargI();
          stream.flags |= arg;
          return 0;
        }
        case 5: {
          var arg = syscallGetVarargP();
          var offset = 0;
          // We're always unlocked.
          HEAP16[(((arg)+(offset))/2)] = 2;
          return 0;
        }
        case 6:
        case 7:
          // Pretend that the locking is successful. These are process-level locks,
          // and Emscripten programs are a single process. If we supported linking a
          // filesystem between programs, we'd need to do more here.
          // See https://github.com/emscripten-core/emscripten/issues/23697
          return 0;
      }
      return -28;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_fstat64(fd, buf) {
    buf = bigintToI53Checked(buf);
  
  
  try {
  
      return SYSCALLS.writeStat(buf, FS.fstat(fd));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_getcwd(buf, size) {
    buf = bigintToI53Checked(buf);
    size = bigintToI53Checked(size);
  
  
  try {
  
      if (size === 0) return -28;
      var cwd = FS.cwd();
      var cwdLengthInBytes = lengthBytesUTF8(cwd) + 1;
      if (size < cwdLengthInBytes) return -68;
      stringToUTF8(cwd, buf, size);
      return cwdLengthInBytes;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_getdents64(fd, dirp, count) {
    dirp = bigintToI53Checked(dirp);
    count = bigintToI53Checked(count);
  
  
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd)
      stream.getdents ||= FS.readdir(stream.path);
  
      var struct_size = 280;
      var pos = 0;
      var off = FS.llseek(stream, 0, 1);
  
      var startIdx = Math.floor(off / struct_size);
      var endIdx = Math.min(stream.getdents.length, startIdx + Math.floor(count/struct_size))
      for (var idx = startIdx; idx < endIdx; idx++) {
        var id;
        var type;
        var name = stream.getdents[idx];
        if (name === '.') {
          id = stream.node.id;
          type = 4; // DT_DIR
        }
        else if (name === '..') {
          var lookup = FS.lookupPath(stream.path, { parent: true });
          id = lookup.node.id;
          type = 4; // DT_DIR
        }
        else {
          var child;
          try {
            child = FS.lookupNode(stream.node, name);
          } catch (e) {
            // If the entry is not a directory, file, or symlink, nodefs
            // lookupNode will raise EINVAL. Skip these and continue.
            if (e?.errno === 28) {
              continue;
            }
            throw e;
          }
          id = child.id;
          type = FS.isChrdev(child.mode) ? 2 :  // DT_CHR, character device.
                 FS.isDir(child.mode) ? 4 :     // DT_DIR, directory.
                 FS.isLink(child.mode) ? 10 :   // DT_LNK, symbolic link.
                 8;                             // DT_REG, regular file.
        }
        assert(id);
        HEAP64[((dirp + pos)/8)] = BigInt(id);
        HEAP64[(((dirp + pos)+(8))/8)] = BigInt((idx + 1) * struct_size);
        HEAP16[(((dirp + pos)+(16))/2)] = 280;
        HEAP8[(dirp + pos)+(18)] = type;
        stringToUTF8(name, dirp + pos + 19, 256);
        pos += struct_size;
      }
      FS.llseek(stream, idx * struct_size, 0);
      return pos;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_ioctl(fd, op, varargs) {
    varargs = bigintToI53Checked(varargs);
  
  
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (op) {
        case 21509: {
          if (!stream.tty) return -59;
          return 0;
        }
        case 21505: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcgets) {
            var termios = stream.tty.ops.ioctl_tcgets(stream);
            var argp = syscallGetVarargP();
            HEAP32[((argp)/4)] = termios.c_iflag || 0;
            HEAP32[(((argp)+(4))/4)] = termios.c_oflag || 0;
            HEAP32[(((argp)+(8))/4)] = termios.c_cflag || 0;
            HEAP32[(((argp)+(12))/4)] = termios.c_lflag || 0;
            for (var i = 0; i < 32; i++) {
              HEAP8[(argp + i)+(17)] = termios.c_cc[i] || 0;
            }
            return 0;
          }
          return 0;
        }
        case 21510:
        case 21511:
        case 21512: {
          if (!stream.tty) return -59;
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21506:
        case 21507:
        case 21508: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcsets) {
            var argp = syscallGetVarargP();
            var c_iflag = HEAP32[((argp)/4)];
            var c_oflag = HEAP32[(((argp)+(4))/4)];
            var c_cflag = HEAP32[(((argp)+(8))/4)];
            var c_lflag = HEAP32[(((argp)+(12))/4)];
            var c_cc = []
            for (var i = 0; i < 32; i++) {
              c_cc.push(HEAP8[(argp + i)+(17)]);
            }
            return stream.tty.ops.ioctl_tcsets(stream.tty, op, { c_iflag, c_oflag, c_cflag, c_lflag, c_cc });
          }
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21519: {
          if (!stream.tty) return -59;
          var argp = syscallGetVarargP();
          HEAP32[((argp)/4)] = 0;
          return 0;
        }
        case 21520: {
          if (!stream.tty) return -59;
          return -28; // not supported
        }
        case 21537:
        case 21531: {
          var argp = syscallGetVarargP();
          return FS.ioctl(stream, op, argp);
        }
        case 21523: {
          // TODO: in theory we should write to the winsize struct that gets
          // passed in, but for now musl doesn't read anything on it
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tiocgwinsz) {
            var winsize = stream.tty.ops.ioctl_tiocgwinsz(stream.tty);
            var argp = syscallGetVarargP();
            HEAP16[((argp)/2)] = winsize[0];
            HEAP16[(((argp)+(2))/2)] = winsize[1];
          }
          return 0;
        }
        case 21524: {
          // TODO: technically, this ioctl call should change the window size.
          // but, since emscripten doesn't have any concept of a terminal window
          // yet, we'll just silently throw it away as we do TIOCGWINSZ
          if (!stream.tty) return -59;
          return 0;
        }
        case 21515: {
          if (!stream.tty) return -59;
          return 0;
        }
        default: return -28; // not supported
      }
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  function ___syscall_listen(fd, backlog) {
  try {
  
      var sock = getSocketFromFD(fd);
      sock.sock_ops.listen(sock, backlog);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }

  
  function ___syscall_lstat64(path, buf) {
    path = bigintToI53Checked(path);
    buf = bigintToI53Checked(buf);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.lstat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_newfstatat(dirfd, path, buf, flags) {
    path = bigintToI53Checked(path);
    buf = bigintToI53Checked(buf);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      var nofollow = flags & 256;
      var allowEmpty = flags & 4096;
      flags = flags & (~6400);
      assert(!flags, `unknown flags in __syscall_newfstatat: ${flags}`);
      path = SYSCALLS.calculateAt(dirfd, path, allowEmpty);
      return SYSCALLS.writeStat(buf, nofollow ? FS.lstat(path) : FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_openat(dirfd, path, flags, varargs) {
    path = bigintToI53Checked(path);
    varargs = bigintToI53Checked(varargs);
  
  
  SYSCALLS.varargs = varargs;
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      var mode = varargs ? syscallGetVarargI() : 0;
      return FS.open(path, flags, mode).fd;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_poll(fds, nfds, timeout) {
    fds = bigintToI53Checked(fds);
  
  
  try {
  
      var nonzero = 0;
      for (var i = 0; i < nfds; i++) {
        var pollfd = fds + 8 * i;
        var fd = HEAP32[((pollfd)/4)];
        var events = HEAP16[(((pollfd)+(4))/2)];
        var mask = 32;
        var stream = FS.getStream(fd);
        if (stream) {
          mask = SYSCALLS.DEFAULT_POLLMASK;
          if (stream.stream_ops.poll) {
            mask = stream.stream_ops.poll(stream, -1);
          }
        }
        mask &= events | 8 | 16;
        if (mask) nonzero++;
        HEAP16[(((pollfd)+(6))/2)] = mask;
      }
      return nonzero;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  
  function ___syscall_readlinkat(dirfd, path, buf, bufsize) {
    path = bigintToI53Checked(path);
    buf = bigintToI53Checked(buf);
    bufsize = bigintToI53Checked(bufsize);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (bufsize <= 0) return -28;
      var ret = FS.readlink(path);
  
      var len = Math.min(bufsize, lengthBytesUTF8(ret));
      var endChar = HEAP8[buf+len];
      stringToUTF8(ret, buf, bufsize+1);
      // readlink is one of the rare functions that write out a C string, but does never append a null to the output buffer(!)
      // stringToUTF8() always appends a null byte, so restore the character under the null byte after the write.
      HEAP8[buf+len] = endChar;
      return len;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  
  function ___syscall_recvfrom(fd, buf, len, flags, addr, addrlen) {
    buf = bigintToI53Checked(buf);
    len = bigintToI53Checked(len);
    addr = bigintToI53Checked(addr);
    addrlen = bigintToI53Checked(addrlen);
  
  
  try {
  
      var sock = getSocketFromFD(fd);
      var msg = sock.sock_ops.recvmsg(sock, len);
      if (!msg) return 0; // socket is closed
      if (addr) {
        var errno = writeSockaddr(addr, sock.family, DNS.lookup_name(msg.addr), msg.port, addrlen);
        assert(!errno);
      }
      HEAPU8.set(msg.buffer, buf);
      return msg.buffer.byteLength;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_rmdir(path) {
    path = bigintToI53Checked(path);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      FS.rmdir(path);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  function ___syscall_sendto(fd, message, length, flags, addr, addr_len) {
    message = bigintToI53Checked(message);
    length = bigintToI53Checked(length);
    addr = bigintToI53Checked(addr);
    addr_len = bigintToI53Checked(addr_len);
  
  
  try {
  
      var sock = getSocketFromFD(fd);
      if (!addr) {
        // send, no address provided
        return FS.write(sock.stream, HEAP8, message, length);
      }
      var dest = getSocketAddress(addr, addr_len);
      // sendto an address
      return sock.sock_ops.sendmsg(sock, HEAP8, message, length, dest.addr, dest.port);
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  function ___syscall_socket(domain, type, protocol) {
  try {
  
      var sock = SOCKFS.createSocket(domain, type, protocol);
      assert(sock.stream.fd < 64); // XXX ? select() assumes socket fd values are in 0..63
      return sock.stream.fd;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }

  
  function ___syscall_stat64(path, buf) {
    path = bigintToI53Checked(path);
    buf = bigintToI53Checked(buf);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function ___syscall_unlinkat(dirfd, path, flags) {
    path = bigintToI53Checked(path);
  
  
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (!flags) {
        FS.unlink(path);
      } else if (flags === 512) {
        FS.rmdir(path);
      } else {
        return -28;
      }
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  var __abort_js = () =>
      abort('native code called abort()');

  var readEmAsmArgsArray = [];
  var readEmAsmArgs = (sigPtr, buf) => {
      // Nobody should have mutated _readEmAsmArgsArray underneath us to be something else than an array.
      assert(Array.isArray(readEmAsmArgsArray));
      // The input buffer is allocated on the stack, so it must be stack-aligned.
      assert(buf % 16 == 0);
      readEmAsmArgsArray.length = 0;
      var ch;
      // Most arguments are i32s, so shift the buffer pointer so it is a plain
      // index into HEAP32.
      while (ch = HEAPU8[sigPtr++]) {
        var chr = String.fromCharCode(ch);
        var validChars = ['d', 'f', 'i', 'p'];
        // In WASM_BIGINT mode we support passing i64 values as bigint.
        validChars.push('j');
        assert(validChars.includes(chr), `Invalid character ${ch}("${chr}") in readEmAsmArgs! Use only [${validChars}], and do not specify "v" for void return argument.`);
        // Floats are always passed as doubles, so all types except for 'i'
        // are 8 bytes and require alignment.
        var wide = (ch != 105);
        buf += wide && (buf % 8) ? 4 : 0;
        readEmAsmArgsArray.push(
          // Special case for pointers under wasm64 or CAN_ADDRESS_2GB mode.
          ch == 112 ? Number(HEAPU64[((buf)/8)]) :
          ch == 106 ? HEAP64[((buf)/8)] :
          ch == 105 ?
            HEAP32[((buf)/4)] :
            HEAPF64[((buf)/8)]
        );
        buf += wide ? 8 : 4;
      }
      return readEmAsmArgsArray;
    };
  var runEmAsmFunction = (code, sigPtr, argbuf) => {
      var args = readEmAsmArgs(sigPtr, argbuf);
      assert(ASM_CONSTS.hasOwnProperty(code), `No EM_ASM constant found at address ${code}.  The loaded WebAssembly file is likely out of sync with the generated JavaScript.`);
      return ASM_CONSTS[code](...args);
    };
  
  function _emscripten_asm_const_int(code, sigPtr, argbuf) {
    code = bigintToI53Checked(code);
    sigPtr = bigintToI53Checked(sigPtr);
    argbuf = bigintToI53Checked(argbuf);
  
  
      return runEmAsmFunction(code, sigPtr, argbuf);
    ;
  }

  
  var _emscripten_asm_const_ptr = function(code, sigPtr, argbuf) {
    code = bigintToI53Checked(code);
    sigPtr = bigintToI53Checked(sigPtr);
    argbuf = bigintToI53Checked(argbuf);
  
  var ret = (() => { 
      return runEmAsmFunction(code, sigPtr, argbuf);
     })();
  return BigInt(ret);
  };

  
  function _emscripten_console_error(str) {
    str = bigintToI53Checked(str);
  
  
      assert(typeof str == 'number');
      console.error(UTF8ToString(str));
    ;
  }

  
  function _emscripten_console_log(str) {
    str = bigintToI53Checked(str);
  
  
      assert(typeof str == 'number');
      console.log(UTF8ToString(str));
    ;
  }

  
  function _emscripten_console_warn(str) {
    str = bigintToI53Checked(str);
  
  
      assert(typeof str == 'number');
      console.warn(UTF8ToString(str));
    ;
  }

  
  function _emscripten_err(str) {
    str = bigintToI53Checked(str);
  
  return err(UTF8ToString(str));
  }

  var abortOnCannotGrowMemory = (requestedSize) => {
      abort(`Cannot enlarge memory arrays to size ${requestedSize} bytes (OOM). Either (1) compile with -sINITIAL_MEMORY=X with X higher than the current value ${HEAP8.length}, (2) compile with -sALLOW_MEMORY_GROWTH which allows increasing the size at runtime, or (3) if you want malloc to return NULL (0) instead of this abort, compile with -sABORTING_MALLOC=0`);
    };
  
  function _emscripten_resize_heap(requestedSize) {
    requestedSize = bigintToI53Checked(requestedSize);
  
  
      var oldSize = HEAPU8.length;
      abortOnCannotGrowMemory(requestedSize);
    ;
  }

  var onExits = [];
  var addOnExit = (cb) => onExits.push(cb);
  var JSEvents = {
  removeAllEventListeners() {
        while (JSEvents.eventHandlers.length) {
          JSEvents._removeHandler(JSEvents.eventHandlers.length - 1);
        }
        JSEvents.deferredCalls = [];
      },
  inEventHandler:0,
  deferredCalls:[],
  deferCall(targetFunction, precedence, argsList) {
        function arraysHaveEqualContent(arrA, arrB) {
          if (arrA.length != arrB.length) return false;
  
          for (var i in arrA) {
            if (arrA[i] != arrB[i]) return false;
          }
          return true;
        }
        // Test if the given call was already queued, and if so, don't add it again.
        for (var call of JSEvents.deferredCalls) {
          if (call.targetFunction == targetFunction && arraysHaveEqualContent(call.argsList, argsList)) {
            return;
          }
        }
        JSEvents.deferredCalls.push({
          targetFunction,
          precedence,
          argsList
        });
  
        JSEvents.deferredCalls.sort((x,y) => x.precedence < y.precedence);
      },
  removeDeferredCalls(targetFunction) {
        JSEvents.deferredCalls = JSEvents.deferredCalls.filter((call) => call.targetFunction != targetFunction);
      },
  canPerformEventHandlerRequests() {
        if (navigator.userActivation) {
          // Verify against transient activation status from UserActivation API
          // whether it is possible to perform a request here without needing to defer. See
          // https://developer.mozilla.org/en-US/docs/Web/Security/User_activation#transient_activation
          // and https://caniuse.com/mdn-api_useractivation
          // At the time of writing, Firefox does not support this API: https://bugzil.la/1791079
          return navigator.userActivation.isActive;
        }
  
        return JSEvents.inEventHandler && JSEvents.currentEventHandler.allowsDeferredCalls;
      },
  runDeferredCalls() {
        if (!JSEvents.canPerformEventHandlerRequests()) {
          return;
        }
        var deferredCalls = JSEvents.deferredCalls;
        JSEvents.deferredCalls = [];
        for (var call of deferredCalls) {
          call.targetFunction(...call.argsList);
        }
      },
  eventHandlers:[],
  removeAllHandlersOnTarget:(target, eventTypeString) => {
        for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
          if (JSEvents.eventHandlers[i].target == target &&
            (!eventTypeString || eventTypeString == JSEvents.eventHandlers[i].eventTypeString)) {
             JSEvents._removeHandler(i--);
           }
        }
      },
  _removeHandler(i) {
        var h = JSEvents.eventHandlers[i];
        h.target.removeEventListener(h.eventTypeString, h.eventListenerFunc, h.useCapture);
        JSEvents.eventHandlers.splice(i, 1);
      },
  registerOrRemoveHandler(eventHandler) {
        if (!eventHandler.target) {
          err('registerOrRemoveHandler: the target element for event handler registration does not exist, when processing the following event handler registration:');
          console.dir(eventHandler);
          return -4;
        }
        if (eventHandler.callbackfunc) {
          eventHandler.eventListenerFunc = function(event) {
            // Increment nesting count for the event handler.
            ++JSEvents.inEventHandler;
            JSEvents.currentEventHandler = eventHandler;
            // Process any old deferred calls the user has placed.
            JSEvents.runDeferredCalls();
            // Process the actual event, calls back to user C code handler.
            eventHandler.handlerFunc(event);
            // Process any new deferred calls that were placed right now from this event handler.
            JSEvents.runDeferredCalls();
            // Out of event handler - restore nesting count.
            --JSEvents.inEventHandler;
          };
  
          eventHandler.target.addEventListener(eventHandler.eventTypeString,
                                               eventHandler.eventListenerFunc,
                                               eventHandler.useCapture);
          JSEvents.eventHandlers.push(eventHandler);
        } else {
          for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
            if (JSEvents.eventHandlers[i].target == eventHandler.target
             && JSEvents.eventHandlers[i].eventTypeString == eventHandler.eventTypeString) {
               JSEvents._removeHandler(i--);
             }
          }
        }
        return 0;
      },
  removeSingleHandler(eventHandler) {
        let success = false;
        for (let i = 0; i < JSEvents.eventHandlers.length; ++i) {
          const handler = JSEvents.eventHandlers[i];
          if (handler.target === eventHandler.target
            && handler.eventTypeId === eventHandler.eventTypeId
            && handler.callbackfunc === eventHandler.callbackfunc
            && handler.userData === eventHandler.userData) {
            // in some very rare cases (ex: Safari / fullscreen events), there is more than 1 handler (eventTypeString is different)
            JSEvents._removeHandler(i--);
            success = true;
          }
        }
        return success ? 0 : -5;
      },
  getNodeNameForTarget(target) {
        if (!target) return '';
        if (target == window) return '#window';
        if (target == screen) return '#screen';
        return target?.nodeName || '';
      },
  fullscreenEnabled() {
        return document.fullscreenEnabled
         ;
      },
  };
  
  /** @type {Object} */
  var specialHTMLTargets = [0, document, window];
  var getBoundingClientRect = (e) => specialHTMLTargets.indexOf(e) < 0 ? e.getBoundingClientRect() : {'left':0,'top':0};
  var fillMouseEventData = (eventStruct, e, target) => {
      assert(eventStruct % 4 == 0);
      HEAPF64[((eventStruct)/8)] = e.timeStamp;
      var idx = ((eventStruct)/4);
      HEAP32[idx + 2] = e.screenX;
      HEAP32[idx + 3] = e.screenY;
      HEAP32[idx + 4] = e.clientX;
      HEAP32[idx + 5] = e.clientY;
      HEAP8[eventStruct + 24] = e.ctrlKey;
      HEAP8[eventStruct + 25] = e.shiftKey;
      HEAP8[eventStruct + 26] = e.altKey;
      HEAP8[eventStruct + 27] = e.metaKey;
      HEAP16[idx*2 + 14] = e.button;
      HEAP16[idx*2 + 15] = e.buttons;
  
      HEAP32[idx + 8] = e["movementX"];
  
      HEAP32[idx + 9] = e["movementY"];
  
      // Note: rect contains doubles (truncated to placate SAFE_HEAP, which is the same behaviour when writing to HEAP32 anyway)
      var rect = getBoundingClientRect(target);
      HEAP32[idx + 10] = e.clientX - (rect.left | 0);
      HEAP32[idx + 11] = e.clientY - (rect.top  | 0);
    };
  
  var maybeCStringToJsString = (cString) => {
      // "cString > 2" checks if the input is a number, and isn't of the special
      // values we accept here, EMSCRIPTEN_EVENT_TARGET_* (which map to 0, 1, 2).
      // In other words, if cString > 2 then it's a pointer to a valid place in
      // memory, and points to a C string.
      return cString > 2 ? UTF8ToString(cString) : cString;
    };
  
  var findEventTarget = (target) => {
      target = maybeCStringToJsString(target);
      var domElement = specialHTMLTargets[target] || document.querySelector(target);
      return domElement;
    };
  
  
  var wasmTableMirror = [];
  
  
  var getWasmTableEntry = (funcPtr) => {
      // Function pointers should show up as numbers, even under wasm64, but
      // we still have some places where bigint values can flow here.
      // https://github.com/emscripten-core/emscripten/issues/18200
      funcPtr = Number(funcPtr);
      var func = wasmTableMirror[funcPtr];
      if (!func) {
        /** @suppress {checkTypes} */
        wasmTableMirror[funcPtr] = func = wasmTable.get(BigInt(funcPtr));
      }
      /** @suppress {checkTypes} */
      assert(wasmTable.get(BigInt(funcPtr)) == func, 'JavaScript-side Wasm function table mirror is out of date!');
      return func;
    };
  var registerMouseEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 64;
      JSEvents.mouseEvent ||= _malloc(eventSize);
      target = findEventTarget(target);
  
      var mouseEventHandlerFunc = (e) => {
        // TODO: Make this access thread safe, or this could update live while app is reading it.
        fillMouseEventData(JSEvents.mouseEvent, e, target);
  
        if (((a1, a2, a3) => getWasmTableEntry(callbackfunc).call(null, a1, BigInt(a2), BigInt(a3)))(eventTypeId, JSEvents.mouseEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: eventTypeString != 'mousemove' && eventTypeString != 'mouseenter' && eventTypeString != 'mouseleave', // Mouse move events do not allow fullscreen/pointer lock requests to be handled in them!
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: mouseEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  function _emscripten_set_click_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 4, "click", targetThread);
  }

  
  function _emscripten_set_dblclick_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 7, "dblclick", targetThread);
  }

  
  
  
  
  var registerKeyEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 160;
      JSEvents.keyEvent ||= _malloc(eventSize);
  
      var keyEventHandlerFunc = (e) => {
        assert(e);
  
        var keyEventData = JSEvents.keyEvent;
        HEAPF64[((keyEventData)/8)] = e.timeStamp;
  
        var idx = ((keyEventData)/4);
  
        HEAP32[idx + 2] = e.location;
        HEAP8[keyEventData + 12] = e.ctrlKey;
        HEAP8[keyEventData + 13] = e.shiftKey;
        HEAP8[keyEventData + 14] = e.altKey;
        HEAP8[keyEventData + 15] = e.metaKey;
        HEAP8[keyEventData + 16] = e.repeat;
        HEAP32[idx + 5] = e.charCode;
        HEAP32[idx + 6] = e.keyCode;
        HEAP32[idx + 7] = e.which;
        stringToUTF8(e.key || '', keyEventData + 32, 32);
        stringToUTF8(e.code || '', keyEventData + 64, 32);
        stringToUTF8(e.char || '', keyEventData + 96, 32);
        stringToUTF8(e.locale || '', keyEventData + 128, 32);
  
        if (((a1, a2, a3) => getWasmTableEntry(callbackfunc).call(null, a1, BigInt(a2), BigInt(a3)))(eventTypeId, keyEventData, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: keyEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  function _emscripten_set_keydown_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerKeyEventCallback(target, userData, useCapture, callbackfunc, 2, "keydown", targetThread);
  }

  
  function _emscripten_set_keypress_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerKeyEventCallback(target, userData, useCapture, callbackfunc, 1, "keypress", targetThread);
  }

  
  function _emscripten_set_keyup_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerKeyEventCallback(target, userData, useCapture, callbackfunc, 3, "keyup", targetThread);
  }

  
  function _emscripten_set_mousedown_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 5, "mousedown", targetThread);
  }

  
  function _emscripten_set_mouseenter_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 33, "mouseenter", targetThread);
  }

  
  function _emscripten_set_mouseleave_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 34, "mouseleave", targetThread);
  }

  
  function _emscripten_set_mousemove_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 8, "mousemove", targetThread);
  }

  
  function _emscripten_set_mouseup_callback_on_thread(target, userData, useCapture, callbackfunc, targetThread) {
    target = bigintToI53Checked(target);
    userData = bigintToI53Checked(userData);
    callbackfunc = bigintToI53Checked(callbackfunc);
    targetThread = bigintToI53Checked(targetThread);
  
  return registerMouseEventCallback(target, userData, useCapture, callbackfunc, 6, "mouseup", targetThread);
  }

  
  var runtimeKeepaliveCounter = 0;
  var keepRuntimeAlive = () => noExitRuntime || runtimeKeepaliveCounter > 0;
  var _proc_exit = (code) => {
      EXITSTATUS = code;
      if (!keepRuntimeAlive()) {
        Module['onExit']?.(code);
        ABORT = true;
      }
      quit_(code, new ExitStatus(code));
    };
  
  
  /** @param {boolean|number=} implicit */
  var exitJS = (status, implicit) => {
      EXITSTATUS = status;
  
      checkUnflushedContent();
  
      // if exit() was called explicitly, warn the user if the runtime isn't actually being shut down
      if (keepRuntimeAlive() && !implicit) {
        var msg = `program exited (with status: ${status}), but keepRuntimeAlive() is set (counter=${runtimeKeepaliveCounter}) due to an async operation, so halting execution but not exiting the runtime or preventing further async execution (you can use emscripten_force_exit, if you want to force a true shutdown)`;
        err(msg);
      }
  
      _proc_exit(status);
    };
  var _exit = exitJS;

  function _fd_close(fd) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.close(stream);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }

  /** @param {number=} offset */
  var doReadv = (stream, iov, iovcnt, offset) => {
      var ret = 0;
      for (var i = 0; i < iovcnt; i++) {
        var ptr = Number(HEAPU64[((iov)/8)]);
        var len = Number(HEAPU64[(((iov)+(8))/8)]);
        iov += 16;
        var curr = FS.read(stream, HEAP8, ptr, len, offset);
        if (curr < 0) return -1;
        ret += curr;
        if (curr < len) break; // nothing more to read
        if (typeof offset != 'undefined') {
          offset += curr;
        }
      }
      return ret;
    };
  
  
  function _fd_read(fd, iov, iovcnt, pnum) {
    iov = bigintToI53Checked(iov);
    iovcnt = bigintToI53Checked(iovcnt);
    pnum = bigintToI53Checked(pnum);
  
  
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doReadv(stream, iov, iovcnt);
      HEAPU64[((pnum)/8)] = BigInt(num);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  ;
  }

  
  function _fd_seek(fd, offset, whence, newOffset) {
    offset = bigintToI53Checked(offset);
    newOffset = bigintToI53Checked(newOffset);
  
  
  try {
  
      if (isNaN(offset)) return 61;
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.llseek(stream, offset, whence);
      HEAP64[((newOffset)/8)] = BigInt(stream.position);
      if (stream.getdents && offset === 0 && whence === 0) stream.getdents = null; // reset readdir state
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  ;
  }

  /** @param {number=} offset */
  var doWritev = (stream, iov, iovcnt, offset) => {
      var ret = 0;
      for (var i = 0; i < iovcnt; i++) {
        var ptr = Number(HEAPU64[((iov)/8)]);
        var len = Number(HEAPU64[(((iov)+(8))/8)]);
        iov += 16;
        var curr = FS.write(stream, HEAP8, ptr, len, offset);
        if (curr < 0) return -1;
        ret += curr;
        if (curr < len) {
          // No more space to write.
          break;
        }
        if (typeof offset != 'undefined') {
          offset += curr;
        }
      }
      return ret;
    };
  
  
  function _fd_write(fd, iov, iovcnt, pnum) {
    iov = bigintToI53Checked(iov);
    iovcnt = bigintToI53Checked(iovcnt);
    pnum = bigintToI53Checked(pnum);
  
  
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doWritev(stream, iov, iovcnt);
      HEAPU64[((pnum)/8)] = BigInt(num);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  ;
  }

  
  
  
  
  
  
  
  
  
  function _getaddrinfo(node, service, hint, out) {
    node = bigintToI53Checked(node);
    service = bigintToI53Checked(service);
    hint = bigintToI53Checked(hint);
    out = bigintToI53Checked(out);
  
  
      // Note getaddrinfo currently only returns a single addrinfo with ai_next defaulting to NULL. When NULL
      // hints are specified or ai_family set to AF_UNSPEC or ai_socktype or ai_protocol set to 0 then we
      // really should provide a linked list of suitable addrinfo values.
      var addrs = [];
      var canon = null;
      var addr = 0;
      var port = 0;
      var flags = 0;
      var family = 0;
      var type = 0;
      var proto = 0;
      var ai, last;
  
      function allocaddrinfo(family, type, proto, canon, addr, port) {
        var sa, salen, ai;
        var errno;
  
        salen = family === 10 ?
          28 :
          16;
        addr = family === 10 ?
          inetNtop6(addr) :
          inetNtop4(addr);
        sa = _malloc(salen);
        errno = writeSockaddr(sa, family, addr, port);
        assert(!errno);
  
        ai = _malloc(48);
        HEAP32[(((ai)+(4))/4)] = family;
        HEAP32[(((ai)+(8))/4)] = type;
        HEAP32[(((ai)+(12))/4)] = proto;
        HEAPU64[(((ai)+(32))/8)] = BigInt(canon);
        HEAPU64[(((ai)+(24))/8)] = BigInt(sa);
        if (family === 10) {
          HEAP32[(((ai)+(16))/4)] = 28;
        } else {
          HEAP32[(((ai)+(16))/4)] = 16;
        }
        HEAP32[(((ai)+(40))/4)] = 0;
  
        return ai;
      }
  
      if (hint) {
        flags = HEAP32[((hint)/4)];
        family = HEAP32[(((hint)+(4))/4)];
        type = HEAP32[(((hint)+(8))/4)];
        proto = HEAP32[(((hint)+(12))/4)];
      }
      if (type && !proto) {
        proto = type === 2 ? 17 : 6;
      }
      if (!type && proto) {
        type = proto === 17 ? 2 : 1;
      }
  
      // If type or proto are set to zero in hints we should really be returning multiple addrinfo values, but for
      // now default to a TCP STREAM socket so we can at least return a sensible addrinfo given NULL hints.
      if (proto === 0) {
        proto = 6;
      }
      if (type === 0) {
        type = 1;
      }
  
      if (!node && !service) {
        return -2;
      }
      if (flags & ~(1|2|4|
          1024|8|16|32)) {
        return -1;
      }
      if (hint !== 0 && (HEAP32[((hint)/4)] & 2) && !node) {
        return -1;
      }
      if (flags & 32) {
        // TODO
        return -2;
      }
      if (type !== 0 && type !== 1 && type !== 2) {
        return -7;
      }
      if (family !== 0 && family !== 2 && family !== 10) {
        return -6;
      }
  
      if (service) {
        service = UTF8ToString(service);
        port = parseInt(service, 10);
  
        if (isNaN(port)) {
          if (flags & 1024) {
            return -2;
          }
          // TODO support resolving well-known service names from:
          // http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt
          return -8;
        }
      }
  
      if (!node) {
        if (family === 0) {
          family = 2;
        }
        if ((flags & 1) === 0) {
          if (family === 2) {
            addr = _htonl(2130706433);
          } else {
            addr = [0, 0, 0, _htonl(1)];
          }
        }
        ai = allocaddrinfo(family, type, proto, null, addr, port);
        HEAPU64[((out)/8)] = BigInt(ai);
        return 0;
      }
  
      //
      // try as a numeric address
      //
      node = UTF8ToString(node);
      addr = inetPton4(node);
      if (addr !== null) {
        // incoming node is a valid ipv4 address
        if (family === 0 || family === 2) {
          family = 2;
        }
        else if (family === 10 && (flags & 8)) {
          addr = [0, 0, _htonl(0xffff), addr];
          family = 10;
        } else {
          return -2;
        }
      } else {
        addr = inetPton6(node);
        if (addr !== null) {
          // incoming node is a valid ipv6 address
          if (family === 0 || family === 10) {
            family = 10;
          } else {
            return -2;
          }
        }
      }
      if (addr != null) {
        ai = allocaddrinfo(family, type, proto, node, addr, port);
        HEAPU64[((out)/8)] = BigInt(ai);
        return 0;
      }
      if (flags & 4) {
        return -2;
      }
  
      //
      // try as a hostname
      //
      // resolve the hostname to a temporary fake address
      node = DNS.lookup_name(node);
      addr = inetPton4(node);
      if (family === 0) {
        family = 2;
      } else if (family === 10) {
        addr = [0, 0, _htonl(0xffff), addr];
      }
      ai = allocaddrinfo(family, type, proto, null, addr, port);
      HEAPU64[((out)/8)] = BigInt(ai);
      return 0;
    ;
  }

  var getCFunc = (ident) => {
      var func = Module['_' + ident]; // closure exported function
      assert(func, 'Cannot call unknown function ' + ident + ', make sure it is exported');
      return func;
    };
  
  var writeArrayToMemory = (array, buffer) => {
      assert(array.length >= 0, 'writeArrayToMemory array must have a length (should be an array or typed array)')
      HEAP8.set(array, buffer);
    };
  
  
  
  var stackAlloc = (sz) => __emscripten_stack_alloc(sz);
  var stringToUTF8OnStack = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = stackAlloc(size);
      stringToUTF8(str, ret, size);
      return ret;
    };
  
  
  
  
  
    /**
     * @param {string|null=} returnType
     * @param {Array=} argTypes
     * @param {Array=} args
     * @param {Object=} opts
     */
  var ccall = (ident, returnType, argTypes, args, opts) => {
      // For fast lookup of conversion functions
      var toC = {
        'pointer': (p) => BigInt(p),
        'string': (str) => {
          var ret = 0;
          if (str !== null && str !== undefined && str !== 0) { // null string
            ret = stringToUTF8OnStack(str);
          }
          return BigInt(ret);
        },
        'array': (arr) => {
          var ret = stackAlloc(arr.length);
          writeArrayToMemory(arr, ret);
          return BigInt(ret);
        }
      };
  
      function convertReturnValue(ret) {
        if (returnType === 'string') {
          return UTF8ToString(Number(ret));
        }
        if (returnType === 'pointer') return Number(ret);
        if (returnType === 'boolean') return Boolean(ret);
        return ret;
      }
  
      var func = getCFunc(ident);
      var cArgs = [];
      var stack = 0;
      assert(returnType !== 'array', 'Return type should not be "array".');
      if (args) {
        for (var i = 0; i < args.length; i++) {
          var converter = toC[argTypes[i]];
          if (converter) {
            if (stack === 0) stack = stackSave();
            cArgs[i] = converter(args[i]);
          } else {
            cArgs[i] = args[i];
          }
        }
      }
      var ret = func(...cArgs);
      function onDone(ret) {
        if (stack !== 0) stackRestore(stack);
        return convertReturnValue(ret);
      }
  
      ret = onDone(ret);
      return ret;
    };
  
    /**
     * @param {string=} returnType
     * @param {Array=} argTypes
     * @param {Object=} opts
     */
  var cwrap = (ident, returnType, argTypes, opts) => {
      return (...args) => ccall(ident, returnType, argTypes, args, opts);
    };

    // Precreate a reverse lookup table from chars
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" back to
    // bytes to make decoding fast.
    for (var base64ReverseLookup = new Uint8Array(123/*'z'+1*/), i = 25; i >= 0; --i) {
      base64ReverseLookup[48+i] = 52+i; // '0-9'
      base64ReverseLookup[65+i] = i; // 'A-Z'
      base64ReverseLookup[97+i] = 26+i; // 'a-z'
    }
    base64ReverseLookup[43] = 62; // '+'
    base64ReverseLookup[47] = 63; // '/'
  ;

  FS.createPreloadedFile = FS_createPreloadedFile;
  FS.preloadFile = FS_preloadFile;
  FS.staticInit();;
// End JS library code

// include: postlibrary.js
// This file is included after the automatically-generated JS library code
// but before the wasm module is created.

{

  // Begin ATMODULES hooks
  if (Module['noExitRuntime']) noExitRuntime = Module['noExitRuntime'];
if (Module['preloadPlugins']) preloadPlugins = Module['preloadPlugins'];
if (Module['print']) out = Module['print'];
if (Module['printErr']) err = Module['printErr'];
if (Module['wasmBinary']) wasmBinary = Module['wasmBinary'];
  // End ATMODULES hooks

  checkIncomingModuleAPI();

  if (Module['arguments']) arguments_ = Module['arguments'];
  if (Module['thisProgram']) thisProgram = Module['thisProgram'];

  // Assertions on removed incoming Module JS APIs.
  assert(typeof Module['memoryInitializerPrefixURL'] == 'undefined', 'Module.memoryInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['pthreadMainPrefixURL'] == 'undefined', 'Module.pthreadMainPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['cdInitializerPrefixURL'] == 'undefined', 'Module.cdInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['filePackagePrefixURL'] == 'undefined', 'Module.filePackagePrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['read'] == 'undefined', 'Module.read option was removed');
  assert(typeof Module['readAsync'] == 'undefined', 'Module.readAsync option was removed (modify readAsync in JS)');
  assert(typeof Module['readBinary'] == 'undefined', 'Module.readBinary option was removed (modify readBinary in JS)');
  assert(typeof Module['setWindowTitle'] == 'undefined', 'Module.setWindowTitle option was removed (modify emscripten_set_window_title in JS)');
  assert(typeof Module['TOTAL_MEMORY'] == 'undefined', 'Module.TOTAL_MEMORY has been renamed Module.INITIAL_MEMORY');
  assert(typeof Module['ENVIRONMENT'] == 'undefined', 'Module.ENVIRONMENT has been deprecated. To force the environment, use the ENVIRONMENT compile-time option (for example, -sENVIRONMENT=web or -sENVIRONMENT=node)');
  assert(typeof Module['STACK_SIZE'] == 'undefined', 'STACK_SIZE can no longer be set at runtime.  Use -sSTACK_SIZE at link time')
  // If memory is defined in wasm, the user can't provide it, or set INITIAL_MEMORY
  assert(typeof Module['wasmMemory'] == 'undefined', 'Use of `wasmMemory` detected.  Use -sIMPORTED_MEMORY to define wasmMemory externally');
  assert(typeof Module['INITIAL_MEMORY'] == 'undefined', 'Detected runtime INITIAL_MEMORY setting.  Use -sIMPORTED_MEMORY to define wasmMemory dynamically');

  if (Module['preInit']) {
    if (typeof Module['preInit'] == 'function') Module['preInit'] = [Module['preInit']];
    while (Module['preInit'].length > 0) {
      Module['preInit'].shift()();
    }
  }
  consumedModuleProp('preInit');
}

// Begin runtime exports
  Module['cwrap'] = cwrap;
  var missingLibrarySymbols = [
  'writeI53ToI64',
  'writeI53ToI64Clamped',
  'writeI53ToI64Signaling',
  'writeI53ToU64Clamped',
  'writeI53ToU64Signaling',
  'readI53FromI64',
  'readI53FromU64',
  'convertI32PairToI53',
  'convertI32PairToI53Checked',
  'convertU32PairToI53',
  'getTempRet0',
  'setTempRet0',
  'createNamedFunction',
  'getHeapMax',
  'growMemory',
  'withStackSave',
  'runMainThreadEmAsm',
  'jstoi_q',
  'getExecutableName',
  'autoResumeAudioContext',
  'getDynCaller',
  'dynCall',
  'handleException',
  'runtimeKeepalivePush',
  'runtimeKeepalivePop',
  'callUserCallback',
  'maybeExit',
  'asmjsMangle',
  'alignMemory',
  'HandleAllocator',
  'addOnInit',
  'addOnPostCtor',
  'addOnPreMain',
  'STACK_SIZE',
  'STACK_ALIGN',
  'POINTER_SIZE',
  'ASSERTIONS',
  'convertJsFunctionToWasm',
  'getEmptyTableSlot',
  'updateTableMap',
  'getFunctionAddress',
  'addFunction',
  'removeFunction',
  'intArrayToString',
  'AsciiToString',
  'stringToAscii',
  'UTF16ToString',
  'stringToUTF16',
  'lengthBytesUTF16',
  'UTF32ToString',
  'stringToUTF32',
  'lengthBytesUTF32',
  'registerWheelEventCallback',
  'registerUiEventCallback',
  'registerFocusEventCallback',
  'fillDeviceOrientationEventData',
  'registerDeviceOrientationEventCallback',
  'fillDeviceMotionEventData',
  'registerDeviceMotionEventCallback',
  'screenOrientation',
  'fillOrientationChangeEventData',
  'registerOrientationChangeEventCallback',
  'fillFullscreenChangeEventData',
  'registerFullscreenChangeEventCallback',
  'JSEvents_requestFullscreen',
  'JSEvents_resizeCanvasForFullscreen',
  'registerRestoreOldStyle',
  'hideEverythingExceptGivenElement',
  'restoreHiddenElements',
  'setLetterbox',
  'softFullscreenResizeWebGLRenderTarget',
  'doRequestFullscreen',
  'fillPointerlockChangeEventData',
  'registerPointerlockChangeEventCallback',
  'registerPointerlockErrorEventCallback',
  'requestPointerLock',
  'fillVisibilityChangeEventData',
  'registerVisibilityChangeEventCallback',
  'registerTouchEventCallback',
  'fillGamepadEventData',
  'registerGamepadEventCallback',
  'registerBeforeUnloadEventCallback',
  'fillBatteryEventData',
  'registerBatteryEventCallback',
  'setCanvasElementSize',
  'getCanvasElementSize',
  'jsStackTrace',
  'getCallstack',
  'convertPCtoSourceLocation',
  'getEnvStrings',
  'checkWasiClock',
  'wasiRightsToMuslOFlags',
  'wasiOFlagsToMuslOFlags',
  'safeSetTimeout',
  'setImmediateWrapped',
  'safeRequestAnimationFrame',
  'clearImmediateWrapped',
  'registerPostMainLoop',
  'registerPreMainLoop',
  'getPromise',
  'makePromise',
  'idsToPromises',
  'makePromiseCallback',
  'ExceptionInfo',
  'findMatchingCatch',
  'Browser_asyncPrepareDataCounter',
  'isLeapYear',
  'ydayFromDate',
  'arraySum',
  'addDays',
  'FS_mkdirTree',
  '_setNetworkCallback',
  'heapObjectForWebGLType',
  'toTypedArrayIndex',
  'webgl_enable_ANGLE_instanced_arrays',
  'webgl_enable_OES_vertex_array_object',
  'webgl_enable_WEBGL_draw_buffers',
  'webgl_enable_WEBGL_multi_draw',
  'webgl_enable_EXT_polygon_offset_clamp',
  'webgl_enable_EXT_clip_control',
  'webgl_enable_WEBGL_polygon_mode',
  'emscriptenWebGLGet',
  'computeUnpackAlignedImageSize',
  'colorChannelsInGlTextureFormat',
  'emscriptenWebGLGetTexPixelData',
  'emscriptenWebGLGetUniform',
  'webglGetUniformLocation',
  'webglPrepareUniformLocationsBeforeFirstUse',
  'webglGetLeftBracePos',
  'emscriptenWebGLGetVertexAttrib',
  '__glGetActiveAttribOrUniform',
  'emscriptenWebGLGetBufferBinding',
  'emscriptenWebGLValidateMapBufferTarget',
  'writeGLArray',
  'registerWebGlEventCallback',
  'runAndAbortIfError',
  'emscriptenWebGLGetIndexed',
  'webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance',
  'webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance',
  'ALLOC_NORMAL',
  'ALLOC_STACK',
  'allocate',
  'writeStringToMemory',
  'writeAsciiToMemory',
  'allocateUTF8',
  'allocateUTF8OnStack',
  'demangle',
  'stackTrace',
  'getNativeTypeSize',
];
missingLibrarySymbols.forEach(missingLibrarySymbol)

  var unexportedSymbols = [
  'run',
  'out',
  'err',
  'callMain',
  'abort',
  'wasmExports',
  'HEAPF32',
  'HEAPF64',
  'HEAP8',
  'HEAP16',
  'HEAPU16',
  'HEAP32',
  'HEAPU32',
  'HEAP64',
  'HEAPU64',
  'writeStackCookie',
  'checkStackCookie',
  'INT53_MAX',
  'INT53_MIN',
  'bigintToI53Checked',
  'stackSave',
  'stackRestore',
  'stackAlloc',
  'ptrToString',
  'zeroMemory',
  'exitJS',
  'abortOnCannotGrowMemory',
  'ENV',
  'ERRNO_CODES',
  'strError',
  'inetPton4',
  'inetNtop4',
  'inetPton6',
  'inetNtop6',
  'readSockaddr',
  'writeSockaddr',
  'DNS',
  'Protocols',
  'Sockets',
  'timers',
  'warnOnce',
  'readEmAsmArgsArray',
  'readEmAsmArgs',
  'runEmAsmFunction',
  'keepRuntimeAlive',
  'asyncLoad',
  'mmapAlloc',
  'wasmTable',
  'wasmMemory',
  'getUniqueRunDependency',
  'noExitRuntime',
  'addRunDependency',
  'removeRunDependency',
  'addOnPreRun',
  'addOnExit',
  'addOnPostRun',
  'ccall',
  'freeTableIndexes',
  'functionsInTableMap',
  'setValue',
  'getValue',
  'PATH',
  'PATH_FS',
  'UTF8Decoder',
  'UTF8ArrayToString',
  'UTF8ToString',
  'stringToUTF8Array',
  'stringToUTF8',
  'lengthBytesUTF8',
  'intArrayFromString',
  'UTF16Decoder',
  'stringToNewUTF8',
  'stringToUTF8OnStack',
  'writeArrayToMemory',
  'JSEvents',
  'registerKeyEventCallback',
  'specialHTMLTargets',
  'maybeCStringToJsString',
  'findEventTarget',
  'findCanvasEventTarget',
  'getBoundingClientRect',
  'fillMouseEventData',
  'registerMouseEventCallback',
  'currentFullscreenStrategy',
  'restoreOldWindowedStyle',
  'UNWIND_CACHE',
  'ExitStatus',
  'doReadv',
  'doWritev',
  'initRandomFill',
  'randomFill',
  'emSetImmediate',
  'emClearImmediate_deps',
  'emClearImmediate',
  'promiseMap',
  'uncaughtExceptionCount',
  'exceptionLast',
  'exceptionCaught',
  'Browser',
  'requestFullscreen',
  'requestFullScreen',
  'setCanvasSize',
  'getUserMedia',
  'createContext',
  'getPreloadedImageData__data',
  'wget',
  'MONTH_DAYS_REGULAR',
  'MONTH_DAYS_LEAP',
  'MONTH_DAYS_REGULAR_CUMULATIVE',
  'MONTH_DAYS_LEAP_CUMULATIVE',
  'base64Decode',
  'SYSCALLS',
  'getSocketFromFD',
  'getSocketAddress',
  'preloadPlugins',
  'FS_createPreloadedFile',
  'FS_preloadFile',
  'FS_modeStringToFlags',
  'FS_getMode',
  'FS_stdin_getChar_buffer',
  'FS_stdin_getChar',
  'FS_unlink',
  'FS_createPath',
  'FS_createDevice',
  'FS_readFile',
  'FS',
  'FS_root',
  'FS_mounts',
  'FS_devices',
  'FS_streams',
  'FS_nextInode',
  'FS_nameTable',
  'FS_currentPath',
  'FS_initialized',
  'FS_ignorePermissions',
  'FS_filesystems',
  'FS_syncFSRequests',
  'FS_readFiles',
  'FS_lookupPath',
  'FS_getPath',
  'FS_hashName',
  'FS_hashAddNode',
  'FS_hashRemoveNode',
  'FS_lookupNode',
  'FS_createNode',
  'FS_destroyNode',
  'FS_isRoot',
  'FS_isMountpoint',
  'FS_isFile',
  'FS_isDir',
  'FS_isLink',
  'FS_isChrdev',
  'FS_isBlkdev',
  'FS_isFIFO',
  'FS_isSocket',
  'FS_flagsToPermissionString',
  'FS_nodePermissions',
  'FS_mayLookup',
  'FS_mayCreate',
  'FS_mayDelete',
  'FS_mayOpen',
  'FS_checkOpExists',
  'FS_nextfd',
  'FS_getStreamChecked',
  'FS_getStream',
  'FS_createStream',
  'FS_closeStream',
  'FS_dupStream',
  'FS_doSetAttr',
  'FS_chrdev_stream_ops',
  'FS_major',
  'FS_minor',
  'FS_makedev',
  'FS_registerDevice',
  'FS_getDevice',
  'FS_getMounts',
  'FS_syncfs',
  'FS_mount',
  'FS_unmount',
  'FS_lookup',
  'FS_mknod',
  'FS_statfs',
  'FS_statfsStream',
  'FS_statfsNode',
  'FS_create',
  'FS_mkdir',
  'FS_mkdev',
  'FS_symlink',
  'FS_rename',
  'FS_rmdir',
  'FS_readdir',
  'FS_readlink',
  'FS_stat',
  'FS_fstat',
  'FS_lstat',
  'FS_doChmod',
  'FS_chmod',
  'FS_lchmod',
  'FS_fchmod',
  'FS_doChown',
  'FS_chown',
  'FS_lchown',
  'FS_fchown',
  'FS_doTruncate',
  'FS_truncate',
  'FS_ftruncate',
  'FS_utime',
  'FS_open',
  'FS_close',
  'FS_isClosed',
  'FS_llseek',
  'FS_read',
  'FS_write',
  'FS_mmap',
  'FS_msync',
  'FS_ioctl',
  'FS_writeFile',
  'FS_cwd',
  'FS_chdir',
  'FS_createDefaultDirectories',
  'FS_createDefaultDevices',
  'FS_createSpecialDirectories',
  'FS_createStandardStreams',
  'FS_staticInit',
  'FS_init',
  'FS_quit',
  'FS_findObject',
  'FS_analyzePath',
  'FS_createFile',
  'FS_createDataFile',
  'FS_forceLoadFile',
  'FS_createLazyFile',
  'FS_absolutePath',
  'FS_createFolder',
  'FS_createLink',
  'FS_joinPath',
  'FS_mmapAlloc',
  'FS_standardizePath',
  'MEMFS',
  'TTY',
  'PIPEFS',
  'SOCKFS',
  'tempFixedLengthArray',
  'miniTempWebGLFloatBuffers',
  'miniTempWebGLIntBuffers',
  'GL',
  'AL',
  'GLUT',
  'EGL',
  'GLEW',
  'IDBStore',
  'SDL',
  'SDL_gfx',
  'print',
  'printErr',
  'jstoi_s',
];
unexportedSymbols.forEach(unexportedRuntimeSymbol);

  // End runtime exports
  // Begin JS library exports
  // End JS library exports

// end include: postlibrary.js

function checkIncomingModuleAPI() {
  ignoredModuleProp('fetchSettings');
}
var ASM_CONSTS = {
  84784: ($0) => { console.log(UTF8ToString($0)); },  
 84819: ($0) => { alert(UTF8ToString($0)); },  
 84848: ($0, $1) => { const element = document.querySelector(UTF8ToString($0)); element.innerHTML = UTF8ToString($1); },  
 84948: ($0, $1) => { const element = document.querySelector(UTF8ToString($0)); element.textContent = UTF8ToString($1); },  
 85050: ($0) => { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.innerHTML); },  
 85155: ($0) => { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.textContent); }
};

// Imports from the Wasm binary.
var _malloc = Module['_malloc'] = makeInvalidEarlyAccess('_malloc');
var _emscripten_create = Module['_emscripten_create'] = makeInvalidEarlyAccess('_emscripten_create');
var _emscripten_eval_compiled = Module['_emscripten_eval_compiled'] = makeInvalidEarlyAccess('_emscripten_eval_compiled');
var _free = Module['_free'] = makeInvalidEarlyAccess('_free');
var _emscripten_eval_macros = Module['_emscripten_eval_macros'] = makeInvalidEarlyAccess('_emscripten_eval_macros');
var _emscripten_eval = Module['_emscripten_eval'] = makeInvalidEarlyAccess('_emscripten_eval');
var _emscripten_destroy = Module['_emscripten_destroy'] = makeInvalidEarlyAccess('_emscripten_destroy');
var _htons = makeInvalidEarlyAccess('_htons');
var _fflush = makeInvalidEarlyAccess('_fflush');
var _htonl = makeInvalidEarlyAccess('_htonl');
var _ntohs = makeInvalidEarlyAccess('_ntohs');
var _emscripten_stack_get_end = makeInvalidEarlyAccess('_emscripten_stack_get_end');
var _emscripten_stack_get_base = makeInvalidEarlyAccess('_emscripten_stack_get_base');
var _strerror = makeInvalidEarlyAccess('_strerror');
var _emscripten_stack_init = makeInvalidEarlyAccess('_emscripten_stack_init');
var _emscripten_stack_get_free = makeInvalidEarlyAccess('_emscripten_stack_get_free');
var __emscripten_stack_restore = makeInvalidEarlyAccess('__emscripten_stack_restore');
var __emscripten_stack_alloc = makeInvalidEarlyAccess('__emscripten_stack_alloc');
var _emscripten_stack_get_current = makeInvalidEarlyAccess('_emscripten_stack_get_current');
var memory = makeInvalidEarlyAccess('memory');
var __indirect_function_table = makeInvalidEarlyAccess('__indirect_function_table');
var wasmMemory = makeInvalidEarlyAccess('wasmMemory');
var wasmTable = makeInvalidEarlyAccess('wasmTable');

function assignWasmExports(wasmExports) {
  assert(typeof wasmExports['malloc'] != 'undefined', 'missing Wasm export: malloc');
  assert(typeof wasmExports['emscripten_create'] != 'undefined', 'missing Wasm export: emscripten_create');
  assert(typeof wasmExports['emscripten_eval_compiled'] != 'undefined', 'missing Wasm export: emscripten_eval_compiled');
  assert(typeof wasmExports['free'] != 'undefined', 'missing Wasm export: free');
  assert(typeof wasmExports['emscripten_eval_macros'] != 'undefined', 'missing Wasm export: emscripten_eval_macros');
  assert(typeof wasmExports['emscripten_eval'] != 'undefined', 'missing Wasm export: emscripten_eval');
  assert(typeof wasmExports['emscripten_destroy'] != 'undefined', 'missing Wasm export: emscripten_destroy');
  assert(typeof wasmExports['htons'] != 'undefined', 'missing Wasm export: htons');
  assert(typeof wasmExports['fflush'] != 'undefined', 'missing Wasm export: fflush');
  assert(typeof wasmExports['htonl'] != 'undefined', 'missing Wasm export: htonl');
  assert(typeof wasmExports['ntohs'] != 'undefined', 'missing Wasm export: ntohs');
  assert(typeof wasmExports['emscripten_stack_get_end'] != 'undefined', 'missing Wasm export: emscripten_stack_get_end');
  assert(typeof wasmExports['emscripten_stack_get_base'] != 'undefined', 'missing Wasm export: emscripten_stack_get_base');
  assert(typeof wasmExports['strerror'] != 'undefined', 'missing Wasm export: strerror');
  assert(typeof wasmExports['emscripten_stack_init'] != 'undefined', 'missing Wasm export: emscripten_stack_init');
  assert(typeof wasmExports['emscripten_stack_get_free'] != 'undefined', 'missing Wasm export: emscripten_stack_get_free');
  assert(typeof wasmExports['_emscripten_stack_restore'] != 'undefined', 'missing Wasm export: _emscripten_stack_restore');
  assert(typeof wasmExports['_emscripten_stack_alloc'] != 'undefined', 'missing Wasm export: _emscripten_stack_alloc');
  assert(typeof wasmExports['emscripten_stack_get_current'] != 'undefined', 'missing Wasm export: emscripten_stack_get_current');
  assert(typeof wasmExports['memory'] != 'undefined', 'missing Wasm export: memory');
  assert(typeof wasmExports['__indirect_function_table'] != 'undefined', 'missing Wasm export: __indirect_function_table');
  _malloc = Module['_malloc'] = createExportWrapper('malloc', 1);
  _emscripten_create = Module['_emscripten_create'] = createExportWrapper('emscripten_create', 0);
  _emscripten_eval_compiled = Module['_emscripten_eval_compiled'] = createExportWrapper('emscripten_eval_compiled', 2);
  _free = Module['_free'] = createExportWrapper('free', 1);
  _emscripten_eval_macros = Module['_emscripten_eval_macros'] = createExportWrapper('emscripten_eval_macros', 2);
  _emscripten_eval = Module['_emscripten_eval'] = createExportWrapper('emscripten_eval', 2);
  _emscripten_destroy = Module['_emscripten_destroy'] = createExportWrapper('emscripten_destroy', 0);
  _htons = createExportWrapper('htons', 1);
  _fflush = createExportWrapper('fflush', 1);
  _htonl = createExportWrapper('htonl', 1);
  _ntohs = createExportWrapper('ntohs', 1);
  _emscripten_stack_get_end = wasmExports['emscripten_stack_get_end'];
  _emscripten_stack_get_base = wasmExports['emscripten_stack_get_base'];
  _strerror = createExportWrapper('strerror', 1);
  _emscripten_stack_init = wasmExports['emscripten_stack_init'];
  _emscripten_stack_get_free = wasmExports['emscripten_stack_get_free'];
  __emscripten_stack_restore = wasmExports['_emscripten_stack_restore'];
  __emscripten_stack_alloc = wasmExports['_emscripten_stack_alloc'];
  _emscripten_stack_get_current = wasmExports['emscripten_stack_get_current'];
  memory = wasmMemory = wasmExports['memory'];
  __indirect_function_table = wasmTable = wasmExports['__indirect_function_table'];
}

var wasmImports = {
  /** @export */
  __syscall_accept4: ___syscall_accept4,
  /** @export */
  __syscall_bind: ___syscall_bind,
  /** @export */
  __syscall_chdir: ___syscall_chdir,
  /** @export */
  __syscall_connect: ___syscall_connect,
  /** @export */
  __syscall_faccessat: ___syscall_faccessat,
  /** @export */
  __syscall_fcntl64: ___syscall_fcntl64,
  /** @export */
  __syscall_fstat64: ___syscall_fstat64,
  /** @export */
  __syscall_getcwd: ___syscall_getcwd,
  /** @export */
  __syscall_getdents64: ___syscall_getdents64,
  /** @export */
  __syscall_ioctl: ___syscall_ioctl,
  /** @export */
  __syscall_listen: ___syscall_listen,
  /** @export */
  __syscall_lstat64: ___syscall_lstat64,
  /** @export */
  __syscall_newfstatat: ___syscall_newfstatat,
  /** @export */
  __syscall_openat: ___syscall_openat,
  /** @export */
  __syscall_poll: ___syscall_poll,
  /** @export */
  __syscall_readlinkat: ___syscall_readlinkat,
  /** @export */
  __syscall_recvfrom: ___syscall_recvfrom,
  /** @export */
  __syscall_rmdir: ___syscall_rmdir,
  /** @export */
  __syscall_sendto: ___syscall_sendto,
  /** @export */
  __syscall_socket: ___syscall_socket,
  /** @export */
  __syscall_stat64: ___syscall_stat64,
  /** @export */
  __syscall_unlinkat: ___syscall_unlinkat,
  /** @export */
  _abort_js: __abort_js,
  /** @export */
  emscripten_asm_const_int: _emscripten_asm_const_int,
  /** @export */
  emscripten_asm_const_ptr: _emscripten_asm_const_ptr,
  /** @export */
  emscripten_console_error: _emscripten_console_error,
  /** @export */
  emscripten_console_log: _emscripten_console_log,
  /** @export */
  emscripten_console_warn: _emscripten_console_warn,
  /** @export */
  emscripten_err: _emscripten_err,
  /** @export */
  emscripten_resize_heap: _emscripten_resize_heap,
  /** @export */
  emscripten_set_click_callback_on_thread: _emscripten_set_click_callback_on_thread,
  /** @export */
  emscripten_set_dblclick_callback_on_thread: _emscripten_set_dblclick_callback_on_thread,
  /** @export */
  emscripten_set_keydown_callback_on_thread: _emscripten_set_keydown_callback_on_thread,
  /** @export */
  emscripten_set_keypress_callback_on_thread: _emscripten_set_keypress_callback_on_thread,
  /** @export */
  emscripten_set_keyup_callback_on_thread: _emscripten_set_keyup_callback_on_thread,
  /** @export */
  emscripten_set_mousedown_callback_on_thread: _emscripten_set_mousedown_callback_on_thread,
  /** @export */
  emscripten_set_mouseenter_callback_on_thread: _emscripten_set_mouseenter_callback_on_thread,
  /** @export */
  emscripten_set_mouseleave_callback_on_thread: _emscripten_set_mouseleave_callback_on_thread,
  /** @export */
  emscripten_set_mousemove_callback_on_thread: _emscripten_set_mousemove_callback_on_thread,
  /** @export */
  emscripten_set_mouseup_callback_on_thread: _emscripten_set_mouseup_callback_on_thread,
  /** @export */
  exit: _exit,
  /** @export */
  fd_close: _fd_close,
  /** @export */
  fd_read: _fd_read,
  /** @export */
  fd_seek: _fd_seek,
  /** @export */
  fd_write: _fd_write,
  /** @export */
  getaddrinfo: _getaddrinfo
};


// Argument name here must shadow the `wasmExports` global so
// that it is recognised by metadce and minify-import-export-names
// passes.
function applySignatureConversions(wasmExports) {
  // First, make a copy of the incoming exports object
  wasmExports = Object.assign({}, wasmExports);
  var makeWrapper_pp = (f) => (a0) => Number(f(BigInt(a0)));
  var makeWrapper__p = (f) => (a0) => f(BigInt(a0));
  var makeWrapper_p = (f) => () => Number(f());
  var makeWrapper_p_ = (f) => (a0) => Number(f(a0));

  wasmExports['malloc'] = makeWrapper_pp(wasmExports['malloc']);
  wasmExports['free'] = makeWrapper__p(wasmExports['free']);
  wasmExports['fflush'] = makeWrapper__p(wasmExports['fflush']);
  wasmExports['emscripten_stack_get_end'] = makeWrapper_p(wasmExports['emscripten_stack_get_end']);
  wasmExports['emscripten_stack_get_base'] = makeWrapper_p(wasmExports['emscripten_stack_get_base']);
  wasmExports['strerror'] = makeWrapper_p_(wasmExports['strerror']);
  wasmExports['_emscripten_stack_restore'] = makeWrapper__p(wasmExports['_emscripten_stack_restore']);
  wasmExports['_emscripten_stack_alloc'] = makeWrapper_pp(wasmExports['_emscripten_stack_alloc']);
  wasmExports['emscripten_stack_get_current'] = makeWrapper_p(wasmExports['emscripten_stack_get_current']);
  return wasmExports;
}


// include: postamble.js
// === Auto-generated postamble setup entry stuff ===

var calledRun;

function stackCheckInit() {
  // This is normally called automatically during __wasm_call_ctors but need to
  // get these values before even running any of the ctors so we call it redundantly
  // here.
  _emscripten_stack_init();
  // TODO(sbc): Move writeStackCookie to native to to avoid this.
  writeStackCookie();
}

function run() {

  if (runDependencies > 0) {
    dependenciesFulfilled = run;
    return;
  }

  stackCheckInit();

  preRun();

  // a preRun added a dependency, run will be called later
  if (runDependencies > 0) {
    dependenciesFulfilled = run;
    return;
  }

  function doRun() {
    // run may have just been called through dependencies being fulfilled just in this very frame,
    // or while the async setStatus time below was happening
    assert(!calledRun);
    calledRun = true;
    Module['calledRun'] = true;

    if (ABORT) return;

    initRuntime();

    Module['onRuntimeInitialized']?.();
    consumedModuleProp('onRuntimeInitialized');

    assert(!Module['_main'], 'compiled without a main, but one is present. if you added it from JS, use Module["onRuntimeInitialized"]');

    postRun();
  }

  if (Module['setStatus']) {
    Module['setStatus']('Running...');
    setTimeout(() => {
      setTimeout(() => Module['setStatus'](''), 1);
      doRun();
    }, 1);
  } else
  {
    doRun();
  }
  checkStackCookie();
}

function checkUnflushedContent() {
  // Compiler settings do not allow exiting the runtime, so flushing
  // the streams is not possible. but in ASSERTIONS mode we check
  // if there was something to flush, and if so tell the user they
  // should request that the runtime be exitable.
  // Normally we would not even include flush() at all, but in ASSERTIONS
  // builds we do so just for this check, and here we see if there is any
  // content to flush, that is, we check if there would have been
  // something a non-ASSERTIONS build would have not seen.
  // How we flush the streams depends on whether we are in SYSCALLS_REQUIRE_FILESYSTEM=0
  // mode (which has its own special function for this; otherwise, all
  // the code is inside libc)
  var oldOut = out;
  var oldErr = err;
  var has = false;
  out = err = (x) => {
    has = true;
  }
  try { // it doesn't matter if it fails
    _fflush(0);
    // also flush in the JS FS layer
    for (var name of ['stdout', 'stderr']) {
      var info = FS.analyzePath('/dev/' + name);
      if (!info) return;
      var stream = info.object;
      var rdev = stream.rdev;
      var tty = TTY.ttys[rdev];
      if (tty?.output?.length) {
        has = true;
      }
    }
  } catch(e) {}
  out = oldOut;
  err = oldErr;
  if (has) {
    warnOnce('stdio streams had content in them that was not flushed. you should set EXIT_RUNTIME to 1 (see the Emscripten FAQ), or make sure to emit a newline when you printf etc.');
  }
}

var wasmExports;

// With async instantation wasmExports is assigned asynchronously when the
// instance is received.
createWasm();

run();

// end include: postamble.js

