/*
 *
 *	Libble Rabble
 *
 */

#include <emscripten.h>
#include <cstring>
#include <vector>
#include "libble_rabble.h"
using namespace std;

unsigned char LibbleRabble::PRG1[0x8000], LibbleRabble::PRG2[0x2000], LibbleRabble::PRG3[0x8000], LibbleRabble::BG[0x2000];
unsigned char LibbleRabble::OBJ[0x4000], LibbleRabble::RED[0x100], LibbleRabble::GREEN[0x100], LibbleRabble::BLUE[0x100];
unsigned char LibbleRabble::BGCOLOR[0x100], LibbleRabble::OBJCOLOR[0x200], LibbleRabble::SND[0x100];
MappySound *LibbleRabble::sound0;

LibbleRabble *game;
vector<int> rom_table = {
	(int)"PRG1", (int)strlen("PRG1"), (int)game->PRG1, (int)sizeof(game->PRG1),
	(int)"PRG2", (int)strlen("PRG2"), (int)game->PRG2, (int)sizeof(game->PRG2),
	(int)"PRG3", (int)strlen("PRG3"), (int)game->PRG3, (int)sizeof(game->PRG3),
	(int)"BG", (int)strlen("BG"), (int)game->BG, (int)sizeof(game->BG),
	(int)"OBJ", (int)strlen("OBJ"), (int)game->OBJ, (int)sizeof(game->OBJ),
	(int)"RED", (int)strlen("RED"), (int)game->RED, (int)sizeof(game->RED),
	(int)"GREEN", (int)strlen("GREEN"), (int)game->GREEN, (int)sizeof(game->GREEN),
	(int)"BLUE", (int)strlen("BLUE"), (int)game->BLUE, (int)sizeof(game->BLUE),
	(int)"BGCOLOR", (int)strlen("BGCOLOR"), (int)game->BGCOLOR, (int)sizeof(game->BGCOLOR),
	(int)"OBJCOLOR", (int)strlen("OBJCOLOR"), (int)game->OBJCOLOR, (int)sizeof(game->OBJCOLOR),
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
	game = new LibbleRabble;
	game->init(rate);
	return geometry.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE int *render() {
	game->updateStatus()->updateInput()->execute()->makeBitmap(data.data());
	return data.data();
}

extern "C" EMSCRIPTEN_KEEPALIVE void update() {
	game->sound0->update();
}

extern "C" EMSCRIPTEN_KEEPALIVE float *sound() {
	fill(sample.begin(), sample.end(), 0);
	game->sound0->makeSound(sample.data(), sample.size());
	return sample.data();
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

extern "C" EMSCRIPTEN_KEEPALIVE void down(bool fDown) {
	game->down(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left(bool fDown) {
	game->left(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void up2(int fDown) {
	game->up2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void right2(int fDown) {
	game->right2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void down2(bool fDown) {
	game->down2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void left2(bool fDown) {
	game->left2(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerA(bool fDown) {
	game->triggerA(fDown != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void triggerB(bool fDown) {
	game->triggerB(fDown != 0);
}
