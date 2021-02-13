/*
 *
 *	Baraduke
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/baraduke.wasm.js';

read('aliensec.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	const PRG1 = Uint8Array.concat(...['bd1_3.9c', 'baraduke/bd1_1.9a', 'baraduke/bd1_2.9b'].map(e => zip.decompress(e)));
	const PRG2 = zip.decompress('baraduke/bd1_4b.3b');
	const PRG2I = zip.decompress('cus60-60a1.mcu');
	const FG = zip.decompress('bd1_5.3j');
	const BG = Uint8Array.concat(...['baraduke/bd1_8.4p', 'bd1_7.4n', 'baraduke/bd1_6.4m'].map(e => zip.decompress(e)));
	const OBJ = Uint8Array.concat(...['bd1_9.8k', 'bd1_10.8l', 'bd1_11.8m', 'bd1_12.8n'].map(e => zip.decompress(e)));
	const GREEN = zip.decompress('bd1-1.1n');
	const RED = zip.decompress('bd1-2.2m');
	const bufferSource = new Zlib.Unzip(archive).decompress('baraduke.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG2I, FG, BG, OBJ, GREEN, RED});
});
