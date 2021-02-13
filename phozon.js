/*
 *
 *	Phozon
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/phozon.wasm.js';

read('phozon.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['6e.rom', '6h.rom', '6c.rom', '6d.rom'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('3b.rom');
	const PRG3 = zip.decompress('9r.rom');
	const BG = Uint8Array.concat(...['7j.rom', '8j.rom'].map(e => zip.decompress(e)));
	const OBJ = zip.decompress('5t.rom');
	const RED = zip.decompress('red.prm');
	const BLUE = zip.decompress('blue.prm');
	const GREEN = zip.decompress('green.prm');
	const BGCOLOR = zip.decompress('chr.prm');
	const OBJCOLOR = zip.decompress('sprite.prm');
	const SND = zip.decompress('sound.prm');
	const bufferSource = new Zlib.Unzip(archive).decompress('phozon.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG3, RED, BLUE, GREEN, SND, BG, BGCOLOR, OBJ, OBJCOLOR});
});
