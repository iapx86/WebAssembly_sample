/*
 *
 *	Elevator Action
 *
 */

import {init, read} from './default_main.js';
import {bufferSource} from './dist/elevator_action.wasm.js';

read('elevator.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	let PRG1 = Uint8Array.concat(...['ea_12.2732.ic69', 'ea_13.2732.ic68', 'ea_14.2732.ic67', 'ea_15.2732.ic66'].map(e => zip.decompress(e)));
	PRG1 = Uint8Array.concat(PRG1, ...['ea_16.2732.ic65', 'ea_17.2732.ic64', 'ea_18.2732.ic55', 'ea_19.2732.ic54'].map(e => zip.decompress(e)));
	const PRG2 = Uint8Array.concat(...['ea_9.2732.ic70', 'ea_10.2732.ic71'].map(e => zip.decompress(e)));
	const PRG3 = zip.decompress('ba3__11.mc68705p3.ic4');
	let GFX = Uint8Array.concat(...['ea_20.2732.ic1', 'ea_21.2732.ic2', 'ea_22.2732.ic3', 'ea_23.2732.ic4'].map(e => zip.decompress(e)));
	GFX = Uint8Array.concat(GFX, ...['ea_24.2732.ic5', 'ea_25.2732.ic6', 'ea_26.2732.ic7', 'ea_27.2732.ic8'].map(e => zip.decompress(e)));
	const PRI = zip.decompress('eb16.ic22');
	init(bufferSource, {PRG1, PRG2, PRG3, GFX, PRI}).then();
});
