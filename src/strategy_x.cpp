/*
 *
 *	Strategy X
 *
 */

#include <emscripten.h>
#include <array>
#include "strategy_x.h"
using namespace std;

StrategyX *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, StrategyX::width * StrategyX::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 5 * 4 + 1> rom_table = {
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"RGB", (int)strlen("RGB"), (int)game->RGB.data(), (int)game->RGB.size(),
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"MAP", (int)strlen("MAP"), (int)game->MAP.data(), (int)game->MAP.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new StrategyX;
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

extern "C" EMSCRIPTEN_KEEPALIVE void triggerX(int fDown) {
	game->triggerX(fDown != 0);
}

AY_3_8910 *StrategyX::sound0, *StrategyX::sound1;

array<unsigned char, 0x1000> StrategyX::BG = {
};

array<unsigned char, 0x20> StrategyX::RGB = {
};

array<unsigned char, 0x6000> StrategyX::PRG1 = {
};

array<unsigned char, 0x2000> StrategyX::PRG2 = {
};

array<unsigned char, 0x20> StrategyX::MAP = {
};

