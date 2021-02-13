/*
 *
 *	Super Pac-Man
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "super_pac-man.h"
using namespace std;

SuperPacMan *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, SuperPacMan::width * SuperPacMan::height> data = {};
DoubleTimer audio;
list<float> samples;

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
	game = new SuperPacMan;
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

MappySound *SuperPacMan::sound0;

array<unsigned char, 0x100> SuperPacMan::SND = {
};

array<unsigned char, 0x1000> SuperPacMan::BG = {
};

array<unsigned char, 0x2000> SuperPacMan::OBJ = {
};

array<unsigned char, 0x100> SuperPacMan::BGCOLOR = {
};

array<unsigned char, 0x100> SuperPacMan::OBJCOLOR = {
};

array<unsigned char, 0x20> SuperPacMan::RGB = {
};

array<unsigned char, 0x4000> SuperPacMan::PRG1 = {
};

array<unsigned char, 0x1000> SuperPacMan::PRG2 = {
};

