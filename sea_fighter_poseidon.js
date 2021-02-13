/*
 *
 *	Sea Fighter Poseidon
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/sea_fighter_poseidon.wasm.js';

read('sfposeid.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['a14-01.1', 'a14-02.2', 'a14-03.3', 'a14-04.6', 'a14-05.7'].map(e => zip.decompress(e)));
	const PRG2 = Uint8Array.concat(...['a14-10.70', 'a14-11.71'].map(e => zip.decompress(e)));
	const PRG3 = zip.decompress('a14-12');
	const GFX = Uint8Array.concat(...['a14-06.4', 'a14-07.5', 'a14-08.9', 'a14-09.10'].map(e => zip.decompress(e)));
	const PRI = zip.decompress('eb16.22');
	const bufferSource = new Zlib.Unzip(archive).decompress('sea_fighter_poseidon.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG3, GFX, PRI});
});
