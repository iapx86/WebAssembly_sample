/*
 * SN76489
 */

#ifndef SN76489_H
#define SN76489_H

#include <algorithm>
#include <list>
#include <mutex>
#include <vector>
using namespace std;

struct SN76489 {
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<int>> tmpwheel;
	list<list<int>> wheel;
	mutex mutex;
	bool enable = true;
	int addr = 0;
	uint16_t reg[8];
	int cycles = 0;
	struct {
		int freq = 0;
		int count = 0;
		int output = 0;
	} channel[3];
	int ncount = 0;
	int rng = 0x4000;

	SN76489(int clock, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		rate = clock / 16;
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
		fill_n(reg, sizeof(reg) / sizeof(uint16_t), 0xffff);
	}

	void mute(bool flag) {
		enable = !flag;
	}

	void write(int data, int timer = 0) {
		tmpwheel[timer].push_back(data);
	}

	void update() {
		mutex.lock();
		if (wheel.size() > resolution) {
			while (!wheel.empty())
				for_each(wheel.front().begin(), wheel.front().end(), [&](int e) { regwrite(e); }), wheel.pop_front();
			count = sampleRate - 1;
		}
		wheel.insert(wheel.end(), tmpwheel.begin(), tmpwheel.end());
		mutex.unlock();
		for_each(tmpwheel.begin(), tmpwheel.end(), [](list<int>& e) { e.clear(); });
	}

	void makeSound(float *data, uint32_t length) {
		for (int i = 0; i < length; i++) {
			for (count += 60 * resolution; count >= sampleRate; count -= sampleRate)
				if (!wheel.empty()) {
					mutex.lock();
					for_each(wheel.front().begin(), wheel.front().end(), [&](int e) { regwrite(e); }), wheel.pop_front();
					mutex.unlock();
				}
			for (int j = 0; j < 3; j++) {
				auto& ch = channel[j];
				ch.freq = reg[j * 2];
				const int vol = ~reg[j * 2 + 1] & 0xf;
				data[i] += ((ch.output & 1) * 2 - 1) * (vol ? pow(10, (vol - 15) / 10.0) : 0.0) * gain * enable;
			}
			const int nfreq = (reg[6] & 3) == 3 ? channel[2].freq << 1 : 32 << (reg[6] & 3), nvol = ~reg[7] & 0xf;
			data[i] += ((rng & 1) * 2 - 1) * (nvol ? pow(10, (nvol - 15) / 10.0) : 0.0) * gain * enable;
			for (cycles += rate; cycles >= sampleRate; cycles -= sampleRate) {
				for (auto& ch: channel)
					!(--ch.count & 0x3ff) && (ch.output = ~ch.output, ch.count = ch.freq);
				!(--ncount & 0x7ff) && (rng = rng >> 1 | (rng << 14 ^ rng << 13 & reg[6] << 12) & 0x4000, ncount = nfreq);
			}
		}
	}

	void regwrite(int data) {
		if (data & 0x80)
			addr = data >> 4 & 7, reg[addr] = reg[addr] & 0x3f0 | data & 0xf;
		else
			reg[addr] = reg[addr] & 0xf | data << 4 & 0x3f0;
		addr == 6 && (rng = 0x4000);
	}
};

#endif //SN76489_H
