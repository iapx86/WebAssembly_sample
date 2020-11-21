/*
 *
 *	Gradius
 *
 */

#ifndef GRADIUS_H
#define GRADIUS_H

#include <list>
#include "mc68000.h"
#include "z80.h"
#include "ay-3-8910.h"
#include "k005289.h"
#include "vlm5030.h"
using namespace std;

enum {
	BONUS_20000_EVERY_70000, BONUS_30000_EVERY_80000, BONUS_20000_ONLY, BONUS_30000_ONLY,
};

enum {
	RANK_EASY, RANK_NORMAL, RANK_HARD, RANK_HARDEST,
};

struct Gradius {
	static unsigned char SND[], PRG1[], PRG2[];

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

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
	int nBonus = BONUS_30000_EVERY_80000;
	int nRank = RANK_NORMAL;
	bool fDemoSound = true;

	bool fInterrupt2Enable = false;
	bool fInterrupt4Enable = false;

	uint8_t ram[0x49000] = {};
	uint8_t ram2[0x4000] = {};
	uint8_t vlm[0x800] = {};
	uint8_t in[6] = {0xff, 0x53, 0xff, 0xff, 0xff, 0xff};
	struct {
		int addr = 0;
	} psg[2];
	struct {
		int freq0 = 0;
		int freq1 = 0;
	} scc;
	int vlm_latch = 0;
	int count = 0;
	int timer = 0;
	list<int> command;

	uint8_t chr[0x20000] = {};
	int rgb[0x800] = {};
	int flip = 0;
	uint8_t intensity[32] = {};

	MC68000 cpu;
	Z80 cpu2;

