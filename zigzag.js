/*
 *
 *	Zig Zag
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/zigzag.wasm.js';

read('zigzagb.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG = Uint8Array.concat(...['zz_d1.7l', 'zz_d2.7k', 'zz_d4.7f', 'zz_d3.7h'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['zz_6.1h', 'zz_5.1k'].map(e => zip.decompress(e).subarray(0, 0x800)));
	const OBJ = Uint8Array.concat(...['zz_6.1h', 'zz_5.1k'].map(e => zip.decompress(e).subarray(0x800)));
	const RGB = zip.decompress('zzbpr_e9.bin');
	const bufferSource = new Zlib.Unzip(archive).decompress('zigzag.wasm');
	return init(bufferSource, {BG, OBJ, RGB, PRG});
});
