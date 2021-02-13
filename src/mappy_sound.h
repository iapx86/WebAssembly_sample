/*
 * Mappy Sound
 */

#ifndef MAPPY_SOUND_H
#define MAPPY_SOUND_H

#include <array>
using namespace std;

struct MappySound {
	const uint8_t *snd;
	double clock;
	double gain;
	double output = 0;
	bool mute = false;
	array<uint8_t, 0x400> ram = {};
	double frac = 0;

	MappySound(const array<uint8_t, 0x100>& SND, double clock = 48000, double gain = 0.1) {
		snd = SND.data();
		this->clock = clock;
		this->gain = gain;
	}

	void control(bool flag) {
		mute = !flag;
	}

	int read(int addr) {
		return ram[addr & 0x3ff];
	}

	void write(int addr, int data) {
		ram[addr & 0x3ff] = data;
	}

	void execute(double rate, double rate_correction = 1) {
		for (frac += clock * rate_correction; frac >= rate; frac -= rate) {
			auto& r = ram;
			for (int ch = 0; ch < 0x40; ch += 8) {
				const int ph = (r[ch | 2] << 16 | r[ch | 1] << 8 | r[ch | 0]) + (r[ch | 6] << 16 & 0xf0000 | r[ch | 5] << 8 | r[ch | 4]);
				r[ch | 2] = ph >> 16, r[ch | 1] = ph >> 8, r[ch | 0] = ph;
			}
		}
	}

	void update() {
		output = 0;
		if (mute)
			return;
		auto& r = ram;
		for (int ch = 0; ch < 0x40; ch += 8)
			output += (snd[r[ch | 6] << 1 & 0xe0 | r[ch | 2] & 0x1f] * 2 / 15.0 - 1) * (r[ch | 3] & 15) / 15.0 * gain;
	}
};

#endif //MAPPY_SOUND_H
