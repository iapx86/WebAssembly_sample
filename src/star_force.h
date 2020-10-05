/*
 *
 *	Star Force
 *
 */

#ifndef STAR_FORCE_H
#define STAR_FORCE_H

#include "z80.h"
#include "sn76489.h"
#include "senjyo_sound.h"

enum {
	EXTEND_50000_200000_500000, EXTEND_100000_300000_800000, EXTEND_50000_200000, EXEND_100000_300000,
	EXTEND_50000, EXTEND_100000, EXTEND_200000, EXTEND_NO,
};

enum {
	DIFFICULTY_NORMAL, DIFFICULTY_DIFFICULT1, DIFFICULTY_DIFFICULT2,
	DIFFICULTY_DIFFICULT3, DIFFICULTY_DIFFICULT4, DIFFICULTY_DIFFICULT5,
};

struct StarForce {
	static unsigned char PRG1[], PRG2[], FG[], BG1[], BG2[], BG3[], OBJ[], SND[];

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

	uint8_t ram[0x3c00] = {};
	uint8_t ram2[0x400] = {};
	uint8_t in[6] = {0, 0, 0, 0, 0xc0, 0};
	int count = 0;
	int timer = 0;
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

	uint8_t fg[0x8000] = {};
	uint8_t bg1[0x10000] = {};
	uint8_t bg2[0x10000] = {};
	uint8_t bg3[0x8000] = {};
	uint8_t obj[0x20000] = {};
	int rgb[0x200] = {};

	Z80 cpu, cpu2;

	StarForce() {
		// CPU周りの初期化
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[i].base = PRG1 + i * 0x100;
		for (int i = 0; i < 0x3c; i++) {
			cpu.memorymap[0x80 + i].base = ram + i * 0x100;
			cpu.memorymap[0x80 + i].write = nullptr;
		}
		cpu.memorymap[0xd0].read = [&](int addr) -> int { return (addr &= 0xff) < 6 ? in[addr] : 0xff; };
		cpu.memorymap[0xd0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 2:
				return void(cpu_irq = false);
			case 4:
				return cpu2_command = data, void(pio.irq = pio.fInterruptEnable);
			}
		};

		cpu.check_interrupt = [&]() { return cpu_irq && cpu.interrupt(); };

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[0x40 + i].base = ram2 + i * 0x100;
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		cpu2.memorymap[0x80].write = [&](int addr, int data) { sound0->write(data, count); };
		cpu2.memorymap[0x90].write = [&](int addr, int data) { sound1->write(data, count); };
		cpu2.memorymap[0xa0].write = [&](int addr, int data) { sound2->write(data, count); };
		cpu2.memorymap[0xd0].write = [&](int addr, int data) { sound3->write(1, data & 15, count); };
		for (int i = 0; i < 0x100; i++) {
			cpu2.iomap[i].read = [&](int addr) { return (addr & 0xff) == 0 ? cpu2_command : 0xff; };
			cpu2.iomap[i].write = [&](int addr, int data) {
				switch (addr & 0xff) {
				case 1:
					return void(data == 0xa7 && (pio.fInterruptEnable = true));
				case 9:
					return void(data == 0xd7 && (ctc.fInterruptEnable = true));
				case 0xa:
					if ((ctc.cmd & 4) != 0) {
						sound3->write(0, (data ? data : 256) * ((ctc.cmd & 0x20) != 0 ? 16 : 1), count);
						ctc.cmd &= ~4;
					}
					else if ((data & 1) != 0)
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
		convertFG();
		convertBG();
		convertOBJ();
	}

	StarForce *execute() {
		cpu_irq = true;
		for (count = 0; count < 3; count++) {
			if (timer == 0)
				ctc.irq = ctc.fInterruptEnable;
			Cpu *cpus[] = {&cpu, &cpu2};
			Cpu::multiple_execute(2, cpus, 0x800);
			if (++timer >= 2)
				timer = 0;
		}
		return this;
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
			cpu.reset();
			cpu_irq = false;
			cpu2.reset();
			timer = 0;
			pio.irq = false;
			pio.fInterruptEnable = false;
			ctc.irq = false;
			ctc.fInterruptEnable = false;
		}
		return this;
	}

	StarForce *updateInput() {
		// クレジット/スタートボタン処理
		if (fCoin)
			in[2] |= 1 << 0, --fCoin;
		else
			in[2] &= ~(1 << 0);
		if (fStart1P)
			in[2] |= 1 << 2, --fStart1P;
		else
			in[2] &= ~(1 << 2);
		if (fStart2P)
			in[2] |= 1 << 3, --fStart2P;
		else
			in[2] &= ~(1 << 3);

		// 連射処理
		if (fTurbo)
			in[0] ^= 1 << 4;
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
			in[0] = in[0] & ~(1 << 3) | 1 << 2;
		else
			in[0] &= ~(1 << 2);
	}

	void right(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 1) | 1 << 0;
		else
			in[0] &= ~(1 << 0);
	}

