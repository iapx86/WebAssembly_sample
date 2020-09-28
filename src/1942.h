/*
 *
 *	1942
 *
 */

#ifndef _1942_H
#define _1942_H

#include "z80.h"
#include "ay-3-8910.h"

enum {
	BONUS_1ST_20000_2ND_80000_EVERY_80000, BONUS_1ST_20000_2ND_100000_EVERY_100000,
	BONUS_1ST_30000_2ND_80000_EVERY_80000, BONUS_1ST_30000_2ND_100000_EVERY_100000,
};

enum {
	RANK_NORMAL, RANK_EASY, RANK_HARD, RANK_VERY_HARD,
};

struct _1942 {
	static unsigned char PRG1[], PRG2[], FG[], BG[], OBJ[], RED[], GREEN[], BLUE[], FGCOLOR[], BGCOLOR[], OBJCOLOR[];

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = false;

	static AY_3_8910 *sound0, *sound1;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	bool fTurbo = 0;
	int nBonus = BONUS_1ST_20000_2ND_80000_EVERY_80000;
	int nLife = 3;
	int nRank = RANK_NORMAL;

	uint8_t ram[0x1d00] = {};
	uint8_t ram2[0x800] = {};
	uint8_t in[5] = {0xff, 0xff, 0xff, 0xf7, 0xff};
	struct {
		int addr = 0;
	} psg[2];
	int command = 0;
	int bank = 0x80;
	int timer = 0;
	bool cpu_irq = false;
	bool cpu_irq2 = false;

	uint8_t fg[0x8000] = {};
	uint8_t bg[0x20000] = {};
	uint8_t obj[0x20000] = {};
	uint8_t fgcolor[0x100] = {};
	uint8_t bgcolor[0x100] = {};
	uint8_t objcolor[0x100] = {};
	int rgb[0x100] = {};
	int dwScroll = 0;
	int palette = 0;
	int frame = 0;

	Z80 cpu, cpu2;

