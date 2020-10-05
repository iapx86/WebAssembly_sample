/*
 *
 *	1942
 *
 */

import {init, read} from './default_main.js'
import {bufferSource} from './dist/1942.wasm.js';

read('1942.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG1 = Uint8Array.concat(...['srb-03.m3', 'srb-04.m4', 'srb-05.m5', 'srb-06.m6'].map(e => zip.decompress(e)));
	PRG1 = Uint8Array.concat(PRG1, new Uint8Array(0x2000).fill(0xff), zip.decompress('srb-07.m7'), new Uint8Array(0x4000).fill(0xff));
	const PRG2 = zip.decompress('sr-01.c11');
	const FG = zip.decompress('sr-02.f2');
	const BG = Uint8Array.concat(...['sr-08.a1', 'sr-09.a2', 'sr-10.a3', 'sr-11.a4', 'sr-12.a5', 'sr-13.a6'].map(e => zip.decompress(e)));
	const OBJ = Uint8Array.concat(...['sr-14.l1', 'sr-15.l2', 'sr-16.n1', 'sr-17.n2'].map(e => zip.decompress(e)));
	const RED = zip.decompress('sb-5.e8');
	const GREEN = zip.decompress('sb-6.e9');
	const BLUE = zip.decompress('sb-7.e10');
	const FGCOLOR = zip.decompress('sb-0.f1');
	const BGCOLOR = zip.decompress('sb-4.d6');
	const OBJCOLOR = zip.decompress('sb-8.k3');
	init(bufferSource, {PRG1, PRG2, FG, BG, OBJ, RED, GREEN, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR}).then();
});
