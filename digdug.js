/*
 *
 *	DigDug
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/digdug.wasm.js';
import {ROM} from "./dist/digdug_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x4000),
	PRG2: new Uint8Array(ROM.buffer, 0x4000, 0x2000),
	PRG3: new Uint8Array(ROM.buffer, 0x6000, 0x1000),
	BG2: new Uint8Array(ROM.buffer, 0x7000, 0x800),
	OBJ: new Uint8Array(ROM.buffer, 0x7800, 0x4000),
	BG4: new Uint8Array(ROM.buffer, 0xb800, 0x1000),
	MAPDATA: new Uint8Array(ROM.buffer, 0xc800, 0x1000),
	RGB: new Uint8Array(ROM.buffer, 0xd800, 0x20),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0xd820, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0xd920, 0x100),
	SND: new Uint8Array(ROM.buffer, 0xda20, 0x100),
	IO: new Uint8Array(ROM.buffer, 0xdb20, 0x400),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
