/*
 *
 *	Super Pac-Man
 *
 */

#ifndef SUPER_PAC_MAN_H
#define SUPER_PAC_MAN_H

#include <cstring>
#include <vector>
#include "mc6809.h"
#include "mappy_sound.h"
using namespace std;

enum {
	RANK_0, RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7,
	RANK_8, RANK_9, RANK_A, RANK_B, RANK_C, RANK_D, RANK_E, RANK_F,
};

enum {
	BONUS_A, BONUS_B, BONUS_C, BONUS_D,
};

struct SuperPacMan {
	static unsigned char SND[], BG[], OBJ[], BGCOLOR[], OBJCOLOR[], RGB[], PRG1[], PRG2[];

	static const int cxScreen = 224;
	static const int cyScreen = 288;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static MappySound *sound0;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nPacman = 3;
	int nRank = RANK_0;
	int nBonus = BONUS_A;
	bool fAttract = true;

	bool fPortTest = false;
	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
//	bool fSoundEnable = false;
	uint8_t ram[0x2000] = {};
	uint8_t port[0x40] = {};
	uint8_t in[10] = {};
	int edge = 0xf;

	uint8_t bg[0x4000] = {};
	uint8_t obj[0x10000] = {};
	uint8_t bgcolor[0x100] = {};
	uint8_t objcolor[0x100] = {};
	int rgb[0x20] = {};

	MC6809 cpu, cpu2;

	SuperPacMan() {
		// CPU周りの初期化
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[i].base = ram + i * 0x100;
			cpu.memorymap[i].write = nullptr;
		}
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x40 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x40 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x48 + i].read = [&](int addr) { return port[addr & 0x3f] | 0xf0; };
			cpu.memorymap[0x48 + i].write = [&](int addr, int data) { port[addr & 0x3f] = data & 0xf; };
		}
		cpu.memorymap[0x50].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0x00: // INTERRUPT STOP
				return void(fInterruptEnable1 = false);
			case 0x01: // INTERRUPT START
				return void(fInterruptEnable1 = true);
			case 0x02: // INTERRUPT STOP
				return void(fInterruptEnable0 = false);
			case 0x03: // INTERRUPT START
				return void(fInterruptEnable0 = true);
