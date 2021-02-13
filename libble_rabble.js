/*
 *
 *	Libble Rabble
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/libble_rabble.wasm.js';

read('liblrabl.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['5b.rom', '5c.rom'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('2c.rom');
	const PRG3 = new Uint8Array(0x8000);
	zip.decompress('8c.rom').forEach((e, i) => PRG3[i << 1] = e);
	zip.decompress('10c.rom').forEach((e, i) => PRG3[1 | i << 1] = e);
	const BG = zip.decompress('5p.rom');
	const OBJ = zip.decompress('9t.rom');
	const RED = zip.decompress('lr1-3.1r');
	const GREEN = zip.decompress('lr1-2.1s');
	const BLUE = zip.decompress('lr1-1.1t');
	const BGCOLOR = zip.decompress('lr1-5.5l');
	const OBJCOLOR = zip.decompress('lr1-6.2p');
	const SND = zip.decompress('lr1-4.3d');
	const bufferSource = new Zlib.Unzip(archive).decompress('libble_rabble.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG3, BG, OBJ, RED, GREEN, BLUE, BGCOLOR, OBJCOLOR, SND});
});
