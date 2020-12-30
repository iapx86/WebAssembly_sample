/*
 *
 *	Sea Fighter Poseidon
 *
 */

#include <emscripten.h>
#include <array>
#include <vector>
#include "sea_fighter_poseidon.h"
using namespace std;

SeaFighterPoseidon *game;
array<int, 7> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
array<int, SeaFighterPoseidon::width * SeaFighterPoseidon::height> data = {};
array<float, 512> sample = {};

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	static array<int, 5 * 4 + 1> rom_table = {
		(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1.data(), (int)game->PRG1.size(),
		(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2.data(), (int)game->PRG2.size(),
		(int)"PRG3", (int)strlen("PRG3"), (int)game->PRG3.data(), (int)game->PRG3.size(),
		(int)"GFX", (int)strlen("GFX"), (int)game->GFX.data(), (int)game->GFX.size(),
		(int)"PRI", (int)strlen("PRI"), (int)game->PRI.data(), (int)game->PRI.size(),
		0
	};
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new SeaFighterPoseidon(rate);
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
	game->sound4->update();
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	sample.fill(0);
	game->sound0->makeSound(sample.data(), sample.size());
	game->sound1->makeSound(sample.data(), sample.size());
	game->sound2->makeSound(sample.data(), sample.size());
	game->sound3->makeSound(sample.data(), sample.size());
	game->sound4->makeSound(sample.data(), sample.size());
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

AY_3_8910 *SeaFighterPoseidon::sound0, *SeaFighterPoseidon::sound1, *SeaFighterPoseidon::sound2, *SeaFighterPoseidon::sound3;
SoundEffect *SeaFighterPoseidon::sound4;

vector<int> SeaFighterPoseidon::pcmtable;
vector<vector<short>> SeaFighterPoseidon::pcm;

array<unsigned char, 0xa000> SeaFighterPoseidon::PRG1 = {
};

array<unsigned char, 0x2000> SeaFighterPoseidon::PRG2 = {
};

array<unsigned char, 0x800> SeaFighterPoseidon::PRG3 = {
};

array<unsigned char, 0x8000> SeaFighterPoseidon::GFX = {
};

array<unsigned char, 0x100> SeaFighterPoseidon::PRI = {
};

