/*
 * Mappy Sound
 */

#ifndef MAPPY_SOUND_H
#define MAPPY_SOUND_H

#include <algorithm>
#include <array>
#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct MappySound {
	array<float, 0x100> snd;
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	array<uint8_t, 0x400> ram = {};
	array<uint8_t, 0x40> reg = {};
	array<uint32_t, 8> phase = {};

	MappySound(array<uint8_t, 0x100>& SND, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		for (int i = 0; i < snd.size(); i++)
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
		if (addr < 0x40)
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
			for (int j = 0; j < 8; j++) {
				data[i] += snd[reg[6 + j * 8] << 1 & 0xe0 | phase[j] >> 27] * (reg[3 + j * 8] & 0x0f) / 15 * gain;
				phase[j] += (reg[4 + j * 8] | reg[5 + j * 8] << 8 | reg[6 + j * 8] << 16 & 0xf0000) * rate;
			}
		}
	}
};

#endif //MAPPY_SOUND_H
