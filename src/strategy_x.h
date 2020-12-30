/*
 *
 *	Strategy X
 *
 */

#ifndef STRATEGY_X_H
#define STRATEGY_X_H

#include <array>
#include <list>
#include "z80.h"
#include "ay-3-8910.h"
#include "utils.h"
using namespace std;

struct StrategyX {
	static array<uint8_t, 0x1000> BG;
	static array<uint8_t, 0x20> RGB;
	static array<uint8_t, 0x6000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x20> MAP;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static AY_3_8910 *sound0, *sound1;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nLife = 3;

	bool fInterruptEnable = false;
//	bool fSoundEnable = false;

	array<uint8_t, 0xd00> ram = {};
	array<uint8_t, 4> ppi0 = {0xff, 0xfc, 0xf1, 0};
	array<uint8_t, 4> ppi1 = {0, 0, 0xfc, 0};
	array<uint8_t, 0x400> ram2 = {};
	struct {
		int addr = 0;
	} psg[2];
	int count = 0;
	int timer = 0;
	list<int> command;

	bool fBackgroundGreen = false;
	bool fBackgroundBlue = false;
	bool fBackgroundRed = false;
	array<uint8_t, 0x4000> bg;
	array<uint8_t, 0x4000> obj;
	array<int, 0x20> rgb;

	Z80 cpu, cpu2;

