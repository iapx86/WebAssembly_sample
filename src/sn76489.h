/*
 * SN76489
 */

#ifndef SN76489_H
#define SN76489_H

#include <array>
using namespace std;

struct SN76489 {
	double rate;
	double gain;
	double output = 0;
	bool mute = false;
	int addr = 0;
	array<uint16_t, 8> reg;
	int frac = 0;
	struct {
		int freq = 0;
		int count = 0;
		int output = 0;
	} channel[3];
	int ncount = 0;
	int rng = 0x4000;

	SN76489(double clock, double gain = 0.1) {
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

	void execute(double rate, double rate_correction) {
		for (frac += this->rate * rate_correction; frac >= rate; frac -= rate) {
			for (int i = 0; i < 3; i++) {
				auto& ch = channel[i];
				ch.freq = reg[i * 2], !(--ch.count & 0x3ff) && (ch.output = ~ch.output, ch.count = ch.freq);
			}
			const int nfreq = (reg[6] & 3) == 3 ? channel[2].freq << 1 : 32 << (reg[6] & 3);
			!(--ncount & 0x7ff) && (rng = rng >> 1 | (rng << 14 ^ rng << 13 & reg[6] << 12) & 0x4000, ncount = nfreq);
		}
	}

	void update() {
		output = 0;
		if (mute)
			return;
		const int nvol = ~reg[7] & 0xf;
		for (int i = 0; i < 3; i++) {
			auto& ch = channel[i];
			const int vol = ~reg[i * 2 + 1] & 0xf;
			output += ((ch.output & 1) * 2 - 1) * (vol ? pow(10, (vol - 15) / 10.0) : 0.0) * gain;
		}
		output += ((rng & 1) * 2 - 1) * (nvol ? pow(10, (nvol - 15) / 10.0) : 0.0) * gain;
	}
};

#endif //SN76489_H
