/*
 *
 *	Mappy
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/mappy.wasm.js';

read('mappy.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['mappyj/mp1_3.1d', 'mp1_2.1c', 'mappyj/mp1_1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('mp1_4.1k');
	const BG = zip.decompress('mp1_5.3b');
	const OBJ = Uint8Array.concat(...['mp1_7.3n', 'mp1_6.3m'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('mp1-5.5b');
	const BGCOLOR = zip.decompress('mp1-6.4c');
	const OBJCOLOR = zip.decompress('mp1-7.5k');
	const SND = zip.decompress('mp1-3.3m');
	const bufferSource = new Zlib.Unzip(archive).decompress('mappy.wasm');
	return init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2});
});
