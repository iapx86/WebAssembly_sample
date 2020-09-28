/*
 *
 *	Star Force
 *
 */

import {init} from './default_main.js'
import {bufferSource} from './dist/star_force.wasm.js';
const url = 'starforc.zip';

window.addEventListener('load', () => $.ajax({url, success, error: () => alert(url + ': failed to get')}));

function success(zip) {
	const PRG1 = new Uint8Array((zip.files['3.3p'].inflate() + zip.files['2.3mn'].inflate()).split('').map(c => c.charCodeAt(0)));
	const PRG2 = new Uint8Array(zip.files['1.3hj'].inflate().split('').map(c => c.charCodeAt(0)));
	const FG = new Uint8Array((zip.files['7.2fh'].inflate()+ zip.files['8.3fh'].inflate() + zip.files['9.3fh'].inflate()).split('').map(c => c.charCodeAt(0)));
	const BG1 = new Uint8Array((zip.files['15.10jk'].inflate()+ zip.files['14.9jk'].inflate() + zip.files['13.8jk'].inflate()).split('').map(c => c.charCodeAt(0)));
	const BG2 = new Uint8Array((zip.files['12.10de'].inflate()+ zip.files['11.9de'].inflate() + zip.files['10.8de'].inflate()).split('').map(c => c.charCodeAt(0)));
	const BG3 = new Uint8Array((zip.files['18.10pq'].inflate()+ zip.files['17.9pq'].inflate() + zip.files['16.8pq'].inflate()).split('').map(c => c.charCodeAt(0)));
	const OBJ = new Uint8Array((zip.files['6.10lm'].inflate()+ zip.files['5.9lm'].inflate() + zip.files['4.8lm'].inflate()).split('').map(c => c.charCodeAt(0)));
	const SND = new Uint8Array(zip.files['07b.bin'].inflate().split('').map(c => c.charCodeAt(0)));
	init(bufferSource, {PRG1, PRG2, FG, BG1, BG2, BG3, OBJ, SND}).then();
}

