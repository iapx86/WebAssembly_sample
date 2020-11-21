/*
 *
 *	Sea Fighter Poseidon
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "sea_fighter_poseidon.h"
using namespace std;

vector<int> SeaFighterPoseidon::pcmtable = {0xa26, 0xa34, 0xa73, 0xa81, 0xa8f, 0xaa5, 0xaeb, 0xb09};
vector<vector<short>> SeaFighterPoseidon::pcm;
int SeaFighterPoseidon::pcm_freq;
unsigned char SeaFighterPoseidon::PRG1[0xa000], SeaFighterPoseidon::PRG2[0x2000], SeaFighterPoseidon::PRG3[0x800];
unsigned char SeaFighterPoseidon::GFX[0x8000], SeaFighterPoseidon::PRI[0x100];
AY_3_8910 *SeaFighterPoseidon::sound0, *SeaFighterPoseidon::sound1, *SeaFighterPoseidon::sound2, *SeaFighterPoseidon::sound3;
SoundEffect *SeaFighterPoseidon::sound4;

SeaFighterPoseidon *game;
vector<int> rom_table = {
	(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1, (int)sizeof(game->PRG1),
	(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2, (int)sizeof(game->PRG2),
	(int)"PRG3", (int)strlen("PRG3"), (int)game->PRG3, (int)sizeof(game->PRG3),
	(int)"GFX", (int)strlen("GFX"), (int)game->GFX, (int)sizeof(game->GFX),
	(int)"PRI", (int)strlen("PRI"), (int)game->PRI, (int)sizeof(game->PRI),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new SeaFighterPoseidon(48000);
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
	fill(sample.begin(), sample.end(), 0);
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

