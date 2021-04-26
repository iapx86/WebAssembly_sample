/*
 *
 *	Gradius
 *
 */

import {init, read} from './main.js';
import {archive} from './dist/gradius.wasm.js';

read('nemesis.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = new Uint8Array(0x50000);
	zip.decompress('gradius/400-a06.15l').forEach((e, i) => PRG1[i << 1] = e);
	zip.decompress('gradius/400-a04.10l').forEach((e, i) => PRG1[1 + (i << 1)] = e);
	zip.decompress('gradius/456-a07.17l').forEach((e, i) => PRG1[0x10000 + (i << 1)] = e);
	zip.decompress('gradius/456-a05.12l').forEach((e, i) => PRG1[0x10001 + (i << 1)] = e);
	const PRG2 = zip.decompress('gradius/400-e03.5l');
	const SND = Uint8Array.concat(...['400-a01.fse', '400-a02.fse'].map(e => zip.decompress(e)));
	const bufferSource = new Zlib.Unzip(archive).decompress('gradius.wasm');
	return init(bufferSource, {PRG1, PRG2, SND});
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
});
