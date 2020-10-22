/*
 *
 *	Toypop
 *
 */

#ifndef TOYPOP_H
#define TOYPOP_H

#include <cstring>
#include <vector>
#include "mc6809.h"
#include "mc68000.h"
#include "mappy_sound.h"
using namespace std;

enum {
	BONUS_A, BONUS_B,
};

enum {
	RANK_EASY, RANK_NORMAL, RANK_HARD, RANK_VERY_HARD,
};

struct Toypop {
	static unsigned char PRG1[], PRG2[], PRG3[], BG[], OBJ[], RED[], GREEN[], BLUE[], BGCOLOR[], OBJCOLOR[], SND[];

	static const int cxScreen = 224;
	static const int cyScreen = 288;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static MappySound *sound0;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nPino = 3;
	int nBonus = BONUS_A;
	int nRank = RANK_NORMAL;
	bool fAttract = true;
	bool fRound = false;

	bool fInterruptEnable = false;
	bool fInterruptEnable2 = false;
	uint8_t ram[0x2000] = {};
	uint8_t ram2[0x800] = {};
	uint8_t ram3[0x40000] = {};
	uint8_t vram[0x10000] = {};
	uint8_t port[0x40] = {};
	uint8_t in[15] = {};

	uint8_t bg[0x8000] = {};
	uint8_t obj[0x10000] = {};
	int rgb[0x100] = {};
	int palette = 0;

	MC6809 cpu, cpu2;
	MC68000 cpu3;

