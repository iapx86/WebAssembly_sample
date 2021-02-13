/*
 * K005289
 */

#ifndef K005289_H
#define K005289_H

#include <array>
using namespace std;

struct K005289 {
	const uint8_t *snd;
	double clock;
	double gain;
	double output = 0;
	array<uint16_t, 8> reg = {};
	double frac = 0;

	K005289(const array<uint8_t, 0x200>& SND, double clock, double gain = 0.1) {
		snd = SND.data();
		this->clock = clock;
		this->gain = gain;
	}

	void write(int addr, int data) {
		reg[addr] = data;
	}

	void execute(double rate, double rate_correction = 1) {
		for (frac += clock * rate_correction; frac >= rate; frac -= rate)
			for (int i = 0; i < 2; i++)
				++reg[i + 4] >= reg[i + 2] && (++reg[i + 6], reg[i + 4] = 0);
	}

	void update() {
		output = 0;
		for (int i = 0; i < 2; i++)
			output += (snd[i << 8 | reg[i] & 0xe0 | reg[i + 6] & 0x1f] * 2 / 15.0 - 1) * (reg[i] & 15) / 15.0 * gain;
	}
};

#endif //K005289_H
