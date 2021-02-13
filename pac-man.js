/*
 *
 *	Pac-Man
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/pac-man.wasm.js';

read('puckman.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG = Uint8Array.concat(...['pm1_prg1.6e', 'pm1_prg2.6k', 'pm1_prg3.6f', 'pm1_prg4.6m'].map(e => zip.decompress(e)));
	PRG = Uint8Array.concat(PRG, ...['pm1_prg5.6h', 'pm1_prg6.6n', 'pm1_prg7.6j', 'pm1_prg8.6p'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['pm1_chg1.5e', 'pm1_chg2.5h'].map(e => zip.decompress(e)));
	const OBJ = Uint8Array.concat(...['pm1_chg3.5f', 'pm1_chg4.5j'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('pm1-1.7f');
	const COLOR = zip.decompress('pm1-4.4a');
	const SND = zip.decompress('pm1-3.1m');
	const bufferSource = new Zlib.Unzip(archive).decompress('pac-man.wasm');
	return init(bufferSource, {BG, COLOR, OBJ, RGB, PRG, SND});
});
