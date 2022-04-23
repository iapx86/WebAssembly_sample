/*
 *
 *	Phozon
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/phozon.wasm.js';
import {ROM} from "./dist/phozon_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	PRG2: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	PRG3: new Uint8Array(ROM.buffer, 0xa000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0xc000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0xe000, 0x2000),
	RED: new Uint8Array(ROM.buffer, 0x10000, 0x100),
	GREEN: new Uint8Array(ROM.buffer, 0x10100, 0x100),
	BLUE: new Uint8Array(ROM.buffer, 0x10200, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x10300, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x10400, 0x100),
	SND: new Uint8Array(ROM.buffer, 0x10500, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
