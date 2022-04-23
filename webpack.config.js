const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const HtmlInlineScriptPlugin = require('html-inline-script-webpack-plugin');

const list = [
	{name: '1942', title: '1942', width: 224, height: 256},
	{name: 'baraduke', title: 'Baraduke', width: 288, height: 224},
	{name: 'chackn_pop', title: 'Chack\'n Pop', width: 256, height: 224},
	{name: 'crush_roller', title: 'Crush Roller', width: 224, height: 288},
	{name: 'digdug', title: 'DigDug', width: 224, height: 288},
	{name: 'digdug_ii', title: 'DigDug II', width: 224, height: 288},
	{name: 'dragon_buster', title: 'Dragon Buster', width: 288, height: 224},
	{name: 'elevator_action', title: 'Elevator Action', width: 256, height: 224},
	{name: 'frogger', title: 'Frogger', width: 224, height: 256},
	{name: 'gradius', title: 'Gradius', width: 256, height: 224},
	{name: 'grobda', title: 'Grobda', width: 224, height: 288},
	{name: 'libble_rabble', title: 'Libble Rabble', width: 288, height: 224},
	{name: 'mappy', title: 'Mappy', width: 224, height: 288},
	{name: 'metro-cross', title: 'Metro-Cross', width: 288, height: 224},
	{name: 'motos', title: 'Motos', width: 224, height: 288},
	{name: 'pac-land', title: 'Pac-Land', width: 288, height: 224},
	{name: 'pac-man', title: 'Pac-Man', width: 224, height: 288},
	{name: 'pac_and_pal', title: 'Pac & Pal', width: 224, height: 288},
	{name: 'pengo', title: 'Pengo', width: 224, height: 288},
	{name: 'phozon', title: 'Phozon', width: 224, height: 288},
	{name: 'sea_fighter_poseidon', title: 'Sea Fighter Poseidon', width: 256, height: 224},
	{name: 'sky_kid', title: 'Sky Kid', width: 288, height: 224},
	{name: 'star_force', title: 'Star Force', width: 224, height: 256},
	{name: 'strategy_x', title: 'Strategy X', width: 256, height: 224},
	{name: 'super_pac-man', title: 'Super Pac-Man', width: 224, height: 288},
	{name: 'the_tower_of_druaga', title: 'The Tower of Druaga', width: 224, height: 288},
	{name: 'time_pilot', title: 'Time Pilot', width: 224, height: 256},
	{name: 'time_tunnel', title: 'Time Tunnel', width: 256, height: 224},
	{name: 'toypop', title: 'Toypop', width: 288, height: 224},
	{name: 'twinbee', title: 'TwinBee', width: 224, height: 256},
	{name: 'vulgus', title: 'Vulgus', width: 224, height: 256},
	{name: 'zigzag', title: 'Zig Zag', width: 224, height: 256},
];

module.exports = {
	mode: 'production',
	entry: list.reduce((a, b) => Object.assign(a, {[b.name]: `./${b.name}.js`}), {}),
	output: {
		filename: '[name].bundle.js',
		path: path.resolve(__dirname, 'dist'),
	},
	devServer: {contentBase: path.resolve(__dirname, 'dist')},
	plugins: [].concat(
		list.map(e => new HtmlWebpackPlugin({
			filename: `${e.name}.html`,
			template: 'index.html',
			chunks: [e.name],
			title: e.title,
			width: e.width,
			height: e.height,
			inlineSource: '.js$',
			inject: 'body',
		})),
		new HtmlInlineScriptPlugin(),
	),
	module: {
		rules: [{
			test: /\.js$/,
			exclude: /node_modules/,
		}]
	},
};