	void down(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 2) | 1 << 3;
		else
			in[0] &= ~(1 << 3);
	}

	void left(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 0) | 1 << 1;
		else
			in[0] &= ~(1 << 1);
	}

	void triggerA(bool fDown) {
		if (fDown)
			in[0] |= 1 << 4;
		else
			in[0] &= ~(1 << 4);
	}

	void triggerB(bool fDown) {
		if (!(fTurbo = fDown))
			in[0] &= ~(1 << 4);
	}

	void convertRGB() {
		for (int j = 0; j < 0x200; j++) {
			const int e = ram[0x1c00 + j], i = e >> 6 & 3, r = e << 2 & 0xc, g = e & 0xc, b = e >> 2 & 0xc;
			rgb[j] = (r ? r | i : 0) * 255 / 15		// Red
				| (g ? g | i : 0) * 255 / 15 << 8	// Green
				| (b ? b | i : 0) * 255 / 15 << 16	// Blue
				| 0xff000000;						// Alpha
		}
	}

	void convertFG() {
		for (int p = 0, q = 0, i = 0; i < 512; q += 8, i++)
			for (int j = 7; j >= 0; --j)
				for (int k = 7; k >= 0; --k)
					fg[p++] = FG[q + k] >> j << 2 & 4 | FG[q + k + 0x1000] >> j << 1 & 2 | FG[q + k + 0x2000] >> j & 1;
	}

	void convertBG() {
		for (int p = 0, q = 0, i = 0; i < 256; q += 32, i++) {
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg1[p++] = BG1[q + k + 16] >> j << 2 & 4 | BG1[q + k + 0x2000 + 16] >> j << 1 & 2 | BG1[q + k + 0x4000 + 16] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg1[p++] = BG1[q + k] >> j << 2 & 4 | BG1[q + k + 0x2000] >> j << 1 & 2 | BG1[q + k + 0x4000] >> j & 1;
			}
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg1[p++] = BG1[q + k + 24] >> j << 2 & 4 | BG1[q + k + 0x2000 + 24] >> j << 1 & 2 | BG1[q + k + 0x4000 + 24] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg1[p++] = BG1[q + k + 8] >> j << 2 & 4 | BG1[q + k + 0x2000 + 8] >> j << 1 & 2 | BG1[q + k + 0x4000 + 8] >> j & 1;
			}
		}
		for (int p = 0, q = 0, i = 0; i < 256; q += 32, i++) {
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg2[p++] = BG2[q + k + 16] >> j << 2 & 4 | BG2[q + k + 0x2000 + 16] >> j << 1 & 2 | BG2[q + k + 0x4000 + 16] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg2[p++] = BG2[q + k] >> j << 2 & 4 | BG2[q + k + 0x2000] >> j << 1 & 2 | BG2[q + k + 0x4000] >> j & 1;
			}
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg2[p++] = BG2[q + k + 24] >> j << 2 & 4 | BG2[q + k + 0x2000 + 24] >> j << 1 & 2 | BG2[q + k + 0x4000 + 24] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg2[p++] = BG2[q + k + 8] >> j << 2 & 4 | BG2[q + k + 0x2000 + 8] >> j << 1 & 2 | BG2[q + k + 0x4000 + 8] >> j & 1;
			}
		}
		for (int p = 0, q = 0, i = 0; i < 128; q += 32, i++) {
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg3[p++] = BG3[q + k + 16] >> j << 2 & 4 | BG3[q + k + 0x1000 + 16] >> j << 1 & 2 | BG3[q + k + 0x2000 + 16] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg3[p++] = BG3[q + k] >> j << 2 & 4 | BG3[q + k + 0x1000] >> j << 1 & 2 | BG3[q + k + 0x2000] >> j & 1;
			}
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					bg3[p++] = BG3[q + k + 24] >> j << 2 & 4 | BG3[q + k + 0x1000 + 24] >> j << 1 & 2 | BG3[q + k + 0x2000 + 24] >> j & 1;
				for (int k = 7; k >= 0; --k)
					bg3[p++] = BG3[q + k + 8] >> j << 2 & 4 | BG3[q + k + 0x1000 + 8] >> j << 1 & 2 | BG3[q + k + 0x2000 + 8] >> j & 1;
			}
		}
	}

	void convertOBJ() {
		for (int p = 0, q = 0, i = 0; i < 512; q += 32, i++) {
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 16] >> j << 2 & 4 | OBJ[q + k + 0x4000 + 16] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 16] >> j & 1;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j << 2 & 4 | OBJ[q + k + 0x4000] >> j << 1 & 2 | OBJ[q + k + 0x8000] >> j & 1;
			}
			for (int j = 7; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 24] >> j << 2 & 4 | OBJ[q + k + 0x4000 + 24] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 24] >> j & 1;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 8] >> j << 2 & 4 | OBJ[q + k + 0x4000 + 8] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 8] >> j & 1;
			}
		}
	}

	void makeBitmap(int *data) {
		convertRGB();

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256, i++)
			memset(&data[p], 0, 224 * sizeof(int));

		// obj描画
		drawObj(data, 0);

		// bg描画
		int hScroll = (ram[0x1e20] | ram[0x1e21] << 8) + 15;
		int vScroll = ram[0x1e25];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		int k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x2000;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_3(data, p, ram[k]);

		// obj描画
		drawObj(data, 1);

		// bg描画
		hScroll = (ram[0x1e30] | ram[0x1e31] << 8) + 15;
		vScroll = ram[0x1e35];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x2800;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_2(data, p, ram[k]);

		// obj描画
		drawObj(data, 2);

		// bg描画
		hScroll = (ram[0x1e30] | ram[0x1e31] << 8) + 15;
		vScroll = ram[0x1e35];
		p = 256 * 8 * 2 + 224 + (hScroll & 15) + (-vScroll & 0x0f) * 256;
		k = vScroll + 15 >> 4 & 0x0f | hScroll & 0x7f0 | 0x3000;
		for (int i = 0; i < 15; k = k + 0x10 & 0x7ff | k & 0xf800, p -= 256 * 16 * 16 + 16, i++)
			for (int j = 0; j < 16; k = k + 1 & 0x0f | k & 0xfff0, p += 256 * 16, j++)
				xfer16x16_1(data, p, ram[k]);

		// obj描画
		drawObj(data, 3);

		// fg描画
		p = 256 * 8 * 2 + 232;
		k = 0x1040;
		for (int i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
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
			}
			else {
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

		if ((px = fg[q | 0x00]) != 0) data[p + 0x000] = idx | px;
		if ((px = fg[q | 0x01]) != 0) data[p + 0x001] = idx | px;
		if ((px = fg[q | 0x02]) != 0) data[p + 0x002] = idx | px;
		if ((px = fg[q | 0x03]) != 0) data[p + 0x003] = idx | px;
		if ((px = fg[q | 0x04]) != 0) data[p + 0x004] = idx | px;
		if ((px = fg[q | 0x05]) != 0) data[p + 0x005] = idx | px;
		if ((px = fg[q | 0x06]) != 0) data[p + 0x006] = idx | px;
		if ((px = fg[q | 0x07]) != 0) data[p + 0x007] = idx | px;
		if ((px = fg[q | 0x08]) != 0) data[p + 0x100] = idx | px;
		if ((px = fg[q | 0x09]) != 0) data[p + 0x101] = idx | px;
		if ((px = fg[q | 0x0a]) != 0) data[p + 0x102] = idx | px;
		if ((px = fg[q | 0x0b]) != 0) data[p + 0x103] = idx | px;
		if ((px = fg[q | 0x0c]) != 0) data[p + 0x104] = idx | px;
		if ((px = fg[q | 0x0d]) != 0) data[p + 0x105] = idx | px;
		if ((px = fg[q | 0x0e]) != 0) data[p + 0x106] = idx | px;
		if ((px = fg[q | 0x0f]) != 0) data[p + 0x107] = idx | px;
		if ((px = fg[q | 0x10]) != 0) data[p + 0x200] = idx | px;
		if ((px = fg[q | 0x11]) != 0) data[p + 0x201] = idx | px;
		if ((px = fg[q | 0x12]) != 0) data[p + 0x202] = idx | px;
		if ((px = fg[q | 0x13]) != 0) data[p + 0x203] = idx | px;
		if ((px = fg[q | 0x14]) != 0) data[p + 0x204] = idx | px;
		if ((px = fg[q | 0x15]) != 0) data[p + 0x205] = idx | px;
		if ((px = fg[q | 0x16]) != 0) data[p + 0x206] = idx | px;
		if ((px = fg[q | 0x17]) != 0) data[p + 0x207] = idx | px;
		if ((px = fg[q | 0x18]) != 0) data[p + 0x300] = idx | px;
		if ((px = fg[q | 0x19]) != 0) data[p + 0x301] = idx | px;
		if ((px = fg[q | 0x1a]) != 0) data[p + 0x302] = idx | px;
		if ((px = fg[q | 0x1b]) != 0) data[p + 0x303] = idx | px;
		if ((px = fg[q | 0x1c]) != 0) data[p + 0x304] = idx | px;
		if ((px = fg[q | 0x1d]) != 0) data[p + 0x305] = idx | px;
		if ((px = fg[q | 0x1e]) != 0) data[p + 0x306] = idx | px;
		if ((px = fg[q | 0x1f]) != 0) data[p + 0x307] = idx | px;
		if ((px = fg[q | 0x20]) != 0) data[p + 0x400] = idx | px;
		if ((px = fg[q | 0x21]) != 0) data[p + 0x401] = idx | px;
		if ((px = fg[q | 0x22]) != 0) data[p + 0x402] = idx | px;
		if ((px = fg[q | 0x23]) != 0) data[p + 0x403] = idx | px;
		if ((px = fg[q | 0x24]) != 0) data[p + 0x404] = idx | px;
		if ((px = fg[q | 0x25]) != 0) data[p + 0x405] = idx | px;
		if ((px = fg[q | 0x26]) != 0) data[p + 0x406] = idx | px;
		if ((px = fg[q | 0x27]) != 0) data[p + 0x407] = idx | px;
		if ((px = fg[q | 0x28]) != 0) data[p + 0x500] = idx | px;
		if ((px = fg[q | 0x29]) != 0) data[p + 0x501] = idx | px;
		if ((px = fg[q | 0x2a]) != 0) data[p + 0x502] = idx | px;
		if ((px = fg[q | 0x2b]) != 0) data[p + 0x503] = idx | px;
		if ((px = fg[q | 0x2c]) != 0) data[p + 0x504] = idx | px;
		if ((px = fg[q | 0x2d]) != 0) data[p + 0x505] = idx | px;
		if ((px = fg[q | 0x2e]) != 0) data[p + 0x506] = idx | px;
		if ((px = fg[q | 0x2f]) != 0) data[p + 0x507] = idx | px;
		if ((px = fg[q | 0x30]) != 0) data[p + 0x600] = idx | px;
		if ((px = fg[q | 0x31]) != 0) data[p + 0x601] = idx | px;
		if ((px = fg[q | 0x32]) != 0) data[p + 0x602] = idx | px;
		if ((px = fg[q | 0x33]) != 0) data[p + 0x603] = idx | px;
		if ((px = fg[q | 0x34]) != 0) data[p + 0x604] = idx | px;
		if ((px = fg[q | 0x35]) != 0) data[p + 0x605] = idx | px;
		if ((px = fg[q | 0x36]) != 0) data[p + 0x606] = idx | px;
		if ((px = fg[q | 0x37]) != 0) data[p + 0x607] = idx | px;
		if ((px = fg[q | 0x38]) != 0) data[p + 0x700] = idx | px;
		if ((px = fg[q | 0x39]) != 0) data[p + 0x701] = idx | px;
		if ((px = fg[q | 0x3a]) != 0) data[p + 0x702] = idx | px;
		if ((px = fg[q | 0x3b]) != 0) data[p + 0x703] = idx | px;
		if ((px = fg[q | 0x3c]) != 0) data[p + 0x704] = idx | px;
		if ((px = fg[q | 0x3d]) != 0) data[p + 0x705] = idx | px;
		if ((px = fg[q | 0x3e]) != 0) data[p + 0x706] = idx | px;
		if ((px = fg[q | 0x3f]) != 0) data[p + 0x707] = idx | px;
	}

	void xfer16x16_1(int *data, int dst, int src) {
		const int idx = src >> 4 & 8 | src >> 1 & 0x30 | 0x40;
		int px;

		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg1[src++]) != 0)
					data[dst] = idx | px;
	}

	void xfer16x16_2(int *data, int dst, int src) {
		const int idx = src >> 2 & 0x38 | 0x80;
		int px;

		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg2[src++]) != 0)
					data[dst] = idx | px;
	}

	void xfer16x16_3(int *data, int dst, int src) {
		const int idx = src >> 2 & 0x38 | 0xc0;
		int px;

		src = src << 8 & 0x7f00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = bg3[src++]) != 0)
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
				if ((px = obj[src++]) != 0)
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
				if ((px = obj[src++]) != 0)
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
				if ((px = obj[--src]) != 0)
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
				if ((px = obj[--src]) != 0)
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new SN76489(2000000, rate, 3);
		sound1 = new SN76489(2000000, rate, 3);
		sound2 = new SN76489(2000000, rate, 3);
		sound3 = new SenjyoSound(SND, 2000000, rate, 3);
		Z80::init();
	}
};

#endif //STAR_FORCE_H
