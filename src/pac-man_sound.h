/*
 * Pac-Man Sound
 */

#ifndef PAC_MAN_SOUND_H
#define PAC_MAN_SOUND_H

#include <list>
#include <mutex>
#include <utility>
#include <vector>
using namespace std;

struct PacManSound {
	float snd[0x100] = {};
	int rate;
	int sampleRate;
	int count;
	int resolution;
	float gain;
	vector<list<pair<int, int>>> tmpwheel;
	list<list<pair<int, int>>> wheel;
	mutex mutex;
	bool enable = true;
	uint8_t reg[0x20] = {};
	uint32_t phase[3] = {};

	PacManSound(uint8_t *SND, int sampleRate = 48000, int resolution = 1, float gain = 0.1) {
		for (int i = 0; i < 0x100; i++)
			snd[i] = (SND[i] & 0xf) * 2 / 15.0 - 1;
		rate = 8192 * 48000 / sampleRate;
		this->sampleRate = sampleRate;
		count = sampleRate - 1;
		this->resolution = resolution;
		this->gain = gain;
		tmpwheel.resize(resolution);
	}

	void mute(bool flag) {
		enable = !flag;
	}

	void write(int addr, int data, int timer = 0) {
		tmpwheel[timer].push_back({addr & 0x1f, data & 0xf});
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
			for (int j = 0; j < 3; j++) {
				data[i] += snd[reg[0x05 + j * 5] << 5 & 0xe0 | phase[j] >> 27] * reg[0x15 + j * 5] / 15 * gain * enable;
				phase[j] += ((j ? 0 : reg[0x10]) | reg[0x11 + j * 5] << 4 | reg[0x12 + j * 5] << 8 | reg[0x13 + j * 5] << 12 | reg[0x14 + j * 5] << 16) * rate;
			}
		}
	}
};

#endif //PAC_MAN_SOUND_H
