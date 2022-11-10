/*
 *
 *	TwinBee
 *
 */

#ifndef TWINBEE_H
#define TWINBEE_H

#include <cmath>
#include <algorithm>
#include <array>
#include <functional>
#include "mc68000.hpp"
#include "z80.hpp"
#include "ay-3-8910.hpp"
#include "k005289.hpp"
#include "vlm5030.hpp"
#include "utils.hpp"
using namespace std;

enum {
	BONUS_20000_100000, BONUS_30000_120000, BONUS_40000_140000, BONUS_50000_160000,
};

enum {
	RANK_EASY, RANK_NORMAL, RANK_HARD, RANK_HARDEST,
};

struct TwinBee {
	static array<uint8_t, 0x50000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x200> SND;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static AY_3_8910 *sound0, *sound1;
	static K005289 *sound2;
	static VLM5030 *sound3;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	bool fTurbo = false;
	int nLife = 3;
	int nBonus = BONUS_30000_120000;
	int nRank = RANK_NORMAL;
	bool fDemoSound = true;

	bool fInterrupt2Enable = false;
	bool fInterrupt4Enable = false;

	array<uint8_t, 0x49000> ram = {};
	array<uint8_t, 0x4000> ram2 = {};
	array<uint8_t, 0x800> vlm = {};
	array<uint8_t, 6> in = {0xff, 0x56, 0xff, 0xff, 0xff, 0xff};
	struct {
		int addr = 0;
	} psg[2];
	struct {
		int freq0 = 0;
		int freq1 = 0;
	} scc;
	int vlm_latch = 0;
	int command = 0;
	bool cpu2_irq = false;

	array<uint8_t, 0x20000> chr = {};
	array<int, 0x800> rgb;
	array<int, width * height> bitmap;
	bool updated = false;
	int flip = 0;
	array<uint8_t, 32> intensity = {};

	MC68000 cpu;
	Z80 cpu2;
	struct {
		int rate = 256 * 60;
		int frac = 0;
		int count = 0;
		void execute(int rate, function<void(int)> fn) {
			for (frac += this->rate; frac >= rate; frac -= rate)
				fn(count = count + 1 & 255);
		}
	} scanline;
	struct {
		double rate = 14318180.0 / 4096;
		double frac = 0;
		int count = 0;
		void execute(double rate, function<void(int)> fn) {
			for (frac += this->rate; frac >= rate; frac -= rate)
				fn(count = count + 1 & 255);
		}
	} timer;

