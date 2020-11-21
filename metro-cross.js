/*
 *
 *	Metro-Cross
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/metro-cross.wasm.js';

read('metrocrs.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['mc1-3.9c', 'mc1-1.9a', 'mc1-2.9b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('mc1-4.3b');
	const PRG2I = zip.decompress('cus60-60a1.mcu');
	const FG = zip.decompress('mc1-5.3j');
	const BG = Uint8Array.concat(...['mc1-7.4p', 'mc1-6.4n'].map(e => zip.decompress(e)), new Uint8Array(0x4000).fill(0xff));
	const OBJ = Uint8Array.concat(...['mc1-8.8k', 'mc1-9.8l'].map(e => zip.decompress(e)));
	const GREEN = zip.decompress('mc1-1.1n');
	const RED = zip.decompress('mc1-2.2m');
	return init(bufferSource, {PRG1, PRG2, PRG2I, FG, BG, OBJ, GREEN, RED});
});
