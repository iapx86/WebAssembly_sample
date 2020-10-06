/*
 * Mappy Sound
 */

#ifndef MAPPY_SOUND_H
#define MAPPY_SOUND_H

#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct MappySound {
	float snd[0x100] = {};
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	uint8_t ram[0x400] = {};
	uint8_t reg[0x40] = {};
	uint32_t phase[8] = {};

	MappySound(uint8_t *SND, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		for (int i = 0; i < 0x100; i++)
			snd[i] = (SND[i] & 0xf) * 2 / 15.0 - 1;
		rate = 2048 * 48000 / sampleRate;
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
		if (addr >= 0x40)
			return;
		tmpwheel[timer].push_back({addr, data});
	}

	void update() {
		mutex.lock();
		if (wheel.size() > resolution) {
			while (!wheel.empty()) {
				for (auto& e: wheel.front())
					reg[e.first] = e.second;
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
						reg[e.first] = e.second;
					wheel.pop_front();
					mutex.unlock();
				}
			for (int j = 0; j < 8; j++) {
				data[i] += snd[reg[6 + j * 8] << 1 & 0xe0 | phase[j] >> 27] * (reg[3 + j * 8] & 0x0f) / 15 * gain;
				phase[j] += (reg[4 + j * 8] | reg[5 + j * 8] << 8 | reg[6 + j * 8] << 16 & 0xf0000) * rate;
			}
		}
	}
};

#endif //MAPPY_SOUND_H
