### Webpackのインストール
```
$ npm init -y
$ npm install webpack webpack-cli html-webpack-plugin html-webpack-inline-source-plugin --save-dev
```
※最新のhtml-webpack-inline-source-pluginに問題があるらしいです。
### その他必要なもの
* 新しいPowerShell https://aka.ms/pscore6
* make http://gnuwin32.sourceforge.net/packages/make.htm
* emscripten https://emscripten.org/
### ビルド方法
```
$ make
$ webpack
```
### ローカルフォルダで実行する場合の注意
* Firefox - about:configのsecurity.fileuri.strict_origin_policyの値をfalseにしてみてください。
* Chrome, Edge - Fetch API cannot load file:///xxx. URL scheme must be "http" or "https" for CORS request. となるのでサーバー経由で実行してください。
* Safari - ローカルファイル、クロスオリジンの制限を無効にしてもファイルの読み込みに失敗するようです。サーバー経由で実行してください。
### http-serverのインストール
```
$ npm install --global http-server
```
### http-serverの実行例
```
$ http-server -o dist/1942.html
```
