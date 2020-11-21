/*
 *
 *	Super Pac-Man
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/super_pac-man.wasm.js';

read('superpac.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['sp1-2.1c', 'sp1-1.1b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('spc-3.1k');
	const BG = zip.decompress('sp1-6.3c');
	const OBJ = zip.decompress('spv-2.3f');
	const RGB = zip.decompress('superpac.4c');
	const BGCOLOR = zip.decompress('superpac.4e');
	const OBJCOLOR = zip.decompress('superpac.3l');
	const SND = zip.decompress('superpac.3m');
	return init(bufferSource, {SND, BG, OBJ, BGCOLOR, OBJCOLOR, RGB, PRG1, PRG2});
});
