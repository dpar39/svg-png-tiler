# SVG-PNG-Tiler

[Live Demo Here](https://dpar39.github.io/svg-png-tiler/)

This library allows you to convert very large scalable vector graphics (SVG) to PNG images in the browser, overcoming current size limits in HTML5 `canvas` and `img` elements. For instance, currently Desktop Chrome's `canvas` has a maximum height/width of 65535 pixels and a total area of 16,384 x 16,384 pixels. In addition, when rasterizing SVG into a `canvas` element, you need to use HTML `<img>` tags, which can only load a maximum of 16384 pixels in any dimension.

This library uses a tiling approach that renders tiles of the SVG moving the viewBox while encoding generated image strips with a PNG encoder simultaneously. All the work is done in WebAssembly. This way, you can control the use of memory at the expense of multiple iterations of rasterizations of same SVG with different view boxes. If your application generates large SVG dimensions, e.g., a large CAD diagram and you'd like to export it to a PNG image, this library will allow you to do os in the browser.

## Installing

```bash
npm install svg-png-tiler
```

## Usage

```javascript
Module.onRuntimeInitialized = () => {
  const svgString = `<svg width="10000" height="80000" ... > ... </svg>`;
  Module.svg2png(svgString, (png) => {
    // png is a byte array representing the PNG encoded image to export/save
  });
};
```
