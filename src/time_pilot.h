/*
 *
 *	Time Pilot
 *
 */

#ifndef TIME_PILOT_H
#define TIME_PILOT_H

#include <algorithm>
#include <array>
#include <list>
#include "z80.h"
#include "ay-3-8910.h"
#include "utils.h"
using namespace std;

enum {
	BONUS_10000_50000, BONUS_20000_60000,
};

struct TimePilot {
	static array<uint8_t, 0x2000> BG;
	static array<uint8_t, 0x4000> OBJ;
	static array<uint8_t, 0x100> BGCOLOR, OBJCOLOR;
	static array<uint8_t, 0x20> RGB_H, RGB_L;
	static array<uint8_t, 0x6000> PRG1;
	static array<uint8_t, 0x1000> PRG2;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static AY_3_8910 *sound0, *sound1;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nLife = 3;
	int nBonus = BONUS_10000_50000;
	int nDifficulty = 4;

	bool fInterruptEnable = false;
//	bool fSoundEnable = false;

	array<uint8_t, 0x1400> ram = {};
	array<uint8_t, 5> in = {0xff, 0xff, 0xff, 0xff, 0x4b};
	array<uint8_t, 0x400> ram2 = {};
	struct {
		int addr = 0;
	} psg[2];
	int count = 0;
	int timer = 0;
	list<int> command;

	array<uint8_t, 0x8000> bg;
	array<uint8_t, 0x10000> obj;
	array<uint8_t, 0x100> bgcolor;
	array<int, 0x20> rgb;
	int vpos = 0;

	Z80 cpu, cpu2;

