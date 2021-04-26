/*
 *
 *	Chack'n Pop
 *
 */

import {init, read} from './main.js';
import {archive} from './dist/chackn_pop.wasm.js';

read('chaknpop.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['ao4_01.ic28', 'ao4_02.ic27', 'ao4_03.ic26', 'ao4_04.ic25', 'ao4_05.ic3'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('ao4_06.ic23');
	const OBJ = Uint8Array.concat(...['ao4_08.ic14', 'ao4_07.ic15'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['ao4_09.ic98', 'ao4_10.ic97'].map(e => zip.decompress(e)));
	const RGB_L = zip.decompress('ao4-11.ic96');
	const RGB_H = zip.decompress('ao4-12.ic95');
	const bufferSource = new Zlib.Unzip(archive).decompress('chackn_pop.wasm');
	return init(bufferSource, {PRG1, PRG2, OBJ, BG, RGB_L, RGB_H});
}).then(game => {
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
});
