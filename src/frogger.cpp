/*
 *
 *	Frogger
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "frogger.h"
using namespace std;

unsigned char Frogger::BG[0x1000], Frogger::RGB[0x20], Frogger::PRG1[0x3000], Frogger::PRG2[0x1800];
AY_3_8910 *Frogger::sound0;

Frogger *game;
vector<int> rom_table = {
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"RGB", (int)strlen("RGB"), (int)game->RGB, (int)sizeof(game->RGB),
	(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1, (int)sizeof(game->PRG1),
	(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2, (int)sizeof(game->PRG2),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new Frogger;
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

extern "C" EMSCRIPTEN_KEEPALIVE void down(bool fDown) {
	game->down(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(bool fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(bool fDown) {
	game->triggerA(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerB(bool fDown) {
	game->triggerB(fDown != 0);
}

