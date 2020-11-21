/*
 *
 *	Vulgus
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/vulgus.wasm.js';

read('vulgus.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['vulgus.002', 'vulgus.003', 'vulgus.004', 'vulgus.005', '1-8n.bin'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('1-11c.bin');
	const FG = zip.decompress('1-3d.bin');
	const BG = Uint8Array.concat(...['2-2a.bin', '2-3a.bin', '2-4a.bin', '2-5a.bin', '2-6a.bin', '2-7a.bin'].map(e => zip.decompress(e)));
	const OBJ = Uint8Array.concat(...['2-2n.bin', '2-3n.bin', '2-4n.bin', '2-5n.bin'].map(e => zip.decompress(e)));
	const RED = zip.decompress('e8.bin');
	const GREEN = zip.decompress('e9.bin');
	const BLUE = zip.decompress('e10.bin');
	const FGCOLOR = zip.decompress('d1.bin');
	const BGCOLOR = zip.decompress('c9.bin');
	const OBJCOLOR = zip.decompress('j2.bin');
	return init(bufferSource, {PRG1, PRG2, FG, BG, OBJ, RED, GREEN, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR});
});
