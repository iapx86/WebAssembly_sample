/*
 *
 *	Metro-Cross
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "metro-cross.h"
using namespace std;

MetroCross *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, MetroCross::width * MetroCross::height> data = {};
DoubleTimer audio;
list<float> samples;

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 8 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"PRG2I", (int)strlen("PRG2I"), (int)game->PRG2I.data(), (int)game->PRG2I.size(),
		(int)"FG", (int)strlen("FG"), (int)game->FG.data(), (int)game->FG.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"GREEN", (int)strlen("GREEN"), (int)game->GREEN.data(), (int)game->GREEN.size(),
		(int)"RED", (int)strlen("RED"), (int)game->RED.data(), (int)game->RED.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new MetroCross;
	game->init(audio.rate = rate);
	audio.fn = []() {
		samples.push_back(game->sound0->output);
		game->sound0->update();
	};
	return geometry.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *render(double timestamp, double rate_correction) {
	game->updateStatus()->updateInput()->execute(audio, rate_correction)->makeBitmap(::data.data());
	return ::data.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void *sound() {
	static vector<float> buf;
	static struct {
		float *addr;
		int size;
	} iov;
	buf.resize(samples.size());
	copy(samples.begin(), samples.end(), buf.begin());
	samples.clear();
	iov.addr = buf.data();
	iov.size = buf.size();
	return &iov;
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

C30 *MetroCross::sound0;

array<unsigned char, 0xa000> MetroCross::PRG1 = {
};

array<unsigned char, 0x2000> MetroCross::PRG2 = {
};

array<unsigned char, 0x1000> MetroCross::PRG2I = {
};

array<unsigned char, 0x2000> MetroCross::FG = {
};

array<unsigned char, 0xc000> MetroCross::BG = {
};

array<unsigned char, 0x8000> MetroCross::OBJ = {
};

array<unsigned char, 0x800> MetroCross::GREEN = {
};

array<unsigned char, 0x800> MetroCross::RED = {
};

