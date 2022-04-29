/*
 *
 *	Elevator Action
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/elevator_action.wasm.js';
import {ROM} from "./dist/elevator_action.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x8000),
	PRG2: new Uint8Array(ROM.buffer, 0x8000, 0x2000),
	PRG3: new Uint8Array(ROM.buffer, 0xa000, 0x800),
	GFX: new Uint8Array(ROM.buffer, 0xa800, 0x8000),
	PRI: new Uint8Array(ROM.buffer, 0x12800, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
