/*
 *
 *	Time Tunnel
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/time_tunnel.wasm.js';

read('timetunl.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG1 = Uint8Array.concat(...['un01.69', 'un02.68', 'un03.67', 'un04.66', 'un05.65'].map(e => zip.decompress(e)));
	PRG1 = Uint8Array.concat(PRG1, ...['un06.64', 'un07.55', 'un08.54', 'un09.53', 'un10.52'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('un19.70');
	const GFX = Uint8Array.concat(...['un11.1', 'un12.2', 'un13.3', 'un14.4', 'un15.5', 'un16.6', 'un17.7', 'un18.8'].map(e => zip.decompress(e)));
	const PRI = zip.decompress('eb16.22');
	const bufferSource = new Zlib.Unzip(archive).decompress('time_tunnel.wasm');
	return init(bufferSource, {PRG1, PRG2, GFX, PRI});
});
