/*
 *
 *	Frogger
 *
 */

#ifndef FROGGER_H
#define FROGGER_H

#include <list>
#include "z80.h"
#include "ay-3-8910.h"
using namespace std;

struct Frogger {
	static unsigned char BG[], RGB[], PRG1[], PRG2[];

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static AY_3_8910 *sound0;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nLife = 3;

	// CPU周りの初期化
	bool fInterruptEnable = false;
	bool fSoundEnable = false;

	uint8_t ram[0xd00] = {};
	uint8_t ppi0[4] = {0xff, 0xfc, 0xf1, 0};
	uint8_t ppi1[4] = {};
	uint8_t ram2[0x400] = {};
	struct {
		int addr = 0;
	} psg;
	int count = 0;
	int timer = 0;
	list<int> command;

	uint8_t bg[0x4000] = {};
	uint8_t obj[0x4000] = {};
	int rgb[0x20] = {};

	Z80 cpu, cpu2;

	Frogger() {
		// CPU周りの初期化
		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x2f))
				cpu.memorymap[page].base = PRG1 + (page & 0x3f) * 0x100;
			else if (range(page, 0x80, 0x87)) {
				cpu.memorymap[page].base = ram + (page & 7) * 0x100;
				cpu.memorymap[page].write = nullptr;
			}
			else if (range(page, 0xa8, 0xab, 0x04)) {
				cpu.memorymap[page].base = ram + (8 | page & 3) * 0x100;
				cpu.memorymap[page].write = nullptr;
			}
			else if (range(page, 0xb0, 0xb0, 0x07)) {
				cpu.memorymap[page].base = ram + 0xc00;
				cpu.memorymap[page].write = nullptr;
			}
			else if (range(page, 0xb8, 0xb8, 0x07))
				cpu.memorymap[page].write = [&](int addr, int data) {
					if ((addr & 0x1c) == 8)
						fInterruptEnable = (data & 1) != 0;
				};
			else if (range(page, 0xc0, 0xff)) {
				cpu.memorymap[page].read = [&](int addr) {
					int data = 0xff;
					if ((addr & 0x1000) != 0)
						data &= ppi1[addr >> 1 & 3];
					if ((addr & 0x2000) != 0)
						data &= ppi0[addr >> 1 & 3];
					return data;
				};
				cpu.memorymap[page].write = [&](int addr, int data) {
					if ((addr & 0x1000) != 0)
						switch (addr >> 1 & 3) {
						case 0:
							return command.push_back(data);
						case 1:
							return void(fSoundEnable = (data & 0x10) == 0);
						}
				};
			}

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x1f))
				cpu2.memorymap[page].base = PRG2 + (page & 0x1f) * 0x100;
			else if (range(page, 0x40, 0x43, 0x1c)) {
				cpu2.memorymap[page].base = ram2 + (page & 3) * 0x100;
				cpu2.memorymap[page].write = nullptr;
			}
		for (int page = 0; page < 0x100; page++) {
			cpu2.iomap[page].read = [&](int addr) { return (addr & 0x40) != 0 ? sound0->read(psg.addr) : 0xff; };
			cpu2.iomap[page].write = [&](int addr, int data) {
				if ((addr & 0x40) != 0)
					sound0->write(psg.addr, data, count);
				else if ((addr & 0x80) != 0)
					psg.addr = data;
			};
		}

		decodeROM();

		// Videoの初期化
		convertRGB();
		convertBG();
		convertOBJ();
	}

	Frogger *execute() {
//		sound0->mute(!fSoundEnable);
		if (fInterruptEnable)
			cpu.non_maskable_interrupt();
		cpu.execute(0x2000);
		for (count = 0; count < 116; count++) { // 14318181 / 60 / 2048
			if (!command.empty() && cpu2.interrupt())
				sound0->write(0x0e, command.front()), command.pop_front();
			const int table[] = {0x26, 0x36, 0x26, 0x36, 0x2e, 0x3e, 0x2e, 0x3e, 0x66, 0x76, 0xa6, 0xb6, 0xa6, 0xb6, 0xae, 0xbe, 0xae, 0xbe, 0xe6, 0xf6};
			sound0->write(0x0f, table[timer]);
			cpu2.execute(36);
			if (++timer >= 20)
				timer = 0;
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	Frogger *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				ppi0[1] &= ~3;
				break;
			case 5:
				ppi0[1] = ppi0[1] & ~3 | 1;
				break;
			case 7:
				ppi0[1] = ppi0[1] & ~3 | 2;
				break;
			case 256:
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
			fSoundEnable = false;
			command.clear();
			cpu2.reset();
			timer = 0;
		}
		return this;
	}

	Frogger *updateInput() {
		// クレジット/スタートボタン処理
		if (fCoin)
			ppi0[0] &= ~(1 << 7), --fCoin;
		else
			ppi0[0] |= 1 << 7;
		if (fStart1P)
			ppi0[1] &= ~(1 << 7), --fStart1P;
		else
			ppi0[1] |= 1 << 7;
		if (fStart2P)
			ppi0[1] &= ~(1 << 6), --fStart2P;
		else
			ppi0[1] |= 1 << 6;
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
		if (fDown)
			ppi0[2] = ppi0[2] & ~(1 << 4) | 1 << 6;
		else
			ppi0[2] |= 1 << 4;
	}

	void right(bool fDown) {
		if (fDown)
			ppi0[0] = ppi0[0] & ~(1 << 4) | 1 << 5;
		else
			ppi0[0] |= 1 << 4;
	}

	void down(bool fDown) {
		if (fDown)
			ppi0[2] = ppi0[2] & ~(1 << 6) | 1 << 4;
		else
			ppi0[2] |= 1 << 6;
	}

	void left(bool fDown) {
		if (fDown)
			ppi0[0] = ppi0[0] & ~(1 << 5) | 1 << 4;
		else
			ppi0[0] |= 1 << 5;
	}

	void triggerA(bool fDown) {
		if (fDown)
			ppi0[0] &= ~(1 << 3);
		else
			ppi0[0] |= 1 << 3;
	}

	void triggerB(bool fDown) {
		if (fDown)
			ppi0[0] &= ~(1 << 1);
		else
			ppi0[0] |= 1 << 1;
	}

	void convertRGB() {
		for (int i = 0; i < 0x20; i++)
			rgb[i] = (RGB[i] & 7) * 255 / 7			// Red
				| (RGB[i] >> 3 & 7) * 255 / 7 << 8	// Green
				| (RGB[i] >> 6) * 255 / 3 << 16		// Blue
				| 0xff000000;						// Alpha
	}

	void convertBG() {
		for (int p = 0, q = 0, i = 256; i != 0; q += 8, --i)
			for (int j = 7; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg[p++] = BG[q + k + 0x800] >> j & 1 | BG[q + k] >> j << 1 & 2;
	}

	void convertOBJ() {
		for (int p = 0, q = 0, i = 64; i != 0; q += 32, --i) {
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = BG[q + k + 0x800 + 16] >> j & 1 | BG[q + k + 16] >> j << 1 & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = BG[q + k + 0x800] >> j & 1 | BG[q + k] >> j << 1 & 2;
			}
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = BG[q + k + 0x800 + 24] >> j & 1 | BG[q + k + 24] >> j << 1 & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = BG[q + k + 0x800 + 8] >> j & 1 | BG[q + k + 8] >> j << 1 & 2;
			}
		}
	}

	static void decodeROM() {
		static bool decoded;
		if (decoded)
			return;
		for (int i = 0; i < 0x800; i++)
			PRG2[i] = PRG2[i] & 0xfc | PRG2[i] << 1 & 2 | PRG2[i] >> 1 & 1;
		for (int i = 0x800; i < 0x1000; i++)
			BG[i] = BG[i] & 0xfc | BG[i] << 1 & 2 | BG[i] >> 1 & 1;
		decoded = true;
	}

	void makeBitmap(int *data) {
		// bg描画
		int p = 256 * 32;
		int k = 0xbe2;
		for (int i = 2; i < 32; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0xc00 + i * 2] >> 4 | ram[0xc00 + i * 2] << 4 & 0xf0;
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(data, p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// obj描画
		for (int k = 0xc5c, i = 7; i >= 0; k -= 4, --i) {
			const int x = ram[k] >> 4 | ram[k] << 4 & 0xf0, y = ram[k + 3] + 16;
			const int src = ram[k + 1] & 0x3f | ram[k + 2] << 5 & 0xc0 | ram[k + 2] << 8 & 0x100;
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

		// bg描画
		p = 256 * 16;
		k = 0xbe0;
		for (int i = 0; i < 2; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0xc00 + i * 2] >> 4 | ram[0xc00 + i * 2] << 4 & 0xf0;
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(data, p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 128; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = data[p] & 3 ? rgb[data[p]] : rgb[2];
		for (int i = 0; i < 128; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k, int i) {
		const int q = ram[k] << 6, idx = ram[0xc01 + i * 2] << 1 & 0xc | ram[0xc01 + i * 2] << 4 & 0x10;

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
				if ((px = obj[src++]) != 0)
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
				if ((px = obj[src++]) != 0)
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
				if ((px = obj[--src]) != 0)
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
				if ((px = obj[--src]) != 0)
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(14318181 / 8, rate, 116, 0.4);
		Z80::init();
	}
};

#endif //FROGGER_H