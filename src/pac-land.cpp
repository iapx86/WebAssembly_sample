/*
 *
 *	Pac-Land
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "pac-land.h"
using namespace std;

unsigned char PacLand::PRG1[0x18000], PacLand::PRG2[0x2000], PacLand::PRG2I[0x1000];
unsigned char PacLand::FG[0x2000], PacLand::BG[0x2000], PacLand::OBJ[0x10000], PacLand::RED[0x400];
unsigned char PacLand::BLUE[0x400], PacLand::FGCOLOR[0x400], PacLand::BGCOLOR[0x400], PacLand::OBJCOLOR[0x400];
C30 *PacLand::sound0;

PacLand *game;
vector<int> rom_table = {
	(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1, (int)sizeof(game->PRG1),
	(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2, (int)sizeof(game->PRG2),
	(int)"PRG2I", (int)strlen("PRG2I"), (int)game->PRG2I, (int)sizeof(game->PRG2I),
	(int)"FG", (int)strlen("FG"), (int)game->FG, (int)sizeof(game->FG),
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"RED", (int)strlen("RED"), (int)game->RED, (int)sizeof(game->RED),
	(int)"BLUE", (int)strlen("BLUE"), (int)game->BLUE, (int)sizeof(game->BLUE),
	(int)"FGCOLOR", (int)strlen("FGCOLOR"), (int)game->FGCOLOR, (int)sizeof(game->FGCOLOR),
	(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR, (int)sizeof(game->BGCOLOR),
	(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR, (int)sizeof(game->OBJCOLOR),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new PacLand;
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

