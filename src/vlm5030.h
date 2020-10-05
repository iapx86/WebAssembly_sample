/*
 * VLM5030
 */

#ifndef VLM5030_H
#define VLM5030_H

#include <algorithm>
#include <cstring>
#include <mutex>
using namespace std;

struct VLM5030 {
	static uint8_t table_p[], table_e[];
	static int16_t table_k0[], table_k1[], table_k2_3[], table_k4_9[];

	uint8_t *base;
	int length;
	int rate;
	int sampleRate;
	float gain;
	mutex mutex;
	int BSY = 0;
	int cycles = 0;
	int param = 0;
	int offset = 0;
	int icount = 0;
	int scount = 0;
	int pcount = 0;
	int pitch0 = 0;
	int energy0 = 0;
	int16_t k0[10] = {};
	int npitch = 0;
	int nenergy = 0;
	int16_t nk[10] = {};
	int pitch1 = 0;
	int energy1 = 0;
	int16_t k1[10] = {};
	int pitch = 0;
	int energy = 0;
	int16_t k[10] = {};
	int x[10] = {};
	float output = 0;

	VLM5030(uint8_t *VLM, int length, int clock, int sampleRate = 48000, float gain = 0.1) {
		base = VLM;
		this->length = length;
		rate = clock / 440;
		this->sampleRate = sampleRate;
		this->gain = gain;
		cycles = sampleRate - 1;
		rst(0);
	}

	void update() {
	}

	void rst(int data) {
		mutex.lock();
		if (BSY) {
			BSY = 0;
			offset = 0;
			icount = scount = pcount = 0;
			pitch0 = energy0 = 0;
			memset(k0, 0, sizeof(k0));
			npitch = nenergy = 0;
			memset(nk, 0, sizeof(nk));
			pitch1 = energy1 = 0;
			memset(k1, 0, sizeof(k1));
			pitch = energy = 0;
			memset(k, 0, sizeof(k));
			memset(x, 0, sizeof(x));
			output = 0;
		}
		param = data;
		mutex.unlock();
	}

	void st(int data) {
		mutex.lock();
		offset = data & 0xfe | data << 8 & 0x100;
		offset = (base[offset] << 8 | base[offset + 1]) & (length - 1);
		const int stable[] = {40, 30, 20, 20, 40, 60, 50, 50};
		scount = stable[param >> 3 & 7];
		icount = 4;
		BSY = 1;
		mutex.unlock();
	}

	void makeSound(float *data, uint32_t length) {
		mutex.lock();
		for (int i = 0; i < length; i++) {
			data[i] += output * gain;
			for (cycles += rate; cycles >= sampleRate; cycles -= sampleRate) {
				if (BSY == 0)
					continue;
				if (scount == 0) {
					const int stable[] = {40, 30, 20, 20, 40, 60, 50, 50};
					scount = stable[param >> 3 & 7];
					if (icount == 0) {
						pitch0 = npitch;
						energy0 = nenergy;
						memcpy(k0, nk, sizeof(k0));
						npitch = nenergy = 0;
						memset(nk, 0, sizeof(nk));
						const uint8_t *frame = base + offset;
						if ((frame[0] & 1) == 0) {
							const int ptable[] = {0, 8, -8, -8};
							npitch = table_p[frame[0] >> 1 & 0x1f] + ptable[param >> 6 & 3] & 0xff;
							nenergy = table_e[frame[0] >> 6 | frame[1] << 2 & 0x1c];
							nk[9] = table_k4_9[frame[1] >> 3 & 7];
							nk[8] = table_k4_9[frame[1] >> 6 | frame[2] << 2 & 4];
							nk[7] = table_k4_9[frame[2] >> 1 & 7];
							nk[6] = table_k4_9[frame[2] >> 4 & 7];
							nk[5] = table_k4_9[frame[2] >> 7 | frame[3] << 1 & 6];
							nk[4] = table_k4_9[frame[3] >> 2 & 7];
							nk[3] = table_k2_3[frame[3] >> 5 | frame[4] << 3 & 8];
							nk[2] = table_k2_3[frame[4] >> 1 & 0xf];
							nk[1] = table_k1[frame[4] >> 5 | frame[5] << 3 & 0x18];
							nk[0] = table_k0[frame[5] >> 2];
							offset += 6;
							icount = 4;
						}
						else if ((frame[0] & 2) == 0) {
							offset++;
							icount = (frame[0] & 0xc) + 4 << 1;
						}
						else if (energy0 != 0)
							icount = 4;
						else {
							BSY = 0;
							continue;
						}
						if (energy0 != 0)
							pitch1 = npitch, energy1 = nenergy, memcpy(k1, nk, sizeof(k1));
						else
							pitch1 = pitch0, energy1 = energy0, memcpy(k1, k0, sizeof(k1));
					}
					const int itable[] = {1, 2, 4, 4};
					const int ieffect = (~(icount -= itable[param & 3]) & 3) + 1;
					pitch = pitch0 > 1 ? pitch0 + ((pitch1 - pitch0) * ieffect >> 2) : 0;
					energy = energy0 + ((energy1 - energy0) * ieffect >> 2);
					for (int j = 0; j < 10; j++)
						k[j] = k0[j] + ((k1[j] - k0[j]) * ieffect >> 2);
				}
				int u[11];
				u[10] = pitch0 > 1 ? energy * (pcount == 0) : rand() & 1 ? energy : -energy;
				for (int j = 9; j >= 0; --j)
					u[j] = u[j + 1] + (k[j] * x[j] >> 9);
				for (int j = 9; j >= 1; --j)
					x[j] = x[j - 1] - (k[j - 1] * u[j - 1] >> 9);
				x[0] = u[0];
				output = std::min(1.0, std::max(-1.0, u[0] / 511.0));
				--scount;
				if (++pcount >= pitch)
					pcount = 0;
			}
		}
		mutex.unlock();
	}
};

#endif //VLM5030_H
