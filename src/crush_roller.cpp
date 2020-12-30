/*
 *
 *	Crush Roller
 *
 */

#include <emscripten.h>
#include <array>
#include "crush_roller.h"
using namespace std;

CrushRoller *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, CrushRoller::width * CrushRoller::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 6 * 4 + 1> rom_table = {
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"COLOR", (int)strlen("COLOR"), (int)game->COLOR.data(), (int)game->COLOR.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RGB", (int)strlen("RGB"), (int)game->RGB.data(), (int)game->RGB.size(),
		(int)"PRG", (int)strlen("PRG"), (int)game->PRG.data(), (int)game->PRG.size(),
		(int)"SND", (int)strlen("SND"), (int)game->SND.data(), (int)game->SND.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new CrushRoller;
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
	sample.fill(0);
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

PacManSound *CrushRoller::sound0;

array<unsigned char, 0x1000> CrushRoller::BG = {
};

array<unsigned char, 0x100> CrushRoller::COLOR = {
};

array<unsigned char, 0x1000> CrushRoller::OBJ = {
};

array<unsigned char, 0x20> CrushRoller::RGB = {
};

array<unsigned char, 0x4000> CrushRoller::PRG = {
};

array<unsigned char, 0x100> CrushRoller::SND = {
};

