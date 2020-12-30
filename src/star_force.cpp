/*
 *
 *	Star Force
 *
 */

#include <emscripten.h>
#include <array>
#include "star_force.h"
using namespace std;

StarForce *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, StarForce::width * StarForce::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 8 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"FG", (int)strlen("FG"), (int)game->FG.data(), (int)game->FG.size(),
		(int)"BG1", (int)strlen("BG1"), (int)game->BG1.data(), (int)game->BG1.size(),
		(int)"BG2", (int)strlen("BG2"), (int)game->BG2.data(), (int)game->BG2.size(),
		(int)"BG3", (int)strlen("BG3"), (int)game->BG3.data(), (int)game->BG3.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"SND", (int)strlen("SND"), (int)game->SND.data(), (int)game->SND.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new StarForce;
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
	game->sound2->update();
	game->sound3->update();
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	sample.fill(0);
	game->sound0->makeSound(sample.data(), sample.size());
	game->sound1->makeSound(sample.data(), sample.size());
	game->sound2->makeSound(sample.data(), sample.size());
	game->sound3->makeSound(sample.data(), sample.size());
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

SN76489 *StarForce::sound0, *StarForce::sound1, *StarForce::sound2;
SenjyoSound *StarForce::sound3;

array<unsigned char, 0x8000> StarForce::PRG1 = {
};

array<unsigned char, 0x2000> StarForce::PRG2 = {
};

array<unsigned char, 0x3000> StarForce::FG = {
};

array<unsigned char, 0x6000> StarForce::BG1 = {
};

array<unsigned char, 0x6000> StarForce::BG2 = {
};

array<unsigned char, 0x3000> StarForce::BG3 = {
};

array<unsigned char, 0xc000> StarForce::OBJ = {
};

array<unsigned char, 0x20> StarForce::SND = {
};