	StrategyX() {
		// CPU周りの初期化
		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x5f))
				cpu.memorymap[page].base = &PRG1[(page & 0x7f) << 8];
			else if (range(page, 0x80, 0x87)) {
				cpu.memorymap[page].base = &ram[(page & 7) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x88, 0x88)) {
				cpu.memorymap[page].base = &ram[0xc00];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x90, 0x93, 0x04)) {
				cpu.memorymap[page].base = &ram [(8 | page & 3) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0xa0, 0xa0))
				cpu.memorymap[page].read = [&](int addr) -> int { return ppi0[addr >> 2 & 3]; };
			else if (range(page, 0xa8, 0xa8)) {
				cpu.memorymap[page].read = [&](int addr) -> int { return ppi1[addr >> 2 & 3]; };
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr >> 2 & 3) {
					case 0:
						return command.push_back(data);
//					case 1:
//						return void(fSoundEnable = (data & 0x10) == 0);
					}
				};
			} else if (range(page, 0xb0, 0xb0))
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr & 0xff) {
					case 0:
						return void(fBackgroundGreen = (data & 1) != 0);
					case 2:
						return void(fBackgroundBlue = (data & 1) != 0);
					case 4:
						return void(fInterruptEnable = (data & 1) != 0);
					case 0xa:
						return void(fBackgroundRed = (data & 1) != 0);
					}
				};

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x1f))
				cpu2.memorymap[page].base = &PRG2[(page & 0x1f) << 8];
			else if (range(page, 0x80, 0x83, 0x0c)) {
				cpu2.memorymap[page].base = &ram2[(page & 3) << 8];
				cpu2.memorymap[page].write = nullptr;
			}
		for (int page = 0; page < 0x100; page++) {
			cpu2.iomap[page].read = [&](int addr) {
				int data = 0xff;
				if (addr & 0x20)
					data &= sound1->read(psg[1].addr);
				if (addr & 0x80)
					data &= sound0->read(psg[0].addr);
				return data;
			};
			cpu2.iomap[page].write = [&](int addr, int data) {
				if (addr & 0x10)
					psg[1].addr = data;
				else if (addr & 0x20)
					sound1->write(psg[1].addr, data, count);
				if (addr & 0x40)
					psg[0].addr = data;
				else if (addr & 0x80)
					sound0->write(psg[0].addr, data, count);
			};
		}

		// Videoの初期化
		bg.fill(3), obj.fill(3);
		convertGFX(&bg[0], &BG[0], 256, {rseq8(0, 8)}, {seq8(0, 1)}, {0, 0x4000}, 8);
		convertGFX(&obj[0], &BG[0], 64, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x4000}, 32);
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | (RGB[i] >> 6) * 255 / 3 << 16 | (RGB[i] >> 3 & 7) * 255 / 7 << 8 | (RGB[i] & 7) * 255 / 7;
	}

	StrategyX *execute() {
//		sound0->mute(!fSoundEnable);
//		sound1->mute(!fSoundEnable);
		fInterruptEnable && cpu.non_maskable_interrupt(), cpu.execute(0x2000);
		for (count = 0; count < 116; count++) { // 14318181 / 60 / 2048
			if (!command.empty() && cpu2.interrupt())
				sound0->write(0x0e, command.front()), command.pop_front();
			sound0->write(0x0f, array<uint8_t, 20>{0x0e, 0x1e, 0x0e, 0x1e, 0x2e, 0x3e, 0x2e, 0x3e, 0x4e, 0x5e, 0x8e, 0x9e, 0x8e, 0x9e, 0xae, 0xbe, 0xae, 0xbe, 0xce, 0xde}[timer]);
			cpu2.execute(36);
			++timer >= 20 && (timer = 0);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	StrategyX *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				ppi0[1] &= ~3;
				break;
			case 4:
				ppi0[1] = ppi0[1] & ~3 | 1;
				break;
			case 5:
				ppi0[1] = ppi0[1] & ~3 | 2;
				break;
			case 255:
				ppi0[1] |= 3;
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
//			fSoundEnable = false;
			command.clear();
			cpu2.reset();
			timer = 0;
		}
		return this;
	}

	StrategyX *updateInput() {
		ppi0[0] = ppi0[0] & ~(1 << 7) | !fCoin << 7;
		ppi0[1] = ppi0[1] & ~0xc0 | !fStart1P << 7 | !fStart2P << 6;
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
		ppi0[2] = ppi0[2] & ~(1 << 4) | fDown << 6 | !fDown << 4;
	}

	void right(bool fDown) {
		ppi0[0] = ppi0[0] & ~(1 << 4) | fDown << 5 | !fDown << 4;
	}

	void down(bool fDown) {
		ppi0[2] = ppi0[2] & ~(1 << 6) | fDown << 4 | !fDown << 6;
	}

	void left(bool fDown) {
		ppi0[0] = ppi0[0] & ~(1 << 5) | fDown << 4 | !fDown << 5;
	}

	void triggerA(bool fDown) {
		ppi0[0] = ppi0[0] & ~(1 << 3) | !fDown << 3;
	}

	void triggerB(bool fDown) {
		ppi0[2] = ppi0[2] & ~(1 << 5) | !fDown << 5;
	}

	void triggerX(bool fDown) {
		ppi0[0] = ppi0[0] & ~(1 << 1) | !fDown << 1;
	}

	void makeBitmap(int *data) {
		// bg描画
		int p = 256 * 32;
		for (int k = 0xbe2, i = 2; i < 32; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0xc00 + i * 2];
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(data, p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// obj描画
		for (int k = 0xc5c, i = 7; i >= 0; k -= 4, --i) {
			const int x = ram[k], y = ram[k + 3] + 16;
			const int src = ram[k + 1] & 0x3f | ram[k + 2] << 6;
			switch (ram[k + 1] & 0xc0) {
			case 0x00: // ノーマル
				xfer16x16(data, x | y << 8, src);
				break;
			case 0x40: // V反転
				xfer16x16V(data, x | y << 8, src);
				break;
			case 0x80: // H反転
				xfer16x16H(data, x | y << 8, src);
				break;
			case 0xc0: // HV反転
				xfer16x16HV(data, x | y << 8, src);
				break;
			}
		}

		// bullets描画
		for (int k = 0xc60, i = 0; i < 8; k += 4, i++) {
			p = ram[k + 1] | 264 - ram[k + 3] << 8;
			data[p] = 7;
		}

		// bg描画
		p = 256 * 16;
		for (int k = 0xbe0, i = 0; i < 2; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0xc00 + i * 2];
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(data, p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++) {
			const int color = (fBackgroundBlue && ~MAP[i >> 3] & 1 ? 0x00470000 : 0)
							| (fBackgroundGreen && ~MAP[i >> 3] & 2 ? 0x00003c00 : 0)
							| (fBackgroundRed && ~MAP[i >> 3] & 2 ? 0x0000007c : 0)
							| 0xff000000;
			for (int j = 0; j < 224; p++, j++)
				data[p] = data[p] & 3 ? rgb[data[p]] : color;
		}
	}

	void xfer8x8(int *data, int p, int k, int i) {
		const int q = ram[k] << 6, idx = ram[0xc01 + i * 2] << 2 & 0x1c;

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

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 4 & 0x1c;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		src = src << 8 & 0x3f00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 4 & 0x1c;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		src = (src << 8 & 0x3f00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[src++]))
					data[dst] = idx | px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 4 & 0x1c;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		src = (src << 8 & 0x3f00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 4 & 0x1c;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		src = (src << 8 & 0x3f00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(14318181 / 8, rate, 116, 0.2);
		sound1 = new AY_3_8910(14318181 / 8, rate, 116, 0.2);
		Z80::init();
	}
};

#endif //STRATEGY_X_H
