/*
 *
 *	Libble Rabble
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "libble_rabble.hpp"
using namespace std;

LibbleRabble *game;
Timer audio(48000);
list<float> samples;

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 11 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"PRG3", (int)strlen("PRG3"), (int)game->PRG3.data(), (int)game->PRG3.size(),
		(int)"BG", (int)strlen("BG"), (int)game->BG.data(), (int)game->BG.size(),
		(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ.data(), (int)game->OBJ.size(),
		(int)"RED", (int)strlen("RED"), (int)game->RED.data(), (int)game->RED.size(),
		(int)"GREEN", (int)strlen("GREEN"), (int)game->GREEN.data(), (int)game->GREEN.size(),
		(int)"BLUE", (int)strlen("BLUE"), (int)game->BLUE.data(), (int)game->BLUE.size(),
		(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR.data(), (int)game->BGCOLOR.size(),
		(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR.data(), (int)game->OBJCOLOR.size(),
		(int)"SND", (int)strlen("SND"), (int)game->SND.data(), (int)game->SND.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void init(int rate) {
	game = new LibbleRabble;
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

extern "C" EMSCRIPTEN_KEEPALIVE void up2(int fDown) {
	game->up2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void right2(int fDown) {
	game->right2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void down2(int fDown) {
	game->down2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left2(int fDown) {
	game->left2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(int fDown) {
	game->triggerA(fDown != 0);
}

MappySound *LibbleRabble::sound0;

array<unsigned char, 0x8000> LibbleRabble::PRG1 = {
};

array<unsigned char, 0x2000> LibbleRabble::PRG2 = {
};

array<unsigned char, 0x8000> LibbleRabble::PRG3 = {
};

array<unsigned char, 0x2000> LibbleRabble::BG = {
};

array<unsigned char, 0x4000> LibbleRabble::OBJ = {
};

array<unsigned char, 0x100> LibbleRabble::RED = {
};

array<unsigned char, 0x100> LibbleRabble::GREEN = {
};

array<unsigned char, 0x100> LibbleRabble::BLUE = {
};

array<unsigned char, 0x100> LibbleRabble::BGCOLOR = {
};

array<unsigned char, 0x200> LibbleRabble::OBJCOLOR = {
};

array<unsigned char, 0x100> LibbleRabble::SND = {
};

