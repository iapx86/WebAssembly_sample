/*
 * K005289
 */

#ifndef K005289_H
#define K005289_H

#include <algorithm>
#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct K005289 {
	float snd[0x200] = {};
	double rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	uint16_t reg[4] = {};
	uint32_t phase[2] = {};

	K005289(uint8_t *SND, int clock, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		for (int i = 0; i < 0x200; i++)
			snd[i] = (SND[i] & 0xf) * 2 / 15.0 - 1;
		rate = (double)clock / sampleRate * (1 << 27);
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
	}

	void write(int addr, int data, int timer = 0) {
		tmpwheel[timer].push_back({addr, data});
	}

	void update() {
		mutex.lock();
		if (wheel.size() > resolution) {
			while (!wheel.empty())
				for_each(wheel.front().begin(), wheel.front().end(), [&](pair<int, int>& e) { reg[e.first] = e.second; }), wheel.pop_front();
			count = sampleRate - 1;
		}
		wheel.insert(wheel.end(), tmpwheel.begin(), tmpwheel.end());
		mutex.unlock();
		for_each(tmpwheel.begin(), tmpwheel.end(), [](list<pair<int, int>>& e) { e.clear(); });
	}

	void makeSound(float *data, uint32_t length) {
		for (int i = 0; i < length; i++) {
			for (count += 60 * resolution; count >= sampleRate; count -= sampleRate)
				if (!wheel.empty()) {
					mutex.lock();
					for_each(wheel.front().begin(), wheel.front().end(), [&](pair<int, int>& e) { reg[e.first] = e.second; }), wheel.pop_front();
					mutex.unlock();
				}
			for (int j = 0; j < 2; j++)
				if (reg[j + 2]) {
					data[i] += snd[j << 8 | reg[j] & 0xe0 | phase[j] >> 27] * (reg[j] & 0xf) / 15 * gain;
					phase[j] += (uint32_t)(rate / reg[j + 2]);
				}
		}
	}
};

#endif //K005289_H
