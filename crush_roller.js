/*
 *
 *	Crush Roller
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/crush_roller.wasm.js';

read('crush.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG = Uint8Array.concat(...['crushkrl.6e', 'crushkrl.6f', 'crushkrl.6h', 'crushkrl.6j'].map(e => zip.decompress(e)));
	const BG = zip.decompress('maketrax.5e');
	const OBJ = zip.decompress('maketrax.5f');
	const RGB = zip.decompress('82s123.7f');
	const COLOR = zip.decompress('2s140.4a');
	const SND = zip.decompress('82s126.1m');
	return init(bufferSource, {BG, COLOR, OBJ, RGB, PRG, SND});
});
