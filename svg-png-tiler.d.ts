type PngCallback = (png: Int8Array) => void;
type OnProgressCallback = (progressPct: number) => void;
interface Options {
  maxTilePixels?: number; // Maximum tile area in pixels (e.g. 1920 x 1080 or 3840 x 2160)
  onProgress: OnProgressCallback; // callback fired on every tile loaded and rendered
}
interface Module {
  svg2png(svg: string, options: Options, cb: PngCallback);
}

export { PngCallback, Module };
