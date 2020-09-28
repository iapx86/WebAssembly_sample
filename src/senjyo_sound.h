/*
 * Senjyo Sound
 */

#ifndef SENJYO_SOUND_H
#define SENJYO_SOUND_H

#include <cmath>
#include <list>
#include <mutex>
#include <utility>
#include <vector>
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
	float snd[0x20] = {};
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	int cycles = 0;
	struct {
		float vol = 0;
		int freq = 256;
		int count = 256;
		int phase = 0;
	} channel;
	BiquadFilter bq;

	SenjyoSound(uint8_t *SND, int clock, int sampleRate = 48000, int resolution = 1, float gain = 0.7) {
		for (int i = 0; i < 0x20; i++)
			snd[i] = SND[i] * 2 / (double)0xbf - 1;
		rate = clock / 16;
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
		bq.bandpass(200, 5, sampleRate);
	}

	void write(int addr, int data, int timer = 0) {
		tmpwheel[timer].push_back({addr, data});
	}

	void update() {
		mutex.lock();
		if (wheel.size() > resolution) {
			while (!wheel.empty()) {
				for (auto& e: wheel.front())
					regwrite(e);
				wheel.pop_front();
			}
			count = sampleRate - 1;
		}
		for (auto& e: tmpwheel)
			wheel.push_back(e);
		mutex.unlock();
		for (auto& e: tmpwheel)
			e.clear();
	}

	void makeSound(float *data, uint32_t length) {
		for (int i = 0; i < length; i++) {
			for (count += 60 * resolution; count >= sampleRate; count -= sampleRate)
				if (!wheel.empty()) {
					mutex.lock();
					for (auto& e: wheel.front())
						regwrite(e);
					wheel.pop_front();
					mutex.unlock();
				}
			data[i] += bq.filter(snd[channel.phase] * channel.vol * gain);
			for (cycles += rate; cycles >= sampleRate; cycles -= sampleRate)
				if (--channel.count <= 0)
					channel.count = channel.freq, channel.phase = channel.phase + 1 & 15;
		}
	}

	void regwrite(pair<int, int>& e) {
		int addr = e.first, data = e.second;
		if (addr)
			channel.vol = data / 15.0;
		else
			channel.count = channel.freq = data;
	}
};

#endif //SENJYO_SOUND_H
