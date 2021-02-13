/*
 *
 *	Frogger
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/frogger.wasm.js';

read('frogger.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['frogger.26', 'frogger.27', 'frsm3.7'].map(e => zip.decompress(e)));
	const PRG2 = Uint8Array.concat(...['frogger.608', 'frogger.609', 'frogger.610'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['frogger.607', 'frogger.606'].map(e => zip.decompress(e)));
	const RGB = zip.decompress('pr-91.6l');
	const bufferSource = new Zlib.Unzip(archive).decompress('frogger.wasm');
	return init(bufferSource, {BG, RGB, PRG1, PRG2});
});