	Gradius() {
		// CPU周りの初期化
		for (int i = 0; i < 0x100; i++)
			cpu.memorymap[i].base = PRG1 + i * 0x100;
		for (int i = 0; i < 0x100; i++) {
			cpu.memorymap[0x100 + i].base = ram + i * 0x100;
			cpu.memorymap[0x100 + i].write = nullptr;
		}
		for (int i = 0; i < 0x80; i++) {
			cpu.memorymap[0x200 + i].read = [&](int addr) { return addr & 1 ? ram2[addr >> 1 & 0x3fff] : 0; };
			cpu.memorymap[0x200 + i].write = [&](int addr, int data) { addr & 1 && (ram2[addr >> 1 & 0x3fff] = data); };
		}
		for (int i = 0; i < 0x100; i++) {
			cpu.memorymap[0x300 + i].base = ram + 0x10000 + i * 0x100;
			cpu.memorymap[0x300 + i].write = [&](int addr, int data) {
				int offset = addr & 0xffff;
				ram[0x10000 | offset] = data, chr[offset <<= 1] = data >> 4, chr[1 | offset] = data & 0xf;
			};
		}
		for (int i = 0; i < 0x80; i++) {
			cpu.memorymap[0x500 + i].base = ram + 0x20000 + i * 0x100;
			cpu.memorymap[0x500 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu.memorymap[0x5a0 + i].base = ram + 0x28000 + i * 0x100;
			cpu.memorymap[0x5a0 + i].write = nullptr;
			cpu.memorymap[0x5a0 + i].write16 = [&](int addr, int data) {
				const int offset = addr & 0xffe;
				ram[0x28000 | offset] = data >> 8, ram[0x28001 | offset] = data;
				rgb[offset >> 1] = intensity[data & 0x1f]	// Red
					| intensity[data >> 5 & 0x1f] << 8		// Green
					| intensity[data >> 10 & 0x1f] << 16	// Blue
					| 0xff000000;							// Alpha
			};
		}
		cpu.memorymap[0x5c0].write = [&](int addr, int data) { if (addr == 0x5c001) command.push_back(data); };
		cpu.memorymap[0x5c4].read = [&](int addr) { return addr >= 0x5c402 && addr < 0x5c408 ? in[addr - 0x5c402 >> 1] : 0xff; };
		cpu.memorymap[0x5cc].read = [&](int addr) { return addr < 0x5cc06 ? in[addr - 0x5cc00 + 6 >> 1] : 0xff; };
		cpu.memorymap[0x5d0].read = [&](int addr) { return addr == 0x5d001 ? 0 : 0xff; };
		cpu.memorymap[0x5e0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 1:
				return void(fInterrupt2Enable = (data & 1) != 0);
			case 5:
				return void(flip = flip & 2 | data & 1);
			case 7:
				return void(flip = flip & 1 | data << 1 & 2);
			case 0xe:
				return void(fInterrupt4Enable = (data & 1) != 0);
			}
		};
		for (int i = 0; i < 0x200; i++) {
			cpu.memorymap[0x600 + i].base = ram + 0x29000 + i * 0x100;
			cpu.memorymap[0x600 + i].write = nullptr;
		}
		for (int i = 0; i < 0x400; i++)
			cpu.memorymap[0x800 + i].base = PRG1 + 0x10000 + i * 0x100;

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 0x40; i++) {
			cpu2.memorymap[0x40 + i].base = ram2 + i * 0x100;
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu2.memorymap[0x80 + i].base = vlm + i * 0x100;
			cpu2.memorymap[0x80 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu2.memorymap[0xa0 + i].write = [&](int addr, int data) { scc.freq0 = ~addr & 0xfff; };
			cpu2.memorymap[0xc0 + i].write = [&](int addr, int data) { scc.freq1 = ~addr & 0xfff; };
		}
		cpu2.memorymap[0xe0].read = [&](int addr) {
			int data;
			switch (addr & 0xff) {
			case 1:
				return !command.empty() ? (data = command.front(), command.pop_front(), data) : 0xff;
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
				return sound2->write(2, scc.freq0, count);
			case 4:
				return sound2->write(3, scc.freq1, count);
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
				sound0->write(psg[0].addr, data, count);
		};
		cpu2.memorymap[0xe2].read = [&](int addr) { return addr == 0xe205 ? sound1->read(psg[1].addr) : 0xff; };
		cpu2.memorymap[0xe4].write = [&](int addr, int data) {
			if (addr == 0xe405) {
				if ((psg[1].addr & 0xe) == 0xe)
					sound2->write(psg[1].addr & 1, data, count);
				sound1->write(psg[1].addr, data, count);
			}
		};

		// Videoの初期化
		for (auto& e : rgb)
			e = 0xff000000;

		// 輝度の計算
		double _intensity[32];
		const double r[5] = {4700, 2400, 1200, 620, 300};
		for (int i = 0; i < 32; i++) {
			double rt = 0, v = 0;
			for (int j = 0; j < 5; j++)
				if (~i >> j & 1)
					rt += 1 / r[j], v += 0.05 / r[j];
			_intensity[i] = ((v + 0.005) / (rt + 0.001) - 0.7) * 255 / 5.0 + 0.4;
		}
		const double black = _intensity[0], white = 255 / (_intensity[31] - black);
		for (int i = 0; i < 32; i++)
			intensity[i] = (_intensity[i] - black) * white + 0.5;
	}

	Gradius *execute() {
		for (int vpos = 0; vpos < 256; vpos++) {
			if (!vpos && fInterrupt2Enable)
				cpu.interrupt(2);
			if (vpos == 120 && fInterrupt4Enable)
				cpu.interrupt(4);
			cpu.execute(64);
		}
		for (count = 0; count < 58; count++) { // 14318180 / 4 / 60 / 1024
			if (!command.empty())
				cpu2.interrupt();
			sound0->write(0x0e, timer & 0x2f | sound3->BSY << 5 | 0xd0);
			cpu2.execute(146);
			timer = timer + 1 & 0xff;
		}
		cpu2.non_maskable_interrupt();
		return this;
	}

	void reset() {
		fReset = true;
	}

	Gradius *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[1] |= 3;
				break;
			case 4:
				in[1] = in[1] & ~3 | 2;
				break;
			case 5:
				in[1] = in[1] & ~3 | 1;
				break;
			case 7:
				in[1] &= ~3;
				break;
			}
			switch (nBonus) {
			case BONUS_20000_EVERY_70000:
				in[1] |= 0x18;
				break;
			case BONUS_30000_EVERY_80000:
				in[1] = in[1] & ~0x18 | 0x10;
				break;
			case BONUS_20000_ONLY:
				in[1] = in[1] & ~0x18 | 8;
				break;
			case BONUS_30000_ONLY:
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
			cpu.reset();
			fInterrupt2Enable = false;
			fInterrupt4Enable = false;
			command.clear();
			cpu2.reset();
			timer = 0;
		}
		return this;
	}

	Gradius *updateInput() {
		in[3] = in[3] & ~0x1c | !fCoin << 2 | !fStart1P << 3 | !fStart2P << 4;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		fTurbo && (in[4] ^= 1 << 6);
		return this;
	}

	void coin() {
		fCoin = 2;
	}

	void start1P() {
		fStart1P = 2;
	}

	void start2P() {
		fStart2P = 2;
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
		in[4] = in[4] & ~(1 << 6) | !fDown << 6;
	}

	void triggerB(bool fDown) {
		in[4] = in[4] & ~(1 << 5) | !fDown << 5;
	}

	void triggerX(bool fDown) {
		in[4] = in[4] & ~(1 << 4) | !fDown << 4;
	}

	void triggerY(bool fDown) {
		!(fTurbo = fDown) && (in[4] |= 1 << 6);
	}

	void makeBitmap(int *data) {
		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256, i++)
			memset(&data[p], 0, 224 * sizeof(int));

		// bg描画
		for (int k = 0x23000; k < 0x24000; k += 2)
			if (!(ram[k] & 0x50) && ram[k] & 0xf8)
				xfer8x8(data, k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if (!(ram[k] & 0x50) && ram[k] & 0xf8)
				xfer8x8(data, k);
		for (int k = 0x23000; k < 0x24000; k += 2)
			if ((ram[k] & 0x50) == 0x40 && ram[k] & 0xf8)
				xfer8x8(data, k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if ((ram[k] & 0x50) == 0x40 && ram[k] & 0xf8)
				xfer8x8(data, k);

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
					xferHxW(data, src, color, y, x, h, w, zoom);
					break;
				case 1:
					xferHxW_V(data, src, color, y, x, h, w, zoom);
					break;
				case 2:
					xferHxW_H(data, src, color, y, x, h, w, zoom);
					break;
				case 3:
					xferHxW_HV(data, src, color, y, x, h, w, zoom);
					break;
				}
			}

		// bg描画
		for (int k = 0x23000; k < 0x24000; k += 2)
			if (ram[k] & 0x10 && ram[k] & 0xf8)
				xfer8x8(data, k);
		for (int k = 0x22000; k < 0x23000; k += 2)
			if (ram[k] & 0x10 && ram[k] & 0xf8)
				xfer8x8(data, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
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
		sound0 = new AY_3_8910(14318180 / 8, rate, 58, 0.3);
		sound1 = new AY_3_8910(14318180 / 8, rate, 58, 0.3);
		sound2 = new K005289(SND, 14318180 / 4, rate, 58, 0.3);
		sound3 = new VLM5030(vlm, sizeof(vlm), 14318180 / 4, rate, 5);
		Z80::init();
	}
};

#endif //GRADIUS_H
