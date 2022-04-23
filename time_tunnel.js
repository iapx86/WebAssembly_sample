/*
 *
 *	Time Tunnel
 *
 */

import {init, expand} from './default_main.js';
import {imageSource, imageSource_size} from './dist/time_tunnel.wasm.js';
import {ROM} from "./dist/time_tunnel_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x1000),
	GFX: new Uint8Array(ROM.buffer, 0xb000, 0x8000),
	PRI: new Uint8Array(ROM.buffer, 0x13000, 0x100),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)));
