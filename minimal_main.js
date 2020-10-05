/*
 *
 *	Minimal Main Module
 *
 */

(window.onresize = () => {
	const cxScreen = canvas.width, cyScreen = canvas.height;
	const zoom = Math.max(1, Math.min(Math.floor(window.innerWidth / cxScreen), Math.floor(window.innerHeight / cyScreen)));
	canvas.style.width = cxScreen * zoom + 'px';
	canvas.style.height = cyScreen * zoom + 'px';
})();

export function init(bufferSource, roms) {
	const importObject = {wasi_snapshot_preview1: {fd_write: () => {}, proc_exit: () => {}, fd_seek: () => {}, fd_close: () => {}}};
	return WebAssembly.instantiate(bufferSource, importObject).then(({instance}) => {
		const memory = instance.exports.memory.buffer, view = new DataView(memory);
		instance.exports._initialize();
		for (let p = instance.exports.roms(); view.getUint32(p, true) !== 0; p += 16) {
			const name = String.fromCharCode(...new Uint8Array(memory, view.getUint32(p, true), view.getUint32(p + 4, true)));
			new Uint8Array(memory, view.getUint32(p + 8, true), view.getUint32(p + 12, true)).set(roms[name]);
		}
		const g = new Int32Array(memory, instance.exports.init(audioCtx.sampleRate), 7 * 4);
		const game = {cxScreen: g[0], cyScreen: g[1], width: g[2], height: g[3], xOffset: g[4], yOffset: g[5], rotate: g[6] !== 0};
		canvas.style.imageRendering = 'pixelated';
		document.addEventListener('keydown', e => {
			switch (e.code) {
			case 'ArrowLeft':
				return 'left' in instance.exports && instance.exports.left(true);
			case 'ArrowUp':
				return 'up' in instance.exports && instance.exports.up(true);
			case 'ArrowRight':
				return 'right' in instance.exports && instance.exports.right(true);
			case 'ArrowDown':
				return 'down' in instance.exports && instance.exports.down(true);
			case 'Digit0':
				return 'coin' in instance.exports && instance.exports.coin();
			case 'Digit1':
				return 'start1P' in instance.exports && instance.exports.start1P();
			case 'Digit2':
				return 'start2P' in instance.exports && instance.exports.start2P();
			case 'KeyR':
				return 'reset' in instance.exports && instance.exports.reset();
			case 'KeyT':
				return 'test' in instance.exports && instance.exports.test();
			case 'Space':
			case 'KeyX':
				return 'triggerA' in instance.exports && instance.exports.triggerA(true);
			case 'KeyZ':
				return 'triggerB' in instance.exports && instance.exports.triggerB(true);
			}
		});
		document.addEventListener('keyup', e => {
			switch (e.code) {
			case 'ArrowLeft':
				return 'left' in instance.exports && instance.exports.left();
			case 'ArrowUp':
				return 'up' in instance.exports && instance.exports.up();
			case 'ArrowRight':
				return 'right' in instance.exports && instance.exports.right();
			case 'ArrowDown':
				return 'down' in instance.exports && instance.exports.down();
			case 'Space':
			case 'KeyX':
				return 'triggerA' in instance.exports && instance.exports.triggerA();
			case 'KeyZ':
				return 'triggerB' in instance.exports && instance.exports.triggerB();
			}
		});
		canvas.addEventListener('click', () => instance.exports.coin());
		void function loop() {
			instance.exports.update();
			const data = new Uint8ClampedArray(memory, instance.exports.render(), game.width * game.height * 4);
			canvas.getContext('2d').putImageData(new ImageData(data, game.width, game.height), -game.xOffset, -game.yOffset);
			requestAnimationFrame(loop);
		}();
		return instance;
	});
}

/*
 *
 *	Array supplementary
 *
 */

Uint8Array.concat = function (...args) {
	const typed_array = new this(args.reduce((a, b) => a + b.length, 0));
	for (let offset = 0, i = 0; i < args.length; offset += args[i++].length)
		typed_array.set(args[i], offset);
	return typed_array;
};

export function read(url) {
	return fetch(url).then(response => {
		if (response.ok)
			return response.arrayBuffer();
		alert(`failed to get: ${url}`);
		throw new Error(url);
	});
}
