/*
 *
 *	SEGA Z80 Emulator
 *
 */

#ifndef SEGA_Z80_H
#define SEGA_Z80_H

#include <array>
#include <vector>
#include "z80.h"
#include "utils.h"
using namespace std;

struct SegaZ80 : Z80 {
	static array<uint8_t, 0x10000> index;
	array<uint8_t, 0x1100> code_table;
	array<uint8_t, 0x1100> data_table;

	SegaZ80(const vector<vector<uint8_t>>& key, int clock = 0) : Z80(clock) {
		for (int i = 0; i < 0x1000; i++) {
			code_table[i] = i & 0x57 | (i & 0x80 ? key[i >> 7 & ~1][bitswap(~i, {5, 3})] ^ 0xa8 : key[i >> 7 & ~1][bitswap(i, {5, 3})]);
			data_table[i] = i & 0x57 | (i & 0x80 ? key[i >> 7 | 1][bitswap(~i, {5, 3})] ^ 0xa8 : key[i >> 7 | 1][bitswap(i, {5, 3})]);
		}
		for (int i = 0x1000; i < 0x1100; i++)
			code_table[i] = data_table[i] = i & 0xff;
	}

	int fetchM1() override {
		const int addr = pc;
		return code_table[index[addr] << 8 | Z80::fetchM1()];
	}

	int fetch() override {
		const int addr = pc;
		return data_table[index[addr] << 8 | Z80::fetch()];
	}

	int read(int addr) override {
		return data_table[index[addr] << 8 | Z80::read(addr)];
	}

	static void init() {
		static bool initialized;
		if (initialized)
			return;
		initialized	= true;
		Z80::init();
		for (int i = 0; i < 0x10000; i++)
			index[i] = i & 0x8000 ? 0x10 : bitswap(i, {12, 8, 4, 0});
	}
};

#endif //SEGA_Z80_H
