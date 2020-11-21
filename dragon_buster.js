/*
 *
 *	Dragon Buster
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/dragon_buster.wasm.js';

read('drgnbstr.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['db1_2b.6c', 'db1_1.6b', 'db1_3.6d'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('db1_4.3c');
	const PRG2I = zip.decompress('cus60-60a1.mcu');
	const FG = zip.decompress('db1_6.6l');
	const BG = zip.decompress('db1_5.7e');
	const OBJ = Uint8Array.concat(...['db1_8.10n', 'db1_7.10m'].map(e => zip.decompress(e)));
	const RED = zip.decompress('db1-1.2n');
	const GREEN = zip.decompress('db1-2.2p');
	const BLUE = zip.decompress('db1-3.2r');
	const BGCOLOR = zip.decompress('db1-4.5n');
	const OBJCOLOR = zip.decompress('db1-5.6n');
	return init(bufferSource, {PRG1, PRG2, PRG2I, FG, BG, OBJ, RED, GREEN, BLUE, BGCOLOR, OBJCOLOR});
});
