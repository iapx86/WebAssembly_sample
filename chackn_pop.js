/*
 *
 *	Chack'n Pop
 *
 */

import {init, read} from './main.js';
import {bufferSource} from './dist/chackn_pop.wasm.js';

read('chaknpop.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['ao4_01.ic28', 'ao4_02.ic27', 'ao4_03.ic26', 'ao4_04.ic25', 'ao4_05.ic3'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('ao4_06.ic23');
	const OBJ = Uint8Array.concat(...['ao4_08.ic14', 'ao4_07.ic15'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['ao4_09.ic98', 'ao4_10.ic97'].map(e => zip.decompress(e)));
	const RGB_L = zip.decompress('ao4-11.ic96');
	const RGB_H = zip.decompress('ao4-12.ic95');
	init(bufferSource, {PRG1, PRG2, OBJ, BG, RGB_L, RGB_H}).then(instance => {
		document.addEventListener('keydown', e => {
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
				return void instance.exports.triggerB(true);
			case 'KeyZ':
				return void instance.exports.triggerA(true);
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
			case 'Space':
			case 'KeyX':
				return void instance.exports.triggerB(false);
			case 'KeyZ':
				return void instance.exports.triggerA(false);
			}
		});
		canvas.addEventListener('click', () => void instance.exports.coin());
	});
});
