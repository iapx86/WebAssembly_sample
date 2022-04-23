/*
 * AY-3-8910
 */

#ifndef AY_3_8910_H
#define AY_3_8910_H

#include <cmath>
#include <array>
using namespace std;

struct AY_3_8910 {
	int rate;
	double gain;
	double output = 0;
	bool mute = false;
	array<uint8_t, 0x10> reg = {};
	int frac = 0;
	struct {
		int freq = 0;
		int count = 0;
		int output = 0;
	} channel[3];
	int ncount = 0;
	int rng = 0xffff;
	int ecount = 0;
	int step = 0;

	AY_3_8910(int clock, double gain = 0.1) {
		rate = clock / 8;
		this->gain = gain;
	}

	void control(bool flag) {
		mute = !flag;
	}

	int read(int addr) {
		return reg[addr & 15];
	}

	void write(int addr, int data) {
		reg[addr &= 15] = data, addr == 13 && (step = 0);
	}

	void execute(int rate) {
		for (frac += this->rate; frac >= rate; frac -= rate) {
			const int nfreq = reg[6] & 31, efreq = reg[11] | reg[12] << 8, etype = reg[13];
			for (int i = 0; i < 3; i++) {
				auto& ch = channel[i];
				ch.freq = reg[1 + i * 2] << 8 & 0xf00 | reg[i * 2], ++ch.count >= ch.freq && (ch.output = ~ch.output, ch.count = 0);
			}
			++ncount >= nfreq << 1 && (rng = (rng >> 16 ^ rng >> 13 ^ 1) & 1 | rng << 1, ncount = 0);
			++ecount >= efreq && (step += ((step < 16) | etype >> 3 & ~etype & 1) - (step >= 47) * 32, ecount = 0);
		}
	}

	void update() {
		static bool initialized = false;
		static array<double, 16> vol = {};
		if (!initialized) {
			for (int i = 0; i < 16; i++)
				vol[i] = i ? pow(10, (i - 15) / 10.0) : 0;
			initialized = true;
		}
		output = 0;
		if (mute)
			return;
		const int etype = reg[13];
		const int evol = (~step ^ ((((etype ^ etype >> 1) & step >> 4 ^ ~etype >> 2) & 1) - 1)) & (~etype >> 3 & step >> 4 & 1) - 1 & 15;
		for (int i = 0; i < 3; i++) {
			auto& ch = channel[i];
			output += (((!ch.freq | reg[7] >> i | ch.output) & (reg[7] >> i + 3 | rng) & 1) * 2 - 1) * vol[reg[8 + i] >> 4 & 1 ? evol : reg[8 + i] & 15] * gain;
		}
	}
};

#endif //AY_3_8910_H
