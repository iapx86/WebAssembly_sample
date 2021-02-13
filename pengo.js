/*
 *
 *	Pengo
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/pengo.wasm.js';

read('pengo.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG = Uint8Array.concat(...['ep1689c.8', 'ep1690b.7', 'ep1691b.15', 'ep1692b.14'].map(e => zip.decompress(e)));
	PRG = Uint8Array.concat(PRG, ...['ep1693b.21', 'ep1694b.20', 'ep5118b.32', 'ep5119c.31'].map(e => zip.decompress(e)));
	const BG = Uint8Array.concat(...['ep1640.92', 'ep1695.105'].map(e => zip.decompress(e).subarray(0, 0x1000)));
	const OBJ = Uint8Array.concat(...['ep1640.92', 'ep1695.105'].map(e => zip.decompress(e).subarray(0x1000)));
	const RGB = zip.decompress('pr1633.78');
	const COLOR = zip.decompress('pr1634.88');
	const SND = zip.decompress('pr1635.51');
	const bufferSource = new Zlib.Unzip(archive).decompress('pengo.wasm');
	return init(bufferSource, {BG, COLOR, OBJ, RGB, PRG, SND});
});
