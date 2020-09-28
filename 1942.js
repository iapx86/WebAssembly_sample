/*
 *
 *	1942
 *
 */

import {init} from './default_main.js'
import {bufferSource} from './dist/1942.wasm.js';
const url = '1942.zip';

window.addEventListener('load', () => $.ajax({url, success, error: () => alert(url + ': failed to get')}));

function success(zip) {
	let PRG1 = zip.files['srb-03.m3'].inflate() + zip.files['srb-04.m4'].inflate() + zip.files['srb-05.m5'].inflate() + zip.files['srb-06.m6'].inflate();
	PRG1 = new Uint8Array((PRG1 + '\xff'.repeat(0x2000) + zip.files['srb-07.m7'].inflate() + '\xff'.repeat(0x4000)).split('').map(c => c.charCodeAt(0)));
	const PRG2 = new Uint8Array(zip.files['sr-01.c11'].inflate().split('').map(c => c.charCodeAt(0)));
	const FG = new Uint8Array(zip.files['sr-02.f2'].inflate().split('').map(c => c.charCodeAt(0)));
	let BG = zip.files['sr-08.a1'].inflate() + zip.files['sr-09.a2'].inflate() + zip.files['sr-10.a3'].inflate() + zip.files['sr-11.a4'].inflate();
	BG = new Uint8Array((BG + zip.files['sr-12.a5'].inflate() + zip.files['sr-13.a6'].inflate()).split('').map(c => c.charCodeAt(0)));
	let OBJ = zip.files['sr-14.l1'].inflate() + zip.files['sr-15.l2'].inflate() + zip.files['sr-16.n1'].inflate() + zip.files['sr-17.n2'].inflate();
	OBJ = new Uint8Array(OBJ.split('').map(c => c.charCodeAt(0)));
	const RED = new Uint8Array(zip.files['sb-5.e8'].inflate().split('').map(c => c.charCodeAt(0)));
	const GREEN = new Uint8Array(zip.files['sb-6.e9'].inflate().split('').map(c => c.charCodeAt(0)));
	const BLUE = new Uint8Array(zip.files['sb-7.e10'].inflate().split('').map(c => c.charCodeAt(0)));
	const FGCOLOR = new Uint8Array(zip.files['sb-0.f1'].inflate().split('').map(c => c.charCodeAt(0)));
	const BGCOLOR = new Uint8Array(zip.files['sb-4.d6'].inflate().split('').map(c => c.charCodeAt(0)));
	const OBJCOLOR = new Uint8Array(zip.files['sb-8.k3'].inflate().split('').map(c => c.charCodeAt(0)));
	init(bufferSource, {PRG1, PRG2, FG, BG, OBJ, RED, GREEN, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR}).then();
}
