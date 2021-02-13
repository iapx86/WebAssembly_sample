/*
 *
 *	The Tower of Druaga
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/the_tower_of_druaga.wasm.js';

read('todruaga.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['td2_3.1d', 'td2_1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('td1_4.1k');
	const BG = zip.decompress('td1_5.3b');
	const OBJ = Uint8Array.concat(...['td1_7.3n', 'td1_6.3m'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('td1-5.5b');
	const BGCOLOR = zip.decompress('td1-6.4c');
	const OBJCOLOR = zip.decompress('td1-7.5k');
	const SND = zip.decompress('td1-3.3m');
	const bufferSource = new Zlib.Unzip(archive).decompress('the_tower_of_druaga.wasm');
	return init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2});
});
