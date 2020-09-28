/*
 *
 *	Elevator Action
 *
 */

import {init} from './default_main.js'
import {bufferSource} from './dist/elevator_action.wasm.js';
const url = 'elevator.zip';

window.addEventListener('load', () => $.ajax({url, success, error: () => alert(url + ': failed to get')}));

function success(zip) {
	let PRG1 = zip.files['ea_12.2732.ic69'].inflate() + zip.files['ea_13.2732.ic68'].inflate() + zip.files['ea_14.2732.ic67'].inflate() + zip.files['ea_15.2732.ic66'].inflate() + zip.files['ea_16.2732.ic65'].inflate();
	PRG1 = new Uint8Array((PRG1 + zip.files['ea_17.2732.ic64'].inflate() + zip.files['ea_18.2732.ic55'].inflate() + zip.files['ea_19.2732.ic54'].inflate()).split('').map(c => c.charCodeAt(0)));
	const PRG2 = new Uint8Array((zip.files['ea_9.2732.ic70'].inflate() + zip.files['ea_10.2732.ic71'].inflate()).split('').map(c => c.charCodeAt(0)));
	const PRG3 = new Uint8Array(zip.files['ba3__11.mc68705p3.ic4'].inflate().split('').map(c => c.charCodeAt(0)));
	let GFX = zip.files['ea_20.2732.ic1'].inflate() + zip.files['ea_21.2732.ic2'].inflate() + zip.files['ea_22.2732.ic3'].inflate() + zip.files['ea_23.2732.ic4'].inflate() + zip.files['ea_24.2732.ic5'].inflate();
	GFX = new Uint8Array((GFX + zip.files['ea_25.2732.ic6'].inflate() + zip.files['ea_26.2732.ic7'].inflate() + zip.files['ea_27.2732.ic8'].inflate()).split('').map(c => c.charCodeAt(0)));
	const PRI = new Uint8Array(zip.files['eb16.ic22'].inflate().split('').map(c => c.charCodeAt(0)));
	init(bufferSource, {PRG1, PRG2, PRG3, GFX, PRI}).then();
}

