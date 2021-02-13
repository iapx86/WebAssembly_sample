/*
 * Senjyo Sound
 */

#ifndef SENJYO_SOUND_H
#define SENJYO_SOUND_H

#include <cmath>
#include <algorithm>
#include <array>
using namespace std;

struct BiquadFilter {
	double b0 = 0;
	double b1 = 0;
	double b2 = 0;
	double a1 = 0;
	double a2 = 0;
	double x1 = 0;
	double x2 = 0;
	double y1 = 0;
	double y2 = 0;

	void bandpass(double freq, double Q, int sampleRate) {
		const double w0 = 2 * M_PI * freq / sampleRate, alpha = sin(w0) / (2 * Q), a0 = 1 + alpha;
		b0 = alpha / a0, b1 = 0, b2 = -alpha / a0, a1 = -2 * cos(w0) / a0, a2 = (1 - alpha) / a0;
	}

	double filter(double x) {
		const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
		x2 = x1, x1 = x, y2 = y1, y1 = y;
		return y;
	}
};

struct SenjyoSound {
	const uint8_t *snd;
	double rate;
	double gain;
	double output = 0;
	double frac = 0;
	struct {
		double vol = 0;
		int freq = 256;
		int count = 256;
		int phase = 0;
	} channel;
	BiquadFilter bq;

	SenjyoSound(const array<uint8_t, 0x20>& SND, double clock, int sampleRate = 48000, double gain = 0.7) {
		snd = SND.data();
		rate = clock / 16;
		this->gain = gain;
		bq.bandpass(200, 5, sampleRate);
	}

	void write(int addr, int data) {
		addr ? (channel.vol = data / 15.0) : (channel.count = channel.freq = data);
	}

	void execute(double rate, double rate_correction) {
		for (frac += this->rate * rate_correction; frac >= rate; frac -= rate)
			--channel.count <= 0 && (channel.count = channel.freq, channel.phase = channel.phase + 1 & 15);
	}

	void update() {
		output = bq.filter((snd[channel.phase] * 2 / (double)0xbf - 1) * channel.vol) * gain;
	}
};

#endif //SENJYO_SOUND_H
