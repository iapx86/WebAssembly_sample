/*
 *
 *	Time Pilot
 *
 */

import {init} from './default_main.js'
import {bufferSource} from './dist/time_pilot.wasm.js';
const url = 'timeplt.zip';

window.addEventListener('load', () => $.ajax({url, success, error: () => alert(url + ': failed to get')}));

function success(zip) {
	const PRG1 = new Uint8Array((zip.files['tm1'].inflate() + zip.files['tm2'].inflate() + zip.files['tm3'].inflate()).split('').map(c => c.charCodeAt(0)));
	const PRG2 = new Uint8Array(zip.files['tm7'].inflate().split('').map(c => c.charCodeAt(0)));
	const BG = new Uint8Array(zip.files['tm6'].inflate().split('').map(c => c.charCodeAt(0)));
	const OBJ = new Uint8Array((zip.files['tm4'].inflate() + zip.files['tm5'].inflate()).split('').map(c => c.charCodeAt(0)));
	const RGB_H = new Uint8Array(zip.files['timeplt.b4'].inflate().split('').map(c => c.charCodeAt(0)));
	const RGB_L = new Uint8Array(zip.files['timeplt.b5'].inflate().split('').map(c => c.charCodeAt(0)));
	const OBJCOLOR = new Uint8Array(zip.files['timeplt.e9'].inflate().split('').map(c => c.charCodeAt(0)));
	const BGCOLOR = new Uint8Array(zip.files['timeplt.e12'].inflate().split('').map(c => c.charCodeAt(0)));
	init(bufferSource, {PRG1, PRG2, BG, OBJ, RGB_H, RGB_L, OBJCOLOR, BGCOLOR}).then();
}

