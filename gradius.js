/*
 *
 *	Gradius
 *
 */

import {init, expand} from './main.js';
import {imageSource, imageSource_size} from './dist/gradius.wasm.js';
import {ROM} from "./dist/gradius_rom.js";
let roms;

window.addEventListener('load', () => expand(ROM).then(ROM => roms = {
	PRG1: new Uint8Array(ROM.buffer, 0x0, 0x50000),
	PRG2: new Uint8Array(ROM.buffer, 0x50000, 0x2000),
	SND: new Uint8Array(ROM.buffer, 0x52000, 0x200),
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
			return game.triggerX(true);
		case 'KeyM': // MUTE
			return void(audioCtx.state === 'suspended' ? audioCtx.resume().catch() : audioCtx.state === 'running' && audioCtx.suspend().catch());
		case 'KeyR':
			return game.reset();
		case 'Space':
		case 'KeyX':
			return game.triggerA(true);
		case 'KeyZ':
			return game.triggerB(true);
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
			return game.triggerX(false);
		case 'Space':
		case 'KeyX':
			return game.triggerA(false);
		case 'KeyZ':
			return game.triggerB(false);
		}
	});
	canvas.addEventListener('click', () => game.coin(true));
}));
