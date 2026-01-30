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
  return binaryDecode(' asm   ÜR`~~~`~`~~~~`~~~`~~~`~|`~~ ` `~~~`~~~~`~ `~~~~`~``~`~~`~~~`~~`~`~~`~`~~`~`  `~~``~~~~`~~`~|`~ `~| `~~`~~~~ `~~~~ `~~~~~ `~~~~~`|~~`~~`~~~ `~~~~ `~~~~ `~~~ `~~~~~~ `~~~~~~~ `~~~ `~~~~`~~`~`~~~ `~~~ `~~~ `~~~~`~~~`~~ `\n~~~~~~ `~~~~~`~~~~ `~~`~~ ` ~`\n~~~~~~~~~ `~~~`~~`~~`~`~~`~~~` `~~`||`|~|`~~~~~`~~~~~~~`~ `|~`~~|``~~~`~~~~~`~~~~`~`Á0envexit envemscripten_asm_const_int envgetaddrinfo 	envemscripten_asm_const_ptr envemscripten_console_log \nenvemscripten_console_warn \nenvemscripten_console_error \nenv*emscripten_set_keypress_callback_on_thread env)emscripten_set_keydown_callback_on_thread env\'emscripten_set_keyup_callback_on_thread env\'emscripten_set_click_callback_on_thread env+emscripten_set_mousedown_callback_on_thread env)emscripten_set_mouseup_callback_on_thread env*emscripten_set_dblclick_callback_on_thread env+emscripten_set_mousemove_callback_on_thread env,emscripten_set_mouseenter_callback_on_thread env,emscripten_set_mouseleave_callback_on_thread env__syscall_faccessat env__syscall_chdir wasi_snapshot_preview1fd_close \renv__syscall_fcntl64 env__syscall_openat env__syscall_ioctl wasi_snapshot_preview1fd_write wasi_snapshot_preview1fd_read env__syscall_getcwd wasi_snapshot_preview1fd_seek env__syscall_fstat64 env__syscall_stat64 env__syscall_newfstatat env__syscall_lstat64 env__syscall_poll envemscripten_err \nenv__syscall_getdents64 env__syscall_readlinkat env__syscall_unlinkat env__syscall_rmdir wasi_snapshot_preview1environ_sizes_get wasi_snapshot_preview1environ_get env	_abort_js envemscripten_resize_heap env__syscall_accept4 env__syscall_bind env__syscall_connect env__syscall_listen env__syscall_recvfrom env__syscall_sendto env__syscall_socket \n  !"! "#       $%   \n\n&\'("))*!++\n\n& ,-\n\n.\n /&01&\n2345&67&6&8&9!!& !#:;<& \n\n                                                   #                                                            ."	;/\r\r\n\n=% #>>># \r\r\r\r\r\r?/\n\n@AB;.>CDC;/\n E%%	  = FGH&1  IJ	\r=/\n   ;";;;,,K\nLMNMOPQ\n;%@\r\r\r\rp|| ~B~B ~B ~ BÏ~ BîÒÔmemory __wasm_call_ctors 0malloc ¤emscripten_create Aemscripten_eval_compiled Bfree ¦emscripten_eval_macros Demscripten_eval Eemscripten_destroy F__indirect_function_table htons Ífflush ´htonl Çntohs Åemscripten_stack_get_end ±emscripten_stack_get_base °strerror Äemscripten_stack_init ®emscripten_stack_get_free ¯_emscripten_stack_restore À_emscripten_stack_alloc Áemscripten_stack_get_current Â__start_em_asm\r__stop_em_asm	ü B{¯°±²³´µ¶·¸¹º»½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáãåæçèéêëìíîïðñòóôõö÷øùúûýþÿ ¡¢£¤¥·¸¹ºÿ\nÝ ®ùêO~# B}! $   7   )7    ) §6  B|A 6  B|$ È~# B}!@@  ( (GAqE\r  A Aq:  A 6@@ (  (HAqE\r  )  (¬|-  !A!  t u! )  (¬|-  !A!@   t uGAqE\r  A Aq:   (Aj6  AAq:  - Aq~# B}! $    7 A 6@@ ( (HAqE\r )  (¬|-  !A!  t u )ì   (Aj6  B|$ Q~# B}! $ B )ð¼ !   )7   ) 7   ³  B|$ ÿ~~# B }! B 7@@  (\r  B 7   ) ,  A-F: @ - AqE\r     ) B|7     (Aj6 A 6@@ (  (HAqE\r  )B\n~7  )  (¬|-  !A!   t uA0k¬ )|7  (Aj6 @ - AqE\r  )! B  }7  )7 )~# B}! B 7 A 6@@ (  (HAqE\r  )  (¬|-  !A!   t u¬ )B| )B| )}7  (Aj6  )Ã~\n# B }! B 7 A 6   ) ,  A-F: @ - AqE\r     ) B|7     (Aj6@ (  (H!A ! Aq! !@ E\r   )  (¬|-  !A!  t uA.G!@ AqE\r   +D      $@¢9  )  (¬|-  !A!	   	t 	uA0k· + 9  (Aj6@ (  (HAqE\r   (Aj6 D      ð?9 @@ (  (HAqE\r  + D      $@¢9   )  (¬|-  !\nA!  \n t uA0k· + £ + 9  (Aj6 @@ - AqE\r   +9  +9 +~~# B}! $    7  6@ ( )( )(kKAqE\r @@ )(E\r @@ ( )( )(kKAqE\r )!  (At6  ))  )(Aj­§ ! ) 7  (! )!   (j6 )(Aj­¤ ! ) 7  B|$ !    ) 7    (6  B|A 6 x~~# B}! $    7  :  )A¸  - ! )) ! )! (!  Aj6  ­| :   B|$ j~# B0}! $    7(  7  )(! ) ! B| ±   )7  )7   ¼  B0|$ ~~# B}! $    7 ) (¸  ))  )(­|! ) ! (­!@ P\r    ü\n   (! )!   (j6 B|$ ~~# B0}! $    7(  7   ) 7 A6@ )B SAqE\r   )B~7  (Aj6@@ )B\nYAqE\r  )B\n7  (Aj6  )( (¸  )()  )((­|! (Aj­!  ) 7   Bø  ý  (! )(!   (j6 B0|$ â~~# B0}! $    7(  7   ) 7 A6@@ )B\nZAqE\r  )B\n7  (Aj6  )( (¸  )()  )((­|! (Aj­!  ) 7   B  ý  (! )(!   (j6 B0|$ B~# B}! $    7  6 ) (­¾  B|$ ¾~~# B0}! $    7(  9   + 9 A6@@ +D      $@fAqE\r  +D      $@£9  (Aj6   (Aj6@@ + +ü¹¡B ¹dAqE\r  +D      $@¢9  (Aj6  )( (¸  )()  )((­|! (Aj­!  + 9   Bë  ý  (! )(!   (j6 B0|$ ¾~~~# B°	}!   $   A6¬	B !   ) 7	   ) 7	Bà!A !  B°|  ü   (¬	!  B	|!  !A !  B°|!    Aq  BðÒ   B°ü\n    B°	|$ q~~# B }! $    7  6 )! (! BðÒ   AAq 7 )Ã ! B |$  	~~~~~# B }! $    7B !  7  7 )! B|!A !A !    Aq Aq   (Aj­¤ 7  ) ! )! (­!	@ 	P\r    	ü\n   )  (­|A :  @B (ÔØ B (ÐØ MAqE\r @@B (ÔØ E\r @@B (ÔØ B (ÐØ MAqE\rB (ÔØ At!\nB  \n6ÔØ  B )ÈØ B (ÔØ ­B§ !B  7ÈØ A!B  6ÔØ B¤ !\rB  \r7ÈØ B )ÈØ B (ÐØ ­B|B|!B )ÈØ B (ÐØ ­B|!B (ÐØ B (ÐØ k­B!@ P\r    ü\n   ) !B )ÈØ B (ÐØ ­B| 7 B (ÐØ Aj!B  6ÐØ  )¦  ) ! B |$  V~# B}! $    7  6 )! (!BðÒ     B|$ õ~~# Bà }! $    7X  7P  )X7@  )X §6H BÀ |B|A 6   )P70  )P §68 B0|B|A 6 BðÒ   )H7   )@7  )87  )07BðÒ !A!   B| B|  7( )(Ã ! Bà |$  Þ~# B0}!   $ B Ø î B )¨Ø ¦   B 7   A 6(  A 6,  )(!B !  7°Ø    ) 7¨Ø B )¸Ø ¦   B 7  A 6  A 6  )!B !  7ÀØ    )7¸Ø BØØ    A 6@@  (B (ÐØ IAqE\rB )ÈØ   (­B|) ¦     (Aj6 B )ÈØ ¦   B0|$ ï~~~# Bà}! $   7Ø  6Ô  7È  7ÀB !   7   7 @ (Ô­BTAqE\r B )è¼ ! A½6  BÍ  ½ B )è¼ Bñ B ½ A    )Ø7° A6¸ B°|B|A 6  B 7  A6¨ B |B|A 6   )¸7P  )°7H  )¨7@  ) 78@ BÈ | B8|² Aq\r B )è¼ ! AÂ60 BÍ  B0|½ B )è¼ BÈ B ½ A    )Ø(6@ (Ô (GAqE\r B )è¼ !	 AÉ6 	BÍ  B|½ B )è¼ !\n (!  (Ô6$  6  \nB§  B |½ A   A6B !  7  7 )Ø!\r )È! B| \r B| È  )À! )!  )7  ) 7    )Ø (­|( 6    (6  (­B|§6    (­B¤ 7  A 6@@ (  (IAqE\rB !  7x  7p  7h  7`  )Ø (­|( 6h  (h6l  (­B|§6  )È (l­B§ì 7` A 6\\@@ (\\ (hIAqE\r )Ø! )È!  B| É ! )` (\\­B| ;   (\\Aj6\\  Bà |B|! )Ø! )È!   B| B| Ê   )  (­B|!  )x7  )p7  )h7  )`7   (Aj6  Bà|$ Ô~~~# BÀ }! $    78  70  7(  7  )0 )(( ­|( ! )8 6 )8(! )8 6 )(!  ( ­B|§6  )  )8(­B~§ì ! )8 7  A 6@@ ( )8(IAqE\r )(( !	 )8)  (­B~| 	6 )8)  (­B~|!\n )0! )(! ) !\r B|   \rË  \n )7 \n )7   (Aj6  BÀ |$ ~# BÀ }! $    78  70  7( )8! )0! )(! B|   Ë   ) 7  )7 B| Aÿÿq! BÀ |$  ×	~~~~	~# B}! $    7  7  7x  7p  7h ) )x( ­|( ! ) 6 )(! ) 6 )x!  ( ­B|§6  )(­B0~¤ !	 ) 	7  A 6d@@ (d )(IAqE\rB !\n  \n7X  \n7P  \n7H  \n7@  \n78  \n70  ) )x5 |( 60 )x!  5 B|>  50!@ BV\r @@@@@@@@@@@@@@@@@@@@@@ § 	\n\r   ) )x )hÉ ;8  ) )x( ­|) 78 )x!\r \r \r( ­B|§6   ) )x( ­|+ 98 )x!  ( ­B|§6  )! )x! )h! B |   Ë   ) 78  ((6@  ) )x( ­|( 6@ )x!  ( ­B|§6   )h (@­B§ì 78 A 6@@ ( (@IAqE\r ) )x )hÉ ! )8 (­B| ;   (Aj6   ) )x5 |( ;H )x!  5 B|>   ) )x5 |-  A G:  )x!  5 B|> @@ - AqE\r   ) )x )hÉ ;J Aÿÿ;J  ) )x( ­|( 68 )x!  ( ­B|§6  ) )x( ­|-  !A !  Aÿq AÿqGAq: @ )x!  ( ­B|§6   ) )x )hÉ ;8  ) )x )hÉ ;8  ) )x )hÉ ;8\r  ) )x )hÉ ;8  ) )x )hÉ ;8  ) )x )hÉ ;8\n  ) )x )hÉ ;8	  ) )x )hÉ ;8  ) )x( ­|( 68 )x!  ( ­B|§6  ) )x( ­|-  !A !  Aÿq AÿqGAq: 8 )x!  ( ­B|§6   ) )x( ­|( 68 )x!  ( ­B|§6   ) )x( ­|( 68 )x!  ( ­B|§6   ) )x( ­|( 6 )x!     ( ­B|§6 @ ( )p(OAqE\r B )è¼ !! A6  !BÍ  ½ B )è¼ !"  (6 "BØ  B|½ A    )p)  (­B~|7P  ) )x( ­|/ ;X )x!# # #( ­B|§6   ) )x( ­|/ ;Z )x!$ $ $( ­B|§6  ))  (d­B0~|!% % )X7( % )P7  % )H7 % )@7 % )87 % )07   (dAj6d  B|$ ä~~# B }! $   7  7  7   ) )( ­|( 6 )!  ( ­B|§6    )  (ì 7  A 6@@ (  (IAqE\r ) )( ­|-  !  )  (­| :   )!  ( Aj6   (Aj6  B |$ À~~~~~~~# Bà}! $   7Ø  6Ô  7È  7ÀB !   7   7 @ (Ô­BTAqE\r B )è¼ ! A©6  BÍ  ½ B )è¼ Bñ B ½ A    )Ø7° A6¸ B°|B|A 6  B 7  A6¨ B |B|A 6   )¸7X  )°7P  )¨7H  ) 7@@ BÐ | BÀ |² Aq\r B )è¼ ! A®60 BÍ  B0|½ B )è¼ BÈ B ½ A    )Ø(6@ (Ô (GAqE\r B )è¼ !	 Aµ6 	BÍ  B|½ B )è¼ !\n (!  (Ô6$  6  \nB§  B |½ A   A6B !  7  7 )Ø!\r )À! B| \r B| È @ )ÈB RAqE\r @ )È( )È( (jIAqE\r  )È( (j! )È 6@@ )È(\r  )È(­B¤ ! )È 7  )È)  )È(­B§ ! )È 7  A 6@@ ( (IAqE\r  )ÀAì 7x ) (­B~|(! )x 6 )À )x(ì ! )x 7  )x) ! ) (­B~|) ! )x(­!@ P\r    ü\n   )x! )È) ! )È! (!  Aj6  ­B| 7   (Aj6    )Ø (­|( 6    (6  (­B|§6   )À  (­B8~§ì 7  A 6t@@ (t  (IAqE\r   )  (t­B8~|7h )Ø! )À!  B| É ! )h ;  )Ø (­|( ! )h 6  (­B|§6 )À )h(­B§ì ! )h 7 A 6d@@ (d )h(IAqE\r )Ø!  )À!!   B| !É !" )h) (d­B| ";   (dAj6d  )hB|!# )Ø!$ )À!% # $ B| B| %Í  )Ø (­|-  !&A !\' &Aÿq \'AÿqG!( )h (Aq: (  (­B|§6  (tAj6t  Bà|$ ~~# B0}! $    7(  7   7  7  7 )  )( ­|( ! )( 6 )!  ( ­B|§6  ) )((­B§ì ! )( 7  A 6@@ ( )((IAqE\r )  ) ) )Î !	 )()  (­B| 	7   (Aj6  B0|$ Ð~~~|~~~~~~\r~~~# Bð }! $    7h  7`  7X  7P  )PAÀ ì 7H )h )`5 |( ! )H 6  )`!  5 B|>  )H5 !@ BV\r @@@@@@@@@@@@@@@@@@ § 	\n\r  )h )` )PÉ ! )H ; )h )`( ­|) !	 )H 	7 )`!\n \n \n( ­B|§6  )h )`( ­|+ ! )H 9 )`!  ( ­B|§6  )h!\r )`! )P! B8| \r  Ë  )8! )H 7 (@! )H 6 )HB| )h )` )X )PÍ \r )h )` )PÉ ! )H ; )h )`( ­|( ! )H 6 )H(! )H 6 )`!  ( ­B|§6  )P )H(­B§ì ! )H 7 A 64@@ (4 )H(IAqE\r )h )` )PÉ ! )H) (4­B| ;   (4Aj64  )HB| )h )` )X )PÍ   )h )`5 |-  A G: 3 )`!  5 B|> @@ - 3AqE\r  )h )` )PÉ ! )H ;( )HAÿÿ;( )HB| )h )` )X )PÍ \n )HB| )h )` )X )PÍ 	 )HB| )h )` )X )PÍ  )h )` )X )PÎ ! )H 7 )h )` )X )PÎ ! )H 7 )h )` )X )PÎ ! )H 7 )h )` )PÉ ! )H ; )h )` )X )PÎ ! )H 7 )h )` )X )PÎ ! )H 7 )HB|B| )h )` )X )PÍ  )h )`( ­|-  ! A !!  Aÿq !AÿqG!" )H "Aq:   )`!# # #( ­B|§6  )h )` )PÉ !$ )H $; )h )` )X )PÎ !% )H %7  )h )`5 |-  A G: 2 )`!& & &5 B|> @ - 2AqE\r  )h )` )X )PÎ !\' )H \'7 )h )` )X )PÎ !( )H (7 )HB|B| )h )` )X )PÍ  )HB|B| )h )` )X )PÍ  )h )` )X )PÎ !) )H )7  )h )`( ­|( 6(  ((6, )`!* * *( ­B|§6   )P (,­B§ì 7  A 6@@ ( ((IAqE\r )h )` )X )PÎ !+ )  (­B| +7  )h )` )X )PÎ !, )  (­B| ,7  (Aj6  )HB|B|!- - )(7 - ) 7   )h )`( ­|( 6 )`!. . .( ­B|§6 @ ( )X(OAqE\r B )è¼ !/ A6  /BÍ  ½ B )è¼ !0  (6 0BØ  B|½ A   )X)  (­B~|!1 )H 170 )h )`( ­|/ !2 )H 2;8 )`!3 3 3( ­B|§6  )h )`( ­|/ !4 )H 4;: )`!5 5 5( ­B|§6  )H!6 Bð |$  6ì~# B0}! $    7   7@@ ) B RAq\r  B 7( B 7  B|7  ) 7 @@ ) B RAqE\r )B|Aì ! ) 7  ) )  )Ð ! ))  7   )) B|7  ) )7    )7( )(! B0|$  ¨	~~~# B }! $    7  7  )Ñ 7 )! )!  ) 7   )7  )7  )7  ) 7  )! ) 7 )A; @@ )( AFAqE\r  )B|Aì ! ) 7 ))) )Ï ! )) 7@@ )( AFAqE\r  )(! ) 6 )B| )(ì !	 ) 	7 ))!\n ))! )(­!@ P\r  \n  ü\n  @@ )( A	FAqE\r  )(!\r ) \r6 )B| )(ì ! ) 7 ))! ))! )(­!@ P\r    ü\n  @@ )( AFAqE\r  )) )Ò ! ) 7@@ )( AFAqE\r  ))!  (4Aj64@ ) ))RAqE\r  )B|AÀ ì ! ) 7 ))! ))!  )878  )070  )(7(  ) 7   )7  )7  )7  ) 7  )B| ))( ­B~§ì ! )) 7 A 6@@ ( ))( IAqE\r ))) (­B~|! ))) (­B~|!  )7  ) 7  ))) (­B~|) )Ð ! ))) (­B~| 7  (Aj6  ))A: 8@ )( AFAqE\r  ))!  (HAj6H@ ) ))RAqE\r  )B|AÐ ì ! ) 7 )) ))BÐ ü\n   )B|Aèì ! )) 7@ )))@ )))@Bèü\n   )! B |$  û\n~# B}! $    7  )B|A(ì 7 @ )( )(MAqE\r @@ )(E\r @@ )( )(MAqE\r )!  (At6  ))  )(­B§ ! ) 7  )A6B¤ ! ) 7  ))  )(­B|B|! ))  )(­B|! )( )(k­B!@ P\r    ü\n   ) ! ))  )(­B| 7  )!	 	 	(Aj6 ) !\n B|$  \n~# BÀ }! $    78  70  )0B|AÐ ì 7( A 6$@@ ($A\nIAqE\r  )8 ($­B|) 7@@ )B RAqE\r  ))  )0Ð 7  )) )0Ð 7 )0 )( ) )   ))7   ($Aj6$  )(! BÀ |$  Ó~# BÀ }! $    78  )8Ñ 70 )0! A 6 B|B|A 6  B|B|!B !  7  7   )87  A ;( A ;* B|B$|A 6   )(7   ) 7  )7  )7  )7  )0! BÀ |$  Ó~# BÀ }! $    78  70  )0Ñ 7( )(! A6  B|A 6  B|!  )87 B|B 7   )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  Õ~# BÀ }! $   78  )8Ñ 70 )0! A6 B|B|A 6  B|B|!   )7   ) 7   )87  A ;( A ;* B|B$|A 6   )(7   ) 7  )7  )7  )7  )0! BÀ |$  Õ~# BÀ }! $   78  )8Ñ 70 )0! A	6 B|B|A 6  B|B|!   )7   ) 7   )87  A ;( A ;* B|B$|A 6   )(7   ) 7  )7  )7  )7  )0! BÀ |$  Ó~# BÀ }! $    78  70  )0Ñ 7( )(! A6  B|A 6  B|!  )87 B|B 7   )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  Ó~# BÀ }! $    98  70  )0Ñ 7( )(! A6  B|A 6  B|!  +89 B|B 7   )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  å~# BÀ }! $    Aq: ?  70  )0Ñ 7( )(! A6  A 6 B|!  - ?Aq:  B|!B !  7   7    )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  Ó~# BÀ }! $    78  70  )0Ñ 7( )(! A6  B|A 6  B|!  )87 B|B 7   )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  Ý~# BÀ }! $    78  70  )0Ñ 7( )8A64 )(! A6  B|A 6  B|!  )87 B|B 7   )07 A ;  A ;" B$|A 6   ) 7   )7  )7  )7  ) 7  )(! BÀ |$  Í~~# B }! $    7  7  )Ñ 7  )B|AÐ ì 7 )!BÐ !A ! B0|  ü BÐ !  B0| ü\n   )! ) 7@ )A6H )! A6 B|B|A 6  B|B|!	  )7 	B|B 7   )7  A ;( A ;* B|B$|A 6   )(7   ) 7  )7  )7  )7  )!\n B |$  \nä~# B0}! $    7(@@ )(( AFAqE\r   )())7 @@ ) B RAqE\r  ) )7 ) ) Ý   )7  @@ )(( AFAqE\r  A 6@@ (A\nIAqE\r  )() (­B|) 7@@ )B RAqE\r )) Ý  ))Ý   ))7   (Aj6 @ )(( AFAqE\r  )()! (HAj!  6H@ \r @ )()) B RAqE\r  )()) ¦ @ )())B RAqE\r  )())¦ @ )()) B RAqE\r  )()) ¦  A 6@@ ( )()(8IAqE\r )())0 (­B|B8|î   (Aj6 @ )())0B RAqE\r  )())0¦  )())@  B0|$ Í~~# Bð }! $    7`  7X@@ )`(  )X( GAqE\r  A Aq: o )`5 !@ B	V\r @@@@@@@@@ §\n 	  AAq: o	  )`))7P  )X))7H@ )PB R!A ! Aq! !@ E\r  )HB R!@ AqE\r @ )P)  )H) Þ Aq\r  A Aq: o  )P)7P  )H)7H )PB Q!A !	 Aq!\n 	!@ \nE\r  )HB Q!  Aq: o )`B|! )XB|!\r  )7   ) 7  \r)7  \r) 7  B| B|² Aq: o  )`) )X)QAq: o  )`+ )X+aAq: o  )`- Aq )X- AqFAq: o A 6D@@ (DA\nIAqE\r  )`) (D­B|) 78  )X) (D­B|) 70@ )8B R!A ! Aq! !@ E\r  )0B R!@ AqE\r @@ )8)  )0) Þ AqE\r  )8) )0)Þ Aq\r A Aq: o@ )8 )0RAqE\r  A Aq: o  (DAj6D  AAq: o@ )`)/0AÿÿqAÿÿGAqE\r   )`)/0Aÿÿq )X)/0AÿÿqFAq: o A Aq: o@ )`( )X(GAqE\r  A Aq: o A 6,@@ (, )`(IAqE\r@ )`) (,­|-  Aÿq )X) (,­|-  AÿqGAqE\r  A Aq: o  (,Aj6,  AAq: o A Aq: o - oAq! Bð |$  ~# B}! $    7 A 6@@ ( )(IAqE\r ))  (­B|) Ý   (Aj6 @ )) B RAqE\r  )) ¦  )A 6 )B|î @ ))B RAqE\r  ))¦ @ ))(B RAqE\r  ))(¦  )A 6  )¦  B|$ ~# BÀ }! $   78  70B !   7   7 @  (  (MAqE\r @@  (E\r @@  (  (MAqE\r    (At6     )   (­B§ 7   A6  B ¤ 7   )   (­B|B |!  )   (­B|!  (  (k­B!@ P\r    ü\n    )   (­B|!B !	  	7(  	7   	7  	7  )(7  ) 7  )7  )7     (Aj6 A 6 A 6@@ (Aj )8(IAqE\r   )8)  (­B|)  )0A  B|AAqá   (Aj6 @ )8(A KAqE\r    )8)  )8(Ak­B|)  )0A  B|A Aqá  BÀ |$ Ñ°-3~~&~~~~~~~~$~~&~~~~~~~~~~=~# B	}! $    7	  7	  7ø  6ô  7è  Aq: ç )	5 !@ BV\r @@@@@@@@@@@@@@@@@@ § 	\n\r B !  7Ø  7Ð  7È  7À  7¸  7° A 6°  )	/;¸ B°|B |!	 )	B0|!\n 	 \n)7 	 \n) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ !\r )	)  (ô­B| \r7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )Ø7(  )Ð7   )È7  )À7  )¸7  )°7  )	)  (ô­B|!  (Aj6B !  7¨  7   7  7  7  7 A6  )	)7 B|B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ ! )	)  (ô­B| 7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )¨7(  ) 7   )7  )7  )7  )7  )	)  (ô­B|!  (Aj6B !  7ø  7ð  7è  7à  7Ø  7Ð A6Ð  )	+9Ø BÐ|B |! )	B0|!    )7   ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!! ! !(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !" )	)  (ô­B| "7 )	)  (ô­B|A6B0¤ !# )	)  (ô­B| #7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!$ )	)  (ô­B|) )	)  (ô­B|(­B0~|!% )	)  (ô­B|( )	)  (ô­B|(k­B0~!&@ &P\r  $ % &ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!\' \' )ø7( \' )ð7  \' )è7 \' )à7 \' )Ø7 \' )Ð7  )	)  (ô­B|!( ( ((Aj6B !)  )7È  )7À  )7¸  )7°  )7¨  )7  A6  B |B|!* )	B|!+ * +)7 * +) 7  B |B |!, )	B0|!- , -)7 , -) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!. . .(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !/ )	)  (ô­B| /7 )	)  (ô­B|A6B0¤ !0 )	)  (ô­B| 07 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!1 )	)  (ô­B|) )	)  (ô­B|(­B0~|!2 )	)  (ô­B|( )	)  (ô­B|(k­B0~!3@ 3P\r  1 2 3ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!4 4 )È7( 4 )À7  4 )¸7 4 )°7 4 )¨7 4 ) 7  )	)  (ô­B|!5 5 5(Aj6 )	!6 )	B|!7 )ø!8 (ô!9 )è!: - ç!;A !< 6 7 8 9 : ;Aq <Aqâ \rB !=  =7  =7  =7  =7  =7ø  =7ð A6ð  )	/;ø Bð|B |!> )	B0|!? > ?)7 > ?) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!@ @ @(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !A )	)  (ô­B| A7 )	)  (ô­B|A6B0¤ !B )	)  (ô­B| B7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!C )	)  (ô­B|) )	)  (ô­B|(­B0~|!D )	)  (ô­B|( )	)  (ô­B|(k­B0~!E@ EP\r  C D Eü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!F F )7( F )7  F )7 F )7 F )ø7 F )ð7  )	)  (ô­B|!G G G(Aj6B !H  H7è  H7à  H7Ø  H7Ð  H7È  H7À A6À BÀ|B|!I )	B|!J I J)7 I J) 7   )	(;Ø  )	/(;Ú BÀ|B |!K )	B0|!L K L)7 K L) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!M M M(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !N )	)  (ô­B| N7 )	)  (ô­B|A6B0¤ !O )	)  (ô­B| O7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!P )	)  (ô­B|) )	)  (ô­B|(­B0~|!Q )	)  (ô­B|( )	)  (ô­B|(k­B0~!R@ RP\r  P Q Rü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!S S )è7( S )à7  S )Ø7 S )Ð7 S )È7 S )À7  )	)  (ô­B|!T T T(Aj6@ )	/(AÿÿqAÿÿFAqE\r B !U  U7¸  U7°  U7¨  U7  B |!V )	B|!W V W)7 V W) 7 @ )	( )	(MAqE\r @@ )	(E\r @@ )	( )	(MAqE\r )	!X X X(At6  )	)  )	(­B§ !Y )	 Y7  )	A6B ¤ !Z )	 Z7  )	)  )	(­B|B |![ )	)  )	(­B|!\\ )	( )	(k­B!]@ ]P\r  [ \\ ]ü\n   )	)  )	(­B|!^ ^ )¸7 ^ )°7 ^ )¨7 ^ ) 7  )	!_ _ _(Aj6 A 6 )	!` )	B|B|!a )ø!b )	(Ak!c B|!dA !e ` a b c d eAq eAqâ  )	!f )	B|!g )ø!h (ô!i )è!jA !kA!l f g h i j kAq lAqâ B !m  m7  m7  m7  m7ø  m7ð  m7è A6è  )	(6ð Bè|B |!n )	B0|!o n o)7 n o) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!p p p(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !q )	)  (ô­B| q7 )	)  (ô­B|A6B0¤ !r )	)  (ô­B| r7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!s )	)  (ô­B|) )	)  (ô­B|(­B0~|!t )	)  (ô­B|( )	)  (ô­B|(k­B0~!u@ uP\r  s t uü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!v v )7( v )7  v )7 v )ø7 v )ð7 v )è7  )	)  (ô­B|!w w w(Aj6\n )	!x )	B|!y )ø!z (ô!{ )è!|A !}A!~ x y z { | }Aq ~Aqâ B !  7à  7Ø  7Ð  7È  7À  7¸ A6¸  )	(Av6À B¸|B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ ! )	)  (ô­B| 7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )à7(  )Ø7   )Ð7  )È7  )À7  )¸7  )	)  (ô­B|!  (Aj6	 )	! )	B|! )ø! (ô! )è!A !A!      Aq Aqâ B !  7°  7¨  7   7  7  7 A6  )	(6 B|B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ ! )	)  (ô­B| 7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )°7(  )¨7   ) 7  )7  )7  )7  )	)  (ô­B|!  (Aj6 )	 )	) )ø (ô )èA Aqá  )	 )	) )ø (ô )èA Aqá  )	 )	) )ø (ô )èA Aqá B !  7  7ø  7ð  7è  7à  7Ø A6Ø BØ|B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !  )	)  (ô­B|  7 )	)  (ô­B|A6B0¤ !¡ )	)  (ô­B| ¡7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!¢ )	)  (ô­B|) )	)  (ô­B|(­B0~|!£ )	)  (ô­B|( )	)  (ô­B|(k­B0~!¤@ ¤P\r  ¢ £ ¤ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!¥ ¥ )7( ¥ )ø7  ¥ )ð7 ¥ )è7 ¥ )à7 ¥ )Ø7  )	)  (ô­B|!¦ ¦ ¦(Aj6 )	 )	) )ø (ô )èA Aqá B !§  §7Ð  §7È  §7À  §7¸  §7°  §7¨ A6¨  )	/;° B¨|B |!¨ )	B0|!© ¨ ©)7 ¨ ©) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!ª ª ª(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !« )	)  (ô­B| «7 )	)  (ô­B|A6B0¤ !¬ )	)  (ô­B| ¬7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!­ )	)  (ô­B|) )	)  (ô­B|(­B0~|!® )	)  (ô­B|( )	)  (ô­B|(k­B0~!¯@ ¯P\r  ­ ® ¯ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!° ° )Ð7( ° )È7  ° )À7 ° )¸7 ° )°7 ° )¨7  )	)  (ô­B|!± ± ±(Aj6 )	!² )	)!³ )ø!´ (ô!µ )è!¶A !· ² ³ ´ µ ¶ ·á  )	) !¸ 5ô!¹B!º  ¸ ¹ º|(6¤ )	 )	B| )ø (ô )è ·Aâ B !»  »7  »7  »7  »7  »7ø  »7ð A6ð  )	(6ø  )	)  5ô º|( (¤k6ü  - çAq:  Bð|B |!¼ )	B0|!½ ¼ ½)7 ¼ ½) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!¾ ¾ ¾(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !¿ )	)  (ô­B| ¿7 )	)  (ô­B|A6B0¤ !À )	)  (ô­B| À7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!Á )	)  (ô­B|) )	)  (ô­B|(­B0~|!Â )	)  (ô­B|( )	)  (ô­B|(k­B0~!Ã@ ÃP\r  Á Â Ãü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ä Ä )7( Ä )7  Ä )7 Ä )7 Ä )ø7 Ä )ð7  )	)  (ô­B|!Å Å Å(Aj6 )	 )	) )ø (ô )èA Aqá B !Æ  Æ7è  Æ7à  Æ7Ø  Æ7Ð  Æ7È  Æ7À A6À  )	/;È BÀ|B |!Ç )	B0|!È Ç È)7 Ç È) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!É É É(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !Ê )	)  (ô­B| Ê7 )	)  (ô­B|A6B0¤ !Ë )	)  (ô­B| Ë7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!Ì )	)  (ô­B|) )	)  (ô­B|(­B0~|!Í )	)  (ô­B|( )	)  (ô­B|(k­B0~!Î@ ÎP\r  Ì Í Îü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ï Ï )è7( Ï )à7  Ï )Ø7 Ï )Ð7 Ï )È7 Ï )À7  )	)  (ô­B|!Ð Ð Ð(Aj6@ )	)B RAqE\r  )	 )	) )ø (ô )èA Aqá B !Ñ  Ñ7¸  Ñ7°  Ñ7¨  Ñ7   Ñ7  Ñ7 A6  )	)B RAq:  B|B |!Ò )	B0|!Ó Ò Ó)7 Ò Ó) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!Ô Ô Ô(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !Õ )	)  (ô­B| Õ7 )	)  (ô­B|A6B0¤ !Ö )	)  (ô­B| Ö7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!× )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ø )	)  (ô­B|( )	)  (ô­B|(k­B0~!Ù@ ÙP\r  × Ø Ùü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ú Ú )¸7( Ú )°7  Ú )¨7 Ú ) 7 Ú )7 Ú )7  )	)  (ô­B|!Û Û Û(Aj6B !Ü  Ü7  Ü7 B|!ÝAì !ÞA!ß Ý Þ ßt ßuº  )è!à à( !á à áAj6  B| á¿  Bè|  )7  )7  Bè| ¹   )ð7  )è7  B| ;þ A6 )è!â â( !ã â ãAj6  B| ã¿  BÐ|  )7(  )7  BÐ| B |¹   )Ø78  )Ð70  B0| ;æ )¦  )	 )	) )ø (ô )èA Aqá B !ä  ä7È  ä7À  ä7¸  ä7°  ä7¨  ä7  A6   /þ;¨ B |B |!å )	B0|!æ å æ)7 å æ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!ç ç ç(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !è )	)  (ô­B| è7 )	)  (ô­B|A6B0¤ !é )	)  (ô­B| é7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!ê )	)  (ô­B|) )	)  (ô­B|(­B0~|!ë )	)  (ô­B|( )	)  (ô­B|(k­B0~!ì@ ìP\r  ê ë ìü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!í í )È7( í )À7  í )¸7 í )°7 í )¨7 í ) 7  )	)  (ô­B|!î î î(Aj6 )	!ï )	B|!ð )ø!ñ (ô!ò )è!ó - ç!ôA !õ ï ð ñ ò ó ôAq õAqâ  A	6   /æ;¨ B |B |!ö )	B0|!÷ ö ÷)7 ö ÷) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!ø ø ø(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !ù )	)  (ô­B| ù7 )	)  (ô­B|A6B0¤ !ú )	)  (ô­B| ú7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!û )	)  (ô­B|) )	)  (ô­B|(­B0~|!ü )	)  (ô­B|( )	)  (ô­B|(k­B0~!ý@ ýP\r  û ü ýü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!þ þ )È7( þ )À7  þ )¸7 þ )°7 þ )¨7 þ ) 7  )	)  (ô­B|!ÿ ÿ ÿ(Aj6 A6   /þ;¨ B |B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ ! )	)  (ô­B| 7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )È7(  )À7   )¸7  )°7  )¨7  ) 7  )	)  (ô­B|!  (Aj6 )	! )	B |! )ø! (ô! )è! - ç!A !      Aq Aqâ  A6   /æ;¨ B |B |! )	B0|!  )7  ) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!  (At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ ! )	)  (ô­B| 7 )	)  (ô­B|A6B0¤ ! )	)  (ô­B| 7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|! )	)  (ô­B|) )	)  (ô­B|(­B0~|! )	)  (ô­B|( )	)  (ô­B|(k­B0~!@ P\r    ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!  )È7(  )À7   )¸7  )°7  )¨7  ) 7  )	)  (ô­B|!  (Aj6B !  7  7 B|!Aì !A!   t uº  )è! ( !    Aj6  B|  ¿  Bø|  )7h  )7` Bø| Bà |¹   )7x  )ø7p  Bð | ; )	 )	) )ø (ô )èA Aqá B !¡  ¡7ð  ¡7è  ¡7à  ¡7Ø  ¡7Ð  ¡7È A\r6È BÈ|B |!¢ )	B0|!£ ¢ £)7 ¢ £) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!¤ ¤ ¤(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !¥ )	)  (ô­B| ¥7 )	)  (ô­B|A6B0¤ !¦ )	)  (ô­B| ¦7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!§ )	)  (ô­B|) )	)  (ô­B|(­B0~|!¨ )	)  (ô­B|( )	)  (ô­B|(k­B0~!©@ ©P\r  § ¨ ©ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!ª ª )ð7( ª )è7  ª )à7 ª )Ø7 ª )Ð7 ª )È7  )	)  (ô­B|!« « «(Aj6 A 6Ä@@ (Ä )	(IAqE\r A6 )è!¬ ¬( !­ ¬ ­Aj6  B| ­¿  B°|  )7H  )7@ B°| BÀ |¹   )¸7X  )°7P  BÐ | ;Â@@ )	) (Ä­B|) B RAqE\r  )	 )	) (Ä­B|)  )ø (ô )èA Aqá  )	 )	) )ø (ô )èA Aqá  A6È  /Â;Ð BÈ|B |!® )	B0|!¯ ® ¯)7 ® ¯) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!° ° °(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !± )	)  (ô­B| ±7 )	)  (ô­B|A6B0¤ !² )	)  (ô­B| ²7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!³ )	)  (ô­B|) )	)  (ô­B|(­B0~|!´ )	)  (ô­B|( )	)  (ô­B|(k­B0~!µ@ µP\r  ³ ´ µü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!¶ ¶ )ð7( ¶ )è7  ¶ )à7 ¶ )Ø7 ¶ )Ð7 ¶ )È7  )	)  (ô­B|!· · ·(Aj6 )	 )	) 5ÄB|) )ø (ô )è - çAqá  A	6È  /;Ð BÈ|B |!¸ )	B0|!¹ ¸ ¹)7 ¸ ¹) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!º º º(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !» )	)  (ô­B| »7 )	)  (ô­B|A6B0¤ !¼ )	)  (ô­B| ¼7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!½ )	)  (ô­B|) )	)  (ô­B|(­B0~|!¾ )	)  (ô­B|( )	)  (ô­B|(k­B0~!¿@ ¿P\r  ½ ¾ ¿ü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!À À )ð7( À )è7  À )à7 À )Ø7 À )Ð7 À )È7  )	)  (ô­B|!Á Á Á(Aj6 A6È  /Â;Ð BÈ|B |!Â )	B0|!Ã Â Ã)7 Â Ã) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!Ä Ä Ä(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !Å )	)  (ô­B| Å7 )	)  (ô­B|A6B0¤ !Æ )	)  (ô­B| Æ7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!Ç )	)  (ô­B|) )	)  (ô­B|(­B0~|!È )	)  (ô­B|( )	)  (ô­B|(k­B0~!É@ ÉP\r  Ç È Éü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ê Ê )ð7( Ê )è7  Ê )à7 Ê )Ø7 Ê )Ð7 Ê )È7  )	)  (ô­B|!Ë Ë Ë(Aj6  (ÄAj6Ä  )¦  A6È  /;Ð BÈ|B |!Ì )	B0|!Í Ì Í)7 Ì Í) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!Î Î Î(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !Ï )	)  (ô­B| Ï7 )	)  (ô­B|A6B0¤ !Ð )	)  (ô­B| Ð7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!Ñ )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ò )	)  (ô­B|( )	)  (ô­B|(k­B0~!Ó@ ÓP\r  Ñ Ò Óü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ô Ô )ð7( Ô )è7  Ô )à7 Ô )Ø7 Ô )Ð7 Ô )È7  )	)  (ô­B|!Õ Õ Õ(Aj6 A6È BÈ|B |!Ö )	B0|!× Ö ×)7 Ö ×) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!Ø Ø Ø(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !Ù )	)  (ô­B| Ù7 )	)  (ô­B|A6B0¤ !Ú )	)  (ô­B| Ú7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!Û )	)  (ô­B|) )	)  (ô­B|(­B0~|!Ü )	)  (ô­B|( )	)  (ô­B|(k­B0~!Ý@ ÝP\r  Û Ü Ýü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!Þ Þ )ð7( Þ )è7  Þ )à7 Þ )Ø7 Þ )Ð7 Þ )È7  )	)  (ô­B|!ß ß ß(Aj6B !à  à7¨  à7   à7  à7  à7  à7 A6 B|B |!á )	B0|!â á â)7 á â) 7 @ )	)  (ô­B|( )	)  (ô­B|(MAqE\r @@ )	)  (ô­B|(E\r @@ )	)  (ô­B|( )	)  (ô­B|(MAqE\r )	)  (ô­B|!ã ã ã(At6  )	)  (ô­B|) )	)  (ô­B|(­B0~§ !ä )	)  (ô­B| ä7 )	)  (ô­B|A6B0¤ !å )	)  (ô­B| å7 )	)  (ô­B|) )	)  (ô­B|(­B0~|B0|!æ )	)  (ô­B|) )	)  (ô­B|(­B0~|!ç )	)  (ô­B|( )	)  (ô­B|(k­B0~!è@ èP\r  æ ç èü\n   )	)  (ô­B|) )	)  (ô­B|(­B0~|!é é )¨7( é ) 7  é )7 é )7 é )7 é )7  )	)  (ô­B|!ê ê ê(Aj6 B	|$ ~# B0}! $    7(  7   7  6  7  :   :  A 6 @@ ( Aj ) (IAqE\r )( ) )  5 B|)  ) ( ) - AsAqá   ( Aj6  @ ) (A KAqE\r  )(! ) !	  	)  	(Aj­B|)  ) ( ) - Aqá  B0|$ ä~~~~# B}! $    7x  7p  7h )pA6  A6d  (d­¤ 7XB !  7P  7H )p! )h! BØ | Bä |   BÈ |ä   )p( 6D A 6@B !  78  70 A 6, A 6(@@ (( )x(IAqE\r  ((6   (( (@k6$@ (< (8MAqE\r @@ (<E\r @@ (< (8MAqE\r  (<At6<   )0 (<­B§ 70 A6< B¤ 70 )0 (8­B|B|! )0 (8­B|!	 (8 (8k­B!\n@ \nP\r   	 \nü\n   )0 (8­B| ) 7   (8Aj68  ((Aj6(  )p!A BØ | Bä | å  )p!  ( ­B|§6  A 6@@ ( )x(IAqE\r  )x)  (­B|7 )p!\rA BØ | Bä | \rå  )(! )X )p( ­| 6  )p!  ( ­B|§6  A 6@@ ( )(IAqE\r ))  (­B|/ ! )p! BØ |! Bä |! Aÿÿq   æ   (Aj6  )B|! )p!  BØ | Bä |  BÈ | B0|ç   (,Aj6,  (Aj6  (,! )X (D­| 6 @ )HB RAqE\r  )H¦ B (  ! )X 6  )p( ! )X 6 )X! B|$  ~~# BÐ }! $    7H  7@  78  70  7( )H! )@! )8!A   å  )0(!	 )H)  )8( ­| 	6  )8!\n \n \n( ­B|§6  A 6$@@ ($ )0(IAqE\r B|! )0)  ($­B|) !  )7  ) 7   )8( 6 B|B|A 6 @ )(( )((MAqE\r @@ )((E\r @@ )(( )((MAqE\r )(!\r \r \r(At6  )()  )((­B~§ ! )( 7  )(A6B¤ ! )( 7  )()  )((­B~|B|! )()  )((­B~|! )(( )((k­B~!@ P\r    ü\n   )()  )((­B~|!  )7  )7  )7  )(!  (Aj6 )0)  ($­B|)  )H )@ )8è   ($Aj6$  BÐ |$ Ê~~# B0}! $    6,  7   7  7  )( 6@@ )(  (,j (KAqE\r  (At6 @ ( )( GAqE\r  (! ) 6  ) )  )( ­§ ! )  7  B0|$ |~~# B0}! $    ;.  7   7  7 /.!  Aÿÿq  ) ! )! )!    è  B0|$ Ë7~~~~|~~~~~~~~~~~~~~~~~~~~~~~~# B°}! $    7¨  7   7  7  7  7 ) ! )! )!	A   	å  )¨(!\n ) )  )( ­| \n6  )!  ( ­B|§6  A 6|@@ (| )¨(IAqE\r  )¨)  5|B0~|7p ) ! )!\r )!A  \r å  )p( ! ) )  )5 | 6  )!  5 B|>  )p5 !@ BV\r @@@@@@@@@@@@@@@@@@@@@@ § 	\n\r  )p/! ) ! )! )! Aÿÿq   æ  ) ! )! )!A   å  )p)! ) )  )( ­| 7  )!  ( ­B|§6  ) ! )! )!A   å  )p+! ) )  )( ­| 9  )!  ( ­B|§6   )p)7`  )p(6h Bà |B|A 6  ) !  )!! )!" Bà |   ! "è  ) !# )!$ )!%A # $ %å  )p(!& ) )  )( ­| &6  )!\' \' \'( ­B|§6  A 6\\@@ (\\ )p(IAqE\r )p) (\\­B|/ !( ) !) )!* )!+ (Aÿÿq ) * +æ   (\\Aj6\\   )p/Aÿÿq6X A 6T@@ (T )(IAqE\r@ ))  (T­B|(  )p/AÿÿqFAqE\r   ))  (T­B|(6X  (TAj6T  ) !, )!- )!.A , - .å  (X!/ ) )  )( ­| /6  )!0 0 0( ­B|§6  ) !1 )!2 )!3A 1 2 3å  )p/AÿÿqAÿÿGAq!4 ) )  )( ­| 4:   )!5 5 5( ­B|§6 @ )p/AÿÿqAÿÿGAqE\r  )p/!6 ) !7 )!8 )!9 6Aÿÿq 7 8 9æ  ) !: )!; )!<A : ; <å  )p(!= ) )  )5 | =6  )!> > >5 B|>  ) !? )!@ )!AA ? @ Aå  )p- Aq!B ) )  )( ­| B:   )!C C C( ­B|§6  )p/!D ) !E )!F )!G DAÿÿq E F Gæ  )p/!H ) !I )!J )!K HAÿÿq I J Kæ  )p/!L ) !M )!N )!O LAÿÿq M N Oæ \r )p/!P ) !Q )!R )!S PAÿÿq Q R Sæ  )p/!T ) !U )!V )!W TAÿÿq U V Wæ  )p/!X ) !Y )!Z )![ XAÿÿq Y Z [æ \n )p/!\\ ) !] )!^ )!_ \\Aÿÿq ] ^ _æ 	 )p/!` ) !a )!b )!c `Aÿÿq a b cæ  ) !d )!e )!fA d e få  )p(!g ) )  )( ­| g6  )!h h h( ­B|§6  ) !i )!j )!kA i j kå  )p- Aq!l ) )  )( ­| l:   )!m m m( ­B|§6  ) !n )!o )!pA n o på  )p(!q ) )  )( ­| q6  )!r r r( ­B|§6  ) !s )!t )!uA s t uå  )p(!v ) )  )( ­| v6  )!w w w( ­B|§6  A : S A 6L@@ (L )(IAqE\r ))  (L­B~|!x )p) !y  x)7@  x) 78  y)70  y) 7(@ B8| B(|² AqE\r  ) !z )!{ )!|A z { |å  (L!} ) )  )( ­| }6  )!~ ~ ~( ­B|§6  A: S  (LAj6L @ - SAq\r B )è¼ ! AÆ6  Bé  ½ B )è¼ ! )p) (!  )p) ) 7  6 B¢  B|½ A   ) ! )! )!A   å  )p/(! ) )  )( ­| ;  )!  ( ­B|§6  )p/*! ) )  )( ­| ;  )!  ( ­B|§6   (|Aj6|  B°|$ ~~~# B0}! $    7(  7   7  7 )((­B|§ )  ) )å  )((! ) )  )( ­| 6  )!  ( ­B|§6  A 6@@ ( )((IAqE\r )()  (­|-  ! ) )  )( ­| :   )!  ( ­B|§6   (Aj6  B0|$ ¬\r~~~~~~~# Bà }! $    7X  7P  7H )PA6  A6D  (D­¤ 78B !  70  7( )P! )H! B8| BÄ |   B(|ä  )P!A B8| BÄ | å  )X(! )8 )P( ­| 6  )P!	 	 	( ­B|§6  A 6$@@ ($ )X(IAqE\r  )X)  ($­B8~|7 )/ !\n )P! B8|! BÄ |!\r \nAÿÿq  \r æ  )P!A B8| BÄ | å  )(! )8 )P( ­| 6  )P!  ( ­B|§6  A 6@@ ( )(IAqE\r )) (­B|/ ! )P! B8|! BÄ |! Aÿÿq   æ   (Aj6 B !  7  7  )B|! )P! )H) ) !  B8| BÄ |  B(|  ê  )P!A B8| BÄ | å  )- (Aq! )8 )P( ­| :   )P!  ( ­B|§6   ($Aj6$ B (  ! )8 6  )P( ! )8 6@ )(B RAqE\r  )(¦  )8! Bà |$  ~~# BÀ }! $    78  70  7(  7   7  7  7 )0! )(!	 ) !\nA  	 \nå  )8(! )0)  ) ( ­| 6  ) !  ( ­B|§6  A 6@@ ( )8(IAqE\r )8)  (­B|)  )0 )( )  ) ) )ë   (Aj6  BÀ |$ ì!~~~|~~~~~~~~~~~~~~# B }! $    7  7  7  7  7x  7p  7h )! )!	 )!\nA  	 \nå  )( ! ))  )5 | 6  )!  5 B|>  )5 !\r@ \rBV\r @@@@@@@@@@@@@@@@@@ \r§ 	\n\r  )/! )! )! )! Aÿÿq   æ  )! )! )!A   å  ))! ))  )( ­| 7  )!  ( ­B|§6  )! )! )!A   å  )+! ))  )( ­| 9  )!  ( ­B|§6   ))7X  )(6` BØ |B|A 6  )! )! )! BØ |   è  )B| ) ) ) )x )p )hê \r )/! )!  )!! )!" Aÿÿq   ! "æ  )!# )!$ )!%A # $ %å  )(!& ))  )( ­| &6  )!\' \' \'( ­B|§6  A 6T@@ (T )(IAqE\r )) (T­B|/ !( )!) )!* )!+ (Aÿÿq ) * +æ   (TAj6T  )B|B| ) ) ) )x )p )hê  )!, )!- )!.A , - .å  )/(AÿÿqAÿÿGAq!/ ))  )( ­| /:   )!0 0 0( ­B|§6 @ )/(AÿÿqAÿÿGAqE\r  )/(!1 )!2 )!3 )!4 1Aÿÿq 2 3 4æ  )B| ) ) ) )x )p )hê \n )B| ) ) ) )x )p )hê 	 )B| ) ) ) )x )p )hê  )) ) ) ) )x )p )hë  )) ) ) ) )x )p )hë  )) ) ) ) )x )p )hë  )/!5 )!6 )!7 )!8 5Aÿÿq 6 7 8æ  )) ) ) ) )x )p )hë  )) ) ) ) )x )p )hë  )B| ) ) ) )x )p )hê  )!9 )!: )!;A 9 : ;å  )-  Aq!< ))  )( ­| <:   )!= = =( ­B|§6  )/!> )!? )!@ )!A >Aÿÿq ? @ Aæ  )) ) ) ) )x )p )hë  )!B )!C )!DA B C Då  ))B RAq!E ))  )( ­| E:   )!F F F( ­B|§6 @ ))B RAqE\r  )) ) ) ) )x )p )hë  )) ) ) ) )x )p )hë  )B|B| ) ) ) )x )p )hê  )B|B| ) ) ) )x )p )hê  )) ) ) ) )x )p )hë  A 6P@@ (P )(IAqE\r  )) (P­B|7H )H)  ) ) ) )x )p )hë  )H) ) ) ) )x )p )hë   (PAj6P  A : G A 6@@@ (@ )x(IAqE\r )x)  (@­B~|!G ))0!H  G)78  G) 70  H)7(  H) 7 @ B0| B |² AqE\r  )!I )!J )!KA I J Kå  (@!L ))  )( ­| L6  )!M M M( ­B|§6  A: G  (@Aj6@ @ - GAq\r B )è¼ !N A¸6  NBé  ½ B )è¼ !O ))0(!P  ))0) 7  P6 OB¢  B|½ A   )!Q )!R )!SA Q R Så  )/8!T ))  )( ­| T;  )!U U U( ­B|§6  )/:!V ))  )( ­| V;  )!W W W( ­B|§6  B |$ À~~~~# BÀ }! $    70  6,  )0) 7   )07@@@ ) B RAqE\r@ ) ( (,j ) (MAqE\r   ) )  ) (­|7 (,! ) !   (j6  )78  ) B|7  ) )7   A 6@ ( (,IAqE\r   (,6 (­B|¤ ! ) 7  )) B|! ))  7  (,! ))  6 (! ))  6 )) B 7 )) ) !	 )) (­!\nA !@ \nP\r  	  \nü   )) ) 78 )8! BÀ |$  }~# B}!   7  )) 7 @@ ) B RAqE\r ) A 6 ) ) ! ) (­!A !@ P\r    ü   ) )7  y~# B }! $    7  )) 7@@ )B RAqE\r  ))7 )¦   )7  )B 7  B |$ \r\r~~~~~~~# Bð}! $    7è  7à  7ØB !  7Ð  7È A 6Ä@@ (Ä )Ø(IAqE\r  )Ø)  (Ä­B0~|7¸@@ )¸( AFAqE\r  )¸/! B¨| Aÿÿq @ (Ô (ÐMAqE\r @@ (ÔE\r @@ (Ô (ÐMAqE\r  (ÔAt6Ô   )à) B| (Ô­B§ì 7  ) ! )È! (Ð­B!@ P\r    ü\n    ) 7È A6Ô  )à) B|Aì 7È )È!	 (Ð!\n  \nAj6Ð 	 \n­B|!  )°7  )¨7 @@ )¸( AFAqE\r  )¸/! B| Aÿÿq  A :  A 6@@ ( (ÐIAqE\r )È (­B|!\r  \r)78  \r) 70  )7(  )7 @ B0| B |² AqE\r  A:   (Aj6 @ - Aq\r  )¸/! Bø | Aÿÿq  B 7p@ )à)  )à)RAqE\r  )à) B|!  )7  )x7   B|ð 7p@ )pB RAq\r  )à)ÐB RAqE\r  )à)ÐB|!  )7  )x7    ð 7p@ )pB RAqE\r  BØ |!  )7  )x7   )p)7h@ )è( )è(MAqE\r @@ )è(E\r @@ )è( )è(MAqE\r )è!  (At6   )à) B| )è(­B~§ì 7P )P! )è) ! )è(­B~!@ P\r    ü\n   )P! )è 7  )èA6 )à) B|Aì ! )è 7  )è) ! )è! (!  Aj6  ­B~|!  )h7  )`7  )X7 @ (Ô (ÐMAqE\r @@ (ÔE\r @@ (Ô (ÐMAqE\r  (ÔAt6Ô   )à) B| (Ô­B§ì 7H )H! )È! (Ð­B!@ P\r    ü\n    )H7È A6Ô  )à) B|Aì 7È )È! (Ð!    Aj6Ð   ­B|!! BØ |!" ! ")7 ! ") 7 @ )¸( AFAqE\r   )¸/Aÿÿq6D )è )à )à)  (D­B|B|ï   (ÄAj6Ä  Bð|$ Ú~# BÀ }! $    70  )0(6,@@@ (,A KAqE\r  )0)  (,­B~|Bh|7  ) !  )7  ) 7  )7  ) 7 @ B| ² AqE\r   ) 78  (,Aj6,  B 78 )8! BÀ |$  æ~~~~~	~\r~~# Bà}! $    7Ø  7Ð  7È  : Ç )Ø) )Ø(­B|! )Ð(­!  B  }B|7¸@@ )Ð/0AÿÿqAÿÿGAqE\r  )Ð/0! B¨| Aÿÿq  )Ø! )Ð(!	 )¸!\n  )°7H  )¨7@   BÀ | 	 \nò 7 @ ) B RAq\r B )è¼ ! Aº6  B  ½ B )è¼ ! )È) (!\r )È) ) ! )È/AÿÿqAj! )È/\nAÿÿqAj! (°! )¨! )Ð(! B8| 6  B0| 7  B(| 6  B$| 6  B | 6   7  \r6 B  B|½  )ØA6° )ØB7¸ ) )@!  )Ø )¸   7@ - ÇAq\r @ )B RAq\r   )Ø) Ó 7@ )Ø( )Ø(MAqE\r @@ )Ø(E\r @@ )Ø( )Ø(MAqE\r )Ø!  (At6  )Ø) )Ø(­B§ ! )Ø 7 )ØA6B¤ ! )Ø 7 )Ø) )Ø(­B|B|! )Ø) )Ø(­B|! )Ø( )Ø(k­B!@ P\r    ü\n   )! )Ø) )Ø(­B| 7  )Ø!  (Aj6 )Øó  A 6@@ ( )Ð(IAqE\r )Ð)  (­B|/ ! B| Aÿÿq  Bè |!  )7  )7   )Ø) )Ø( )Ð(k (j­B|) 7x@ )Ø) ($ )Ø) ( MAqE\r @@ )Ø) ($E\r @@ )Ø) ($ )Ø) ( MAqE\r )Ø) !  ($At6$  )Ø) ) )Ø) ($­B~§ !  )Ø)   7 )Ø) A6$B¤ !! )Ø)  !7 )Ø) ) )Ø) ( ­B~|B|!" )Ø) ) )Ø) ( ­B~|!# )Ø) (  )Ø) ( k­B~!$@ $P\r  " # $ü\n   )Ø) ) )Ø) ( ­B~|!% % )x7 % )p7 % )h7  )Ø) !& & &( Aj6   (Aj6   )Ø(6d  )Ø)Ð7X )Ð!\' )Ø \'7Ð )Ø!( ( ((ØAj6Ø )Ø )Ø)  )Ð(­B|B|ô @ )Ø(°AFAqE\r  )ØA 6° B 7P@ - ÇAq\r @@ (d )Ø(FAqE\r   )Ø) )HÓ 7P  )Øõ  )Ø) )HÐ 7P )Ø!) ) )(ØAk6Ø )X!* )Ø *7Ð (d!+ )Ø +6 )Øö  )PB RAqE\r @ )Ø( )Ø(MAqE\r @@ )Ø(E\r @@ )Ø( )Ø(MAqE\r )Ø!, , ,(At6  )Ø) )Ø(­B§ !- )Ø -7 )ØA6B¤ !. )Ø .7 )Ø) )Ø(­B|B|!/ )Ø) )Ø(­B|!0 )Ø( )Ø(k­B!1@ 1P\r  / 0 1ü\n   )P!2 )Ø) )Ø(­B| 27  )Ø!3 3 3(Aj6 Bà|$ ³~# Bà }! $    7P  6L  7@  )7(  ) 7   B |¶ B<78  )PB | )8B|) 70@@@ )0B RAqE\r )0!  )7  ) 7  )7  ) 7 @ B| ² AqE\r  )0( (LFAqE\r  (L )@ )0B|÷ AqE\r   )07X  )0)H70  B 7X )X! Bà |$  ý~~# Bà }! $    7X@ )X) )@B QAqE\r BÐ ¤ ! )X) 7@ )X))@!BÐ !A ! B|  ü BÐ !  B| ü\n   )X)! )X))@ 7H )X))@! )X 7 )X)!	 )X)  	7@ )X) )@!\n )X \n7  Bà |$ lc~%~~	~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~~~~~~~~\r~~~	~~# Bà\r}! $    7Ø\r  7Ð\r A 6Ì\r@@ (Ì\r )Ð\r(IAqE\r  )Ð\r)  5Ì\rB0~|7À\r )À\r5 !@ BV\r @@@@@@@@@@@@@@@@@@@@@@ § 	\n\r  )À\r/! B°\r| Aÿÿq  )Ø\r) !  )¸\r7  )°\r7    Õ 7¨\r@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|!	 )Ø\r) )Ø\r(­B|!\n )Ø\r( )Ø\r(k­B!@ P\r  	 \n ü\n   )¨\r! )Ø\r) )Ø\r(­B| 7  )Ø\r!\r \r \r(Aj6  )À\r) )Ø\r) × 7 \r@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|! )Ø\r) )Ø\r(­B|! )Ø\r( )Ø\r(k­B!@ P\r    ü\n   ) \r! )Ø\r) )Ø\r(­B| 7  )Ø\r!  (Aj6  )À\r+ )Ø\r) Ø 7\r@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|! )Ø\r) )Ø\r(­B|! )Ø\r( )Ø\r(k­B!@ P\r    ü\n   )\r! )Ø\r) )Ø\r(­B| 7  )Ø\r!  (Aj6 )À\rB|! )Ø\r) !  )7  ) 7  B| Ö 7\r@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!     (At6  )Ø\r) )Ø\r(­B§ !! )Ø\r !7 )Ø\rA6B¤ !" )Ø\r "7 )Ø\r) )Ø\r(­B|B|!# )Ø\r) )Ø\r(­B|!$ )Ø\r( )Ø\r(k­B!%@ %P\r  # $ %ü\n   )\r!& )Ø\r) )Ø\r(­B| &7  )Ø\r!\' \' \'(Aj6  )Ø\r) B|AÀ ì 7\r )\r!( )À\rB|!) ( ))7 ( )) 7  )À\r/Aÿÿq )Ø\r(Üj!* )\r *6 )À\r/!+ )\r +;0 )\rB|!, B 7ø A 6\r A 6\r , )\r7 , )ø7 B !-  -7ð  -7è@ )À\r/AÿÿqAÿÿGAqE\r  )À\r/!. BØ| .Aÿÿq   )à7ð  )Ø7è@ )èB RAq\r   )Ø\r)  )\r(­B|7Ð )\rB| )Ø\r )ÐB|ï @ )Ø\r(°E\r   )\r )Ø\r) Û 7È@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!/ / /(At6  )Ø\r) )Ø\r(­B§ !0 )Ø\r 07 )Ø\rA6B¤ !1 )Ø\r 17 )Ø\r) )Ø\r(­B|B|!2 )Ø\r) )Ø\r(­B|!3 )Ø\r( )Ø\r(k­B!4@ 4P\r  2 3 4ü\n   )È!5 )Ø\r) )Ø\r(­B| 57  )Ø\r!6 6 6(Aj6 )Ø\r!7 7/àAj!8 7 8;à@ 8AÿÿqAèNAqE\r B )è¼ !9 Aª6@ 9B  BÀ |½ B )è¼ !: )À\r) (!; )À\r) ) !< )À\r/(AÿÿqAj!= )À\r/*AÿÿqAj!> Bä | >6  Bà | =6   <7X  ;6P :B¨  BÐ |½  A¬6 B  B |å  A60Bý  B0|å  )Ø\rA;ä )Ø\rA6° )Ø\rB7¸@ )Ø\r )À\r(Aj )À\rB |ø Aq\r   )À\r(Aj6Ä  )Ø\r) )Ø\r( (Äk­B|) 7¸@ )¸( AGAqE\r B !?  ?7°  ?7¨ )¸!@ B¨|!AA !BA!CA !D A @ B CAq DAq  A¿6ÐB  BÐ|å Bå B å  )Ø\rB|ù B )è¼ !E AÃ6p EB  Bð |½ B )è¼ !F )À\r) (!G )À\r) ) !H )À\r/(AÿÿqAj!I )À\r/*AÿÿqAj!J B|  )°7  )¨7 B| B|¹  ( !K B|  )°7  )¨7 B| B|¹  )!L BÀ| L7  B¸| K6  B´| J6  B°| I6   H7¨  G6  FBÒ  B |½  )Ø\rA6° )Ø\rB7¸ )¨¦ @ )¸)( )À\r(GAqE\r B )è¼ !M AÎ6à MB  Bà|½ B )è¼ !N )À\r) (!O )À\r) ) !P )À\r/(AÿÿqAj!Q )À\r/*AÿÿqAj!R )¸)(!S )À\r(!T B| T6  B| S6  B| R6  B| Q6   P7ø  O6ð NB¥  Bð|½  )Ø\rA6° )Ø\rB7¸ A 6@@ ( )¸)( IAqE\r  )¸)) (­B~|7ø@ )ø)B RAq\r  )Ø\r)B|!U )ø!V  V)7Ø  V) 7Ð  U BÐ|ð 7ð@ )ðB RAq\r B )è¼ !W A×6 WB  B|½ B )è¼ !X )À\r) (!Y )À\r) ) !Z )À\r/(AÿÿqAj![ )À\r/*AÿÿqAj!\\ )ø(!] )ø) !^ BÀ| ^7  B¸| ]6  B´| \\6  B°| [6   Z7¨  Y6  XB¿  B |½  )Ø\rA6° )Ø\rB7¸ )ð)!_ _ _/ Aj;  )ð)!` )ø `7  (Aj6  )Ø\r!a )¸)!b )À\r!c a b cB | c- Aqñ @ )Ø\r(°AFAqE\r  )Ø\r!d d/â!e d eAj;â@ eAÿÿq )Ø\r/äAÿÿqHAqE\r  )Ø\r)¸B RAqE\r B !f  f)° 7è  f)¨ 7à  (Ì\r )À\r(kAk6Ü@ )Ð\r)  (Ü­B0~|( AFAqE\r   )Ð\r)  (Ü­B0~|/;Ú /Ú!g BÈ| gAÿÿq   )Ð7è  )È7à Aï6àB  Bà|å  )À\r) (!h )À\r) ) !i )À\r/(AÿÿqAj!j )À\r/*AÿÿqAj!k (è!l )à!m B| m7  B| l6  B| k6  B| j6   i7ø  h6ðB  Bð|å  B 7À@ )À\r- Aq\r   )Ø\rõ 7À )À\r(Aj!n )Ø\r!o o o( nk6@ )ÀB RAqE\r  )À!p )Ø\r) )Ø\r(Ak­B| p7  )Ø\r!q q q/àAj;à )Ø\r!r )À\rB |!s@ rA sø Aq\r   )Ø\rõ 7¸ )¸!t t t/ Aj;  )À\r/!u B¨| uAÿÿq  B|!v v )°7 v )¨7   )¸7 @ )Ø\r) ($ )Ø\r) ( MAqE\r @@ )Ø\r) ($E\r @@ )Ø\r) ($ )Ø\r) ( MAqE\r )Ø\r) !w w w($At6$  )Ø\r) ) )Ø\r) ($­B~§ !x )Ø\r)  x7 )Ø\r) A6$B¤ !y )Ø\r)  y7 )Ø\r) ) )Ø\r) ( ­B~|B|!z )Ø\r) ) )Ø\r) ( ­B~|!{ )Ø\r) (  )Ø\r) ( k­B~!|@ |P\r  z { |ü\n   )Ø\r) ) )Ø\r) ( ­B~|!} } ) 7 } )7 } )7  )Ø\r) !~ ~ ~( Aj6  )Ø\r!  (Aj6 )À\r/! B| Aÿÿq  )Ø\r!  )7è  )7à   Bà|ú 7ø\n@ )ø\nB RAq\r B )è¼ ! A6  B  B |½ B )è¼ ! )À\r) (! )À\r) ) ! )À\r/(AÿÿqAj! )À\r/*AÿÿqAj! (! )! BÐ| 7  BÈ| 6  BÄ| 6  BÀ| 6   7¸  6° B¿  B°|½  )Ø\rA6° )Ø\rB7¸@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|! )Ø\r) )Ø\r(­B|! )Ø\r( )Ø\r(k­B!@ P\r    ü\n   )ø\n)! )Ø\r) )Ø\r(­B| 7  )Ø\r!  (Aj6 )Ø\r! )À\rB |!@ A ø Aq\r  )À\r/! Bè\n| Aÿÿq  )Ø\r!  )ð\n7¸  )è\n7°   B°|ú 7à\n@ )à\nB RAq\r B )è¼ ! A¡6ð B  Bð|½ B )è¼ ! )À\r) (! )À\r) ) ! )À\r/(AÿÿqAj! )À\r/*AÿÿqAj! (ð\n! )è\n! B | 7  B| 6  B| 6  B| 6   7  6 B¿  B|½  )Ø\rA6° )Ø\rB7¸  )Ø\rõ 7Ø\n@@ )Ø\n) )à\n))QAqE\r  )Ø\n!  / Aj;   )Ø\n )à\n))Ð 7Ø\n )Ø\r! (! A!¡    ¡j6 )à\n)!¢ ¢ ¡ ¢/ j;  )Ø\n!£ )à\n £7\r  )Ø\r )À\r/Aÿÿqû 6Ô\n@ (Ô\nAFAqE\r  )À\r/!¤ BÀ\n| ¤Aÿÿq B )è¼ !¥ Aµ6À ¥B  BÀ|½ B )è¼ !¦ )À\r) (!§ )À\r) ) !¨ )À\r/(AÿÿqAj!© )À\r/*AÿÿqAj!ª (È\n!« )À\n!¬ Bð| ¬7  Bè| «6  Bä| ª6  Bà| ©6   ¨7Ø  §6Ð ¦BÊ  BÐ|½  )Ø\rA6° )Ø\rB7¸  (Ô\nAk6Ì\r )Ø\r!­ )À\rB |!®@ ­A ®ø Aq\r   )Ø\rõ 7¸\n@ )¸\n AqE\r   )Ø\r )À\r/Aÿÿqû 6´\n@ (´\nAFAqE\r  )À\r/!¯ B \n| ¯Aÿÿq B )è¼ !° AÆ6 °B  B|½ B )è¼ !± )À\r) (!² )À\r) ) !³ )À\r/(AÿÿqAj!´ )À\r/*AÿÿqAj!µ (¨\n!¶ ) \n!· B°| ·7  B¨| ¶6  B¤| µ6  B | ´6   ³7  ²6 ±BÊ  B|½  )Ø\rA6° )Ø\rB7¸  (´\nAk6Ì\r )Ø\r!¸ ¸ ¸(Aj6 )Ø\r!¹ )À\rB |!º@ ¹A ºø Aq\r \r  )Ø\rõ 7\n@ )\n Aq\r   )Ø\r )À\r/Aÿÿqû 6\n@ (\nAFAqE\r  )À\r/!» B\n| »Aÿÿq B )è¼ !¼ AÚ6À ¼B  BÀ|½ B )è¼ !½ )À\r) (!¾ )À\r) ) !¿ )À\r/(AÿÿqAj!À )À\r/*AÿÿqAj!Á (\n!Â )\n!Ã Bð| Ã7  Bè| Â6  Bä| Á6  Bà| À6   ¿7Ø  ¾6Ð ½BÊ  BÐ|½  )Ø\rA6° )Ø\rB7¸  (\nAk6Ì\r )Ø\r!Ä Ä Ä(Aj6\n	 )Ø\r!Å )À\rB |!Æ@ ÅA Æø Aq\r @ )Ø\r) (4 )Ø\r) (0MAqE\r @@ )Ø\r) (4E\r @@ )Ø\r) (4 )Ø\r) (0MAqE\r )Ø\r) !Ç Ç Ç(4At64  )Ø\r) )( )Ø\r) (4­B§ !È )Ø\r)  È7( )Ø\r) A64B¤ !É )Ø\r)  É7( )Ø\r) )( )Ø\r) (0­B|B|!Ê )Ø\r) )( )Ø\r) (0­B|!Ë )Ø\r) (0 )Ø\r) (0k­B!Ì@ ÌP\r  Ê Ë Ìü\n   )Ø\rõ !Í )Ø\r) )( )Ø\r) (0­B| Í7  )Ø\r) !Î Î Î(0Aj60 )Ø\r!Ï Ï Ï(Aj6@ )Ø\r) (0\r B )è¼ !Ð Að6 ÐB  B|½ B )è¼ !Ñ )À\r) (!Ò )À\r) ) !Ó )À\r/(AÿÿqAj!Ô )À\r/*AÿÿqAj!Õ B¤| Õ6  B | Ô6   Ó7  Ò6 ÑB«  B|½  )Ø\rA6° )Ø\rB7¸\n )Ø\r!Ö )À\rB |!×@ ÖA ×ø Aq\r \n  )Ø\r) B(|7ø	  )ø	)  )ø	(Ak­B|) 7ð	  )Ø\rõ 7è	@ )è	 )ð	Þ Aq\r   )Ø\r )À\r/Aÿÿqû 6ä	@ (ä	AFAqE\r  )À\r/!Ø BÐ	| ØAÿÿq B )è¼ !Ù Aþ6° ÙB  B°|½ B )è¼ !Ú )À\r) (!Û )À\r) ) !Ü )À\r/(AÿÿqAj!Ý )À\r/*AÿÿqAj!Þ (Ø	!ß )Ð	!à Bà| à7  BØ| ß6  BÔ| Þ6  BÐ| Ý6   Ü7È  Û6À ÚB  BÀ|½  )Ø\rA6° )Ø\rB7¸  (ä	Ak6Ì\r )Ø\r!á á á(Aj6@ )Ø\r) (0\r B )è¼ !â A6ð âB  Bð|½ B )è¼ !ã )À\r) (!ä )À\r) ) !å )À\r/(AÿÿqAj!æ )À\r/*AÿÿqAj!ç B| ç6  B| æ6   å7  ä6 ãBç  B|½  )Ø\rA6° )Ø\rB7¸	 )Ø\r) !è è è(0Aj60@ )Ø\r )À\r( )À\rB |ø Aq\r   )Ø\r) )Ø\r( )À\r(k­B|) 7È	 A6Ä	@@ (Ä	 )À\r(IAqE\r  )Ø\r) )Ø\r( )À\r(k (Ä	j­B|) 7¸	@@ )È	( AFAqE\r @ )¸	( AGAqE\r B )è¼ !é A6  éB  B |½ B )è¼ !ê )À\r) (!ë )À\r) ) !ì )À\r/(AÿÿqAj!í )À\r/*AÿÿqAj!î BÄ| î6  BÀ| í6   ì7¸  ë6° êB²  B°|½  )Ø\rA6° )Ø\rB7¸ )Ø\r!ï )È	B|!ð )¸	)!ñ  ð)7  ð) 7  ï B| ñ 7°	@ )°	B RAq\r B )è¼ !ò A6Ð òB  BÐ|½ B )è¼ !ó )À\r) (!ô )À\r) ) !õ )À\r/(AÿÿqAj!ö )À\r/*AÿÿqAj!÷ Bô| ÷6  Bð| ö6   õ7è  ô6à óB  Bà|½  )Ø\rA6° )Ø\rB7¸  )°	7È	  )È	 )¸	 )À\rB | )Ø\r 7¨	@ )Ø\r(°E\r @ )¨	B RAq\r   )Ø\r) Ó 7È	  )¨	) 7È	  (Ä	Aj6Ä	  )À\r(!ø )Ø\r!ù ù ù( øk6@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!ú ú ú(At6  )Ø\r) )Ø\r(­B§ !û )Ø\r û7 )Ø\rA6B¤ !ü )Ø\r ü7 )Ø\r) )Ø\r(­B|B|!ý )Ø\r) )Ø\r(­B|!þ )Ø\r( )Ø\r(k­B!ÿ@ ÿP\r  ý þ ÿü\n   )È	! )Ø\r) )Ø\r(­B| 7  )Ø\r!  (Aj6 )Ø\r! )À\rB |!@ A ø Aq\r   )Ø\r) )Ø\r(Ak­B|) 7 	  )Ø\r) )Ø\r(Ak­B|) 7	  )Ø\r) )Ø\r(Ak­B|) 7	@ ) 	( AFAqE\r B )è¼ ! A¾6 B  B|½ B )è¼ ! )À\r) (! )À\r) ) ! )À\r/(AÿÿqAj! )À\r/*AÿÿqAj! B´| 6  B°| 6   7¨  6  BÙ  B |½  )Ø\rA6° )Ø\rB7¸  ) 	 )	 )À\rB | )Ø\r 7	@ )Ø\r(°E\r @ )	B RAq\r  ) 	( AFAqE\r  ) 	) ) 	) )	 ) 	)Ó    ) 	 )	 )À\rB | )Ø\r 7	@@ )	) ) 	)QAqE\r  )	!  / Aj;   )	 ) 	)Ð 7	 )Ø\r!  (A}j6 )	) !  / Aj;  )	! )	 7 @ )Ø\r )À\r( )À\rB |ø Aq\r   )Ø\r) B|Aì 7	  )	7ø A 6ô@@ (ô )À\r(IAqE\r )Ø\r) B|Aì ! )ø 7 )Ø\r) )Ø\r( )À\r(k (ôj­B|) ! )ø) 7   )ø)7ø  (ôAj6ô  )À\r(! )Ø\r!  ( k6  )	 )Ø\r) Ô 7è@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|! )Ø\r) )Ø\r(­B|! )Ø\r( )Ø\r(k­B!@ P\r    ü\n   )è! )Ø\r) )Ø\r(­B| 7  )Ø\r!  (Aj6@ )Ø\r )À\r(At )À\rB |ø Aq\r   )Ø\r) B|AÐ ì 7à A 6Ü@@ (Ü )À\r(IAqE\r  )Ø\r) )Ø\r( )À\r(Atk (ÜAtj­B|) 7Ð  )Ø\r) )Ø\r( )À\r(Atk (ÜAtjAj­B|) 7È )Ø\r)  )à )Ð )È   (ÜAj6Ü  )À\r(At! )Ø\r!  ( k6  )à )Ø\r) Ú 7À@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!  (At6  )Ø\r) )Ø\r(­B§ ! )Ø\r 7 )Ø\rA6B¤ ! )Ø\r 7 )Ø\r) )Ø\r(­B|B|! )Ø\r) )Ø\r(­B|!  )Ø\r( )Ø\r(k­B!¡@ ¡P\r     ¡ü\n   )À!¢ )Ø\r) )Ø\r(­B| ¢7  )Ø\r!£ £ £(Aj6  )Ø\r)Ð )Ø\r) Û 7¸@ )Ø\r( )Ø\r(MAqE\r @@ )Ø\r(E\r @@ )Ø\r( )Ø\r(MAqE\r )Ø\r!¤ ¤ ¤(At6  )Ø\r) )Ø\r(­B§ !¥ )Ø\r ¥7 )Ø\rA6B¤ !¦ )Ø\r ¦7 )Ø\r) )Ø\r(­B|B|!§ )Ø\r) )Ø\r(­B|!¨ )Ø\r( )Ø\r(k­B!©@ ©P\r  § ¨ ©ü\n   )¸!ª )Ø\r) )Ø\r(­B| ª7  )Ø\r!« « «(Aj6  (Ì\rAj6Ì\r  Bà\r|$ 2~# B}!   7 )) )(Ak­B|) þ~# B }! $    7  )) 7 A 6@@ ( )(IAqE\r ))  (­B|) Ý   (Aj6  )A 6 )B|í  A 6@@ ( )( IAqE\r )) 5B~|)!  / Aj;   (Aj6  )A 6  A 6@@ ( )(0IAqE\r ))( (­B|) Ý   (Aj6  )A 60@ )) )HB RAqE\r  )) )H! ) 7  B |$ ¹~# B }!   6  7  7 A 6@@@ ( (IAqE\r@ ) (­B|) (  ) (­B|( GAqE\r  ) (­B|( E\r  A Aq:   (Aj6  AAq:  - Aq¢~~# Bà }! $    7P  6L  7@@@ )P( (LIAqE\r  A260B  B0|å Bå B å  )PB|ù B )è¼ ! A56  B  ½ B )è¼ ! )@) (! )@) ) ! )@/AÿÿqAj! )@/\nAÿÿqAj!	 (L!\n )P(! B,| 6  B(| \n6  B$| 	6  B | 6   7  6 BÜ  B|½  )PB7¸ A Aq: _ AAq: _ - _Aq! Bà |$  è~~# BÐ }! $    7HB !  7@  78 A 64@ )H(A\nOAqE\r   )H(A\nk64  (460@@ (0 )H(IAqE\r B8|Bµ »  )H)  (0­B|) ! B8|!A !A !A!    Aq Aq  B8|!A\n!	A!\n  	 \nt \nuº   (0Aj60  B |  )@7  )87  B | ¹   )(7  ) 7 B|´  )8¦  BÐ |$ Ç~# BÐ }! $    7@ )@) B|!  )70  ) 7(   B(|ð 78@@ )8B RAqE\r   )87H@ )@)ÐB RAqE\r  )@)ÐB|!  )7   ) 7   B|ð 78@ )8B RAqE\r   )87H )@)B|!  )7  ) 7   B|ð 78@ )8B RAqE\r   )87H B 7H )H! BÐ |$  å~# B0}!   7   ; A 6@ ) )ÐB RAqE\r   ) )Ð(6  /AÿÿqA<o¬7  ) ) (­Bà~| )B|) 7@@@ )B RAqE\r@ )/ Aÿÿq /AÿÿqFAqE\r   )(6,  ))7  A6, (,²~~~~# B }! $    7  7  : @ )( )( )(jIAqE\r @@ )(E\r @@ )( )( )(jIAqE\r )!  (At6  ))  )(­B§ ! ) 7  )( )(j! ) 6 )(­B¤ ! ) 7  ))  )(­B|! )) !	 )(­B!\n@ \nP\r   	 \nü\n   )(! )!   (j6 )!\r )! B|! ) !B! \r   |ý  )  )) |ô  )(! )!   (Üj6Ü@@ - Aq\r  B 7@ )(\r   ))Ó 7  )õ 7 )! B |$  û~~~~# BÀ}! $    7¸  7°  7¨ )¸(! )°!   (j6  )¨ )°(­Bà~§ì 7  ) ! )°) ! )°(­Bà~!@ P\r    ü\n   ) !	 )° 	7  A 6@@ ( )¸(IAqE\rBà!\nA ! B8|  \nü  A 64@@ (4 )¸)  (­B|(IAqE\r  )¸)  (­B|) (4­B0~|7(@@ )(( AGAqE\r   )(/AÿÿqA<o¬7   B8| ) B|) 7@ )B R!A !\r Aq! \r!@ E\r  ))B R!@ AqE\r   ))7  )¨Aì 7 )!  )(/;  B|A ;   (46 B 7  )7  ) 7 @@ )B RAqE\r  )! ) 7 )! B8| ) B| 7   (4Aj64  )°)  ( )°(j­Bà~|!Bà!  B8| ü\n    (Aj6  )¸(! )°!   (j6 BÀ|$ ù~~# BÐ }! $    7H  7@  6<  70 A 6,@@ (, (<IAqE\r )@ (,­BÐ ~|!  )7  ) 7   ¶ B<7   )H ) B|) 7@ )B R!A ! Aq! !	@ E\r  ))HB R!	@ 	AqE\r   ))H7  )0AÐ ì 7 ) )@ (,­BÐ ~|BÐ ü\n  @@ )B RAqE\r  )!\n ) \n7H )! )H ) B| 7   (,Aj6,  BÐ |$ è~~# BÀ}! $   6¼  7°  7¨Bè!  A  ü   BÐ ¤ 7  )!BÐ !A ! BØ |  ü BÐ !	  BØ | 	ü\n      )7    )7   Aÿÿ;ä   ) B|Aì 7P  )P7H A 6D@@ (D (¼IAqE\r  )° (D­B|)  §6@   ) B| (@ì 78 )8!\n )° (D­B|) ! (@­!@ P\r  \n  ü\n     ) B|Aì 70  ) B|A(ì !\r )0 \r7  )0) ! A6 B|B|A 6  B|B|!  )87  (@6 B|A 6  B 7  A;( A ;* B|B$|A 6   )(7   ) 7  )7  )7  )7  )0! )H 7  )07H  (DAj6D    )P )¨  BÀ|$ £~~~~~~~~~~)~# BÀ}! $    7¸  7°  7¨ )¨!B (à¤ ! )¸)B|! Bè   þ  )¨!B (« ! )¸)B|!	 Bð¤   	þ  )¨!\nB (ð° ! )¸)B|! \nB «   þ  )¨!\rB (Ð± ! )¸)B|! \rB±   þ  )¨!B (à¶ ! )¸)B|! Bà±   þ  )¨!B (ð½ ! )¸)B|! B¼   þ  )¨!B (ð» ! )¸)B|! Bð¶   þ  )¨!B (ð¿ ! )¸)B|! B¾   þ  )¨!B (¨Þ ! )¸)B|! B¨Þ   þ  )¨!B ( Ë !  )¸)B|!! BÀ    !þ  )¸B | )¨Bàü\n   )°!" )¸ "7¨  )¸) Ó 7  B|!# BÐ 7 A6 #B|A 6   ) 7@ )¸)($ )¸)( MAqE\r @@ )¸)($E\r @@ )¸)($ )¸)( MAqE\r )¸)!$ $ $($At6$  )¸)) )¸)($­B~§ !% )¸) %7 )¸)A6$B¤ !& )¸) &7 )¸)) )¸)( ­B~|B|!\' )¸)) )¸)( ­B~|!( )¸)(  )¸)( k­B~!)@ )P\r  \' ( )ü\n   )¸)) )¸)( ­B~|!* * )7 * )7 * )7  )¸)!+ + +( Aj6  B 7p A6x Bð |B|A 6  )¸) !,  )x7  )p7  B| ,Õ 7 BØ |!- Bÿ 7X A6` -B|A 6   )7h@ )¸)($ )¸)( MAqE\r @@ )¸)($E\r @@ )¸)($ )¸)( MAqE\r )¸)!. . .($At6$  )¸)) )¸)($­B~§ !/ )¸) /7 )¸)A6$B¤ !0 )¸) 07 )¸)) )¸)( ­B~|B|!1 )¸)) )¸)( ­B~|!2 )¸)(  )¸)( k­B~!3@ 3P\r  1 2 3ü\n   )¸)) )¸)( ­B~|!4 4 )h7 4 )`7 4 )X7  )¸)!5 5 5( Aj6  )¸) !6 AAq 6Ù 7P B8|!7 B¹ 78 A6@ 7B|A 6   )P7H@ )¸)($ )¸)( MAqE\r @@ )¸)($E\r @@ )¸)($ )¸)( MAqE\r )¸)!8 8 8($At6$  )¸)) )¸)($­B~§ !9 )¸) 97 )¸)A6$B¤ !: )¸) :7 )¸)) )¸)( ­B~|B|!; )¸)) )¸)( ­B~|!< )¸)(  )¸)( k­B~!=@ =P\r  ; < =ü\n   )¸)) )¸)( ­B~|!> > )H7 > )@7 > )87  )¸)!? ? ?( Aj6  )¸) !@ A Aq @Ù 70 B|!A BÍ 7 A6  AB|A 6   )07(@ )¸)($ )¸)( MAqE\r @@ )¸)($E\r @@ )¸)($ )¸)( MAqE\r )¸)!B B B($At6$  )¸)) )¸)($­B~§ !C )¸) C7 )¸)A6$B¤ !D )¸) D7 )¸)) )¸)( ­B~|B|!E )¸)) )¸)( ­B~|!F )¸)(  )¸)( k­B~!G@ GP\r  E F Gü\n   )¸)) )¸)( ­B~|!H H )(7 H ) 7 H )7  )¸)!I I I( Aj6  BÀ|$ ~# B }! $    7  ))7@@ )B RAqE\r  ))@7 )ß   )7 @ ))B RAqE\r  ))¦  B |$ ~~~# Bð}! $   6ì  7à  : ß  7ÐABø ü B°!  A  ü Bà!A ! Bð|  ü @ )ÐB RAq\r   Bð|7Ð  BÈ |!	 (ì!\n )à! )Ð! B| \n  ÿ Bè!\r 	 B| \rü\n   - ß!  AÿÿA  Aq;¬ Bð|$ #~~~# B}! $    7  :  Bð |  )7  ) 7 Bð | B| @ )(D )(@MAqE\r @@ )(DE\r @@ )(D )(@MAqE\r )!  (DAt6D  ))8 )(D­B§ ! ) 78 )A6DB¤ ! ) 78 ))8 )(@­B|B|! ))8 )(@­B|!	 )(@ )(@k­B!\n@ \nP\r   	 \nü\n   ))8 )(@­B|!  )x7  )p7  )!  (@Aj6@@ )(D )(@MAqE\r @@ )(DE\r @@ )(D )(@MAqE\r )!\r \r \r(DAt6D  ))8 )(D­B§ ! ) 78 )A6DB¤ ! ) 78 ))8 )(@­B|B|! ))8 )(@­B|! )(@ )(@k­B!@ P\r    ü\n   ))8 )(@­B|! Bß 7` A6h Bà |B|A 6   )h7  )`7  )!  (@Aj6@@ )(D )(@MAqE\r @@ )(DE\r @@ )(D )(@MAqE\r )!  (DAt6D  ))8 )(D­B§ ! ) 78 )A6DB¤ ! ) 78 ))8 )(@­B|B|! ))8 )(@­B|! )(@ )(@k­B!@ P\r    ü\n   ))8 )(@­B|! BÊ 7P A6X BÐ |B|A 6   )X7  )P7  )!  (@Aj6@ )!  )7  ) 7 )!B!  |! B!!  !|!" B(|!# B8|!$ B|!%  )7  ) 7 B !&A !\' BÀ |    " # $ %  \' &   )!( ( !|!) ( |!* BÀ | ) & & \' ( * \' \' \'  )!+ B0| BÀ | +à  )BÈ |!, - !-  , B0| -Aqü 7( A 6$@@ ($ (8IAqE\r )0 ($­B|)¦   ($Aj6$  )0¦  )(!. B|$  .þ~~~# BÀ }! $    78  70  6,  Aq: + )0! (,! )8! B|! B|    Ç  )8BÈ |!	 - +!\n  	 B| \nAqü 7 A 6@@ ( ( IAqE\r ) (­B|)¦   (Aj6  )¦  )! BÀ |$  Ò~~~~# B0}! $    7(  7   6 ) ! (! )(! B|  B  Ì @ )(($ )((  (jIAqE\r  )((  (j! )( 6$@@ )(( \r  )(($­B8~¤ ! )( 7 )() )(($­B8~§ !	 )( 	7 )() )(( ­B8~|!\n )! (­B8~!@ P\r  \n  ü\n   (!\r )(!  \r ( j6  B0|$ ð\n~~~~~# BÀ }! $  A ;<@@@ /<AÿÿqB (ÈÝ IAqE\rB )ÀÝ  /<Aÿÿq­B|!  )7  ) 7   )7   ) 7 @ B| ² AqE\r   /<;>  /<Aj;<    (60  (! BÐÝ  ì 7( )(!  ) ! (0­!@ P\r    ü\n  @B (ÌÝ B (ÈÝ MAqE\r @@B (ÌÝ E\r @@B (ÌÝ B (ÈÝ MAqE\rB (ÌÝ At!B  6ÌÝ  B (ÌÝ ­B§! BÐÝ  ì 7  ) !	B )ÀÝ !\nB (ÈÝ ­B!@ P\r  	 \n ü\n   ) !B  7ÀÝ A!\rB  \r6ÌÝ BÐÝ Aì !B  7ÀÝ B )ÀÝ !B (ÈÝ ! Aj!B  6ÈÝ   ­B|!  )07  )(7  B (ÈÝ Ak;> />Aÿÿq! BÀ |$  J~# B}!  ;B )ÀÝ  /Aÿÿq­B|!   )7   ) 7 ¦~# B}!  (6@@@ (A KAqE\r )  (Ak­|-  !A!@  t uA/FAqE\r    ) 7    (6  B|A 6   (Aj6   B 7   A 6  B|A 6 ¹\n~# B }! $    7  7  7@@ )) B RAq\r  )A6 )Aì ! ) 7 @ )( )(MAqE\r  )!  (At6  ) )(­B§ì 7  ) ! )) ! )(­B!@ P\r    ü\n   ) !	 ) 	7  )!\n )) ! )! (!\r  \rAj6  \r­B| \n7  B |$ Û\n~~~~~# B}!\n \n$  \n  7 \n 7 \n 7x \n 7p \n : o \n 7` \n 7X \n ;V \n ;T \n 	: SB ! \n 7H \n 7@B ! \n 78 \n 70 \nA 6,@@ \n(, \n)(IAqE\r \n \n))  \n5,B|) 7  \n)x!\r \n)p! \n)`! \n- oAq! \n \nB | \r  \nBÀ |   : @ \n- Aq\r  \n)x! \n)`! \nB |    \n) ! \n)`! \nBÀ |    \n \n(,6@@ \n( \n(HIAqE\r@ \n(< \n(8MAqE\r @@ \n(<E\r @@ \n(< \n(8MAqE\r \n \n(<At6<  \n \n)0 \n(<­B § 70 \nA6< \nB¤ 70 \n)0 \n58|B|B  \n- ! \n)0 \n(8­| Aq:   \n \n(8Aj68 \n \n(Aj6  \n \n(,Aj6,  \nA 6@@ \n( \n(HIAqE\r \n \n)0 \n5|-  Aq: @@ \n- AqE\r A ! \n/V!A!  t u! \n ;@@ \n- AqE\r A ! \n/T!A!  t u! \n ; \n)@ \n5B|) ! \n)! \n)x! \n)p! \n- o!  \n)`!! \n)X!" \n/!# \n/!$ \n- SAq \n- AqrA G!%  Aq!&A!\' # \'t \'u!(A!)     & ! " ( $ )t )u %Aq  \n \n(Aj6  \n)@!* \n) *7  \n(H!+ \n) +6@ \n)0B RAqE\r  \n)0¦  \nB|$ Õ~~~~# BÐ }! $    7@  78  70  7(  : \'  7@@@ )8B RAqE\r  )0B RAq\r A Aq: O@ )@) ( A\rFAqE\r  )@) B| )8 )0  A Aq: O@ )@) ( AFAqE\r  A 6@@ ( )@) (IAqE\r )@) ) (­B| )8 )0   (Aj6  A Aq: O@ )@) ( AFAqE\r  )@) /! )8!  Aÿÿq  6@ (AGAqE\r @@ )(B RAqE\r  (!	 )8!\n )0! )(! - \'!\r )! 	 \n   \rAq    )0)  (­B|) 7 )8! )! B|    )! )@ 7  AAq: O A Aq: O - OAq! BÐ |$  ª	~# B0}! $    7(  7   7@@@ ) B RAqE\r  )()  )  Aq\r  )AÀ ì 7 )! )() !  )878  )070  )(7(  ) 7   )7  )7  )7  ) 7  )! )( 7  )5 ! BV\r @@@@@@@@@@@@@@@@@@ § 	\n\r  )B| )  ) \r  ) )(­B§ì 7 )! ))!	 )(­B!\n@ \nP\r   	 \nü\n   )! ) 7 )B|B| )  )  )B| )  ) \n )B| )  ) 	 )B| )  )  )B| )  )  )B|B| )  )  )B|B| )  )  )B|B| )  )  )B| )  )  )B|B| )  )  )B|B| )  ) @ ))B RAqE\r  )B| )  )  )B| )  )  )B|B| )  )  )B|B| )  )  )B| )  )  A 6@@ ( )(IAqE\r@ )) (­B|) B RAqE\r  )) (­B| )  )  )) (­B|B| )  )   (Aj6  B0|$ Ý7x~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~# BÀ}!\n \n$  \n  7¸ \n 7° \n 7¨ \n 7 A! \n  q:  \n 7 \n 7 \n ; \n ; \n 	 q:  \n)¸5 !@ BV\r @@@@@@@@@@@@@@@@@@ § 	\n\r  \n)¸B|!\r \n)°! \n)¨! \n) ! \n- ! \n)! \n)! \n/! \n/! \n- ! Aq!A!  t u!A! \r         t u Aq \r \n)¸B|! \n)°! \n)¨! \n) ! \n- ! \n)!  \n)!! \n/!" \n/!# \n- !$ Aq!%A!& " &t &u!\'A!(     %   ! \' # (t (u $Aq  \n)¸B|!) \n)°!* \n)¨!+ \n) !, \n- !- \n)!. \n)!/ \n/!0 \n/!1 \n- !2 -Aq!3A!4 0 4t 4u!5A!6 ) * + , 3 . / 5 1 6t 6u 2Aq \n \n)¸B|!7 \n)°!8 \n)¨!9 \n) !: \n- !; \n)!< \n)!= \n/!> \n/!? \n- !@ ;Aq!AA!B > Bt Bu!CA!D 7 8 9 : A < = C ? Dt Du @Aq 	 \n)¸B|!E \n)°!F \n)¨!G \n) !H \n- !I \n)!J \n)!K \n/!L \n/!M \n- !N IAq!OA!P L Pt Pu!QA!R E F G H O J K Q M Rt Ru NAq  \n)¸B|!S \n)¨!T \n) !U \n)!V \n- Aq!W \n S T UB  W V : @ \n- AqE\r  \nA ; \nA ; \n)¸)!X \n)°!Y \n)¨!Z \n) ![ \n- !\\ \n)!] \n)!^ \n/!_ \n/!` \n- Aq \n- AqrA G!a \\Aq!bA!c _ ct cu!dA!e X Y Z [ b ] ^ d ` et eu aAq  \n)¸B|!f \n)¨!g \n) !h \n)!i \n- Aq!j \n f g hB  j i : @ \n- AqE\r  \nA ; \nA ; \n)¸)!k \n)°!l \n)¨!m \n) !n \n- !o \n)!p \n)!q \n/!r \n/!s \n- Aq \n- AqrA G!t oAq!uA!v r vt vu!wA!x k l m n u p q w s xt xu tAq  \n)¸B|!y \n)¨!z \n) !{ \n)!| \n- Aq!} \n y z {B  } | : @ \n- AqE\r  \nA ; \nA ; \n)¸)!~ \n)°! \n)¨! \n) ! \n- ! \n)! \n)! \n/! \n/! \n- Aq \n- AqrA G! Aq!A!  t u!A! ~         t u Aq  \n)¸B|! \n)¨! \n) ! \n)! \n- Aq! \n   B    : ÿ@ \n- ÿAqE\r  \nA ; \nA ; \n)¸)! \n)°! \n)¨! \n) ! \n- ! \n)! \n)! \n/! \n/! \n- Aq \n- ÿAqrA G! Aq!A!  t u!A!          t u Aq  \n)¸B|! \n)¨!  \n) !¡ \n)!¢ \n- Aq!£ \n    ¡B  £ ¢ : þ@ \n- þAqE\r  \nA ; \nA ; \n)¸)!¤ \n)°!¥ \n)¨!¦ \n) !§ \n- !¨ \n)!© \n)!ª \n/!« \n/!¬ \n- Aq \n- þAqrA G!­ ¨Aq!®A!¯ « ¯t ¯u!°A!± ¤ ¥ ¦ § ® © ª ° ¬ ±t ±u ­Aq  \n)¸B|!² \n)°!³ \n)¨!´ \n) !µ \n- !¶ \n)!· \n)!¸ \n/!¹ \n/!º \n- !» ¶Aq!¼A!½ ¹ ½t ½u!¾A!¿ ² ³ ´ µ ¼ · ¸ ¾ º ¿t ¿u »Aq @ \n)¸)( AFAqE\r  \n \n)¸)/;ü \n)°!À \n/ü!Á \n)¸(!Â \n À ÁAÿÿq Â 7ð@ \n)ðB RAqE\r  \n \n)¸)7à \n \n)ð(6è \nBì|A 6 @ \n)ð- (AqE\r  \n \n(èAj6è \n \n)¸( \n(èk6Ø \n \n) \n(Ø­B§ì 7Ð \nA 6Ì@@ \n(Ì \n(ØIAqE\r \n)¸) \n(è \n(Ìj­B|) !Ã \n)Ð \n(Ì­B| Ã7  \n \n(ÌAj6Ì  \n \n)AÀ ì 7À \n)ÀA6  \n)Ð!Ä \n)À Ä7 \n(Ø!Å \n)À Å6 \n \n) \n(èAj­B§ì 7¸ \n)¸!Æ \n)à!Ç \n(è­B!È@ ÈP\r  Æ Ç Èü\n   \n \n)¸7à \n)À!É \n)à!Ê \n(è!Ë \n ËAj6è Ê Ë­B| É7 B !Ì \n Ì7° \n Ì7¨B !Í \n Í7  \n Í7 \n)ð/ !Î \nB| ÎAÿÿq  \nB| \n \n)78 \n \n)70 \nB| \nB0|¼  \nB|!ÏAÀ !ÐA!Ñ Ï Ð Ñt Ñuº  \n \n(¤6 \nA 6@@ \n( \n)ð(IAqE\r \n)ð) \n(­B|/ !Ò \nBð | ÒAÿÿq  \nB| \n \n)x7 \n \n)p7  \nB| \n¼  \nBØ | \n \n) 7 \n \n)7 \nBØ | \nB|¹  \n \n)`7( \n \n)X7  \n \nB | ;n@ \n(´ \n(°MAqE\r @@ \n(´E\r @@ \n(´ \n(°MAqE\r \n \n(´At6´  \n \n)¨ \n(´­B§ 7¨ \nA6´ \nB¤ 7¨ \n)¨ \n(°­B|B|!Ó \n)¨ \n(°­B|!Ô \n(° \n(°k­B!Õ@ ÕP\r  Ó Ô Õü\n   \n/n!Ö \n)¨ \n(°­B| Ö;  \n \n(°Aj6° \n \n(6¤ \n \n(Aj6  \n)¦  \n)¸A6  \n)¸!× \n)ð!Ø × Ø) 7 × Ø)7 \n \n) \n)¸(Atì 7P \n)P!Ù \n)¸!Ú Ú)!Û Ú5B!Ü@ ÜP\r  Ù Û Üü\n   \n)P!Ý \n)¸ Ý7 \n)¸!ÞB!ß Þ ß|!à \n)!á à \nB¨| á  ß \n)¸|!â ß \n)ð|!ã \n)!ä â ã \nB¨| ä  ß \n)¸|!å \n)°!æ \n)ð- (!ç \n)!è \n)¸)0!é \n)¸/8!êA!ë ê ët ëu!ì \n)ð(,!íA!î ì í ît îuk!ï \n)¸/:!ðA!ñ ð ñt ñu!ò \n)ð(0!óA!ô ò ó ôt ôuk!õ \nB¨|!ö \nBà|!÷A !ø çAq!ùA!ú ï út úu!ûA!ü å æ ö ÷ ù è é û õ üt üu øAq @ \n)¨B RAqE\r  \n)¨¦  \n)¸B|!ý \n)¨!þ \n) !ÿ \n)! \n- Aq! \n ý þ ÿB    : O@ \n- OAqE\r  \nA ; \nA ; \n)¸)! \n)°! \n)¨! \n) ! \n- ! \n)! \n)! \n/! \n/! \n- Aq \n- OAqrA G! Aq!A!  t u!A!          t u Aq @ \n)¸)B RAqE\r  \n)¸B|! \n)¨! \n) ! \n)! \n- Aq! \n   B    : N@ \n- NAqE\r  \nA ; \nA ; \n)¸)! \n)°! \n)¨! \n) ! \n- ! \n)! \n)! \n/! \n/! \n- Aq \n- NAqrA G! Aq!A!    t  u!¡A!¢        ¡  ¢t ¢u Aq  \n)¸B|!£ \n)¨!¤ \n) !¥ \n)!¦ \n- Aq!§ \n £ ¤ ¥B  § ¦ : M@ \n- MAqE\r  \nA ; \nA ; \n)¸)!¨ \n)°!© \n)¨!ª \n) !« \n- !¬ \n)!­ \n)!® \n/!¯ \n/!° \n- Aq \n- MAqrA G!± ¬Aq!²A!³ ¯ ³t ³u!´A!µ ¨ © ª « ² ­ ® ´ ° µt µu ±Aq  \n)¸B|!¶ \n)°!· \n)¨!¸ \n) !¹ \n- !º \n)!» \n)!¼ \n/!½ \n/!¾ \n- !¿ ºAq!ÀA!Á ½ Át Áu!ÂA!Ã ¶ · ¸ ¹ À » ¼ Â ¾ Ãt Ãu ¿Aq  \n)¸B |!Ä \n)°!Å \n)¨!Æ \n) !Ç \n- !È \n)!É \n)!Ê \n/!Ë \n/!Ì \n- !Í ÈAq!ÎA!Ï Ë Ït Ïu!ÐA!Ñ Ä Å Æ Ç Î É Ê Ð Ì Ñt Ñu ÍAq  \n)¸B|!Ò \n)¨!Ó \n) !Ô \n)!Õ \n- Aq!Ö \n Ò Ó ÔB  Ö Õ : L@ \n- LAqE\r  \nA ; \nA ; \n)¸)!× \n)°!Ø \n)¨!Ù \n) !Ú \n- !Û \n)!Ü \n)!Ý \n/!Þ \n/!ß \n- Aq \n- LAqrA G!à ÛAq!áA!â Þ ât âu!ãA!ä × Ø Ù Ú á Ü Ý ã ß ät äu àAq  \nA 6H@@ \n(H \n)¸(IAqE\r@ \n)¸) \n(H­B|) B RAqE\r  \n)¸) \n5HB|!å \n)¨!æ \n) !ç \n)!è \n- Aq!é \n å æ çB  é è : G@ \n- GAqE\r  \nA ; \nA ; \n)¸) \n5HB|) !ê \n)°!ë \n)¨!ì \n) !í \n- !î \n)!ï \n)!ð \n/!ñ \n/!ò \n- Aq \n- GAqrA G!ó îAq!ôA!õ ñ õt õu!öA!÷ ê ë ì í ô ï ð ö ò ÷t ÷u óAq  \n)¸) \n5HB|B|!ø \n)¨!ù \n) !ú \n)!û \n- Aq!ü \n ø ù úB  ü û : F@ \n- FAqE\r  \nA ; \nA ; \n)¸) \n5HB|)!ý \n)°!þ \n)¨!ÿ \n) ! \n- ! \n)! \n)! \n/! \n/! \n- Aq \n- FAqrA G! Aq!A!  t u!A! ý þ ÿ       t u Aq  \n \n(HAj6H @ \n)¨B RAqE\r  \n) B RAqE\r  \n- Aq\r  \n)! \n)¸ 70 \n)¸/8!A!  t u! \n/!A!   t uj! \n)¸ ;8 \n)¸/:!A!  t u! \n/!A!   t uj! \n)¸ ;: \nBÀ|$ ¹~~# B }! $    7  7  7 )/ ! )!  Aÿÿq  6@ (AGAqE\r  ))  (­B|) ( AFAqE\r  ))  (­B|) /! ) ;  B |$ ~# B }!   ;  7 A 6@@@ ( )(IAqE\r@ ))  (­B|/ Aÿÿq /AÿÿqFAqE\r   (6  (Aj6  A6 (¦~~# BÐ }! $    6L  7@  78  70  Aq: /  7   )8)  5LB|) 7@@ - /AqE\r  (LAj )@(FAqE\r   )8)  )8(Ak­B|) B|7 A 6@@ ( )(IAqE\r  ))  5B|) 7  )@! )8! )0!	 - /!\n ) !@    	 \nAq  Aq\r  )@! ) !\r   \r  )0 )  )    (Aj6  )@! ) ! B|    )0 ) )   BÐ |$ Ï~~# B }! $    7  7 )( A{j! AK@@@@@ \r  )/! )!  Aÿÿq  AGAq:   ))B RAq:  A Aq:  AAq:  - Aq! B |$  ~# B0}! $    7(  7   7B !  7  7  )((6  ) (­B§ì 7 )! )() ! (­B!@ P\r    ü\n   A 6@@ ( (IAqE\r ) (­B| )  )   (Aj6  )(!  )7  )7  B0|$ ã~# B0}!   7   ;  6 A 6@@@ ( ) (IAqE\r  ) )  (­B8~|7@ )/ Aÿÿq /AÿÿqFAqE\r @ )( (FAq\r  )- (AqE\r )( (AjMAqE\r  )7(  (Aj6  B 7( )(~# B0}! $    7(  7   7  7 A 6@@ ( )((IAqE\r )()  (­B|)  )  ) )   (Aj6  B0|$ ²~# BÀ }! $    78  70  7(  7  )85 !@ BV\r @@@@@@@@@@@@@@@@@@ § 	\n\r  )8B| )0 )( )  \r A 6@@ ( )0(IAqE\r@ )8/Aÿÿq )0)  (­B|/ AÿÿqFAqE\r  )()  (­B|/ ! )8 ;  (Aj6  A 6@@ ( )8(IAqE\r A 6@@ ( )0(IAqE\r@ )8) (­B|/ Aÿÿq )0)  (­B|/ AÿÿqFAqE\r  )()  (­B|/ ! )8) (­B| ;   (Aj6   (Aj6  )8B|B| )0 )( )   )8B| )0 )( )  \n )8B| )0 )( )  	 )8B| )0 )( )   )8) )0 )( )   )8) )0 )( )   )8) )0 )( )   A 6@@ ( )0(IAqE\r@ )8/Aÿÿq )0)  (­B|/ AÿÿqFAqE\r  )()  (­B|/ ! )8 ;  (Aj6  )8) )0 )( )   )8) )0 )( )   )8B|B| )0 )( )   )8) )0 )( )   A 6@@ ( )0(IAqE\r@ )8/Aÿÿq )0)  (­B|/ AÿÿqFAqE\r  )()  (­B|/ !	 )8 	;  (Aj6 @ )8)B RAqE\r  )8) )0 )( )   )8) )0 )( )   )8B|B| )0 )( )   )8B|B| )0 )( )   )8) )0 )( )   A 6@@ ( )8(IAqE\r )8) (­B|)  )0 )( )   )8) (­B|) )0 )( )    (Aj6  BÀ |$ ~# B0}! $   7(  7   )(BÏ ¼ 7@@ )B RAq\r   B 7   A6  B|A 6  )B AÅ   )È §6  )  (ì 7 )B A Å  )! (­! )! B  Â  )²    )7   )7  B0|$ ­~# B }! $    7  )B ¼ 7@@ )B RAq\r  A Aq:  ) ! (­! )! B  Ë  )²  AAq:  - Aq! B |$  ß~# B}!   7 @@ ) ( \r  A Aq: @ ) ( AFAqE\r   ) ))B RAq: @ ) ( AFAqE\r   ) (A GAq: @ ) ( AFAqE\r   ) )B RAq: @ ) ( AFAqE\r   ) +B ¹bAq: @ ) ( AFAqE\r   ) - Aq: @ ) ( A	FAqE\r  ) (!A !@ E\r  ) )-  AÿqA G!  Aq:  AAq:  - Aq~# Bà }! $    7X B 7P )X5 !@ B	V\r @@@@@@@@@@ §\n 	 	  )X))7H@@ )HB RAqE\r  )H)   )P|7P  )XB|!  )7  ) 7  B|¶ 7P  )X)7P  )X+ü7P  )X- Aq­7P A 6D@@ (DA\nIAqE\r  )X) (D­B|) 78@@ )8B RAqE\r  )8)   )P|7P  )8)  )P|7P  )8)78   (DAj6D   )X)7(  )X(60 B(|B|A 6   )07   )(7  B|¶ 7P )P! Bà |$  ¡~~# B0}! $    7   7  ) B\n7  )  )B|) 7@ )B R!A ! Aq! !@ E\r  ))B R!A ! Aq!	 ! 	E\r  ))  )Þ As!@ AqE\r   ))7@@ )B RAqE\r  ))  )Þ AqE\r   )B|7( B 7( )(!\n B0|$  \n¼~~# B0}! $    7(  7   7  7  ) B\n7  )  )B|) 7 @ ) B R!A ! Aq! !@ E\r  ) )B R!	A !\n 	Aq! \n! E\r  ) )  )Þ As!@ AqE\r   ) )7 @@ ) B RAqE\r @@ ) )  )Þ AqE\r  ) )!  / Aj;  )(B|Aì !\r )  \r7  ) )7  )! )  7   )(B|Aì 7  )! )  7  ) ! )  )B| 7  )! )  7 B0|$ ¡	~~~~~~~~# B}! $    7ð  7è  7à  7Ø@@ )ð( AFAqE\r @ )è( AGAqE\r B )è¼ ! A6  B¿  ½ B )è¼ ! )à) (! )à) ) ! )à/AÿÿqAj!	 )à/\nAÿÿqAj!\n B$| \n6  B | 	6   7  6 BÀ  B|½  )ØA6° )ØB7¸ B 7ø A 6Ô  )ð))7È@ (Ô­ )è)S!A ! Aq!\r !@ \rE\r  )ÈB R!@ AqE\r   )È)7È  (ÔAj6Ô@ )ÈB RAq\r B )è¼ ! A¡60 B¿  B0|½ B )è¼ ! )à) (! )à) ) ! )à/AÿÿqAj! )à/\nAÿÿqAj! BÔ | 6  BÐ | 6   7H  6@ Bâ  BÀ |½  )ØA6° )ØB7¸ B 7ø  )È7ø@ )ð( AFAqE\r   )ð) )è 7øB !  7À  7¸ )ð! B¸|!A !A!    Aq Aq  B¨|  )À7   )¸7 B¨| B|¹ B )è¼ ! A­6` B¿  Bà |½ B )è¼ ! )à) (! )à) ) ! )à/AÿÿqAj! )à/\nAÿÿqAj! (°!  )¨!! B| !7  B|  6  B| 6  B| 6   7x  6p Bî  Bð |½  )ØA6° )ØB7¸ B 7ø )ø!" B|$  "¢~~~~~~~~~~~~~~~~# BÀ}! $    7¸  7°  6¬A!   q: «   q: ª )°5 !@@ B	V\r @@@@@@@@@@ §\n 	  )¸BÐ » \n )¸!AÛ !	A!\n  	 \nt \nuº   )°))7 @@ ) B RAqE\r@ )  )°))RAqE\r  )¸!A !A!\r   \rt \ruº  )¸! ) ) ! (¬! - «!A!    Aq Aq   ) )7   )¸!AÝ !A!   t uº 	@@ - «AqE\r  )¸BÂ » @ - ªAqE\r  )¸!A\'!A!   t uº  A 6@@ ( )°(IAqE\r  )°) (­|-  :  - !A!@@  t uA\nFAqE\r  - ªAqE\r  )¸Bü »  )¸! - !A!   t uº   (Aj6 @ - ªAqE\r  )¸!A\'!A!     t  uº @@ - «AqE\r  )¸B¢ »  )¸ )°)½ @@ - «AqE\r  )¸Bþ »  )¸ )°+À @@ - «AqE\r  )¸B¤ » @@ )°- AqE\r  )¸B¹ »  )¸BÍ »  )¸!!AÜ !"A!# ! " #t #uº  A 6@@ ( )°)(IAqE\r@ (A KAqE\r  )¸!$A !%A!& $ % &t &uº  )°))  (­B|/ !\' B| \'Aÿÿq  )¸!(  )7   )7 ( B|¼   (Aj6 @ )°)(A KAqE\r  )¸!)A !*A!+ ) * +t +uº @@ )°)/0AÿÿqAÿÿFAqE\r  )¸Bç »  )°)/0!, Bð | ,Aÿÿq  )¸!-  )x70  )p7( - B(|¼  )¸BÙ »  A 6l@@ (lA\nIAqE\r  )°) (l­B|) 7`@@ )`B RAqE\r A 6\\@@ (\\ (¬AjIAqE\r )¸BÖ »   (\\Aj6\\  )¸!. )`) !/ (¬!0A!1 . / 0 1j 1 - «q 1  )¸BÐ »  )¸!2 )`)!3 1 (¬j!4 - «!5A!6 2 3 4 5Aq 6Aq  )¸!7A\n!8A!9 7 8 9t 9uº   )`)7`   (lAj6l  A 6X@@ (X (¬IAqE\r )¸BÖ »   (XAj6X  )¸!:Aý !;A!< : ; <t <uº  )¸B¦ » @@ - «AqE\r  )¸BÔ »   )°)7H  )°(6P BÈ |B|A 6  )¸!=  )P7@  )H78 = B8|¼ B )è¼ !> A¶6  >B¿  ½ B )è¼ !?  )°( 6 ?B   B|½  BÀ|$ Ü~~# Bà }! $    7P  7H A 6D A 6@ A 6< A : 7@@ (@!  )7  ) 7 B|  B8|¦ E\r@ (@­ )HQAqE\r   (<6D A: 7  (@Aj6@  (8 (<j6<@ - 7AqE\r  @@ - 7Aq\r  B 7X  )  (D­|7   (< (Dk6( B |B|A 6  )P) !  )(7  ) 7    Õ 7X )X! Bà |$   Bðå ò~~~~~\n~~~# Bð}!\n \n$  \n 7è \n 7à \n 7Ø \n 7Ð \n 7È \n 7À \n : ¿ \n 	7° \n )7 \n ) 7  \n \n¶ 7¨ \nA 6¤@@@ \n(¤ \n)È(IAqE\r \n \n)È)  \n(¤­B|7@ \n))  \n)¨QAqE\r @ \n)à( \n)à( \n)( jIAqE\r @@ \n)à(E\r @@ \n)à( \n)à( \n)( jIAqE\r \n)à!  (At6  \n)à)  \n)à(­B8~§ ! \n)à 7  \n)à( \n)( j!\r \n)à \r6 \n)à(­B8~¤ ! \n)à 7  \n)à)  \n)à(­B8~|! \n))! \n)( ­B8~!@ P\r    ü\n   \n)( ! \n)à!   (j6@ \n)Ø( \n)Ø( \n)(0jIAqE\r @@ \n)Ø(E\r @@ \n)Ø( \n)Ø( \n)(0jIAqE\r \n)Ø!  (At6  \n)Ø)  \n)Ø(­B§ ! \n)Ø 7  \n)Ø( \n)(0j! \n)Ø 6 \n)Ø(­B¤ ! \n)Ø 7  \n)Ø)  \n)Ø(­B|! \n))(! \n)(0­B!@ P\r    ü\n   \n)(0! \n)Ø!   (j6 \n)À \n)B8|) 7  \n)B|!   )7   ) 7  \n \n(¤Aj6¤  \n)Ø \n)è¡ Bà !A ! \nB¸|  ü B !  \n  7° \n  7¨ \n  7  \n  7 \n  7 \n  7 \nB|!! ! )7 ! ) 7  \n 7  \nA6@@ \n(AGAqE\r \n)è!" \n \nB| \nBð | "¢ 6@ \n(AGAqE\r  \n(AGAqE\r @ \n(Ä \n(ÀMAqE\r @@ \n(ÄE\r @@ \n(Ä \n(ÀMAqE\r \n \n(ÄAt6Ä  \n \n)¸ \n(Ä­B§ 7¸ \nA6Ä \nB¤ 7¸ \n)¸ \n(À­B|B|!# \n)¸ \n(À­B|!$ \n(À \n(Àk­B!%@ %P\r  # $ %ü\n   \n)¸ \n(À­B|!& & \n)x7 & \n)p7  \n \n(ÀAj6À  \n)¨¦  \n \n)à7È \n \n)è7Ð \n \n)Ø7Ø \n \n)È7è \n \n)Ð7à \n \n)À7ð \n \n- ¿Aq: ø@@ \n)°B RAqE\r @ \n( \n(MAqE\r @@ \n(E\r @@ \n( \n(MAqE\r \n \n(At6  \n \n) \n(­B§ 7 \nA6 \nB¤ 7 \n) \n(­B|B|!\' \n) \n(­B|!( \n( \n(k­B!)@ )P\r  \' ( )ü\n   \n)°( !* \n) \n(­B| *6  \n \n(Aj6@ \n( \n(MAqE\r @@ \n(E\r @@ \n( \n(MAqE\r \n \n(At6  \n \n) \n(­B§ 7 \nA6 \nB¤ 7 \n) \n(­B|B|!+ \n) \n(­B|!, \n( \n(k­B!-@ -P\r  + , -ü\n   \n) \n(­B|A 6  \n \n(Aj6   \nB¸|B £ @ \n)°B RAqE\r  \n)( !. \n)° .6 @ \n)B RAqE\r  \n)¦ @ \n)¸B RAqE\r  \n)¸¦  \n \n)à(6h \n \n(h6l \n \n)À \n(l­B8~§ì 7` \n)`!/ \n)à) !0 \n(h­B8~!1@ 1P\r  / 0 1ü\n   \n \n)Ø(6X \n \n(X6\\ \n \n)À \n(\\­B§ì 7P \n)P!2 \n)Ø) !3 \n(X­B!4@ 4P\r  2 3 4ü\n   \n \n)¨7 \nB|B|!5 5  )7 5  ) 7  \nB|B|!6 6 \n)h7 6 \n)`7  \nB|B(|!7 7 \n)X7 7 \n)P7  \nB|B8| \n)À) 7 @ \n)È( \n)È(MAqE\r @@ \n)È(E\r @@ \n)È( \n)È(MAqE\r \n)È!8 8 8(At6  \n)È)  \n)È(­B§ !9 \n)È 97  \n)ÈA6BÀ ¤ !: \n)È :7  \n)È)  \n)È(­B|BÀ |!; \n)È)  \n)È(­B|!< \n)È( \n)È(k­B!=@ =P\r  ; < =ü\n   \n)È)  \n)È(­B|!> > \n)H78 > \n)@70 > \n)87( > \n)07  > \n)(7 > \n) 7 > \n)7 > \n)7  \n)È!? ? ?(Aj6 \nBð|$ ú~# BÀ }! $    78  70 A 6,@@@ (, )8(IAqE\r )8)  (,­B|) ! )0!  )7   ) 7  )7  ) 7@ B| B|² AqE\r   (,Aj6, @ )8( )8(MAqE\r @@ )8(E\r @@ )8( )8(MAqE\r )8!  (At6  )8)  )8(­B§ ! )8 7  )8A6B¤ ! )8 7  )8)  )8(­B|B|! )8)  )8(­B|!	 )8( )8(k­B!\n@ \nP\r   	 \nü\n   )0! )8)  )8(­B| 7  )8!  (Aj6 BÀ |$ ï ~~~~~~~~~~~~~~~~# BÀ}! $    7°  7¨  7 @@ )°(A KAqE\r  B 7 )°)! )°! B|   B| B|§   )°(;þ  )°(;ü@ )BQAqE\r  )°!  (Aj6 )°A 6 A6¼@ )BQAqE\r @ )°!  )7  ) 7  A  Bø|¦ !  6ôA !	@ E\r  (ôA\nG!	@ 	AqE\r  (ø!\n )°!  )  \n­|7  (ø! )°!\r \r \r( k6 A6¼@ )B QAqE\r  (! )°!   (j6 A6¼@ )BQAqE\r  )°!  )7H  ) 7@A !  BÀ |  Bð|¦ 6ìB )è¼ ! AÞ6 B  B|½ B )è¼ ! ) (! ) ) ! )°(Aj! )°(Aj! (ì! B8| 6  B4| 6  B0| 6   7(  6  B°  B |½ A  @@ )BQAqE\r  )°B |! )°) B|-  !A!   t uº  A : ë@ )°(A K!A ! Aq! !@ E\r  )°) -  ! A!!   !t !u!" )°) -  !#A!$ " # $t $uG!%A!& %Aq!\' &!(@ \'\r  - ë!( (!@ AqE\r  )°!)  ))7X  )) 7PA !*  BÐ | * Bä|¦ 6à@@ - ëAq\r  (àAÜ GAqE\r@@ - ëAqE\r  )°B |!+ )° )°B|¤ !,A!- + , -t -uº  A 6Ü@@ (Ü (äIAqE\r )°B |!. )°)  (Ü­|-  !/A!0 . / 0t 0uº   (ÜAj6Ü @@ - ëAqE\r  A : ë@ (àAÜ FAqE\r  A: ë (ä!1 )°!2 2 2)  1­|7  (ä!3 )°!4 4 4( 3k6 )°!5 5 5(Aj6@ )°(\r B )è¼ !6 A6` 6B  Bà |½ B )è¼ !7 ) (!8 ) ) !9 /þAÿÿqAj!: /üAÿÿqAj!; B| ;6  B| :6   97x  86p 7B  Bð |½ A   )°B |!< )°) -  !=A!> < = >t >uº  )°!? ? ?) B|7  )°!@ @ @(Aj6 )°!A A A(Aj6 )°B |!B BÈ|  B)7¨  B) 7  BÈ| B |¹   )Ð7  )È7 )°A 6, (!C )°!D D C D(j6  )7  )7  B| ;Æ )¨!E  )7°  /Æ;¸  /þ;º  /ü;¼ B°|B|A ;  E )¸7 E )°7  A 6¼ A6¼ (¼!F BÀ|$  FÑ~~# BÀ }! $   78  70B !  7(  7   )8¥ 7@ )B R!A ! Aq! !@ E\r  )) !	B 	 )0B RAs!@ AqE\r   )8A Aq¦ 7@ (, ((MAqE\r @@ (,E\r @@ (, ((MAqE\r  (,At6,   )8)8 (,­B§ì 7 )!\n ) ! ((­B!@ P\r  \n  ü\n    )7  A6,  )8)8Aì 7  )!\r ) ! ((!  Aj6(  ­B| \r7   )8¥ 7   ) 7    ((6  B|A 6  BÀ |$ ©\n~~8~	~~# B }!   7  7  )) -  :  , APj! AÈ K@@@@@@@@@@@@@ I	 \n A\n:  A\r: \n A	: 	 A:  A:  A:  A :  AÜ :  A :  )!  ) B|7  )!  (Aj6 )!  ( Aj6 @ )(A K!A ! Aq!	 !\n@ 	E\r  )) -  !A!@@  t uA0NAqE\r  )) -  !\rA! \r t uA9L!A! Aq! ! \r )) -  !A!@  t uAá NAqE\r  )) -  !A!  t uAæ L!A! Aq! ! \r )) -  !A!  t uAÁ N!A ! Aq! !@ E\r  )) -  ! A!!   !t !uAÆ L! ! !\n@ \nAqE\r  - !"A!#  " #t #uAt:  )) -  !$A!%@@ $ %t %uA0NAqE\r  )) -  !&A!\' & \'t \'uA9LAqE\r  )) -  !(A!) ( )t )uA0k!* - !+A!,  * + ,t ,uj:  )) -  !-A!.@@ - .t .uAá NAqE\r  )) -  !/A!0 / 0t 0uAæ LAqE\r  )) -  !1A!2 1 2t 2uAá kA\nj!3 - !4A!5  3 4 5t 5uj:  )) -  !6A!7@ 6 7t 7uAÁ NAqE\r  )) -  !8A!9 8 9t 9uAÆ LAqE\r  )) -  !:A!; : ;t ;uAÁ kA\nj!< - !=A!>  < = >t >uj:  )!? ? ?) B|7  )!@ @ @(Aj6 )!A A A) B|7  )!B B B(Aj6 )!C C C( Aj6   - :  A :  )!D D D) B|7  )!E E E(Aj6 )!F F F( Aj6 @ )(A K!GA !H GAq!I H!J@ IE\r  )) -  !KA!L K Lt LuA0N!MA !N MAq!O N!J OE\r  )) -  !PA!Q P Qt QuA9L!J@ JAqE\r  - !RA!S  R St SuA\nl:  )) -  !TA!U@ T Ut UuA0NAqE\r  )) -  !VA!W V Wt WuA9LAqE\r  )) -  !XA!Y X Yt YuA0k!Z - ![A!\\  Z [ \\t \\uj:  )!] ] ]) B|7  )!^ ^ ^(Aj6 )!_ _ _( Aj6  )!` ` `) B|7  )!a a a(Aj6 )!b b b( Aj6   - :  A :  )!c c c) B|7  )!d d d(Aj6 )!e e e( Aj6 @ )(A K!fA !g fAq!h g!i@ hE\r  )) -  !jA!k j kt kuA0N!lA !m lAq!n m!i nE\r  )) -  !oA!p o pt puA7L!i@ iAqE\r  - !qA!r  q rt ruAt:  )) -  !sA!t@ s tt tuA0NAqE\r  )) -  !uA!v u vt vuA7LAqE\r  )) -  !wA!x w xt xuA0k!y - !zA!{  y z {t {uj:  )!| | |) B|7  )!} } }(Aj6 )!~ ~ ~( Aj6  )!  ) B|7  )!  (Aj6 )!  ( Aj6   - :   - :  - !A!  t u\\~# B}!   7 @@ ) (D ) (FAqE\r  B 7  ) )  ) (D­B|7 )Æ/*~~~~|~~~~~~~~~~~~~~~~~# BÀ}! $    7°A!   q: ¯  )°)8AÀ ì 7   )°Bª§ 7 )°)! )  70 )/\n! )  ;8 )/! )  ;:  :  )) Bo|!@@ BV\r @@@@@@@@ §  )/! B| Aÿÿq  ) A 6   )B|7ð  (Ak6ø Bð|B|A 6   )ø7   )ð7 B| !	 )  	; ) A6  )/!\n )  \n; )/! Bà| Aÿÿq  ) A6   )è7°  )à7¨ B¨|µ ! )  7 )/!\r BÐ| \rAÿÿq  ) A6   )Ø7À  )Ð7¸ B¸|· ! )  9 ) A6  ) B|! )°! BÀ| B£   )È7  )À7  )°B§  ) A6  ) B|! )°! B| ¨   )¸7   )°7  )¨7  ) 7  )7  ) A6  ) B|! )°! B| ©   )7  )7  ) A6  A : @@ - Aq\r   )°¥ 7 )) B}|!@@ BV\r @@@@@@@@@ § 			  )°ª   )°B§ 7ø ) A\r6  )ø/! )  ; )°A Aq¦ ! )  7 )°B§ 	 ) A6  ) B|! )°! BÐ| «   )ð7   )è7  )à7  )Ø7  )Ð7  )°¬  ) A6  )°ª   )°B§ /;Î /Î! B¸| Aÿÿq   )¸B|7¸  (ÀAk6À )°B§ B !  7°  7¨B !  )À 7   )¸ 7B !  7  7 A 6@@ ( )°)((IAqE\r )°)()  5B|!  )7  ) 7 B¨| B|¼   )À7  )¸7ø B¨| Bø|¼ @ )°- @AqE\r  BÈ 7ð A6ø Bð|B|A 6  B¨|  )ø7ð  )ð7è B¨| Bè|¼  )¨! )°)8!  Bà|      )è7   )à7@ ( AGAqE\r   (´Aj6´ BÈ|  )°7   )¨7 BÈ| B|¹   )Ð7°  )È7¨  B¨| ;Þ /Þ!! B¸| !Aÿÿq   )À7  )¸7@ )°- @AqE\r   (´Ak6´ BÎ 7¨ A6° B¨|B|A 6  B¨|  )°7à  )¨7Ø B¨| BØ|¼  )¨!" )°)8!# B| " #   ) 7   )7@ ( AGAqE\r   (´Aj6´ B|  )°7À  )¨7¸ B| B¸|¹   )7Ð  )7È  BÈ| ; /!$ Bð| $Aÿÿq   )ø7  )ð7 A 6´  (Aj6 @ )¨B RAqE\r  )¨¦ @ ( AFAqE\r B )è¼ !% A6  %B  ½ B )è¼ !& )°)(!\' )°)) !( )/\nAÿÿqAj!) )/AÿÿqAj!* (À!+ )¸!, B0| ,7  B(| +6  B$| *6  B | )6   (7  \'6 &B  B|½ A   ) A6  A 6ì@@ (ì )°) (IAqE\r )°) )  (ì­B|) !-  -)7P  -) 7H  )7@  )78@ BÈ | B8|² AqE\r   ) 7¸  (ìAj6ì  BØ|  )7  )7 BØ| B| @ )°)(( )°)((MAqE\r @@ )°)((E\r @@ )°)(( )°)((MAqE\r )°)(!. . .(At6  )°)()  )°)((­B§ !/ )°)( /7  )°)(A6B¤ !0 )°)( 07  )°)()  )°)((­B|B|!1 )°)()  )°)((­B|!2 )°)(( )°)((k­B!3@ 3P\r  1 2 3ü\n   )°)()  )°)((­B|!4 4 )à7 4 )Ø7  )°)(!5 5 5(Aj6  )7È A6Ð BÈ|B|A 6   )°)8Aì 7À )À!6 6 )7 6 )7  )°)  )À¡  BÓ 7° A6¸ B°|B|A 6   )Ð7  )È7x  )¸7p  )°7h@@ Bø | Bè |² AqE\r  )!7 ( !8 )°) !9 )°)8!: B | 7 8 9 :Ì @ )°)( )°)( (¨jIAqE\r  )°)( (¨j!; )°) ;6@@ )°)(\r  )°)(­B8~¤ !< )°) <7  )°))  )°)(­B8~§ != )°) =7  )°))  )°)(­B8~|!> ) !? (¨­B8~!@@ @P\r  > ? @ü\n   (¨!A )°)!B B A B(j6 B 7 ) B|!C )À!D )°!E E)!F E) !G E)(!H E)0!I E- @!J )°)P!K B|  ) 7`  )7X JAq!L B| BØ | D F G H I B| L K   C )7 C )7  )°ª   )°ª /; ) A6  /!M )  M; )°A Aq¦ !N )  N7 )°B§  )°ª  ) A\n6  )°A Aq¦ !O )  O7 )°A Aq¦ !P )  P7 )°A Aq¦ !Q )  Q7 )°B§  )°ª  ) A6   )°¥ 7@ )B RAqE\r  )) BRAqE\r  )°A Aq¦ !R )  R7 )°B§  ) A6  ) B|!S )°!T Bè| T­  S )ø7 S )ð7 S )è7  )°ª  ) A6  ) B|!U )°!V BØ| VB£  U )à7 U )Ø7  )°B§  ) A6  )°A Aq¦ !W )  W7B !X  X7Ð  X7È  )°¥ 7À@ )ÀB R!YA !Z YAq![ Z!\\@ [E\r  )À) BR!\\@ \\AqE\r   )°A Aq¦ 7¸@ (Ô (ÐMAqE\r @@ (ÔE\r @@ (Ô (ÐMAqE\r  (ÔAt6Ô   )°)8 (Ô­B§ì 7° )°!] )È!^ (Ð­B!_@ _P\r  ] ^ _ü\n    )°7È A6Ô  )°)8Aì 7È )¸!` )È!a (Ð!b  bAj6Ð a b­B| `7   )°¥ 7À ) B|B|!c  )È7   (Ð6¨ B |B|A 6  c )¨7 c ) 7  )°B§ @ - ¯Aq\r   )°¥ 7  )7@ )B RAqE\r  )) BQAqE\r B !d  d7  d7B !e  e7ø  e7ð@ ( (MAqE\r @@ (E\r @@ ( (MAqE\r  (At6   )°)8 (­B§ì 7è )è!f )!g (­B!h@ hP\r  f g hü\n    )è7 A6  )°)8Aì 7 ) !i )!j (!k  kAj6 j k­B| i7 @ )B R!lA !m lAq!n m!o@ nE\r  )) BQ!o@ oAqE\r  )°ª   )°AAq¦ 7à@ ( (MAqE\r @@ (E\r @@ ( (MAqE\r  (At6   )°)8 (­B§ì 7Ø )Ø!p )!q (­B!r@ rP\r  p q rü\n    )Ø7 A6  )°)8Aì 7 )à!s )!t (!u  uAj6 t u­B| s7   )°¥ 7 Bð|!v  )7È  (6Ð BÈ|B|A 6  v )Ð7 v )È7   )°)8AÀ ì 7  ) A	6  ) B|!w w )ø7 w )ð7  )°)!x )  x70 )/\n!y )  y;8 )/!z )  z;:  ) 7¸ )¸!{ BÀ|$  {ø\n~~~~~# B}! $    7  7  )ª 7x@ )xB RAq\r B )è¼ ! AÁ6@ B  BÀ |½ B )è¼ ! ))(!  ))) 7X  6P B  BÐ |½  )® B )è¼ BÖ B ½ A   )x) !@B  )B RAqE\r  )x! B|$   )x/! Bè | Aÿÿq B )è¼ !	 AÎ6 	B  B|½ B )è¼ !\n ))(! ))) ! )x/\nAÿÿqAj!\r )x/AÿÿqAj! B4| 6  B0| \r6   7(  6  \nBÿ  B |½  )® B )è¼ ! (p!  )h7  6  B  ½ A  ~~~~# B}! $   7xB !   7    7   7   7   7   )xB§ 7p@ )pB R!A ! Aq! !@ E\r  )p) BR!@ AqE\r @  (  (MAqE\r @@  (E\r @@  (  (MAqE\r    (At6   )x)8  (­B§ì 7h )h!  ) !	  (­B!\n@ \nP\r   	 \nü\n     )h7   A6   )x)8Aì 7  )p/!  ) !  (!\r   \rAj6  \r­B| ;   )xB§ 7p  )x¥ 7`@@ )`B RAqE\r  )`) BQAqE\r  )xª   )xB§ 7X )X/! BÈ | Aÿÿq   )HB|78  (PAk6@ B8|B|A 6   )@7  )87   B| ;   )x(H64  B|! )x! B | BÔ £   )(7  ) 7   Aÿÿ;  (4! )x 6H  )x¥ 7@ )B RAqE\r  )) BQAqE\r  )xª  B|$ Ú~~~# BÀ }! $   78B !  70  7(  )8¥ 7 @ ) B R!A ! Aq! !@ E\r  ) ) BR!@ AqE\r   )8A Aq¦ 7@ (4 (0MAqE\r @@ (4E\r @@ (4 (0MAqE\r  (4At64   )8)8 (4­B§ì 7 )! )(!	 (0­B!\n@ \nP\r   	 \nü\n    )7( A64  )8)8Aì 7( )! )(! (0!\r  \rAj60  \r­B| 7  )8B§   )8A Aq¦ 7@ (4 (0MAqE\r @@ (4E\r @@ (4 (0MAqE\r  (4At64   )8)8 (4­B§ì 7 )! )(! (0­B!@ P\r    ü\n    )7( A64  )8)8Aì 7( )! )(! (0!  Aj60  ­B| 7   )8¥ 7  )8BÀ §    )(7    (06  B|A 6  BÀ |$ t~# B}!   7 @@ ) (D ) (FAqE\r  B 7 ) ) ! ) ! (D!  Aj6D   ­B|7 )ë~~~# BÐ }! $   7H )Hª B !   7    7   7   7   7    )HA Aq¦ 7   B|! )H! B8| Bà£   )@7  )87    70  )HBà§ 7(@ )(B R!A ! Aq! !	@ E\r  )() BQ!	@ 	AqE\r   )H)8AÀ ì 7  ) A6  )HA Aq¦ !\n )  \n7 ) B|B|! )H! B| Bà£   )7  )7  )H)!\r )  \r70 )(/\n! )  ;8 )(/! )  ;: )0A6  )H)8Aì ! )0 7 ) ! )0) 7   ) B|70  )HBà§ 7(@ )() BQAqE\r  )0B|! )H!  B£   )7  ) 7  )HB§  BÐ |$ Õ\n~~~~~\n~# B}! $    7xB !  7p  7h  7`  7X  7P  7H  7@  )xª 78  )8/\nAÿÿq6l  )8/Aÿÿq6p  )xB§ 70  )0/;@ )xB§   )x¥ 7(@ )(B R!A ! Aq! !@ E\r  )() BR!@ AqE\r   )xB§ 7 @ ) ) BQAqE\r  A: h  )xB§ 7 @ (T (PMAqE\r @@ (TE\r @@ (T (PMAqE\r  (TAt6T   )x)8 (T­B§ì 7 )! )H! (P­B!	@ 	P\r    	ü\n    )7H A6T  )x)8Aì 7H ) /!\n )H! (P!  Aj6P  ­B| \n; @ (T (PMAqE\r @@ (TE\r @@ (T (PMAqE\r  (TAt6T   )x)8 (T­B§ì 7 )!\r )H! (P­B!@ P\r  \r  ü\n    )7H A6T  )x)8Aì 7H ) /! )H! (P!  Aj6P  ­B| ;   )x¥ 7( )xB§  BÀ |B|! )x!  B£   )7  ) 7  )xB§ @ )x)( )x)(MAqE\r @@ )x)(E\r @@ )x)( )x)(MAqE\r )x)!  (At6  )x))  )x)(­B8~§ ! )x) 7  )x)A6B8¤ ! )x) 7  )x))  )x)(­B8~|B8|! )x))  )x)(­B8~|! )x)( )x)(k­B8~!@ P\r    ü\n   )x))  )x)(­B8~|!  )p70  )h7(  )`7   )X7  )P7  )H7  )@7  )x)!  (Aj6 B|$ Ù~~~~# Bà }! $   7X )Xª B !   7   7   7    )XA Aq¦ 7 @ )X¥ !  7P B R!A ! Aq! !@ E\r  )P) BR!@ AqE\r @ )P) BQAqE\r  )Xª  )XBÀ §   )XA Aq¦ 7H B 78  )H7@@  (  (MAqE\r @@  (E\r @@  (  (MAqE\r    (At6   )X)8  (­B§ì 70 )0!	  )!\n  (­B!@ P\r  	 \n ü\n     )07  A6   )X)8Aì 7  )!  (!\r   \rAj6  \r­B|!  )@7  )87   )XA Aq¦ 7( )XBÀ §   )XA Aq¦ 7   )(7  ) 7@  (  (MAqE\r @@  (E\r @@  (  (MAqE\r    (At6   )X)8  (­B§ì 7 )!  )!  (­B!@ P\r    ü\n     )7  A6   )X)8Aì 7  )!  (!   Aj6  ­B|!  )7  )7  )XB§  Bà |$ ¶~~# B0}! $    7( A 6$ B 7@@ )B"TAqE\r )!@B  )(B RAqE\r   ($Aj6$  )B|7  B 7 B 7@ )BÀ T!A ! Aq! !@ E\r  ) ($­T!@ AqE\r  )!@B  )(B RAqE\r @ )B VAqE\r @@ )B| ($­QAqE\r B )è¼ !Bú  ¾ B )è¼ !	BÓ  	¾  )!\nBæ  \nB|) B )è¼ ¾   )B|7  )B|7 B0|$ ~# B }! $    7  7  )) 7 @@ ) ))B RAq\r   )) Ó 7  ) ))) 7 )! B |$  Â~# B0}! $    7   7  )) 7@@ )))B RAq\r   ) ) Ó 7(  ) ) B|Aì 7 ))))! ) 7  ) ) ) Ô 7( )(! B0|$  â~~# B0}! $    7   7  )) 7@@ )))B RAq\r   ) ) Ó 7(  )))7@ )B R!A ! Aq! !@ E\r  ))B R!@ AqE\r   ))7  )) 7( )(! B0|$  Ç~# B°}! $    7   7  )) 7  ))7@@@ )( AFAqE\r   )))7 A 6|@@ )B RAqE\r@ ))  )Þ AqE\r   (|­ ) ) × 7¨  ))7  (|Aj6| @@ )( AFAqE\r @ )( )(MAqE\r  A 6x@@ (x )( )(kIAqE\r  )) (x­|7h  )(6p Bè |B|A 6  )B|!  )p7  )h7  )7  ) 7 @ B| ² AqE\r   (x­ ) ) × 7¨  (xAj6x @ )( A	FAqE\r   ))7X  )(6` BØ |B|A 6 @ )( )(MAqE\r  A 6T@@ (T )( )(kIAqE\r  )) (T­|7@  )(6H BÀ |B|A 6   )H78  )@70  )`7(  )X7 @ B0| B |² AqE\r   (T­ ) ) × 7¨  (TAj6T   ) ) Ó 7¨ )¨! B°|$  «~~# BÐ }! $    7@  78  )8) 70@@ )0( AFAqE\r   )0))7( A 6$@@ )(B RAqE\r  )()7(  ($Aj6$   ($­ )@) × 7H@ )0( AFAqE\r  A 6  A 6@@ )0B|! (!  )7  ) 7 B|  B|¦ E\r  ( Aj6   ( (j6   ( ­ )@) × 7H@ )0( A	FAqE\r   )0(­ )@) × 7H  )@) Ó 7H )H! BÐ |$  Ä~# B }! $    7  7  )) 7 @@ ) ( AFAqE\r   ) (­ )) × 7@ ) ( A	FAqE\r   ) (­ )) × 7  )) Ó 7 )! B |$  	~~# BÀ}! $    7°  7¨  )¨) 7   )¨)7  )¨)7@@ ) ( A	FAqE\r   )° B |´ 7  )° B |³ 7@ ))B SAqE\r  )B 7@ )) ))UAqE\r  ))! ) 7@ )) ))UAqE\r  ))! ) 7@ )) ))SAqE\r  ))! ) 7@@ ) ( AFAqE\r   ) ))7  )°) B|Aì 7x  )x7p A 6l@@ (l ))§IAqE\r  ))7  (lAj6l  A 6h@@ (h­ ))§­ ))}SAqE\r )°) B|Aì ! )p 7 )) ! )p) 7   ))7  )p)7p  (hAj6h   )x )°) Ô 7¸@ ) ( AFAqE\r  A 6d A 6` A 6\\@ ) B|! (`!	  )7   ) 7 B| 	 BØ |¦ !\nA !@ \nE\r  (`­ ))S!@ AqE\r @ (`­ ))QAqE\r   (\\6d  (`Aj6`  (X (\\j6\\  ) ) (d­|7H  (\\ (dk6P BÈ |B|A 6  )°) !  )P7  )H7  B| Õ 7¸@ ) ( A	FAqE\r   ) ) ))|78  )) ))}§6@ B8|B|A 6  )°) !\r  )@70  )87(  B(| \rÖ 7¸  )°) Ó 7¸ )¸! BÀ|$  ¶~# BÀ }! $    78  70  )0) 7(  )0)7   )8) B|Aì 7  )B|7  )()7@@ ) ) )SAqE\r )8) B|Aì ! ) 7  )8) Ñ ! ))  7  )) ) A6  )! )) )  7  )) B|7  )B|7  ) )8) Ô ! BÀ |$  Þ~~~~# BÐ }! $    7@  78  )8) 70  )8)7( B 7  B 7 )(/"!A !@ Aÿÿq AÿÿqGAq\r   )@) B|Aì 7   ) B|7  )())7@@ )B RAqE\r@ )@( )@(MAqE\r @@ )@(E\r @@ )@( )@(MAqE\r )@!  (At6  )@) )@(­B§ ! )@ 7 )@A6B¤ ! )@ 7 )@) )@(­B|B|! )@) )@(­B|!	 )@( )@(k­B!\n@ \nP\r   	 \nü\n   )) ! )@) )@(­B| 7  )@!  (Aj6 )@ )0)B A Aqñ @ )@(°AFAqE\r   )@õ 7 )(/"!\rA !@@ \rAÿÿq AÿÿqGAqE\r  )) !  / Aj;  )! ) 7  )@) B|Aì ! ) 7  )! ))  7   )) B|7 )@!  (Ak6  ))7  )(/"!A !@@ Aÿÿq AÿÿqGAqE\r   )(7H  )  )@) Ô 7H )H! BÐ |$  Ë	~	~~~~# Bà }! $    7P  7H  )H) 7@  )H)78 B 70 B 7( )8/"!A !@ Aÿÿq AÿÿqGAq\r   )P) B|Aì 70  )0B|7(  )8)7   )8))7@@@ )B RAqE\r@ )P( )P(MAqE\r @@ )P(E\r @@ )P( )P(MAqE\r )P!  (At6  )P) )P(­B§ ! )P 7 )PA6B¤ ! )P 7 )P) )P(­B|B|! )P) )P(­B|!	 )P( )P(k­B!\n@ \nP\r   	 \nü\n   )) ! )P) )P(­B| 7  )P!  (Aj6 )P )@)B A Aqñ @ )P(°AFAqE\r   )Põ 7@ )( AGAqE\r B )è¼ !\r A6  \rB«  ½ B )è¼ Bý B ½  )PA6° )PB7¸  )P) Ó 7X@@ )- AqE\r  )8/"!A !@ Aÿÿq AÿÿqGAq\r  )P) B|Aì ! )( 7  )) ! )()  7   )() B|7(  ) )7  )8/"!A !@ Aÿÿq AÿÿqGAqE\r  ) )) !  / Aj;  ))! )  7 )P!  (Ak6  ))7  )8/"!A !@ Aÿÿq AÿÿqGAqE\r   )87X  )0 )P) Ô 7X )X! Bà |$  Þ~# BÀ }! $    78  70  )0) 7(  )0)7   )0)7  ) 7  )))7@@ )B RAqE\r@ )8( )8(MAqE\r @@ )8(E\r @@ )8( )8(MAqE\r )8!  (At6  )8) )8(­B§ ! )8 7 )8A6B¤ ! )8 7 )8) )8(­B|B|! )8) )8(­B|! )8( )8(k­B!@ P\r    ü\n   )!	 )8) )8(­B| 	7  )8!\n \n \n(Aj6@ )8( )8(MAqE\r @@ )8(E\r @@ )8( )8(MAqE\r )8!  (At6  )8) )8(­B§ ! )8 7 )8A6B¤ !\r )8 \r7 )8) )8(­B|B|! )8) )8(­B|! )8( )8(k­B!@ P\r    ü\n   )) ! )8) )8(­B| 7  )8!  (Aj6 )8 )()B A Aqñ @ )8(°AFAqE\r   )8õ 7 )8!  (Ak6  ))7  )! BÀ |$  æ~~~# BÐ }! $    7H  7@  )@) 78  )@)70 B 7( B 7  )8/"!A !@ Aÿÿq AÿÿqGAq\r   )H) B|Aì 7(  )(B|7   )8))7  )0))7@ )B R!A ! Aq! !@ E\r  )B R!@ AqE\r  )H) B|Aì !	 )  	7   )H) B|Aì 7 )H) B|Aì !\n ) \n7 )) ! )) 7  )H) B|Aì ! )) 7 )) !\r ))) \r7  )8/"!A !@@ Aÿÿq AÿÿqGAqE\r  ) )H) Ô ! ) 7  ) )H) Ô ! ) )  7   ) ) B|7   ))7  ))7 )( )H) Ô ! BÐ |$  ý\r~~~~~~~# B }! $    7  7  )) 7  ) )³ 7x  )x)B¤ 7p  )))7h A 6d@@ )hB RAqE\r )h) ! )p (d­B| 7   )h)7h  (dAj6d B !  )ø 7X  )ð 7P  )è 7H  )à 7@ A 6<@@ (<­BTAqE\r (<­!  BÀ | B|( 68@@ (8 )x)§IAqE\r  )p (8­B|) 70  (86,@ (,! (<­!  BÀ | B|( O!A !	 Aq!\n 	!@ \nE\r  )p! (,!\r (<­!  \r BÀ | B|( k­B|)  )0¼ !@ AqE\r  )p! (,! (<­!   BÀ | B|( k­B|) ! )p (,­B| 7  (<­! BÀ | B|( !  (, k6, )0! )p (,­B| 7   (8Aj68   (<Aj6<  )/"!A !@@ Aÿÿq AÿÿqGAqE\r   )))7  A 6@@ ( )x)§IAqE\r )p (­B|) ! )  7   ) )7   (Aj6  )p¦   )7  )) B|Aì 7  )B|7 A 6@@ ( )x)§IAqE\r )) B|Aì ! ) 7  )p (­B|)  )) Ð ! ))  7   )) B|7  (Aj6  )p¦   ) )) Ô 7 )! B |$  ¤~\n# B0}!   7   7@@ ) (  )( GAqE\r  A Aq: / ) 5 !@ B	V\r @@@@@ §\n @@ ) ( )(IAqE\r  ) ! )!  7 A 6@@ ( )(IAqE\r ) ) (­|-  !A!  t u! )) (­|-  !A!	@   	t 	uJAqE\r  AAq: / ) ) (­|-  !\nA! \n t u! )) (­|-  !\rA!@  \r t uHAqE\r  A Aq: /  (Aj6   ) ( )(KAq: /  ) ) ))UAq: /  ) + )+dAq: /  ) - Aq )- AqJAq: / A Aq: / A Aq: / - /Aqã~~~# Bð}! $    7à  7Ø  )Ø) 7Ð  )Ø)7È@@@ )Ð( AFAqE\r   )Ð))7À@@ )ÀB RAqE\r@ )à( )à(MAqE\r @@ )à(E\r @@ )à( )à(MAqE\r )à!  (At6  )à) )à(­B§ ! )à 7 )àA6B¤ ! )à 7 )à) )à(­B|B|! )à) )à(­B|! )à( )à(k­B!@ P\r    ü\n   )À) !	 )à) )à(­B| 	7  )à!\n \n \n(Aj6 )à )È)B A Aqñ @ )à(°E\r   )àõ 7¸@ )à(°AFAqE\r  )¸( \r  )àA 6° )à!  (Aj6  )¸7è  )À)7À )à!  (Aj6 @@ )Ð( AFAqE\r   )à) B|Aì 7°  )°7 A6  B|B|A 6  )à) !\r  ) 7  )7  B| \rÕ 7¨@ )à( )à(MAqE\r @@ )à(E\r @@ )à( )à(MAqE\r )à!  (At6  )à) )à(­B§ ! )à 7 )àA6B¤ ! )à 7 )à) )à(­B|B|! )à) )à(­B|! )à( )à(k­B!@ P\r    ü\n   )¨! )à) )à(­B| 7  )à!  (Aj6 A 6@@ )ÐB|! (!  )7  ) 7    B|¦ !  6 E\r (! )¨) 6  )à )È)B A Aqñ @ )à(°E\r   )àõ 7@ )à(°AFAqE\r  )( \r  )àA 6° )à!  (Aj6  )7è  ( (j6 )à!  (Aj6  )à!  (Aj6@@ )Ð( A	FAqE\r  )à) ! B  × 7ø@ )à( )à(MAqE\r @@ )à(E\r @@ )à( )à(MAqE\r )à!  (At6  )à) )à(­B§ ! )à 7 )àA6B¤ !  )à  7 )à) )à(­B|B|!! )à) )à(­B|!" )à( )à(k­B!#@ #P\r  ! " #ü\n   )ø!$ )à) )à(­B| $7  )à!% % %(Aj6 A 6ô@@ (ô )Ð(IAqE\r )Ð) (ô­|-  Aÿq­!& )ø &7 )à )È)B A Aqñ @ )à(°E\r   )àõ 7è@ )à(°AFAqE\r  )è( \r  )àA 6° )à!\' \' \'(Aj6  )è7è )à!( ( ((Aj6  (ôAj6ô  )à!) ) )(Aj6@ )Ð( AFAqE\r   )à) B|AÐ ì 7à BÒ 7È A6Ð BÈ|B|A 6  )à) !*  )Ð7(  )È7   B | *Õ 7Ø )à)  )à )ØB   B¾ 7° A6¸ B°|B|A 6  )à) !+  )¸78  )°70  B0| +Õ 7À )à)  )à )ÀB    )à )à) Ú 7¨@ )à( )à(MAqE\r @@ )à(E\r @@ )à( )à(MAqE\r )à!, , ,(At6  )à) )à(­B§ !- )à -7 )àA6B¤ !. )à .7 )à) )à(­B|B|!/ )à) )à(­B|!0 )à( )à(k­B!1@ 1P\r  / 0 1ü\n   )¨!2 )à) )à(­B| 27  )à!3 3 3(Aj6 A 6¤@@ (¤A\nIAqE\r  )Ð) (¤­B|) 7@@ )B RAqE\r )) !4 )¨))  47  ))!5 )¨)) 57 )à )È)B AAqñ @ )à(°E\r   )àõ 7@ )à(°AFAqE\r  )( \r  )àA 6° )à!6 6 6(Aj6  )7è	  ))7   (¤Aj6¤  )¨)!7BÐ !8A !9 BÀ | 9 8ü BÐ !: 7 BÀ | :ü\n   )à!; ; ;(Aj6  )à) Ó 7è )è!< Bð|$  <~~# BÀ }! $    78  70  )0) 7(  )0)7   )((6 )8) B|! (­B|§!  6   ì 7 )! )()! (­!@ P\r    ü\n   ) )! ) (­B| 7  )8) !	  )7  )7   	Ö !\n BÀ |$  \n~~~# BÀ }! $    78  70  )0) 7(  )0)7   )((6 )8) B|! (­B|§!  6   ì 7 )! )()! (­!@ P\r    ü\n   ) )§! ) (­B| 6  )8) !	  )7  )7   	Ö !\n BÀ |$  \n~~~# BÀ }! $    78  70  )0) 7(  )0)7   )((6 )8) B|! (­B|§!  6   ì 7 )! )()! (­!@ P\r    ü\n   ) )§! ) (­B| ;  )8) !	  )7  )7   	Ö !\n BÀ |$  \n~~~# BÀ }! $    78  70  )0) 7(  )0)7   )((6 )8) B|! (­B|§!  6   ì 7 )! )()! (­!@ P\r    ü\n   ) )§! ) (­| :   )8) !	  )7  )7   	Ö !\n BÀ |$  \n~~# BÐ }! $    7H  7@  )@) 78B !  70  7( )8! B(|!A !A !A!    Aq Aq   (46   )H) B| ( ì 7 )!	 )(!\n ( ­!@ P\r  	 \n ü\n   )(¦  )H) !  ) 7  )7 B| Õ !\r BÐ |$  \r~~# BÀ }! $    70  7(  )() 7 B !  7  7@@ ) ( AFAqE\r   ) (6  )0) B| (ì 7 )! ) )! (­!@ P\r    ü\n  @@ ) ( AFAqE\r  A6  )0) B|Aì 7 ) )§! ) :  @@ ) ( AFAqE\r A!  6  )0) B| ì 7 ) +ü!	 ) 	:  @ ) ( AFAqE\r A!\n  \n6  )0) B| \nì 7 ) - Aq! ) :  @@ (\r   )0) Ó 78 )0) !  )7  )7    Ö 78 )8!\r BÀ |$  \r§~~# B0}! $    7   7  )) 7@@ )( AFAqE\r  )B|!  )7  ) 7   µ  ) ) × 7(@ )( AFAqE\r   )- Aq­ ) ) × 7(@ )( AFAqE\r   )+ü ) ) × 7(@ )( A	FAqE\r @ )(­BZAqE\r   )))  ) ) × 7(@ )(­BZAqE\r   ))( ¬ ) ) × 7(@ )(­BZAqE\r  ))/ !A!   t u¬ ) ) × 7(@ )(­BZAqE\r  ))-  !A!   t u¬ ) ) × 7(  ) ) Ó 7( )(! B0|$  «~# B0}! $    7   7  )) 7@@ )( AFAqE\r   ))¹ ) ) Ø 7(@ )( AFAqE\r  )B|!  )7  ) 7   ·  ) ) Ø 7(@ )( A	FAqE\r @ )(­BZAqE\r   ))+  ) ) Ø 7(  ) ) Ó 7( )(! B0|$  k~~# B }! $    7  7  )) 7 ) ! )) ! Aq Ù ! B |$  Þ~~~~~~	~~~# B°}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )( AFAqE\r   )) ))| ) ) × 7¨@ )( AFAqE\r  )( AFAqE\r   )+ )+  ) ) Ø 7¨@ )( AFAqE\r  )( AFAqE\r   )( )(j6  ) ) B| (ì 7x )x! ))! )(­!@ P\r    ü\n   )x )(­|! ))! )(­!@ P\r    ü\n   )/"!	A !\n@ 	Aÿÿq \nAÿÿqGAqE\r  )B|!  )7  )x7   )7¨ ) ) !  )7  )x7  B| Õ 7¨@ )( AFAqE\r  )( AFAqE\r   ))7p )/"!\rA !@ \rAÿÿq AÿÿqGAq\r   ) ) B|Aì 7p ))) ) ) Ï ! )p 7  )p7h@ )hB R!A ! Aq! !@ E\r  )h)B R!@ AqE\r   )h)7h )/"!A !@@ Aÿÿq AÿÿqGAqE\r  ))) ))Ï ! )h 7 ))) ) ) Ï ! )h 7 )/"!A !@ Aÿÿq AÿÿqGAqE\r   )7¨  )p ) ) Ô 7¨@ )( AFAqE\r   ))7` )/"!A !@ Aÿÿq AÿÿqGAq\r   ) ) B|Aì 7` ))) ) ) Ï ! )` 7  )`7X@ )XB R!A ! Aq! ! @ E\r  )X)B R! @  AqE\r   )X)7X )/"!!A !"@@ !Aÿÿq "AÿÿqGAqE\r  ))B|Aì !# )X #7 ) ) B|Aì !$ )X $7@@ )/"AÿÿqE\r  )) ))RAqE\r  ) ))Ð !% )X) %7  )!& )X) &7  )X)B 7 )/"!\'A !(@ \'Aÿÿq (AÿÿqGAqE\r   )7¨  )` ) ) Ô 7¨@ )( AFAqE\r   ) ) B|Aì 7P ) ) B|Aì !) )P )7 )!* )P) *7  ))) ) ) Ï !+ )P) +7  )P ) ) Ô 7¨@ )( A	FAqE\r  )( A	FAqE\r   )( )(j6H  ) ) B| (Hì 7@ )@!, ))!- )(­!.@ .P\r  , - .ü\n   )@ )(­|!/ ))!0 )(­!1@ 1P\r  / 0 1ü\n   )/"!2A !3@ 2Aÿÿq 3AÿÿqGAqE\r  )B|!4 4 )H7 4 )@7   )7¨ ) ) !5  )H7   )@7  B| 5Ö 7¨@ )( AFAqE\r   )78 )/"!6A !7@ 6Aÿÿq 7AÿÿqGAq\r   )8 ) ) Ð 78 A 64@@ (4A\nIAqE\r  )) (4­B|) 7(@@ )(B RAqE\r ) )  )8) )()  )()   )()7(   (4Aj64   )87¨  ) ) Ó 7¨ )¨!8 B°|$  8á~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r   )) ))} ) ) × 7(@ )( AFAqE\r   )+ )+¡ ) ) Ø 7(  ) ) Ó 7( )(! B0|$  ~# Bð }! $    7`  7X  )X) 7P  )X)7H@@ )P( AFAqE\r   )P) )H)~ )`) × 7h@ )P( AFAqE\r   )P+ )H+¢ )`) Ø 7h@ )P( AFAqE\r B !  7@  78 A 64@@ (4 )H)§IAqE\r )PB|! B8|  )7  ) 7  B8| ¼   (4Aj64   )`) B| (Dì 7   (D6( B |B|A 6  ) ! )8! ((­!@ P\r    ü\n   )8¦  )`) !  )(7  ) 7  B| Õ 7h  )`) Ó 7h )h!	 Bð |$  	á~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r   )) )) ) ) × 7(@ )( AFAqE\r   )+ )+£ ) ) Ø 7(  ) ) Ó 7( )(! B0|$  o~# B }! $    7  7  )) 7  ))7  )) ) ) )) × ! B |$  }~~# B }! $    7  7  )) 7  ))7  ) ) Þ ! )) ! Aq Ù ! B |$  ~~# B }! $    7  7  )) 7  ))7  ) ) Þ As! )) ! Aq Ù ! B |$  Õ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))S! ) ) !  Aq Ù 7( )+ )+c! ) ) !  Aq Ù 7( )(! B0|$  ÿ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))W! ) ) !  Aq Ù 7(@ )( AFAqE\r  )+ )+e! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  ÿ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))U! ) ) !  Aq Ù 7(@ )( AFAqE\r  )+ )+d! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  ÿ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))Y! ) ) !  Aq Ù 7(@ )( AFAqE\r  )+ )+f! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )( AFAqE\r  )) ))B R! ) ) !  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqqA G! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )) ))B R! ) ) !  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqrA G! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  ~~~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r  )( AFAqE\r  )) ))B R! ) ) !  Aq Ù 7(@ )( AFAqE\r  )- Aq )- AqsA G! ) ) !  Aq Ù 7(  ) ) Ó 7( )(! B0|$  d~~# B}! $    7  7  ) )  As! )) ! Aq Ù ! B|$  Ø	~# Bð}! $    7à  7Ø )Ø) 5 !@@ B	V\r @@@@@@@@@@ §\n 	  BÐ 7È A6Ð BÈ|B|A 6  )à) !  )Ð7   )È7  B| Õ 7è\n Bµ 7¸ A6À B¸|B|A 6  )à) !  )À70  )¸7(  B(| Õ 7è	 BÂ 7¨ A6° B¨|B|A 6  )à) !  )°7@  )¨78  B8| Õ 7è B¢ 7 A6  B|B|A 6  )à) !  ) 7P  )7H  BÈ | Õ 7è Bþ 7 A6 B|B|A 6  )à) !  )7`  )7X  BØ | Õ 7è B¤ 7ø A6 Bø|B|A 6  )à) !	  )7p  )ø7h  Bè | 	Õ 7è B 7è A6ð Bè|B|A 6  )à) !\n  )ð7  )è7x  Bø | \nÕ 7è Bí 7Ø A6à BØ|B|A 6  )à) !  )à7  )Ø7  B| Õ 7è B 7È A6Ð BÈ|B|A 6  )à) !  )Ð7   )È7  B| Õ 7è BÔ 7¸ A6À B¸|B|A 6  )à) !\r  )À7°  )¸7¨  B¨| \rÕ 7èB )è¼ ! A6  B«  ½ B )è¼ !  )Ø) ( 6 B  B|½  )àA6° )àB7¸  )à) Ó 7è )è! Bð|$  a~~# B}! $    7  7  ) ) ( A F! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( AF! )) ! Aq Ù ! B|$  a~~# B}! $    7  7  ) ) ( A	F! )) ! Aq Ù ! B|$  Î	~~~# B°	}! $    7 	  7	  )	) 7	B !  7	  7	  )	))7ø@@@ )øB RAqE\r@ )ø) ( AGAqE\r B )è¼ ! AÜ6  B«  ½ B )è¼ B¢ B ½  ) 	A6° ) 	B7¸  ) 	) Ó 7¨	 )ø) B|!  )7  ) 7  B|â 7ð@ (	 (	MAqE\r @@ (	E\r @@ (	 (	MAqE\r  (	At6	   )	 (	­B§ 7	 A6	 B¤ 7	 )	 (	­B|B|! )	 (	­B|! (	 (	k­B!@ P\r    ü\n   )ð!	 )	 (	­B| 	7   (	Aj6	  )ø)7ø Bà!\nA ! B|  \nü   ) 	) B|Aèì 7 )! (	!\r )	! B | \r  B|ÿ Bè!  B | ü\n   A 6@@ ( (	IAqE\r )	 (­B|) ¦   (Aj6  )	¦   ) ) 	) Ü 7¨	 )¨	! B°	|$  ~~# B }! $ B !  7  7 B|   )7   ) 7  B| ¼  B|!A !A!   t uº  )! B |$  ù~~~# B}! $    7  7ø  )ø) 7ð  )ø)7è  )ø)7à  )ø)7Ø  )ø) 7Ð  )è)7À A6È BÀ|B|A 6  B 7° A6¸ B°|B|A 6   )È7  )À7  )¸7  )°7@@@ B| B|² Aq\r  B 7  A6¨ B |B|A 6   )È7x  )À7p  )¨7h  ) 7` Bð | Bà |² AqE\r  )è7  )ð)(6 B 7B !  7  7B !  7ø  7ðB !  7è  7à )àB|! BÐ|  )7X  ) 7P BÐ| BÐ | @ (ü (øMAqE\r @@ (üE\r @@ (ü (øMAqE\r  (üAt6ü   )ð (ü­B§ 7ð A6ü B¤ 7ð )ð (ø­B|B|! )ð (ø­B|! (ø (øk­B!	@ 	P\r    	ü\n   )ð (ø­B|!\n \n )Ø7 \n )Ð7   (øAj6ø )Ø! )!  Bð| Bá  ä 7È@ )ÈB RAqE\r   )È7 )è!\r )à!B!  |! )ð)!  \r)7H  \r)7@B !A! B¸| BÀ |   B| Bð| Bà| B|     )ð)!  )à|!A ! B¸|     B|      B¨| B¸| B|à   7   7  B||!  B¨|  B|ã 7 )) !B!   | ( ì 7 )! )! 5 !@ P\r    ü\n   )¦   )7  ))  |AÐ ì 7 )) !  ) 78  )70  B0| Õ 7 B 7è A6ð Bô| 6  )) !  )ð7(  )è7   B | Õ 7ø ))  ) )ø ) @@ )Ð- AqE\r  )ð)( (KAqE\r   )ð))  (­B8~|7Ð  )ð)( (k6Ø  )ð)( (k6ÜB !  7È  7À BÀ|B|!   BÐ|   B|é 7À  )) B| (Èì 7¸ )¸!! )À!" (È­!#@ #P\r  ! " #ü\n   )À¦   )¸7À )) !$  )È7  )À7  B| $Õ 7à  )) Ó 7à B© 7  A6¨ B |B|A 6  )) !%  )¨7  ) 7    %Õ 7° ))  ) )° )à @ )B RAqE\r  )¦ @ )ðB RAqE\r  )ð¦   ) )) Ú 7 )!& B|$  &õ\r~# BÐ }! $    7@  78  70  7(  )8))7 @@@ ) B RAqE\r@ ) ) ( AGAqE\r B )è¼ ! A÷6  B«  ½ B )è¼ !  )07 Bå  B|½  )(A6° )(B7¸  )() Ó 7H@ )@( )@(MAqE\r @@ )@(E\r @@ )@( )@(MAqE\r )@!  (At6  )@)  )@(­B§ ! )@ 7  )@A6B¤ !	 )@ 	7  )@)  )@(­B|B|!\n )@)  )@(­B|! )@( )@(k­B!@ P\r  \n  ü\n   )@)  )@(­B|!\r ) ) B|! \r )7 \r ) 7  )@!  (Aj6  ) )7   B 7H )H! BÐ |$  í~~# BÐ }! $    7H  7@  )@) 78  )@)70 B 7( )0)! )0(! )8))@BÀ|! B|   B(| Ç   )8))@ B|AAqü 7 A 6@@ ( ( IAqE\r ) (­B|)¦   (Aj6  )¦ @ )8))@(°E\r  )8))@A 6°@ )8))@)¸B RAqE\r  )8))@B 7¸ ) )H) Ð ! BÐ |$  ~~~~# BÀ }! $    78  70  )0) 7(  )0)7  B 7 ) )! ) (! )()B|! B|    B|Ì @ )()( )()( (jIAqE\r  )()( (j! )() 6 )())  )()(­B8~§ ! )() 7  )())  )()(­B8~|! )!	 (­B8~!\n@ \nP\r   	 \nü\n   (! )()!   (j6 )¦  )8) Ó !\r BÀ |$  \rÿ~~~~# BÐ}! $    7À  7¸  )¸) 7°  )¸)7¨  )¸)7   )¸)7 B 7B !  7  7B !  7x  7p ) B|! Bà |  )7   ) 7 Bà | B| @ (| (xMAqE\r @@ (|E\r @@ (| (xMAqE\r  (|At6|   )p (|­B§ 7p A6| B¤ 7p )p (x­B|B|! )p (x­B|! (x (xk­B!@ P\r    ü\n   )p (x­B|!	 	 )h7 	 )`7   (xAj6x )!\n )À!  Bð | \nBÃ  ä 7X@@ )XB RAqE\r   )X7È )¨B|! ) B|!\r )°)! )°)B0|! BÈ |  )7  ) 7B !A ! BÈ | B| \r  B| Bð |  B|     )°)! ) B|! BÈ |!B !A ! B|!A ! Aq!A!  t u!A!          t u Aq  B8| BÈ | B|à  )°))@BÀ|! ) B|!  )7  ) 7   )°))@ B8|AAqü 70 A 6,@@ (, (@IAqE\r )8 (,­B|)¦   (,Aj6,  )8¦ @ )B RAqE\r  )¦ @ )pB RAqE\r  )p¦ @ )°))@(°E\r  )°))@A 6°@ )°))@)¸B RAqE\r  )°))@B 7¸  )0 )À) Ð 7È )È! BÐ|$  :~# B }!   7  7  )) 7 )A;" )L~# B}! $    7  7  ) )  )) Ð ! B|$  q~# B }! $    7  7  )) 7 ))! ) 7¸ )A6° )) Ó ! B |$  í~# B }! $    7  7  )) 7 @@ ) ( AFAqE\r  ) )B SAqE\r  ) )! B  } )) × 7@ ) ( AFAqE\r  ) +B ¹cAqE\r   ) + )) Ø 7  )) Ó 7 )! B |$  ­~|~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r @@ )) ))WAqE\r  ))! ))!   ) ) × 7(@ )( AFAqE\r @@ )+ )+eAqE\r  )+! )+!   ) ) Ø 7(  ) ) Ó 7( )(! B0|$  ®~|~# B0}! $    7   7  )) 7  ))7@@ )( AFAqE\r @@ )) ))YAqE\r  ))! ))!  ¹ ) ) Ø 7(@ )( AFAqE\r @@ )+ )+fAqE\r  )+! )+!   ) ) Ø 7(  ) ) Ó 7( )(! B0|$  è~# BÐ }! $    7@  78  )8) 70  )8)7(@@ )0( AFAqE\r  B7  A 6@@ ( )()§IAqE\r  )0) ) ~7   (Aj6   )  )@) × 7H@ )0( AFAqE\r  D      ð?9 A 6@@ ( )()§IAqE\r  )0+ +¢9  (Aj6   + )@) Ø 7H  )@) Ó 7H )H! BÐ |$  Z~# B }! $    7  7  )) 7 )+ )) Ø ! B |$  _~# B }! $    7  7  )) 7 )+ø  )) Ø ! B |$  ó~# BÐ }! $    7H  7@  )@) 78  )@)70  )@)7(  )8( )((j6   )H) B| ( ì 7 )! )8)! )0)!@ P\r    ü\n   ) )0)|! )()! )((­!@ P\r    ü\n   ) )0)| )((­|!	 )8) )0)|!\n )8(­ )0)}!@ P\r  	 \n ü\n   )H) !  ) 7  )7 B| Õ !\r BÐ |$  \ré\n~# Bà }! $    7P  7H  )H) 7@  )H)78  )H)70@@ )8) )0)| )@(­UAqE\r B )è¼ ! A 6  Bº  ½ B )è¼ BÀ B ½  )PA6° )PB7¸  )P) Ó 7X  )@(­ )0)}§6(  )P) B| ((ì 7  ) ! )@)! )8)!@ P\r    ü\n   )  )8)|! )@) )8)|! )@(­ )8)} )0)}!	@ 	P\r    	ü\n   )P) !\n  )(7  ) 7  B| \nÕ 7X )X! Bà |$  ¿~# BÐ }! $    7H  7@  )@) 78  )@)70  )@)7(  )8(6$@ ($­ )0) )((­|SAqE\r   )0) )((­|§6$  ($6  )H) B| (ì 7 )! )8)! )0)!@ P\r    ü\n   ) )0)|! )()! )((­!@ P\r    ü\n   ) )0)| )((­|!	 )8) )0)| )((­|!\n )8(­ )0)} )((­}!@ P\r  	 \n ü\n   )H) !  )7  )7   Õ !\r BÐ |$  \rë~	\r~# B}! $    7x  7p  )p) 7h  )p)7`  )x) B|Aì 7X  )X7P A 6L A 6H@@ (H )h(IAqE\r A6D A 6@@ (@ (Hj )h(I!A ! Aq! !@ E\r  (@ )`(I!@ AqE\r  )h) (@ (Hj­|-  !A!  t u!	 )`) (@­|-  !\nA!@ 	 \n t uGAqE\r  A 6D  (@Aj6@@ (@ )`(GAqE\r  A 6D@ (DE\r  )x) B|Aì ! )P 7 )P)B 7  (H (Lk68  )x) B| (8ì 70 )0!\r )h) (L­|! (8­!@ P\r  \r  ü\n   )x) !  )87  )07   Õ ! )P) 7   (H )`(j6L  )P)7P  (HAj6H @ (HA KAqE\r  )x) B|Aì ! )P 7 )P)B 7  (H (Lk6(  )x) B| ((ì 7  ) ! )h) (L­|! ((­!@ P\r    ü\n   )x) !  )(7  ) 7 B| Õ ! )P) 7  )X )x) Ô ! B|$  ¤~~	~# B}! $    7  7x  )x) 7p  )x)7hB !  7`  7X A : W  )p))7H@@@ )HB RAqE\r@ )H )p))RAqE\r  )h! BØ |!A !A !    Aq Aq @@ )H) ( A	FAqE\r  A: W@ )H) ( AGAqE\r B )è¼ ! A6  Bº  ½ B )è¼ Bã B ½  )A6° )B7¸  )) Ó 7 )H) !	 BØ |!\nA !A ! \n 	  Aq Aq   )H)7H @ - WAqE\r   )) B| (dì 78  (d6@ B8|B|A 6  )8!\r )X! (d­!@ P\r  \r  ü\n   )X¦  )) !  )@7  )87  B| Ö 7  )) B| (dì 7(  (d60 B(|B|A 6  )(! )X! (d­!@ P\r    ü\n   )X¦  )) !  )07   )(7  B| Õ 7 )! B|$  ë~~# BÀ }! $    78  70  )0) 7( A: \' A 6@@ )(B|! (!  )7  ) 7 B|  B |¦ !  6 E\r@ (Ñ \r  A : \'  (  (j6  - \'! )8) ! Aq Ù ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7( A: \' A 6@@ )(B|! (!  )7  ) 7 B|  B |¦ !  6 E\r@@@A AqE\r  (Ò \r (A0kA\nIAq\r A : \'  (  (j6  - \'! )8) ! Aq Ù ! BÀ |$  ë~~# BÀ }! $    78  70  )0) 7( A: \' A 6@@ )(B|! (!  )7  ) 7 B|  B |¦ !  6 E\r@ (Ð \r  A : \'  (  (j6  - \'! )8) ! Aq Ù ! BÀ |$  ë~~# BÀ }! $    78  70  )0) 7( A: \' A 6@@ )(B|! (!  )7  ) 7 B|  B |¦ !  6 E\r@ (Ó \r  A : \'  (  (j6  - \'! )8) ! Aq Ù ! BÀ |$  Í~~~~# BÀ }! $    78  70  )0) 7(A !B  6äÝ   )())7 @@ ) B RAqE\r ) ) !BØÝ !A !A !    Aq Aq   ) )7  BØÝ !A !	A!\n  	 \nt \nuº  BÏ 7 )! Að :  A :  B|! B )ØÝ 7      )8) Ó !\r BÀ |$  \r÷~~# B}! $    7  7ø  )ø) 7ð )ðB|!  )7(  ) 7   B |ü 7è@@ )èA ª E\r  )è¦   )) Ó 7  )) B|AÐ ì 7à  )èã 7Ø  )) Ñ 7Ð )ÐA6  )ØB R!A! Aq! !@ \r © ( A6G! ! )Ð Aq:  B 7¸ A6À B¸|B|A 6  )) !	  )À7  )¸7  B| 	Õ 7È ))  )à )È )Ð @ )ØB RAqE\r  )Ø® @ )è BÐ |þ A HAqE\r  )è¦   )) Ó 7  )) Ñ 7H )HA6  )p!\n )H \n7 B 70 A68 B0|B|A 6  )) !  )87  )07    Õ 7@ ))  )à )@ )H  )è¦   )à )) Ú 7 )! B|$  y~# B}! $    (Aj­¤ 7 )!  ) !  (­!@ P\r    ü\n   )  (­|A :   )! B|$  ~# Bà }! $    7P  7H  )H) 7@ )@B|!  )7   ) 7  B|ü 78 )8! )P) B|! B(|    )8¦ @@ (0AFAqE\r   )P) Ó 7X )P) !  )07  )(7  B| Õ 7X )X! Bà |$  Â~# BÀ }! $    70  7(  )0 )(ý 7 @@ ) ( \r   ) 78  ) )7  ) (6 B|B|A 6  )0) !  )7  )7    Ö 78 )8! BÀ |$  ~~# B}! $    7x  7p  )p) 7h  )p)7`B !  7X  7P@@ )`( AFAqE\r   )`)7@  )`(6H BÀ |B|A 6   )H7X  )@7P@ )`( A	FAqE\r   )`)70  )`(68 B0|B|A 6   )87X  )07P )h!  )7  )7  B|ü 7( )(!  )X7  )P7     : \' )(¦  - \'! )x) ! Aq Ù ! B|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ü 7 )÷  )¦  )() Ó ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7   ü 7 )BÊ AÀ A	Ý  )() Ó ! B0|$  P~# B }! $    7  7  6  7  )÷ ! B |$  û\n~# B}! $    7p  7h  )h) 7`  )p) B|Aì 7X  )X7P )`B|!  )7   ) 7  B|ü 7H  )Hã 7@@@@ )@B RAqE\r @@ )@ò !  78 B RAqE\r  )8B| §60  )p) B| (0ì 7( )(! )8B|! (0­!@ P\r    ü\n   )p) B|Aì ! )P 7  )P)7P )p) !	  )07  )(7 B| 	Õ !\n )P \n7   )@®  )H¦   )p) Ó 7x )H¦   )X )p) Ô 7x )x! B|$  §~~# BÀ }! $    70  7(  )() 7  AAA ¿ 6@@ (A HAqE\r   )0) Ó 78 (! A6  A ³  A6 (AA B|A¾  (AA B|A¾ B !  7  7 A; A 6  ) )§AÿÿqÍ ;\n@ ( B|A¶ A HAqE\r  (­   )0) Ó 78@ (A¹ A HAqE\r  (­   )0) Ó 78  (¬ )0) × 78 )8! BÀ |$  Ã~~# B}! $    7  7x  )x) 7p  )x)7h  )p(Aj­¤ 7` )`! )p)! )p(­!@ P\r    ü\n   )` )p(­|A :  B !  7X  7P )h)! BÐ | ½  BÐ |!A !	A!\n  	 \nt \nuº   )P7HB !  7@  78  70  7(  7   7 A6 A6 @@ )` )H B| B| A HAqE\r  )`¦  )H¦   )) Ó 7  )( )( )(¿ 6@ (A HAqE\r  )`¦  )H¦   )) Ó 7 A6 (AA B|A¾   ( )) )(· 6@ (A HAqE\r  )`¦  )H¦  )¸   )) Ó 7 )`¦  )H¦  )¸   (¬ )) × 7 )! B|$  ~# BÐ }! $    7@  78  )8) 70  )8)7( A; A 6  )()§AÿÿqÍ ; A6  )0)§ B| B|µ 6@@ (A HAqE\r   )@) Ó 7H A6 (AA B|A¾   (¬ )@) × 7H )H! BÐ |$  a~# B }! $    7  7  )) 7 ))§­  )) Ó ! B |$  Ñ~# B }! $    7  7  )) 7  ))7 @@ ) ( AFAqE\r  ))§ ) ) ) (­A ¼ @ ) ( A	FAqE\r  ))§ ) ) ) (­A ¼  )) Ó ! B |$  Á~~# BÐ }! $    7@  78  )8) 70  )8)7(  )@) B| )()§ì 7 A 6  B|B|A 6   )0)§6 A; B|BA\nä  /!A!@  t uE\r   )0)§ ) )()A º §6 @@ ( \r   )@) Ó 7H )@) !  ) 7  )7    Ö 7H )H! BÐ |$  ~~# Bà }! $    7P  7H  )H) 7@ AÀ 6<  )P) B| (<ì 7( A 60 B(|B|A 6  B 7   )@)§6  A;$ A 6@@@ B |BA\nä  /&!A!@  t u\r   )@)§ )( (0­| (< (0k­A º §6@ (\r @ (A HAqE\r   )P) Ó 7X  ( (0j60@ (0 (<OAqE\r   )(7  (<AÀ j6<  )P) B| (<ì 7( )(! )! (0­!@ P\r    ü\n   @ (0\r   )P) Ó 7X )P) !  )07  )(7    Ö 7X )X!	 Bà |$  	´~# BÀ }! $    78  70  )8) B|AÀ ì 7( )(BÀ Ì   )(7  )( §6  B|B|A 6  )8) !  ) 7  )7 B| Õ ! BÀ |$  Á~# B }! $    7  7  )) 7  )(Aj­¤ 7  ) ! ))! )(­!@ P\r    ü\n   )  )(­|A :   ) «  ) ¦  )) Ó ! B |$  §~# BÐ }! $    7H  7@  )@) 78  )8(Aj­¤ 70 )0! )8)! )8(­!@ P\r    ü\n   )0 )8(­|A :    )H) B|AÀ ì 7( )0 )(õ  )0¦   )(7  )( §6  B|B|A 6  )H) !  ) 7  )7 B| Õ ! BÐ |$  â~~# B}! $    7  7  Bø |7 A¨!A   Ï   )) B|AÐ ì 7p  )) Ñ 7h )hA6  /xAÿÿq­! )h 7 B 7P A6X BÐ |B|A 6  )) !  )X7  )P7  B| Õ 7` ))  )p )` )h   )) Ñ 7H )HA6  /zAÿÿq­! )H 7 B¹ 70 A68 B0|B|A 6  )) !  )87(  )07   B | Õ 7@ ))  )p )@ )H  )p )) Ú ! B|$  Ô~~~# BÐ }! $    7H  7@@B - éÝ Aq\r A BìÝ  A!B  : éÝ B !  (¤Þ 68  )Þ 70  )Þ 7(  )Þ 7   )Þ 7  )üÝ 7  )ôÝ 7  )ìÝ 7  Au6A !    A!B  : èÝ  )H) Ó ! BÐ |$  ~~# B}! $    7  7 @B - éÝ AqE\r A !  BìÝ  A !B  : èÝ  )) Ó ! B|$  Ð~# BÐ }! $    7H  7@  )@) 78 )8B|!  )7  ) 7    70 B³Ï 7( )(! Að : & A : \' B&|!  )07   B|  )0¦  )H) Ó ! BÐ |$  y~# B}! $    (Aj­¤ 7 )!  ) !  (­!@ P\r    ü\n   )  (­|A :   )! B|$  ¬~# Bð }! $    7h  7`  )`) 7X  )`)7P )XB|!  )7  ) 7    7H )PB|!  )7  ) 7  B| 7@ BÐÏ 78 )8! Að : 5 Að : 6 A : 7 B5|! )H!  )@7(  7    B |  )H¦  )@¦  )h) Ó ! Bð |$  ¬~# Bð }! $    7h  7`  )`) 7X  )`)7P )XB|!  )7  ) 7    7H )PB|!  )7  ) 7  B| 7@ B´Ð 78 )8! Að : 5 Að : 6 A : 7 B5|! )H!  )@7(  7    B |  )H¦  )@¦  )h) Ó ! Bð |$  ó	~# B}! $    7x  7p  )p) 7h )hB|!  )7  ) 7    7` BÑ 7P )P! Að : N A : O BÎ |!  )`7    B| 7X  )X §6H  )x) B| (Hì 7@ )@! )X! (H­!@ P\r    ü\n   )`¦  )X¦   )@70  (H68 B0|B|A 6  )x) !	  )87(  )07  B | 	Õ !\n B|$  \nó	~# B}! $    7x  7p  )p) 7h )hB|!  )7  ) 7    7` BÒ 7P )P! Að : N A : O BÎ |!  )`7    B| 7X  )X §6H  )x) B| (Hì 7@ )@! )X! (H­!@ P\r    ü\n   )`¦  )X¦   )@70  (H68 B0|B|A 6  )x) !	  )87(  )07  B | 	Õ !\n B|$  \nõ\n~\n~~\n~\n~# Bð}! $    6ì  7à  7Ø  )Ø7Ð  )Ð) ) B|AÐ ì 7È )à!B !   | >Ä   )à|7°  (Ä6¸B!  B°||!A !  6  )à!	BÀ !\n  	 \n| >¬  \n )à|7  (¬6   B|| 6  BÒ 7 A6  B|| 6  )Ð) ) !  )7  )7  B| Õ 7 )Ð) ) ! )È!\r )!  )¸7  )°7x  \r  Bø | Õ   BÁ 7ð A6ø  Bð|| 6  )Ð) ) !  )ø7p  )ð7h  Bè | Õ 7 )Ð) ) ! )È! )!  ) 7`  )7X    BØ | Õ   BÄ 7àA!  6è  Bà|| 6  )Ð) ) !  )è7P  )à7H  BÈ | Õ 7 )Ð) ) ! )È! )! )à- !A!     q Ù   Bº 7Ð A	6Ø  BÐ|| 6  )Ð) ) !  )Ø7@  )Ð78  B8| Õ 7 )Ð) ) !  )È )  )à- \rq Ù   B² 7À A6È  BÀ|| 6  )Ð) ) !  )È70  )À7(  B(| Õ 7 )Ð) ) !  )È )  )à- q Ù   BÍ 7°  6¸  B°|| 6  )Ð) ) !  )¸7   )°7  B| Õ 7 )Ð) ) !  )È )  )à- q Ù   B 7  A6¨  B || 6  )Ð) ) !   )¨7  ) 7  B|  Õ 7 )Ð) ) !! )È!" )!# )à- !$ )Ð) ) !% ! " # $Aq %Ù    )È )Ð) ) Ú 7@ )Ð) ( )Ð) (MAqE\r @@ )Ð) (E\r @@ )Ð) ( )Ð) (MAqE\r )Ð) !& & &(At6  )Ð) ) )Ð) (­B§ !\' )Ð)  \'7 )Ð) A6B¤ !( )Ð)  (7 )Ð) ) )Ð) (­B|B|!) )Ð) ) )Ð) (­B|!* )Ð) ( )Ð) (k­B!+@ +P\r  ) * +ü\n   )!, )Ð) ) )Ð) (­B| ,7  )Ð) !- - -(Aj6 )Ð)  )Ð)B A Aqñ  )Ð) !. . .(Aj6AAq!/ Bð|$  /\r~# B }! $    6  7  7  )7  )) ) B|AÐ ì 7x Bÿ 7` A6h Bà |B|A 6  )) ) !  )h7  )`7  B| Õ 7p )) )  )x )p )(¬ )) ) ×   BÔ 7P A6X BÐ |B|A 6  )) ) !  )X7   )P7  B| Õ 7p )) )  )x )p )(¬ )) ) ×   B° 7@ A6H BÀ |B|A 6  )) ) !  )H70  )@7(  B(| Õ 7p )) )  )x )p )/Aÿÿq­ )) ) ×    )x )) ) Ú 78@ )) ( )) (MAqE\r @@ )) (E\r @@ )) ( )) (MAqE\r )) !  (At6  )) ) )) (­B§ ! ))  7 )) A6B¤ !	 ))  	7 )) ) )) (­B|B|!\n )) ) )) (­B|! )) ( )) (k­B!@ P\r  \n  ü\n   )8!\r )) ) )) (­B| \r7  )) !  (Aj6 ))  ))B A Aqñ  )) !  (Aj6AAq! B |$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7    7 )  )¦  )() Ó ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7    7 )  )¦  )() Ó ! B0|$  ~# B0}! $    7(  7   ) ) 7 )B|!  )7  ) 7    7 )  )¦  )() Ó ! B0|$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bß !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bß !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bß !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  ~~# BÀ }! $    78  70  )0) 7(  )0)7  )(B|!  )7  ) 7    7  )8)B|Aì 7 )8! ) 7  ) )! ) 7 )! )!A !Bà !	B!\n   Aq 	 \n  )¦  )8) Ó ! BÀ |$  Í~# B }!  6  7@@  ( (MAqE\r  A 6   )  (­|-  :   - Aÿq6@@ (Aq\r  )A6 @@ (AàqAÀFAqE\r  )A6   (Aq6@@ (AðqAàFAqE\r  )A6   (Aq6@@ (AøqAðFAqE\r  )A6   (Aq6 )A6  A 6@  ( (k )( IAqE\r  A 6 A6@@ ( )( IAqE\r   )  ( (j­|-  : @ - AÿqAÀqAGAqE\r  (! ) 6  A 6  (At6  - AÿqA?q (r6  (Aj6 @ )( AFAqE\r  (AIAqE\r  A 6@ )( AFAqE\r  (AIAqE\r  A 6@ )( AFAqE\r  (AIAqE\r  A 6@ (A°OAqE\r  (Aÿ¿MAqE\r  A 6@ (AÿÿÃ KAqE\r  A 6  (6 (º~~~~# BÐ }! $   7H  7@  78  70   )@) 7   A 6  B|A 6  B7( )0A 6  A 6$@@ ($ )H(IAqE\rA !  6   6 )H)  5$B|! )@!  )7  ) 7   B| B | B|¨ : @ - AqE\r  ( )0( KAqE\r    ( 6 (!	 )0 	6   ($­7(  ($Aj6$ @ )(BRAqE\r   (!\n )@!  )  \n­|7   (! )@!\r \r \r( k6@ )8B RAqE\r  )(! )8 7  BÐ |$ ï~# Bà }! $    7P  7H  7@ A6< A 68 A 64@@ (8!  )7  ) 7  B|  B0|¦ 6, A : + A 6$@@ ($ )P(IAqE\r  )P)  ($­B|7@@ )(  (<GAqE\r @ )(AGAqE\r @ (,E\r  (, )(IAq\r  (, )(KAqE\r@ )(AGAqE\r   (0 (8j68  (4Aj64 A: +  )(6<@ (<\r  (8! )H 6  (4! )@ 6  AAq: _  ($Aj6$ @@@ - +AqE\r  (,\r A Aq: _ - _Aq! Bà |$  \r B¬Þ  A   A  ¬ §    ¬ §     A   ¬  "   AF   (­ !  ¦   A  Ï~@@  (A N\r A!  ¯ E!  ´ !    )  !@ \r   ° @  -  Aq\r   ± ß !  )p!@  )h"P\r   7p@ P\r   7h@ )   R\r   7 à   )¨¦   ¦   rù~# B}"$ B !@@@ Aj  A	F\r  B|Bx"B|7x ) !@@ AK\r @@A tAàq\r  AF\r A	G\r  Bø |70@  A B0| "AdG\r   7   A	 B | !@ E\r  ¬ §!A  (|"k  (xAF!  7p    Bð | ¬ §!  7  A B| ¬ §!@ AF\r   B  AF7      ¬ §!  7`@@  A Bà | "AdF\r  ¬! B 7P@  A BÐ | "AdF\r Bd! A H\r    7@  A  BÀ | ¬!  §! B|$  ¯~@  B R\r @@B )Ï PE\r A !B )Ï ´ !@B )Í P\r B )Í ´  r!@ß ) " P\r @@@  (A N\r A!  ¯ E!@  )(  )8Q\r   ´  r!@ \r   °   )p" B R\r à  @@  (A N\r A!  ¯ E!@@@  )(  )8Q\r   B B   )H    )(B R\r A! E\r@  )"  )"Q\r     }A  )P    B 78  B 7   B 7(  B 7  B 7A ! \r  °  A!@  A+ B R\r   -  Aò G!  Ar  Aø  P" A r  Aå  P" AÀ r  -  "Aò F"Ar  A÷ F"Ar  Aá Fð~@ P\r    :     |"B| :   BT\r    :    :  B}| :   B~| :   BT\r    :  B|| :   B	T\r   B   }B"|" AÿqAl"6    }B|"|"B|| 6  B	T\r   6  6 Bx| 6  Bt| 6  BT\r   6  6  6  6 Bp| 6  Bl| 6  Bh| 6  Bd| 6   BB"}"B T\r  ­B~!  |!@  7  7  7  7  B |! B`|"BV\r      (x  Ô ~~# B0}"$    )8"7  )(!  7(  7    }"7  |! B|!A!@@@@@  (x B|B B|  E\r  !@  )"Q\r@ BU\r  ! BB   )"V"	|" )   B  	}"|7  BB 	|" )  }7   }! !  (x   	k"¬ B|  E\r  BR\r    )X"78   7(     )`|7  !B !  B 78  B 7   B 7(    ( A r6  AF\r   )}! B0|$  ú~# B0}"$   7B !    )`"B R­}7  )X!  7(  7 A !@@@  (x B|B B|  \r  )"B U\rAA  P!    (  r6  !  )"X\r     )X"7     }|7@  )`P\r    B|7  |B| -  :   ! B0|$     (x¬   §~# B }"$ B !@@B£  ,   B R\r © A6 Bð	¤ "P\r  A Bè¶ @ A+ B R\r  AA -  Aò F6 @@ -  Aá F\r  ( !@  AB  "Aq\r   Ar¬7  A B|   ( Ar"6  A6 B7`   6x  Bð|7X@ Aq\r   B|7   A¨  \r  A\n6 Bó 7P Bô 7H Bõ 7@ Bö 7@B - ±Þ \r  A6 á ! B |$  ©~# B}"$ B !@@B£  ,   B R\r © A6  µ ! B¶7 A   Ar  ¬ §"A H\r   » "B R\r   B ! B|$  9~# B}"$   7     ! B|$  $~   !AA    B  Ë R  §@    ü\n    ~@ BT\r     ¿    |!@@   BB R\r @@  BPE\r   !@ PE\r   !  !@  -  :   B|! B|"BP\r  T\r  B|!@ BÀ T\r   B@|"V\r @  ( 6   (6  (6  (6  (6  (6  (6  (6  ( 6   ($6$  ((6(  (,6,  (060  (464  (868  (<6< BÀ |! BÀ |" X\r   Z\r@  ( 6  B|! B|" T\r @ BZ\r   !@ BZ\r   ! B||!  !@  -  :    - :   - :   - :  B|! B|" X\r @  Z\r @  -  :   B|! B|" R\r   ~    ("Aj r6@  )(  )8Q\r   B B   )H    B 78  B 7   B 7(@  ( "AqE\r    A r6 A    )X  )`|"7   7 AtAu~~@@ (A N\r A! ¯ E!  ~!  ("Aj r6@@ )" )"R\r  !     }"   T"À   ) |7  }!   |! @ P\r @@@ Á \r      )@  "B R\r@ \r  °   }    |!   }"B R\r B   P! @ \r  °   ¾~@@ AI\r © A6 @ AG\r   )"P\r   }  )|!@  )(  )8Q\r   B B   )H    )(P\r  B 78  B 7   B 7(      )P  B S\r   B 7  B 7    ( Aoq6 A AI@  (AJ\r     Ã   ¯ !    Ã !@ E\r   °       Ä ~~  )P!A!@  -  AqE\r AA  )(  )8Q!@  B     "B S\r @@  )"B Q\r B!  )8"P\rB(!  }   |) |! C~@  (AJ\r   Æ   ¯ !  Æ !@ E\r   °  \n   Ç g~    ("Aj r6@  ( "AqE\r    A r6 A  B 7  B 7    )X"78   7(     )`|7 A ê~B !@@ ) "B R\r  É \r ) !@   )("}X\r      )H  @@ (A H\r  P\r  !@@   |"B|-  A\nF\r B|"P\r      )H  " T\r  }! )(!  !B !   À   )( |7(  |! k~  ~!@@ (AJ\r     Ê !  ¯ !    Ê !  E\r  ° @   R\r B   P   ½~~# "!B ! B B  P"}"$  !@@ \r B ! !  ! B R\r © A6 B !   ¬ " B S\r @@  P\r  -  A/F\r© A,6 @  Q\r  !  ! $  \n   Î    At  AvrAÿÿqT~# B}"$   B|Bx"B|7  ) 7      ¬ ! B|$  §$A!@  APjA\nI\r   Ñ A G! J @  AÿÿK\r   ­B-  At  AvAqr­-    AqvAq  AþÿI\n   APjA\nI" @  \r A BÐº    B RK~# B}"$     Aÿq B|  ! )! B|$ B   A  A   ²@@@@  A H\r  A G\r  -  \r    ! @@  AF\r  -  !@ \r  AÿqA/F\r AG\r AÿqA/G\r AF\r \r   !       !    !   ¬ § A   AÙ . @  AJ\r Bx §  Bø  A Ù »~# Bð }"$ @@   B|Û A N\r B !B !@  AB ³ AqE\r © A6 @ (AàqAF\r © A66 BBª "P\r  A6   A ³    6 ! Bð |$  ~# B }"$ @@ AN\r A !@   "B T\r © A%6 A!    B|À A B |Ö     B Þ ! ( B Ö  B |$  é~~~~# B}"$ @@   "P\r    B|"|-  A/F\r ! B 7 A 6(@@@@@@@@@@@@ Aq"E\r    B(|Ú E\r© !	   B(|þ AJ\r © "	( "\nA,G\r   B(|Ú \rA!A !\nA !\n@ (,Aàq"AÀF\r @ AF\r A!AA Aq!A!\nAA ! ((!@ AqE\r  P\r   (G\r  6  7  )7  6  )7  7 B R\r 	( !\n \nAG\r  ((6  )7  7A!A !\n P\r (!  §Aj6$  Aj"6   6  (6 A ! A 6  A 6  §Aj6$@ P\r  !	@@   	|-  A/F\r @@@   	|B|-  A/G\r  	!B ! 	B|"	B R\r  §! 	B|"	PE\r   6 @ \nE\r   A B â !A © ( "\rAF  A H! \r  ­ @ Aq"\r    B(|     "\n\r@ P\r  )!	 ((!\n@@ ( \nG\r  ) 	Q\r ) "B R\r @ E\r  AqAG\r A!\n@ AJ\r ©  \r6  Ü "	P\r@ 	ò "P\r  Aj!B  }!   |"B|!@@@ - A.G\r  - "\nE\r \nA.G\r  - E\r@ B|"  T\r © A%6  	®  A/:           B|Þ "\nE\r  	®  	ò "PE\r  	®    |A :   E\r    B(|     "\n\rA !\n ­ A!\n B|$  \n Bß × B ß  Bß Ø 4~  ß ") "7p@ P\r    7h   7 à   z~# B}"$ @@ AÀ q\r B ! AqAG\r  B|7 5 !  7 A   Ar  ¬ ! B|$  §P~B !@  A$B â "A H\r @BBª "B R\r   B   6 !     §  ¬ §B~# B}"$   7BÍ     ! B|$   A* Bá   AN æ \r B¨ß B BèÞ 7Ðà è ! B B B }7à B B 7à B   6Øß B B 5¤Ë 7à »~~# B}"$   : @@  ) "B R\r @  É E\r A!  ) !@  )(" Q\r   ( Aÿq"F\r    B|7(  :  @   B|B  )H  BQ\r A! - ! B|$      í ~@@ ("A H\r  E\r Aÿÿÿÿqé (0G\r@  Aÿq" (F\r  )(" ) Q\r   B|7(   :     ë    î ~~@ B|"ï E\r  ¯ @@  Aÿq" (F\r  )(" ) Q\r   B|7(   :    ë !@ ð AqE\r  ñ      ( "Aÿÿÿÿ 6    ( !  A 6  \r   AÕ ~@@  ("  (H\r A !@  (  B|B¡ "A J\r B !  ATF\r E\r© A  k6 B    6      ¬|"B(|/ j6   B |) 7  B|!   ~@   Q\r @    |"}B  B}V\r     À    B!@@@   Z\r @ B Q\r   !@  BB R\r   !  !@ P\r  -  :   B|! B|! B|"BP\r @ B R\r @ BB Q\r @ P\r   B|"|"  |-  :   BPE\r  BX\r @   Bx|"|  |) 7  BV\r  P\r@   B|"|  |-  :   B R\r  BX\r @  ) 7  B|! B|! Bx|"BV\r  P\r @  -  :   B|! B|! B|"PE\r   b~# B}"$ A   B|  P" B BV¢ "Au q   B|Q¬ ! B|$  »	~~~# BÀ }"$ B !@@  B R\r © A6 @@  B  "B R\r © A,6 @ BÿV\r  B |B  }"|   B|À B ! B !B !A !@@@ B | |"-  A/G\r B!  B | B|"|-  ! A/:  A !B ! A/G\r B|-  A/F\r A/: B! @@@@ A/  }"	B R"\n\r  E\r@ 	BR\r  -  A.G\r  B|!  P"\r   |B|-  A/F\r P\r B | B|"|A/:   	B|!   |A :  @ -  A/F\r  B |B Ì P\rB !	 B | !@ P\r @B !@ BT\r @B! B | |B|-  A/F\r B|"BV\r B!B ! 	B| 	B|"	 	  T!	  |! B|"B R\r    	}!@   	Q\r  B | |" B|-  A/F\r   A/:   B|!  |" B`|B`T\r  |  	| B|ó   B | À @ P\r     B|À !  ! 	!   |"BÿV\r   | B | | À   |A :    |!A!@@@@@@ 	BR\r  B | |"B~|-  A.G\r  B|-  A.G\r @   B~V\r  B|! ! A ! E\r  B | ô " Q\r@ B R\r © A,6 	 BU\r© ( AG\r \r \r@@   |B|-  A/F\r  B|" P\r A !  BR\rB!     \n!  B | |,  !@ B|"B(R\r © A 6  !	@ B | |B|-  A/G\r @ "	B|! 	 B ||-  A/F\r  B | 	 }"| B | ó   B|" B -  AÿqA/G  BR! A !B !  B | |ö  |! © A%6 B ! BÀ |$  #~  !@ "B|! -  A/F\r    }/@A  A £ "AaG\r   ¤ ! ¬ §¯~|@  ½"B4§Aÿq"A²K\r @ AýK\r   D        ¢@@  " D      0C D      0Ã   ¡"D      à?dE\r     D      ð¿ !     !  D      à¿eE\r   D      ð? !      B S!   ¦~# B}" $ @  B|  ¥ \r B   )BB|¤ "7á  P\r @  ) ¤ "P\r B )á "  )B|B 7   ¦ E\rB B 7á   B|$ ~@  A= "  R\r B B !@     }"|-  \r B )á "P\r  ) "P\r @@@     \r  )  |"-  A=F\r )! B|! PE\r  B|! ~~@ -  \r @Bô ú "P\r  -  \r@  ¬B~B¼ |ú "P\r  -  \r@Bû ú "P\r  -  \rB !B !@@@  |-  "E\r A/F\rB! B|"BR\r  !B !@@@@@ -  "A.F\r   |-  \r  ! AÃ G\r - E\r B  E\r  Bê  \r@  \r BÈ» ! - A.F\rB @B ) á "P\r @  B| E\r )("B R\r @B0¤ "P"\r  B )Ð» 7 B )È» 7  B|"  À   |A :   B ) á 7(B  7 á  BÈ»     ! õ~# BÐ }"$ B !@  AK\r Bá × @@  AG\r B ! B Q\r B )à¼ 7 B )Ø¼ 7 B )Ð¼ 7 @@@ A; " }"BU\r    À   |A :   B|  -  ! § û "BQ\r B | B| 7  B|"BR\r BèÞ  B |B0ü\n  Bá Ø B !@@ P\r @   û "BR\r Bá Ø   ­B 7èÞ   ­B)èÞ !Bá Ø B  B| P!B°á !A ! B !@B  B)èÞ "B| P!B )èÞ !    "À   |"A;:   B|!    Qj!  B|"BR\r  A :  Bá Ø  B°á   AF! BÐ |$  ;~# B}"$   7      ! B|$   A   A Ù  A  B      " B   -   AÿqF·~@@@@ Aÿq"E\r @  BP\r  Aÿq!@  -  "E\r  F\r  B|" BB R\r B À  ) "} B ÀB ÀR\r ­B À~!@B À  "} B ÀB ÀR\r  )!  B|"!  B À }B ÀB ÀQ\r      |  ! Aÿq!@ " -  "E\r  B|!  G\r   Y -  !@  -  "E\r   AÿqG\r @ - !  - "E\r B|!  B|!   AÿqF\r   Aÿqk~@@@   BP\r  -  !@ BB Q\r @   -  ":   E\r  B|!  B|"BPE\r @B À ) "} B ÀB ÀR\r @   7   B|!  "B|!B À )"} B ÀB ÀQ\r  §!   :   AÿqE\r @   - ":   B|!  B|! \r          /~@   B|"¤ "PE\r B     À ~  !@@  BP\r @  -  \r     }  !@ B|"BB Q\r -  \r @ "B|!B À ) "} B ÀB ÀQ\r @ "B|! -  \r    }w@ PE\r A @@  -  "\r A !@@ Aÿq -  "G\r E\r B|"P\r B|!  - !  B|!  \r A ! Aÿq!  -  k~~B ! B R!@@@  BP\r  P\r  Aÿq!@  -   F\r B|"B R!  B|" BP\r B R\r  E\r@  -   AÿqF\r  BT\r  Aÿq­B À~!@B À  )  "} B ÀB ÀR\r  B|!  Bx|"BV\r  P\r Aÿq!@@  -   G\r     B|! B ! B|"B R\r  ~   A   "  } P" @  B`T\r © A   §k6 B!   >~# B}"$   7   A¨ Ï !  B|$ AA   Z~# B}"$ @@ AI\r © A6 A!  7    A¨j Ï ! B|$  ~@  ½"B4§Aÿq"AÿF\r @ \r @@  D        b\r A !  D      ðC¢  !  ( A@j!  6     Axj6  BÿÿÿÿÿÿÿBð?¿!   «~# Bà}"$   7Ø B |A B(ü   )Ø7Ð@@B   BÐ| BÐ | B |   A N\r A!@@  (A N\r A!  ¯ E!    ( "A_q6 B !@@@@  )`B R\r   BÐ 7`  B 78  B 7   B 7(  )X!   7X  ) B R\rA!  É \r    BÐ| BÐ | B |   ! A q!@ P\r   B B   )H    B 7`   7X  B 78  B 7   )(!  B 7(A  P!    ( "	 r6 A  	A q! \r   °  Bà|$  ~~	~# BÀ }"$   78 B\'|! B(|!	A !\nA !@@@@@A !@ !\r  AÿÿÿÿsJ\r  j! \r!@@@@@@@ \r-  "E\r @@@@ Aÿq"\r  ! A%G\r !@@ - A%F\r  ! B|! - ! B|"! A%F\r   \r}" Aÿÿÿÿs"­U\r §!@  P"\r    \r Ä  \r	  78 B|!A!@ , APj"A	K\r  - A$G\r  B|!A!\n !  78A !@@ ,  "A`j"AM\r  !A ! !A t"AÑqE\r @  B|"78  r! , "A`j"A O\r !A t"AÑq\r @@ A*G\r @@ , APj"A	K\r  - A$G\r  ­!@@  B R\r   B|A\n6 A !  B|( ! B|!A!\n \n\r B|!@  B R\r   78A !\nA !  ) "B|7  ( !A !\n  78 AJ\rA  k! AÀ r! B8| "A H\r )8!A !A!@@ -  A.F\r A !@ - A*G\r @@ , APj"A	K\r  - A$G\r  ­!@@  B R\r   B|A\n6 A !  B|( ! B|! \n\r B|!@ E\r A !  ) "B|7  ( !  78 AJ!  B|78A! B8| ! )8!@ !A! ",  "AjAFI\r\r B|! ­B:~ ¬|B¿¼ |-  "AjAÿqAI\r   78@@ AF\r  E\r@ A H\r  ­!@  B R\r   B| 6    B|) 70 \r\n B0|     AJ\r\rA ! \r\n  -  A q\r\r Aÿÿ{q"  AÀ q!A !Bä ! 	!@@@@@@@@@@@@@@@@@ -  "À"ASq  AqAF  "A¨j!	\n  	!@ A¿j  AÓ F\rA !Bä ! )0!A !@@@@@@@   )0 6  )0 ¬7  )0 ¬7  )0 ;  )0 :   )0 ¬7  )0 ¬7  A AK! Ar!Aø !A !Bä ! )0" 	 A q !\r P\r AqE\r Av­Bä |!A!A !Bä ! )0" 	 !\r AqE\r 	 \r}" ¬S\r §Aj!@ )0"BU\r  B  }"70A!Bä !@ AqE\r A!Bå !Bæ Bä  Aq"!  	 !\r  A Hq\r Aÿÿ{q  !@ B R\r  \r A ! 	! 	!\r 	 \r} P­|" ¬"  U§! - 0!Bî  )0" P!\r \r \r Aÿÿÿÿ AÿÿÿÿI­ "|!@ AJ\r  -  \r §! )0"PE\rA !	@ E\r  ¬!\r )0!A !  A  A    A 6  >  B|70 B|!B!\rB !@@ ( "E\r B| ¢ "A H\r \r } ­"T\r B|!  |" \rT\r A=! BÿÿÿÿV\r  A   §"  @ PE\r A !B !\r )0!@ ( "E\r \r B| ¢ ¬"|"\r V\r   B|   B|! \r T\r   A    AÀ s     J!\n  A Hq\rA=!   +0       "A N\r	 - ! B|!   B R\r \nE\rB!@@  B|( "E\r  B|     B|"B\nR\r A!A! B\nZ\r@  B|( \r B|"B\nQ\r A!  : \'A! 	! !\r ! 	! Aÿÿÿÿs  \r}" ¬"  U§"H\rA=!   j"  J" K\r  A         ­   A0   As   A0  §A     \r    A    AÀ s  )8!A !A=!©  6 A! BÀ |$   @  -  A q\r     Ê ~~A !@  ) ",  APj"A	M\r A @A!@ AÌ³æ K\r A  A\nl"j  AÿÿÿÿsK!   B|"7  , ! ! ! APj"A\nI\r  â @@@@@@@@@@@@@@@@@@@ Awj 	\n\r  ) B|Bx"B|7    ) 7   ) "B|7    4 7   ) "B|7    5 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) "B|7    2 7   ) "B|7    3 7   ) "B|7    0  7   ) "B|7    1  7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    ) 7   ) B|Bx"B|7    + 9       9 @  P\r @ B|"  B- ÐÀ  r:    B" B R\r  . @  P\r @ B|"  §AqA0r:    B" B R\r  =~@  P\r @ B|"    B\n"B\n~}§A0r:    B	V! !  \r  ~# B}"$ @  L\r  AÀq\r     k"A AI"­¶ @ \r @   B  A~j"AÿK\r     ­  B|$ $     Bù Bú  ~~~~~|~# B°}"$ A ! A 6,@@  "BU\r A!	Bî !\n " !@ AqE\r A!	Bñ !\nBô Bï  Aq"	!\n 	E!@@ Bøÿ Bøÿ R\r   A   	Aj" Aÿÿ{q    \n 	­   Bø Bð  A q"BÐ B    bB   A    AÀ s     J! B|!\r@@@@  B,| "  "D        a\r   (,"Aj6, A r"Aá G\r A r"Aá F\rA  A H! (,!  Acj"6,A  A H! D      °A¢! B0|B B  A H|"!@  ü"6  B|!  ¸¡D    eÍÍA¢"D        b\r @@ AN\r  ! ! !@ A AI!@ B||" T\r  ­!B !@  5   |" BëÜ"BëÜ~}>  B||" Z\r  BëÜT\r  B||" > @@ " X\r B||"( E\r   (, k"6, ! A J\r @ AJ\r  AjA	nAj­! Aæ F!@A  k"A	 A	I!@@  T\r B B ( !AëÜ v!A tAs!A ! !@  ( " v j6   q l! B|" T\r B B ( ! E\r   6  B|!  (, j"6,   |" " B|   }B U! A H\r A !@  Z\r   }B§A	l!A\n! ( "A\nI\r @ Aj!  A\nl"O\r @  }BB	~Bw| A   Aæ Fk A G Aç Fqk"¬W\r   AÈ j"A	m"¬B|"B`|!A\n!@  A	lk"AJ\r @ A\nl! Aj"AG\r  B`|!@@ ( "  n" lk"\r   Q\r@@ Aq\r D      @C! AëÜG\r  X\r B`|-  AqE\rD     @C!D      à?D      ð?D      ø?  QD      ø?  Av"F  I!@ \r  \n-  A-G\r  ! !   k"6     a\r    j"6 @ AëÜI\r @ A 6 @ B||" Z\r  B||"A 6   ( Aj"6  AÿëÜK\r   }B§A	l!A\n! ( "A\nI\r @ Aj!  A\nl"O\r  B|"   V!@@ " X"\r B||"( E\r @@ Aç F\r  Aq! AsA A " J A{Jq" j!AA~  j! Aq"\r B	!@ \r B	! B||( "E\r A\n!B ! A\np\r A !@ Aj!  A\nl"pE\r  ­!  }BB	~! ¬!@@ A_qAÆ G\r   }Bw|"B  B U"   S§! ¬ | }Bw|"B  B U"   S§!A !A! AýÿÿÿAþÿÿÿ  r"J\r  A GjAj!@@ A_q"AÆ G\r   AÿÿÿÿsJ\r A  A J!@ \r  Au"s k­ \r "}BU\r @ B|"A0:   \r }BS\r  B~|" :   B|A-A+ A H:   \r }" Aÿÿÿÿs­U\r §!  j" 	AÿÿÿÿsJ\r  A    	j"     \n 	­   A0   As @@@@ AÆ G\r  B|B	!    V"!@ 5   !@@  Q\r   B|X\r@ B|"A0:    B|V\r   R\r  B|"A0:       }  B|" X\r @ E\r   Bì B   Z\r AH\r@@ 5   " B|X\r @ B|"A0:    B|V\r     A	 A	H­  Awj! B|" Z\r A	J! ! \r @ A H\r   B|  V! B|B	! !@@ 5   " R\r  B|"A0:  @@  Q\r   B|X\r@ B|"A0:    B|V\r    B  B|!  rE\r   Bì B      }" ­"  S   §k! B|" Z\r AJ\r   A0 AjAA      \r }  !  A0 A	jA	A    A    AÀ s     J! \nB	B  A q"|!@ AK\r A k!D      0@!@ D      0@¢! Aj"\r @ -  A-G\r    ¡ !    ¡!@ (," Au"s k­ \r " \rR\r  B|"A0:   (,! 	Ar! B~|" Aj:   B|A-A+ A H:   AH AqEq! B|!@ " ü"¬BÐÀ |-   r:    ·¡D      0@¢!@ B|" B|}BR\r  D        a q\r  A.:  B|! D        b\r A!Býÿÿÿ \r }" ­"|} ¬"S\r   A    §jAj  B|} |§"  B|}"B~| S  " j"         A0   As    B|    A0   |§kA A         A    AÀ s     J! B°|$  .~  ) B|Bx"B|7    )  )´ 9    ½¥~# B}"$   Bþ|   P" 7è B  B|"  V7ð A Bèü  A6 Bû 7H A6  Bÿ|7X  Bè|7  A :      ! B|$  ·~  )") !@ )"  )(  )8"}"  T"P\r    À   )  |"7   ) }"7@    T"P\r    À   )  |"7   ) }7 A :      )X"78   7(  @  \r A ©   6 AD~@ E\r @@  "( "E\r B|!   G\r  B        B|#~  !@ "B|! ( \r    }B°~B!@@  P\r  Aÿ M\r@@é )¨) B R\r  AqA¿F\r© A6 @ AÿK\r    A?qAr:    AvAÀr:  B@@ A°I\r  A@qAÀG\r   A?qAr:    AvAàr:     AvA?qAr: B@ A|jAÿÿ?K\r    A?qAr:    AvAðr:     AvA?qAr:    AvA?qAr: B© A6 B!    :  B @  PE\r A    B ¡ §	 §  .~~~# B}"$ @@@@@  BðV\r @B (Øê "B   B|Bø  BT"B§"v"AqE\r @@ AsAq j"At­B" B ë |"  )°ë ")" R\r B  A~ wq6Øê   B )ðê T\r  ) R\r   7   7 B|!   At­"B7  |" )B7 B )àê "X\r@ E\r @@  tA t"A  krqh"At­B" B ë |"  )°ë ")" R\r B  A~ wq"6Øê   B )ðê T\r  ) R\r   7   7 B|! @ At­" }"	BV\r   B7  |" )B7  B7  |"\n 	B7  | 	7 @ P\r  BBðÿÿÿÿ B ë |!B )øê !@@ A B§t"q\r B   r6Øê  ! )"B )ðê T\r  7  7  7  7B  \n7øê B  	7àê B (Üê "E\r h­B)°ï ")Bx }! !	@@@ ) " B R\r  )(" P\r  )Bx }"   T"!   	 !	  !  	B )ðê "T\r 	)0!@@ 	)"  	Q\r  	)" T\r ) 	R\r  ) 	R\r   7   7@@ 	)("B Q\r  	B(|!\n@ 	) "PE\r B !  	B |!\n@ \n!\r " B(|!\n  )("B R\r   B |!\n  ) "B R\r  \r T\r \rB 7 @ P\r @@ 	 	(8"­B")°ï R\r  B°ï |  7   B R\rB  A~ wq6Üê   T\r@@ )  	R\r    7    7(  P\r   T\r   70@ 	) "P\r   T\r   7    70 	)("P\r   T\r   7(   70@@ BV\r  	  |" B7 	  |"   )B7 	 B7 	 |" B7  | 7 @ P\r  BBðÿÿÿÿ B ë |!B )øê ! @@A B§t" q\r B   r6Øê  !\n )"\n T\r   7 \n  7   7   \n7B  7øê B  7àê  	B|! B!  Bÿ~V\r   B|" Bx!B (Üê "E\r @@  B§"\r A !@ AÿÿM\r A! A& g"k­§Aq AtrA>s!B  }!@@@@ ­B)°ï "PE\r B ! B !	 B B? AvAj­} AF!\nB ! B !	@@ )Bx }"\r Z\r  \r! !	 \rPE\r B ! !	 !      )("\r \r  \nB<B|) "Q \rP!  \nB!\n ! B R\r @   	B R\r A t"A  kr q"E\r h­B)°ï ! B !	  P\r@  )Bx }"\n T!@  ) "B R\r   )(! \n  !   	 !	 !  B R\r  	P\r  B )àê  }Z\r  	B )ðê "T\r 	)0!@@ 	)"  	Q\r  	)" T\r ) 	R\r  ) 	R\r   7   7@@ 	)("B Q\r  	B(|!\n@ 	) "PE\r B !  	B |!\n@ \n!\r " B(|!\n  )("B R\r   B |!\n  ) "B R\r  \r T\r \rB 7 @ P\r @@ 	 	(8"­B")°ï R\r  B°ï |  7   B R\rB  A~ wq"6Üê   T\r@@ )  	R\r    7    7(  P\r   T\r   70@ 	) "P\r   T\r   7    70 	)("P\r   T\r   7(   70@@ BV\r  	  |" B7 	  |"   )B7 	 B7 	 |"\n B7 \n | 7 @ BÿV\r  B"BB ë |! @@B (Øê "A §t"q\r B   r6Øê   !  )" T\r   \n7  \n7 \n  7 \n 7@@ B§"\r A !@ AÿÿM\r A! A& g"k­§Aq AtrA>s! \nB 7( \n 68 \nB 7  ­BB°ï |!@@@ A t"q\r B   r6Üê   \n7  \n 70 B B? AvAj­} AF!  ) !@ ")Bx Q\r  B<!  B!   B|"\r) "B R\r  \rB |"  T\r   \n7  \n 70 \n \n7 \n \n7  T\r )"  T\r   \n7  \n7 \nB 70 \n 7 \n  7 	B|! @B )àê "  T\r B )øê !@@   }"B T\r   |"	 B7   | 7   B7   B7   |"   )B7B !B !	B  7àê B  	7øê  B|! @B )èê "	 X\r B  	 }"7èê B B )ë "  |"7ë   B7   B7  B|! @@B )ò P\r B )ò !B !B B 7ò B A 6¨ò B B7 ò B B7ò B B 7ò B A 6Èñ B  B|BpBØªÕª7ò B !   BÏ |"|"\rB  }""\n X\rB ! @B )Àñ "P\r B )°ñ " \n|" X\r  V\r@@@B - Èñ Aq\r @@@@@B )ë "P\r BÐñ ! @@   ) "T\r     )|T\r  )" B R\r B ¬ "	BQ\r \n!\r@B )ò " B|" 	P\r  \n 	}  	|B   }|!\r \r X\r@B )Àñ " P\r B )°ñ " \r|" X\r   V\r \r¬ "  	R\r \r 	} "\r¬ "	  )   )|Q\r 	!   BQ\r@ \r BÐ |T\r   !	  \r}B )ò "|B  }"¬ BQ\r  \r|!\r  !	 	BR\rB B (Èñ Ar6Èñ  \n¬ !	B ¬ !  	BQ\r  BQ\r 	  Z\r   	}"\r BÈ |X\rB B )°ñ  \r|" 7°ñ @  B )¸ñ X\r B   7¸ñ @@@@B )ë "B Q\r BÐñ ! @ 	  ) "  )"\n|Q\r  )" PE\r @@B )ðê " P\r  	  Z\rB  	7ðê B ! B A 6èñ B  \r7Øñ B  	7Ðñ B B7ë B B )ò 7ë @  B" B ë |"7°ë   7¸ë   B|" B R\r B  \rB¸|" Bp 	}B"}"7èê B  	 |"7ë   B7 	  |BÈ 7B B ) ò 7ë   	Z\r   T\r   (Aq\r    \n \r|7B  Bp }B" |"7ë B B )èê  \r|"	  }" 7èê    B7  	|BÈ 7B B ) ò 7ë @ 	B )ðê Z\r B  	7ðê  	 \r|!BÐñ ! @@@  ) "\n Q\r  )" PE\r   - AqE\rBÐñ ! @@@   ) "T\r     )|"T\r  )!  B  \rB¸|" Bp 	}B"\n}"7èê B  	 \n|"\n7ë  \n B7 	  |BÈ 7B B ) ò 7ë   B? }B|B±|"    B |T"\nB+7 \nB )èñ 7( \nB )àñ 7  \nB )Øñ 7 \nB )Ðñ 7B  \r7Øñ B  \nB|7àñ B A 6èñ B  	7Ðñ  \nB(|! @  B7  B|!	  B|!  	 T\r  \n Q\r  \n \n)B~7  \n }"\rB7 \n \r7 @@ \rBÿV\r  \rB"BB ë |! @@B (Øê "A §t"q\r B   r6Øê   !  )"B )ðê T\r   7  7B!	B!\n@@ \rB§"\r A !@ AÿÿM\r A! \rA& g"k­§Aq AtrA>s! B 7(  68 B 7  ­BB°ï |!@@@B (Üê "A t"q\r B   r6Üê   7   70 \rB B? AvAj­} AF!  ) !	@ 	")Bx \rQ\r  B<!	  B!   	B|"\n) "	B R\r  \nB |" B )ðê T\r   7   70B!	B!\n ! !  B )ðê "	T\r )"  	T\r   7  7   7B ! B0!	B!\n  \n| 7   	|  7 B )èê "  X\r B    }"7èê B B )ë "  |"7ë   B7   B7  B|! © A06 B ! £     	7     ) \r|7 	 \n ¥ !  B|$   Ý~  Bp  }B|" B7 Bp }B|"  |"}!@@@ B )ë R\r B  7ë B B )èê  |"7èê   B7@ B )øê R\r B  7øê B B )àê  |"7àê   B7  | 7 @ )"BBR\r @@ BÿV\r  )!@ )"  B"BB ë |"Q\r   B )ðê T\r  ) R\r@   R\r B B (Øê A~ §wq6Øê @  Q\r  B )ðê T\r ) R\r   7   7 )0!	@@ )" Q\r  )" B )ðê T\r  ) R\r ) R\r   7   7@@ )(" B Q\r  B(|!@ ) " PE\r B ! B |!@ !  "B(|! )(" B R\r  B |! ) " B R\r  B )ðê T\r B 7  	P\r @@  (8"\n­B" )°ï R\r   B°ï | 7  B R\rB B (Üê A~ \nwq6Üê  	B )ðê T\r@@ 	)  R\r  	 7  	 7( P\r B )ðê "T\r  	70@ ) " P\r    T\r   7    70 )(" P\r    T\r   7(   70 Bx" |!  |")!  B~7  B7  | 7 @ BÿV\r  B" BB ë |!@@B (Øê "\nA  §t"q\r B  \n r6Øê  !  )" B )ðê T\r  7   7  7   7@@ B§"\n\r A !\n@ \nAÿÿM\r A!\n A& \ng"\nk­§Aq \nAtrA>s!\n B 7(  \n68 B 7  \n­BB°ï |! @@@B (Üê "A \nt"q\r B   r6Üê    7    70 B B? \nAvAj­} \nAF!  ) !@ " )Bx Q\r B<! B!   B|") "B R\r  B |"B )ðê T\r  7    70  7  7  B )ðê "T\r  )" T\r  7   7 B 70   7  7 B|£  ×~~@@  P\r   Bp|"B )ðê "T\r  Bx|) "BBQ\r  Bx" |!@ §Aq\r  BP\r  ) "}" T\r   |! @ B )øê Q\r @ BÿV\r  )!@ )" B"BB ë |"Q\r   T\r ) R\r@  R\r B B (Øê A~ §wq6Øê @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r@@  (8"	­B")°ï R\r  B°ï | 7  B R\rB B (Üê A~ 	wq6Üê   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r  T\r  7(  70 )"BBR\r B   7àê   B~7   B7   7   Z\r )"BP\r@@ BB R\r @ B )ë R\r B  7ë B B )èê   |" 7èê    B7 B )øê R\rB B 7àê B B 7øê @ B )øê "\nR\r B  7øê B B )àê   |" 7àê    B7   |  7 @@ BÿV\r  )!@ )" B"BB ë |"Q\r   T\r ) R\r@  R\r B B (Øê A~ §wq6Øê @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r @@  (8"	­B")°ï R\r  B°ï | 7  B R\rB B (Üê A~ 	wq6Üê   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70  Bx  |" B7   |  7   \nR\rB   7àê   B~7   B7   |  7 @  BÿV\r   B"BB ë |! @@B (Øê "	A §t"q\r B  	 r6Øê   !  )" T\r   7  7   7  7@@  B§"	\r A !	@ 	AÿÿM\r A!	  A& 	g"	k­§Aq 	AtrA>s!	 B 7(  	68 B 7  	­BB°ï |!@@@@B (Üê "A 	t"\rq\r B   \rr6Üê   7 B! B0!  B B? 	AvAj­} 	AF! ) !@ ")Bx  Q\r B<! B!  B|") "B R\r  B |"  T\r   7 B! B0! ! ! !  T\r )" T\r  7  7B !B0! B!  | 7   7   | 7 B BB )ë B|" P7ë £  ¥~@  B R\r  ¤ @ BT\r © A06 B @  Bp|B  B|Bx BT¨ "P\r  B|@ ¤ "PE\r B    BpBx  Bx|) "BP Bx|"   TÀ   ¦  \n	~@@  B )ðê "T\r   )"B"BQ\r  Bx"P\r    |")"BP\r B !@ B R\r  BT\r@  B|T\r   !  }B )ò BX\rB !@  T\r @  }"B T\r     BB7   |" B7  )B7  ©   B !@ B )ë R\r B )èê  |" X\r    BB7   |"  }"B7B  7èê B  7ë   @ B )øê R\r B !B )àê  |" T\r@@  }"B T\r     BB7   |" B7   |" 7   )B~7   B B7   |" )B7B !B !B  7øê B  7àê   B ! BB R\r Bx |"	 T\r@@ BÿV\r  )!@ )" B"BB ë |"Q\r   T\r ) R\r@  R\r B B (Øê A~ §wq6Øê @  Q\r   T\r ) R\r  7  7 )0!\n@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  \nP\r @@  (8"­B")°ï R\r  B°ï | 7  B R\rB B (Üê A~ wq6Üê  \n T\r@@ \n)  R\r  \n 7  \n 7( P\r  T\r  \n70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70@ 	 }"BV\r    B 	B7   	|" )B7      BB7   |" B7   	|" )B7  ©   £   ~~   |!@@@@  )"BP\r B )ðê ! BP\r    ) "}" B )ðê "T\r  |!@  B )øê Q\r @ BÿV\r   )!@  )" B"BB ë |"Q\r   T\r )  R\r@  R\r B B (Øê A~ §wq6Øê @  Q\r   T\r )  R\r  7  7  )0!@@  )"  Q\r   )" T\r )  R\r )  R\r  7  7@@  )("B Q\r   B(|!@  ) "PE\r B !  B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r@@    (8"	­B")°ï R\r  B°ï | 7  B R\rB B (Üê A~ 	wq6Üê   T\r@@ )   R\r   7   7( P\r  T\r  70@  ) "P\r   T\r  7   70  )("P\r  T\r  7(  70 )"BBR\r B  7àê   B~7   B7  7   T\r@@ )"BB R\r @ B )ë R\r B   7ë B B )èê  |"7èê    B7  B )øê R\rB B 7àê B B 7øê @ B )øê "\nR\r B   7øê B B )àê  |"7àê    B7   | 7 @@ BÿV\r  )!@ )" B"BB ë |"Q\r   T\r ) R\r@  R\r B B (Øê A~ §wq6Øê @  Q\r   T\r ) R\r  7  7 )0!@@ )" Q\r  )" T\r ) R\r ) R\r  7  7@@ )("B Q\r  B(|!@ ) "PE\r B ! B |!@ ! "B(|! )("B R\r  B |! ) "B R\r   T\r B 7  P\r @@  (8"	­B")°ï R\r  B°ï | 7  B R\rB B (Üê A~ 	wq6Üê   T\r@@ )  R\r   7   7( P\r  T\r  70@ ) "P\r   T\r  7   70 )("P\r   T\r  7(  70   Bx |"B7   | 7    \nR\rB  7àê   B~7   B7   | 7 @ BÿV\r  B"BB ë |!@@B (Øê "	A §t"q\r B  	 r6Øê  ! )" T\r   7   7   7   7@@ B§"	\r A !	@ 	AÿÿM\r A!	 A& 	g"	k­§Aq 	AtrA>s!	  B 7(   	68  B 7  	­BB°ï |!@@@B (Üê "A 	t"\rq\r B   \rr6Üê    7    70 B B? 	AvAj­} 	AF! ) !@ ")Bx Q\r B<! B!  B|") "B R\r  B |" T\r   7    70    7    7  T\r )" T\r   7   7  B 70   7   7£  ~# B}"$ @@  PE\r B !   B  B ­  ) !   BT\r B  )B R!@ ¤ " P\r   Bx|-  AqE\r   A  ¶  B|$    ? B~~@@  B S\r   B|Bx! B B   }Bøÿÿÿÿÿÿÿÿ }! @B )Ï "  |" « X\r   ¨ \r © A06 BB   7Ï  u~    ~  ~| B " B "~| Bÿÿÿÿ" Bÿÿÿÿ"~"B   ~|"B | Bÿÿÿÿ  ~|"B |7   B  Bÿÿÿÿ7 * B $ B B|Bp$  # # } #  # S~@@ AÀ qE\r   A@j­!B ! E\r  AÀ  k­  ­"!  !   7    7S~@@ AÀ qE\r   A@j­!B ! E\r  AÀ  k­  ­"!  !   7    7§~# B }"$  Bÿÿÿÿÿÿ?!@@ B0Bÿÿ"§"AÿjAýK\r   B< B! Aj­!@@  Bÿÿÿÿÿÿÿÿ" BT\r  B|!  BR\r  B |!B   BÿÿÿÿÿÿÿV"!  ­ |!@   P\r  BÿÿR\r   B< BB! Bÿ!@ AþM\r Bÿ!B ! @Aø Aø  P"" k"Að L\r B ! B !  BÀ  !A !@  F\r  B|   A k²  ) )B R!     ³  ) "B< )B! @@ Bÿÿÿÿÿÿÿÿ ­"BT\r   B|!  BR\r   B  |!   B    BÿÿÿÿÿÿÿV"!  ­! B |$  B4 B  ¿     A A A © ¬ §     ­A A A ª ¬ §     ­A A A « ¬ §   )¦   ¦     A A A A ¬ ¬ §      B B »         ­ ¬       B A ½         ­® ¬        ­A ç ¬ §Ã~~# B }"$ @@    A A A ¯ "AdF\r  A¾G\r A qE\r    Aÿï_q A A A ¯ "A H\r @ A qE\r  B7 A B|  AqE\r  B7  A   ¬ ! B |$  §\n   $ ~#   }Bp"$   # \\~Bó !@  AK\r   ­B/àÀ !@  E\r  AÿÿqE\r ­BÿÿBÃ |! ~   Ã \n   Æ    At  AvrAÿÿq\n   È    AÿüqAx  AxAÿüqrÓ BRlist-directory is-directory delete-directory copy alt-key shift-key ctrl-key meta-key get-index max -+   0X0x -0X+0X 0X-0x+0x 0x pow is-env make-env div %lu get-text update-text is-list last sqrt sort import str-insert alert warning: unsupported syscall: __syscall_setsockopt not is-int to-int environment comment create-client exit is-unit split gt set ret let is-dict is-float to-float repeat rows on-key-press eval-macros compiled-macros cols is-bytes to-bytes len-bytes abs to-str console-error Unknown error create-server on-mouse-enter filter identifier aether is-number is-alpha-number eq on-key-up on-mouse-up zip map macro get-file-info do on-key-down on-mouse-down console-warn button accept-connection close-connection term/raw-mode-on join min len nan \\n current-platform atom mul is-bool to-bool get-html update-html tail eval string literal on-click on-double-click set-current-path get-current-path get-absolute-path match for-each console-log is-string printf inf elif term/raw-mode-off %f term/get-size receive-size str-remove on-mouse-move receive on-mouse-leave true value use else false type new line compile write-binary-file read-binary-file write-file delete-file read-file get-range gen-range code is-whitespace str-replace mod round send and fold %ld eval-compiled add head is-func sub web rwa is-alpha `}` `{` `]` `\\` `[` `=>` `<>` `->` `:` `::` `...` `)` `(` POSIX NAN LC_ALL LANG INF C <lambda> <-> add-byte-8 C.UTF-8 add-byte-16 add-byte-64 add-byte-32 /usr/include/aether/ ae-src/ -> ... (null) set!  or  %.*s:%u:%u: [ERROR] Expected  %.*s: [ERROR] Expected   ->  src/std/str.c:%d:  src/lib/deserializer.c:%d:  src/lib/serializer.c:%d:  src/lib/parser.c:%d:  src/lib/vm.c:%d:  src/std/core.c:%d:  src/lib/misc.c:%d:  ,     {\n [ERROR] %.*s:%u:%u: Not enough values on the stack: expected %u, got %u\n [ERROR] %.*s:%u:%u: Expected %u arguments, got %u\n [ERROR] Offset index out of bounds: %u\n [ERROR] Unknown value king: %u\n [ERROR] Unknown value kind: %u\n [ERROR] %.*s:%u:%u: Lists can only be indexed with integers\n [INFO] Long trace, showing last %u calls\n [ERROR] Corrupted bytecode: expected %u, but got %u bytes\n [ERROR] %.*s:%u:%u: List index out of bounds\n [ERROR] %.*s:%u:%u: String index out of bounds\n [ERROR] str-remove: out of bounds\n [ERROR] join: wrong part kinds\n [INFO] Trace: %.*s:%u:%u %.*s\n [ERROR] Could not find offset for %.*s\n [ERROR] %.*s:%u:%u: Target label was not found: %.*s\n [ERROR] %.*s:%u:%u: Target label not found: %.*s\n [ERROR] %.*s:%u:%u: Value of type string can only be indexed with integer\n [ERROR] filter: predicate should return bool\n [ERROR] %.*s:%u:%u: Match case not inside of a match block\n [ERROR] %.*s:%u:%u: Match end not inside of a match block\n [ERROR] make-env: every program argument should be of type string\n [ERROR] %s: every include path should be of type string\n %.*s:%u:%u: [ERROR] Could not import `%.*s` module\n [ERROR] %.*s:%u:%u: Value of type %.*s is not callable\n [ERROR] %.*s:%u:%u: Intrinsic %.*s:%u was not found\n [ERROR] %.*s:%u:%u: Symbol %.*s was not found\n [ERROR] %.*s:%u:%u: Value of type %.*s cannot be indexed\n [ERROR] %.*s:%u:%u: Infinite recursion detected\n [ERROR] %.*s:%u:%u: Value of type string cannot be mutated\n %.*s:%u:%u: [ERROR] String literal was not closed\n [ERROR] Corrupted bytecode: wrong magic\n [ERROR] Corrupted bytecode: not enough data\n , but got `%.*s`\n %.*s:%u:%u: [ERROR] Unexpected `%lc`\n , but got EOF\n [INFO] Stack dump:\n        0             ABC  ABM  ABC  ABM                          ÿÿÿÿ    .abm  .ae  ABM          ½  -     9      \n         ABC  ABM         !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxy                                ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ        þÿÿþÿÿ      ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÃÿ P                ß¼@×ÿÿûÿÿÿÿÿÿÿÿÿ¿ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿüÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿþÿÿÿÿÿÿÿÿ    ÿ¿¶ ÿÿÿ   ÿÿÿÿÿÿÿÿþÿÃÿÿÿÿÿÿÿÿÿÿÿÿïþáÿ  ÿÿÿÿÿÿ àÿÿÿÿÿÿÿÿÿÿÿÿ ÿÿÿÿÿ0ÿÿÿüÿ  ÿÿÿÿ      ÿÿß?  ðÿøÿÿÿÿÿÿÿÿÿïÿßáÿÏÿþÿïùÿÿýÅãY°ÏÿîùÿÿýmÃ^Àÿ? î¿ûÿÿýíã¿ Ïÿ îùÿÿýíãÀ°Ïÿ ìÇ=ÖÇÿÃÇ Àÿ  ïßýÿÿýÿãß`Ïÿ  ïßýÿÿýïãß`@Ïÿ ïßýÿÿÿÿçß]ðÏÿ üìÿüÿÿû/_ÿÀÿ þÿÿÿÿÿ? ÿ    Ö÷ÿÿ¯ÿÿ;_ ÿó       ÿ  ÿþÿÿÿþÿÿÿþÿÿÿ        ÿÿÿÿÿÿùÿÿÿÿÿÿÿÿÿÿ?ÿÿÿÿ¿ ÿÿÿÿÿ÷ÿÿÿÿÿÿÿÿÿ==ÿÿÿÿÿ=ÿÿÿÿ==ÿÿÿÿÿÿÿÿ=ÿÿÿÿÿÿÿÿ    ÿÿ  ÿÿÿÿÿÿÿÿÿÿ??þÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿþÿÿÿÿÿÿÿÿÿÿÿÇÿÿß ÿÿ ÿÿ ÿß\r ÿÿÿÿÿÿÏÿÿÿ    ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ? ÿÿÿÿÿÀÿÿÿÿ? ÿÿÿÿÿÿÿÿÿ    ÿÿÿÿÿÿÿÿÿÿþÿ ÿÿ           ÿÿÿÿÿÿïÿïÿ    ÿÿÿÿÿóÿÿÿÿÿÿ¿ÿ ÿÿÿÿÿÿ ÿãÿÿÿÿÿ?ÿÿÿÿÿÿç     Þoÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ    ÿ ÿÿ??ÿÿÿÿ??ÿªÿÿÿ?ÿÿÿÿÿÿß_ÜÏÿÜ                ÿ            ü/>P½ÿóàC  ÿÿÿÿÿ                                    Àÿÿÿÿÿÿ  ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿx ÿÿÿÿ¿ ÿÿÿÿÿÿÿ  ÿÿ ÿÿÿÿ                               à   þ>þÿÿÿÿÿÿÿÿÿàþÿÿÿÿÿÿÿÿÿÿ÷àÿÿÿÿÿþÿÿÿÿÿÿÿÿÿÿ  ÿÿÿ      ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ?         ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ  ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ        ÿÿÿÿÿ?ÿÿÿÿ  ÿÿÿÿÿðÿÿÿÿÿÿÿÿÿÿÿÿÿÿ    ÿüÿÿÿÿÿÿÿÿÿÿÿÿùÿÿÿÿÿÿ|     ÿ¿ÿÿÿÿ   ÿÿÿÿÿÿ ÿÿÿÿÿÿÿÿ/ ÿ  üèÿÿÿÿÿÿÿÿÿ ÿÿÿÿÿÿÿÿÿ÷ÿ ÿÿÿÿÿÿÿÿÿÿ ÿ?ÿÿÿüÿÿÿÿÿÿÿ  8ÿÿ< ~~~ ÿÿÿÿÿ÷ÿ ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ ÿÿøÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ?ÿÿÿÿÿÿÿÿÿÿÿÿÿ     øàÿý_Ûÿÿÿÿÿÿÿÿÿÿÿÿÿ   øÿÿÿÿÿÿÿÿÿÿÿÿ?  ÿÿÿÿÿÿÿÿüÿÿÿÿÿÿ     ÿ              ßÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ  ÿþÿÿþÿÿÀÿÿÿÿÿÿÿÿÿÿüüü    ÿïÿÿÿÿ·ÿ?ÿ?    ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ        ÿÿÿÿÿÿ                                 ÿÿÿÿÿÿÿÿÿ     ÿÿÿÿ àÿÿÿÿÿÿÿÿÿÿÿ?ÿÿÿÿÿ>     ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ?ÿÿÿÿÿÿÿÿÿÿÿÿÿÿ ÿÿÿÿÿÿ                   ÿÿÿÿÿÿ ÿÿ? ÿ                   ?ýÿÿÿÿ¿ÿÿ? ÿÿ ÿÿÿ        ÿÿ7 ÿÿ? ÿÿÿ        ÿÿÿÿÿÿÿÀ        oðïþÿÿ?     ÿÿÿÿÿÿ    ÿþÿÿ   ÿÿÿÿÿÿ? ÿÿ? ÿÿ ÿÿ             ÿÿÿÿÿÿÿÿÿ      ÿÿÿÿÿÿ ÿÿÿÿÿÿ ÿÿÿÿÿ ÿ                        ÿÿÿ ÿÿ?                   ÿÿ ÿÿÿÿÿÿÿÿ?   Àÿ  üÿÿÿÿÿÿ  ÿÿÿÿÿÿÿÿÿÿÇÿp ÿÿÿÿG ÿÿÿÿÿÿÿÿ ÿ    ÿÿûÿÿÿ@        ½ÿ¿ÿÿÿÿÿÿÿÿÿïùÿÿýíãà                   ÿÿÿÿÿÿÿÿ»ÿ    ÿÿÿÿÿÿÿÿ³ ÿ                    ÿÿÿÿÿÿ?   ?    ÿÿÿÿÿÿÿ ÿ    ÿÿÿÿÿÿ?ÿ      ÿÿÿçÿÿ                        ÿÿÿÿÿÿÿ            ÿÿÿÿÿÿÿÿÿ                     ÿüÿÿÿÿÿü   ÿÿÿÿÿÿç  ÿÿÿÿÿÿÿÿÿ     ÿÿÿÿÿÿÿÿýÿÿÿÿ ÿ  üÿÿÿüÿÿþ         ûÿÿÿÿ´Ë ÿ¿ýÿÿÿ{ÿ                                      ÿÿ ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ            ÿÿÿÿÿÿÿÿÿÿÿÿÿ  ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ                       ÿÿÿÿÿ                          ÿÿÿÿÿÿÿÿ                       ÿÿÿÿÿÿÿÿÿÿÿ            ÿÿÿ?  ÿÿÿÿÿÿ   ÿøÿÿàÿÿ                      ÿÿÿÿÿÿÿÿ                ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ           ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ ÿÿÿ       ð ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿC            ÿÿÿÿÿÿÿÿÿÿßÿÿÿÿÿÿÿÿßdÞÿëïÿÿÿÿÿÿÿ¿çßßÿÿÿ{_üýÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ?ÿÿÿýÿÿ÷ÿÿÿ÷ÿÿßÿÿÿßÿÿÿÿÿÿÿÿýÿÿÿýÿÿ÷ÏÿÿÿÿÿÿÿÿùÛ                          ÿÿÿÿÿ?ÿC                                              ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ       ÿÿÿÿÿÿÿÿÿ                    ïÿÿÿþ÷\nêª÷÷^ÿûÿîûÿ              ÿÿÿÿÿÿÿÿÿ                  	   \n   \r                                     	   \n   (   )   _    0              Þ    ÿÿÿÿÿÿÿÿÿÿÿÿ    °            C.UTF-8                                 LC_CTYPE    LC_NUMERIC  LC_TIME     LC_COLLATE  LC_MONETARY LC_MESSAGES         C.UTF-8                 ¨e     f                         	             \n\n\n  	  	                               \r \r   	   	                                               	                                                  	                                                   	                                              	                                                      	                                                   	         0123456789ABCDEF   N ë§~ uú ¹,ý·z¼ Ì¢ =I×  *_·úXÙýÊ½áÍÜ@x }gaì å\nÔ Ì>Ov¯  D ® ®` úw!ë+ `A ©£nN                                                        *                    \'9H                                  8R`S  Ê        »Ûë+;PSuccess Illegal byte sequence Domain error Result not representable Not a tty Permission denied Operation not permitted No such file or directory No such process File exists Value too large for defined data type No space left on device Out of memory Resource busy Interrupted system call Resource temporarily unavailable Invalid seek Cross-device link Read-only file system Directory not empty Connection reset by peer Operation timed out Connection refused Host is down Host is unreachable Address in use Broken pipe I/O error No such device or address Block device required No such device Not a directory Is a directory Text file busy Exec format error Invalid argument Argument list too long Symbolic link loop Filename too long Too many open files in system No file descriptors available Bad file descriptor No child process Bad address File too large Too many links No locks available Resource deadlock would occur State not recoverable Owner died Operation canceled Function not implemented No message of desired type Identifier removed Device not a stream No data available Device timeout Out of streams resources Link has been severed Protocol error Bad message File descriptor in bad state Not a socket Destination address required Message too large Protocol wrong type for socket Protocol not available Protocol not supported Socket type not supported Not supported Protocol family not supported Address family not supported by protocol Address not available Network is down Network unreachable Connection reset by network Connection aborted No buffer space available Socket is connected Socket not connected Cannot send after socket shutdown Operation already in progress Operation in progress Stale file handle Remote I/O error Quota exceeded No medium found Wrong medium type Multihop attempted Required key not available Key has expired Key has been revoked Key was rejected by service  BÒ}                 	   	         \r   \r         ÿÿÿÿÿÿÿÿ       \n   \n          ;   ;          l   l         e   e         t   t          i   i         f   f          e   e         l   l         i   i         f   f          e   e         l   l         s   s         e   e          m   m         a   a         c   c         r   r         o   o          s   s         e   e         t   t          u   u         s   s         e   e          r   r         e   e         t   t          i   i         m   m         p   p         o   o         r   r         t   t          m   m         a   a         t   t         c   c         h   h          d   d         o   o          s   s         e   e         t   t         !   !          (   (          )   )          [   [          ]   ]          {   {          }   }          "   "          \'   \'          .   .         .   .         .   .          -   -         >   >          :   :          :   :         :   :          <   <         >   >          =   =         >   >          \\   \\          <   <         -   -         >   >          -   -         ÿÿÿÿÿÿÿÿ      0   9         0   9         ÿÿÿÿÿÿÿÿ       -   -         ÿÿÿÿÿÿÿÿ      0   9         0   9         ÿÿÿÿÿÿÿÿ      .   .         0   9         0   9         ÿÿÿÿÿÿÿÿ       a   z         A   Z         _   _         -   -         !   !         ?   ?         #   #         $   $         %   %         ^   ^         &   &         *   *         +   +         /   /         =   =         <   <         >   >         |   |         a   z         A   Z         _   _         -   -         !   !         ?   ?         #   #         $   $         %   %         ^   ^         &   &         *   *         +   +         /   /         =   =         <   <         >   >         |   |         0   9         ÿÿÿÿÿÿÿÿ     )            @)            P)            `)            )            °)            ð)            0*            *            °*            à*            +            p+            À+            à+             ,            0,            @,            P,            `,            p,            ,             ,            Ð,            ð,             -             -            @-            `-            p-             -            ð-     	       .     &       à0     !       É     X     2     f     Ö     Ô     H     r     ^     D     b     É      ¤          õ     f     b     @     8     4     0     H     \\     N     S     W     I     D     <          "     ~     $     %                                                                             >                                                                        º                                                                         V      	                                                                   V      	                                                                  V      	             	   	                                                  ô                                                                        ô                                                                        ô                  	                                                      Ð     	                                                                   Ð     	             	                                                      ­     	                                                                 ­     	                                                                 ­     	             	                                                    ·     	                                                                  n                                                        	                                                                       \n               ó                                                                       j                                                                       Ä                                                          \r               ª                                                                        ª                                                                        ª                   	                                                     ª                                                                        ²                   	                                                     ¾                   	                                                     ¦                   	                                                          \n              	                                                     Þ                                                                         Ç                                                                        Ç                                                                        Ç                                                                        Ç                                                                                                                                                                                                                                                                                                                  	                                                      {                                                                        {                                                                        {                  	                                                      !                                                                         \n                                                                       \n                                                                       \n                                                                       \n                                                                       \n                                                                        \n                                                                        \n                  	   	                                                  \n                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          à                                                                       à                                                                       Q                                                                         ^                                                                         »                                                                        »                                                                        ª                                                                         ª                                                                         [                                                         !               [                                                         !               ¾                                                         "               ¾                                                         "               ï                                                        #               ï                                                        #               þ                                                        $               þ                                                        $               ï                                                        %               ï                                                        %                                                                         &               S                                                          \'               M                                                          (               ²                                                           )               ¿     	                                                     *                                                                         +               r                                                          ,                                                                         -                                                                         .               j                                                          /                                                                          0               ¾                                                          1                                                                         2               a                                                     3               ü     \r                 	                                   4                                     	                                   5               C                                                      6                                                                        7                                                                        7                                                                        7                                 	                                       7               -                                                           8               H                                                          9               a               Ú                                                         :               Ú                                                         :               ð                                                        ;               ð                                                        ;               `                                                         <               `                                                         <                                                                        =                                                                        =               ¿                                                          >               ä                                                         ?               \n               Ð      \n                                                  @               	     \n                                                  A               Ô                                                       B               U                                                        C               ë                                                        D               \'                                                         E               7     	                                                    F               A                                                         G               Æ     \r                                                    H               	               É                                                          I                              x     \r                                                    K               £     	                                                    L               {                                                         M                    \n                                                    N               i                     	                                   N                                                                         O                                                                          P                                                                          Q                                   \r                                                    R               :     \r                                                   S               ·                                                        T               É                                                          U               ê                                                         V               ê                      	                                   V               ü                                                        W               "                                                         X                                                                                         Y               p                                                          Z                                                                        [                              î     \r                                                      \\               Ú                                                            ]               Ù                                                            ^                              Û                                                           a               2                                                         b               ¦                                                          c               )                                                         d                                                                         e                                                                        f                                                                        g               T     	                                                    h               W                                                         i                    \r                                                    j               ^                                                         k               `                                                         l                    \r                                                    m                                                                        n               *                                                         o               ³                                                          p               £                                                          q               å     \r                                                     r                                             v                                               t       s       Hq                                                ÿÿÿÿÿÿÿÿ                                                                                    ¨e                            w                                               t       x       Xq                                               ÿÿÿÿ\n                                                                                       f     0y      BÏÞ{ console.log(UTF8ToString($0)); } { alert(UTF8ToString($0)); } { const element = document.querySelector(UTF8ToString($0)); element.innerHTML = UTF8ToString($1); } { const element = document.querySelector(UTF8ToString($0)); element.textContent = UTF8ToString($1); } { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.innerHTML); } { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.textContent); }  target_features	+bulk-memory+bulk-memory-opt+call-indirect-overlong+memory64+\nmultivalue+mutable-globals+nontrapping-fptoint+reference-types+sign-ext');
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

  var ENV = {
  };
  
  var getExecutableName = () => thisProgram || './this.program';
  var getEnvStrings = () => {
      if (!getEnvStrings.strings) {
        // Default values.
        // Browser language detection #8751
        var lang = (globalThis.navigator?.language ?? 'C').replace('-', '_') + '.UTF-8';
        var env = {
          'USER': 'web_user',
          'LOGNAME': 'web_user',
          'PATH': '/',
          'PWD': '/',
          'HOME': '/home/web_user',
          'LANG': lang,
          '_': getExecutableName()
        };
        // Apply the user-provided values, if any.
        for (var x in ENV) {
          // x is a key in ENV; if ENV[x] is undefined, that means it was
          // explicitly set to be so. We allow user code to do that to
          // force variables with default values to remain unset.
          if (ENV[x] === undefined) delete env[x];
          else env[x] = ENV[x];
        }
        var strings = [];
        for (var x in env) {
          strings.push(`${x}=${env[x]}`);
        }
        getEnvStrings.strings = strings;
      }
      return getEnvStrings.strings;
    };
  
  
  function _environ_get(__environ, environ_buf) {
    __environ = bigintToI53Checked(__environ);
    environ_buf = bigintToI53Checked(environ_buf);
  
  
      var bufSize = 0;
      var envp = 0;
      for (var string of getEnvStrings()) {
        var ptr = environ_buf + bufSize;
        HEAPU64[(((__environ)+(envp))/8)] = BigInt(ptr);
        bufSize += stringToUTF8(string, ptr, Infinity) + 1;
        envp += 8;
      }
      return 0;
    ;
  }

  
  
  function _environ_sizes_get(penviron_count, penviron_buf_size) {
    penviron_count = bigintToI53Checked(penviron_count);
    penviron_buf_size = bigintToI53Checked(penviron_buf_size);
  
  
      var strings = getEnvStrings();
      HEAPU64[((penviron_count)/8)] = BigInt(strings.length);
      var bufSize = 0;
      for (var string of strings) {
        bufSize += lengthBytesUTF8(string) + 1;
      }
      HEAPU64[((penviron_buf_size)/8)] = BigInt(bufSize);
      return 0;
    ;
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
  'getExecutableName',
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
  'getEnvStrings',
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
  92048: ($0) => { console.log(UTF8ToString($0)); },  
 92083: ($0) => { alert(UTF8ToString($0)); },  
 92112: ($0, $1) => { const element = document.querySelector(UTF8ToString($0)); element.innerHTML = UTF8ToString($1); },  
 92212: ($0, $1) => { const element = document.querySelector(UTF8ToString($0)); element.textContent = UTF8ToString($1); },  
 92314: ($0) => { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.innerHTML); },  
 92419: ($0) => { const element = document.querySelector(UTF8ToString($0)); return stringToNewUTF8(element.textContent); }
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
  environ_get: _environ_get,
  /** @export */
  environ_sizes_get: _environ_sizes_get,
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

