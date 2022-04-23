/*
 *
 *	Frogger
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/frogger.wasm.js';
import {ROM} from "./dist/frogger_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x3000),
	PRG2: new Uint8Array(ROM.buffer, 0x3000, 0x1800),
	BG: new Uint8Array(ROM.buffer, 0x4800, 0x1000),
	RGB: new Uint8Array(ROM.buffer, 0x5800, 0x20),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
