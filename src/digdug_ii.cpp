/*
 *
 *	DigDug II
 *
 */

#include <emscripten.h>
#include <array>
#include "digdug_ii.h"
using namespace std;

DigDugII *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, DigDugII::width * DigDugII::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 8 * 4 + 1> rom_table = {
		(int)"SND", (int)strlen("SND"), (int)game->SND.data(), (int)game->SND.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR.data(), (int)game->BGCOLOR.size(),
		(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR.data(), (int)game->OBJCOLOR.size(),
		(int)"RGB", (int)strlen("RGB"), (int)game->RGB.data(), (int)game->RGB.size(),
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new DigDugII;
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

MappySound *DigDugII::sound0;

array<unsigned char, 0x100> DigDugII::SND = {
};

array<unsigned char, 0x1000> DigDugII::BG = {
};

array<unsigned char, 0x8000> DigDugII::OBJ = {
};

array<unsigned char, 0x100> DigDugII::BGCOLOR = {
};

array<unsigned char, 0x100> DigDugII::OBJCOLOR = {
};

array<unsigned char, 0x20> DigDugII::RGB = {
};

array<unsigned char, 0x8000> DigDugII::PRG1 = {
};

array<unsigned char, 0x2000> DigDugII::PRG2 = {
};

