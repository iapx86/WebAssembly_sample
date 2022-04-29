### Webpackのインストール
```
$ npm init -y
$ npm install -D webpack webpack-cli html-webpack-plugin html-inline-script-webpack-plugin canvas zlibjs
```
### その他必要なもの
* make http://gnuwin32.sourceforge.net/packages/make.htm
* emscripten https://emscripten.org/
### ビルド方法
```
$ make
$ webpack
```
### http-serverのインストール
```
$ npm install -g http-server
```
### http-serverの実行例
```
$ http-server -o dist/1942.html
```
