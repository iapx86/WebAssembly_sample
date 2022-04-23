/*
 *
 *	Sea Fighter Poseidon
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/sea_fighter_poseidon.wasm.js';
import {ROM} from "./dist/sea_fighter_poseidon_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x2000),
	PRG3: new Uint8Array(ROM.buffer, 0xc000, 0x800),
	GFX: new Uint8Array(ROM.buffer, 0xc800, 0x8000),
	PRI: new Uint8Array(ROM.buffer, 0x14800, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
