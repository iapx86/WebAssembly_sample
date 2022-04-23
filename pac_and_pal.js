/*
 *
 *	Pac & Pal
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/pac_and_pal.wasm.js';
import {ROM} from "./dist/pac_and_pal_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x6000),
	PRG2: new Uint8Array(ROM.buffer, 0x6000, 0x1000),
	BG: new Uint8Array(ROM.buffer, 0x7000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	RGB: new Uint8Array(ROM.buffer, 0xa000, 0x20),
	BGCOLOR: new Uint8Array(ROM.buffer, 0xa020, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0xa120, 0x100),
	SND: new Uint8Array(ROM.buffer, 0xa220, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
