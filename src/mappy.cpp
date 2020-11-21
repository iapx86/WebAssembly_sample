/*
 *
 *	Mappy
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "mappy.h"
using namespace std;

unsigned char Mappy::SND[0x100], Mappy::BG[0x1000], Mappy::OBJ[0x4000], Mappy::BGCOLOR[0x100];
unsigned char Mappy::OBJCOLOR[0x100], Mappy::RGB[0x20], Mappy::PRG1[0x6000], Mappy::PRG2[0x2000];
MappySound *Mappy::sound0;

Mappy *game;
vector<int> rom_table = {
	(int)"SND", (int)strlen("SND"), (int)game->SND, (int)sizeof(game->SND),
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR, (int)sizeof(game->BGCOLOR),
	(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR, (int)sizeof(game->OBJCOLOR),
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
	game = new Mappy;
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

extern "C" EMSCRIPTEN_KEEPALIVE void right(int fDown) {
	game->right(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(int fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(int fDown) {
	game->triggerA(fDown != 0);
}

