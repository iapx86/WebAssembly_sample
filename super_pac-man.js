/*
 *
 *	Super Pac-Man
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/super_pac-man.wasm.js';
import {ROM} from "./dist/super_pac-man.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x4000),
	PRG2: new Uint8Array(ROM.buffer, 0x4000, 0x1000),
	BG: new Uint8Array(ROM.buffer, 0x5000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0x6000, 0x2000),
	RGB: new Uint8Array(ROM.buffer, 0x8000, 0x20),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x8020, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x8120, 0x100),
	SND: new Uint8Array(ROM.buffer, 0x8220, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
