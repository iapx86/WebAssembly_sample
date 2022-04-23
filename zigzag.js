/*
 *
 *	Zig Zag
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/zigzag.wasm.js';
import {ROM} from "./dist/zigzag_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG: new Uint8Array(ROM.buffer, 0x0, 0x4000),
	BG: new Uint8Array(ROM.buffer, 0x4000, 0x1000),
	OBJ: new Uint8Array(ROM.buffer, 0x5000, 0x1000),
	RGB: new Uint8Array(ROM.buffer, 0x6000, 0x20),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
