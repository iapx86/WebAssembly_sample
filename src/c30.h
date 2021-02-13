/*
 * C30
 */

#ifndef C30_H
#define C30_H

#include <array>
using namespace std;

struct C30 {
	double clock;
	double gain;
	double output = 0;
	array<uint8_t, 0x400> ram = {};
	array<double, 0x200> snd = {};
	double frac = 0;
	struct {
		uint32_t phase = 0;
		int ncount = 0;
		int rng = 1;
		int output = 0;
	} channel[8];

	C30(double clock = 48000, double gain = 0.1) {
		this->clock = clock;
		this->gain = gain;
	}

	int read(int addr) {
		return ram[addr & 0x3ff];
	}

	void write(int addr, int data) {
		ram[addr &= 0x3ff] = data;
		addr < 0x100 && (snd[addr * 2] = (data >> 4) * 2 / 15.0 - 1, snd[1 + addr * 2] = (data & 15) * 2 / 15.0 - 1);
	}

	void execute(double rate, double rate_correction = 1) {
		for (frac += clock * rate_correction; frac >= rate; frac -= rate) {
			auto& r = ram;
			for (int ch = 0x100; ch < 0x140; ch += 8)
				if (ch >= 0x120 || ~r[0x104 | ch - 8 & 0x38] & 0x80) {
					const int ph = (r[ch | 5] << 16 | r[ch | 6] << 8 | r[ch | 7]) + (r[ch | 1] << 16 & 0xf0000 | r[ch | 2] << 8 | r[ch | 3]);
					r[ch | 5] = ph >> 16, r[ch | 6] = ph >> 8, r[ch | 7] = ph;
				} else {
					for (channel[ch >> 3 & 3].ncount += r[ch | 3]; channel[ch >> 3 & 3].ncount >= 0x100; channel[ch >> 3 & 3].ncount -= 0x100) {
						channel[ch >> 3 & 3].output ^= channel[ch >> 3 & 3].rng + 1 >> 1 & 1;
						channel[ch >> 3 & 3].rng = (channel[ch >> 3 & 3].rng ^ (~channel[ch >> 3 & 3].rng & 1) - 1 & 0x28000) >> 1;
					}
				}
		}
	}

	void update() {
		auto& r = ram;
		output = 0;
		for (int ch = 0x100; ch < 0x140; ch += 8)
			if (ch >= 0x120 || ~r[0x104 | ch - 8 & 0x38] & 0x80)
				output += snd[r[ch | 1] << 1 & 0x1e0 | r[ch | 5] & 0x1f] * (r[ch | 0] & 15) / 15.0 * gain;
			else
				output += (channel[ch >> 3 & 3].output * 2 - 1) * (r[ch | 0] & 15) / 15.0 * gain;
	}
};

#endif //C30_H
