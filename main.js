/*
 *
 *	Main Module
 *
 */

const volume0 = 'data:image/png;base64,\
iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAC4jAAAuIwF4pT92AAAAeElEQVR42u3XywrAIAxE0fz/T09XXRRaTCvDnYoBV7o4\
oHlYkopctQF/B5yBAEQCRAJEAkQBRmEDdGMaMBtvUvOy7wSok7oOwBPi9rwL0M4YJ6BVsJYGoFeAPkI8DT8VIrwUxzajmHYcM5BEjGQRQ2nEWL5/RmsADqNJ\
E6QZh85dAAAAAElFTkSuQmCC\
';
const volume1 = 'data:image/png;base64,\
iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAC4jAAAuIwF4pT92AAAAfklEQVR42u2XwQrAIAxD+/8/nZ12GrhoWoolBS9SyEOl\
iQEgOlcY4HaAt1oAQAIs+zLEofRmiEMBzhAH+TYkgL9i7/2zrwozAGAA1IrTU6gECOYUDGAAA4ydA9uTsHIUS16gmlGaG+7acVkeOAkk6YlIiWQtoXRuLPfP\
aAbAA72UT2ikWgrdAAAAAElFTkSuQmCC\
';
const vsSource = `
	attribute vec4 aVertexPosition;
	attribute vec2 aTextureCoord;
	varying highp vec2 vTextureCoord;
	void main(void) {
		gl_Position = aVertexPosition;
		vTextureCoord = aTextureCoord;
	}
`;
const fsSource = `
	varying highp vec2 vTextureCoord;
	uniform sampler2D uSampler;
	void main(void) {
		gl_FragColor = texture2D(uSampler, vTextureCoord);
	}
`;
const game = {cxScreen: 0, cyScreen: 0, width: 1, height: 1, xOffset: 0, yOffset: 0, rotate: false};
const cxScreen = canvas.width, cyScreen = canvas.height;
const gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
let source, scriptNode, button, state = '';
let instance, memory, view;

(window.onresize = () => {
	const zoom = Math.max(1, Math.min(Math.floor(window.innerWidth / cxScreen), Math.floor(window.innerHeight / cyScreen)));
	canvas.width = cxScreen * zoom;
	canvas.height = cyScreen * zoom;
	gl.viewport(0, 0, canvas.width, canvas.height);
})();

function loadShader(gl, type, source) {
	const shader = gl.createShader(type);
	gl.shaderSource(shader, source);
	gl.compileShader(shader);
	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader));
		gl.deleteShader(shader);
		return null;
	}
	return shader;
}

function initShaderProgram(gl, vsSource, fsSource) {
	const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
	const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);
	const shaderProgram = gl.createProgram();
	gl.attachShader(shaderProgram, vertexShader);
	gl.attachShader(shaderProgram, fragmentShader);
	gl.linkProgram(shaderProgram);
	if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
		alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
		return null;
	}
	return shaderProgram;
}

function fd_write(fd, iovs, iovsLen, nwritten) {
	let str = '';
	for (let p = iovs, i = 0; i < iovsLen; p += 8, i++)
		str += String.fromCharCode(...new Uint8Array(memory, view.getUint32(p, true), view.getUint32(p + 4, true)));
	view.setUint32(nwritten, str.length, true);
	console.log(str);
	return 0;
}

function proc_exit(rval) {
	console.log(`proc_exit: rval=${rval}`);
}

