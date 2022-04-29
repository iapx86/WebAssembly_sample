/*
 *
 *	Star Force
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/star_force.wasm.js';
import {ROM} from "./dist/star_force.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	PRG2: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	FG: new Uint8Array(ROM.buffer, 0xa000, 0x3000),
	BG1: new Uint8Array(ROM.buffer, 0xd000, 0x6000),
	BG2: new Uint8Array(ROM.buffer, 0x13000, 0x6000),
	BG3: new Uint8Array(ROM.buffer, 0x19000, 0x3000),
	OBJ: new Uint8Array(ROM.buffer, 0x1c000, 0xc000),
	SND: new Uint8Array(ROM.buffer, 0x28000, 0x20),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
