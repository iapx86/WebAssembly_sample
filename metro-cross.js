/*
 *
 *	Metro-Cross
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/metro-cross.wasm.js';
import {ROM} from "./dist/metro-cross.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x2000),
	PRG2I: new Uint8Array(ROM.buffer, 0xc000, 0x1000),
	FG: new Uint8Array(ROM.buffer, 0xd000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0xf000, 0xc000),
	OBJ: new Uint8Array(ROM.buffer, 0x1b000, 0x8000),
	GREEN: new Uint8Array(ROM.buffer, 0x23000, 0x800),
	RED: new Uint8Array(ROM.buffer, 0x23800, 0x800),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
