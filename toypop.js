/*
 *
 *	Toypop
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/toypop.wasm.js';
import {ROM} from "./dist/toypop.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	PRG2: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	PRG3: new Uint8Array(ROM.buffer, 0xa000, 0x8000),
	BG: new Uint8Array(ROM.buffer, 0x12000, 0x2000),
	OBJ: new Uint8Array(ROM.buffer, 0x14000, 0x4000),
	RED: new Uint8Array(ROM.buffer, 0x18000, 0x100),
	GREEN: new Uint8Array(ROM.buffer, 0x18100, 0x100),
	BLUE: new Uint8Array(ROM.buffer, 0x18200, 0x100),
	BGCOLOR: new Uint8Array(ROM.buffer, 0x18300, 0x100),
	OBJCOLOR: new Uint8Array(ROM.buffer, 0x18400, 0x200),
	SND: new Uint8Array(ROM.buffer, 0x18600, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
