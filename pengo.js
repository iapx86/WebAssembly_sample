/*
 *
 *	Pengo
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/pengo.wasm.js';

read('pengo.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG = Uint8Array.concat(...['pengo3u/ep5120.8', 'pengo3u/ep5121.7', 'pengo3u/ep5122.15', 'pengo3u/ep5123.14'].map(e => zip.decompress(e)));
	PRG = Uint8Array.concat(PRG, ...['pengo2u/ep5124.21', 'pengo3u/ep5125.20', 'pengo2u/ep5126.32', 'pengo3u/ep5127.31'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['ep1640.92', 'ep1695.105'].map(e => zip.decompress(e).subarray(0, 0x1000)));
	const OBJ = Uint8Array.concat(...['ep1640.92', 'ep1695.105'].map(e => zip.decompress(e).subarray(0x1000)));
	const RGB = zip.decompress('pr1633.78');
	const COLOR = zip.decompress('pr1634.88');
	const SND = zip.decompress('pr1635.51');
	init(bufferSource, {BG, COLOR, OBJ, RGB, PRG, SND}).then();
});