	TwinBee() : cpu(18432000 / 2), cpu2(14318180 / 8) {
		// CPU周りの初期化
		for (int i = 0; i < 0x100; i++)
			cpu.memorymap[i].base = &PRG1[i << 8];
		for (int i = 0; i < 0x100; i++) {
			cpu.memorymap[0x100 + i].base = &ram[i << 8];
			cpu.memorymap[0x100 + i].write = nullptr;
		}
		for (int i = 0; i < 0x80; i++) {
			cpu.memorymap[0x200 + i].read = [&](int addr) { return addr & 1 ? ram2[addr >> 1 & 0x3fff] : 0; };
			cpu.memorymap[0x200 + i].write = [&](int addr, int data) { addr & 1 && (ram2[addr >> 1 & 0x3fff] = data); };
		}
		for (int i = 0; i < 0x100; i++) {
			cpu.memorymap[0x300 + i].base = &ram[0x100 + i << 8];
			cpu.memorymap[0x300 + i].write = [&](int addr, int data) {
				int offset = addr & 0xffff;
				ram[0x10000 | offset] = data, chr[offset <<= 1] = data >> 4, chr[1 | offset] = data & 0xf;
			};
		}
		for (int i = 0; i < 0x80; i++) {
			cpu.memorymap[0x500 + i].base = &ram[0x200 + i << 8];
			cpu.memorymap[0x500 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu.memorymap[0x5a0 + i].base = &ram[0x280 + i << 8];
			cpu.memorymap[0x5a0 + i].write = nullptr;
			cpu.memorymap[0x5a0 + i].write16 = [&](int addr, int data) {
				const int offset = addr & 0xffe;
				ram[0x28000 | offset] = data >> 8, ram[0x28001 | offset] = data;
				rgb[offset >> 1] = 0xff000000 | intensity[data >> 10 & 31] << 16 | intensity[data >> 5 & 31] << 8 | intensity[data & 31];
			};
		}
		cpu.memorymap[0x5c0].write = [&](int addr, int data) { addr == 0x5c001 && (command = data); };
		cpu.memorymap[0x5c4].read = [&](int addr) { return addr >= 0x5c402 && addr < 0x5c408 ? in[addr - 0x5c402 >> 1] : 0xff; };
		cpu.memorymap[0x5cc].read = [&](int addr) { return addr < 0x5cc06 ? in[addr - 0x5cc00 + 6 >> 1] : 0xff; };
		cpu.memorymap[0x5d0].read = [&](int addr) { return addr == 0x5d001 ? 0 : 0xff; };
		cpu.memorymap[0x5e0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 1:
				return void(fInterrupt2Enable = (data & 1) != 0);
			case 4:
				return void(data & 1 && (cpu2_irq = true));
			case 5:
				return void(flip = flip & 2 | data & 1);
			case 7:
				return void(flip = flip & 1 | data << 1 & 2);
			case 0xe:
				return void(fInterrupt4Enable = (data & 1) != 0);
			}
		};
		for (int i = 0; i < 0x200; i++) {
			cpu.memorymap[0x600 + i].base = &ram[0x290 + i << 8];
			cpu.memorymap[0x600 + i].write = nullptr;
		}
		for (int i = 0; i < 0x400; i++)
			cpu.memorymap[0x800 + i].base = &PRG1[0x100 + i << 8];

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = &PRG2[i << 8];
		for (int i = 0; i < 0x40; i++) {
			cpu2.memorymap[0x40 + i].base = &ram2[i << 8];
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu2.memorymap[0x80 + i].base = &vlm[i << 8];
			cpu2.memorymap[0x80 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu2.memorymap[0xa0 + i].write = [&](int addr, int data) { scc.freq0 = ~addr & 0xfff; };
			cpu2.memorymap[0xc0 + i].write = [&](int addr, int data) { scc.freq1 = ~addr & 0xfff; };
		}
		cpu2.memorymap[0xe0].read = [&](int addr) {
			switch (addr & 0xff) {
			case 1:
				return command;
			case 0x86:
				return sound0->read(psg[0].addr);
			}
			return 0xff;
		};
		cpu2.memorymap[0xe0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(vlm_latch = data);
			case 3:
				return sound2->write(2, scc.freq0);
			case 4:
				return sound2->write(3, scc.freq1);
			case 5:
				return void(psg[1].addr = data);
			case 6:
				return void(psg[0].addr = data);
			case 0x30:
				return sound3->st(vlm_latch);
			}
		};
		cpu2.memorymap[0xe1].write = [&](int addr, int data) {
			if (addr == 0xe106 && psg[0].addr != 0xe)
				sound0->write(psg[0].addr, data);
		};
		cpu2.memorymap[0xe2].read = [&](int addr) { return addr == 0xe205 ? sound1->read(psg[1].addr) : 0xff; };
		cpu2.memorymap[0xe4].write = [&](int addr, int data) {
			if (addr == 0xe405) {
				if ((psg[1].addr & 0xe) == 0xe)
					sound2->write(psg[1].addr & 1, data);
				sound1->write(psg[1].addr, data);
			}
		};

		cpu2.check_interrupt = [&]() { return cpu2_irq && cpu2.interrupt() && (cpu2_irq = false, true); };

		// Videoの初期化
		rgb.fill(0xff000000), bitmap.fill(0xff000000);

		// 輝度の計算
		array<double, 32> _intensity;
		const array<double, 5> r = {4700, 2400, 1200, 620, 300};
		for (int i = 0; i < _intensity.size(); i++) {
			double rt = 0, v = 0;
			for (int j = 0; j < r.size(); j++)
				if (~i >> j & 1)
					rt += 1 / r[j], v += 0.05 / r[j];
			_intensity[i] = ((v + 0.005) / (rt + 0.001) - 0.7) * 255 / 5.0 + 0.4;
		}
		const double black = _intensity[0], white = 255 / (_intensity[31] - black);
		for (int i = 0; i < intensity.size(); i++)
			intensity[i] = (_intensity[i] - black) * white + 0.5;
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			scanline.execute(tick_rate, [&](int vpos) {
				vpos == 120 && fInterrupt4Enable && cpu.interrupt(4);
				!vpos && (update(), fInterrupt2Enable && cpu.interrupt(2), cpu2.non_maskable_interrupt());
			});
			timer.execute(tick_rate, [&](int cnt) { sound0->write(0xe, cnt & 0x2f | sound3->BSY << 5 | 0xd0); });
			sound0->execute(tick_rate);
			sound1->execute(tick_rate);
			sound2->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	TwinBee *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 2:
				in[1] |= 3;
				break;
			case 3:
				in[1] = in[1] & ~3 | 2;
				break;
			case 4:
				in[1] = in[1] & ~3 | 1;
				break;
			case 7:
				in[1] &= ~3;
				break;
			}
			switch (nBonus) {
			case BONUS_20000_100000:
				in[1] |= 0x18;
				break;
			case BONUS_30000_120000:
				in[1] = in[1] & ~0x18 | 0x10;
				break;
			case BONUS_40000_140000:
				in[1] = in[1] & ~0x18 | 8;
				break;
			case BONUS_50000_160000:
				in[1] &= ~0x18;
				break;
			}
			switch (nRank) {
			case RANK_EASY:
				in[1] |= 0x60;
				break;
			case RANK_NORMAL:
				in[1] = in[1] & ~0x60 | 0x40;
				break;
			case RANK_HARD:
				in[1] = in[1] & ~0x60 | 0x20;
				break;
			case RANK_HARDEST:
				in[1] &= ~0x60;
				break;
			}
			if (fDemoSound)
				in[1] &= ~0x80;
			else
				in[1] |= 0x80;
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			fInterrupt2Enable = false;
			fInterrupt4Enable = false;
			cpu.reset();
			cpu2_irq = false;
			cpu2.reset();
		}
		return this;
	}

	TwinBee *updateInput() {
		in[3] = in[3] & ~0x1c | !fCoin << 2 | !fStart1P << 3 | !fStart2P << 4;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		fTurbo && (in[4] ^= 1 << 4);
		return this;
	}

	void coin(bool fDown) {
		fDown && (fCoin = 2);
	}

	void start1P(bool fDown) {
		fDown && (fStart1P = 2);
	}

	void start2P(bool fDown) {
		fDown && (fStart2P = 2);
	}

	void up(bool fDown) {
		in[4] = in[4] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void right(bool fDown) {
		in[4] = in[4] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void down(bool fDown) {
		in[4] = in[4] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void left(bool fDown) {
		in[4] = in[4] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void triggerA(bool fDown) {
		in[4] = in[4] & ~(1 << 4) | !fDown << 4;
	}

	void triggerB(bool fDown) {
		in[4] = in[4] & ~(1 << 5) | !fDown << 5;
	}

	void triggerY(bool fDown) {
		!(fTurbo = fDown) && (in[4] |= 1 << 4);
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256, i++)
			fill_n(&bitmap[p], 224, 0);

		// bg描画
		for (int k = 0x23000; k < 0x24000; k += 2)
			if (!(ram[k] & 0x50) && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if (!(ram[k] & 0x50) && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);
		for (int k = 0x23000; k < 0x24000; k += 2)
			if ((ram[k] & 0x50) == 0x40 && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if ((ram[k] & 0x50) == 0x40 && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);

		// obj描画
		const int size[8][2] = {{32, 32}, {16, 32}, {32, 16}, {64, 64}, {8, 8}, {16, 8}, {8, 16}, {16, 16}};
		for (int pri = 0; pri < 256; pri++)
			for (int k = 0x26000; k < 0x27000; k += 0x10) {
				if (ram[k + 1] != pri)
					continue;
				int zoom = ram[k + 5];
				int src = ram[k + 9] << 9 & 0x18000 | ram[k + 7] << 7;
				if (!ram[k + 4] && ram[k + 6] != 0xff)
					src = src + (ram[k + 6] << 15) & 0x1ff80;
				if (zoom == 0xff && !src || !(zoom |= ram[k + 3] << 2 & 0x300))
					continue;
				const int color = ram[k + 9] << 3 & 0xf0;
				const int y = (ram[k + 9] << 8 | ram[k + 11]) + 16 & 0x1ff;
				const int x = ~ram[k + 13] & 0xff;
				const int h = size[ram[k + 3] >> 3 & 7][0];
				const int w = size[ram[k + 3] >> 3 & 7][1];
				switch (ram[k + 9] >> 4 & 2 | ram[k + 3] & 1) {
				case 0:
					xferHxW(bitmap.data(), src, color, y, x, h, w, zoom);
					break;
				case 1:
					xferHxW_V(bitmap.data(), src, color, y, x, h, w, zoom);
					break;
				case 2:
					xferHxW_H(bitmap.data(), src, color, y, x, h, w, zoom);
					break;
				case 3:
					xferHxW_HV(bitmap.data(), src, color, y, x, h, w, zoom);
					break;
				}
			}

		// bg描画
		for (int k = 0x23000; k < 0x24000; k += 2)
			if (ram[k] & 0x10 && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if (ram[k] & 0x10 && ram[k] & 0xf8)
				xfer8x8(bitmap.data(), k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void xfer8x8(int *data, int k) {
		const int x0 = ((flip & 2 ? k : ~k) >> 4 & 0xf8 | 7) + ram[0x20f01 | ~k >> 5 & 0x80 | k & 0x7e] & 0xff;
		const int color = ram[k + 0x2001] << 4 & 0x7f0;
		int src = (ram[k] << 8 & 0x700 | ram[k + 1]) << 6, px;

		if (x0 < 16 || x0 >= 247)
			return;
		if (~ram[k] & 0x20 || (ram[k] & 0xc0) == 0x40)
			switch ((ram[k] >> 2 & 2 | ram[k + 0x2001] >> 7) ^ flip) {
			case 0: // ノーマル
				for (int x = x0, i = 0; i < 8; src += 8, --x, i++) {
					const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
					const int y = (k << 2) - scroll + 16 & 0x1ff;
					if (y < 9 || y > 271)
						continue;
					data[y << 8 | x] = color | chr[src | 0];
					data[y + 1 << 8 | x] = color | chr[src | 1];
					data[y + 2 << 8 | x] = color | chr[src | 2];
					data[y + 3 << 8 | x] = color | chr[src | 3];
					data[y + 4 << 8 | x] = color | chr[src | 4];
					data[y + 5 << 8 | x] = color | chr[src | 5];
					data[y + 6 << 8 | x] = color | chr[src | 6];
					data[y + 7 << 8 | x] = color | chr[src | 7];
				}
				return;
			case 1: // V反転
				for (int x = x0, i = 0; i < 8; src += 8, --x, i++){
					const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
					const int y = (k << 2) - scroll + 16 & 0x1ff;
					if (y < 9 || y > 271)
						continue;
					data[y + 7 << 8 | x] = color | chr[src | 0];
					data[y + 6 << 8 | x] = color | chr[src | 1];
					data[y + 5 << 8 | x] = color | chr[src | 2];
					data[y + 4 << 8 | x] = color | chr[src | 3];
					data[y + 3 << 8 | x] = color | chr[src | 4];
					data[y + 2 << 8 | x] = color | chr[src | 5];
					data[y + 1 << 8 | x] = color | chr[src | 6];
					data[y << 8 | x] = color | chr[src | 7];
				}
				return;
			case 2: // H反転
				for (int x = x0 - 7, i = 0; i < 8; src += 8, x++, i++) {
					const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
					const int y = (k << 2) - scroll + 16 & 0x1ff;
					if (y < 9 || y > 271)
						continue;
					data[y << 8 | x] = color | chr[src | 0];
					data[y + 1 << 8 | x] = color | chr[src | 1];
					data[y + 2 << 8 | x] = color | chr[src | 2];
					data[y + 3 << 8 | x] = color | chr[src | 3];
					data[y + 4 << 8 | x] = color | chr[src | 4];
					data[y + 5 << 8 | x] = color | chr[src | 5];
					data[y + 6 << 8 | x] = color | chr[src | 6];
					data[y + 7 << 8 | x] = color | chr[src | 7];
				}
				return;
			case 3: // HV反転
				for (int x = x0 - 7, i = 0; i < 8; src += 8, x++, i++) {
					const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
					const int y = (k << 2) - scroll + 16 & 0x1ff;
					if (y < 9 || y > 271)
						continue;
					data[y + 7 << 8 | x] = color | chr[src | 0];
					data[y + 6 << 8 | x] = color | chr[src | 1];
					data[y + 5 << 8 | x] = color | chr[src | 2];
					data[y + 4 << 8 | x] = color | chr[src | 3];
					data[y + 3 << 8 | x] = color | chr[src | 4];
					data[y + 2 << 8 | x] = color | chr[src | 5];
					data[y + 1 << 8 | x] = color | chr[src | 6];
					data[y << 8 | x] = color | chr[src | 7];
				}
				return;
			}
		switch ((ram[k] >> 2 & 2 | ram[k + 0x2001] >> 7) ^ flip) {
		case 0: // ノーマル
			for (int x = x0, i = 0; i < 8; src += 8, --x, i++) {
				const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
				const int y = (k << 2) - scroll + 16 & 0x1ff;
				if (y < 9 || y > 271)
					continue;
				(px = chr[src | 0]) && (data[y << 8 | x] = color | px);
				(px = chr[src | 1]) && (data[y + 1 << 8 | x] = color | px);
				(px = chr[src | 2]) && (data[y + 2 << 8 | x] = color | px);
				(px = chr[src | 3]) && (data[y + 3 << 8 | x] = color | px);
				(px = chr[src | 4]) && (data[y + 4 << 8 | x] = color | px);
				(px = chr[src | 5]) && (data[y + 5 << 8 | x] = color | px);
				(px = chr[src | 6]) && (data[y + 6 << 8 | x] = color | px);
				(px = chr[src | 7]) && (data[y + 7 << 8 | x] = color | px);
			}
			break;
		case 1: // V反転
			for (int x = x0, i = 0; i < 8; src += 8, --x, i++){
				const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
				const int y = (k << 2) - scroll + 16 & 0x1ff;
				if (y < 9 || y > 271)
					continue;
				(px = chr[src | 0]) && (data[y + 7 << 8 | x] = color | px);
				(px = chr[src | 1]) && (data[y + 6 << 8 | x] = color | px);
				(px = chr[src | 2]) && (data[y + 5 << 8 | x] = color | px);
				(px = chr[src | 3]) && (data[y + 4 << 8 | x] = color | px);
				(px = chr[src | 4]) && (data[y + 3 << 8 | x] = color | px);
				(px = chr[src | 5]) && (data[y + 2 << 8 | x] = color | px);
				(px = chr[src | 6]) && (data[y + 1 << 8 | x] = color | px);
				(px = chr[src | 7]) && (data[y << 8 | x] = color | px);
			}
			break;
		case 2: // H反転
			for (int x = x0 - 7, i = 0; i < 8; src += 8, x++, i++) {
				const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
				const int y = (k << 2) - scroll + 16 & 0x1ff;
				if (y < 9 || y > 271)
					continue;
				(px = chr[src | 0]) && (data[y << 8 | x] = color | px);
				(px = chr[src | 1]) && (data[y + 1 << 8 | x] = color | px);
				(px = chr[src | 2]) && (data[y + 2 << 8 | x] = color | px);
				(px = chr[src | 3]) && (data[y + 3 << 8 | x] = color | px);
				(px = chr[src | 4]) && (data[y + 4 << 8 | x] = color | px);
				(px = chr[src | 5]) && (data[y + 5 << 8 | x] = color | px);
				(px = chr[src | 6]) && (data[y + 6 << 8 | x] = color | px);
				(px = chr[src | 7]) && (data[y + 7 << 8 | x] = color | px);
			}
			break;
		case 3: // HV反転
			for (int x = x0 - 7, i = 0; i < 8; src += 8, x++, i++) {
				const int offset = k >> 2 & 0x400 | ~x << 1 & 0x1fe, scroll = ram[0x20001 | offset] | ram[0x20201 | offset] << 8;
				const int y = (k << 2) - scroll + 16 & 0x1ff;
				if (y < 9 || y > 271)
					continue;
				(px = chr[src | 0]) && (data[y + 7 << 8 | x] = color | px);
				(px = chr[src | 1]) && (data[y + 6 << 8 | x] = color | px);
				(px = chr[src | 2]) && (data[y + 5 << 8 | x] = color | px);
				(px = chr[src | 3]) && (data[y + 4 << 8 | x] = color | px);
				(px = chr[src | 4]) && (data[y + 3 << 8 | x] = color | px);
				(px = chr[src | 5]) && (data[y + 2 << 8 | x] = color | px);
				(px = chr[src | 6]) && (data[y + 1 << 8 | x] = color | px);
				(px = chr[src | 7]) && (data[y << 8 | x] = color | px);
			}
			break;
		}
	}

	void xferHxW(int *data, int src, int color, int y0, int x0, int h, int w, int zoom) {
		const int dh = h * 0x80 / zoom, dw = w * 0x80 / zoom, y1 = y0 + dh - 1 & 0x1ff, x1 = x0 - dw + 1 & 0xff;
		int px;

		if (dh <= 256 && (y0 < 16 || y0 >= 272) && (y1 < 16 || y1 >= 272) || dw <= 32 && (x0 < 16 || x0 >= 240) && (x1 < 16 || x1 >= 240))
			return;
		for (int x = x0, i = 0; i >> 7 < w; x = x - 1 & 0xff, i += zoom)
			for (int y = y0, j = 0; j >> 7 < h; y = y + 1 & 0x1ff, j += zoom)
				if ((px = chr[src | (i >> 7) * h | j >> 7]))
					data[y << 8 | x] = color | px;
	}

	void xferHxW_V(int *data, int src, int color, int y0, int x0, int h, int w, int zoom) {
		const int dh = h * 0x80 / zoom, dw = w * 0x80 / zoom, y1 = y0 + dh - 1 & 0x1ff, x1 = x0 - dw + 1 & 0xff;
		int px;

		if (dh <= 256 && (y0 < 16 || y0 >= 272) && (y1 < 16 || y1 >= 272) || dw <= 32 && (x0 < 16 || x0 >= 240) && (x1 < 16 || x1 >= 240))
			return;
		for (int x = x0, i = 0; i >> 7 < w; x = x - 1 & 0xff, i += zoom)
			for (int y = y1, j = 0; j >> 7 < h; y = y - 1 & 0x1ff, j += zoom)
				if ((px = chr[src | (i >> 7) * h | j >> 7]))
					data[y << 8 | x] = color | px;
	}

	void xferHxW_H(int *data, int src, int color, int y0, int x0, int h, int w, int zoom) {
		const int dh = h * 0x80 / zoom, dw = w * 0x80 / zoom, y1 = y0 + dh - 1 & 0x1ff, x1 = x0 - dw + 1 & 0xff;
		int px;

		if (dh <= 256 && (y0 < 16 || y0 >= 272) && (y1 < 16 || y1 >= 272) || dw <= 32 && (x0 < 16 || x0 >= 240) && (x1 < 16 || x1 >= 240))
			return;
		for (int x = x1, i = 0; i >> 7 < w; x = x + 1 & 0xff, i += zoom)
			for (int y = y0, j = 0; j >> 7 < h; y = y + 1 & 0x1ff, j += zoom)
				if ((px = chr[src | (i >> 7) * h | j >> 7]))
					data[y << 8 | x] = color | px;
	}

	void xferHxW_HV(int *data, int src, int color, int y0, int x0, int h, int w, int zoom) {
		const int dh = h * 0x80 / zoom, dw = w * 0x80 / zoom, y1 = y0 + dh - 1 & 0x1ff, x1 = x0 - dw + 1 & 0xff;
		int px;

		if (dh <= 256 && (y0 < 16 || y0 >= 272) && (y1 < 16 || y1 >= 272) || dw <= 32 && (x0 < 16 || x0 >= 240) && (x1 < 16 || x1 >= 240))
			return;
		for (int x = x1, i = 0; i >> 7 < w; x = x + 1 & 0xff, i += zoom)
			for (int y = y1, j = 0; j >> 7 < h; y = y - 1 & 0x1ff, j += zoom)
				if ((px = chr[src | (i >> 7) * h | j >> 7]))
					data[y << 8 | x] = color | px;
	}

	void init(int rate) {
		sound0 = new AY_3_8910(14318180 / 8, 0.3);
		sound1 = new AY_3_8910(14318180 / 8, 0.3);
		sound2 = new K005289(SND, 14318180 / 4, 0.3);
		sound3 = new VLM5030(vlm, 14318180 / 4, rate, 5);
		Z80::init();
	}
};

#endif //TWINBEE_H
