/*
 *
 *	DigDug
 *
 */

import {init, read} from './default_main.js';
import {archive} from './dist/digdug.wasm.js';
let bufferSource, PRG1, PRG2, PRG3, BG2, MAPDATA, BG4, OBJ, SND, BGCOLOR, OBJCOLOR, RGB, IO;

read('digdug.zip').then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	PRG1 = Uint8Array.concat(...['dd1a.1', 'dd1a.2', 'dd1a.3', 'dd1a.4'].map(e => zip.decompress(e)));
	PRG2 = Uint8Array.concat(...['dd1a.5', 'dd1a.6'].map(e => zip.decompress(e)));
	PRG3 = zip.decompress('dd1.7');
	BG2 = zip.decompress('dd1.9');
	OBJ = Uint8Array.concat(...['dd1.15', 'dd1.14', 'dd1.13', 'dd1.12'].map(e => zip.decompress(e)));
	BG4 = zip.decompress('dd1.11');
	MAPDATA = zip.decompress('dd1.10b');
	RGB = zip.decompress('136007.113');
	OBJCOLOR = zip.decompress('136007.111');
	BGCOLOR = zip.decompress('136007.112');
	SND = zip.decompress('136007.110');
}).then(() =>read('namco51.zip')).then(buffer => new Zlib.Unzip(new Uint8Array(buffer))).then(zip => {
	IO = zip.decompress('51xx.bin');
	bufferSource = new Zlib.Unzip(archive).decompress('digdug.wasm');
	return init(bufferSource, {PRG1, PRG2, PRG3, BG2, MAPDATA, BG4, OBJ, SND, BGCOLOR, OBJCOLOR, RGB, IO});
});
