/*
 *
 *	Sky Kid
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/sky_kid.wasm.js';

read('skykid.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['sk2_2.6c', 'sk1-1c.6b', 'sk1_3.6d'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('sk2_4.3c');
	const PRG2I = zip.decompress('cus63-63a1.mcu');
	const FG = zip.decompress('sk1_6.6l');
	const BG = zip.decompress('sk1_5.7e');
	const OBJ = Uint8Array.concat(...['sk1_8.10n', 'sk1_7.10m'].map(e => zip.decompress(e)));
	const RED = zip.decompress('sk1-1.2n');
	const GREEN = zip.decompress('sk1-2.2p');
	const BLUE = zip.decompress('sk1-3.2r');
	const BGCOLOR = zip.decompress('sk1-4.5n');
	const OBJCOLOR = zip.decompress('sk1-5.6n');
	const bufferSource = new Zlib.Unzip(archive).decompress('sky_kid.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG2I, FG, BG, OBJ, RED, GREEN, BLUE, BGCOLOR, OBJCOLOR});
});
