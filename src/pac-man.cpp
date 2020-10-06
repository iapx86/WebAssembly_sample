/*
 *
 *	Pac-Man
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "pac-man.h"
using namespace std;

unsigned char PacMan::BG[0x1000], PacMan::COLOR[0x100], PacMan::OBJ[0x1000], PacMan::RGB[0x20];
unsigned char PacMan::PRG[0x4000], PacMan::SND[0x100];
PacManSound *PacMan::sound0;

PacMan *game;
vector<int> rom_table = {
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"COLOR", (int)strlen("COLOR"), (int)game->COLOR, (int)sizeof(game->COLOR),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"RGB", (int)strlen("RGB"), (int)game->RGB, (int)sizeof(game->RGB),
	(int)"PRG", (int)strlen("PRG"), (int)game->PRG, (int)sizeof(game->PRG),
	(int)"SND", (int)strlen("SND"), (int)game->SND, (int)sizeof(game->SND),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new PacMan;
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

extern "C" EMSCRIPTEN_KEEPALIVE void test() {
	if ((game->fTest = !game->fTest) != false)
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

