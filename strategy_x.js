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
}).then(instance => {
	document.addEventListener('keydown', e => {
		if (e.repeat)
			return;
		switch (e.code) {
		case 'ArrowLeft':
			return void instance.exports.left(true);
		case 'ArrowUp':
			return void instance.exports.up(true);
		case 'ArrowRight':
			return void instance.exports.right(true);
		case 'ArrowDown':
			return void instance.exports.down(true);
		case 'Digit0':
			return void instance.exports.coin();
		case 'Digit1':
			return void instance.exports.start1P();
		case 'Digit2':
			return void instance.exports.start2P();
		case 'KeyC':
			return void instance.exports.triggerB(true);
		case 'KeyM': // MUTE
			if (audioCtx.state === 'suspended')
				audioCtx.resume().catch();
			else if (audioCtx.state === 'running')
				audioCtx.suspend().catch();
			return;
		case 'KeyR':
			return void instance.exports.reset();
		case 'Space':
		case 'KeyX':
			return void instance.exports.triggerA(true);
		case 'KeyZ':
			return void instance.exports.triggerX(true);
		}
	});
	document.addEventListener('keyup', e => {
		switch (e.code) {
		case 'ArrowLeft':
			return void instance.exports.left(false);
		case 'ArrowUp':
			return void instance.exports.up(false);
		case 'ArrowRight':
			return void instance.exports.right(false);
		case 'ArrowDown':
			return void instance.exports.down(false);
		case 'KeyC':
			return void instance.exports.triggerB(false);
		case 'Space':
		case 'KeyX':
			return void instance.exports.triggerA(false);
		case 'KeyZ':
			return void instance.exports.triggerX(false);
		}
	});
	canvas.addEventListener('click', () => void instance.exports.coin());
});
