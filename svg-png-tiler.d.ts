
type PngCallback = (png: Int8Array) => void;
interface Module {
    svg2png(svg: string, cb: PngCallback);
}

export {
    PngCallback,
    Module
}