/*
 *
 *	Mappy
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/mappy.wasm.js';
import {ROM} from "./dist/mappy.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x6000),
	PRG2: new Uint8Array(ROM.buffer, 0x6000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x8000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0x9000, 0x4000),
	RGB: new Uint8Array(ROM.buffer, 0xd000, 0x20),
	BGCOLOR: new Uint8Array(ROM.buffer, 0xd020, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0xd120, 0x100),
	SND: new Uint8Array(ROM.buffer, 0xd220, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
