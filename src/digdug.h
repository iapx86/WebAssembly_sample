/*
 *
 *	DigDug
 *
 */

#ifndef DIGDUG_H
#define DIGDUG_H

#include <cstring>
#include "z80.h"
#include "mb8840.h"
#include "pac-man_sound.h"

enum {
	RANK_A, RANK_B, RANK_C, RANK_D,
};

enum {
	BONUS_NONE,	BONUS_A, BONUS_B, BONUS_C, BONUS_D, BONUS_E, BONUS_F, BONUS_G,
};

struct DigDug {
	static unsigned char RAM[], PRG1[], PRG2[], PRG3[], BG2[], MAPDATA[], BG4[], OBJ[], SND[], BGCOLOR[], OBJCOLOR[], RGB[], IO[];

	static const int cxScreen = 224;
	static const int cyScreen = 288;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static PacManSound *sound0;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int dwStick = 0xf;
	int nDigdug = 3;
	int nBonus = BONUS_F;
	int nRank = RANK_B;
	bool fContinue = false;
	bool fAttract = true;

	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
	bool fInterruptEnable2 = false;
	bool fNmiEnable = false;
	uint8_t ram[0x2000] = {};
	uint8_t mmi[0x100] = {};
	uint8_t mmo[0x100] = {};
	int count = 0;
	int dmactrl = 0;
	uint8_t ioport[0x100] = {};

	bool fBG2Attribute = true;
	bool fBG4Disable = true;
	bool fFlip = true;
	int dwBG4Color = 3;
	int dwBG4Select = 3;
	uint8_t bg2[0x2000] = {};
	uint8_t bg4[0x10000] = {};
	uint8_t obj[0x10000] = {};
	uint8_t bgcolor[0x100] = {};
	uint8_t objcolor[0x100] = {};
	int rgb[0x20] = {};

	Z80 cpu[3];
	MB8840 mcu;

