/*
 *
 *	Chack'n Pop
 *
 */

#include <emscripten.h>
#include <array>
#include "chackn_pop.h"
using namespace std;

ChacknPop *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, ChacknPop::width * ChacknPop::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 6 * 4 + 1> rom_table = {
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RGB_L", (int)strlen("RGB_L"), (int)game->RGB_L.data(), (int)game->RGB_L.size(),
		(int)"RGB_H", (int)strlen("RGB_H"), (int)game->RGB_H.data(), (int)game->RGB_H.size(),
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new ChacknPop;
	game->init(rate);
	return geometry.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *render() {
	game->updateStatus()->updateInput()->execute()->makeBitmap(data.data());
	return data.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void update() {
	game->sound0->update();
	game->sound1->update();
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	sample.fill(0);
	game->sound0->makeSound(sample.data(), sample.size());
	game->sound1->makeSound(sample.data(), sample.size());
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

extern "C" EMSCRIPTEN_KEEPALIVE void triggerB(int fDown) {
	game->triggerB(fDown != 0);
}

AY_3_8910 *ChacknPop::sound0, *ChacknPop::sound1;

array<unsigned char, 0x4000> ChacknPop::BG = {
};

array<unsigned char, 0x4000> ChacknPop::OBJ = {
};

array<unsigned char, 0x400> ChacknPop::RGB_L = {
};

array<unsigned char, 0x400> ChacknPop::RGB_H = {
};

array<unsigned char, 0xa000> ChacknPop::PRG1 = {
};

array<unsigned char, 0x800> ChacknPop::PRG2 = {
};

