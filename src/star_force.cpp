/*
 *
 *	Star Force
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "star_force.h"
using namespace std;

unsigned char StarForce::PRG1[0x8000], StarForce::PRG2[0x2000], StarForce::FG[0x3000], StarForce::BG1[0x6000];
unsigned char StarForce::BG2[0x6000], StarForce::BG3[0x3000], StarForce::OBJ[0xc000], StarForce::SND[0x20];
SN76489 *StarForce::sound0, *StarForce::sound1, *StarForce::sound2;
SenjyoSound *StarForce::sound3;

StarForce *game;
vector<int> rom_table = {
	(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1, (int)sizeof(game->PRG1),
	(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2, (int)sizeof(game->PRG2),
	(int)"FG", (int)strlen("FG"), (int)game->FG, (int)sizeof(game->FG),
	(int)"BG1", (int)strlen("BG1"), (int)game->BG1, (int)sizeof(game->BG1),
	(int)"BG2", (int)strlen("BG2"), (int)game->BG2, (int)sizeof(game->BG2),
	(int)"BG3", (int)strlen("BG3"), (int)game->BG3, (int)sizeof(game->BG3),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"SND", (int)strlen("SND"), (int)game->SND, (int)sizeof(game->SND),
	0
};
vector<int> geometry = {game->cxScreen, game->cyScreen, game->width, game->height, game->xOffset, game->yOffset, game->rotate};
vector<int> data(game->width * game->height);
vector<float> sample(512);

extern "C" EMSCRIPTEN_KEEPALIVE int *roms() {
	return rom_table.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *init(int rate) {
	game = new StarForce;
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
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	fill(sample.begin(), sample.end(), 0);
	game->sound0->makeSound(sample.data(), sample.size());
	game->sound1->makeSound(sample.data(), sample.size());
	game->sound2->makeSound(sample.data(), sample.size());
	game->sound3->makeSound(sample.data(), sample.size());
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

extern "C" EMSCRIPTEN_KEEPALIVE void down(bool fDown) {
	game->down(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(bool fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(bool fDown) {
	game->triggerA(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerB(bool fDown) {
	game->triggerB(fDown != 0);
}