	DigDug() {
		// CPU周りの初期化
		memcpy(ram, RAM, 0x2000);
		memset(mmi, 0xff, 0x100);

		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };
		auto interrupt = [](MB8840& _mcu) {
			_mcu.cause = _mcu.cause & ~4 | !_mcu.interrupt() << 2;
			for (int op = _mcu.execute(); op != 0x3c && (op != 0x25 || _mcu.cause & 4); op = _mcu.execute())
				op == 0x25 && (_mcu.cause &= ~4);
		};

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x3f))
				cpu[0].memorymap[page].base = PRG1 + (page & 0x3f) * 0x100;
			else if (range(page, 0x68, 0x68))
				cpu[0].memorymap[page].write = [&](int addr, int data) {
					switch (addr & 0xf0) {
					case 0x00:
					case 0x10:
						return sound0->write(addr, data, count);
					case 0x20:
						switch (addr & 0x0f) {
						case 0:
							return void(fInterruptEnable0 = (data & 1) != 0);
						case 1:
							return void(fInterruptEnable1 = (data & 1) != 0);
						case 2:
							return mmo[0x22] && !data && (fInterruptEnable2 = true), void(mmo[0x22] = data);
						case 3:
							return data & 1 ? (cpu[1].enable(), cpu[2].enable()) : (cpu[1].disable(), cpu[2].disable());
						}
					}
				};
			else if (range(page, 0x70, 0x70)) {
				cpu[0].memorymap[page].read = [&](int addr) {
					int data = 0xff;
					if (dmactrl & 1)
						data &= mcu.o, mcu.k |= 8, interrupt(mcu);
					if (dmactrl & 2)
						data &= ioport[addr & 0xff];
					return data;
				};
				cpu[0].memorymap[page].write = [&](int addr, int data) {
					if (dmactrl & 1)
						mcu.k = data & 7, interrupt(mcu);
				};
			} else if (range(page, 0x71, 0x71)) {
				cpu[0].memorymap[page].read = [&](int addr) { return dmactrl; };
				cpu[0].memorymap[page].write = [&](int addr, int data) {
					fNmiEnable = (data & 0xe0) != 0;
					switch (dmactrl = data) {
					case 0x71:
					case 0xb1:
						if (mcu.mask & 4)
							for (mcu.execute(); mcu.pc != 0x182; mcu.execute()) {}
						return mcu.t = mcu.t + 1 & 0xff, mcu.k |= 8, interrupt(mcu);
					case 0xd2:
						return void(memcpy(ioport, mmi, 2));
					}
				};
			} else if (range(page, 0x80, 0x87)) {
				cpu[0].memorymap[page].base = ram + (page & 7) * 0x100;
				cpu[0].memorymap[page].write = nullptr;
			} else if (range(page, 0x88, 0x8b, 4)) {
				cpu[0].memorymap[page].base = ram + (8 | page & 3) * 0x100;
				cpu[0].memorymap[page].write = nullptr;
			} else if (range(page, 0x90, 0x93, 4)) {
				cpu[0].memorymap[page].base = ram + (0x10 | page & 3) * 0x100;
				cpu[0].memorymap[page].write = nullptr;
			} else if (range(page, 0x98, 0x9b, 4)) {
				cpu[0].memorymap[page].base = ram + (0x18 | page & 3) * 0x100;
				cpu[0].memorymap[page].write = nullptr;
			} else if (range(page, 0xa0, 0xa0))
				cpu[0].memorymap[0xa0].write = [&](int addr, int data) {
					switch (addr & 7) {
					case 0:
						return void(dwBG4Select = dwBG4Select & 2 | data & 1);
					case 1:
						return void(dwBG4Select = dwBG4Select & 1 | data << 1 & 2);
					case 2:
						return void(fBG2Attribute = (data & 1) != 0);
					case 3:
						return void(fBG4Disable = (data & 1) != 0);
					case 4:
						return void(dwBG4Color = dwBG4Color & 2 | data & 1);
					case 5:
						return void(dwBG4Color = dwBG4Color & 1 | data << 1 & 2);
					case 7:
						return void(fFlip = false);
					}
				};

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x1f))
				cpu[1].memorymap[page].base = PRG2 + (page & 0x1f) * 0x100;
			else if (range(page, 0x40, 0xff))
				cpu[1].memorymap[page] = cpu[0].memorymap[page];

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0xf))
				cpu[2].memorymap[page].base = PRG3 + (page & 0xf) * 0x100;
			else if (range(page, 0x40, 0xff))
				cpu[2].memorymap[page] = cpu[0].memorymap[page];

		memcpy(mcu.rom, IO, 0x400);
		mcu.r = 0xffff;

		mmi[0] = 0x99; // DIPSW A
		mmi[1] = 0x2e; // DIPSW B

		// Videoの初期化
		convertRGB();
		convertBG();
		convertOBJ();
	}

	DigDug *execute() {
		Cpu *cpus[] = {&cpu[0], &cpu[1], &cpu[2]};
		if (fInterruptEnable0)
			cpu[0].interrupt();
		if (fInterruptEnable1)
			cpu[1].interrupt();
		count = 0;
		if (fInterruptEnable2) {
			fInterruptEnable2 = false;
			cpu[2].non_maskable_interrupt();		// SOUND INTERRUPT
		}
		for (int i = 128; i != 0; --i) {
			if (fNmiEnable)
				cpu[0].non_maskable_interrupt();	// DMA INTERRUPT
			Cpu::multiple_execute(3, cpus, 32);
		}
		count = 1;
		if (fInterruptEnable2) {
			fInterruptEnable2 = false;
			cpu[2].non_maskable_interrupt();		// SOUND INTERRUPT
		}
		for (int i = 128; i != 0; --i) {
			if (fNmiEnable)
				cpu[0].non_maskable_interrupt();	// DMA INTERRUPT
			Cpu::multiple_execute(3, cpus, 32);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	DigDug *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nDigdug) {
			case 1:
				mmi[0] &= 0x3f;
				break;
			case 2:
				mmi[0] = mmi[0] & 0x3f | 0x40;
				break;
			case 3:
				mmi[0] = mmi[0] & 0x3f | 0x80;
				break;
			case 5:
				mmi[0] |= 0xc0;
				break;
			}
			switch (nRank) {
			case RANK_A:
				mmi[1] &= 0xfc;
				break;
			case RANK_B:
				mmi[1] = mmi[1] & 0xfc | 0x02;
				break;
			case RANK_C:
				mmi[1] = mmi[1] & 0xfc | 0x01;
				break;
			case RANK_D:
				mmi[1] |= 0x03;
				break;
			}
			switch (nBonus) {
			case BONUS_NONE:
				mmi[0] &= 0xc7;
				break;
			case BONUS_A:
				mmi[0] = mmi[0] & 0xc7 | 0x20;
				break;
			case BONUS_B:
				mmi[0] = mmi[0] & 0xc7 | 0x10;
				break;
			case BONUS_C:
				mmi[0] = mmi[0] & 0xc7 | 0x30;
				break;
			case BONUS_D:
				mmi[0] = mmi[0] & 0xc7 | 0x08;
				break;
			case BONUS_E:
				mmi[0] = mmi[0] & 0xc7 | 0x28;
				break;
			case BONUS_F:
				mmi[0] = mmi[0] & 0xc7 | 0x18;
				break;
			case BONUS_G:
				mmi[0] |= 0x38;
				break;
			}
			if (fContinue)
				mmi[1] &= 0xf7;
			else
				mmi[1] |= 0x08;
			if (fAttract)
				mmi[1] &= 0xef;
			else
				mmi[1] |= 0x10;
			if (!fTest)
				fReset = true;
		}

		mcu.r = mcu.r & ~0x8000 | !fTest << 15;

		// リセット処理
		if (fReset) {
			fReset = false;
			fInterruptEnable0 = fInterruptEnable1 = fInterruptEnable2 = false;
			cpu[0].reset();
			cpu[1].disable();
			cpu[2].disable();
			for (mcu.reset(); ~mcu.mask & 4; mcu.execute()) {}
		}
		return this;
	}

	DigDug *updateInput() {
		mcu.r = mcu.r & ~0x4c0f | dwStick | !fCoin << 14 | !fStart1P << 10 | !fStart2P << 11;
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
		dwStick = dwStick & ~(1 << 0) | fDown << 2 | !fDown << 0;
	}

	void right(bool fDown) {
		dwStick = dwStick & ~(1 << 1) | fDown << 3 | !fDown << 1;
	}

	void down(bool fDown) {
		dwStick = dwStick & ~(1 << 2) | fDown << 0 | !fDown << 2;
	}

	void left(bool fDown) {
		dwStick = dwStick & ~(1 << 3) | fDown << 1 | !fDown << 3;
	}

	void triggerA(bool fDown) {
		mcu.r = mcu.r & ~(1 << 8) | !fDown << 8;
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
			bgcolor[i] = BGCOLOR[i] & 0xf;
		for (int p = 0, q = 0, i = 128; i != 0; q += 8, --i)
			for (int j = 0; j < 8; j++)
				for (int k = 7; k >= 0; --k)
					bg2[p++] = BG2[q + k] >> j & 1;
		for (int p = 0, q = 0, i = 256; i != 0; q += 16, --i) {
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg4[p++] = BG4[q + k + 8] >> j & 1 | BG4[q + k + 8] >> (j + 3) & 2;
			for (int j = 3; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					bg4[p++] = BG4[q + k] >> j & 1 | BG4[q + k] >> (j + 3) & 2;
		}
	}

	void convertOBJ() {
		for (int i = 0; i < 256; i++)
			objcolor[i] = OBJCOLOR[i] & 0xf | 0x10;
		for (int p = 0, q = 0, i = 256; i != 0; q += 64, --i) {
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
		// bg描画
		if (!fFlip) {
			int p = 256 * 8 * 4 + 232;
			int k = 0x40;
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
		} else {
			int p = 256 * 8 * 35 + 16;
			int k = 0x40;
			for (int i = 0; i < 28; p += 256 * 8 * 32 + 8, i++)
				for (int j = 0; j < 32; k++, p -= 256 * 8, j++)
					xfer8x8HV(data, p, k);
			p = 256 * 8 * 3 + 16;
			k = 2;
			for (int i = 0; i < 28; p += 8, k++, i++)
				xfer8x8HV(data, p, k);
			p = 256 * 8 * 2 + 16;
			k = 0x22;
			for (int i = 0; i < 28; p += 8, k++, i++)
				xfer8x8HV(data, p, k);
			p = 256 * 8 * 37 + 16;
			k = 0x3c2;
			for (int i = 0; i < 28; p += 8, k++, i++)
				xfer8x8HV(data, p, k);
			p = 256 * 8 * 36 + 16;
			k = 0x3e2;
			for (int i = 0; i < 28; p += 8, k++, i++)
				xfer8x8HV(data, p, k);
		}

		// obj描画
		if (!fFlip)
			for (int k = 0xb80, i = 64; i != 0; k += 2, --i) {
				const int x = ram[k + 0x800] - 1 & 0xff;
				const int y = (ram[k + 0x801] - 55 & 0xff) + 32;
				if (ram[k] < 0x80) {
					const int src = ram[k] | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
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
				} else if (ram[k] < 0xc0) {
					const int src = ram[k] << 2 & 0x3c | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
					case 0: // ノーマル
						xfer16x16(data, x | y << 8, src | 0x82);
						xfer16x16(data, x | y + 16 << 8, src | 0x83);
						xfer16x16(data, x + 16 & 0xff | y << 8, src | 0x80);
						xfer16x16(data, x + 16 & 0xff | y + 16 << 8, src | 0x81);
						break;
					case 1: // V反転
						xfer16x16V(data, x | y << 8, src | 0x83);
						xfer16x16V(data, x | y + 16 << 8, src | 0x82);
						xfer16x16V(data, x + 16 & 0xff | y << 8, src | 0x81);
						xfer16x16V(data, x + 16 & 0xff | y + 16 << 8, src | 0x80);
						break;
					case 2: // H反転
						xfer16x16H(data, x | y << 8, src | 0x80);
						xfer16x16H(data, x | y + 16 << 8, src | 0x81);
						xfer16x16H(data, x + 16 & 0xff | y << 8, src | 0x82);
						xfer16x16H(data, x + 16 & 0xff | y + 16 << 8, src | 0x83);
						break;
					case 3: // HV反転
						xfer16x16HV(data, x | y << 8, src | 0x81);
						xfer16x16HV(data, x | y + 16 << 8, src | 0x80);
						xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 0x83);
						xfer16x16HV(data, x + 16 & 0xff | y + 16 << 8, src | 0x82);
						break;
					}
				} else {
					const int src = ram[k] << 2 & 0x3c | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
					case 0: // ノーマル
						xfer16x16(data, x | y << 8, src | 0xc2);
						xfer16x16(data, x | y + 16 << 8, src | 0xc3);
						xfer16x16(data, x + 16 & 0xff | y << 8, src | 0xc0);
						xfer16x16(data, x + 16 & 0xff | y + 16 << 8, src | 0xc1);
						break;
					case 1: // V反転
						xfer16x16V(data, x | y << 8, src | 0xc3);
						xfer16x16V(data, x | y + 16 << 8, src | 0xc2);
						xfer16x16V(data, x + 16 & 0xff | y << 8, src | 0xc1);
						xfer16x16V(data, x + 16 & 0xff | y + 16 << 8, src | 0xc0);
						break;
					case 2: // H反転
						xfer16x16H(data, x | y << 8, src | 0xc0);
						xfer16x16H(data, x | y + 16 << 8, src | 0xc1);
						xfer16x16H(data, x + 16 & 0xff | y << 8, src | 0xc2);
						xfer16x16H(data, x + 16 & 0xff | y + 16 << 8, src | 0xc3);
						break;
					case 3: // HV反転
						xfer16x16HV(data, x | y << 8, src | 0xc1);
						xfer16x16HV(data, x | y + 16 << 8, src | 0xc0);
						xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 0xc3);
						xfer16x16HV(data, x + 16 & 0xff | y + 16 << 8, src | 0xc2);
						break;
					}
				}
			}
		else
			for (int k = 0xb80, i = 64; i != 0; k += 2, --i) {
				const int x = ram[k + 0x800] - 1 & 0xff;
				const int y = (ram[k + 0x801] - 55 & 0xff) + 32;
				if (ram[k] < 0x80) {
					const int src = ram[k] | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
					case 0: // ノーマル
						xfer16x16HV(data, x | y << 8, src);
						break;
					case 1: // V反転
						xfer16x16H(data, x | y << 8, src);
						break;
					case 2: // H反転
						xfer16x16V(data, x | y << 8, src);
						break;
					case 3: // HV反転
						xfer16x16(data, x | y << 8, src);
						break;
					}
				} else if (ram[k] < 0xc0) {
					const int src = ram[k] << 2 & 0x3c | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
					case 0: // ノーマル
						xfer16x16HV(data, x | y << 8, src | 0x81);
						xfer16x16HV(data, x | y + 16 << 8, src | 0x80);
						xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 0x83);
						xfer16x16HV(data, x + 16 & 0xff | y + 16 << 8, src | 0x82);
						break;
					case 1: // V反転
						xfer16x16H(data, x | y << 8, src | 0x80);
						xfer16x16H(data, x | y + 16 << 8, src | 0x81);
						xfer16x16H(data, x + 16 & 0xff | y << 8, src | 0x82);
						xfer16x16H(data, x + 16 & 0xff | y + 16 << 8, src | 0x83);
						break;
					case 2: // H反転
						xfer16x16V(data, x | y << 8, src | 0x83);
						xfer16x16V(data, x | y + 16 << 8, src | 0x82);
						xfer16x16V(data, x + 16 & 0xff | y << 8, src | 0x81);
						xfer16x16V(data, x + 16 & 0xff | y + 16 << 8, src | 0x80);
						break;
					case 3: // HV反転
						xfer16x16(data, x | y << 8, src | 0x82);
						xfer16x16(data, x | y + 16 << 8, src | 0x83);
						xfer16x16(data, x + 16 & 0xff | y << 8, src | 0x80);
						xfer16x16(data, x + 16 & 0xff | y + 16 << 8, src | 0x81);
						break;
					}
				} else {
					const int src = ram[k] << 2 & 0x3c | ram[k + 1] << 8;
					switch (ram[k + 0x1000] & 3) {
					case 0: // ノーマル
						xfer16x16HV(data, x | y << 8, src | 0xc1);
						xfer16x16HV(data, x | y + 16 << 8, src | 0xc0);
						xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 0xc3);
						xfer16x16HV(data, x + 16 & 0xff | y + 16 << 8, src | 0xc2);
						break;
					case 1: // V反転
						xfer16x16H(data, x | y << 8, src | 0xc0);
						xfer16x16H(data, x | y + 16 << 8, src | 0xc1);
						xfer16x16H(data, x + 16 & 0xff | y << 8, src | 0xc2);
						xfer16x16H(data, x + 16 & 0xff | y + 16 << 8, src | 0xc3);
						break;
					case 2: // H反転
						xfer16x16V(data, x | y << 8, src | 0xc3);
						xfer16x16V(data, x | y + 16 << 8, src | 0xc2);
						xfer16x16V(data, x + 16 & 0xff | y << 8, src | 0xc1);
						xfer16x16V(data, x + 16 & 0xff | y + 16 << 8, src | 0xc0);
						break;
					case 3: // HV反転
						xfer16x16(data, x | y << 8, src | 0xc2);
						xfer16x16(data, x | y + 16 << 8, src | 0xc3);
						xfer16x16(data, x + 16 & 0xff | y << 8, src | 0xc0);
						xfer16x16(data, x + 16 & 0xff | y + 16 << 8, src | 0xc1);
						break;
					}
				}
			}

		// palette変換
		int p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k) {
		const int color = fBG2Attribute ? ram[k + 0x400] & 0xf : ram[k] >> 4 & 0xe | ram[k] >> 3 & 2;
		const int q = ram[k] << 6 & 0x1fc0, r = MAPDATA[k | dwBG4Select << 10] << 6, idx = MAPDATA[k | dwBG4Select << 10] >> 2 & 0x3c | dwBG4Color << 6;

		if (fBG4Disable) {
			data[p + 0x000] = bg2[q | 0x00] * color;
			data[p + 0x001] = bg2[q | 0x01] * color;
			data[p + 0x002] = bg2[q | 0x02] * color;
			data[p + 0x003] = bg2[q | 0x03] * color;
			data[p + 0x004] = bg2[q | 0x04] * color;
			data[p + 0x005] = bg2[q | 0x05] * color;
			data[p + 0x006] = bg2[q | 0x06] * color;
			data[p + 0x007] = bg2[q | 0x07] * color;
			data[p + 0x100] = bg2[q | 0x08] * color;
			data[p + 0x101] = bg2[q | 0x09] * color;
			data[p + 0x102] = bg2[q | 0x0a] * color;
			data[p + 0x103] = bg2[q | 0x0b] * color;
			data[p + 0x104] = bg2[q | 0x0c] * color;
			data[p + 0x105] = bg2[q | 0x0d] * color;
			data[p + 0x106] = bg2[q | 0x0e] * color;
			data[p + 0x107] = bg2[q | 0x0f] * color;
			data[p + 0x200] = bg2[q | 0x10] * color;
			data[p + 0x201] = bg2[q | 0x11] * color;
			data[p + 0x202] = bg2[q | 0x12] * color;
			data[p + 0x203] = bg2[q | 0x13] * color;
			data[p + 0x204] = bg2[q | 0x14] * color;
			data[p + 0x205] = bg2[q | 0x15] * color;
			data[p + 0x206] = bg2[q | 0x16] * color;
			data[p + 0x207] = bg2[q | 0x17] * color;
			data[p + 0x300] = bg2[q | 0x18] * color;
			data[p + 0x301] = bg2[q | 0x19] * color;
			data[p + 0x302] = bg2[q | 0x1a] * color;
			data[p + 0x303] = bg2[q | 0x1b] * color;
			data[p + 0x304] = bg2[q | 0x1c] * color;
			data[p + 0x305] = bg2[q | 0x1d] * color;
			data[p + 0x306] = bg2[q | 0x1e] * color;
			data[p + 0x307] = bg2[q | 0x1f] * color;
			data[p + 0x400] = bg2[q | 0x20] * color;
			data[p + 0x401] = bg2[q | 0x21] * color;
			data[p + 0x402] = bg2[q | 0x22] * color;
			data[p + 0x403] = bg2[q | 0x23] * color;
			data[p + 0x404] = bg2[q | 0x24] * color;
			data[p + 0x405] = bg2[q | 0x25] * color;
			data[p + 0x406] = bg2[q | 0x26] * color;
			data[p + 0x407] = bg2[q | 0x27] * color;
			data[p + 0x500] = bg2[q | 0x28] * color;
			data[p + 0x501] = bg2[q | 0x29] * color;
			data[p + 0x502] = bg2[q | 0x2a] * color;
			data[p + 0x503] = bg2[q | 0x2b] * color;
			data[p + 0x504] = bg2[q | 0x2c] * color;
			data[p + 0x505] = bg2[q | 0x2d] * color;
			data[p + 0x506] = bg2[q | 0x2e] * color;
			data[p + 0x507] = bg2[q | 0x2f] * color;
			data[p + 0x600] = bg2[q | 0x30] * color;
			data[p + 0x601] = bg2[q | 0x31] * color;
			data[p + 0x602] = bg2[q | 0x32] * color;
			data[p + 0x603] = bg2[q | 0x33] * color;
			data[p + 0x604] = bg2[q | 0x34] * color;
			data[p + 0x605] = bg2[q | 0x35] * color;
			data[p + 0x606] = bg2[q | 0x36] * color;
			data[p + 0x607] = bg2[q | 0x37] * color;
			data[p + 0x700] = bg2[q | 0x38] * color;
			data[p + 0x701] = bg2[q | 0x39] * color;
			data[p + 0x702] = bg2[q | 0x3a] * color;
			data[p + 0x703] = bg2[q | 0x3b] * color;
			data[p + 0x704] = bg2[q | 0x3c] * color;
			data[p + 0x705] = bg2[q | 0x3d] * color;
			data[p + 0x706] = bg2[q | 0x3e] * color;
			data[p + 0x707] = bg2[q | 0x3f] * color;
		} else {
			data[p + 0x000] = bg2[q | 0x00] ? color : bgcolor[idx | bg4[r | 0x00]];
			data[p + 0x001] = bg2[q | 0x01] ? color : bgcolor[idx | bg4[r | 0x01]];
			data[p + 0x002] = bg2[q | 0x02] ? color : bgcolor[idx | bg4[r | 0x02]];
			data[p + 0x003] = bg2[q | 0x03] ? color : bgcolor[idx | bg4[r | 0x03]];
			data[p + 0x004] = bg2[q | 0x04] ? color : bgcolor[idx | bg4[r | 0x04]];
			data[p + 0x005] = bg2[q | 0x05] ? color : bgcolor[idx | bg4[r | 0x05]];
			data[p + 0x006] = bg2[q | 0x06] ? color : bgcolor[idx | bg4[r | 0x06]];
			data[p + 0x007] = bg2[q | 0x07] ? color : bgcolor[idx | bg4[r | 0x07]];
			data[p + 0x100] = bg2[q | 0x08] ? color : bgcolor[idx | bg4[r | 0x08]];
			data[p + 0x101] = bg2[q | 0x09] ? color : bgcolor[idx | bg4[r | 0x09]];
			data[p + 0x102] = bg2[q | 0x0a] ? color : bgcolor[idx | bg4[r | 0x0a]];
			data[p + 0x103] = bg2[q | 0x0b] ? color : bgcolor[idx | bg4[r | 0x0b]];
			data[p + 0x104] = bg2[q | 0x0c] ? color : bgcolor[idx | bg4[r | 0x0c]];
			data[p + 0x105] = bg2[q | 0x0d] ? color : bgcolor[idx | bg4[r | 0x0d]];
			data[p + 0x106] = bg2[q | 0x0e] ? color : bgcolor[idx | bg4[r | 0x0e]];
			data[p + 0x107] = bg2[q | 0x0f] ? color : bgcolor[idx | bg4[r | 0x0f]];
			data[p + 0x200] = bg2[q | 0x10] ? color : bgcolor[idx | bg4[r | 0x10]];
			data[p + 0x201] = bg2[q | 0x11] ? color : bgcolor[idx | bg4[r | 0x11]];
			data[p + 0x202] = bg2[q | 0x12] ? color : bgcolor[idx | bg4[r | 0x12]];
			data[p + 0x203] = bg2[q | 0x13] ? color : bgcolor[idx | bg4[r | 0x13]];
			data[p + 0x204] = bg2[q | 0x14] ? color : bgcolor[idx | bg4[r | 0x14]];
			data[p + 0x205] = bg2[q | 0x15] ? color : bgcolor[idx | bg4[r | 0x15]];
			data[p + 0x206] = bg2[q | 0x16] ? color : bgcolor[idx | bg4[r | 0x16]];
			data[p + 0x207] = bg2[q | 0x17] ? color : bgcolor[idx | bg4[r | 0x17]];
			data[p + 0x300] = bg2[q | 0x18] ? color : bgcolor[idx | bg4[r | 0x18]];
			data[p + 0x301] = bg2[q | 0x19] ? color : bgcolor[idx | bg4[r | 0x19]];
			data[p + 0x302] = bg2[q | 0x1a] ? color : bgcolor[idx | bg4[r | 0x1a]];
			data[p + 0x303] = bg2[q | 0x1b] ? color : bgcolor[idx | bg4[r | 0x1b]];
			data[p + 0x304] = bg2[q | 0x1c] ? color : bgcolor[idx | bg4[r | 0x1c]];
			data[p + 0x305] = bg2[q | 0x1d] ? color : bgcolor[idx | bg4[r | 0x1d]];
			data[p + 0x306] = bg2[q | 0x1e] ? color : bgcolor[idx | bg4[r | 0x1e]];
			data[p + 0x307] = bg2[q | 0x1f] ? color : bgcolor[idx | bg4[r | 0x1f]];
			data[p + 0x400] = bg2[q | 0x20] ? color : bgcolor[idx | bg4[r | 0x20]];
			data[p + 0x401] = bg2[q | 0x21] ? color : bgcolor[idx | bg4[r | 0x21]];
			data[p + 0x402] = bg2[q | 0x22] ? color : bgcolor[idx | bg4[r | 0x22]];
			data[p + 0x403] = bg2[q | 0x23] ? color : bgcolor[idx | bg4[r | 0x23]];
			data[p + 0x404] = bg2[q | 0x24] ? color : bgcolor[idx | bg4[r | 0x24]];
			data[p + 0x405] = bg2[q | 0x25] ? color : bgcolor[idx | bg4[r | 0x25]];
			data[p + 0x406] = bg2[q | 0x26] ? color : bgcolor[idx | bg4[r | 0x26]];
			data[p + 0x407] = bg2[q | 0x27] ? color : bgcolor[idx | bg4[r | 0x27]];
			data[p + 0x500] = bg2[q | 0x28] ? color : bgcolor[idx | bg4[r | 0x28]];
			data[p + 0x501] = bg2[q | 0x29] ? color : bgcolor[idx | bg4[r | 0x29]];
			data[p + 0x502] = bg2[q | 0x2a] ? color : bgcolor[idx | bg4[r | 0x2a]];
			data[p + 0x503] = bg2[q | 0x2b] ? color : bgcolor[idx | bg4[r | 0x2b]];
			data[p + 0x504] = bg2[q | 0x2c] ? color : bgcolor[idx | bg4[r | 0x2c]];
			data[p + 0x505] = bg2[q | 0x2d] ? color : bgcolor[idx | bg4[r | 0x2d]];
			data[p + 0x506] = bg2[q | 0x2e] ? color : bgcolor[idx | bg4[r | 0x2e]];
			data[p + 0x507] = bg2[q | 0x2f] ? color : bgcolor[idx | bg4[r | 0x2f]];
			data[p + 0x600] = bg2[q | 0x30] ? color : bgcolor[idx | bg4[r | 0x30]];
			data[p + 0x601] = bg2[q | 0x31] ? color : bgcolor[idx | bg4[r | 0x31]];
			data[p + 0x602] = bg2[q | 0x32] ? color : bgcolor[idx | bg4[r | 0x32]];
			data[p + 0x603] = bg2[q | 0x33] ? color : bgcolor[idx | bg4[r | 0x33]];
			data[p + 0x604] = bg2[q | 0x34] ? color : bgcolor[idx | bg4[r | 0x34]];
			data[p + 0x605] = bg2[q | 0x35] ? color : bgcolor[idx | bg4[r | 0x35]];
			data[p + 0x606] = bg2[q | 0x36] ? color : bgcolor[idx | bg4[r | 0x36]];
			data[p + 0x607] = bg2[q | 0x37] ? color : bgcolor[idx | bg4[r | 0x37]];
			data[p + 0x700] = bg2[q | 0x38] ? color : bgcolor[idx | bg4[r | 0x38]];
			data[p + 0x701] = bg2[q | 0x39] ? color : bgcolor[idx | bg4[r | 0x39]];
			data[p + 0x702] = bg2[q | 0x3a] ? color : bgcolor[idx | bg4[r | 0x3a]];
			data[p + 0x703] = bg2[q | 0x3b] ? color : bgcolor[idx | bg4[r | 0x3b]];
			data[p + 0x704] = bg2[q | 0x3c] ? color : bgcolor[idx | bg4[r | 0x3c]];
			data[p + 0x705] = bg2[q | 0x3d] ? color : bgcolor[idx | bg4[r | 0x3d]];
			data[p + 0x706] = bg2[q | 0x3e] ? color : bgcolor[idx | bg4[r | 0x3e]];
			data[p + 0x707] = bg2[q | 0x3f] ? color : bgcolor[idx | bg4[r | 0x3f]];
		}
	}

	void xfer8x8HV(int *data, int p, int k) {
		const int color = fBG2Attribute ? ram[k + 0x400] & 0xf : ram[k] >> 4 & 0xe | ram[k] >> 3 & 2;
		const int q = ram[k] << 6 & 0x1fc0, r = MAPDATA[k | dwBG4Select << 10] << 6, idx = MAPDATA[k | dwBG4Select << 10] >> 2 & 0x3c | dwBG4Color << 6;

		if (fBG4Disable) {
			data[p + 0x000] = bg2[q | 0x3f] * color;
			data[p + 0x001] = bg2[q | 0x3e] * color;
			data[p + 0x002] = bg2[q | 0x3d] * color;
			data[p + 0x003] = bg2[q | 0x3c] * color;
			data[p + 0x004] = bg2[q | 0x3b] * color;
			data[p + 0x005] = bg2[q | 0x3a] * color;
			data[p + 0x006] = bg2[q | 0x39] * color;
			data[p + 0x007] = bg2[q | 0x38] * color;
			data[p + 0x100] = bg2[q | 0x37] * color;
			data[p + 0x101] = bg2[q | 0x36] * color;
			data[p + 0x102] = bg2[q | 0x35] * color;
			data[p + 0x103] = bg2[q | 0x34] * color;
			data[p + 0x104] = bg2[q | 0x33] * color;
			data[p + 0x105] = bg2[q | 0x32] * color;
			data[p + 0x106] = bg2[q | 0x31] * color;
			data[p + 0x107] = bg2[q | 0x30] * color;
			data[p + 0x200] = bg2[q | 0x2f] * color;
			data[p + 0x201] = bg2[q | 0x2e] * color;
			data[p + 0x202] = bg2[q | 0x2d] * color;
			data[p + 0x203] = bg2[q | 0x2c] * color;
			data[p + 0x204] = bg2[q | 0x2b] * color;
			data[p + 0x205] = bg2[q | 0x2a] * color;
			data[p + 0x206] = bg2[q | 0x29] * color;
			data[p + 0x207] = bg2[q | 0x28] * color;
			data[p + 0x300] = bg2[q | 0x27] * color;
			data[p + 0x301] = bg2[q | 0x26] * color;
			data[p + 0x302] = bg2[q | 0x25] * color;
			data[p + 0x303] = bg2[q | 0x24] * color;
			data[p + 0x304] = bg2[q | 0x23] * color;
			data[p + 0x305] = bg2[q | 0x22] * color;
			data[p + 0x306] = bg2[q | 0x21] * color;
			data[p + 0x307] = bg2[q | 0x20] * color;
			data[p + 0x400] = bg2[q | 0x1f] * color;
			data[p + 0x401] = bg2[q | 0x1e] * color;
			data[p + 0x402] = bg2[q | 0x1d] * color;
			data[p + 0x403] = bg2[q | 0x1c] * color;
			data[p + 0x404] = bg2[q | 0x1b] * color;
			data[p + 0x405] = bg2[q | 0x1a] * color;
			data[p + 0x406] = bg2[q | 0x19] * color;
			data[p + 0x407] = bg2[q | 0x18] * color;
			data[p + 0x500] = bg2[q | 0x17] * color;
			data[p + 0x501] = bg2[q | 0x16] * color;
			data[p + 0x502] = bg2[q | 0x15] * color;
			data[p + 0x503] = bg2[q | 0x14] * color;
			data[p + 0x504] = bg2[q | 0x13] * color;
			data[p + 0x505] = bg2[q | 0x12] * color;
			data[p + 0x506] = bg2[q | 0x11] * color;
			data[p + 0x507] = bg2[q | 0x10] * color;
			data[p + 0x600] = bg2[q | 0x0f] * color;
			data[p + 0x601] = bg2[q | 0x0e] * color;
			data[p + 0x602] = bg2[q | 0x0d] * color;
			data[p + 0x603] = bg2[q | 0x0c] * color;
			data[p + 0x604] = bg2[q | 0x0b] * color;
			data[p + 0x605] = bg2[q | 0x0a] * color;
			data[p + 0x606] = bg2[q | 0x09] * color;
			data[p + 0x607] = bg2[q | 0x08] * color;
			data[p + 0x700] = bg2[q | 0x07] * color;
			data[p + 0x701] = bg2[q | 0x06] * color;
			data[p + 0x702] = bg2[q | 0x05] * color;
			data[p + 0x703] = bg2[q | 0x04] * color;
			data[p + 0x704] = bg2[q | 0x03] * color;
			data[p + 0x705] = bg2[q | 0x02] * color;
			data[p + 0x706] = bg2[q | 0x01] * color;
			data[p + 0x707] = bg2[q | 0x00] * color;
		} else {
			data[p + 0x000] = bg2[q | 0x3f] ? color : bgcolor[idx | bg4[r | 0x3f]];
			data[p + 0x001] = bg2[q | 0x3e] ? color : bgcolor[idx | bg4[r | 0x3e]];
			data[p + 0x002] = bg2[q | 0x3d] ? color : bgcolor[idx | bg4[r | 0x3d]];
			data[p + 0x003] = bg2[q | 0x3c] ? color : bgcolor[idx | bg4[r | 0x3c]];
			data[p + 0x004] = bg2[q | 0x3b] ? color : bgcolor[idx | bg4[r | 0x3b]];
			data[p + 0x005] = bg2[q | 0x3a] ? color : bgcolor[idx | bg4[r | 0x3a]];
			data[p + 0x006] = bg2[q | 0x39] ? color : bgcolor[idx | bg4[r | 0x39]];
			data[p + 0x007] = bg2[q | 0x38] ? color : bgcolor[idx | bg4[r | 0x38]];
			data[p + 0x100] = bg2[q | 0x37] ? color : bgcolor[idx | bg4[r | 0x37]];
			data[p + 0x101] = bg2[q | 0x36] ? color : bgcolor[idx | bg4[r | 0x36]];
			data[p + 0x102] = bg2[q | 0x35] ? color : bgcolor[idx | bg4[r | 0x35]];
			data[p + 0x103] = bg2[q | 0x34] ? color : bgcolor[idx | bg4[r | 0x34]];
			data[p + 0x104] = bg2[q | 0x33] ? color : bgcolor[idx | bg4[r | 0x33]];
			data[p + 0x105] = bg2[q | 0x32] ? color : bgcolor[idx | bg4[r | 0x32]];
			data[p + 0x106] = bg2[q | 0x31] ? color : bgcolor[idx | bg4[r | 0x31]];
			data[p + 0x107] = bg2[q | 0x30] ? color : bgcolor[idx | bg4[r | 0x30]];
			data[p + 0x200] = bg2[q | 0x2f] ? color : bgcolor[idx | bg4[r | 0x2f]];
			data[p + 0x201] = bg2[q | 0x2e] ? color : bgcolor[idx | bg4[r | 0x2e]];
			data[p + 0x202] = bg2[q | 0x2d] ? color : bgcolor[idx | bg4[r | 0x2d]];
			data[p + 0x203] = bg2[q | 0x2c] ? color : bgcolor[idx | bg4[r | 0x2c]];
			data[p + 0x204] = bg2[q | 0x2b] ? color : bgcolor[idx | bg4[r | 0x2b]];
			data[p + 0x205] = bg2[q | 0x2a] ? color : bgcolor[idx | bg4[r | 0x2a]];
			data[p + 0x206] = bg2[q | 0x29] ? color : bgcolor[idx | bg4[r | 0x29]];
			data[p + 0x207] = bg2[q | 0x28] ? color : bgcolor[idx | bg4[r | 0x28]];
			data[p + 0x300] = bg2[q | 0x27] ? color : bgcolor[idx | bg4[r | 0x27]];
			data[p + 0x301] = bg2[q | 0x26] ? color : bgcolor[idx | bg4[r | 0x26]];
			data[p + 0x302] = bg2[q | 0x25] ? color : bgcolor[idx | bg4[r | 0x25]];
			data[p + 0x303] = bg2[q | 0x24] ? color : bgcolor[idx | bg4[r | 0x24]];
			data[p + 0x304] = bg2[q | 0x23] ? color : bgcolor[idx | bg4[r | 0x23]];
			data[p + 0x305] = bg2[q | 0x22] ? color : bgcolor[idx | bg4[r | 0x22]];
			data[p + 0x306] = bg2[q | 0x21] ? color : bgcolor[idx | bg4[r | 0x21]];
			data[p + 0x307] = bg2[q | 0x20] ? color : bgcolor[idx | bg4[r | 0x20]];
			data[p + 0x400] = bg2[q | 0x1f] ? color : bgcolor[idx | bg4[r | 0x1f]];
			data[p + 0x401] = bg2[q | 0x1e] ? color : bgcolor[idx | bg4[r | 0x1e]];
			data[p + 0x402] = bg2[q | 0x1d] ? color : bgcolor[idx | bg4[r | 0x1d]];
			data[p + 0x403] = bg2[q | 0x1c] ? color : bgcolor[idx | bg4[r | 0x1c]];
			data[p + 0x404] = bg2[q | 0x1b] ? color : bgcolor[idx | bg4[r | 0x1b]];
			data[p + 0x405] = bg2[q | 0x1a] ? color : bgcolor[idx | bg4[r | 0x1a]];
			data[p + 0x406] = bg2[q | 0x19] ? color : bgcolor[idx | bg4[r | 0x19]];
			data[p + 0x407] = bg2[q | 0x18] ? color : bgcolor[idx | bg4[r | 0x18]];
			data[p + 0x500] = bg2[q | 0x17] ? color : bgcolor[idx | bg4[r | 0x17]];
			data[p + 0x501] = bg2[q | 0x16] ? color : bgcolor[idx | bg4[r | 0x16]];
			data[p + 0x502] = bg2[q | 0x15] ? color : bgcolor[idx | bg4[r | 0x15]];
			data[p + 0x503] = bg2[q | 0x14] ? color : bgcolor[idx | bg4[r | 0x14]];
			data[p + 0x504] = bg2[q | 0x13] ? color : bgcolor[idx | bg4[r | 0x13]];
			data[p + 0x505] = bg2[q | 0x12] ? color : bgcolor[idx | bg4[r | 0x12]];
			data[p + 0x506] = bg2[q | 0x11] ? color : bgcolor[idx | bg4[r | 0x11]];
			data[p + 0x507] = bg2[q | 0x10] ? color : bgcolor[idx | bg4[r | 0x10]];
			data[p + 0x600] = bg2[q | 0x0f] ? color : bgcolor[idx | bg4[r | 0x0f]];
			data[p + 0x601] = bg2[q | 0x0e] ? color : bgcolor[idx | bg4[r | 0x0e]];
			data[p + 0x602] = bg2[q | 0x0d] ? color : bgcolor[idx | bg4[r | 0x0d]];
			data[p + 0x603] = bg2[q | 0x0c] ? color : bgcolor[idx | bg4[r | 0x0c]];
			data[p + 0x604] = bg2[q | 0x0b] ? color : bgcolor[idx | bg4[r | 0x0b]];
			data[p + 0x605] = bg2[q | 0x0a] ? color : bgcolor[idx | bg4[r | 0x0a]];
			data[p + 0x606] = bg2[q | 0x09] ? color : bgcolor[idx | bg4[r | 0x09]];
			data[p + 0x607] = bg2[q | 0x08] ? color : bgcolor[idx | bg4[r | 0x08]];
			data[p + 0x700] = bg2[q | 0x07] ? color : bgcolor[idx | bg4[r | 0x07]];
			data[p + 0x701] = bg2[q | 0x06] ? color : bgcolor[idx | bg4[r | 0x06]];
			data[p + 0x702] = bg2[q | 0x05] ? color : bgcolor[idx | bg4[r | 0x05]];
			data[p + 0x703] = bg2[q | 0x04] ? color : bgcolor[idx | bg4[r | 0x04]];
			data[p + 0x704] = bg2[q | 0x03] ? color : bgcolor[idx | bg4[r | 0x03]];
			data[p + 0x705] = bg2[q | 0x02] ? color : bgcolor[idx | bg4[r | 0x02]];
			data[p + 0x706] = bg2[q | 0x01] ? color : bgcolor[idx | bg4[r | 0x01]];
			data[p + 0x707] = bg2[q | 0x00] ? color : bgcolor[idx | bg4[r | 0x00]];
		}
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px, h;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (dst >= 288 * 0x100)
			dst -= 0x10000;
		if ((h = 288 - (dst >> 8)) >= 16) {
			src = src << 8 & 0xff00;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
		} else {
			src = src << 8 & 0xff00;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
		}
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px, h;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (dst >= 288 * 0x100)
			dst -= 0x10000;
		if ((h = 288 - (dst >> 8)) >= 16) {
			src = (src << 8 & 0xff00) + 256 - 16;
			for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
		} else {
			src = (src << 8 & 0xff00) + 256 - 16;
			for (int i = h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[src++]]) != 0x1f)
						data[dst] = px;
		}
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px, h;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (dst >= 288 * 0x100)
			dst -= 0x10000;
		if ((h = 288 - (dst >> 8)) >= 16) {
			src = (src << 8 & 0xff00) + 16;
			for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
		} else {
			src = (src << 8 & 0xff00) + 16;
			for (int i = h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
		}
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc;
		int px, h;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (dst >= 288 * 0x100)
			dst -= 0x10000;
		if ((h = 288 - (dst >> 8)) >= 16) {
			src = (src << 8 & 0xff00) + 256;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
		} else {
			src = (src << 8 & 0xff00) + 256;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = objcolor[idx | obj[--src]]) != 0x1f)
						data[dst] = px;
		}
	}

	static void init(int rate) {
		sound0 = new PacManSound(SND, rate, 2);
		Z80::init();
	}
};

#endif //DIGDUG_H
