/*
 *
 *	Vulgus
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "vulgus.h"
using namespace std;

Vulgus *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, Vulgus::width * Vulgus::height> data = {};
DoubleTimer audio;
list<float> samples;

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 11 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"FG", (int)strlen("FG"), (int)game->FG.data(), (int)game->FG.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RED", (int)strlen("RED"), (int)game->RED.data(), (int)game->RED.size(),
		(int)"GREEN", (int)strlen("GREEN"), (int)game->GREEN.data(), (int)game->GREEN.size(),
		(int)"BLUE", (int)strlen("BLUE"), (int)game->BLUE.data(), (int)game->BLUE.size(),
		(int)"FGCOLOR", (int)strlen("FGCOLOR"), (int)game->FGCOLOR.data(), (int)game->FGCOLOR.size(),
		(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR.data(), (int)game->BGCOLOR.size(),
		(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR.data(), (int)game->OBJCOLOR.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new Vulgus;
	game->init(audio.rate = rate);
	audio.fn = []() {
		samples.push_back(game->sound0->output + game->sound1->output);
		game->sound0->update();
		game->sound1->update();
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

extern "C" EMSCRIPTEN_KEEPALIVE void triggerY(int fDown) {
	game->triggerY(fDown != 0);
}

AY_3_8910 *Vulgus::sound0, *Vulgus::sound1;

array<unsigned char, 0xa000> Vulgus::PRG1 = {
};

array<unsigned char, 0x2000> Vulgus::PRG2 = {
};

array<unsigned char, 0x2000> Vulgus::FG = {
};

array<unsigned char, 0xc000> Vulgus::BG = {
};

array<unsigned char, 0x8000> Vulgus::OBJ = {
};

array<unsigned char, 0x100> Vulgus::RED = {
};

array<unsigned char, 0x100> Vulgus::GREEN = {
};

array<unsigned char, 0x100> Vulgus::BLUE = {
};

array<unsigned char, 0x100> Vulgus::FGCOLOR = {
};

array<unsigned char, 0x100> Vulgus::BGCOLOR = {
};

array<unsigned char, 0x100> Vulgus::OBJCOLOR = {
};

