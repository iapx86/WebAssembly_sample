/*
 *
 *	Sky Kid
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/sky_kid.wasm.js';
import {ROM} from "./dist/sky_kid.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xc000),
	PRG2: new Uint8Array(ROM.buffer, 0xc000, 0x2000),
	PRG2I: new Uint8Array(ROM.buffer, 0xe000, 0x1000),
	FG: new Uint8Array(ROM.buffer, 0xf000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x11000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0x13000, 0x8000),
	RED: new Uint8Array(ROM.buffer, 0x1b000, 0x100),
	GREEN: new Uint8Array(ROM.buffer, 0x1b100, 0x100),
	BLUE: new Uint8Array(ROM.buffer, 0x1b200, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x1b300, 0x200),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x1b500, 0x200),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
