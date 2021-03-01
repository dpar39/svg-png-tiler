var clpPromise = new Promise(function (resolve) {
  var wasmBlobStr = null;
  if (wasmBlobStr) {
    if (typeof atob === "function")
      Module['wasmBinary'] = Uint8Array.from(atob(wasmBlobStr), c => c.charCodeAt(0));
    else
      Module['wasmBinary'] = Buffer.from(wasmBlobStr, 'base64');
  }
  Module["preInit"] = [
    function () {
      FS.mkdir("/out");
      FS.mount(MEMFS, { root: "." }, "/out");
    },
  ];

  Module["onRuntimeInitialized"] = function () {
    var m = this;
    this["svg2png"] = (function () {
      var methods = {
      };
      var pub = {};
      Object.assign(pub, m, methods);
      return Object.freeze(pub);
    })();
    resolve(this["svg2png"]);
  };
//});
