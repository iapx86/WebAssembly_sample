/*
 *
 *	Grobda
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/grobda.wasm.js';

read('grobda.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['gr2-3.1d', 'gr2-2.1c', 'gr2-1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('gr1-4.1k');
	const BG = zip.decompress('gr1-7.3c');
	const OBJ = Uint8Array.concat(...['gr1-5.3f', 'gr1-6.3e'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('gr1-6.4c');
	const BGCOLOR = zip.decompress('gr1-5.4e');
	const OBJCOLOR = zip.decompress('gr1-4.3l');
	const SND = zip.decompress('gr1-3.3m');
	init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2}).then();
});
