/*
 *
 *	Pac-Land
 *
 */

#include <emscripten.h>
#include <array>
#include "pac-land.h"
using namespace std;

PacLand *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, PacLand::width * PacLand::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 11 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"PRG2I", (int)strlen("PRG2I"), (int)game->PRG2I.data(), (int)game->PRG2I.size(),
		(int)"FG", (int)strlen("FG"), (int)game->FG.data(), (int)game->FG.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RED", (int)strlen("RED"), (int)game->RED.data(), (int)game->RED.size(),
		(int)"BLUE", (int)strlen("BLUE"), (int)game->BLUE.data(), (int)game->BLUE.size(),
		(int)"FGCOLOR", (int)strlen("FGCOLOR"), (int)game->FGCOLOR.data(), (int)game->FGCOLOR.size(),
		(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR.data(), (int)game->BGCOLOR.size(),
		(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR.data(), (int)game->OBJCOLOR.size(),
		0
	};
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

extern "C" EMSCRIPTEN_KEEPALIVE void right(int fDown) {
	game->right(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(int fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(int fDown) {
	game->triggerA(fDown != 0);
}

C30 *PacLand::sound0;

array<unsigned char, 0x18000> PacLand::PRG1 = {
};

array<unsigned char, 0x2000> PacLand::PRG2 = {
};

array<unsigned char, 0x1000> PacLand::PRG2I = {
};

array<unsigned char, 0x2000> PacLand::FG = {
};

array<unsigned char, 0x2000> PacLand::BG = {
};

array<unsigned char, 0x10000> PacLand::OBJ = {
};

array<unsigned char, 0x400> PacLand::RED = {
};

array<unsigned char, 0x400> PacLand::BLUE = {
};

array<unsigned char, 0x400> PacLand::FGCOLOR = {
};

array<unsigned char, 0x400> PacLand::BGCOLOR = {
};

array<unsigned char, 0x400> PacLand::OBJCOLOR = {
};

