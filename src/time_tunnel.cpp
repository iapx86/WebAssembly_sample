/*
 *
 *	Time Tunnel
 *
 */

#include <emscripten.h>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include "time_tunnel.h"
using namespace std;

TimeTunnel *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, TimeTunnel::width * TimeTunnel::height> data = {};
DoubleTimer audio;
list<float> samples;

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 4 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"GFX", (int)strlen("GFX"), (int)game->GFX.data(), (int)game->GFX.size(),
		(int)"PRI", (int)strlen("PRI"), (int)game->PRI.data(), (int)game->PRI.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new TimeTunnel();
	game->init(audio.rate = rate);
	audio.fn = []() {
		samples.push_back(game->sound0->output + game->sound1->output + game->sound2->output + game->sound3->output + game->sound4->output);
		game->sound0->update();
		game->sound1->update();
		game->sound2->update();
		game->sound3->update();
		game->sound4->update();
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
		float *addr = nullptr;
		int size = 0;
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

AY_3_8910 *TimeTunnel::sound0, *TimeTunnel::sound1, *TimeTunnel::sound2, *TimeTunnel::sound3;
Dac1Ch *TimeTunnel::sound4;

array<unsigned char, 0xa000> TimeTunnel::PRG1 = {
};

array<unsigned char, 0x1000> TimeTunnel::PRG2 = {
};

array<unsigned char, 0x8000> TimeTunnel::GFX = {
};

array<unsigned char, 0x100> TimeTunnel::PRI = {
};

