/*
 * SN76489
 */

#ifndef SN76489_H
#define SN76489_H

#include <cmath>
#include <array>
using namespace std;

struct SN76489 {
	int rate;
	double gain;
	double output = 0;
	bool mute = false;
	int addr = 0;
	array<uint16_t, 8> reg;
	int frac = 0;
	struct {
		int count = 0;
		int output = 0;
	} channel[3];
	int ncount = 0;
	int rng = 0x4000;

	SN76489(int clock, double gain = 0.1) {
		rate = clock / 16;
		this->gain = gain;
		reg.fill(0xffff);
	}

	void control(bool flag) {
		mute = !flag;
	}

	void write(int data) {
		if (data & 0x80)
			addr = data >> 4 & 7, reg[addr] = reg[addr] & 0x3f0 | data & 0xf;
		else
			reg[addr] = reg[addr] & 0xf | data << 4 & 0x3f0;
		addr == 6 && (rng = 0x4000);
	}

	void execute(int rate) {
		for (frac += this->rate; frac >= rate; frac -= rate) {
			const int nfreq = (reg[6] & 3) == 3 ? reg[4] << 1 : 32 << (reg[6] & 3);
			for (int i = 0; i < 3; i++) {
				auto& ch = channel[i];
				!(--ch.count & 0x3ff) && (ch.output = ~ch.output, ch.count = reg[i * 2]);
			}
			!(--ncount & 0x7ff) && (rng = rng >> 1 | (rng << 14 ^ rng << 13 & reg[6] << 12) & 0x4000, ncount = nfreq);
		}
	}

	void update() {
		static bool initialized = false;
		static array<double, 16> vol = {};
		if (!initialized) {
			for (int i = 0; i < 16; i++)
				vol[i] = i < 15 ? pow(10, -i / 10.0) : 0;
			initialized = true;
		}
		output = 0;
		if (mute)
			return;
		for (int i = 0; i < 3; i++) {
			auto& ch = channel[i];
			output += ((ch.output & 1) * 2 - 1) * vol[reg[i * 2 + 1] & 15] * gain;
		}
		output += ((rng & 1) * 2 - 1) * vol[reg[7] & 15] * gain;
	}
};

#endif //SN76489_H
