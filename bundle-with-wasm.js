const moduleName = 'svg-png-tiler';
const fs = require('fs');
const contents = fs.readFileSync(`${moduleName}.wasm`, { encoding: 'base64' });
let js = fs.readFileSync(`${moduleName}.js`, 'utf8')
const newBundle = js.replace('var wasmBlobStr=null;', 'var wasmBlobStr="' + contents + '";');
fs.writeFileSync(`${moduleName}.all.js`, newBundle, 'utf8');