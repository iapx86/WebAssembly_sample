/*
 *
 *	Strategy X
 *
 */

import {init, read} from './main.js';
import {archive} from './dist/strategy_x.wasm.js';

read('stratgyx.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['2c_1.bin', '2e_2.bin', '2f_3.bin', '2h_4.bin', '2j_5.bin', '2l_6.bin'].map(e => zip.decompress(e)));
	const PRG2 = Uint8Array.concat(...['s1.bin', 's2.bin'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['5f_c2.bin', '5h_c1.bin'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('strategy.6e');
	const MAP = zip.decompress('strategy.10k');
	const bufferSource = new Zlib.Unzip(archive).decompress('strategy_x.wasm');
	return init(bufferSource, {BG, RGB, PRG1, PRG2, MAP});
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
});
