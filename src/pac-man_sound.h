/*
 * Pac-Man Sound
 */

#ifndef PAC_MAN_SOUND_H
#define PAC_MAN_SOUND_H

#include <array>
using namespace std;

struct PacManSound {
	const uint8_t *snd;
	int clock;
	double gain;
	double output = 0;
	bool mute = false;
	array<uint8_t, 0x20> reg = {};
	int frac;

	PacManSound(const array<uint8_t, 0x100>& SND, int clock = 96000, double gain = 0.1) {
		snd = SND.data();
		this->clock = clock;
		this->gain = gain;
	}

	void control(bool flag) {
		mute = !flag;
	}

	void write(int addr, int data) {
		reg[addr & 0x1f] = data & 15;
	}

	void execute(int rate) {
		for (frac += clock; frac >= rate; frac -= rate) {
			auto& r = reg;
			for (int ch = 0; ch < 15; ch += 5) {
				int ph = r[ch + 4] << 16 | r[ch + 3] << 12 | r[ch + 2] << 8 | r[ch + 1] << 4 | (ch ? 0 : r[0]);
				ph += r[ch + 0x14] << 16 | r[ch + 0x13] << 12 | r[ch + 0x12] << 8 | r[ch + 0x11] << 4 | (ch ? 0 : r[0x10]);
				r[ch + 4] = ph >> 16 & 15, r[ch + 3] = ph >> 12 & 15, r[ch + 2] = ph >> 8, r[ch + 1] = ph >> 4 & 15, !ch && (r[0] = ph & 15);
			}
		}
	}

	void update() {
		output = 0;
		if (mute)
			return;
		auto& r = reg;
		for (int ch = 0; ch < 15; ch += 5)
			output += (snd[r[ch + 5] << 5 & 0xe0 | r[ch + 4] << 1 | r[ch + 3] >> 3] * 2 / 15.0 - 1) * r[ch + 0x15] / 15.0 * gain;
	}
};

#endif //PAC_MAN_SOUND_H
