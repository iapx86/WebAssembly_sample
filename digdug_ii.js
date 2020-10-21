/*
 *
 *	DigDug II
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/digdug_ii.wasm.js';

read('digdug2.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['d23_3.1d', 'd23_1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('d21_4.1k');
	const BG = zip.decompress('d21_5.3b');
	const OBJ = Uint8Array.concat(...['d21_7.3n', 'd21_6.3m'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('d21-5.5b');
	const BGCOLOR = zip.decompress('d21-6.4c');
	const OBJCOLOR = zip.decompress('d21-7.5k');
	const SND = zip.decompress('d21-3.3m');
	init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2}).then();
});
