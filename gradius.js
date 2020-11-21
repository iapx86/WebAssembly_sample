/*
 *
 *	Gradius
 *
 */

import {init, read} from './main.js';
import {bufferSource} from './dist/gradius.wasm.js';

read('nemesis.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = new Uint8Array(0x50000);
	zip.decompress('gradius/400-a06.15l').forEach((e, i) => PRG1[i << 1] = e);
	zip.decompress('gradius/400-a04.10l').forEach((e, i) => PRG1[1 + (i << 1)] = e);
	zip.decompress('gradius/456-a07.17l').forEach((e, i) => PRG1[0x10000 + (i << 1)] = e);
	zip.decompress('gradius/456-a05.12l').forEach((e, i) => PRG1[0x10001 + (i << 1)] = e);
	const PRG2 = zip.decompress('gradius/400-e03.5l');
	const SND = Uint8Array.concat(...['400-a01.fse', '400-a02.fse'].map(e => zip.decompress(e)));
	return init(bufferSource, {PRG1, PRG2, SND});
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
			return void instance.exports.triggerX(true);
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
			return void instance.exports.triggerB(true);
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
			return void instance.exports.triggerX(false);
		case 'Space':
		case 'KeyX':
			return void instance.exports.triggerA(false);
		case 'KeyZ':
			return void instance.exports.triggerB(false);
		}
	});
	canvas.addEventListener('click', () => void instance.exports.coin());
});
