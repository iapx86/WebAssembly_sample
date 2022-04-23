/*
 *
 *	Strategy X
 *
 */

import {init, expand} from './main.js';
import {imageSource, imageSource_size} from './dist/strategy_x.wasm.js';
import {ROM} from "./dist/strategy_x_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x6000),
	PRG2: new Uint8Array(ROM.buffer, 0x6000, 0x2000),
	BG: new Uint8Array(ROM.buffer, 0x8000, 0x1000),
	RGB: new Uint8Array(ROM.buffer, 0x9000, 0x20),
	MAP: new Uint8Array(ROM.buffer, 0x9020, 0x20),
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
		case 'KeyC':
			return game.triggerB(true);
		case 'KeyM': // MUTE
			return void(audioCtx.state === 'suspended' ? audioCtx.resume().catch() : audioCtx.state === 'running' && audioCtx.suspend().catch());
		case 'KeyR':
			return game.reset();
		case 'Space':
		case 'KeyX':
			return game.triggerA(true);
		case 'KeyZ':
			return game.triggerX(true);
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
		case 'KeyC':
			return game.triggerB(false);
		case 'Space':
		case 'KeyX':
			return game.triggerA(false);
		case 'KeyZ':
			return game.triggerX(false);
		}
	});
	canvas.addEventListener('click', () => game.coin(true));
}));
