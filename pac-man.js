/*
 *
 *	Pac-Man
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/pac-man.wasm.js';
import {ROM} from "./dist/pac-man_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG: new Uint8Array(ROM.buffer, 0x0, 0x4000),
	BG: new Uint8Array(ROM.buffer, 0x4000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0x5000, 0x1000),
	RGB: new Uint8Array(ROM.buffer, 0x6000, 0x20),
	COLOR: new Uint8Array(ROM.buffer, 0x6020, 0x100),
	SND: new Uint8Array(ROM.buffer, 0x6120, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