export function init(bufferSource, roms) {
	const importObject = {wasi_snapshot_preview1: {fd_write, proc_exit, fd_seek: () => {}, fd_close: () => {}}};
	return WebAssembly.instantiate(bufferSource, importObject).then(result => {
		instance = result.instance, memory = instance.exports.memory.buffer, view = new DataView(memory);
		instance.exports._initialize();
		for (let p = instance.exports.roms(); view.getUint32(p, true) !== 0; p += 16) {
			const name = String.fromCharCode(...new Uint8Array(memory, view.getUint32(p, true), view.getUint32(p + 4, true)));
			new Uint8Array(memory, view.getUint32(p + 8, true), view.getUint32(p + 12, true)).set(roms[name]);
		}
		const g = new Int32Array(memory, instance.exports.init(audioCtx.sampleRate), 7 * 4);
		Object.assign(game, {cxScreen: g[0], cyScreen: g[1], width: g[2], height: g[3], xOffset: g[4], yOffset: g[5], rotate: g[6] !== 0});
		const positions = new Float32Array(game.rotate ? [-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0] : [-1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0]);
		const textureCoordinates = new Float32Array([
			game.xOffset / game.width, game.yOffset / game.height,
			game.xOffset / game.width, (game.yOffset + game.cyScreen) / game.height,
			(game.xOffset + game.cxScreen) / game.width, game.yOffset / game.height,
			(game.xOffset + game.cxScreen) / game.width, (game.yOffset + game.cyScreen) / game.height
		]);
		const program = initShaderProgram(gl, vsSource, fsSource);
		const aVertexPositionHandle = gl.getAttribLocation(program, 'aVertexPosition');
		const aTextureCoordHandle = gl.getAttribLocation(program, 'aTextureCoord');
//		const uSamplerHandle = gl.getUniformLocation(program, 'uSampler');
		const positionBuffer = gl.createBuffer();
		const textureCoordBuffer = gl.createBuffer();
		const texture = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, texture);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, game.width, game.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(game.width * game.height * 4));
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.useProgram(program);
		gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, positions, gl.STATIC_DRAW);
		gl.vertexAttribPointer(aVertexPositionHandle, 2, gl.FLOAT, false, 0, 0);
		gl.enableVertexAttribArray(aVertexPositionHandle);
		gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, textureCoordinates, gl.STATIC_DRAW);
		gl.vertexAttribPointer(aTextureCoordHandle, 2, gl.FLOAT, false, 0, 0);
		gl.enableVertexAttribArray(aTextureCoordHandle);
		source = audioCtx.createBufferSource(), scriptNode = audioCtx.createScriptProcessor(512, 1, 1);
		scriptNode.onaudioprocess = ({outputBuffer}) => outputBuffer.getChannelData(0).set(new Float32Array(memory, instance.exports.sound(), 512));
		source.connect(scriptNode).connect(audioCtx.destination);
		source.start();
		button = new Image();
		(button.update = () => {
			button.src = audioCtx.state === 'suspended' ? volume0 : volume1;
			button.alt = 'audio state: ' + audioCtx.state;
		})();
		audioCtx.onstatechange = button.update;
		document.body.appendChild(button);
		button.addEventListener('click', () => {
			if (audioCtx.state === 'suspended')
				audioCtx.resume().catch();
			else if (audioCtx.state === 'running')
				audioCtx.suspend().catch();
		});
		window.addEventListener('blur', () => {
			state = audioCtx.state;
			audioCtx.suspend().catch();
		});
		window.addEventListener('focus', () => {
			if (state === 'running')
				audioCtx.resume().catch();
		});
		void function loop() {
			instance.exports.update();
			updateGamepad(instance.exports);
			const data = new Uint8Array(memory, instance.exports.render() + game.yOffset * game.width * 4, game.width * game.cyScreen * 4);
			gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, game.yOffset, game.width, game.cyScreen, gl.RGBA, gl.UNSIGNED_BYTE, data);
			gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
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

/*
 *
 *	Gamepad Module
 *
 */

const haveEvents = 'ongamepadconnected' in window;
const controllers = [];
const gamepadStatus = {up: false, right: false, down: false, left: false, up2: false, right2: false, down2: false, left2: false, buttons: new Array(16).fill(false)};

window.addEventListener('gamepadconnected', e => controllers[e.gamepad.index] = e.gamepad);
window.addEventListener('gamepaddisconnected', e => delete controllers[e.gamepad.index]);

