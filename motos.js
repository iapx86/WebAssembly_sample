/*
 *
 *	Motos
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/motos.wasm.js';

read('motos.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['mo1_3.1d', 'mo1_1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('mo1_4.1k');
	const BG = zip.decompress('mo1_5.3b');
	const OBJ = Uint8Array.concat(...['mo1_7.3n', 'mo1_6.3m'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('mo1-5.5b');
	const BGCOLOR = zip.decompress('mo1-6.4c');
	const OBJCOLOR = zip.decompress('mo1-7.5k');
	const SND = zip.decompress('mo1-3.3m');
	return init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2});
});