	_1942() {
		// CPU周りの初期化
		for (int i = 0; i < 0xc0; i++)
			cpu.memorymap[i].base = PRG1 + i * 0x100;
		cpu.memorymap[0xc0].read = [&](int addr) -> int { return (addr &= 0xff) < 5 ? in[addr] : 0xff; };
		cpu.memorymap[0xc8].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(command = data);
			case 2:
				return void(dwScroll = dwScroll & 0xff00 | data);
			case 3:
				return void(dwScroll = dwScroll & 0xff | data << 8);
			case 4:
				return (data & 0x10) != 0 ? cpu2.disable() : cpu2.enable();
			case 5:
				return void(palette = data << 4 & 0x30);
			case 6:
				const int bank = (data << 6 & 0xc0) + 0x80;
				if (bank == this->bank)
					return;
				for (int i = 0; i < 0x40; i++)
					cpu.memorymap[0x80 + i].base = PRG1 + (bank + i) * 0x100;
				return void(this->bank = bank);
			}
		};
		cpu.memorymap[0xcc].base = ram;
		cpu.memorymap[0xcc].write = nullptr;
		for (int i = 0; i < 0x0c; i++) {
			cpu.memorymap[0xd0 + i].base = ram + 0x100 + i * 0x100;
			cpu.memorymap[0xd0 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu.memorymap[0xe0 + i].base = ram + 0xd00 + i * 0x100;
			cpu.memorymap[0xe0 + i].write = nullptr;
		}

		cpu.check_interrupt = [&]() {
			if (cpu_irq && cpu.interrupt(0xd7)) // RST 10H
				return cpu_irq = false, true;
			if (cpu_irq2 && cpu.interrupt(0xcf)) // RST 08H
				return cpu_irq2 = false, true;
			return false;
		};

		for (int i = 0; i < 0x40; i++)
			cpu2.memorymap[i].base = PRG2 + i * 0x100;
		for (int i = 0; i < 8; i++) {
			cpu2.memorymap[0x40 + i].base = ram2 + i * 0x100;
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		cpu2.memorymap[0x60].read = [&](int addr) { return (addr & 0xff) == 0 ? command : 0xff; };
		cpu2.memorymap[0x80].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(psg[0].addr = data);
			case 1:
				return sound0->write(psg[0].addr, data, timer);
			}
		};
		cpu2.memorymap[0xc0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(psg[1].addr = data);
			case 1:
				return sound1->write(psg[1].addr, data, timer);
			}
		};

		// Videoの初期化
		convertRGB();
		convertFG();
		convertBG();
		convertOBJ();
	}

	_1942 *execute() {
		for (int i = 0; i < 16; i++) {
			if (i == 0)
				cpu_irq = true;
			if (i == 1)
				cpu_irq2 = true;
			if ((i & 3) == 0) {
				timer = i >> 2;
				cpu2.interrupt();
			}
			Cpu *cpus[] = {&cpu, &cpu2};
			Cpu::multiple_execute(2, cpus, 800);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	_1942 *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[3] |= 0xc0;
				break;
			case 1:
				in[3] = in[3] & ~0xc0 | 0x80;
				break;
			case 2:
				in[3] = in[3] & ~0xc0 | 0x40;
				break;
			case 5:
				in[3] &= ~0xc0;
				break;
			}
			switch (nBonus) {
			case BONUS_1ST_20000_2ND_80000_EVERY_80000:
				in[3] |= 0x30;
				break;
			case BONUS_1ST_20000_2ND_100000_EVERY_100000:
				in[3] = in[3] & ~0x30 | 0x20;
				break;
			case BONUS_1ST_30000_2ND_80000_EVERY_80000:
				in[3] = in[3] & ~0x30 | 0x10;
				break;
			case BONUS_1ST_30000_2ND_100000_EVERY_100000:
				in[3] &= ~0x30;
				break;
			}
			switch (nRank) {
			case RANK_NORMAL:
				in[4] |= 0x60;
				break;
			case RANK_EASY:
				in[4] = in[4] & ~0x60 | 0x40;
				break;
			case RANK_HARD:
				in[4] = in[4] & ~0x60 | 0x20;
				break;
			case RANK_VERY_HARD:
				in[4] &= ~0x60;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu_irq = cpu_irq2 = false;
			cpu.reset();
			cpu2.disable();
		}
		return this;
	}

	_1942 *updateInput() {
		// クレジット/スタートボタン処理
		if (fCoin)
			in[0] &= ~(1 << 4), --fCoin;
		else
			in[0] |= 1 << 4;
		if (fStart1P)
			in[0] &= ~(1 << 0), --fStart1P;
		else
			in[0] |= 1 << 0;
		if (fStart2P)
			in[0] &= ~(1 << 1), --fStart2P;
		else
			in[0] |= 1 << 1;

		// 連射処理
		if (fTurbo && (frame & 1) == 0)
			in[1] ^= 1 << 4;
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
			in[1] = in[1] & ~(1 << 3) | 1 << 2;
		else
			in[1] |= 1 << 3;
	}

	void right(bool fDown) {
		if (fDown)
			in[1] = in[1] & ~(1 << 0) | 1 << 1;
		else
			in[1] |= 1 << 0;
	}

	void down(bool fDown) {
		if (fDown)
			in[1] = in[1] & ~(1 << 2) | 1 << 3;
		else
			in[1] |= 1 << 2;
	}

	void left(bool fDown) {
		if (fDown)
			in[1] = in[1] & ~(1 << 1) | 1 << 0;
		else
			in[1] |= 1 << 1;
	}

	void triggerA(bool fDown) {
		if (fDown)
			in[1] &= ~(1 << 4);
		else
			in[1] |= 1 << 4;
	}

	void triggerB(bool fDown) {
		if (fDown)
			in[1] &= ~(1 << 5);
		else
			in[1] |= 1 << 5;
	}

	void triggerY(bool fDown) {
		if (!(fTurbo = fDown))
			in[1] |= 1 << 4;
	}

	void convertRGB() {
		for (int i = 0; i < 0x100; i++)
			rgb[i] = (RED[i] & 0xf) * 255 / 15		// Red
				| (GREEN[i] & 0xf) * 255 / 15 << 8	// Green
				| (BLUE[i] & 0xf) * 255 / 15 << 16	// Blue
				| 0xff000000;						// Alpha
	}

	void convertFG() {
		for (int i = 0; i < 256; i++)
			fgcolor[i] = FGCOLOR[i] & 0xf | 0x80;
		for (int p = 0, q = 0, i = 512; i != 0; q += 16, --i) {
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 16; k += 2)
					fg[p++] = FG[q + k + 1] >> (j + 4) & 1 | FG[q + k + 1] >> j << 1 & 2;
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 16; k += 2)
					fg[p++] = FG[q + k] >> (j + 4) & 1 | FG[q + k] >> j << 1 & 2;
		}
	}

	void convertBG() {
		for (int i = 0; i < 256; i++)
			bgcolor[i] = BGCOLOR[i] & 0xf;
		for (int p = 0, q = 0, i = 512; i != 0; q += 32, --i) {
			for (int j = 0; j < 8; j++)
				for (int k = 0; k < 16; k++)
					bg[p++] = BG[q + k + 0x8000 + 16] >> j & 1 | BG[q + k + 0x4000 + 16] >> j << 1 & 2 | BG[q + k + 16] >> j << 2 & 4;
			for (int j = 0; j < 8; j++)
				for (int k = 0; k < 16; k++)
					bg[p++] = BG[q + k + 0x8000] >> j & 1 | BG[q + k + 0x4000] >> j << 1 & 2 | BG[q + k] >> j << 2 & 4;
		}
	}

	void convertOBJ() {
		for (int i = 0; i < 256; i++)
			objcolor[i] = OBJCOLOR[i] & 0xf | 0x40;
		for (int p = 0, q = 0, i = 512; i != 0; q += 64, --i) {
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 32; k += 2)
					obj[p++] = OBJ[q + k + 33] >> (j + 4) & 1 | OBJ[q + k + 33] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 33] >> (j + 2) & 4 | OBJ[q + k + 0x8000 + 33] >> j << 3 & 8;
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 32; k += 2)
					obj[p++] = OBJ[q + k + 32] >> (j + 4) & 1 | OBJ[q + k + 32] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 32] >> (j + 2) & 4 | OBJ[q + k + 0x8000 + 32] >> j << 3 & 8;
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 32; k += 2)
					obj[p++] = OBJ[q + k + 1] >> (j + 4) & 1 | OBJ[q + k + 1] >> j << 1 & 2 | OBJ[q + k + 0x8000 + 1] >> (j + 2) & 4 | OBJ[q + k + 0x8000 + 1] >> j << 3 & 8;
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 32; k += 2)
					obj[p++] = OBJ[q + k] >> (j + 4) & 1 | OBJ[q + k] >> j << 1 & 2 | OBJ[q + k + 0x8000] >> (j + 2) & 4 | OBJ[q + k + 0x8000] >> j << 3 & 8;
		}
	}

	void makeBitmap(int *data) {
		frame++;

		// bg描画
		int p = 256 * 256 + 16 + (dwScroll & 0x0f) * 256;
		int k = dwScroll << 1 & 0x3e0 | 1;
		for (int i = 0; i < 17; k = k + 0x12 & 0x3ff, p -= 14 * 16 + 256 * 16, i++)
			for (int j = 0; j < 14; k++, p += 16, j++)
				xfer16x16x3(data, p, 0x900 + k);

		// obj描画
		for (int k = 0x7c, i = 32; i != 0; k -= 4, --i) {
			const int x = ram[k + 2];
			const int y = 256 - (ram[k + 3] | ram[k + 1] << 4 & 0x100) & 0x1ff;
			const int src = ram[k] & 0x7f | ram[k + 1] << 2 & 0x80 | ram[k] << 1 & 0x100 | ram[k + 1] << 9 & 0x1e00;
			switch (ram[k + 1] >> 6) {
			case 0:
				xfer16x16x4(data, x | y << 8, src);
				break;
			case 1:
				xfer16x16x4(data, x | y << 8, src);
				xfer16x16x4(data, x + 16 & 0xff | y << 8, src + 1);
				break;
			case 2:
			case 3:
				xfer16x16x4(data, x | y << 8, src);
				xfer16x16x4(data, x + 16 & 0xff | y << 8, src + 1);
				xfer16x16x4(data, x + 32 & 0xff | y << 8, src + 2);
				xfer16x16x4(data, x + 48 & 0xff | y << 8, src + 3);
				break;
			}
		}

		// fg描画
		p = 256 * 8 * 33 + 16;
		k = 0x140;
		for (int i = 0; i < 28; p += 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p -= 256 * 8, j++)
				xfer8x8(data, p, k);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = (ram[k] | ram[k + 0x400] << 1 & 0x100) << 6, idx = ram[k + 0x400] << 2 & 0xfc;
		int px;

		if ((px = fgcolor[idx | fg[q | 0x00]]) != 0x8f) data[p + 0x000] = px;
		if ((px = fgcolor[idx | fg[q | 0x01]]) != 0x8f) data[p + 0x001] = px;
		if ((px = fgcolor[idx | fg[q | 0x02]]) != 0x8f) data[p + 0x002] = px;
		if ((px = fgcolor[idx | fg[q | 0x03]]) != 0x8f) data[p + 0x003] = px;
		if ((px = fgcolor[idx | fg[q | 0x04]]) != 0x8f) data[p + 0x004] = px;
		if ((px = fgcolor[idx | fg[q | 0x05]]) != 0x8f) data[p + 0x005] = px;
		if ((px = fgcolor[idx | fg[q | 0x06]]) != 0x8f) data[p + 0x006] = px;
		if ((px = fgcolor[idx | fg[q | 0x07]]) != 0x8f) data[p + 0x007] = px;
		if ((px = fgcolor[idx | fg[q | 0x08]]) != 0x8f) data[p + 0x100] = px;
		if ((px = fgcolor[idx | fg[q | 0x09]]) != 0x8f) data[p + 0x101] = px;
		if ((px = fgcolor[idx | fg[q | 0x0a]]) != 0x8f) data[p + 0x102] = px;
		if ((px = fgcolor[idx | fg[q | 0x0b]]) != 0x8f) data[p + 0x103] = px;
		if ((px = fgcolor[idx | fg[q | 0x0c]]) != 0x8f) data[p + 0x104] = px;
		if ((px = fgcolor[idx | fg[q | 0x0d]]) != 0x8f) data[p + 0x105] = px;
		if ((px = fgcolor[idx | fg[q | 0x0e]]) != 0x8f) data[p + 0x106] = px;
		if ((px = fgcolor[idx | fg[q | 0x0f]]) != 0x8f) data[p + 0x107] = px;
		if ((px = fgcolor[idx | fg[q | 0x10]]) != 0x8f) data[p + 0x200] = px;
		if ((px = fgcolor[idx | fg[q | 0x11]]) != 0x8f) data[p + 0x201] = px;
		if ((px = fgcolor[idx | fg[q | 0x12]]) != 0x8f) data[p + 0x202] = px;
		if ((px = fgcolor[idx | fg[q | 0x13]]) != 0x8f) data[p + 0x203] = px;
		if ((px = fgcolor[idx | fg[q | 0x14]]) != 0x8f) data[p + 0x204] = px;
		if ((px = fgcolor[idx | fg[q | 0x15]]) != 0x8f) data[p + 0x205] = px;
		if ((px = fgcolor[idx | fg[q | 0x16]]) != 0x8f) data[p + 0x206] = px;
		if ((px = fgcolor[idx | fg[q | 0x17]]) != 0x8f) data[p + 0x207] = px;
		if ((px = fgcolor[idx | fg[q | 0x18]]) != 0x8f) data[p + 0x300] = px;
		if ((px = fgcolor[idx | fg[q | 0x19]]) != 0x8f) data[p + 0x301] = px;
		if ((px = fgcolor[idx | fg[q | 0x1a]]) != 0x8f) data[p + 0x302] = px;
		if ((px = fgcolor[idx | fg[q | 0x1b]]) != 0x8f) data[p + 0x303] = px;
		if ((px = fgcolor[idx | fg[q | 0x1c]]) != 0x8f) data[p + 0x304] = px;
		if ((px = fgcolor[idx | fg[q | 0x1d]]) != 0x8f) data[p + 0x305] = px;
		if ((px = fgcolor[idx | fg[q | 0x1e]]) != 0x8f) data[p + 0x306] = px;
		if ((px = fgcolor[idx | fg[q | 0x1f]]) != 0x8f) data[p + 0x307] = px;
		if ((px = fgcolor[idx | fg[q | 0x20]]) != 0x8f) data[p + 0x400] = px;
		if ((px = fgcolor[idx | fg[q | 0x21]]) != 0x8f) data[p + 0x401] = px;
		if ((px = fgcolor[idx | fg[q | 0x22]]) != 0x8f) data[p + 0x402] = px;
		if ((px = fgcolor[idx | fg[q | 0x23]]) != 0x8f) data[p + 0x403] = px;
		if ((px = fgcolor[idx | fg[q | 0x24]]) != 0x8f) data[p + 0x404] = px;
		if ((px = fgcolor[idx | fg[q | 0x25]]) != 0x8f) data[p + 0x405] = px;
		if ((px = fgcolor[idx | fg[q | 0x26]]) != 0x8f) data[p + 0x406] = px;
		if ((px = fgcolor[idx | fg[q | 0x27]]) != 0x8f) data[p + 0x407] = px;
		if ((px = fgcolor[idx | fg[q | 0x28]]) != 0x8f) data[p + 0x500] = px;
		if ((px = fgcolor[idx | fg[q | 0x29]]) != 0x8f) data[p + 0x501] = px;
		if ((px = fgcolor[idx | fg[q | 0x2a]]) != 0x8f) data[p + 0x502] = px;
		if ((px = fgcolor[idx | fg[q | 0x2b]]) != 0x8f) data[p + 0x503] = px;
		if ((px = fgcolor[idx | fg[q | 0x2c]]) != 0x8f) data[p + 0x504] = px;
		if ((px = fgcolor[idx | fg[q | 0x2d]]) != 0x8f) data[p + 0x505] = px;
		if ((px = fgcolor[idx | fg[q | 0x2e]]) != 0x8f) data[p + 0x506] = px;
		if ((px = fgcolor[idx | fg[q | 0x2f]]) != 0x8f) data[p + 0x507] = px;
		if ((px = fgcolor[idx | fg[q | 0x30]]) != 0x8f) data[p + 0x600] = px;
		if ((px = fgcolor[idx | fg[q | 0x31]]) != 0x8f) data[p + 0x601] = px;
		if ((px = fgcolor[idx | fg[q | 0x32]]) != 0x8f) data[p + 0x602] = px;
		if ((px = fgcolor[idx | fg[q | 0x33]]) != 0x8f) data[p + 0x603] = px;
		if ((px = fgcolor[idx | fg[q | 0x34]]) != 0x8f) data[p + 0x604] = px;
		if ((px = fgcolor[idx | fg[q | 0x35]]) != 0x8f) data[p + 0x605] = px;
		if ((px = fgcolor[idx | fg[q | 0x36]]) != 0x8f) data[p + 0x606] = px;
		if ((px = fgcolor[idx | fg[q | 0x37]]) != 0x8f) data[p + 0x607] = px;
		if ((px = fgcolor[idx | fg[q | 0x38]]) != 0x8f) data[p + 0x700] = px;
		if ((px = fgcolor[idx | fg[q | 0x39]]) != 0x8f) data[p + 0x701] = px;
		if ((px = fgcolor[idx | fg[q | 0x3a]]) != 0x8f) data[p + 0x702] = px;
		if ((px = fgcolor[idx | fg[q | 0x3b]]) != 0x8f) data[p + 0x703] = px;
		if ((px = fgcolor[idx | fg[q | 0x3c]]) != 0x8f) data[p + 0x704] = px;
		if ((px = fgcolor[idx | fg[q | 0x3d]]) != 0x8f) data[p + 0x705] = px;
		if ((px = fgcolor[idx | fg[q | 0x3e]]) != 0x8f) data[p + 0x706] = px;
		if ((px = fgcolor[idx | fg[q | 0x3f]]) != 0x8f) data[p + 0x707] = px;
	}

	void xfer16x16x3(int *data, int p, int k) {
		const int idx = ram[k + 0x10] << 3 & 0xf8;
		int i, j, q = (ram[k] | ram[k + 0x10] << 1 & 0x100) << 8;

		switch (ram[k + 0x10] >> 5 & 3) {
		case 0:
			for (i = 16; i != 0; p += 256 - 16, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | bgcolor[idx | bg[q++]];
			break;
		case 1:
			for (q += 256 - 16, i = 16; i != 0; p += 256 - 16, q -= 32, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | bgcolor[idx | bg[q++]];
			break;
		case 2:
			for (q += 16, i = 16; i != 0; p += 256 - 16, q += 32, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | bgcolor[idx | bg[--q]];
			break;
		case 3:
			for (q += 256, i = 16; i != 0; p += 256 - 16, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | bgcolor[idx | bg[--q]];
			break;
		}
	}

	void xfer16x16x4(int *data, int dst, int src) {
		const int idx = src >> 5 & 0xf0;
		int px, i, j;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		for (src = src << 8 & 0x1ff00, i = 16; i != 0; dst += 256 - 16, --i)
			for (j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[src++]]) != 0x4f)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(1500000, rate, 4);
		sound1 = new AY_3_8910(1500000, rate, 4);
		Z80::init();
	}
};

#endif //_1942_H