function updateGamepad(game) {
	if (!haveEvents) {
		const gamepads = 'getGamepads' in navigator && navigator.getGamepads() || 'webkitGetGamepads' in navigator && navigator.webkitGetGamepads() || [];
		controllers.splice(0);
		for (let i = 0, n = gamepads.length; i < n; i++)
			if (gamepads[i])
				controllers[gamepads[i].index] = gamepads[i];
	}
	const controller = controllers.find(() => true);
	if (!controller)
		return;
	let val, pressed;
	val = controller.buttons[0];
	if ('triggerA' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[0])
		game.triggerA(gamepadStatus.buttons[0] = pressed);
	val = controller.buttons[1];
	if ('triggerB' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[1])
		game.triggerB(gamepadStatus.buttons[1] = pressed);
	val = controller.buttons[2];
	if ('triggerX' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[2])
		game.triggerX(gamepadStatus.buttons[2] = pressed);
	val = controller.buttons[3];
	if ('triggerY' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[3])
		game.triggerY(gamepadStatus.buttons[3] = pressed);
	val = controller.buttons[4];
	if ('triggerL1' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[4])
		game.triggerL1(gamepadStatus.buttons[4] = pressed);
	val = controller.buttons[5];
	if ('triggerR1' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[5])
		game.triggerR1(gamepadStatus.buttons[5] = pressed);
	val = controller.buttons[6];
	if ('triggerL2' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[6])
		game.triggerL2(gamepadStatus.buttons[6] = pressed);
	val = controller.buttons[7];
	if ('triggerR2' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[7])
		game.triggerR2(gamepadStatus.buttons[7] = pressed);
	val = controller.buttons[8];
	if ('coin' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[8] && (gamepadStatus.buttons[8] = pressed))
		game.coin();
	val = controller.buttons[9];
	if ('start1P' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[9] && (gamepadStatus.buttons[9] = pressed))
		game.start1P();
	val = controller.buttons[10];
	if ('triggerL3' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[10])
		game.triggerL3(gamepadStatus.buttons[4] = pressed);
	val = controller.buttons[11];
	if ('triggerR3' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[11])
		game.triggerR3(gamepadStatus.buttons[5] = pressed);
	val = controller.buttons[12];
	if ('up' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[12])
		game.up(gamepadStatus.buttons[12] = pressed);
	val = controller.buttons[13];
	if ('down' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[13])
		game.down(gamepadStatus.buttons[13] = pressed);
	val = controller.buttons[14];
	if ('left' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[14])
		game.left(gamepadStatus.buttons[14] = pressed);
	val = controller.buttons[15];
	if ('right' in game && (pressed = typeof val === 'object' ? val.pressed : val === 1.0) !== gamepadStatus.buttons[15])
		game.right(gamepadStatus.buttons[15] = pressed);
	if ('up' in game && (pressed = controller.axes[1] < -0.5) !== gamepadStatus.up)
		game.up(gamepadStatus.up = pressed);
	if ('right' in game && (pressed = controller.axes[0] > 0.5) !== gamepadStatus.right)
		game.right(gamepadStatus.right = pressed);
	if ('down' in game && (pressed = controller.axes[1] > 0.5) !== gamepadStatus.down)
		game.down(gamepadStatus.down = pressed);
	if ('left' in game && (pressed = controller.axes[0] < -0.5) !== gamepadStatus.left)
		game.left(gamepadStatus.left = pressed);
	if ('up2' in game && (pressed = controller.axes[3] < -0.5) !== gamepadStatus.up2)
		game.up2(gamepadStatus.up2 = pressed);
	if ('right2' in game && (pressed = controller.axes[2] > 0.5) !== gamepadStatus.right2)
		game.right2(gamepadStatus.right2 = pressed);
	if ('down2' in game && (pressed = controller.axes[3] > 0.5) !== gamepadStatus.down2)
		game.down2(gamepadStatus.down2 = pressed);
	if ('left2' in game && (pressed = controller.axes[2] < -0.5) !== gamepadStatus.left2)
		game.left2(gamepadStatus.left2 = pressed);
}
