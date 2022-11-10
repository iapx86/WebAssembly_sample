/*
 *
 *	Zig Zag
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "zigzag.hpp"
using namespace std;

ZigZag *game;
Timer audio(48000);
list<float> samples;

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 4 * 4 + 1> rom_table = {
		(int)"PRG", (int)strlen("PRG"), (int)game->PRG.data(), (int)game->PRG.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RGB", (int)strlen("RGB"), (int)game->RGB.data(), (int)game->RGB.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void init(int rate) {
	game = new ZigZag;
	game->init(audio.rate = rate);
	game->updateStatus()->updateInput();
	audio.fn = []() {
		samples.push_back(game->sound0->output);
		game->sound0->update();
	};
}

extern "C" EMSCRIPTEN_KEEPALIVE int execute(double, int length) {
	game->execute(audio, length - samples.size());
	return samples.size();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *geometry() {
	static array<int, 7> buf;
	buf = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
	return buf.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *image() {
	return game->makeBitmap(false);
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	static vector<float> buf;
	buf.resize(samples.size());
	copy(samples.begin(), samples.end(), buf.begin());
	samples.clear();
	return buf.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void reset() {
	game->reset();
}

extern "C" EMSCRIPTEN_KEEPALIVE void coin(int fDown) {
	game->coin(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void start1P(int fDown) {
	game->start1P(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void start2P(int fDown) {
	game->start2P(fDown != 0);
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

AY_3_8910 *ZigZag::sound0;

array<unsigned char, 0x4000> ZigZag::PRG = {
};

array<unsigned char, 0x1000> ZigZag::BG = {
};

array<unsigned char, 0x1000> ZigZag::OBJ = {
};

array<unsigned char, 0x20> ZigZag::RGB = {
};

