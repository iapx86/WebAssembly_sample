/*
 *
 *	Pac & Pal
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/pac_and_pal.wasm.js';

read('pacnpal.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['pap1-3b.1d', 'pap1-2b.1c', 'pap3-1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('pap1-4.1k');
	const BG = zip.decompress('pap1-6.3c');
	const OBJ = zip.decompress('pap1-5.3f');
	const RGB = zip.decompress('pap1-6.4c');
	const BGCOLOR = zip.decompress('pap1-5.4e');
	const OBJCOLOR = zip.decompress('pap1-4.3l');
	const SND = zip.decompress('pap1-3.3m');
	const bufferSource = new Zlib.Unzip(archive).decompress('pac_and_pal.wasm');
	return init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2});
});
