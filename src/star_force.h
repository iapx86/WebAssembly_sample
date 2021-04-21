/*
 *
 *	Star Force
 *
 */

#ifndef STAR_FORCE_H
#define STAR_FORCE_H

#include <algorithm>
#include <array>
#include "z80.h"
#include "sn76489.h"
#include "senjyo_sound.h"
#include "utils.h"
using namespace std;

enum {
	EXTEND_50000_200000_500000, EXTEND_100000_300000_800000, EXTEND_50000_200000, EXEND_100000_300000,
	EXTEND_50000, EXTEND_100000, EXTEND_200000, EXTEND_NO,
};

enum {
	DIFFICULTY_NORMAL, DIFFICULTY_DIFFICULT1, DIFFICULTY_DIFFICULT2,
	DIFFICULTY_DIFFICULT3, DIFFICULTY_DIFFICULT4, DIFFICULTY_DIFFICULT5,
};

struct StarForce {
	static array<uint8_t, 0x8000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x3000> FG;
	static array<uint8_t, 0x6000> BG1, BG2;
	static array<uint8_t, 0x3000> BG3;
	static array<uint8_t, 0xc000> OBJ;
	static array<uint8_t, 0x20> SND;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static SN76489 *sound0, *sound1, *sound2;
	static SenjyoSound *sound3;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	bool fTurbo = false;
	int nLife = 3;
	bool fDemoSound = true;
	int nExtend = EXTEND_50000_200000_500000;
	int nDifficulty = DIFFICULTY_NORMAL;

	array<uint8_t, 0x3c00> ram = {};
	array<uint8_t, 0x400> ram2 = {};
	array<uint8_t, 6> in = {0, 0, 0, 0, 0xc0, 0};
	bool cpu_irq = false;
	int cpu2_command = 0;
	struct {
		bool irq = false;
		bool fInterruptEnable = false;
	} pio;
	struct {
		bool irq = false;
		bool fInterruptEnable = false;
		int cmd = 0;
	} ctc;

	array<uint8_t, 0x8000> fg;
	array<uint8_t, 0x10000> bg1;
	array<uint8_t, 0x10000> bg2;
	array<uint8_t, 0x8000> bg3;
	array<uint8_t, 0x20000> obj;
	array<int, 0x200> rgb;
	array<int, width * height> bitmap;
	bool updated = false;

	Z80 cpu, cpu2;
	Timer<int> timer;
	Timer<double> timer2;

	StarForce() : cpu(4000000), cpu2(2000000), timer(60), timer2(2000000 / 2048 / 11) {
		// CPU周りの初期化
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[i].base = &PRG1[i << 8];
		for (int i = 0; i < 0x3c; i++) {
			cpu.memorymap[0x80 + i].base = &ram[i << 8];
			cpu.memorymap[0x80 + i].write = nullptr;
		}
		cpu.memorymap[0xd0].read = [&](int addr) -> int { return (addr &= 0xff) < 6 ? in[addr] : 0xff; };
		cpu.memorymap[0xd0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 2:
				return void(cpu_irq = false);
			case 4:
				return pio.irq = pio.fInterruptEnable, void(cpu2_command = data);
			}
		};

		cpu.check_interrupt = [&]() { return cpu_irq && cpu.interrupt(); };

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = &PRG2[i << 8];
		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[0x40 + i].base = &ram2[i << 8];
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		cpu2.memorymap[0x80].write = [&](int addr, int data) { sound0->write(data); };
		cpu2.memorymap[0x90].write = [&](int addr, int data) { sound1->write(data); };
		cpu2.memorymap[0xa0].write = [&](int addr, int data) { sound2->write(data); };
		cpu2.memorymap[0xd0].write = [&](int addr, int data) { sound3->write(1, data & 15); };
		for (int i = 0; i < 0x100; i++) {
			cpu2.iomap[i].read = [&](int addr) { return addr & 0xff ? 0xff : cpu2_command; };
			cpu2.iomap[i].write = [&](int addr, int data) {
				switch (addr & 0xff) {
				case 1:
					return void(data == 0xa7 && (pio.fInterruptEnable = true));
				case 9:
					return void(data == 0xd7 && (ctc.fInterruptEnable = true));
				case 0xa:
					if (ctc.cmd & 4) {
						sound3->write(0, (data ? data : 256) * (ctc.cmd & 0x20 ? 16 : 1));
						ctc.cmd &= ~4;
					} else if (data & 1)
						ctc.cmd = data;
					return;
				}
			};
		}