	TimePilot() {
		// CPU周りの初期化
		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x5f))
				cpu.memorymap[page].base = &PRG1[(page & 0x7f) << 8];
			else if (range(page, 0xa0, 0xaf)) {
				cpu.memorymap[page].base = &ram[(page & 0xf) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0xb0, 0xb0, 0x0b)) {
				cpu.memorymap[page].base = &ram[0x1000];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0xb4, 0xb4, 0x0b)) {
				cpu.memorymap[page].base = &ram[0x1100];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0xc0, 0xc0, 0x0c)) {
				cpu.memorymap[page].read = [&](int addr) { return vpos; };
				cpu.memorymap[page].write = [&](int addr, int data) { command.push_back(data); };
			} else if (range(page, 0xc2, 0xc2, 0x0c))
				cpu.memorymap[page].read = [&](int addr) -> int { return in[4]; };
			else if (range(page, 0xc3, 0xc3, 0x0c)) {
				cpu.memorymap[page].read = [&](int addr) -> int { return in[addr >> 5 & 3]; };
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr >> 1 & 0x7f) {
					case 0:
						return void(fInterruptEnable = (data & 1) != 0);
//					case 3:
//						return void(fSoundEnable = (data & 1) == 0);
					}
				};
			}

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x0f))
				cpu2.memorymap[page].base = &PRG2[(page & 0xf) << 8];
			else if (range(page, 0x30, 0x33, 0x0c)) {
				cpu2.memorymap[page].base = &ram2[(page & 3) << 8];
				cpu2.memorymap[page].write = nullptr;
			} else if (range(page, 0x40, 0x40, 0x0f)) {
				cpu2.memorymap[page].read = [&](int addr) { return sound0->read(psg[0].addr); };
				cpu2.memorymap[page].write = [&](int addr, int data) { sound0->write(psg[0].addr, data, count); };
			} else if (range(page, 0x50, 0x50, 0x0f))
				cpu2.memorymap[page].write = [&](int addr, int data) { psg[0].addr = data; };
			else if (range(page, 0x60, 0x60, 0x0f)) {
				cpu2.memorymap[page].read = [&](int addr) { return sound1->read(psg[1].addr); };
				cpu2.memorymap[page].write = [&](int addr, int data) { sound1->write(psg[1].addr, data, count); };
			} else if (range(page, 0x70, 0x70, 0x0f))
				cpu2.memorymap[page].write = [&](int addr, int data) { psg[1].addr = data; };

		// Videoの初期化
		bg.fill(3), obj.fill(3);
		convertGFX(&bg[0], &BG[0], 512, {rseq8(0, 8)}, {seq4(0, 1), seq4(64, 1)}, {4, 0}, 16);
		convertGFX(&obj[0], &OBJ[0], 256, {rseq8(256, 8), rseq8(0, 8)}, {rseq4(192, 1), rseq4(128, 1), rseq4(64, 1), rseq4(0, 1)}, {4, 0}, 64);
		for (int i = 0; i < bgcolor.size(); i++)
			bgcolor[i] = 0x10 | BGCOLOR[i];
		for (int i = 0; i < rgb.size(); i++) {
			const int e = RGB_H[i] << 8 | RGB_L[i];
			rgb[i] = 0xff000000 | (e >> 11) * 255 / 31 << 16 | (e >> 6 & 31) * 255 / 31 << 8 | (e >> 1 & 31) * 255 / 31;
		}
	}

	TimePilot *execute() {
//		sound0->mute(!fSoundEnable);
//		sound1->mute(!fSoundEnable);
		for (int i = 0; i < 256; i++) {
			vpos = i + 144 & 0xff;
			if (!vpos)
				copy_n(&ram[0x1000], 0x200, &ram[0x1200]);
			vpos == 240 && fInterruptEnable && cpu.non_maskable_interrupt(), cpu.execute(32);
		}
		for (count = 0; count < 58; count++) { // 14318181 / 8 / 60 / 512
			if (command.size() && cpu2.interrupt())
				sound0->write(0x0e, command.front()), command.pop_front();
			sound0->write(0x0f, array<uint8_t, 10>{0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0}[timer]);
			cpu2.execute(73);
			++timer >= 10 && (timer = 0);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	TimePilot *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[4] |= 3;
				break;
			case 4:
				in[4] = in[4] & ~3 | 2;
				break;
			case 5:
				in[4] = in[4] & ~3 | 1;
				break;
			case 255:
				in[4] &= ~3;
				break;
			}
			switch (nBonus) {
			case BONUS_10000_50000:
				in[4] |= 8;
				break;
			case BONUS_20000_60000:
				in[4] &= ~8;
				break;
			}
			switch (nDifficulty) {
			case 1:
				in[4] |= 0x70;
				break;
			case 2:
				in[4] = in[4] & ~0x70 | 0x60;
				break;
			case 3:
				in[4] = in[4] & ~0x70 | 0x50;
				break;
			case 4:
				in[4] = in[4] & ~0x70 | 0x40;
				break;
			case 5:
				in[4] = in[4] & ~0x70 | 0x30;
				break;
			case 6:
				in[4] = in[4] & ~0x70 | 0x20;
				break;
			case 7:
				in[4] = in[4] & ~0x70 | 0x10;
				break;
			case 8:
				in[4] &= ~0x70;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			fInterruptEnable = false;
			command.clear();
			cpu2.reset();
			timer = 0;
		}
		return this;
	}

	TimePilot *updateInput() {
		in[0] = in[0] & ~0x19 | !fCoin << 0 | !fStart1P << 3 | !fStart2P << 4;
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
		in[1] = in[1] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void right(bool fDown) {
		in[1] = in[1] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void down(bool fDown) {
		in[1] = in[1] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void left(bool fDown) {
		in[1] = in[1] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void triggerA(bool fDown) {
		in[1] = in[1] & ~(1 << 4) | !fDown << 4;
	}

	void makeBitmap(int *data) {
		// bg描画
		int p = 256 * 8 * 2 + 232;
		for (int k = 0x40, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k, 0);

		// obj描画
		for (int k = 0x103e; k >= 0x1010; k -= 2) {
			const int dst0 = ram[k + 0x301] - 1 & 0xff | ram[k + 0x200] + 16 << 8;
			const int src0 = ram[k + 0x201] | ram[k + 0x300] << 8;
			switch (src0 >> 14) {
			case 0: // ノーマル
				xfer16x16(data, dst0, src0);
				break;
			case 1: // V反転
				xfer16x16V(data, dst0, src0);
				break;
			case 2: // H反転
				xfer16x16H(data, dst0, src0);
				break;
			case 3: // HV反転
				xfer16x16HV(data, dst0, src0);
				break;
			}
			const int dst1 = ram[k + 0x101] - 1 & 0xff | ram[k] + 16 << 8;
			const int src1 = ram[k + 1] | ram[k + 0x100] << 8;
			if (dst1 == dst0 && src1 == src0)
				continue;
			switch (src1 >> 14) {
			case 0: // ノーマル
				xfer16x16(data, dst1, src1);
				break;
			case 1: // V反転
				xfer16x16V(data, dst1, src1);
				break;
			case 2: // H反転
				xfer16x16H(data, dst1, src1);
				break;
			case 3: // HV反転
				xfer16x16HV(data, dst1, src1);
				break;
			}
		}

		// bg描画
		p = 256 * 8 * 2 + 232;
		for (int k = 0x40, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k, 1);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k, int pri) {
		const int q = (ram[0x400 + k] | ram[k] << 3 & 0x100) << 6, idx = ram[k] << 2 & 0x7c;

		if ((ram[k] >> 4 & 1) != pri)
			return;
		switch (ram[k] >> 6) {
		case 0: // ノーマル
			data[p + 0x000] = bgcolor[idx | bg[q | 0x00]];
			data[p + 0x001] = bgcolor[idx | bg[q | 0x01]];
			data[p + 0x002] = bgcolor[idx | bg[q | 0x02]];
			data[p + 0x003] = bgcolor[idx | bg[q | 0x03]];
			data[p + 0x004] = bgcolor[idx | bg[q | 0x04]];
			data[p + 0x005] = bgcolor[idx | bg[q | 0x05]];
			data[p + 0x006] = bgcolor[idx | bg[q | 0x06]];
			data[p + 0x007] = bgcolor[idx | bg[q | 0x07]];
			data[p + 0x100] = bgcolor[idx | bg[q | 0x08]];
			data[p + 0x101] = bgcolor[idx | bg[q | 0x09]];
			data[p + 0x102] = bgcolor[idx | bg[q | 0x0a]];
			data[p + 0x103] = bgcolor[idx | bg[q | 0x0b]];
			data[p + 0x104] = bgcolor[idx | bg[q | 0x0c]];
			data[p + 0x105] = bgcolor[idx | bg[q | 0x0d]];
			data[p + 0x106] = bgcolor[idx | bg[q | 0x0e]];
			data[p + 0x107] = bgcolor[idx | bg[q | 0x0f]];
			data[p + 0x200] = bgcolor[idx | bg[q | 0x10]];
			data[p + 0x201] = bgcolor[idx | bg[q | 0x11]];
			data[p + 0x202] = bgcolor[idx | bg[q | 0x12]];
			data[p + 0x203] = bgcolor[idx | bg[q | 0x13]];
			data[p + 0x204] = bgcolor[idx | bg[q | 0x14]];
			data[p + 0x205] = bgcolor[idx | bg[q | 0x15]];
			data[p + 0x206] = bgcolor[idx | bg[q | 0x16]];
			data[p + 0x207] = bgcolor[idx | bg[q | 0x17]];
			data[p + 0x300] = bgcolor[idx | bg[q | 0x18]];
			data[p + 0x301] = bgcolor[idx | bg[q | 0x19]];
			data[p + 0x302] = bgcolor[idx | bg[q | 0x1a]];
			data[p + 0x303] = bgcolor[idx | bg[q | 0x1b]];
			data[p + 0x304] = bgcolor[idx | bg[q | 0x1c]];
			data[p + 0x305] = bgcolor[idx | bg[q | 0x1d]];
			data[p + 0x306] = bgcolor[idx | bg[q | 0x1e]];
			data[p + 0x307] = bgcolor[idx | bg[q | 0x1f]];
			data[p + 0x400] = bgcolor[idx | bg[q | 0x20]];
			data[p + 0x401] = bgcolor[idx | bg[q | 0x21]];
			data[p + 0x402] = bgcolor[idx | bg[q | 0x22]];
			data[p + 0x403] = bgcolor[idx | bg[q | 0x23]];
			data[p + 0x404] = bgcolor[idx | bg[q | 0x24]];
			data[p + 0x405] = bgcolor[idx | bg[q | 0x25]];
			data[p + 0x406] = bgcolor[idx | bg[q | 0x26]];
			data[p + 0x407] = bgcolor[idx | bg[q | 0x27]];
			data[p + 0x500] = bgcolor[idx | bg[q | 0x28]];
			data[p + 0x501] = bgcolor[idx | bg[q | 0x29]];
			data[p + 0x502] = bgcolor[idx | bg[q | 0x2a]];
			data[p + 0x503] = bgcolor[idx | bg[q | 0x2b]];
			data[p + 0x504] = bgcolor[idx | bg[q | 0x2c]];
			data[p + 0x505] = bgcolor[idx | bg[q | 0x2d]];
			data[p + 0x506] = bgcolor[idx | bg[q | 0x2e]];
			data[p + 0x507] = bgcolor[idx | bg[q | 0x2f]];
			data[p + 0x600] = bgcolor[idx | bg[q | 0x30]];
			data[p + 0x601] = bgcolor[idx | bg[q | 0x31]];
			data[p + 0x602] = bgcolor[idx | bg[q | 0x32]];
			data[p + 0x603] = bgcolor[idx | bg[q | 0x33]];
			data[p + 0x604] = bgcolor[idx | bg[q | 0x34]];
			data[p + 0x605] = bgcolor[idx | bg[q | 0x35]];
			data[p + 0x606] = bgcolor[idx | bg[q | 0x36]];
			data[p + 0x607] = bgcolor[idx | bg[q | 0x37]];
			data[p + 0x700] = bgcolor[idx | bg[q | 0x38]];
			data[p + 0x701] = bgcolor[idx | bg[q | 0x39]];
			data[p + 0x702] = bgcolor[idx | bg[q | 0x3a]];
			data[p + 0x703] = bgcolor[idx | bg[q | 0x3b]];
			data[p + 0x704] = bgcolor[idx | bg[q | 0x3c]];
			data[p + 0x705] = bgcolor[idx | bg[q | 0x3d]];
			data[p + 0x706] = bgcolor[idx | bg[q | 0x3e]];
			data[p + 0x707] = bgcolor[idx | bg[q | 0x3f]];
			break;
		case 1: // V反転
			data[p + 0x000] = bgcolor[idx | bg[q | 0x38]];
			data[p + 0x001] = bgcolor[idx | bg[q | 0x39]];
			data[p + 0x002] = bgcolor[idx | bg[q | 0x3a]];
			data[p + 0x003] = bgcolor[idx | bg[q | 0x3b]];
			data[p + 0x004] = bgcolor[idx | bg[q | 0x3c]];
			data[p + 0x005] = bgcolor[idx | bg[q | 0x3d]];
			data[p + 0x006] = bgcolor[idx | bg[q | 0x3e]];
			data[p + 0x007] = bgcolor[idx | bg[q | 0x3f]];
			data[p + 0x100] = bgcolor[idx | bg[q | 0x30]];
			data[p + 0x101] = bgcolor[idx | bg[q | 0x31]];
			data[p + 0x102] = bgcolor[idx | bg[q | 0x32]];
			data[p + 0x103] = bgcolor[idx | bg[q | 0x33]];
			data[p + 0x104] = bgcolor[idx | bg[q | 0x34]];
			data[p + 0x105] = bgcolor[idx | bg[q | 0x35]];
			data[p + 0x106] = bgcolor[idx | bg[q | 0x36]];
			data[p + 0x107] = bgcolor[idx | bg[q | 0x37]];
			data[p + 0x200] = bgcolor[idx | bg[q | 0x28]];
			data[p + 0x201] = bgcolor[idx | bg[q | 0x29]];
			data[p + 0x202] = bgcolor[idx | bg[q | 0x2a]];
			data[p + 0x203] = bgcolor[idx | bg[q | 0x2b]];
			data[p + 0x204] = bgcolor[idx | bg[q | 0x2c]];
			data[p + 0x205] = bgcolor[idx | bg[q | 0x2d]];
			data[p + 0x206] = bgcolor[idx | bg[q | 0x2e]];
			data[p + 0x207] = bgcolor[idx | bg[q | 0x2f]];
			data[p + 0x300] = bgcolor[idx | bg[q | 0x20]];
			data[p + 0x301] = bgcolor[idx | bg[q | 0x21]];
			data[p + 0x302] = bgcolor[idx | bg[q | 0x22]];
			data[p + 0x303] = bgcolor[idx | bg[q | 0x23]];
			data[p + 0x304] = bgcolor[idx | bg[q | 0x24]];
			data[p + 0x305] = bgcolor[idx | bg[q | 0x25]];
			data[p + 0x306] = bgcolor[idx | bg[q | 0x26]];
			data[p + 0x307] = bgcolor[idx | bg[q | 0x27]];
			data[p + 0x400] = bgcolor[idx | bg[q | 0x18]];
			data[p + 0x401] = bgcolor[idx | bg[q | 0x19]];
			data[p + 0x402] = bgcolor[idx | bg[q | 0x1a]];
			data[p + 0x403] = bgcolor[idx | bg[q | 0x1b]];
			data[p + 0x404] = bgcolor[idx | bg[q | 0x1c]];
			data[p + 0x405] = bgcolor[idx | bg[q | 0x1d]];
			data[p + 0x406] = bgcolor[idx | bg[q | 0x1e]];
			data[p + 0x407] = bgcolor[idx | bg[q | 0x1f]];
			data[p + 0x500] = bgcolor[idx | bg[q | 0x10]];
			data[p + 0x501] = bgcolor[idx | bg[q | 0x11]];
			data[p + 0x502] = bgcolor[idx | bg[q | 0x12]];
			data[p + 0x503] = bgcolor[idx | bg[q | 0x13]];
			data[p + 0x504] = bgcolor[idx | bg[q | 0x14]];
			data[p + 0x505] = bgcolor[idx | bg[q | 0x15]];
			data[p + 0x506] = bgcolor[idx | bg[q | 0x16]];
			data[p + 0x507] = bgcolor[idx | bg[q | 0x17]];
			data[p + 0x600] = bgcolor[idx | bg[q | 0x08]];
			data[p + 0x601] = bgcolor[idx | bg[q | 0x09]];
			data[p + 0x602] = bgcolor[idx | bg[q | 0x0a]];
			data[p + 0x603] = bgcolor[idx | bg[q | 0x0b]];
			data[p + 0x604] = bgcolor[idx | bg[q | 0x0c]];
			data[p + 0x605] = bgcolor[idx | bg[q | 0x0d]];
			data[p + 0x606] = bgcolor[idx | bg[q | 0x0e]];
			data[p + 0x607] = bgcolor[idx | bg[q | 0x0f]];
			data[p + 0x700] = bgcolor[idx | bg[q | 0x00]];
			data[p + 0x701] = bgcolor[idx | bg[q | 0x01]];
			data[p + 0x702] = bgcolor[idx | bg[q | 0x02]];
			data[p + 0x703] = bgcolor[idx | bg[q | 0x03]];
			data[p + 0x704] = bgcolor[idx | bg[q | 0x04]];
			data[p + 0x705] = bgcolor[idx | bg[q | 0x05]];
			data[p + 0x706] = bgcolor[idx | bg[q | 0x06]];
			data[p + 0x707] = bgcolor[idx | bg[q | 0x07]];
			break;
		case 2: // H反転
			data[p + 0x000] = bgcolor[idx | bg[q | 0x07]];
			data[p + 0x001] = bgcolor[idx | bg[q | 0x06]];
			data[p + 0x002] = bgcolor[idx | bg[q | 0x05]];
			data[p + 0x003] = bgcolor[idx | bg[q | 0x04]];
			data[p + 0x004] = bgcolor[idx | bg[q | 0x03]];
			data[p + 0x005] = bgcolor[idx | bg[q | 0x02]];
			data[p + 0x006] = bgcolor[idx | bg[q | 0x01]];
			data[p + 0x007] = bgcolor[idx | bg[q | 0x00]];
			data[p + 0x100] = bgcolor[idx | bg[q | 0x0f]];
			data[p + 0x101] = bgcolor[idx | bg[q | 0x0e]];
			data[p + 0x102] = bgcolor[idx | bg[q | 0x0d]];
			data[p + 0x103] = bgcolor[idx | bg[q | 0x0c]];
			data[p + 0x104] = bgcolor[idx | bg[q | 0x0b]];
			data[p + 0x105] = bgcolor[idx | bg[q | 0x0a]];
			data[p + 0x106] = bgcolor[idx | bg[q | 0x09]];
			data[p + 0x107] = bgcolor[idx | bg[q | 0x08]];
			data[p + 0x200] = bgcolor[idx | bg[q | 0x17]];
			data[p + 0x201] = bgcolor[idx | bg[q | 0x16]];
			data[p + 0x202] = bgcolor[idx | bg[q | 0x15]];
			data[p + 0x203] = bgcolor[idx | bg[q | 0x14]];
			data[p + 0x204] = bgcolor[idx | bg[q | 0x13]];
			data[p + 0x205] = bgcolor[idx | bg[q | 0x12]];
			data[p + 0x206] = bgcolor[idx | bg[q | 0x11]];
			data[p + 0x207] = bgcolor[idx | bg[q | 0x10]];
			data[p + 0x300] = bgcolor[idx | bg[q | 0x1f]];
			data[p + 0x301] = bgcolor[idx | bg[q | 0x1e]];
			data[p + 0x302] = bgcolor[idx | bg[q | 0x1d]];
			data[p + 0x303] = bgcolor[idx | bg[q | 0x1c]];
			data[p + 0x304] = bgcolor[idx | bg[q | 0x1b]];
			data[p + 0x305] = bgcolor[idx | bg[q | 0x1a]];
			data[p + 0x306] = bgcolor[idx | bg[q | 0x19]];
			data[p + 0x307] = bgcolor[idx | bg[q | 0x18]];
			data[p + 0x400] = bgcolor[idx | bg[q | 0x27]];
			data[p + 0x401] = bgcolor[idx | bg[q | 0x26]];
			data[p + 0x402] = bgcolor[idx | bg[q | 0x25]];
			data[p + 0x403] = bgcolor[idx | bg[q | 0x24]];
			data[p + 0x404] = bgcolor[idx | bg[q | 0x23]];
			data[p + 0x405] = bgcolor[idx | bg[q | 0x22]];
			data[p + 0x406] = bgcolor[idx | bg[q | 0x21]];
			data[p + 0x407] = bgcolor[idx | bg[q | 0x20]];
			data[p + 0x500] = bgcolor[idx | bg[q | 0x2f]];
			data[p + 0x501] = bgcolor[idx | bg[q | 0x2e]];
			data[p + 0x502] = bgcolor[idx | bg[q | 0x2d]];
			data[p + 0x503] = bgcolor[idx | bg[q | 0x2c]];
			data[p + 0x504] = bgcolor[idx | bg[q | 0x2b]];
			data[p + 0x505] = bgcolor[idx | bg[q | 0x2a]];
			data[p + 0x506] = bgcolor[idx | bg[q | 0x29]];
			data[p + 0x507] = bgcolor[idx | bg[q | 0x28]];
			data[p + 0x600] = bgcolor[idx | bg[q | 0x37]];
			data[p + 0x601] = bgcolor[idx | bg[q | 0x36]];
			data[p + 0x602] = bgcolor[idx | bg[q | 0x35]];
			data[p + 0x603] = bgcolor[idx | bg[q | 0x34]];
			data[p + 0x604] = bgcolor[idx | bg[q | 0x33]];
			data[p + 0x605] = bgcolor[idx | bg[q | 0x32]];
			data[p + 0x606] = bgcolor[idx | bg[q | 0x31]];
			data[p + 0x607] = bgcolor[idx | bg[q | 0x30]];
			data[p + 0x700] = bgcolor[idx | bg[q | 0x3f]];
			data[p + 0x701] = bgcolor[idx | bg[q | 0x3e]];
			data[p + 0x702] = bgcolor[idx | bg[q | 0x3d]];
			data[p + 0x703] = bgcolor[idx | bg[q | 0x3c]];
			data[p + 0x704] = bgcolor[idx | bg[q | 0x3b]];
			data[p + 0x705] = bgcolor[idx | bg[q | 0x3a]];
			data[p + 0x706] = bgcolor[idx | bg[q | 0x39]];
			data[p + 0x707] = bgcolor[idx | bg[q | 0x38]];
			break;
		case 3: // HV反転
			data[p + 0x000] = bgcolor[idx | bg[q | 0x3f]];
			data[p + 0x001] = bgcolor[idx | bg[q | 0x3e]];
			data[p + 0x002] = bgcolor[idx | bg[q | 0x3d]];
			data[p + 0x003] = bgcolor[idx | bg[q | 0x3c]];
			data[p + 0x004] = bgcolor[idx | bg[q | 0x3b]];
			data[p + 0x005] = bgcolor[idx | bg[q | 0x3a]];
			data[p + 0x006] = bgcolor[idx | bg[q | 0x39]];
			data[p + 0x007] = bgcolor[idx | bg[q | 0x38]];
			data[p + 0x100] = bgcolor[idx | bg[q | 0x37]];
			data[p + 0x101] = bgcolor[idx | bg[q | 0x36]];
			data[p + 0x102] = bgcolor[idx | bg[q | 0x35]];
			data[p + 0x103] = bgcolor[idx | bg[q | 0x34]];
			data[p + 0x104] = bgcolor[idx | bg[q | 0x33]];
			data[p + 0x105] = bgcolor[idx | bg[q | 0x32]];
			data[p + 0x106] = bgcolor[idx | bg[q | 0x31]];
			data[p + 0x107] = bgcolor[idx | bg[q | 0x30]];
			data[p + 0x200] = bgcolor[idx | bg[q | 0x2f]];
			data[p + 0x201] = bgcolor[idx | bg[q | 0x2e]];
			data[p + 0x202] = bgcolor[idx | bg[q | 0x2d]];
			data[p + 0x203] = bgcolor[idx | bg[q | 0x2c]];
			data[p + 0x204] = bgcolor[idx | bg[q | 0x2b]];
			data[p + 0x205] = bgcolor[idx | bg[q | 0x2a]];
			data[p + 0x206] = bgcolor[idx | bg[q | 0x29]];
			data[p + 0x207] = bgcolor[idx | bg[q | 0x28]];
			data[p + 0x300] = bgcolor[idx | bg[q | 0x27]];
			data[p + 0x301] = bgcolor[idx | bg[q | 0x26]];
			data[p + 0x302] = bgcolor[idx | bg[q | 0x25]];
			data[p + 0x303] = bgcolor[idx | bg[q | 0x24]];
			data[p + 0x304] = bgcolor[idx | bg[q | 0x23]];
			data[p + 0x305] = bgcolor[idx | bg[q | 0x22]];
			data[p + 0x306] = bgcolor[idx | bg[q | 0x21]];
			data[p + 0x307] = bgcolor[idx | bg[q | 0x20]];
			data[p + 0x400] = bgcolor[idx | bg[q | 0x1f]];
			data[p + 0x401] = bgcolor[idx | bg[q | 0x1e]];
			data[p + 0x402] = bgcolor[idx | bg[q | 0x1d]];
			data[p + 0x403] = bgcolor[idx | bg[q | 0x1c]];
			data[p + 0x404] = bgcolor[idx | bg[q | 0x1b]];
			data[p + 0x405] = bgcolor[idx | bg[q | 0x1a]];
			data[p + 0x406] = bgcolor[idx | bg[q | 0x19]];
			data[p + 0x407] = bgcolor[idx | bg[q | 0x18]];
			data[p + 0x500] = bgcolor[idx | bg[q | 0x17]];
			data[p + 0x501] = bgcolor[idx | bg[q | 0x16]];
			data[p + 0x502] = bgcolor[idx | bg[q | 0x15]];
			data[p + 0x503] = bgcolor[idx | bg[q | 0x14]];
			data[p + 0x504] = bgcolor[idx | bg[q | 0x13]];
			data[p + 0x505] = bgcolor[idx | bg[q | 0x12]];
			data[p + 0x506] = bgcolor[idx | bg[q | 0x11]];
			data[p + 0x507] = bgcolor[idx | bg[q | 0x10]];
			data[p + 0x600] = bgcolor[idx | bg[q | 0x0f]];
			data[p + 0x601] = bgcolor[idx | bg[q | 0x0e]];
			data[p + 0x602] = bgcolor[idx | bg[q | 0x0d]];
			data[p + 0x603] = bgcolor[idx | bg[q | 0x0c]];
			data[p + 0x604] = bgcolor[idx | bg[q | 0x0b]];
			data[p + 0x605] = bgcolor[idx | bg[q | 0x0a]];
			data[p + 0x606] = bgcolor[idx | bg[q | 0x09]];
			data[p + 0x607] = bgcolor[idx | bg[q | 0x08]];
			data[p + 0x700] = bgcolor[idx | bg[q | 0x07]];
			data[p + 0x701] = bgcolor[idx | bg[q | 0x06]];
			data[p + 0x702] = bgcolor[idx | bg[q | 0x05]];
			data[p + 0x703] = bgcolor[idx | bg[q | 0x04]];
			data[p + 0x704] = bgcolor[idx | bg[q | 0x03]];
			data[p + 0x705] = bgcolor[idx | bg[q | 0x02]];
			data[p + 0x706] = bgcolor[idx | bg[q | 0x01]];
			data[p + 0x707] = bgcolor[idx | bg[q | 0x00]];
			break;
		}
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]))
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0xff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]))
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0xff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]))
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		src = (src << 8 & 0xff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]))
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(14318181 / 8, rate, 58, 0.2);
		sound1 = new AY_3_8910(14318181 / 8, rate, 58, 0.2);
		Z80::init();
	}
};

#endif //TIME_PILOT_H
