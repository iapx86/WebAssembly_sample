/*
 *
 *	Pac-Land
 *
 */

#ifndef PAC_LAND_H
#define PAC_LAND_H

#include <cstring>
#include "mc6809.h"
#include "mc6801.h"
#include "c30.h"

enum {
	BONUS_A, BONUS_B, BONUS_C, BONUS_D, BONUS_E, BONUS_F, BONUS_G, BONUS_H,
};

enum {
	RANK_A, RANK_B, RANK_C, RANK_D,
};

struct PacLand {
	static unsigned char PRG1[], PRG2[], PRG2I[], FG[], BG[], OBJ[], RED[], BLUE[], FGCOLOR[], BGCOLOR[], OBJCOLOR[];

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
	int nLife = 3;
	bool fAttract = true;
	int nBonus = BONUS_A;
	int nRank = RANK_A;

	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
	int bank = 0x80;

	uint8_t ram[0x3800] = {};
	uint8_t ram2[0x900] = {};
	uint8_t in[5] = {0xff, 0xff, 0xff, 0xff, 0xff};

	uint8_t fg[0x8000] = {};
	uint8_t bg[0x8000] = {};
	uint8_t obj[0x20000] = {};
	int rgb[0x400] = {};
	uint8_t opaque[3][0x100] = {};
	int dwScroll0 = 0;
	int dwScroll1 = 0;
	int palette = 0;
	bool fFlip = false;

	MC6809 cpu;
	MC6801 mcu;

