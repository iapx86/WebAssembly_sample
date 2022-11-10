/*
 *
 *	Pac-Land
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "pac-land.hpp"
using namespace std;

PacLand *game;
Timer audio(48000);
list<float> samples;

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

extern "C" EMSCRIPTEN_KEEPALIVE void init(int rate) {
	game = new PacLand;
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

extern "C" EMSCRIPTEN_KEEPALIVE void test() {
	if ((game->fTest = !game->fTest) != false)
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

