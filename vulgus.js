/*
 *
 *	Vulgus
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/vulgus.wasm.js';
import {ROM} from "./dist/vulgus_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x2000),
	FG: new Uint8Array(ROM.buffer, 0xc000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0xe000, 0xc000),
	OBJ: new Uint8Array(ROM.buffer, 0x1a000, 0x8000),
	RED: new Uint8Array(ROM.buffer, 0x22000, 0x100),
	GREEN: new Uint8Array(ROM.buffer, 0x22100, 0x100),
	BLUE: new Uint8Array(ROM.buffer, 0x22200, 0x100),
	FGCOLOR: new Uint8Array(ROM.buffer, 0x22300, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x22400, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x22500, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
