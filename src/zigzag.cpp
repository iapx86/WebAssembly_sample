/*
 *
 *	Zig Zag
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "zigzag.h"
using namespace std;

unsigned char ZigZag::BG[0x1000], ZigZag::OBJ[0x1000], ZigZag::RGB[0x20], ZigZag::PRG[0x4000];
AY_3_8910 *ZigZag::sound0;

ZigZag *game;
vector<int> rom_table = {
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"RGB", (int)strlen("RGB"), (int)game->RGB, (int)sizeof(game->RGB),
	(int)"PRG", (int)strlen("PRG"), (int)game->PRG, (int)sizeof(game->PRG),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new ZigZag;
	game->init(rate);
	return geometry.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *render() {
	game->updateStatus()->updateInput()->execute()->makeBitmap(data.data());
	return data.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void update() {
	game->sound0->update();
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	fill(sample.begin(), sample.end(), 0);
	game->sound0->makeSound(sample.data(), sample.size());
	return sample.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void reset() {
	game->reset();
}

extern "C" EMSCRIPTEN_KEEPALIVE void coin() {
	game->coin();
}

extern "C" EMSCRIPTEN_KEEPALIVE void start1P() {
	game->start1P();
}

extern "C" EMSCRIPTEN_KEEPALIVE void start2P() {
	game->start2P();
}

extern "C" EMSCRIPTEN_KEEPALIVE void up(int fDown) {
	game->up(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void right(int fDown) {
	game->right(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void down(int fDown) {
	game->down(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(int fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(int fDown) {
	game->triggerA(fDown != 0);
}

