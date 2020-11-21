/*
 * C30
 */

#ifndef C30_H
#define C30_H

#include <algorithm>
#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct C30 {
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	float snd[0x200] = {};
	uint8_t ram[0x400] = {};
	uint8_t reg[0x140] = {};
	struct {
		uint32_t phase = 0;
		int ncount = 0;
		int rng = 1;
		int output = 0;
	} channel[8];

	C30(int clock = 24000, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		rate = clock * 4096 / sampleRate;
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
	}

	int read(int addr) {
		return ram[addr & 0x3ff];
	}

	void write(int addr, int data, int timer = 0) {
		ram[addr &= 0x3ff] = data;
		if (addr < 0x140)
			tmpwheel[timer].push_back({addr, data});
	}

	void update() {
		mutex.lock();
		if (wheel.size() > resolution) {
			while (!wheel.empty())
				for_each(wheel.front().begin(), wheel.front().end(), [&](pair<int, int>& e) { regwrite(e); }), wheel.pop_front();
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
					for_each(wheel.front().begin(), wheel.front().end(), [&](pair<int, int>& e) { regwrite(e); }), wheel.pop_front();
					mutex.unlock();
				}
			for (int j = 0; j < 8; j++) {
				auto& ch = channel[j];
				if (j >= 4 || ~reg[0x100 | -4 + j * 8 & 0x3f] & 0x80) {
					data[i] += snd[reg[0x101 + j * 8] << 1 & 0x1e0 | ch.phase >> 27] * (reg[0x100 + j * 8] & 0xf) / 15 * gain;
					ch.phase += (reg[0x103 + j * 8] | reg[0x102 + j * 8] << 8 | reg[0x101 + j * 8] << 16 & 0xf0000) * rate;
				} else {
					data[i] += (ch.output * 2 - 1) * (reg[0x100 + j * 8] & 0xf) / 15.0 * gain;
					for (ch.ncount += reg[0x103 + j * 8] * rate; ch.ncount >= 0x80000; ch.ncount -= 0x80000)
						ch.output ^= ch.rng + 1 >> 1 & 1, ch.rng = (ch.rng ^ (~ch.rng & 1) - 1 & 0x28000) >> 1;
				}
			}
		}
	}

	void regwrite(pair<int, int>& e) {
		int addr = e.first, data = e.second;
		reg[addr] = data, addr < 0x100 && (snd[addr * 2] = (data >> 4) * 2 / 15.0 - 1, snd[1 + addr * 2] = (data & 0xf) * 2 / 15.0 - 1);
	}
};

#endif //C30_H
