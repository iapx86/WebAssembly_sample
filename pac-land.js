/*
 *
 *	Pac-Land
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/pac-land.wasm.js';
import {ROM} from "./dist/pac-land_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x18000),
	PRG2: new Uint8Array(ROM.buffer, 0x18000, 0x2000),
	PRG2I: new Uint8Array(ROM.buffer, 0x1a000, 0x1000),
	FG: new Uint8Array(ROM.buffer, 0x1b000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x1d000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0x1f000, 0x10000),
	RED: new Uint8Array(ROM.buffer, 0x2f000, 0x400),
	BLUE: new Uint8Array(ROM.buffer, 0x2f400, 0x400),
	FGCOLOR: new Uint8Array(ROM.buffer, 0x2f800, 0x400),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x2fc00, 0x400),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x30000, 0x400),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