	PacLand() {
		// CPU周りの初期化
		for (int i = 0; i < 0x38; i++) {
			cpu.memorymap[i].base = ram + i * 0x100;
			cpu.memorymap[i].write = nullptr;
		}
		cpu.memorymap[0x38].write = [&](int addr, int data) { dwScroll0 = data | addr << 8 & 0x100; };
		cpu.memorymap[0x3a].write = [&](int addr, int data) { dwScroll1 = data | addr << 8 & 0x100; };
		cpu.memorymap[0x3c].write = [&](int addr, int data) {
			const int bank = (data << 5 & 0xe0) + 0x80;
			if ((addr & 0xff) != 0)
				return;
			if (bank != this->bank) {
				for (int i = 0; i < 0x20; i++)
					cpu.memorymap[0x40 + i].base = PRG1 + (bank + i) * 0x100;
				this->bank = bank;
			}
			palette = data << 5 & 0x300;
		};
		for (int i = 0; i < 0x20; i++)
			cpu.memorymap[0x40 + i].base = PRG1 + 0x8000 + i * 0x100;
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x68 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x68 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x70 + i].write = [&](int addr, int data) { fInterruptEnable0 = (addr & 0x800) == 0; };
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[0x80 + i].base = PRG1 + i * 0x100;
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x80 + i].write = [&](int addr, int data) { (addr & 0x800) == 0 ? mcu.enable() : mcu.disable(); };
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x90 + i].write = [&](int addr, int data) { fFlip = (addr & 0x800) == 0; };

		mcu.memorymap[0].base = ram2;
		mcu.memorymap[0].read = [&](int addr) -> int { return addr == 2 ? in[4] : ram2[addr]; };
		mcu.memorymap[0].write = nullptr;
		for (int i = 0; i < 4; i++) {
			mcu.memorymap[0x10 + i].read = [&](int addr) { return sound0->read(addr); };
			mcu.memorymap[0x10 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x40; i++)
			mcu.memorymap[0x40 + i].write = [&](int addr, int data) { fInterruptEnable1 = (addr & 0x2000) == 0; };
		for (int i = 0; i < 0x20; i++)
			mcu.memorymap[0x80 + i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 8; i++) {
			mcu.memorymap[0xc0 + i].base = ram2 + 0x100 + i * 0x100;
			mcu.memorymap[0xc0 + i].write = nullptr;
		}
		mcu.memorymap[0xd0].read = [&](int addr) -> int { return (addr & 0xfc) == 0 ? in[addr & 3] : 0xff; };
		for (int i = 0; i < 0x10; i++)
			mcu.memorymap[0xf0 + i].base = PRG2I + i * 0x100;

		// Videoの初期化
		convertRGB();
		convertFG();
		convertBG();
		convertOBJ();
	}

	PacLand *execute() {
		if (fInterruptEnable0)
			cpu.interrupt();
		if (fInterruptEnable1)
			mcu.interrupt();
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		if ((ram2[8] & 8) != 0)
			mcu.interrupt(MC6801_OCF);
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		return this;
	}

	void reset() {
		fReset = true;
	}

	PacLand *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[0] |= 0x60;
				break;
			case 2:
				in[0] = in[0] & ~0x60 | 0x40;
				break;
			case 4:
				in[0] = in[0] & ~0x60 | 0x20;
				break;
			case 5:
				in[0] &= ~0x60;
				break;
			}
			if (fAttract)
				in[1] |= 0x40;
			else
				in[1] &= ~0x40;
			switch (nBonus) {
			case BONUS_A: // 30000 80000 150000 300000 500000 1000000
				in[0] |= 0x0e;
				break;
			case BONUS_B: // 30000 100000 200000 400000 600000 1000000
				in[0] = in[0] & ~0xe | 0xc;
				break;
			case BONUS_C: // 40000 100000 180000 300000 500000 1000000
				in[0] = in[0] & ~0xe | 0xa;
				break;
			case BONUS_D: // 30000 80000 every 100000
				in[0] = in[0] & ~0xe | 8;
				break;
			case BONUS_E: // 50000 150000 every 200000
				in[0] = in[0] & ~0xe | 6;
				break;
			case BONUS_F: // 30000 80000 150000
				in[0] = in[0] & ~0xe | 4;
				break;
			case BONUS_G: // 40000 100000 200000
				in[0] = in[0] & ~0xe | 2;
				break;
			case BONUS_H: // 40000
				in[0] &= ~0xe;
				break;
			}
			switch (nRank) {
			case RANK_A: // Normal
				in[0] |= 1, in[1] |= 8;
				break;
			case RANK_B: // Easy
				in[0] |= 1, in[1] &= ~8;
				break;
			case RANK_C: // Hard
				in[0] &= ~1, in[1] |= 8;
				break;
			case RANK_D: // Very Hard
				in[0] &= ~1, in[1] &= ~8;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[0] &= ~0x80;
		else
			in[0] |= 0x80;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			mcu.disable();
		}
		return this;
	}

	PacLand *updateInput() {
		// クレジット/スタートボタン処理
		if (fCoin)
			in[3] &= ~(1 << 5), --fCoin;
		else
			in[3] |= 1 << 5;
		if (fStart1P)
			in[2] &= ~(1 << 4), --fStart1P;
		else
			in[2] |= 1 << 4;
		if (fStart2P)
			in[2] &= ~(1 << 5), --fStart2P;
		else
			in[2] |= 1 << 5;
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
	}

	void right(bool fDown) {
		if (fDown)
			in[4] &= ~(1 << 5);
		else
			in[4] |= 1 << 5;
	}

	void down(bool fDown) {
	}

	void left(bool fDown) {
		if (fDown)
			in[4] &= ~(1 << 4);
		else
			in[4] |= 1 << 4;
	}

	void triggerA(bool fDown) {
		if (fDown)
			in[4] &= ~(1 << 3);
		else
			in[4] |= 1 << 3;
	}

	void triggerB(bool fDown) {
	}

	void convertRGB() {
		for (int i = 0; i < 0x400; i++)
			rgb[i] = (RED[i] & 0xf) * 255 / 15		// Red
				| (RED[i] >> 4) * 255 / 15 << 8		// Green
				| (BLUE[i] & 0xf) * 255 / 15 << 16	// Blue
				| 0xff000000;						// Alpha
	}

	void convertFG() {
		for (int p = 0, q = 0, i = 512; i != 0; q += 16, --i) {
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					fg[p++] = FG[q + k + 8] >> j & 1 | FG[q + k + 8] >> (j + 3) & 2;
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					fg[p++] = FG[q + k] >> j & 1 | FG[q + k] >> (j + 3) & 2;
		}
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
		memset(opaque[0], 1, 0x80);
		memset(opaque[1], 1, 0x7f);
		memset(opaque[1] + 0x80, 1, 0x7f);
		memset(opaque[2] + 0xf0, 1, 0x0f);

		for (int p = 0, q = 0, i = 512; i != 0; q += 64, --i) {
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 32] >> j & 1 | OBJ[q + k + 0x8000 + 32] >> (j + 3) & 2 | OBJ[q + k + 32] >> j << 2 & 4 | OBJ[q + k + 32] >> (j + 1) & 8;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000] >> j & 1 | OBJ[q + k + 0x8000] >> (j + 3) & 2 | OBJ[q + k] >> j << 2 & 4 | OBJ[q + k] >> (j + 1) & 8;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 40] >> j & 1 | OBJ[q + k + 0x8000 + 40] >> (j + 3) & 2 | OBJ[q + k + 40] >> j << 2 & 4 | OBJ[q + k + 40] >> (j + 1) & 8;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 8] >> j & 1 | OBJ[q + k + 0x8000 + 8] >> (j + 3) & 2 | OBJ[q + k + 8] >> j << 2 & 4 | OBJ[q + k + 8] >> (j + 1) & 8;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 48] >> j & 1 | OBJ[q + k + 0x8000 + 48] >> (j + 3) & 2 | OBJ[q + k + 48] >> j << 2 & 4 | OBJ[q + k + 48] >> (j + 1) & 8;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 16] >> j & 1 | OBJ[q + k + 0x8000 + 16] >> (j + 3) & 2 | OBJ[q + k + 16] >> j << 2 & 4 | OBJ[q + k + 16] >> (j + 1) & 8;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 56] >> j & 1 | OBJ[q + k + 0x8000 + 56] >> (j + 3) & 2 | OBJ[q + k + 56] >> j << 2 & 4 | OBJ[q + k + 56] >> (j + 1) & 8;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 0x8000 + 24] >> j & 1 | OBJ[q + k + 0x8000 + 24] >> (j + 3) & 2 | OBJ[q + k + 24] >> j << 2 & 4 | OBJ[q + k + 24] >> (j + 1) & 8;
			}
		}
	}

	void makeBitmap(int *data) {
		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = 0;

		// obj描画
		drawObj(data, 0);

		// bg描画
		p = 256 * 8 * 2 + 232 - (fFlip ? 4 + dwScroll1 & 7 : 5 + dwScroll1 & 7) * 256;
		int k = 0x1100 | (fFlip ? (4 + dwScroll1 >> 2) + 0x30 : (5 + dwScroll1 >> 2) + 4) & 0x7e;
		for (int i = 0; i < 28; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				xfer8x8(data, BGCOLOR, bg, ram[k + 1] << 1 & 0x7c | ram[k] << 1 & 0x180 | ram[k + 1] << 9 & 0x200, p, k);

		// fg描画
		p = 256 * 8 * 2 + 208 - (fFlip ? 1 + dwScroll0 & 7 : dwScroll0 & 7) * 256;
		k = 0x280 | (fFlip ? (1 + dwScroll0 >> 2) + 0x30 : (dwScroll0 >> 2) + 6) & 0x7e;
		for (int i = 0; i < 24; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				if ((ram[k + 1] & 0x20) == 0)
					xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 232;
		k = fFlip ? 0x132 : 0x106;
		for (int i = 0; i < 3; p -= 256 * 8 * 36 + 8, k += 56, i++)
			for (int j = 0; j < 36; p += 256 * 8, k += 2, j++)
				if ((ram[k + 1] & 0x20) == 0)
					xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 16;
		k = fFlip ? 0xeb2 : 0xe86;
		for (int i = 0; i < 36; p += 256 * 8, k += 2, i++)
			if ((ram[k + 1] & 0x20) == 0)
				xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);

		// obj描画
		drawObj(data, 1);

		// fg描画
		p = 256 * 8 * 2 + 208 - (fFlip ? 1 + dwScroll0 & 7 : dwScroll0 & 7) * 256;
		k = 0x280 | (fFlip ? (1 + dwScroll0 >> 2) + 0x30 : (dwScroll0 >> 2) + 6) & 0x7e;
		for (int i = 0; i < 24; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				if ((ram[k + 1] & 0x20) != 0)
					xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 232;
		k = fFlip ? 0x132 : 0x106;
		for (int i = 0; i < 3; p -= 256 * 8 * 36 + 8, k += 56, i++)
			for (int j = 0; j < 36; p += 256 * 8, k += 2, j++)
				if ((ram[k + 1] & 0x20) != 0)
					xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 16;
		k = fFlip ? 0xeb2 : 0xe86;
		for (int i = 0; i < 36; p += 256 * 8, k += 2, i++)
			if ((ram[k + 1] & 0x20) != 0)
				xfer8x8(data, FGCOLOR, fg, ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);

		// obj描画
		drawObj(data, 2);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[palette | data[p]];
	}

	void drawObj(int *data, int cat) {
		if (fFlip) {
			for (int k = 0x2780, i = 64; i != 0; k += 2, --i) {
				const int x = 0xe9 - ram[k + 0x800] & 0xff;
				const int y = 0x167 - ram[k + 0x801] - ram[k + 0x1001] * 0x100 & 0x1ff;
				const int src = ram[k + 1] << 9 | ram[k + 0x1000] << 1 & 0x100 | ram[k];
				switch (ram[k + 0x1000] & 0x0f) {
				case 0x00: // ノーマル
					xfer16x16(data, x | y << 8, src, cat);
					break;
				case 0x01: // V反転
					xfer16x16V(data, x | y << 8, src, cat);
					break;
				case 0x02: // H反転
					xfer16x16H(data, x | y << 8, src, cat);
					break;
				case 0x03: // HV反転
					xfer16x16HV(data, x | y << 8, src, cat);
					break;
				case 0x04: // ノーマル
					xfer16x16(data, x | y << 8, src | 1, cat);
					xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x05: // V反転
					xfer16x16V(data, x | y << 8, src & ~1, cat);
					xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x06: // H反転
					xfer16x16H(data, x | y << 8, src | 1, cat);
					xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x07: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~1, cat);
					xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x08: // ノーマル
					xfer16x16(data, x | y << 8, src & ~2, cat);
					xfer16x16(data, x - 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x09: // V反転
					xfer16x16V(data, x | y << 8, src & ~2, cat);
					xfer16x16V(data, x - 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0a: // H反転
					xfer16x16H(data, x | y << 8, src | 2, cat);
					xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0b: // HV反転
					xfer16x16HV(data, x | y << 8, src | 2, cat);
					xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0c: // ノーマル
					xfer16x16(data, x | y << 8, src & ~3 | 1, cat);
					xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~3, cat);
					xfer16x16(data, x - 16 & 0xff | y << 8, src | 3, cat);
					xfer16x16(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					break;
				case 0x0d: // V反転
					xfer16x16V(data, x | y << 8, src & ~3, cat);
					xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					xfer16x16V(data, x - 16 & 0xff | y << 8, src & ~3 | 2, cat);
					xfer16x16V(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src | 3, cat);
					break;
				case 0x0e: // H反転
					xfer16x16H(data, x | y << 8, src | 3, cat);
					xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~3 | 1, cat);
					xfer16x16H(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3, cat);
					break;
				case 0x0f: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~3 | 2, cat);
					xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 3, cat);
					xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~3, cat);
					xfer16x16HV(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					break;
				}
			}
		}
		else {
			for (int k = 0x2780, i = 64; i != 0; k += 2, --i) {
				const int x = ram[k + 0x800] + 7 & 0xff;
				const int y = (ram[k + 0x801] | ram[k + 0x1001] << 8) - 55 & 0x1ff;
				const int src = ram[k + 1] << 9 | ram[k + 0x1000] << 1 & 0x100 | ram[k];
				switch (ram[k + 0x1000] & 0x0f) {
				case 0x00: // ノーマル
					xfer16x16(data, x | y << 8, src, cat);
					break;
				case 0x01: // V反転
					xfer16x16V(data, x | y << 8, src, cat);
					break;
				case 0x02: // H反転
					xfer16x16H(data, x | y << 8, src, cat);
					break;
				case 0x03: // HV反転
					xfer16x16HV(data, x | y << 8, src, cat);
					break;
				case 0x04: // ノーマル
					xfer16x16(data, x | y << 8, src & ~1, cat);
					xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x05: // V反転
					xfer16x16V(data, x | y << 8, src | 1, cat);
					xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x06: // H反転
					xfer16x16H(data, x | y << 8, src & ~1, cat);
					xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x07: // HV反転
					xfer16x16HV(data, x | y << 8, src | 1, cat);
					xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x08: // ノーマル
					xfer16x16(data, x | y << 8, src | 2, cat);
					xfer16x16(data, x + 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x09: // V反転
					xfer16x16V(data, x | y << 8, src | 2, cat);
					xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0a: // H反転
					xfer16x16H(data, x | y << 8, src & ~2, cat);
					xfer16x16H(data, x + 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0b: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~2, cat);
					xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0c: // ノーマル
					xfer16x16(data, x | y << 8, src & ~3 | 2, cat);
					xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 3, cat);
					xfer16x16(data, x + 16 & 0xff | y << 8, src & ~3, cat);
					xfer16x16(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					break;
				case 0x0d: // V反転
					xfer16x16V(data, x | y << 8, src | 3, cat);
					xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~3 | 1, cat);
					xfer16x16V(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3, cat);
					break;
				case 0x0e: // H反転
					xfer16x16H(data, x | y << 8, src & ~3, cat);
					xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					xfer16x16H(data, x + 16 & 0xff | y << 8, src & ~3 | 2, cat);
					xfer16x16H(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src | 3, cat);
					break;
				case 0x0f: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~3 | 1, cat);
					xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~3, cat);
					xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 3, cat);
					xfer16x16HV(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					break;
				}
			}
		}
	}

	void xfer8x8(int *data, uint8_t *color, uint8_t *pattern, int idx, int p, int k) {
		const int q = (ram[k] | ram[k + 1] << 8) << 6 & 0x7fc0;
		int px;

		switch (ram[k + 1] >> 6) {
		case 0: // ノーマル
			if ((px = color[idx | pattern[q | 0x00]]) != 0xff) data[p + 0x000] = px;
			if ((px = color[idx | pattern[q | 0x01]]) != 0xff) data[p + 0x001] = px;
			if ((px = color[idx | pattern[q | 0x02]]) != 0xff) data[p + 0x002] = px;
			if ((px = color[idx | pattern[q | 0x03]]) != 0xff) data[p + 0x003] = px;
			if ((px = color[idx | pattern[q | 0x04]]) != 0xff) data[p + 0x004] = px;
			if ((px = color[idx | pattern[q | 0x05]]) != 0xff) data[p + 0x005] = px;
			if ((px = color[idx | pattern[q | 0x06]]) != 0xff) data[p + 0x006] = px;
			if ((px = color[idx | pattern[q | 0x07]]) != 0xff) data[p + 0x007] = px;
			if ((px = color[idx | pattern[q | 0x08]]) != 0xff) data[p + 0x100] = px;
			if ((px = color[idx | pattern[q | 0x09]]) != 0xff) data[p + 0x101] = px;
			if ((px = color[idx | pattern[q | 0x0a]]) != 0xff) data[p + 0x102] = px;
			if ((px = color[idx | pattern[q | 0x0b]]) != 0xff) data[p + 0x103] = px;
			if ((px = color[idx | pattern[q | 0x0c]]) != 0xff) data[p + 0x104] = px;
			if ((px = color[idx | pattern[q | 0x0d]]) != 0xff) data[p + 0x105] = px;
			if ((px = color[idx | pattern[q | 0x0e]]) != 0xff) data[p + 0x106] = px;
			if ((px = color[idx | pattern[q | 0x0f]]) != 0xff) data[p + 0x107] = px;
			if ((px = color[idx | pattern[q | 0x10]]) != 0xff) data[p + 0x200] = px;
			if ((px = color[idx | pattern[q | 0x11]]) != 0xff) data[p + 0x201] = px;
			if ((px = color[idx | pattern[q | 0x12]]) != 0xff) data[p + 0x202] = px;
			if ((px = color[idx | pattern[q | 0x13]]) != 0xff) data[p + 0x203] = px;
			if ((px = color[idx | pattern[q | 0x14]]) != 0xff) data[p + 0x204] = px;
			if ((px = color[idx | pattern[q | 0x15]]) != 0xff) data[p + 0x205] = px;
			if ((px = color[idx | pattern[q | 0x16]]) != 0xff) data[p + 0x206] = px;
			if ((px = color[idx | pattern[q | 0x17]]) != 0xff) data[p + 0x207] = px;
			if ((px = color[idx | pattern[q | 0x18]]) != 0xff) data[p + 0x300] = px;
			if ((px = color[idx | pattern[q | 0x19]]) != 0xff) data[p + 0x301] = px;
			if ((px = color[idx | pattern[q | 0x1a]]) != 0xff) data[p + 0x302] = px;
			if ((px = color[idx | pattern[q | 0x1b]]) != 0xff) data[p + 0x303] = px;
			if ((px = color[idx | pattern[q | 0x1c]]) != 0xff) data[p + 0x304] = px;
			if ((px = color[idx | pattern[q | 0x1d]]) != 0xff) data[p + 0x305] = px;
			if ((px = color[idx | pattern[q | 0x1e]]) != 0xff) data[p + 0x306] = px;
			if ((px = color[idx | pattern[q | 0x1f]]) != 0xff) data[p + 0x307] = px;
			if ((px = color[idx | pattern[q | 0x20]]) != 0xff) data[p + 0x400] = px;
			if ((px = color[idx | pattern[q | 0x21]]) != 0xff) data[p + 0x401] = px;
			if ((px = color[idx | pattern[q | 0x22]]) != 0xff) data[p + 0x402] = px;
			if ((px = color[idx | pattern[q | 0x23]]) != 0xff) data[p + 0x403] = px;
			if ((px = color[idx | pattern[q | 0x24]]) != 0xff) data[p + 0x404] = px;
			if ((px = color[idx | pattern[q | 0x25]]) != 0xff) data[p + 0x405] = px;
			if ((px = color[idx | pattern[q | 0x26]]) != 0xff) data[p + 0x406] = px;
			if ((px = color[idx | pattern[q | 0x27]]) != 0xff) data[p + 0x407] = px;
			if ((px = color[idx | pattern[q | 0x28]]) != 0xff) data[p + 0x500] = px;
			if ((px = color[idx | pattern[q | 0x29]]) != 0xff) data[p + 0x501] = px;
			if ((px = color[idx | pattern[q | 0x2a]]) != 0xff) data[p + 0x502] = px;
			if ((px = color[idx | pattern[q | 0x2b]]) != 0xff) data[p + 0x503] = px;
			if ((px = color[idx | pattern[q | 0x2c]]) != 0xff) data[p + 0x504] = px;
			if ((px = color[idx | pattern[q | 0x2d]]) != 0xff) data[p + 0x505] = px;
			if ((px = color[idx | pattern[q | 0x2e]]) != 0xff) data[p + 0x506] = px;
			if ((px = color[idx | pattern[q | 0x2f]]) != 0xff) data[p + 0x507] = px;
			if ((px = color[idx | pattern[q | 0x30]]) != 0xff) data[p + 0x600] = px;
			if ((px = color[idx | pattern[q | 0x31]]) != 0xff) data[p + 0x601] = px;
			if ((px = color[idx | pattern[q | 0x32]]) != 0xff) data[p + 0x602] = px;
			if ((px = color[idx | pattern[q | 0x33]]) != 0xff) data[p + 0x603] = px;
			if ((px = color[idx | pattern[q | 0x34]]) != 0xff) data[p + 0x604] = px;
			if ((px = color[idx | pattern[q | 0x35]]) != 0xff) data[p + 0x605] = px;
			if ((px = color[idx | pattern[q | 0x36]]) != 0xff) data[p + 0x606] = px;
			if ((px = color[idx | pattern[q | 0x37]]) != 0xff) data[p + 0x607] = px;
			if ((px = color[idx | pattern[q | 0x38]]) != 0xff) data[p + 0x700] = px;
			if ((px = color[idx | pattern[q | 0x39]]) != 0xff) data[p + 0x701] = px;
			if ((px = color[idx | pattern[q | 0x3a]]) != 0xff) data[p + 0x702] = px;
			if ((px = color[idx | pattern[q | 0x3b]]) != 0xff) data[p + 0x703] = px;
			if ((px = color[idx | pattern[q | 0x3c]]) != 0xff) data[p + 0x704] = px;
			if ((px = color[idx | pattern[q | 0x3d]]) != 0xff) data[p + 0x705] = px;
			if ((px = color[idx | pattern[q | 0x3e]]) != 0xff) data[p + 0x706] = px;
			if ((px = color[idx | pattern[q | 0x3f]]) != 0xff) data[p + 0x707] = px;
			break;
		case 1: // V反転
			if ((px = color[idx | pattern[q | 0x38]]) != 0xff) data[p + 0x000] = px;
			if ((px = color[idx | pattern[q | 0x39]]) != 0xff) data[p + 0x001] = px;
			if ((px = color[idx | pattern[q | 0x3a]]) != 0xff) data[p + 0x002] = px;
			if ((px = color[idx | pattern[q | 0x3b]]) != 0xff) data[p + 0x003] = px;
			if ((px = color[idx | pattern[q | 0x3c]]) != 0xff) data[p + 0x004] = px;
			if ((px = color[idx | pattern[q | 0x3d]]) != 0xff) data[p + 0x005] = px;
			if ((px = color[idx | pattern[q | 0x3e]]) != 0xff) data[p + 0x006] = px;
			if ((px = color[idx | pattern[q | 0x3f]]) != 0xff) data[p + 0x007] = px;
			if ((px = color[idx | pattern[q | 0x30]]) != 0xff) data[p + 0x100] = px;
			if ((px = color[idx | pattern[q | 0x31]]) != 0xff) data[p + 0x101] = px;
			if ((px = color[idx | pattern[q | 0x32]]) != 0xff) data[p + 0x102] = px;
			if ((px = color[idx | pattern[q | 0x33]]) != 0xff) data[p + 0x103] = px;
			if ((px = color[idx | pattern[q | 0x34]]) != 0xff) data[p + 0x104] = px;
			if ((px = color[idx | pattern[q | 0x35]]) != 0xff) data[p + 0x105] = px;
			if ((px = color[idx | pattern[q | 0x36]]) != 0xff) data[p + 0x106] = px;
			if ((px = color[idx | pattern[q | 0x37]]) != 0xff) data[p + 0x107] = px;
			if ((px = color[idx | pattern[q | 0x28]]) != 0xff) data[p + 0x200] = px;
			if ((px = color[idx | pattern[q | 0x29]]) != 0xff) data[p + 0x201] = px;
			if ((px = color[idx | pattern[q | 0x2a]]) != 0xff) data[p + 0x202] = px;
			if ((px = color[idx | pattern[q | 0x2b]]) != 0xff) data[p + 0x203] = px;
			if ((px = color[idx | pattern[q | 0x2c]]) != 0xff) data[p + 0x204] = px;
			if ((px = color[idx | pattern[q | 0x2d]]) != 0xff) data[p + 0x205] = px;
			if ((px = color[idx | pattern[q | 0x2e]]) != 0xff) data[p + 0x206] = px;
			if ((px = color[idx | pattern[q | 0x2f]]) != 0xff) data[p + 0x207] = px;
			if ((px = color[idx | pattern[q | 0x20]]) != 0xff) data[p + 0x300] = px;
			if ((px = color[idx | pattern[q | 0x21]]) != 0xff) data[p + 0x301] = px;
			if ((px = color[idx | pattern[q | 0x22]]) != 0xff) data[p + 0x302] = px;
			if ((px = color[idx | pattern[q | 0x23]]) != 0xff) data[p + 0x303] = px;
			if ((px = color[idx | pattern[q | 0x24]]) != 0xff) data[p + 0x304] = px;
			if ((px = color[idx | pattern[q | 0x25]]) != 0xff) data[p + 0x305] = px;
			if ((px = color[idx | pattern[q | 0x26]]) != 0xff) data[p + 0x306] = px;
			if ((px = color[idx | pattern[q | 0x27]]) != 0xff) data[p + 0x307] = px;
			if ((px = color[idx | pattern[q | 0x18]]) != 0xff) data[p + 0x400] = px;
			if ((px = color[idx | pattern[q | 0x19]]) != 0xff) data[p + 0x401] = px;
			if ((px = color[idx | pattern[q | 0x1a]]) != 0xff) data[p + 0x402] = px;
			if ((px = color[idx | pattern[q | 0x1b]]) != 0xff) data[p + 0x403] = px;
			if ((px = color[idx | pattern[q | 0x1c]]) != 0xff) data[p + 0x404] = px;
			if ((px = color[idx | pattern[q | 0x1d]]) != 0xff) data[p + 0x405] = px;
			if ((px = color[idx | pattern[q | 0x1e]]) != 0xff) data[p + 0x406] = px;
			if ((px = color[idx | pattern[q | 0x1f]]) != 0xff) data[p + 0x407] = px;
			if ((px = color[idx | pattern[q | 0x10]]) != 0xff) data[p + 0x500] = px;
			if ((px = color[idx | pattern[q | 0x11]]) != 0xff) data[p + 0x501] = px;
			if ((px = color[idx | pattern[q | 0x12]]) != 0xff) data[p + 0x502] = px;
			if ((px = color[idx | pattern[q | 0x13]]) != 0xff) data[p + 0x503] = px;
			if ((px = color[idx | pattern[q | 0x14]]) != 0xff) data[p + 0x504] = px;
			if ((px = color[idx | pattern[q | 0x15]]) != 0xff) data[p + 0x505] = px;
			if ((px = color[idx | pattern[q | 0x16]]) != 0xff) data[p + 0x506] = px;
			if ((px = color[idx | pattern[q | 0x17]]) != 0xff) data[p + 0x507] = px;
			if ((px = color[idx | pattern[q | 0x08]]) != 0xff) data[p + 0x600] = px;
			if ((px = color[idx | pattern[q | 0x09]]) != 0xff) data[p + 0x601] = px;
			if ((px = color[idx | pattern[q | 0x0a]]) != 0xff) data[p + 0x602] = px;
			if ((px = color[idx | pattern[q | 0x0b]]) != 0xff) data[p + 0x603] = px;
			if ((px = color[idx | pattern[q | 0x0c]]) != 0xff) data[p + 0x604] = px;
			if ((px = color[idx | pattern[q | 0x0d]]) != 0xff) data[p + 0x605] = px;
			if ((px = color[idx | pattern[q | 0x0e]]) != 0xff) data[p + 0x606] = px;
			if ((px = color[idx | pattern[q | 0x0f]]) != 0xff) data[p + 0x607] = px;
			if ((px = color[idx | pattern[q | 0x00]]) != 0xff) data[p + 0x700] = px;
			if ((px = color[idx | pattern[q | 0x01]]) != 0xff) data[p + 0x701] = px;
			if ((px = color[idx | pattern[q | 0x02]]) != 0xff) data[p + 0x702] = px;
			if ((px = color[idx | pattern[q | 0x03]]) != 0xff) data[p + 0x703] = px;
			if ((px = color[idx | pattern[q | 0x04]]) != 0xff) data[p + 0x704] = px;
			if ((px = color[idx | pattern[q | 0x05]]) != 0xff) data[p + 0x705] = px;
			if ((px = color[idx | pattern[q | 0x06]]) != 0xff) data[p + 0x706] = px;
			if ((px = color[idx | pattern[q | 0x07]]) != 0xff) data[p + 0x707] = px;
			break;
		case 2: // H反転
			if ((px = color[idx | pattern[q | 0x07]]) != 0xff) data[p + 0x000] = px;
			if ((px = color[idx | pattern[q | 0x06]]) != 0xff) data[p + 0x001] = px;
			if ((px = color[idx | pattern[q | 0x05]]) != 0xff) data[p + 0x002] = px;
			if ((px = color[idx | pattern[q | 0x04]]) != 0xff) data[p + 0x003] = px;
			if ((px = color[idx | pattern[q | 0x03]]) != 0xff) data[p + 0x004] = px;
			if ((px = color[idx | pattern[q | 0x02]]) != 0xff) data[p + 0x005] = px;
			if ((px = color[idx | pattern[q | 0x01]]) != 0xff) data[p + 0x006] = px;
			if ((px = color[idx | pattern[q | 0x00]]) != 0xff) data[p + 0x007] = px;
			if ((px = color[idx | pattern[q | 0x0f]]) != 0xff) data[p + 0x100] = px;
			if ((px = color[idx | pattern[q | 0x0e]]) != 0xff) data[p + 0x101] = px;
			if ((px = color[idx | pattern[q | 0x0d]]) != 0xff) data[p + 0x102] = px;
			if ((px = color[idx | pattern[q | 0x0c]]) != 0xff) data[p + 0x103] = px;
			if ((px = color[idx | pattern[q | 0x0b]]) != 0xff) data[p + 0x104] = px;
			if ((px = color[idx | pattern[q | 0x0a]]) != 0xff) data[p + 0x105] = px;
			if ((px = color[idx | pattern[q | 0x09]]) != 0xff) data[p + 0x106] = px;
			if ((px = color[idx | pattern[q | 0x08]]) != 0xff) data[p + 0x107] = px;
			if ((px = color[idx | pattern[q | 0x17]]) != 0xff) data[p + 0x200] = px;
			if ((px = color[idx | pattern[q | 0x16]]) != 0xff) data[p + 0x201] = px;
			if ((px = color[idx | pattern[q | 0x15]]) != 0xff) data[p + 0x202] = px;
			if ((px = color[idx | pattern[q | 0x14]]) != 0xff) data[p + 0x203] = px;
			if ((px = color[idx | pattern[q | 0x13]]) != 0xff) data[p + 0x204] = px;
			if ((px = color[idx | pattern[q | 0x12]]) != 0xff) data[p + 0x205] = px;
			if ((px = color[idx | pattern[q | 0x11]]) != 0xff) data[p + 0x206] = px;
			if ((px = color[idx | pattern[q | 0x10]]) != 0xff) data[p + 0x207] = px;
			if ((px = color[idx | pattern[q | 0x1f]]) != 0xff) data[p + 0x300] = px;
			if ((px = color[idx | pattern[q | 0x1e]]) != 0xff) data[p + 0x301] = px;
			if ((px = color[idx | pattern[q | 0x1d]]) != 0xff) data[p + 0x302] = px;
			if ((px = color[idx | pattern[q | 0x1c]]) != 0xff) data[p + 0x303] = px;
			if ((px = color[idx | pattern[q | 0x1b]]) != 0xff) data[p + 0x304] = px;
			if ((px = color[idx | pattern[q | 0x1a]]) != 0xff) data[p + 0x305] = px;
			if ((px = color[idx | pattern[q | 0x19]]) != 0xff) data[p + 0x306] = px;
			if ((px = color[idx | pattern[q | 0x18]]) != 0xff) data[p + 0x307] = px;
			if ((px = color[idx | pattern[q | 0x27]]) != 0xff) data[p + 0x400] = px;
			if ((px = color[idx | pattern[q | 0x26]]) != 0xff) data[p + 0x401] = px;
			if ((px = color[idx | pattern[q | 0x25]]) != 0xff) data[p + 0x402] = px;
			if ((px = color[idx | pattern[q | 0x24]]) != 0xff) data[p + 0x403] = px;
			if ((px = color[idx | pattern[q | 0x23]]) != 0xff) data[p + 0x404] = px;
			if ((px = color[idx | pattern[q | 0x22]]) != 0xff) data[p + 0x405] = px;
			if ((px = color[idx | pattern[q | 0x21]]) != 0xff) data[p + 0x406] = px;
			if ((px = color[idx | pattern[q | 0x20]]) != 0xff) data[p + 0x407] = px;
			if ((px = color[idx | pattern[q | 0x2f]]) != 0xff) data[p + 0x500] = px;
			if ((px = color[idx | pattern[q | 0x2e]]) != 0xff) data[p + 0x501] = px;
			if ((px = color[idx | pattern[q | 0x2d]]) != 0xff) data[p + 0x502] = px;
			if ((px = color[idx | pattern[q | 0x2c]]) != 0xff) data[p + 0x503] = px;
			if ((px = color[idx | pattern[q | 0x2b]]) != 0xff) data[p + 0x504] = px;
			if ((px = color[idx | pattern[q | 0x2a]]) != 0xff) data[p + 0x505] = px;
			if ((px = color[idx | pattern[q | 0x29]]) != 0xff) data[p + 0x506] = px;
			if ((px = color[idx | pattern[q | 0x28]]) != 0xff) data[p + 0x507] = px;
			if ((px = color[idx | pattern[q | 0x37]]) != 0xff) data[p + 0x600] = px;
			if ((px = color[idx | pattern[q | 0x36]]) != 0xff) data[p + 0x601] = px;
			if ((px = color[idx | pattern[q | 0x35]]) != 0xff) data[p + 0x602] = px;
			if ((px = color[idx | pattern[q | 0x34]]) != 0xff) data[p + 0x603] = px;
			if ((px = color[idx | pattern[q | 0x33]]) != 0xff) data[p + 0x604] = px;
			if ((px = color[idx | pattern[q | 0x32]]) != 0xff) data[p + 0x605] = px;
			if ((px = color[idx | pattern[q | 0x31]]) != 0xff) data[p + 0x606] = px;
			if ((px = color[idx | pattern[q | 0x30]]) != 0xff) data[p + 0x607] = px;
			if ((px = color[idx | pattern[q | 0x3f]]) != 0xff) data[p + 0x700] = px;
			if ((px = color[idx | pattern[q | 0x3e]]) != 0xff) data[p + 0x701] = px;
			if ((px = color[idx | pattern[q | 0x3d]]) != 0xff) data[p + 0x702] = px;
			if ((px = color[idx | pattern[q | 0x3c]]) != 0xff) data[p + 0x703] = px;
			if ((px = color[idx | pattern[q | 0x3b]]) != 0xff) data[p + 0x704] = px;
			if ((px = color[idx | pattern[q | 0x3a]]) != 0xff) data[p + 0x705] = px;
			if ((px = color[idx | pattern[q | 0x39]]) != 0xff) data[p + 0x706] = px;
			if ((px = color[idx | pattern[q | 0x38]]) != 0xff) data[p + 0x707] = px;
			break;
		case 3: // HV反転
			if ((px = color[idx | pattern[q | 0x3f]]) != 0xff) data[p + 0x000] = px;
			if ((px = color[idx | pattern[q | 0x3e]]) != 0xff) data[p + 0x001] = px;
			if ((px = color[idx | pattern[q | 0x3d]]) != 0xff) data[p + 0x002] = px;
			if ((px = color[idx | pattern[q | 0x3c]]) != 0xff) data[p + 0x003] = px;
			if ((px = color[idx | pattern[q | 0x3b]]) != 0xff) data[p + 0x004] = px;
			if ((px = color[idx | pattern[q | 0x3a]]) != 0xff) data[p + 0x005] = px;
			if ((px = color[idx | pattern[q | 0x39]]) != 0xff) data[p + 0x006] = px;
			if ((px = color[idx | pattern[q | 0x38]]) != 0xff) data[p + 0x007] = px;
			if ((px = color[idx | pattern[q | 0x37]]) != 0xff) data[p + 0x100] = px;
			if ((px = color[idx | pattern[q | 0x36]]) != 0xff) data[p + 0x101] = px;
			if ((px = color[idx | pattern[q | 0x35]]) != 0xff) data[p + 0x102] = px;
			if ((px = color[idx | pattern[q | 0x34]]) != 0xff) data[p + 0x103] = px;
			if ((px = color[idx | pattern[q | 0x33]]) != 0xff) data[p + 0x104] = px;
			if ((px = color[idx | pattern[q | 0x32]]) != 0xff) data[p + 0x105] = px;
			if ((px = color[idx | pattern[q | 0x31]]) != 0xff) data[p + 0x106] = px;
			if ((px = color[idx | pattern[q | 0x30]]) != 0xff) data[p + 0x107] = px;
			if ((px = color[idx | pattern[q | 0x2f]]) != 0xff) data[p + 0x200] = px;
			if ((px = color[idx | pattern[q | 0x2e]]) != 0xff) data[p + 0x201] = px;
			if ((px = color[idx | pattern[q | 0x2d]]) != 0xff) data[p + 0x202] = px;
			if ((px = color[idx | pattern[q | 0x2c]]) != 0xff) data[p + 0x203] = px;
			if ((px = color[idx | pattern[q | 0x2b]]) != 0xff) data[p + 0x204] = px;
			if ((px = color[idx | pattern[q | 0x2a]]) != 0xff) data[p + 0x205] = px;
			if ((px = color[idx | pattern[q | 0x29]]) != 0xff) data[p + 0x206] = px;
			if ((px = color[idx | pattern[q | 0x28]]) != 0xff) data[p + 0x207] = px;
			if ((px = color[idx | pattern[q | 0x27]]) != 0xff) data[p + 0x300] = px;
			if ((px = color[idx | pattern[q | 0x26]]) != 0xff) data[p + 0x301] = px;
			if ((px = color[idx | pattern[q | 0x25]]) != 0xff) data[p + 0x302] = px;
			if ((px = color[idx | pattern[q | 0x24]]) != 0xff) data[p + 0x303] = px;
			if ((px = color[idx | pattern[q | 0x23]]) != 0xff) data[p + 0x304] = px;
			if ((px = color[idx | pattern[q | 0x22]]) != 0xff) data[p + 0x305] = px;
			if ((px = color[idx | pattern[q | 0x21]]) != 0xff) data[p + 0x306] = px;
			if ((px = color[idx | pattern[q | 0x20]]) != 0xff) data[p + 0x307] = px;
			if ((px = color[idx | pattern[q | 0x1f]]) != 0xff) data[p + 0x400] = px;
			if ((px = color[idx | pattern[q | 0x1e]]) != 0xff) data[p + 0x401] = px;
			if ((px = color[idx | pattern[q | 0x1d]]) != 0xff) data[p + 0x402] = px;
			if ((px = color[idx | pattern[q | 0x1c]]) != 0xff) data[p + 0x403] = px;
			if ((px = color[idx | pattern[q | 0x1b]]) != 0xff) data[p + 0x404] = px;
			if ((px = color[idx | pattern[q | 0x1a]]) != 0xff) data[p + 0x405] = px;
			if ((px = color[idx | pattern[q | 0x19]]) != 0xff) data[p + 0x406] = px;
			if ((px = color[idx | pattern[q | 0x18]]) != 0xff) data[p + 0x407] = px;
			if ((px = color[idx | pattern[q | 0x17]]) != 0xff) data[p + 0x500] = px;
			if ((px = color[idx | pattern[q | 0x16]]) != 0xff) data[p + 0x501] = px;
			if ((px = color[idx | pattern[q | 0x15]]) != 0xff) data[p + 0x502] = px;
			if ((px = color[idx | pattern[q | 0x14]]) != 0xff) data[p + 0x503] = px;
			if ((px = color[idx | pattern[q | 0x13]]) != 0xff) data[p + 0x504] = px;
			if ((px = color[idx | pattern[q | 0x12]]) != 0xff) data[p + 0x505] = px;
			if ((px = color[idx | pattern[q | 0x11]]) != 0xff) data[p + 0x506] = px;
			if ((px = color[idx | pattern[q | 0x10]]) != 0xff) data[p + 0x507] = px;
			if ((px = color[idx | pattern[q | 0x0f]]) != 0xff) data[p + 0x600] = px;
			if ((px = color[idx | pattern[q | 0x0e]]) != 0xff) data[p + 0x601] = px;
			if ((px = color[idx | pattern[q | 0x0d]]) != 0xff) data[p + 0x602] = px;
			if ((px = color[idx | pattern[q | 0x0c]]) != 0xff) data[p + 0x603] = px;
			if ((px = color[idx | pattern[q | 0x0b]]) != 0xff) data[p + 0x604] = px;
			if ((px = color[idx | pattern[q | 0x0a]]) != 0xff) data[p + 0x605] = px;
			if ((px = color[idx | pattern[q | 0x09]]) != 0xff) data[p + 0x606] = px;
			if ((px = color[idx | pattern[q | 0x08]]) != 0xff) data[p + 0x607] = px;
			if ((px = color[idx | pattern[q | 0x07]]) != 0xff) data[p + 0x700] = px;
			if ((px = color[idx | pattern[q | 0x06]]) != 0xff) data[p + 0x701] = px;
			if ((px = color[idx | pattern[q | 0x05]]) != 0xff) data[p + 0x702] = px;
			if ((px = color[idx | pattern[q | 0x04]]) != 0xff) data[p + 0x703] = px;
			if ((px = color[idx | pattern[q | 0x03]]) != 0xff) data[p + 0x704] = px;
			if ((px = color[idx | pattern[q | 0x02]]) != 0xff) data[p + 0x705] = px;
			if ((px = color[idx | pattern[q | 0x01]]) != 0xff) data[p + 0x706] = px;
			if ((px = color[idx | pattern[q | 0x00]]) != 0xff) data[p + 0x707] = px;
			break;
		}
	}

	void xfer16x16(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0x1ff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[src++]]])
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[src++]]])
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[--src]]])
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[--src]]])
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new C30(49152000 / 2 / 1024, rate);
	}
};

#endif //PAC_LAND_H
