/*
 *
 *	Sky Kid
 *
 */

#ifndef SKY_KID_H
#define SKY_KID_H

#include <cstring>
#include "mc6809.h"
#include "mc6801.h"
#include "c30.h"

enum {
	BONUS_1ST_30000_2ND_90000, BONUS_1ST_30000_EVERY_90000, BONUS_1ST_20000_2ND_80000, BONUS_1ST_20000_EVERY_80000,
};

struct SkyKid {
	static unsigned char PRG1[], PRG2[], PRG2I[], FG[], BG[], OBJ[], RED[], GREEN[], BLUE[], BGCOLOR[], OBJCOLOR[];

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
	bool fAttract = true;
	bool fSkip = false;
	int nLife = 3;
	int nBonus = BONUS_1ST_30000_2ND_90000;
	bool fContinue = true;

	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
	int bank = 0x80;

	uint8_t ram[0x3000] = {};
	uint8_t ram2[0x900] = {};
	uint8_t in[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int select = 0;

	uint8_t fg[0x8000] = {};
	uint8_t bg[0x8000] = {};
	uint8_t obj[0x20000] = {};
	int rgb[0x100] = {};
	int priority = 0;
	int vScroll = 0;
	int hScroll = 0;
	bool fFlip = false;

	MC6809 cpu;
	MC6801 mcu;

	SkyKid() {
		// CPU周りの初期化
		for (int i = 0; i < 0x20; i++)
			cpu.memorymap[i].base = PRG1 + 0x8000 + i * 0x100;
		for (int i = 0; i < 0x10; i++) {
			cpu.memorymap[0x20 + i].base = ram + i * 0x100;
			cpu.memorymap[0x20 + i].write = nullptr;
		}
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[0x40 + i].base = ram + 0x1000 + i * 0x100;
			cpu.memorymap[0x40 + i].write = nullptr;
		}
		cpu.memorymap[0x60].write = [&](int addr, int data) { hScroll = addr & 0xff; };
		for (int i = 0; i < 2; i++)
			cpu.memorymap[0x62 + i].write = [&](int addr, int data) { vScroll = addr & 0x1ff; };
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x68 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x68 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x70 + i].write = [&](int addr, int data) { fInterruptEnable0 = !(addr & 0x800); };
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[0x80 + i].base = PRG1 + i * 0x100;
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x80 + i].write = [&](int addr, int data) { addr & 0x800 ? mcu.disable() : mcu.enable(); };
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x90 + i].write = [&](int addr, int data) {
				const int _bank = ~addr >> 6 & 0x20 | 0x80;
				if (_bank == bank)
					return;
				for (int i = 0; i < 0x20; i++)
					cpu.memorymap[i].base = PRG1 + (_bank + i) * 0x100;
				bank = _bank;
			};
		cpu.memorymap[0xa0].write = [&](int addr, int data) { !(addr & 0xfe) && (priority = data, fFlip = (addr & 1) != 0); };

		mcu.memorymap[0].read = [&](int addr) -> int { return addr == 2 ? in[select] : ram2[addr]; };
		mcu.memorymap[0].write = [&](int addr, int data) { addr == 2 && (data & 0xe0) == 0x60 && (select = data & 7), ram2[addr] = data; };
		for (int i = 0; i < 4; i++) {
			mcu.memorymap[0x10 + i].read = [&](int addr) { return sound0->read(addr); };
			mcu.memorymap[0x10 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x40; i++)
			mcu.memorymap[0x40 + i].write = [&](int addr, int data) { fInterruptEnable1 = !(addr & 0x2000); };
		for (int i = 0; i < 0x20; i++)
			mcu.memorymap[0x80 + i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 8; i++) {
			mcu.memorymap[0xc0 + i].base = ram2 + 0x100 + i * 0x100;
			mcu.memorymap[0xc0 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++)
			mcu.memorymap[0xf0 + i].base = PRG2I + i * 0x100;

		// Videoの初期化
		convertRGB();
		convertFG();
		convertBG();
		convertOBJ();
	}

	SkyKid *execute() {
		if (fInterruptEnable0)
			cpu.interrupt();
		if (fInterruptEnable1)
			mcu.interrupt();
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		if (ram2[8] & 8)
			mcu.interrupt(MC6801_OCF);
		for (int i = 0; i < 800; i++)
			cpu.execute(5), mcu.execute(6);
		return this;
	}

	void reset() {
		fReset = true;
	}

	SkyKid *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[0] |= 0x18;
				break;
			case 1:
				in[0] = in[0] & ~0x18 | 0x10;
				break;
			case 2:
				in[0] = in[0] & ~0x18 | 8;
				break;
			case 5:
				in[0] &= ~0x18;
				break;
			}
			if (fAttract)
				in[2] |= 8;
			else
				in[2] &= ~8;
			if (fSkip)
				in[2] &= ~4;
			else
				in[2] |= 4;
			switch (nBonus) {
			case BONUS_1ST_30000_2ND_90000:
				in[0] |= 6;
				break;
			case BONUS_1ST_30000_EVERY_90000:
				in[0] = in[0] & ~6 | 4;
				break;
			case BONUS_1ST_20000_2ND_80000:
				in[0] = in[0] & ~6 | 2;
				break;
			case BONUS_1ST_20000_EVERY_80000:
				in[0] &= ~6;
				break;
			}
			if (fContinue)
				in[1] |= 8;
			else
				in[1] &= ~8;
			in[1] &= ~4; // Flip
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[1] &= ~2;
		else
			in[1] |= 2;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			mcu.disable();
		}
		return this;
	}

	SkyKid *updateInput() {
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

	void triggerB(bool fDown) {
		in[3] = in[3] & ~(1 << 3) | !fDown << 3;
	}

	void convertRGB() {
		for (int i = 0; i < 0x100; i++)
			rgb[i] = (RED[i] & 0xf) * 255 / 15		// Red
				| (GREEN[i] & 0xf) * 255 / 15 << 8	// Green
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
				for (int k = 14; k >= 0; k -= 2)
					bg[p++] = BG[q + k] >> j & 1 | BG[q + k] >> (j + 3) & 2;
			for (int j = 3; j >= 0; --j)
				for (int k = 14; k >= 0; k -= 2)
					bg[p++] = BG[q + k + 1] >> j & 1 | BG[q + k + 1] >> (j + 3) & 2;
		}
	}

	void convertOBJ() {
		for (int p = 0, q = 0, i = 128; i != 0; q += 64, --i) {
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 32] >> j & 1 | OBJ[q + k + 32] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 32] << 2 >> j & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2 | OBJ[q + k + 0x4000] << 2 >> j & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 40] >> j & 1 | OBJ[q + k + 40] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 40] << 2 >> j & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 8] >> j & 1 | OBJ[q + k + 8] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 8] << 2 >> j & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 48] >> j & 1 | OBJ[q + k + 48] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 48] << 2 >> j & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 16] >> j & 1 | OBJ[q + k + 16] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 16] << 2 >> j & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 56] >> j & 1 | OBJ[q + k + 56] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 56] << 2 >> j & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 24] >> j & 1 | OBJ[q + k + 24] >> (j + 3) & 2 | OBJ[q + k + 0x4000 + 24] << 2 >> j & 4;
			}
		}
		for (int p = 0x8000, q = 0x2000, i = 128; i != 0; q += 64, --i) {
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 32] >> j & 1 | OBJ[q + k + 32] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 32] >> (j + 2) & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2 | OBJ[q + k + 0x2000] >> (j + 2) & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 40] >> j & 1 | OBJ[q + k + 40] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 40] >> (j + 2) & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 8] >> j & 1 | OBJ[q + k + 8] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 8] >> (j + 2) & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 48] >> j & 1 | OBJ[q + k + 48] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 48] >> (j + 2) & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 16] >> j & 1 | OBJ[q + k + 16] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 16] >> (j + 2) & 4;
			}
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 56] >> j & 1 | OBJ[q + k + 56] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 56] >> (j + 2) & 4;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 24] >> j & 1 | OBJ[q + k + 24] >> (j + 3) & 2 | OBJ[q + k + 0x2000 + 24] >> (j + 2) & 4;
			}
		}
		for (int p = 0x10000, q = 0x6000, i = 128; i != 0; q += 64, --i) {
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
		memset(obj + 0x18000, 3, 0x8000);
	}

	void makeBitmap(int *data) {
		// bg描画
		int p = 256 * 8 * 2 + 232 + (fFlip ? (7 - hScroll & 7) - (4 - vScroll & 7) * 256 : (1 + hScroll & 7) - (3 + vScroll & 7) * 256);
		int _k = fFlip ? 7 - hScroll << 3 & 0x7c0 | (4 - vScroll >> 3) + 23 & 0x3f : 0x18 + 1 + hScroll << 3 & 0x7c0 | (3 + vScroll >> 3) + 4 & 0x3f;
		for (int k = _k, i = 0; i < 29; k = k + 27 & 0x3f | k + 0x40 & 0x7c0, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 1 & 0x3f | k & 0x7c0, p += 256 * 8, j++)
				xfer8x8b(data, p, k);

		// fg描画
		if (priority & 4) {
			p = 256 * 8 * 4 + 232;
			for (int k = 0x1040, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
				for (int j = 0; j < 32; k++, p += 256 * 8, j++)
					if (!((ram[k] ^ priority) & 0xf0))
						xfer8x8f(data, p, k);
			p = 256 * 8 * 36 + 232;
			for (int k = 0x1002, i = 0; i < 28; p -= 8, k++, i++)
				if (!((ram[k] ^ priority) & 0xf0))
					xfer8x8f(data, p, k);
			p = 256 * 8 * 37 + 232;
			for (int k = 0x1022, i = 0; i < 28; p -= 8, k++, i++)
				if (!((ram[k] ^ priority) & 0xf0))
					xfer8x8f(data, p, k);
			p = 256 * 8 * 2 + 232;
			for (int k = 0x13c2, i = 0; i < 28; p -= 8, k++, i++)
				if (!((ram[k] ^ priority) & 0xf0))
					xfer8x8f(data, p, k);
			p = 256 * 8 * 3 + 232;
			for (int k = 0x13e2, i = 0; i < 28; p -= 8, k++, i++)
				if (!((ram[k] ^ priority) & 0xf0))
					xfer8x8f(data, p, k);
		}

		// obj描画
		if (fFlip) {
			for (int k = 0x1f80, i = 64; i != 0; k += 2, --i) {
				const int x = 0xe9 - ram[k + 0x800] & 0xff;
				const int y = 0x167 - ram[k + 0x801] - ram[k + 0x1001] * 0x100 & 0x1ff;
				const int src = ram[k] | ram[k + 0x1000] << 1 & 0x100 | ram[k + 1] << 9;
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
		} else {
			for (int k = 0x1f80, i = 64; i != 0; k += 2, --i) {
				const int x = ram[k + 0x800] + 7 & 0xff;
				const int y = (ram[k + 0x801] | ram[k + 0x1001] << 8) - 55 & 0x1ff;
				const int src = ram[k] | ram[k + 0x1000] << 1 & 0x100 | ram[k + 1] << 9;
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
		}

		// fg描画
		p = 256 * 8 * 4 + 232;
		for (int k = 0x1040, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				if (~priority & 4 || (ram[k] ^ priority) & 0xf0)
					xfer8x8f(data, p, k);
		p = 256 * 8 * 36 + 232;
		for (int k = 0x1002, i = 0; i < 28; p -= 8, k++, i++)
			if (~priority & 4 || (ram[k] ^ priority) & 0xf0)
				xfer8x8f(data, p, k);
		p = 256 * 8 * 37 + 232;
		for (int k = 0x1022, i = 0; i < 28; p -= 8, k++, i++)
			if (~priority & 4 || (ram[k] ^ priority) & 0xf0)
				xfer8x8f(data, p, k);
		p = 256 * 8 * 2 + 232;
		for (int k = 0x13c2, i = 0; i < 28; p -= 8, k++, i++)
			if (~priority & 4 || (ram[k] ^ priority) & 0xf0)
				xfer8x8f(data, p, k);
		p = 256 * 8 * 3 + 232;
		for (int k = 0x13e2, i = 0; i < 28; p -= 8, k++, i++)
			if (~priority & 4 || (ram[k] ^ priority) & 0xf0)
				xfer8x8f(data, p, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8f(int *data, int p, int k) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0xfc;
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

	void xfer8x8b(int *data, int p, int k) {
		const int q = (ram[k] | ram[k + 0x800] << 8) << 6 & 0x7fc0;
		const int idx = ram[k + 0x800] << 1 & 0xfc | ram[k + 0x800] << 8 & 0x100;

		data[p + 0x000] = BGCOLOR[idx | bg[q | 0x00]];
		data[p + 0x001] = BGCOLOR[idx | bg[q | 0x01]];
		data[p + 0x002] = BGCOLOR[idx | bg[q | 0x02]];
		data[p + 0x003] = BGCOLOR[idx | bg[q | 0x03]];
		data[p + 0x004] = BGCOLOR[idx | bg[q | 0x04]];
		data[p + 0x005] = BGCOLOR[idx | bg[q | 0x05]];
		data[p + 0x006] = BGCOLOR[idx | bg[q | 0x06]];
		data[p + 0x007] = BGCOLOR[idx | bg[q | 0x07]];
		data[p + 0x100] = BGCOLOR[idx | bg[q | 0x08]];
		data[p + 0x101] = BGCOLOR[idx | bg[q | 0x09]];
		data[p + 0x102] = BGCOLOR[idx | bg[q | 0x0a]];
		data[p + 0x103] = BGCOLOR[idx | bg[q | 0x0b]];
		data[p + 0x104] = BGCOLOR[idx | bg[q | 0x0c]];
		data[p + 0x105] = BGCOLOR[idx | bg[q | 0x0d]];
		data[p + 0x106] = BGCOLOR[idx | bg[q | 0x0e]];
		data[p + 0x107] = BGCOLOR[idx | bg[q | 0x0f]];
		data[p + 0x200] = BGCOLOR[idx | bg[q | 0x10]];
		data[p + 0x201] = BGCOLOR[idx | bg[q | 0x11]];
		data[p + 0x202] = BGCOLOR[idx | bg[q | 0x12]];
		data[p + 0x203] = BGCOLOR[idx | bg[q | 0x13]];
		data[p + 0x204] = BGCOLOR[idx | bg[q | 0x14]];
		data[p + 0x205] = BGCOLOR[idx | bg[q | 0x15]];
		data[p + 0x206] = BGCOLOR[idx | bg[q | 0x16]];
		data[p + 0x207] = BGCOLOR[idx | bg[q | 0x17]];
		data[p + 0x300] = BGCOLOR[idx | bg[q | 0x18]];
		data[p + 0x301] = BGCOLOR[idx | bg[q | 0x19]];
		data[p + 0x302] = BGCOLOR[idx | bg[q | 0x1a]];
		data[p + 0x303] = BGCOLOR[idx | bg[q | 0x1b]];
		data[p + 0x304] = BGCOLOR[idx | bg[q | 0x1c]];
		data[p + 0x305] = BGCOLOR[idx | bg[q | 0x1d]];
		data[p + 0x306] = BGCOLOR[idx | bg[q | 0x1e]];
		data[p + 0x307] = BGCOLOR[idx | bg[q | 0x1f]];
		data[p + 0x400] = BGCOLOR[idx | bg[q | 0x20]];
		data[p + 0x401] = BGCOLOR[idx | bg[q | 0x21]];
		data[p + 0x402] = BGCOLOR[idx | bg[q | 0x22]];
		data[p + 0x403] = BGCOLOR[idx | bg[q | 0x23]];
		data[p + 0x404] = BGCOLOR[idx | bg[q | 0x24]];
		data[p + 0x405] = BGCOLOR[idx | bg[q | 0x25]];
		data[p + 0x406] = BGCOLOR[idx | bg[q | 0x26]];
		data[p + 0x407] = BGCOLOR[idx | bg[q | 0x27]];
		data[p + 0x500] = BGCOLOR[idx | bg[q | 0x28]];
		data[p + 0x501] = BGCOLOR[idx | bg[q | 0x29]];
		data[p + 0x502] = BGCOLOR[idx | bg[q | 0x2a]];
		data[p + 0x503] = BGCOLOR[idx | bg[q | 0x2b]];
		data[p + 0x504] = BGCOLOR[idx | bg[q | 0x2c]];
		data[p + 0x505] = BGCOLOR[idx | bg[q | 0x2d]];
		data[p + 0x506] = BGCOLOR[idx | bg[q | 0x2e]];
		data[p + 0x507] = BGCOLOR[idx | bg[q | 0x2f]];
		data[p + 0x600] = BGCOLOR[idx | bg[q | 0x30]];
		data[p + 0x601] = BGCOLOR[idx | bg[q | 0x31]];
		data[p + 0x602] = BGCOLOR[idx | bg[q | 0x32]];
		data[p + 0x603] = BGCOLOR[idx | bg[q | 0x33]];
		data[p + 0x604] = BGCOLOR[idx | bg[q | 0x34]];
		data[p + 0x605] = BGCOLOR[idx | bg[q | 0x35]];
		data[p + 0x606] = BGCOLOR[idx | bg[q | 0x36]];
		data[p + 0x607] = BGCOLOR[idx | bg[q | 0x37]];
		data[p + 0x700] = BGCOLOR[idx | bg[q | 0x38]];
		data[p + 0x701] = BGCOLOR[idx | bg[q | 0x39]];
		data[p + 0x702] = BGCOLOR[idx | bg[q | 0x3a]];
		data[p + 0x703] = BGCOLOR[idx | bg[q | 0x3b]];
		data[p + 0x704] = BGCOLOR[idx | bg[q | 0x3c]];
		data[p + 0x705] = BGCOLOR[idx | bg[q | 0x3d]];
		data[p + 0x706] = BGCOLOR[idx | bg[q | 0x3e]];
		data[p + 0x707] = BGCOLOR[idx | bg[q | 0x3f]];
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x1f8;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0x1ff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x1f8;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x1f8;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x1f8;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new C30(49152000 / 2048, rate);
	}
};

#endif //SKY_KID_H
