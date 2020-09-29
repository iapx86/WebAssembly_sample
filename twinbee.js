/*
 *
 *	TwinBee
 *
 */

import {init} from './default_main.js'
import {bufferSource} from './dist/twinbee.wasm.js';
const url = 'twinbee.zip';

window.addEventListener('load', () => $.ajax({url, success, error: () => alert(url + ': failed to get')}));

function success(zip) {
	const PRG1 = new Uint8Array(0x50000);
	zip.files['400-a06.15l'].inflate().split('').forEach((c, i) => PRG1[i << 1] = c.charCodeAt(0));
	zip.files['400-a04.10l'].inflate().split('').forEach((c, i) => PRG1[1 + (i << 1)] = c.charCodeAt(0));
	zip.files['412-a07.17l'].inflate().split('').forEach((c, i) => PRG1[0x10000 + (i << 1)] = c.charCodeAt(0));
	zip.files['412-a05.12l'].inflate().split('').forEach((c, i) => PRG1[0x10001 + (i << 1)] = c.charCodeAt(0));
	const PRG2 = new Uint8Array(zip.files['400-e03.5l'].inflate().split('').map(c => c.charCodeAt(0)));
	const SND = new Uint8Array((zip.files['400-a01.fse'].inflate() + zip.files['400-a02.fse'].inflate()).split('').map(c => c.charCodeAt(0)));
	init(bufferSource, {PRG1, PRG2, SND}).then();
}

