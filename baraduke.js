/*
 *
 *	Baraduke
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/baraduke.wasm.js';
import {ROM} from "./dist/baraduke.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x4000),
	PRG2I: new Uint8Array(ROM.buffer, 0xe000, 0x1000),
	FG: new Uint8Array(ROM.buffer, 0xf000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x11000, 0xc000),
	OBJ: new Uint8Array(ROM.buffer, 0x1d000, 0x10000),
	GREEN: new Uint8Array(ROM.buffer, 0x2d000, 0x800),
	RED: new Uint8Array(ROM.buffer, 0x2d800, 0x800),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
