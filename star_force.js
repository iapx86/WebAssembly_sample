/*
 *
 *	Star Force
 *
 */

import {init, read} from './default_main.js'
import {bufferSource} from './dist/star_force.wasm.js';

read('starforc.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['3.3p', '2.3mn'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('1.3hj');
	const FG = Uint8Array.concat(...['7.2fh', '8.3fh', '9.3fh'].map(e => zip.decompress(e)));
	const BG1 = Uint8Array.concat(...['15.10jk', '14.9jk', '13.8jk'].map(e => zip.decompress(e)));
	const BG2 = Uint8Array.concat(...['12.10de', '11.9de', '10.8de'].map(e => zip.decompress(e)));
	const BG3 = Uint8Array.concat(...['18.10pq', '17.9pq', '16.8pq'].map(e => zip.decompress(e)));
	const OBJ = Uint8Array.concat(...['6.10lm', '5.9lm', '4.8lm'].map(e => zip.decompress(e)));
	const SND = zip.decompress('07b.bin');
	init(bufferSource, {PRG1, PRG2, FG, BG1, BG2, BG3, OBJ, SND}).then();
});

