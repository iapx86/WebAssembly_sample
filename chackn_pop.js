/*
 *
 *	Chack'n Pop
 *
 */

import {init, expand} from './main.js';
import {imageSource, imageSource_size} from './dist/chackn_pop.wasm.js';
import {ROM} from "./dist/chackn_pop.png.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0xa000),
	PRG2: new Uint8Array(ROM.buffer, 0xa000, 0x800),
	OBJ: new Uint8Array(ROM.buffer, 0xa800, 0x4000),
	BG: new Uint8Array(ROM.buffer, 0xe800, 0x4000),
	RGB_L: new Uint8Array(ROM.buffer, 0x12800, 0x400),
	RGB_H: new Uint8Array(ROM.buffer, 0x12c00, 0x400),
}).then(() => expand(imageSource, imageSource_size)).then(buf => init(buf, roms)).then(game => {
	document.addEventListener('keydown', e => {
		if (e.repeat)
			return;
		switch (e.code) {
		case 'ArrowLeft':
			return game.left(true);
		case 'ArrowUp':
			return game.up(true);
		case 'ArrowRight':
			return game.right(true);
		case 'ArrowDown':
			return game.down(true);
		case 'Digit0':
			return game.coin(true);
		case 'Digit1':
			return game.start1P(true);
		case 'Digit2':
			return game.start2P(true);
		case 'KeyM': // MUTE
			return void(audioCtx.state === 'suspended' ? audioCtx.resume().catch() : audioCtx.state === 'running' && audioCtx.suspend().catch());
		case 'KeyR':
			return game.reset();
		case 'Space':
		case 'KeyX':
			return game.triggerB(true);
		case 'KeyZ':
			return game.triggerA(true);
		}
	});
	document.addEventListener('keyup', e => {
		switch (e.code) {
		case 'ArrowLeft':
			return game.left(false);
		case 'ArrowUp':
			return game.up(false);
		case 'ArrowRight':
			return game.right(false);
		case 'ArrowDown':
			return game.down(false);
		case 'Space':
		case 'KeyX':
			return game.triggerB(false);
		case 'KeyZ':
			return game.triggerA(false);
		}
	});
	canvas.addEventListener('click', () => game.coin(true));
}));
