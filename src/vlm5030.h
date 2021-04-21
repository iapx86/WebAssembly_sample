/*
 * VLM5030
 */

#ifndef VLM5030_H
#define VLM5030_H

#include <array>
#include <mutex>
using namespace std;

struct VLM5030 {
	static const array<uint8_t, 32> table_p, table_e;
	static const array<int16_t, 64> table_k0;
	static const array<int16_t, 32> table_k1;
	static const array<int16_t, 16> table_k2_3;
	static const array<int16_t, 8> table_k4_9;

	const uint8_t *base;
	int size;
	int clock;
	int sampleRate;
	double gain;
	int BSY = 0;
	int frac = 0;
	int param = 0;
	int offset = 0;
	int icount = 0;
	int scount = 0;
	int pcount = 0;
	int pitch0 = 0;
	int energy0 = 0;
	array<int16_t, 10> k0 = {};
	int npitch = 0;
	int nenergy = 0;
	array<int16_t, 10> nk = {};
	int pitch1 = 0;
	int energy1 = 0;
	array<int16_t, 10> k1 = {};
	int pitch = 0;
	int energy = 0;
	array<int16_t, 10> k = {};
	array<int, 10> x = {};
	double output = 0;

	template<class T> VLM5030(const T& VLM, int clock, int sampleRate = 48000, double gain = 0.1) {
		base = VLM.data();
		size = VLM.size();
		this->clock = clock;
		this->sampleRate = sampleRate;
		this->gain = gain;
		rst(0);
	}

	void rst(int data) {
		if (BSY) {
			BSY = 0;
			offset = 0;
			icount = scount = pcount = 0;
			pitch0 = energy0 = 0, k0.fill(0);
			npitch = nenergy = 0, nk.fill(0);
			pitch1 = energy1 = 0, k1.fill(0);
			pitch = energy = 0;
			k.fill(0);
			x.fill(0);
			output = 0;
		}
		param = data;
	}

	void st(int data) {
		offset = data & 0xfe | data << 8 & 0x100;
		offset = (base[offset] << 8 | base[offset + 1]) & size - 1;
		scount = array<int, 8>{40, 30, 20, 20, 40, 60, 50, 50}[param >> 3 & 7];
		icount = 4;
		BSY = 1;
	}

	void update() {
		for (frac += clock; frac >= sampleRate * 440; frac -= sampleRate * 440) {
			if (!BSY)
				continue;
			if (!scount) {
				scount = array<int, 8>{40, 30, 20, 20, 40, 60, 50, 50}[param >> 3 & 7];
				if (!icount) {
					pitch0 = npitch, energy0 = nenergy, k0 = nk;
					npitch = nenergy = 0, nk.fill(0);
					const uint8_t *frame = base + offset;
					if (~frame[0] & 1) {
						npitch = table_p[frame[0] >> 1 & 0x1f] + array<int, 4>{0, 8, -8, -8}[param >> 6 & 3] & 0xff;
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
					} else if (~frame[0] & 2) {
						offset++;
						icount = (frame[0] & 0xc) + 4 << 1;
					} else if (energy0)
						icount = 4;
					else {
						BSY = 0;
						continue;
					}
					if (energy0)
						pitch1 = npitch, energy1 = nenergy, k1 = nk;
					else
						pitch1 = pitch0, energy1 = energy0, k1 = k0;
				}
				const int ieffect = (~(icount -= array<int, 4>{1, 2, 4, 4}[param & 3]) & 3) + 1;
				pitch = pitch0 > 1 ? pitch0 + ((pitch1 - pitch0) * ieffect >> 2) : 0;
				energy = energy0 + ((energy1 - energy0) * ieffect >> 2);
				for (int j = 0; j < 10; j++)
					k[j] = k0[j] + ((k1[j] - k0[j]) * ieffect >> 2);
			}
			int u[11];
			u[10] = pitch0 > 1 ? energy * !pcount : rand() & 1 ? energy : -energy;
			for (int j = 9; j >= 0; --j)
				u[j] = u[j + 1] + (k[j] * x[j] >> 9);
			for (int j = 9; j >= 1; --j)
				x[j] = x[j - 1] - (k[j - 1] * u[j - 1] >> 9);
			x[0] = u[0];
			output = std::min(1.0, std::max(-1.0, u[0] / 511.0)) * gain;
			--scount;
			if (++pcount >= pitch)
				pcount = 0;
		}
	}
};

#endif //VLM5030_H
