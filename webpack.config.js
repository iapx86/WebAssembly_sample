const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');

const list = [
	{name: '1942', title: '1942', width: 224, height: 256},
	{name: 'baraduke', title: 'Baraduke', width: 288, height: 224},
	{name: 'chackn_pop', title: 'Chack\'n Pop', width: 256, height: 224},
	{name: 'crush_roller', title: 'Crush Roller', width: 224, height: 288},
	{name: 'dragon_buster', title: 'Dragon Buster', width: 288, height: 224},
	{name: 'elevator_action', title: 'Elevator Action', width: 256, height: 224},
	{name: 'frogger', title: 'Frogger', width: 224, height: 256},
	{name: 'gradius', title: 'Gradius', width: 256, height: 224},
	{name: 'libble_rabble', title: 'Libble Rabble', width: 288, height: 224},
	{name: 'star_force', title: 'Star Force', width: 224, height: 256},
	{name: 'time_pilot', title: 'Time Pilot', width: 224, height: 256},
	{name: 'twinbee', title: 'TwinBee', width: 224, height: 256},
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
		})),
		new HtmlWebpackInlineSourcePlugin(),
	),
	module: {
		rules: [{
			test: /\.js$/,
			exclude: /node_modules/,
		}]
	},
};

