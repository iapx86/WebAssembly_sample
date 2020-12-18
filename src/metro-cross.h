/*
 *
 *	Metro-Cross
 *
 */

#ifndef METRO_CROSS_H
#define METRO_CROSS_H

#include <array>
#include <vector>
#include "mc6809.h"
#include "mc6801.h"
#include "c30.h"
#include "utils.h"
using namespace std;

enum {
	RANK_A, RANK_B, RANK_C, RANK_D,
};

struct MetroCross {
	static unsigned char PRG1[], PRG2[], PRG2I[], FG[], BG[], OBJ[], GREEN[], RED[];

	static const int cxScreen = 224;
	static const int cyScreen = 288;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static C30 *sound0;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nRank = RANK_A;
	bool fContinue = true;
	bool fAttract = true;
	bool fSelect = false;

	uint8_t ram[0x4800] = {};
	uint8_t ram2[0x900] = {};
	uint8_t in[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int select = 0;
	bool cpu_irq = false;
	bool mcu_irq = false;

	array<uint8_t, 0x8000> fg;
	array<uint8_t, 0x20000> bg;
	array<uint8_t, 0x20000> obj;
	array<int, 0x800> rgb;
	int vScroll[2] = {};
	int hScroll[2] = {};

	MC6809 cpu;
	MC6801 mcu;

	MetroCross() {
		// CPU周りの初期化
		for (int i = 0; i < 0x40; i++) {
			cpu.memorymap[i].base = ram + i * 0x100;
			cpu.memorymap[i].write = nullptr;
		}
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x40 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x40 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x48 + i].base = ram + 0x4000 + i * 0x100;
			cpu.memorymap[0x48 + i].write = nullptr;
		}
		for (int i = 0; i < 0xa0; i++)
			cpu.memorymap[0x60 + i].base = PRG1 + i * 0x100;
		cpu.memorymap[0xb0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(vScroll[0] = vScroll[0] & 0xff | data << 8);
			case 1:
				return void(vScroll[0] = vScroll[0] & 0xff00 | data);
			case 2:
				return void(hScroll[0] = data);
			case 4:
				return void(vScroll[1] = vScroll[1] & 0xff | data << 8);
			case 5:
				return void(vScroll[1] = vScroll[1] & 0xff00 | data);
			case 6:
				return void(hScroll[1] = data);
			}
		};

		cpu.check_interrupt = [&]() { return cpu_irq && cpu.interrupt() ? (cpu_irq = false, true) : false; };

		mcu.memorymap[0].read = [&](int addr) -> int {
			int data;
			switch (addr) {
			case 2:
				return in[select];
			case 8:
				return data = ram2[8], ram2[8] &= ~0xe0, data;
			}
			return ram2[addr];
		};
		mcu.memorymap[0].write = [&](int addr, int data) {
			if (addr == 2 && (data & 0xe0) == 0x60)
				select = data & 7;
			ram2[addr] = data;
		};
		for (int i = 0; i < 4; i++) {
			mcu.memorymap[0x10 + i].read = [&](int addr) { return sound0->read(addr); };
			mcu.memorymap[0x10 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x20; i++)
			mcu.memorymap[0x80 + i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 8; i++) {
			mcu.memorymap[0xc0 + i].base = ram2 + 0x100 + i * 0x100;
			mcu.memorymap[0xc0 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++)
			mcu.memorymap[0xf0 + i].base = PRG2I + i * 0x100;

		mcu.check_interrupt = [&]() { return mcu_irq && mcu.interrupt() ? (mcu_irq = false, true) : (ram2[8] & 0x48) == 0x48 && mcu.interrupt(MC6801_OCF); };

		// Videoの初期化
		fg.fill(3), bg.fill(7), obj.fill(15);
		convertGFX(&fg[0], FG, 512, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&bg[0], BG, 512, {rseq8(0, 16)}, {seq4(0, 1), seq4(8, 1)}, {0x40000, 0, 4}, 16);
		convertGFX(&bg[0x8000], &BG[0x2000], 512, {rseq8(0, 16)}, {seq4(0, 1), seq4(8, 1)}, {0x30004, 0, 4}, 16);
		convertGFX(&bg[0x10000], &BG[0x4000], 512, {rseq8(0, 16)}, {seq4(0, 1), seq4(8, 1)}, {0x30000, 0, 4}, 16);
		convertGFX(&bg[0x18000], &BG[0x6000], 512, {rseq8(0, 16)}, {seq4(0, 1), seq4(8, 1)}, {0x20004, 0, 4}, 16);
		convertGFX(&obj[0], OBJ, 256, {rseq16(0, 64)}, {seq16(0, 4)}, {seq4(0, 1)}, 128);
		for (int i = 0; i < 0x800; i++)
			rgb[i] = 0xff000000 | (GREEN[i] >> 4) * 255 / 15 << 16 | (GREEN[i] & 15) * 255 / 15 << 8 | RED[i] * 255 / 15;
	}

	MetroCross *execute() {
		cpu_irq = mcu_irq = true;
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		ram2[8] |= ram2[8] << 3 & 0x40;
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		return this;
	}

	void reset() {
		fReset = true;
	}

	MetroCross *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nRank) {
			case RANK_A: // Normal
				in[0] |= 3;
				break;
			case RANK_B: // Easy
				in[0] = in[0] & ~3 | 2;
				break;
			case RANK_C: // Hard
				in[0] = in[0] & ~3 | 1;
				break;
			case RANK_D: // Very Hard
				in[0] &= ~3;
				break;
			}
			if (fContinue)
				in[1] |= 0x10;
			else
				in[1] &= ~0x10;
			if (fAttract)
				in[1] |= 2;
			else
				in[1] &= ~2;
			if (fSelect)
				in[1] &= ~1;
			else
				in[1] |= 1;
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[0] &= ~0x10;
		else
			in[0] |= 0x10;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu_irq = mcu_irq = false;
			cpu.reset();
			mcu.reset();
		}
		return this;
	}

	MetroCross *updateInput() {
		in[4] = in[4] & ~0x19 | !fCoin << 0 | !fStart1P << 3 | !fStart2P << 4;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
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
		in[6] = in[6] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void right(bool fDown) {
		in[6] = in[6] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void down(bool fDown) {
		in[6] = in[6] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void left(bool fDown) {
		in[6] = in[6] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void triggerA(bool fDown) {
		in[6] = in[6] & ~(1 << 4) | !fDown << 4;
	}

	void makeBitmap(int *data) {
		int p, k, back;

		// bg描画
		back = (vScroll[0] & 0xe00) == 0xc00 ? 1 : 0;
		p = 256 * 8 * 2 + 232 + (25 + hScroll[back] & 7) - (25 - back * 2 + vScroll[back] & 7) * 256;
		k = 0x2000 | back << 12 | 25 + hScroll[back] << 4 & 0xf80 | 25 - back * 2 + vScroll[back] >> 2 & 0x7e;
		for (int i = 0; i < 29; k = k + 54 & 0x7e | k + 0x80 & 0xf80 | k & 0x3000, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x3f80, p += 256 * 8, j++)
				xfer8x8b0(data, p, k, back);

		// obj描画
		drawObj(data, 0);

		// bg描画
		back ^= 1;
		p = 256 * 8 * 2 + 232 + (25 + hScroll[back] & 7) - (25 - back * 2 + vScroll[back] & 7) * 256;
		k = 0x2000 | back << 12 | 25 + hScroll[back] << 4 & 0xf80 | 25 - back * 2 + vScroll[back] >> 2 & 0x7e;
		for (int i = 0; i < 29; k = k + 54 & 0x7e | k + 0x80 & 0xf80 | k & 0x3000, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x3f80, p += 256 * 8, j++)
				xfer8x8b1(data, p, k, back);

		// obj描画
		drawObj(data, 1);

		// fg描画
		p = 256 * 8 * 4 + 232;
		k = 0x4040;
		for (int i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8f(data, p, k);
		p = 256 * 8 * 36 + 232;
		k = 0x4002;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8f(data, p, k);
		p = 256 * 8 * 37 + 232;
		k = 0x4022;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8f(data, p, k);
		p = 256 * 8 * 2 + 232;
		k = 0x43c2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8f(data, p, k);
		p = 256 * 8 * 3 + 232;
		k = 0x43e2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8f(data, p, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void drawObj(int *data, int pri) {
		for (int k = 0x1800, i = 127; i != 0; k += 16, --i) {
			if ((ram[k + 4] & 1) != pri)
				continue;
			const int x = -1 + ram[k + 9] + ram[0x1ff7] & 0xff;
			const int y = -54 + (ram[k + 7] | ram[k + 6] << 8 & 0x100) + (ram[0x1ff5] | ram[0x1ff4] << 8 & 0x100) & 0x1ff;
			const int src = ram[k + 4] >> 4 & 1 | ram[k + 8] >> 3 & 2 | ram[k + 5] << 2 & 0x1fc | ram[k + 6] << 8 & 0xfe00;
			switch (ram[k + 8] & 5 | ram[k + 4] >> 4 & 0x0a) {
			case 0x00:
				xfer16x16(data, x | y << 8, src);
				break;
			case 0x01:
				xfer16x16H(data, x | y << 8, src);
				break;
			case 0x02:
				xfer16x16V(data, x | y << 8, src);
				break;
			case 0x03:
				xfer16x16HV(data, x | y << 8, src);
				break;
			case 0x04:
				xfer16x16(data, x | y << 8, src | 2);
				xfer16x16(data, x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x05:
				xfer16x16H(data, x | y << 8, src & ~2);
				xfer16x16H(data, x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x06:
				xfer16x16V(data, x | y << 8, src | 2);
				xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x07:
				xfer16x16HV(data, x | y << 8, src & ~2);
				xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x08:
				xfer16x16(data, x | y << 8, src & ~1);
				xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x09:
				xfer16x16H(data, x | y << 8, src & ~1);
				xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x0a:
				xfer16x16V(data, x | y << 8, src | 1);
				xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x0b:
				xfer16x16HV(data, x | y << 8, src | 1);
				xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x0c:
				xfer16x16(data, x | y << 8, src & ~3 | 2);
				xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 3);
				xfer16x16(data, x + 16 & 0xff | y << 8, src & ~3);
				xfer16x16(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				break;
			case 0x0d:
				xfer16x16H(data, x | y << 8, src & ~3);
				xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				xfer16x16H(data, x + 16 & 0xff | y << 8, src & ~3 | 2);
				xfer16x16H(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src | 3);
				break;
			case 0x0e:
				xfer16x16V(data, x | y << 8, src | 3);
				xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~3 | 1);
				xfer16x16V(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3);
				break;
			case 0x0f:
				xfer16x16HV(data, x | y << 8, src & ~3 | 1);
				xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~3);
				xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 3);
				xfer16x16HV(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				break;
			}
		}
	}

	void xfer8x8b0(int *data, int p, int k, int back) {
		const int q = (ram[k] | ram[k + 1] << 8 & 0x300 | back << 10) << 6;
		const int idx = ram[k + 1] << 3;

		data[p + 0x000] = idx | bg[q | 0x00];
		data[p + 0x001] = idx | bg[q | 0x01];
		data[p + 0x002] = idx | bg[q | 0x02];
		data[p + 0x003] = idx | bg[q | 0x03];
		data[p + 0x004] = idx | bg[q | 0x04];
		data[p + 0x005] = idx | bg[q | 0x05];
		data[p + 0x006] = idx | bg[q | 0x06];
		data[p + 0x007] = idx | bg[q | 0x07];
		data[p + 0x100] = idx | bg[q | 0x08];
		data[p + 0x101] = idx | bg[q | 0x09];
		data[p + 0x102] = idx | bg[q | 0x0a];
		data[p + 0x103] = idx | bg[q | 0x0b];
		data[p + 0x104] = idx | bg[q | 0x0c];
		data[p + 0x105] = idx | bg[q | 0x0d];
		data[p + 0x106] = idx | bg[q | 0x0e];
		data[p + 0x107] = idx | bg[q | 0x0f];
		data[p + 0x200] = idx | bg[q | 0x10];
		data[p + 0x201] = idx | bg[q | 0x11];
		data[p + 0x202] = idx | bg[q | 0x12];
		data[p + 0x203] = idx | bg[q | 0x13];
		data[p + 0x204] = idx | bg[q | 0x14];
		data[p + 0x205] = idx | bg[q | 0x15];
		data[p + 0x206] = idx | bg[q | 0x16];
		data[p + 0x207] = idx | bg[q | 0x17];
		data[p + 0x300] = idx | bg[q | 0x18];
		data[p + 0x301] = idx | bg[q | 0x19];
		data[p + 0x302] = idx | bg[q | 0x1a];
		data[p + 0x303] = idx | bg[q | 0x1b];
		data[p + 0x304] = idx | bg[q | 0x1c];
		data[p + 0x305] = idx | bg[q | 0x1d];
		data[p + 0x306] = idx | bg[q | 0x1e];
		data[p + 0x307] = idx | bg[q | 0x1f];
		data[p + 0x400] = idx | bg[q | 0x20];
		data[p + 0x401] = idx | bg[q | 0x21];
		data[p + 0x402] = idx | bg[q | 0x22];
		data[p + 0x403] = idx | bg[q | 0x23];
		data[p + 0x404] = idx | bg[q | 0x24];
		data[p + 0x405] = idx | bg[q | 0x25];
		data[p + 0x406] = idx | bg[q | 0x26];
		data[p + 0x407] = idx | bg[q | 0x27];
		data[p + 0x500] = idx | bg[q | 0x28];
		data[p + 0x501] = idx | bg[q | 0x29];
		data[p + 0x502] = idx | bg[q | 0x2a];
		data[p + 0x503] = idx | bg[q | 0x2b];
		data[p + 0x504] = idx | bg[q | 0x2c];
		data[p + 0x505] = idx | bg[q | 0x2d];
		data[p + 0x506] = idx | bg[q | 0x2e];
		data[p + 0x507] = idx | bg[q | 0x2f];
		data[p + 0x600] = idx | bg[q | 0x30];
		data[p + 0x601] = idx | bg[q | 0x31];
		data[p + 0x602] = idx | bg[q | 0x32];
		data[p + 0x603] = idx | bg[q | 0x33];
		data[p + 0x604] = idx | bg[q | 0x34];
		data[p + 0x605] = idx | bg[q | 0x35];
		data[p + 0x606] = idx | bg[q | 0x36];
		data[p + 0x607] = idx | bg[q | 0x37];
		data[p + 0x700] = idx | bg[q | 0x38];
		data[p + 0x701] = idx | bg[q | 0x39];
		data[p + 0x702] = idx | bg[q | 0x3a];
		data[p + 0x703] = idx | bg[q | 0x3b];
		data[p + 0x704] = idx | bg[q | 0x3c];
		data[p + 0x705] = idx | bg[q | 0x3d];
		data[p + 0x706] = idx | bg[q | 0x3e];
		data[p + 0x707] = idx | bg[q | 0x3f];
	}

	void xfer8x8b1(int *data, int p, int k, int back) {
		const int q = (ram[k] | ram[k + 1] << 8 & 0x300 | back << 10) << 6;
		const int idx = ram[k + 1] << 3;
		int px;

		(px = bg[q | 0x00]) != 7 && (data[p + 0x000] = idx | px);
		(px = bg[q | 0x01]) != 7 && (data[p + 0x001] = idx | px);
		(px = bg[q | 0x02]) != 7 && (data[p + 0x002] = idx | px);
		(px = bg[q | 0x03]) != 7 && (data[p + 0x003] = idx | px);
		(px = bg[q | 0x04]) != 7 && (data[p + 0x004] = idx | px);
		(px = bg[q | 0x05]) != 7 && (data[p + 0x005] = idx | px);
		(px = bg[q | 0x06]) != 7 && (data[p + 0x006] = idx | px);
		(px = bg[q | 0x07]) != 7 && (data[p + 0x007] = idx | px);
		(px = bg[q | 0x08]) != 7 && (data[p + 0x100] = idx | px);
		(px = bg[q | 0x09]) != 7 && (data[p + 0x101] = idx | px);
		(px = bg[q | 0x0a]) != 7 && (data[p + 0x102] = idx | px);
		(px = bg[q | 0x0b]) != 7 && (data[p + 0x103] = idx | px);
		(px = bg[q | 0x0c]) != 7 && (data[p + 0x104] = idx | px);
		(px = bg[q | 0x0d]) != 7 && (data[p + 0x105] = idx | px);
		(px = bg[q | 0x0e]) != 7 && (data[p + 0x106] = idx | px);
		(px = bg[q | 0x0f]) != 7 && (data[p + 0x107] = idx | px);
		(px = bg[q | 0x10]) != 7 && (data[p + 0x200] = idx | px);
		(px = bg[q | 0x11]) != 7 && (data[p + 0x201] = idx | px);
		(px = bg[q | 0x12]) != 7 && (data[p + 0x202] = idx | px);
		(px = bg[q | 0x13]) != 7 && (data[p + 0x203] = idx | px);
		(px = bg[q | 0x14]) != 7 && (data[p + 0x204] = idx | px);
		(px = bg[q | 0x15]) != 7 && (data[p + 0x205] = idx | px);
		(px = bg[q | 0x16]) != 7 && (data[p + 0x206] = idx | px);
		(px = bg[q | 0x17]) != 7 && (data[p + 0x207] = idx | px);
		(px = bg[q | 0x18]) != 7 && (data[p + 0x300] = idx | px);
		(px = bg[q | 0x19]) != 7 && (data[p + 0x301] = idx | px);
		(px = bg[q | 0x1a]) != 7 && (data[p + 0x302] = idx | px);
		(px = bg[q | 0x1b]) != 7 && (data[p + 0x303] = idx | px);
		(px = bg[q | 0x1c]) != 7 && (data[p + 0x304] = idx | px);
		(px = bg[q | 0x1d]) != 7 && (data[p + 0x305] = idx | px);
		(px = bg[q | 0x1e]) != 7 && (data[p + 0x306] = idx | px);
		(px = bg[q | 0x1f]) != 7 && (data[p + 0x307] = idx | px);
		(px = bg[q | 0x20]) != 7 && (data[p + 0x400] = idx | px);
		(px = bg[q | 0x21]) != 7 && (data[p + 0x401] = idx | px);
		(px = bg[q | 0x22]) != 7 && (data[p + 0x402] = idx | px);
		(px = bg[q | 0x23]) != 7 && (data[p + 0x403] = idx | px);
		(px = bg[q | 0x24]) != 7 && (data[p + 0x404] = idx | px);
		(px = bg[q | 0x25]) != 7 && (data[p + 0x405] = idx | px);
		(px = bg[q | 0x26]) != 7 && (data[p + 0x406] = idx | px);
		(px = bg[q | 0x27]) != 7 && (data[p + 0x407] = idx | px);
		(px = bg[q | 0x28]) != 7 && (data[p + 0x500] = idx | px);
		(px = bg[q | 0x29]) != 7 && (data[p + 0x501] = idx | px);
		(px = bg[q | 0x2a]) != 7 && (data[p + 0x502] = idx | px);
		(px = bg[q | 0x2b]) != 7 && (data[p + 0x503] = idx | px);
		(px = bg[q | 0x2c]) != 7 && (data[p + 0x504] = idx | px);
		(px = bg[q | 0x2d]) != 7 && (data[p + 0x505] = idx | px);
		(px = bg[q | 0x2e]) != 7 && (data[p + 0x506] = idx | px);
		(px = bg[q | 0x2f]) != 7 && (data[p + 0x507] = idx | px);
		(px = bg[q | 0x30]) != 7 && (data[p + 0x600] = idx | px);
		(px = bg[q | 0x31]) != 7 && (data[p + 0x601] = idx | px);
		(px = bg[q | 0x32]) != 7 && (data[p + 0x602] = idx | px);
		(px = bg[q | 0x33]) != 7 && (data[p + 0x603] = idx | px);
		(px = bg[q | 0x34]) != 7 && (data[p + 0x604] = idx | px);
		(px = bg[q | 0x35]) != 7 && (data[p + 0x605] = idx | px);
		(px = bg[q | 0x36]) != 7 && (data[p + 0x606] = idx | px);
		(px = bg[q | 0x37]) != 7 && (data[p + 0x607] = idx | px);
		(px = bg[q | 0x38]) != 7 && (data[p + 0x700] = idx | px);
		(px = bg[q | 0x39]) != 7 && (data[p + 0x701] = idx | px);
		(px = bg[q | 0x3a]) != 7 && (data[p + 0x702] = idx | px);
		(px = bg[q | 0x3b]) != 7 && (data[p + 0x703] = idx | px);
		(px = bg[q | 0x3c]) != 7 && (data[p + 0x704] = idx | px);
		(px = bg[q | 0x3d]) != 7 && (data[p + 0x705] = idx | px);
		(px = bg[q | 0x3e]) != 7 && (data[p + 0x706] = idx | px);
		(px = bg[q | 0x3f]) != 7 && (data[p + 0x707] = idx | px);
	}

	void xfer8x8f(int *data, int p, int k) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 4 & 0x7f0;
		int px;

		(px = fg[q | 0x00]) != 3 && (data[p + 0x000] = idx | px);
		(px = fg[q | 0x01]) != 3 && (data[p + 0x001] = idx | px);
		(px = fg[q | 0x02]) != 3 && (data[p + 0x002] = idx | px);
		(px = fg[q | 0x03]) != 3 && (data[p + 0x003] = idx | px);
		(px = fg[q | 0x04]) != 3 && (data[p + 0x004] = idx | px);
		(px = fg[q | 0x05]) != 3 && (data[p + 0x005] = idx | px);
		(px = fg[q | 0x06]) != 3 && (data[p + 0x006] = idx | px);
		(px = fg[q | 0x07]) != 3 && (data[p + 0x007] = idx | px);
		(px = fg[q | 0x08]) != 3 && (data[p + 0x100] = idx | px);
		(px = fg[q | 0x09]) != 3 && (data[p + 0x101] = idx | px);
		(px = fg[q | 0x0a]) != 3 && (data[p + 0x102] = idx | px);
		(px = fg[q | 0x0b]) != 3 && (data[p + 0x103] = idx | px);
		(px = fg[q | 0x0c]) != 3 && (data[p + 0x104] = idx | px);
		(px = fg[q | 0x0d]) != 3 && (data[p + 0x105] = idx | px);
		(px = fg[q | 0x0e]) != 3 && (data[p + 0x106] = idx | px);
		(px = fg[q | 0x0f]) != 3 && (data[p + 0x107] = idx | px);
		(px = fg[q | 0x10]) != 3 && (data[p + 0x200] = idx | px);
		(px = fg[q | 0x11]) != 3 && (data[p + 0x201] = idx | px);
		(px = fg[q | 0x12]) != 3 && (data[p + 0x202] = idx | px);
		(px = fg[q | 0x13]) != 3 && (data[p + 0x203] = idx | px);
		(px = fg[q | 0x14]) != 3 && (data[p + 0x204] = idx | px);
		(px = fg[q | 0x15]) != 3 && (data[p + 0x205] = idx | px);
		(px = fg[q | 0x16]) != 3 && (data[p + 0x206] = idx | px);
		(px = fg[q | 0x17]) != 3 && (data[p + 0x207] = idx | px);
		(px = fg[q | 0x18]) != 3 && (data[p + 0x300] = idx | px);
		(px = fg[q | 0x19]) != 3 && (data[p + 0x301] = idx | px);
		(px = fg[q | 0x1a]) != 3 && (data[p + 0x302] = idx | px);
		(px = fg[q | 0x1b]) != 3 && (data[p + 0x303] = idx | px);
		(px = fg[q | 0x1c]) != 3 && (data[p + 0x304] = idx | px);
		(px = fg[q | 0x1d]) != 3 && (data[p + 0x305] = idx | px);
		(px = fg[q | 0x1e]) != 3 && (data[p + 0x306] = idx | px);
		(px = fg[q | 0x1f]) != 3 && (data[p + 0x307] = idx | px);
		(px = fg[q | 0x20]) != 3 && (data[p + 0x400] = idx | px);
		(px = fg[q | 0x21]) != 3 && (data[p + 0x401] = idx | px);
		(px = fg[q | 0x22]) != 3 && (data[p + 0x402] = idx | px);
		(px = fg[q | 0x23]) != 3 && (data[p + 0x403] = idx | px);
		(px = fg[q | 0x24]) != 3 && (data[p + 0x404] = idx | px);
		(px = fg[q | 0x25]) != 3 && (data[p + 0x405] = idx | px);
		(px = fg[q | 0x26]) != 3 && (data[p + 0x406] = idx | px);
		(px = fg[q | 0x27]) != 3 && (data[p + 0x407] = idx | px);
		(px = fg[q | 0x28]) != 3 && (data[p + 0x500] = idx | px);
		(px = fg[q | 0x29]) != 3 && (data[p + 0x501] = idx | px);
		(px = fg[q | 0x2a]) != 3 && (data[p + 0x502] = idx | px);
		(px = fg[q | 0x2b]) != 3 && (data[p + 0x503] = idx | px);
		(px = fg[q | 0x2c]) != 3 && (data[p + 0x504] = idx | px);
		(px = fg[q | 0x2d]) != 3 && (data[p + 0x505] = idx | px);
		(px = fg[q | 0x2e]) != 3 && (data[p + 0x506] = idx | px);
		(px = fg[q | 0x2f]) != 3 && (data[p + 0x507] = idx | px);
		(px = fg[q | 0x30]) != 3 && (data[p + 0x600] = idx | px);
		(px = fg[q | 0x31]) != 3 && (data[p + 0x601] = idx | px);
		(px = fg[q | 0x32]) != 3 && (data[p + 0x602] = idx | px);
		(px = fg[q | 0x33]) != 3 && (data[p + 0x603] = idx | px);
		(px = fg[q | 0x34]) != 3 && (data[p + 0x604] = idx | px);
		(px = fg[q | 0x35]) != 3 && (data[p + 0x605] = idx | px);
		(px = fg[q | 0x36]) != 3 && (data[p + 0x606] = idx | px);
		(px = fg[q | 0x37]) != 3 && (data[p + 0x607] = idx | px);
		(px = fg[q | 0x38]) != 3 && (data[p + 0x700] = idx | px);
		(px = fg[q | 0x39]) != 3 && (data[p + 0x701] = idx | px);
		(px = fg[q | 0x3a]) != 3 && (data[p + 0x702] = idx | px);
		(px = fg[q | 0x3b]) != 3 && (data[p + 0x703] = idx | px);
		(px = fg[q | 0x3c]) != 3 && (data[p + 0x704] = idx | px);
		(px = fg[q | 0x3d]) != 3 && (data[p + 0x705] = idx | px);
		(px = fg[q | 0x3e]) != 3 && (data[p + 0x706] = idx | px);
		(px = fg[q | 0x3f]) != 3 && (data[p + 0x707] = idx | px);
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 5 & 0x7f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0x1ff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]) != 0xf)
					data[dst] = idx | px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 5 & 0x7f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]) != 0xf)
					data[dst] = idx | px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 5 & 0x7f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]) != 0xf)
					data[dst] = idx | px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 5 & 0x7f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]) != 0xf)
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new C30(49152000 / 2048, rate);
	}
};

#endif //METRO_CROSS_H
