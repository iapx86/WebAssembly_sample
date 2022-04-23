/*
 *
 *	DigDug II
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/digdug_ii.wasm.js';
import {ROM} from "./dist/digdug_ii_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	PRG2: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0xa000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0xb000, 0x8000),
	RGB: new Uint8Array(ROM.buffer, 0x13000, 0x20),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x13020, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x13120, 0x100),
	SND: new Uint8Array(ROM.buffer, 0x13220, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
