/*
 *
 *	TwinBee
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/twinbee.wasm.js';
import {ROM} from "./dist/twinbee_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x50000),
	PRG2: new Uint8Array(ROM.buffer, 0x50000, 0x2000),
	SND: new Uint8Array(ROM.buffer, 0x52000, 0x200),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