	Toypop() {
		// CPU周りの初期化
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[i].base = ram + i * 0x100;
			cpu.memorymap[i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x28 + i].base = ram2 + i * 0x100;
			cpu.memorymap[0x28 + i].write = nullptr;
		}
		cpu.memorymap[0x60].read = [&](int addr) { return port[addr & 0x3f] | 0xf0; };
		cpu.memorymap[0x60].write = [&](int addr, int data) { port[addr & 0x3f] = data & 0xf; };
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x68 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x68 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		cpu.memorymap[0x70].read = [&](int addr) { return fInterruptEnable = true, 0; };
		cpu.memorymap[0x70].write = [&](int addr, int data) { fInterruptEnable = false; };
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[0x80 + i].base = PRG1 + i * 0x100;
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x80 + i].write = [&](int addr, int data) { (addr & 0x800) == 0 ? cpu3.enable() : cpu3.disable(); };
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x90 + i].write = [&](int addr, int data) { (addr & 0x800) == 0 ? cpu2.enable() : cpu2.disable(); };
		cpu.memorymap[0xa0].write = [&](int addr, int data) { palette = addr << 7 & 0x80; };

		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[i].read = [&](int addr) { return sound0->read(addr); };
			cpu2.memorymap[i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[0xe0 + i].base = PRG2 + i * 0x100;

		for (int i = 0; i < 0x80; i++)
			cpu3.memorymap[i].base = PRG3 + i * 0x100;
		for (int i = 0; i < 0x400; i++) {
			cpu3.memorymap[0x800 + i].base = ram3 + i * 0x100;
			cpu3.memorymap[0x800 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu3.memorymap[0x1000 + i].read = [&](int addr) -> int { return ram2[addr >> 1 & 0x7ff]; };
			cpu3.memorymap[0x1000 + i].write = [&](int addr, int data) { ram2[addr >> 1 & 0x7ff] = data; };
		}
		for (int i = 0; i < 0x80; i++) {
			cpu3.memorymap[0x1800 + i].read = [&](int addr) -> int { return addr = addr << 1 & 0xfffe, vram[addr] << 4 | vram[addr | 1] & 0xf; };
			cpu3.memorymap[0x1800 + i].write = [&](int addr, int data) { vram[addr = addr << 1 & 0xfffe] = data >> 4, vram[addr | 1] = data & 0xf; };
		}
		for (int i = 0; i < 0x500; i++) {
			cpu3.memorymap[0x1900 + i].base = vram + (i & 0xff) * 0x100;
			cpu3.memorymap[0x1900 + i].write = nullptr;
		}
		for (int i = 0; i < 0x1000; i++)
			cpu3.memorymap[0x3000 + i].write16 = [&](int addr, int data) { fInterruptEnable2 = (addr & 0x80000) == 0; };

		// Videoの初期化
		convertRGB();
		convertBG();
		convertOBJ();
	}

	Toypop *execute() {
		if (fInterruptEnable)
			cpu.interrupt();
		cpu2.interrupt();
		if (fInterruptEnable2)
			cpu3.interrupt(6);
		for (int i = 0; i < 0x100; i++) {
			Cpu *cpus[] = {&cpu, &cpu2};
			Cpu::multiple_execute(2, cpus, 32);
			cpu3.execute(48);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	Toypop *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nPino) {
			case 1:
				in[8] = in[8] & ~3 | 1;
				break;
			case 2:
				in[8] = in[8] & ~3 | 2;
				break;
			case 3:
				in[8] &= ~3;
				break;
			case 5:
				in[8] |= 3;
				break;
			}
			switch (nBonus) {
			case BONUS_A:
				in[7] &= ~8;
				break;
			case BONUS_B:
				in[7] |= 8;
				break;
			}
			switch (nRank) {
			case RANK_EASY:
				in[7] = in[7] & ~6 | 2;
				break;
			case RANK_NORMAL:
				in[7] &= ~6;
				break;
			case RANK_HARD:
				in[7] = in[7] & ~6 | 4;
				break;
			case RANK_VERY_HARD:
				in[7] |= 6;
				break;
			}
			if (fAttract)
				in[6] &= ~8;
			else
				in[6] |= 8;
			if (fRound)
				in[6] |= 2;
			else
				in[6] &= ~2;
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[5] |= 8;
		else
			in[5] &= ~8;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			cpu2.disable();
			cpu3.disable();
		}
		return this;
	}

	Toypop *updateInput() {
		in[0] = (fCoin != 0) << 3, in[3] = in[3] & 3 | (fStart1P != 0) << 2 | (fStart2P != 0) << 3;
		fCoin -= (fCoin > 0), fStart1P -= (fStart1P > 0), fStart2P -= (fStart2P > 0);
		if (port[8] == 1)
			memcpy(port + 4, in, 4);
		else if (port[8] == 5)
			port[2] = 0xf, port[6] = 0xc;
		if (port[0x18] == 1)
			memcpy(port + 0x10, in + 5, 4);
		else if (port[0x18] == 8)
			port[0x10] = 6, port[0x11] = 9;
		if (port[0x28] == 8)
			port[0x20] = 6, port[0x21] = 9;
		else if (port[0x28] == 9)
			memcpy(port + 0x20, vector<uint8_t>{in[10], in[14], in[11], in[11], in[12], in[12], in[13], in[13]}.data(), 8);
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
		in[1] = fDown ? in[1] & ~(1 << 2) | 1 << 0 : in[1] & ~(1 << 0);
	}

	void right(bool fDown) {
		in[1] = fDown ? in[1] & ~(1 << 3) | 1 << 1 : in[1] & ~(1 << 1);
	}

	void down(bool fDown) {
		in[1] = fDown ? in[1] & ~(1 << 0) | 1 << 2 : in[1] & ~(1 << 2);
	}

	void left(bool fDown) {
		in[1] = fDown ? in[1] & ~(1 << 1) | 1 << 3 : in[1] & ~(1 << 3);
	}

	void triggerA(bool fDown) {
		in[3] = fDown ? in[3] | 1 << 0: in[3] & ~(1 << 0);
	}

	void triggerB(bool fDown) {
	}

	void convertRGB() {
		for (int i = 0; i < 0x100; i++)
			rgb[i] = (RED[i] & 0xf) * 255 / 15		// Red
				| (GREEN[i] & 0xf) * 255 / 15 << 8	// Green
				| (BLUE[i] & 0xf) * 255 / 15 << 16	// Blue
				| 0xff000000;						// Alpha
	}

	void convertBG() {
		for (int p = 0, q = 0, i = 512; i != 0; q += 16, --i) {
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg[p++] = BG[q + k + 8] >> j & 1 | BG[q + k + 8] >> (j + 3) & 2;
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg[p++] = BG[q + k] >> j & 1 | BG[q + k] >> (j + 3) & 2;
		}
	}

	void convertOBJ() {
		for (int p = 0, q = 0, i = 256; i != 0; q += 64, --i) {
			for (int j = 3; j >= 0; --j) {
				for (int k = 39; k >= 32; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 47; k >= 40; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
				for (int k = 15; k >= 8; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 55; k >= 48; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
				for (int k = 23; k >= 16; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 63; k >= 56; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
				for (int k = 31; k >= 24; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
		}
	}

	void makeBitmap(int *data) {
		// graphic描画
		int p = 256 * 8 * 2 + 239;
		int k = 0x200;
		int idx = 0x60 | palette;
		for (int i = 0; i < 224; p -= 256 * 288 + 1, i++)
			for (int j = 0; j < 288; k++, p += 256, j++)
				data[p] = idx | vram[k];

		// bg描画
		p = 256 * 8 * 4 + 232;
		k = 0x40;
		for (int i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k);
		p = 256 * 8 * 36 + 232;
		k = 2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 37 + 232;
		k = 0x22;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 2 + 232;
		k = 0x3c2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 3 + 232;
		k = 0x3e2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);

		// obj描画
		for (int k = 0xf80, i = 64; i != 0; k += 2, --i) {
			const int x = 0xe9 - ram[k + 0x800] & 0xff;
			const int y = 0x167 - (ram[k + 0x801] | ram[k + 0x1001] << 8) & 0x1ff;
			const int src = ram[k] | ram[k + 1] << 8;
			switch (ram[k + 0x1000] & 0x0f) {
			case 0x00: // ノーマル
				xfer16x16(data, x | y << 8, src);
				break;
			case 0x01: // V反転
				xfer16x16V(data, x | y << 8, src);
				break;
			case 0x02: // H反転
				xfer16x16H(data, x | y << 8, src);
				break;
			case 0x03: // HV反転
				xfer16x16HV(data, x | y << 8, src);
				break;
			case 0x04: // ノーマル
				xfer16x16(data, x | y << 8, src | 1);
				xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x05: // V反転
				xfer16x16V(data, x | y << 8, src & ~1);
				xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x06: // H反転
				xfer16x16H(data, x | y << 8, src | 1);
				xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x07: // HV反転
				xfer16x16HV(data, x | y << 8, src & ~1);
				xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x08: // ノーマル
				xfer16x16(data, x | y << 8, src & ~2);
				xfer16x16(data, x - 16 & 0xff | y << 8, src | 2);
				break;
			case 0x09: // V反転
				xfer16x16V(data, x | y << 8, src & ~2);
				xfer16x16V(data, x - 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0a: // H反転
				xfer16x16H(data, x | y << 8, src | 2);
				xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0b: // HV反転
				xfer16x16HV(data, x | y << 8, src | 2);
				xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0c: // ノーマル
				xfer16x16(data, x | y << 8, src & ~3 | 1);
				xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~3);
				xfer16x16(data, x - 16 & 0xff | y << 8, src | 3);
				xfer16x16(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 2);
				break;
			case 0x0d: // V反転
				xfer16x16V(data, x | y << 8, src & ~3);
				xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 1);
				xfer16x16V(data, x - 16 & 0xff | y << 8, src & ~3 | 2);
				xfer16x16V(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src | 3);
				break;
			case 0x0e: // H反転
				xfer16x16H(data, x | y << 8, src | 3);
				xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 2);
				xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~3 | 1);
				xfer16x16H(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3);
				break;
			case 0x0f: // HV反転
				xfer16x16HV(data, x | y << 8, src & ~3 | 2);
				xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 3);
				xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~3);
				xfer16x16HV(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 1);
				break;
			}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0xfc, idx2 = 0x70 | palette;
		int px;

		if ((px = BGCOLOR[idx | bg[q | 0x00]]) != 0xf) data[p + 0x000] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x01]]) != 0xf) data[p + 0x001] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x02]]) != 0xf) data[p + 0x002] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x03]]) != 0xf) data[p + 0x003] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x04]]) != 0xf) data[p + 0x004] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x05]]) != 0xf) data[p + 0x005] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x06]]) != 0xf) data[p + 0x006] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x07]]) != 0xf) data[p + 0x007] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x08]]) != 0xf) data[p + 0x100] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x09]]) != 0xf) data[p + 0x101] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0a]]) != 0xf) data[p + 0x102] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0b]]) != 0xf) data[p + 0x103] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0c]]) != 0xf) data[p + 0x104] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0d]]) != 0xf) data[p + 0x105] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0e]]) != 0xf) data[p + 0x106] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x0f]]) != 0xf) data[p + 0x107] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x10]]) != 0xf) data[p + 0x200] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x11]]) != 0xf) data[p + 0x201] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x12]]) != 0xf) data[p + 0x202] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x13]]) != 0xf) data[p + 0x203] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x14]]) != 0xf) data[p + 0x204] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x15]]) != 0xf) data[p + 0x205] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x16]]) != 0xf) data[p + 0x206] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x17]]) != 0xf) data[p + 0x207] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x18]]) != 0xf) data[p + 0x300] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x19]]) != 0xf) data[p + 0x301] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1a]]) != 0xf) data[p + 0x302] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1b]]) != 0xf) data[p + 0x303] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1c]]) != 0xf) data[p + 0x304] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1d]]) != 0xf) data[p + 0x305] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1e]]) != 0xf) data[p + 0x306] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x1f]]) != 0xf) data[p + 0x307] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x20]]) != 0xf) data[p + 0x400] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x21]]) != 0xf) data[p + 0x401] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x22]]) != 0xf) data[p + 0x402] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x23]]) != 0xf) data[p + 0x403] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x24]]) != 0xf) data[p + 0x404] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x25]]) != 0xf) data[p + 0x405] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x26]]) != 0xf) data[p + 0x406] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x27]]) != 0xf) data[p + 0x407] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x28]]) != 0xf) data[p + 0x500] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x29]]) != 0xf) data[p + 0x501] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2a]]) != 0xf) data[p + 0x502] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2b]]) != 0xf) data[p + 0x503] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2c]]) != 0xf) data[p + 0x504] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2d]]) != 0xf) data[p + 0x505] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2e]]) != 0xf) data[p + 0x506] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x2f]]) != 0xf) data[p + 0x507] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x30]]) != 0xf) data[p + 0x600] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x31]]) != 0xf) data[p + 0x601] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x32]]) != 0xf) data[p + 0x602] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x33]]) != 0xf) data[p + 0x603] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x34]]) != 0xf) data[p + 0x604] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x35]]) != 0xf) data[p + 0x605] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x36]]) != 0xf) data[p + 0x606] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x37]]) != 0xf) data[p + 0x607] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x38]]) != 0xf) data[p + 0x700] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x39]]) != 0xf) data[p + 0x701] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3a]]) != 0xf) data[p + 0x702] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3b]]) != 0xf) data[p + 0x703] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3c]]) != 0xf) data[p + 0x704] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3d]]) != 0xf) data[p + 0x705] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3e]]) != 0xf) data[p + 0x706] = idx2 | px;
		if ((px = BGCOLOR[idx | bg[q | 0x3f]]) != 0xf) data[p + 0x707] = idx2 | px;
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new MappySound(SND, rate);
	}
};

#endif //TOYPOP_H