//			case 0x06: // SND STOP
//				return void(fSoundEnable = false);
//			case 0x07: // SND START
//				return void(fSoundEnable = true);
			case 0x08: // PORT TEST START
				return void(fPortTest = true);
			case 0x09: // PORT TEST END
				return void(fPortTest = false);
			case 0x0a: // SUB CPU STOP
				return cpu2.disable();
			case 0x0b: // SUB CPU START
				return cpu2.enable();
			}
		};
		for (int i = 0; i < 0x40; i++)
			cpu.memorymap[0xc0 + i].base = PRG1 + i * 0x100;

		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[i].read = [&](int addr) { return sound0->read(addr); };
			cpu2.memorymap[i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		cpu2.memorymap[0x20].write = cpu.memorymap[0x50].write;
		for (int i = 0; i < 0x10; i++)
			cpu2.memorymap[0xf0 + i].base = PRG2 + i * 0x100;

		// Videoの初期化
		convertRGB();
		convertBG();
		convertOBJ();
	}

	SuperPacMan *execute() {
//		sound0->mute(!fSoundEnable);
		if (fInterruptEnable0)
			cpu.interrupt();
		if (fInterruptEnable1)
			cpu2.interrupt();
		Cpu *cpus[] = {&cpu, &cpu2};
		Cpu::multiple_execute(2, cpus, 0x2000);
		if (fInterruptEnable0)
			Cpu::multiple_execute(2, cpus, 0x2000);
		return this;
	}

	void reset() {
		fReset = true;
	}

	SuperPacMan *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nPacman) {
			case 1:
				in[9] = in[9] & ~0xc | 4;
				break;
			case 2:
				in[9] = in[9] & ~0xc | 8;
				break;
			case 3:
				in[9] &= ~0xc;
				break;
			case 5:
				in[9] |= 0xc;
				break;
			}
			switch (nRank) {
			case RANK_0:
				in[6] = 0;
				break;
			case RANK_1:
				in[6] = 1;
				break;
			case RANK_2:
				in[6] = 2;
				break;
			case RANK_3:
				in[6] = 3;
				break;
			case RANK_4:
				in[6] = 4;
				break;
			case RANK_5:
				in[6] = 5;
				break;
			case RANK_6:
				in[6] = 6;
				break;
			case RANK_7:
				in[6] = 7;
				break;
			case RANK_8:
				in[6] = 8;
				break;
			case RANK_9:
				in[6] = 9;
				break;
			case RANK_A:
				in[6] = 0xa;
				break;
			case RANK_B:
				in[6] = 0xb;
				break;
			case RANK_C:
				in[6] = 0xc;
				break;
			case RANK_D:
				in[6] = 0xd;
				break;
			case RANK_E:
				in[6] = 0xe;
				break;
			case RANK_F:
				in[6] = 0xf;
				break;
			}
			switch (nBonus) {
			case BONUS_A:
				in[9] &= ~3;
				break;
			case BONUS_B:
				in[9] = in[9] & ~3 | 1;
				break;
			case BONUS_C:
				in[9] = in[9] & ~3 | 2;
				break;
			case BONUS_D:
				in[9] |= 3;
				break;
			}
			if (fAttract)
				in[7] &= ~4;
			else
				in[7] |= 4;
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[8] |= 8;
		else
			in[8] &= ~8;

		// リセット処理
		if (fReset) {
			fReset = false;
//			fSoundEnable = false;
			cpu.reset();
			cpu2.disable();
		}
		return this;
	}

	SuperPacMan *updateInput() {
		in[0] = (fCoin != 0) << 3, in[3] = in[3] & 3 | (fStart1P != 0) << 2 | (fStart2P != 0) << 3;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		edge &= in[3];
		if (fPortTest)
			return edge = in[3] ^ 0xf, this;
		if (port[8] == 1)
			memcpy(port, in, 4);
		else if (port[8] == 4) {
			int credit = port[0] * 10 + port[1];
			if (fCoin && credit < 150)
				port[2] += 1, credit = min(credit + 1, 99);
			if (!port[9] && fStart1P && credit > 0)
				port[3] += 1, credit -= (credit < 150);
			if (!port[9] && fStart2P && credit > 1)
				port[3] += 2, credit -= (credit < 150) * 2;
			port[0] = credit / 10, port[1] = credit % 10;
			memcpy(port + 4, vector<uint8_t>{in[1], uint8_t(in[3] << 1 & 0xa | edge & 5), in[2], uint8_t(in[3] & 0xa | edge >> 1 & 5)}.data(), 4);
		} else if (port[8] == 8)
			port[0] = 6, port[1] = 9;
		if (port[0x18] == 8)
			port[0x10] = 6, port[0x11] = 9;
		else if (port[0x18] == 9)
			memcpy(port + 0x10, vector<uint8_t>{in[5], in[9], in[6], in[6], in[7], in[7], in[8], in[8]}.data(), 8);
		return edge = in[3] ^ 0xf, this;
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
		in[1] = in[1] & ~(1 << 0 | fDown << 2) | fDown << 0;
	}

	void right(bool fDown) {
		in[1] = in[1] & ~(1 << 1 | fDown << 3) | fDown << 1;
	}

	void down(bool fDown) {
		in[1] = in[1] & ~(1 << 2 | fDown << 0) | fDown << 2;
	}

	void left(bool fDown) {
		in[1] = in[1] & ~(1 << 3 | fDown << 1) | fDown << 3;
	}

	void triggerA(bool fDown) {
		in[3] = in[3] & ~(1 << 0) | fDown << 0;
	}

	void convertRGB() {
		for (int i = 0; i < 0x20; i++)
			rgb[i] = (RGB[i] & 7) * 255 / 7			// Red
				| (RGB[i] >> 3 & 7) * 255 / 7 << 8	// Green
				| (RGB[i] >> 6) * 255 / 3 << 16		// Blue
				| 0xff000000;						// Alpha
	}

	void convertBG() {
		for (int i = 0; i < 256; i++)
			bgcolor[i] = ~BGCOLOR[i] & 0xf | 0x10;
		for (int p = 0, q = 0, i = 256; i != 0; q += 16, --i) {
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg[p++] = BG[q + k + 8] >> j & 1 | BG[q + k + 8] >> (j + 3) & 2;
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg[p++] = BG[q + k] >> j & 1 | BG[q + k] >> (j + 3) & 2;
		}
	}

	void convertOBJ() {
		for (int i = 0; i < 256; i++)
			objcolor[i] = OBJCOLOR[i] & 0xf;
		memset(obj, 3, 0x10000);
		for (int p = 0, q = 0, i = 128; i != 0; q += 64, --i) {
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 32] >> j & 1 | OBJ[q + k + 32] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 40] >> j & 1 | OBJ[q + k + 40] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 8] >> j & 1 | OBJ[q + k + 8] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 48] >> j & 1 | OBJ[q + k + 48] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 16] >> j & 1 | OBJ[q + k + 16] >> (j + 3) & 2;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 56] >> j & 1 | OBJ[q + k + 56] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 24] >> j & 1 | OBJ[q + k + 24] >> (j + 3) & 2;
			}
		}
	}

	void makeBitmap(int *data) {
		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = 0x1f;

		// bg描画
		drawBG(data, 0);

		// obj描画
		for (int k = 0x0f80, i = 64; i != 0; k += 2, --i) {
			const int x = ram[k + 0x800] - 1 & 0xff;
			const int y = (ram[k + 0x801] | ram[k + 0x1001] << 8) - 24 & 0x1ff;
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
				xfer16x16(data, x | y << 8, src & ~1);
				xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x05: // V反転
				xfer16x16V(data, x | y << 8, src | 1);
				xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x06: // H反転
				xfer16x16H(data, x | y << 8, src & ~1);
				xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x07: // HV反転
				xfer16x16HV(data, x | y << 8, src | 1);
				xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x08: // ノーマル
				xfer16x16(data, x | y << 8, src | 2);
				xfer16x16(data, x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x09: // V反転
				xfer16x16V(data, x | y << 8, src | 2);
				xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0a: // H反転
				xfer16x16H(data, x | y << 8, src & ~2);
				xfer16x16H(data, x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0b: // HV反転
				xfer16x16HV(data, x | y << 8, src & ~2);
				xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0c: // ノーマル
				xfer16x16(data, x | y << 8, src & ~3 | 2);
				xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 3);
				xfer16x16(data, x + 16 & 0xff | y << 8, src & ~3);
				xfer16x16(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				break;
			case 0x0d: // V反転
				xfer16x16V(data, x | y << 8, src | 3);
				xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~3 | 1);
				xfer16x16V(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3);
				break;
			case 0x0e: // H反転
				xfer16x16H(data, x | y << 8, src & ~3);
				xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				xfer16x16H(data, x + 16 & 0xff | y << 8, src & ~3 | 2);
				xfer16x16H(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src | 3);
				break;
			case 0x0f: // HV反転
				xfer16x16HV(data, x | y << 8, src & ~3 | 1);
				xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~3);
				xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 3);
				xfer16x16HV(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				break;
			}
		}

		// bg描画
		drawBG(data, 1);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void drawBG(int *data, int pri) {
		int p = 256 * 8 * 4 + 232;
		int k = 0x40;
		for (int i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k, pri);
		p = 256 * 8 * 36 + 232;
		k = 2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 37 + 232;
		k = 0x22;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 2 + 232;
		k = 0x3c2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 3 + 232;
		k = 0x3e2;
		for (int i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
	}

	void xfer8x8(int *data, int p, int k, int pri) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0xfc;
		int px;

		if ((ram[k + 0x400] >> 6 & 1) != pri)
			return;
		(px = bgcolor[idx | bg[q | 0x00]]) != 0x1f && (data[p + 0x000] = px);
		(px = bgcolor[idx | bg[q | 0x01]]) != 0x1f && (data[p + 0x001] = px);
		(px = bgcolor[idx | bg[q | 0x02]]) != 0x1f && (data[p + 0x002] = px);
		(px = bgcolor[idx | bg[q | 0x03]]) != 0x1f && (data[p + 0x003] = px);
		(px = bgcolor[idx | bg[q | 0x04]]) != 0x1f && (data[p + 0x004] = px);
		(px = bgcolor[idx | bg[q | 0x05]]) != 0x1f && (data[p + 0x005] = px);
		(px = bgcolor[idx | bg[q | 0x06]]) != 0x1f && (data[p + 0x006] = px);
		(px = bgcolor[idx | bg[q | 0x07]]) != 0x1f && (data[p + 0x007] = px);
		(px = bgcolor[idx | bg[q | 0x08]]) != 0x1f && (data[p + 0x100] = px);
		(px = bgcolor[idx | bg[q | 0x09]]) != 0x1f && (data[p + 0x101] = px);
		(px = bgcolor[idx | bg[q | 0x0a]]) != 0x1f && (data[p + 0x102] = px);
		(px = bgcolor[idx | bg[q | 0x0b]]) != 0x1f && (data[p + 0x103] = px);
		(px = bgcolor[idx | bg[q | 0x0c]]) != 0x1f && (data[p + 0x104] = px);
		(px = bgcolor[idx | bg[q | 0x0d]]) != 0x1f && (data[p + 0x105] = px);
		(px = bgcolor[idx | bg[q | 0x0e]]) != 0x1f && (data[p + 0x106] = px);
		(px = bgcolor[idx | bg[q | 0x0f]]) != 0x1f && (data[p + 0x107] = px);
		(px = bgcolor[idx | bg[q | 0x10]]) != 0x1f && (data[p + 0x200] = px);
		(px = bgcolor[idx | bg[q | 0x11]]) != 0x1f && (data[p + 0x201] = px);
		(px = bgcolor[idx | bg[q | 0x12]]) != 0x1f && (data[p + 0x202] = px);
		(px = bgcolor[idx | bg[q | 0x13]]) != 0x1f && (data[p + 0x203] = px);
		(px = bgcolor[idx | bg[q | 0x14]]) != 0x1f && (data[p + 0x204] = px);
		(px = bgcolor[idx | bg[q | 0x15]]) != 0x1f && (data[p + 0x205] = px);
		(px = bgcolor[idx | bg[q | 0x16]]) != 0x1f && (data[p + 0x206] = px);
		(px = bgcolor[idx | bg[q | 0x17]]) != 0x1f && (data[p + 0x207] = px);
		(px = bgcolor[idx | bg[q | 0x18]]) != 0x1f && (data[p + 0x300] = px);
		(px = bgcolor[idx | bg[q | 0x19]]) != 0x1f && (data[p + 0x301] = px);
		(px = bgcolor[idx | bg[q | 0x1a]]) != 0x1f && (data[p + 0x302] = px);
		(px = bgcolor[idx | bg[q | 0x1b]]) != 0x1f && (data[p + 0x303] = px);
		(px = bgcolor[idx | bg[q | 0x1c]]) != 0x1f && (data[p + 0x304] = px);
		(px = bgcolor[idx | bg[q | 0x1d]]) != 0x1f && (data[p + 0x305] = px);
		(px = bgcolor[idx | bg[q | 0x1e]]) != 0x1f && (data[p + 0x306] = px);
		(px = bgcolor[idx | bg[q | 0x1f]]) != 0x1f && (data[p + 0x307] = px);
		(px = bgcolor[idx | bg[q | 0x20]]) != 0x1f && (data[p + 0x400] = px);
		(px = bgcolor[idx | bg[q | 0x21]]) != 0x1f && (data[p + 0x401] = px);
		(px = bgcolor[idx | bg[q | 0x22]]) != 0x1f && (data[p + 0x402] = px);
		(px = bgcolor[idx | bg[q | 0x23]]) != 0x1f && (data[p + 0x403] = px);
		(px = bgcolor[idx | bg[q | 0x24]]) != 0x1f && (data[p + 0x404] = px);
		(px = bgcolor[idx | bg[q | 0x25]]) != 0x1f && (data[p + 0x405] = px);
		(px = bgcolor[idx | bg[q | 0x26]]) != 0x1f && (data[p + 0x406] = px);
		(px = bgcolor[idx | bg[q | 0x27]]) != 0x1f && (data[p + 0x407] = px);
		(px = bgcolor[idx | bg[q | 0x28]]) != 0x1f && (data[p + 0x500] = px);
		(px = bgcolor[idx | bg[q | 0x29]]) != 0x1f && (data[p + 0x501] = px);
		(px = bgcolor[idx | bg[q | 0x2a]]) != 0x1f && (data[p + 0x502] = px);
		(px = bgcolor[idx | bg[q | 0x2b]]) != 0x1f && (data[p + 0x503] = px);
		(px = bgcolor[idx | bg[q | 0x2c]]) != 0x1f && (data[p + 0x504] = px);
		(px = bgcolor[idx | bg[q | 0x2d]]) != 0x1f && (data[p + 0x505] = px);
		(px = bgcolor[idx | bg[q | 0x2e]]) != 0x1f && (data[p + 0x506] = px);
		(px = bgcolor[idx | bg[q | 0x2f]]) != 0x1f && (data[p + 0x507] = px);
		(px = bgcolor[idx | bg[q | 0x30]]) != 0x1f && (data[p + 0x600] = px);
		(px = bgcolor[idx | bg[q | 0x31]]) != 0x1f && (data[p + 0x601] = px);
		(px = bgcolor[idx | bg[q | 0x32]]) != 0x1f && (data[p + 0x602] = px);
		(px = bgcolor[idx | bg[q | 0x33]]) != 0x1f && (data[p + 0x603] = px);
		(px = bgcolor[idx | bg[q | 0x34]]) != 0x1f && (data[p + 0x604] = px);
		(px = bgcolor[idx | bg[q | 0x35]]) != 0x1f && (data[p + 0x605] = px);
		(px = bgcolor[idx | bg[q | 0x36]]) != 0x1f && (data[p + 0x606] = px);
		(px = bgcolor[idx | bg[q | 0x37]]) != 0x1f && (data[p + 0x607] = px);
		(px = bgcolor[idx | bg[q | 0x38]]) != 0x1f && (data[p + 0x700] = px);
		(px = bgcolor[idx | bg[q | 0x39]]) != 0x1f && (data[p + 0x701] = px);
		(px = bgcolor[idx | bg[q | 0x3a]]) != 0x1f && (data[p + 0x702] = px);
		(px = bgcolor[idx | bg[q | 0x3b]]) != 0x1f && (data[p + 0x703] = px);
		(px = bgcolor[idx | bg[q | 0x3c]]) != 0x1f && (data[p + 0x704] = px);
		(px = bgcolor[idx | bg[q | 0x3d]]) != 0x1f && (data[p + 0x705] = px);
		(px = bgcolor[idx | bg[q | 0x3e]]) != 0x1f && (data[p + 0x706] = px);
		(px = bgcolor[idx | bg[q | 0x3f]]) != 0x1f && (data[p + 0x707] = px);
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[src++]]) != 0xf)
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[src++]]) != 0xf)
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[--src]]) != 0xf)
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[--src]]) != 0xf)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new MappySound(SND, rate);
	}
};

#endif //SUPER_PAC_MAN_H
