/*
 *
 *	Pengo
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/pengo.wasm.js';
import {ROM} from "./dist/pengo_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	BG: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0xa000, 0x2000),
	RGB: new Uint8Array(ROM.buffer, 0xc000, 0x20),
	COLOR: new Uint8Array(ROM.buffer, 0xc020, 0x400),
	SND: new Uint8Array(ROM.buffer, 0xc420, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
