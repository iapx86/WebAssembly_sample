/*
 *
 *	Toypop
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/toypop.wasm.js';

read('toypop.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['tp1-2.5b', 'tp1-1.5c'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('tp1-3.2c');
	const PRG3 = new Uint8Array(0x8000);
	zip.decompress('tp1-4.8c').forEach((e, i) => PRG3[i << 1] = e);
	zip.decompress('tp1-5.10c').forEach((e, i) => PRG3[1 | i << 1] = e);
	const BG = zip.decompress('tp1-7.5p');
	const OBJ = zip.decompress('tp1-6.9t');
	const RED = zip.decompress('tp1-3.1r');
	const GREEN = zip.decompress('tp1-2.1s');
	const BLUE = zip.decompress('tp1-1.1t');
	const BGCOLOR = zip.decompress('tp1-4.5l');
	const OBJCOLOR = zip.decompress('tp1-5.2p');
	const SND = zip.decompress('tp1-6.3d');
	const bufferSource = new Zlib.Unzip(archive).decompress('toypop.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG3, BG, OBJ, RED, GREEN, BLUE, BGCOLOR, OBJCOLOR, SND});
});
