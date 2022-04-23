/*
 *
 *	Time Pilot
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/time_pilot.wasm.js';
import {ROM} from "./dist/time_pilot_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x6000),
	PRG2: new Uint8Array(ROM.buffer, 0x6000, 0x1000),
	BG: new Uint8Array(ROM.buffer, 0x7000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0x9000, 0x4000),
	RGB_H: new Uint8Array(ROM.buffer, 0xd000, 0x20),
	RGB_L: new Uint8Array(ROM.buffer, 0xd020, 0x20),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0xd040, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0xd140, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
