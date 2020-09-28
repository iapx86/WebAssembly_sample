### Webpackのインストール
```
$ npm init -y
$ npm install webpack webpack-cli html-webpack-plugin html-webpack-inline-source-plugin --save-dev
```
現在、html-webpack-inline-source-pluginに問題があるということらしいので、
```
$ npm init -y
```
のあとに、package.jsonに以下のブロックを挿入して、
```
  "devDependencies": {
    "core-js": "^2.6.11",
    "html-webpack-inline-source-plugin": "0.0.10",
    "html-webpack-plugin": "^3.2.0",
    "webpack": "^4.44.1",
    "webpack-cli": "^3.3.12"
  }
```
以下を実行するといいかもしれません。
```
$ npm install
```
### その他必要なもの
* make http://gnuwin32.sourceforge.net/packages/make.htm
* emscripten https://emscripten.org/
### ビルド方法
```
$ make
$ webpack
```

