/*
 * AY-3-8910
 */

#ifndef AY_3_8910_H
#define AY_3_8910_H

#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct AY_3_8910 {
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	uint8_t ram[0x10] = {};
	uint8_t reg[0x10] = {};
	int cycles = 0;
	struct {
		int freq = 0;
		int count = 0;
		int output = 0;
	} channel[3];
	int ncount = 0;
	int rng = 0xffff;
	int ecount = 0;
	int step = 0;

	AY_3_8910(int clock, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		rate = clock / 8;
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
	}

	int read(int addr) {
		return ram[addr & 0x0f];
	}

	void write(int addr, int data, int timer = 0) {
		ram[addr &= 0x0f] = data;
		if (addr >= 0x0e)
			return;
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
			const int nfreq = reg[6] & 0x1f, efreq = reg[11] | reg[12] << 8, etype = reg[13];
			const int evol = (~step ^ ((((etype ^ etype >> 1) & step >> 4 ^ ~etype >> 2) & 1) - 1)) & (~etype >> 3 & step >> 4 & 1) - 1 & 15;
			for (int j = 0; j < 3; j++) {
				auto& ch = channel[j];
				ch.freq = reg[j * 2] | reg[1 + j * 2] << 8 & 0xf00;
				const int vol = (reg[8 + j] >> 4 & 1) != 0 ? evol : reg[8 + j] & 0x0f;
				data[i] += ((((ch.freq == 0) | reg[7] >> j | ch.output) & (reg[7] >> j + 3 | rng) & 1) * 2 - 1) * (vol ? pow(10, (vol - 15) / 10.0) : 0.0) * gain;
			}
			for (cycles += rate; cycles >= sampleRate; cycles -= sampleRate) {
				for (auto& ch: channel) {
					if (++ch.count >= ch.freq) {
						ch.count = 0;
						ch.output = ~ch.output;
					}
				}
				if (++ncount >= nfreq << 1) {
					ncount = 0;
					rng = (rng >> 16 ^ rng >> 13 ^ 1) & 1 | rng << 1;
				}
				if (++ecount >= efreq) {
					ecount = 0;
					step += ((step < 16) | etype >> 3 & ~etype & 1) - (step >= 47) * 32;
				}
			}
		}
	}

	void regwrite(pair<int, int>& e) {
		int addr = e.first, data = e.second;
		reg[addr] = data;
		if (addr == 13)
			step = 0;
	}
};

#endif //AY_3_8910_H