		cpu2.check_interrupt = [&]() {
			if (pio.irq && cpu2.interrupt(0))
				return pio.irq = false, true;
			if (ctc.irq && cpu2.interrupt(10))
				return ctc.irq = false, true;
			return false;
		};

		// Videoの初期化
		fg.fill(7), bg1.fill(7), bg2.fill(7), bg3.fill(7), obj.fill(7), bitmap.fill(0xff000000);
		convertGFX(&fg[0], &FG[0], 512, {rseq8(0, 8)}, {seq8(0, 1)}, {0, 0x8000, 0x10000}, 8);
		convertGFX(&bg1[0], &BG1[0], 256, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x10000, 0x20000}, 32);
		convertGFX(&bg2[0], &BG2[0], 256, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x10000, 0x20000}, 32);
		convertGFX(&bg3[0], &BG3[0], 128, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x8000, 0x10000}, 32);
		convertGFX(&obj[0], &OBJ[0], 512, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x20000, 0x40000}, 32);
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), cpu_irq = true; });
			timer2.execute(tick_rate, [&]() { ctc.irq = ctc.fInterruptEnable; });
			sound0->execute(tick_rate);
			sound1->execute(tick_rate);
			sound2->execute(tick_rate);
			sound3->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	StarForce *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 2:
				in[4] |= 0x30;
				break;
			case 3:
				in[4] &= ~0x30;
				break;
			case 4:
				in[4] = in[4] & ~0x30 | 0x10;
				break;
			case 5:
				in[4] = in[4] & ~0x30 | 0x20;
				break;
			}
			if (fDemoSound)
				in[4] |= 0x80;
			else
				in[4] &= ~0x80;
			switch (nExtend) {
			case EXTEND_50000_200000_500000:
				in[5] &= ~7;
				break;
			case EXTEND_100000_300000_800000:
				in[5] = in[5] & ~7 | 1;
				break;
			case EXTEND_50000_200000:
				in[5] = in[5] & ~7 | 2;
				break;
			case EXEND_100000_300000:
				in[5] = in[5] & ~7 | 3;
				break;
			case EXTEND_50000:
				in[5] = in[5] & ~7 | 4;
				break;
			case EXTEND_100000:
				in[5] = in[5] & ~7 | 5;
				break;
			case EXTEND_200000:
				in[5] = in[5] & ~7 | 6;
				break;
			case EXTEND_NO:
				in[5] |= 7;
				break;
			}
			switch (nDifficulty) {
			case DIFFICULTY_NORMAL:
				in[5] &= ~0x38;
				break;
			case DIFFICULTY_DIFFICULT1:
				in[5] = in[5] & ~0x38 | 8;
				break;
			case DIFFICULTY_DIFFICULT2:
				in[5] = in[5] & ~0x38 | 0x10;
				break;
			case DIFFICULTY_DIFFICULT3:
				in[5] = in[5] & ~0x38 | 0x18;
				break;
			case DIFFICULTY_DIFFICULT4:
				in[5] = in[5] & ~0x38 | 0x20;
				break;
			case DIFFICULTY_DIFFICULT5:
				in[5] = in[5] & ~0x38 | 0x28;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu_irq = false;
			cpu.reset();
			pio.irq = false;
			pio.fInterruptEnable = false;
			ctc.irq = false;
			ctc.fInterruptEnable = false;
			ctc.cmd = 0;
			cpu2.reset();
		}
		return this;
	}

	StarForce *updateInput() {
		in[2] = in[2] & ~0xd | (fCoin != 0) << 0 | (fStart1P != 0) << 2 | (fStart2P != 0) << 3;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		fTurbo && (in[0] ^= 1 << 4);
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
		in[0] = in[0] & ~(1 << 2 | fDown << 3) | fDown << 2;
	}

	void right(bool fDown) {
		in[0] = in[0] & ~(1 << 0 | fDown << 1) | fDown << 0;
	}

	void down(bool fDown) {
		in[0] = in[0] & ~(1 << 3 | fDown << 2) | fDown << 3;
	}

	void left(bool fDown) {
		in[0] = in[0] & ~(1 << 1 | fDown << 0) | fDown << 1;
	}

	void triggerA(bool fDown) {
		in[0] = in[0] & ~(1 << 4) | fDown << 4;
	}

	void triggerB(bool fDown) {
		!(fTurbo = fDown) && (in[0] &= ~(1 << 4));
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		for (int j = 0; j < 0x200; j++) {
			const int e = ram[0x1c00 + j], i = e >> 6 & 3, r = e << 2 & 12, g = e & 12, b = e >> 2 & 12;
			rgb[j] = 0xff000000 | (b ? b | i : 0) * 255 / 15 << 16 | (g ? g | i : 0) * 255 / 15 << 8 | (r ? r | i : 0) * 255 / 15;
		}

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256, i++)
			fill_n(&bitmap[p], 224, 0);

		// obj描画
		drawObj(bitmap.data(), 0);

		// bg描画
		int hScroll = (ram[0x1e20] | ram[0x1e21] << 8) + 15;
		int vScroll = ram[0x1e25];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		int k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x2000;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_3(bitmap.data(), p, ram[k]);

		// obj描画
		drawObj(bitmap.data(), 1);

		// bg描画
		hScroll = (ram[0x1e30] | ram[0x1e31] << 8) + 15;
		vScroll = ram[0x1e35];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x2800;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_2(bitmap.data(), p, ram[k]);

		// obj描画
		drawObj(bitmap.data(), 2);

		// bg描画
		hScroll = (ram[0x1e30] | ram[0x1e31] << 8) + 15;
		vScroll = ram[0x1e35];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x3000;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_1(bitmap.data(), p, ram[k]);

		// obj描画
		drawObj(bitmap.data(), 3);

		// fg描画
		p = 256 * 8 * 2 + 232;
		k = 0x1040;
		for (int i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(bitmap.data(), p, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void drawObj(int *data, int pri) {
		for (int k = 0x187c, i = 32; i != 0; k -= 4, --i) {
			if ((ram[k + 1] >> 4 & 3) != pri)
				continue;
			const int x = ram[k + 2] - 1 & 0xff;
			const int y = (ram[k + 3] - 1 & 0xff) + 16;
			if (ram[k] < 0xc0) {
				const int src = ram[k] | ram[k + 1] << 9;
				switch (ram[k + 1] >> 6) {
				case 0: // ノーマル
					xfer16x16(data, x | y << 8, src);
					break;
				case 1: // V反転
					xfer16x16V(data, x | y << 8, src);
					break;
				case 2: // H反転
					xfer16x16H(data, x | y << 8, src);
					break;
				case 3: // HV反転
					xfer16x16HV(data, x | y << 8, src);
					break;
				}
			} else {
				const int src = ram[k] << 2 & 0x1fc | ram[k + 1] << 9;
				switch (ram[k + 1] >> 6) {
				case 0: // ノーマル
					xfer16x16(data, x | y << 8, src | 2);
					xfer16x16(data, x + 16 & 0xff | y << 8, src | 0);
					xfer16x16(data, x | (y & 0xff) + 16 << 8, src | 3);
					xfer16x16(data, x + 16 & 0xff | (y & 0xff) + 16 << 8, src | 1);
					break;
				case 1: // V反転
					xfer16x16V(data, x | y << 8, src | 3);
					xfer16x16V(data, x + 16 & 0xff | y << 8, src | 1);
					xfer16x16V(data, x | (y & 0xff) + 16 << 8, src | 2);
					xfer16x16V(data, x + 16 & 0xff | (y & 0xff) + 16 << 8, src | 0);
					break;
				case 2: // H反転
					xfer16x16H(data, x | y << 8, src | 0);
					xfer16x16H(data, x + 16 & 0xff | y << 8, src | 2);
					xfer16x16H(data, x | (y & 0xff) + 16 << 8, src | 1);
					xfer16x16H(data, x + 16 & 0xff | (y & 0xff) + 16 << 8, src | 3);
					break;
				case 3: // HV反転
					xfer16x16HV(data, x | y << 8, src | 1);
					xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 3);
					xfer16x16HV(data, x | (y & 0xff) + 16 << 8, src | 0);
					xfer16x16HV(data, x + 16 & 0xff | (y & 0xff) + 16 << 8, src | 2);
					break;
				}
			}
		}
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = (ram[k] | ram[k + 0x400] << 4 & 0x100) << 6, idx = ram[k + 0x400] << 3 & 0x38;
		int px;

		(px = fg[q | 0x00]) && (data[p + 0x000] = idx | px);
		(px = fg[q | 0x01]) && (data[p + 0x001] = idx | px);
		(px = fg[q | 0x02]) && (data[p + 0x002] = idx | px);
		(px = fg[q | 0x03]) && (data[p + 0x003] = idx | px);
		(px = fg[q | 0x04]) && (data[p + 0x004] = idx | px);
		(px = fg[q | 0x05]) && (data[p + 0x005] = idx | px);
		(px = fg[q | 0x06]) && (data[p + 0x006] = idx | px);
		(px = fg[q | 0x07]) && (data[p + 0x007] = idx | px);
		(px = fg[q | 0x08]) && (data[p + 0x100] = idx | px);
		(px = fg[q | 0x09]) && (data[p + 0x101] = idx | px);
		(px = fg[q | 0x0a]) && (data[p + 0x102] = idx | px);
		(px = fg[q | 0x0b]) && (data[p + 0x103] = idx | px);
		(px = fg[q | 0x0c]) && (data[p + 0x104] = idx | px);
		(px = fg[q | 0x0d]) && (data[p + 0x105] = idx | px);
		(px = fg[q | 0x0e]) && (data[p + 0x106] = idx | px);
		(px = fg[q | 0x0f]) && (data[p + 0x107] = idx | px);
		(px = fg[q | 0x10]) && (data[p + 0x200] = idx | px);
		(px = fg[q | 0x11]) && (data[p + 0x201] = idx | px);
		(px = fg[q | 0x12]) && (data[p + 0x202] = idx | px);
		(px = fg[q | 0x13]) && (data[p + 0x203] = idx | px);
		(px = fg[q | 0x14]) && (data[p + 0x204] = idx | px);
		(px = fg[q | 0x15]) && (data[p + 0x205] = idx | px);
		(px = fg[q | 0x16]) && (data[p + 0x206] = idx | px);
		(px = fg[q | 0x17]) && (data[p + 0x207] = idx | px);
		(px = fg[q | 0x18]) && (data[p + 0x300] = idx | px);
		(px = fg[q | 0x19]) && (data[p + 0x301] = idx | px);
		(px = fg[q | 0x1a]) && (data[p + 0x302] = idx | px);
		(px = fg[q | 0x1b]) && (data[p + 0x303] = idx | px);
		(px = fg[q | 0x1c]) && (data[p + 0x304] = idx | px);
		(px = fg[q | 0x1d]) && (data[p + 0x305] = idx | px);
		(px = fg[q | 0x1e]) && (data[p + 0x306] = idx | px);
		(px = fg[q | 0x1f]) && (data[p + 0x307] = idx | px);
		(px = fg[q | 0x20]) && (data[p + 0x400] = idx | px);
		(px = fg[q | 0x21]) && (data[p + 0x401] = idx | px);
		(px = fg[q | 0x22]) && (data[p + 0x402] = idx | px);
		(px = fg[q | 0x23]) && (data[p + 0x403] = idx | px);
		(px = fg[q | 0x24]) && (data[p + 0x404] = idx | px);
		(px = fg[q | 0x25]) && (data[p + 0x405] = idx | px);
		(px = fg[q | 0x26]) && (data[p + 0x406] = idx | px);
		(px = fg[q | 0x27]) && (data[p + 0x407] = idx | px);
		(px = fg[q | 0x28]) && (data[p + 0x500] = idx | px);
		(px = fg[q | 0x29]) && (data[p + 0x501] = idx | px);
		(px = fg[q | 0x2a]) && (data[p + 0x502] = idx | px);
		(px = fg[q | 0x2b]) && (data[p + 0x503] = idx | px);
		(px = fg[q | 0x2c]) && (data[p + 0x504] = idx | px);
		(px = fg[q | 0x2d]) && (data[p + 0x505] = idx | px);
		(px = fg[q | 0x2e]) && (data[p + 0x506] = idx | px);
		(px = fg[q | 0x2f]) && (data[p + 0x507] = idx | px);
		(px = fg[q | 0x30]) && (data[p + 0x600] = idx | px);
		(px = fg[q | 0x31]) && (data[p + 0x601] = idx | px);
		(px = fg[q | 0x32]) && (data[p + 0x602] = idx | px);
		(px = fg[q | 0x33]) && (data[p + 0x603] = idx | px);
		(px = fg[q | 0x34]) && (data[p + 0x604] = idx | px);
		(px = fg[q | 0x35]) && (data[p + 0x605] = idx | px);
		(px = fg[q | 0x36]) && (data[p + 0x606] = idx | px);
		(px = fg[q | 0x37]) && (data[p + 0x607] = idx | px);
		(px = fg[q | 0x38]) && (data[p + 0x700] = idx | px);
		(px = fg[q | 0x39]) && (data[p + 0x701] = idx | px);
		(px = fg[q | 0x3a]) && (data[p + 0x702] = idx | px);
		(px = fg[q | 0x3b]) && (data[p + 0x703] = idx | px);
		(px = fg[q | 0x3c]) && (data[p + 0x704] = idx | px);
		(px = fg[q | 0x3d]) && (data[p + 0x705] = idx | px);
		(px = fg[q | 0x3e]) && (data[p + 0x706] = idx | px);
		(px = fg[q | 0x3f]) && (data[p + 0x707] = idx | px);
	}

	void xfer16x16_1(int *data, int dst, int src) {
		const int idx = src >> 4 & 8 | src >> 1 & 0x30 | 0x40;
		int px;

		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg1[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16_2(int *data, int dst, int src) {
		const int idx = src >> 2 & 0x38 | 0x80;
		int px;

		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg2[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16_3(int *data, int dst, int src) {
		const int idx = src >> 2 & 0x38 | 0xc0;
		int px;

		src = src << 8 & 0x7f00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg3[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x38 | 0x140;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = src << 8 & 0x1ff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x38 | 0x140;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0x1ff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x38 | 0x140;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0x1ff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x38 | 0x140;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0x1ff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new SN76489(2000000);
		sound1 = new SN76489(2000000);
		sound2 = new SN76489(2000000);
		sound3 = new SenjyoSound(SND, 2000000, rate);
		Z80::init();
	}
};

#endif //STAR_FORCE_H
