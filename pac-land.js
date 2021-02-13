/*
 *
 *	Pac-Land
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/pac-land.wasm.js';

read('pacland.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG1 = Uint8Array.concat(...['paclandj/pl6_01.8b', 'paclandj/pl6_02.8d', 'pl1_3.8e'].map(e => zip.decompress(e)));
	PRG1 = Uint8Array.concat(PRG1, ...['pl1_4.8f', 'pl1_5.8h', 'paclandj/pl1_6.8j'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('pl1_7.3e');
	const PRG2I = zip.decompress('cus60-60a1.mcu');
	const FG = zip.decompress('paclandj/pl6_12.6n');
	const BG = zip.decompress('paclandj/pl1_13.6t');
	let OBJ = Uint8Array.concat(...['paclandj/pl1_9b.6f', 'paclandj/pl1_8.6e'].map(e => zip.decompress(e)));
	OBJ = Uint8Array.concat(OBJ, ...['paclandj/pl1_10b.7e', 'paclandj/pl1_11.7f'].map(e => zip.decompress(e)));
	const RED = zip.decompress('pl1-2.1t');
	const BLUE = zip.decompress('pl1-1.1r');
	const FGCOLOR = zip.decompress('pl1-5.5t');
	const BGCOLOR = zip.decompress('pl1-4.4n');
	const OBJCOLOR = zip.decompress('pl1-3.6l');
	const bufferSource = new Zlib.Unzip(archive).decompress('pac-land.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG2I, FG, BG, OBJ, RED, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR});
});
