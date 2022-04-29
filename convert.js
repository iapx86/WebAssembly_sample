const fs = require('fs');
const {createCanvas, createImageData} = require('canvas');

const pngString = array => {
	const w = 1024, h = Math.ceil(array.byteLength / w), canvas = createCanvas(w, h);
	const palette = new Uint8ClampedArray(new Uint32Array(256).map((e, i) => i | 0xff000000).buffer);
	canvas.getContext('2d', {pixelFormat: 'A8'}).putImageData(createImageData(new Uint8ClampedArray(array.buffer), w, h), 0, 0);
	return Buffer.from(canvas.toBuffer('image/png', {compressionLevel: 9, filters: canvas.PNG_FILTER_NONE, palette})).toString('base64');
};

const wasm = fs.readFileSync(process.argv[2]);
const str = `export const imageSource = 'data:image/png;base64,${pngString(wasm)}';\nexport const imageSource_size = ${wasm.length};\n`;
fs.writeFileSync(process.argv[3], str);
