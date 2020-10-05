/*
 *
 *	TwinBee
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/twinbee.wasm.js';

read('twinbee.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = new Uint8Array(0x50000);
	zip.decompress('400-a06.15l').forEach((e, i) => PRG1[i << 1] = e);
	zip.decompress('400-a04.10l').forEach((e, i) => PRG1[1 + (i << 1)] = e);
	zip.decompress('412-a07.17l').forEach((e, i) => PRG1[0x10000 + (i << 1)] = e);
	zip.decompress('412-a05.12l').forEach((e, i) => PRG1[0x10001 + (i << 1)] = e);
	const PRG2 = zip.decompress('400-e03.5l');
	const SND = Uint8Array.concat(...['400-a01.fse', '400-a02.fse'].map(e => zip.decompress(e)));
	init(bufferSource, {PRG1, PRG2, SND}).then();
});
