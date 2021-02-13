/*
 *
 *	Time Pilot
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/time_pilot.wasm.js';

read('timeplt.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['tm1', 'tm2', 'tm3'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('tm7');
	const BG = zip.decompress('tm6');
	const OBJ = Uint8Array.concat(...['tm4', 'tm5'].map(e => zip.decompress(e)));
	const RGB_H = zip.decompress('timeplt.b4');
	const RGB_L = zip.decompress('timeplt.b5');
	const OBJCOLOR = zip.decompress('timeplt.e9');
	const BGCOLOR = zip.decompress('timeplt.e12');
	const bufferSource = new Zlib.Unzip(archive).decompress('time_pilot.wasm');
	return init(bufferSource, {PRG1, PRG2, BG, OBJ, RGB_H, RGB_L, OBJCOLOR, BGCOLOR});
});
