/*
 *
 *	1942
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/1942.wasm.js';
import {ROM} from "./dist/1942.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x18000),
	PRG2: new Uint8Array(ROM.buffer, 0x18000, 0x4000),
	FG: new Uint8Array(ROM.buffer, 0x1c000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x1e000, 0xc000),
	OBJ: new Uint8Array(ROM.buffer, 0x2a000, 0x10000),
	RED: new Uint8Array(ROM.buffer, 0x3a000, 0x100),
	GREEN: new Uint8Array(ROM.buffer, 0x3a100, 0x100),
	BLUE: new Uint8Array(ROM.buffer, 0x3a200, 0x100),
	FGCOLOR: new Uint8Array(ROM.buffer, 0x3a300, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x3a400, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x3a500, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
