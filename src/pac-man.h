/*
 *
 *	Pac-Man
 *
 */

#ifndef PAC_MAN_H
#define PAC_MAN_H

#include "z80.h"
#include "pac-man_sound.h"

enum {
	BONUS_10000, BONUS_15000, BONUS_20000, BONUS_NONE,
};

struct PacMan {
	static unsigned char BG[], COLOR[], OBJ[], RGB[], PRG[], SND[];

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
	int nPacman = 3;
	int nBonus = BONUS_10000;

	bool fInterruptEnable = false;
	bool fSoundEnable = false;
	uint8_t ram[0xd00] = {};
	uint8_t in[4] = {0xff, 0xff, 0xc9, 0x00};
	int vector = 0;

	uint8_t bg[0x4000] = {};
	uint8_t obj[0x4000] = {};
	uint8_t color[0x100] = {};
	int rgb[0x20] = {};

	Z80 cpu;

	PacMan() {
		// CPU周りの初期化
		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x3f, 0x80))
				cpu.memorymap[page].base = PRG + (page & 0x3f) * 0x100;
			else if (range(page, 0x40, 0x47, 0xa0)) {
				cpu.memorymap[page].base = ram + (page & 7) * 0x100;
				cpu.memorymap[page].write = nullptr;
			}
			else if (range(page, 0x48, 0x48, 0xa3))
				cpu.memorymap[page].read = [&](int addr) { return 0xbf; };
			else if (range(page, 0x4c, 0x4f, 0xa0)) {
				cpu.memorymap[page].base = ram + (8 | page & 3) * 0x100;
				cpu.memorymap[page].write = nullptr;
			}
			else if (range(page, 0x50, 0x50, 0xaf)) {
				cpu.memorymap[page].read = [&](int addr) -> int { return in[addr >> 6 & 3]; };
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr >> 4 & 0xf) {
					case 0:
						switch (addr & 7) {
						case 0:
							return void(fInterruptEnable = (data & 1) != 0);
						case 1:
							return void(fSoundEnable = (data & 1) != 0);
						}
						return;
					case 4:
					case 5:
						return sound0->write(addr, data);
					case 6:
						return void(ram[0xc60 | addr & 0xf] = data);
					}
				};
			}
		for (int page = 0; page < 0x100; page++)
			cpu.iomap[page].write = [&](int addr, int data) { (addr & 0xff) == 0 && (vector = data); };

		// Videoの初期化
		convertRGB();
		convertBG();
		convertOBJ();
	}

	PacMan *execute() {
		sound0->mute(!fSoundEnable);
		if (fInterruptEnable)
			cpu.interrupt(vector);
		cpu.execute(0x2000);
		if (cpu.fActive && !cpu.fSuspend && cpu.iff == 3)
			cpu.execute(0x1400);
		return this;
	}

	void reset() {
		fReset = true;
	}

	PacMan *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nPacman) {
			case 1:
				in[2] &= ~0xc;
				break;
			case 2:
				in[2] = in[2] & ~0xc | 4;
				break;
			case 3:
				in[2] = in[2] & ~0xc | 8;
				break;
			case 5:
				in[2] |= 0xc;
				break;
			}
			switch (nBonus) {
			case BONUS_10000:
				in[2] &= ~0x30;
				break;
			case BONUS_15000:
				in[2] = in[2] & ~0x30 | 0x10;
				break;
			case BONUS_20000:
				in[2] = in[2] & ~0x30 | 0x20;
				break;
			case BONUS_NONE:
				in[2] |= 0x30;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[1] &= 0xef;
		else
			in[1] |= 0x10;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
		}
		return this;
	}

	PacMan *updateInput() {
		// クレジット/スタートボタン処理
		if (fCoin)
			in[0] &= ~(1 << 5), --fCoin;
		else
			in[0] |= 1 << 5;
		if (fStart1P)
			in[1] &= ~(1 << 5), --fStart1P;
		else
			in[1] |= 1 << 5;
		if (fStart2P)
			in[1] &= ~(1 << 6), --fStart2P;
		else
			in[1] |= 1 << 6;
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
			in[0] = in[0] & ~(1 << 0) | 1 << 3, in[1] = in[1] & ~(1 << 0) | 1 << 3;
		else
			in[0] |= 1 << 0, in[1] |= 1 << 0;
	}

	void right(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 2) | 1 << 1, in[1] = in[1] & ~(1 << 2) | 1 << 1;
		else
			in[0] |= 1 << 2, in[1] |= 1 << 2;
	}

	void down(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 3) | 1 << 0, in[1] = in[1] & ~(1 << 3) | 1 << 0;
		else
			in[0] |= 1 << 3, in[1] |= 1 << 3;
	}

	void left(bool fDown) {
		if (fDown)
			in[0] = in[0] & ~(1 << 1) | 1 << 2, in[1] = in[1] & ~(1 << 1) | 1 << 2;
		else
			in[0] |= 1 << 1, in[1] |= 1 << 1;
	}

	void triggerA(bool fDown) {
	}

	void triggerB(bool fDown) {
	}

	void convertRGB() {
		for (int i = 0; i < 0x20; i++)
			rgb[i] = (RGB[i] & 7) * 255 / 7			// Red
				| (RGB[i] >> 3 & 7) * 255 / 7 << 8	// Green
				| (RGB[i] >> 6) * 255 / 3 << 16		// Blue
				| 0xff000000;						// Alpha
	}

	void convertBG() {
		for (int i = 0; i < 0x100; i++)
			color[i] = COLOR[i] & 0xf;
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
		for (int p = 0, q = 0, i = 64; i != 0; q += 64, --i) {
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
			for (int j = 3; j >= 0; --j) {
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k + 32] >> j & 1 | OBJ[q + k + 32] >> (j + 3) & 2;
				for (int k = 7; k >= 0; --k)
					obj[p++] = OBJ[q + k] >> j & 1 | OBJ[q + k] >> (j + 3) & 2;
			}
		}
	}

	void makeBitmap(int *data) {
		// bg描画
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

		// obj描画
		for (int k = 0x0bfe, i = 7; i != 0; k -= 2, --i) {
			const int x = (~ram[k + 0x70] - (i < 3)) & 0xff;
			const int y = (-ram[k + 0x71] & 0xff) + 32;
			const int src = ram[k] | ram[k + 1] << 8;
			switch (ram[k] & 3) {
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

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				data[p] = rgb[data[p]];
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0x7c;

		data[p + 0x000] = color[idx | bg[q | 0x00]];
		data[p + 0x001] = color[idx | bg[q | 0x01]];
		data[p + 0x002] = color[idx | bg[q | 0x02]];
		data[p + 0x003] = color[idx | bg[q | 0x03]];
		data[p + 0x004] = color[idx | bg[q | 0x04]];
		data[p + 0x005] = color[idx | bg[q | 0x05]];
		data[p + 0x006] = color[idx | bg[q | 0x06]];
		data[p + 0x007] = color[idx | bg[q | 0x07]];
		data[p + 0x100] = color[idx | bg[q | 0x08]];
		data[p + 0x101] = color[idx | bg[q | 0x09]];
		data[p + 0x102] = color[idx | bg[q | 0x0a]];
		data[p + 0x103] = color[idx | bg[q | 0x0b]];
		data[p + 0x104] = color[idx | bg[q | 0x0c]];
		data[p + 0x105] = color[idx | bg[q | 0x0d]];
		data[p + 0x106] = color[idx | bg[q | 0x0e]];
		data[p + 0x107] = color[idx | bg[q | 0x0f]];
		data[p + 0x200] = color[idx | bg[q | 0x10]];
		data[p + 0x201] = color[idx | bg[q | 0x11]];
		data[p + 0x202] = color[idx | bg[q | 0x12]];
		data[p + 0x203] = color[idx | bg[q | 0x13]];
		data[p + 0x204] = color[idx | bg[q | 0x14]];
		data[p + 0x205] = color[idx | bg[q | 0x15]];
		data[p + 0x206] = color[idx | bg[q | 0x16]];
		data[p + 0x207] = color[idx | bg[q | 0x17]];
		data[p + 0x300] = color[idx | bg[q | 0x18]];
		data[p + 0x301] = color[idx | bg[q | 0x19]];
		data[p + 0x302] = color[idx | bg[q | 0x1a]];
		data[p + 0x303] = color[idx | bg[q | 0x1b]];
		data[p + 0x304] = color[idx | bg[q | 0x1c]];
		data[p + 0x305] = color[idx | bg[q | 0x1d]];
		data[p + 0x306] = color[idx | bg[q | 0x1e]];
		data[p + 0x307] = color[idx | bg[q | 0x1f]];
		data[p + 0x400] = color[idx | bg[q | 0x20]];
		data[p + 0x401] = color[idx | bg[q | 0x21]];
		data[p + 0x402] = color[idx | bg[q | 0x22]];
		data[p + 0x403] = color[idx | bg[q | 0x23]];
		data[p + 0x404] = color[idx | bg[q | 0x24]];
		data[p + 0x405] = color[idx | bg[q | 0x25]];
		data[p + 0x406] = color[idx | bg[q | 0x26]];
		data[p + 0x407] = color[idx | bg[q | 0x27]];
		data[p + 0x500] = color[idx | bg[q | 0x28]];
		data[p + 0x501] = color[idx | bg[q | 0x29]];
		data[p + 0x502] = color[idx | bg[q | 0x2a]];
		data[p + 0x503] = color[idx | bg[q | 0x2b]];
		data[p + 0x504] = color[idx | bg[q | 0x2c]];
		data[p + 0x505] = color[idx | bg[q | 0x2d]];
		data[p + 0x506] = color[idx | bg[q | 0x2e]];
		data[p + 0x507] = color[idx | bg[q | 0x2f]];
		data[p + 0x600] = color[idx | bg[q | 0x30]];
		data[p + 0x601] = color[idx | bg[q | 0x31]];
		data[p + 0x602] = color[idx | bg[q | 0x32]];
		data[p + 0x603] = color[idx | bg[q | 0x33]];
		data[p + 0x604] = color[idx | bg[q | 0x34]];
		data[p + 0x605] = color[idx | bg[q | 0x35]];
		data[p + 0x606] = color[idx | bg[q | 0x36]];
		data[p + 0x607] = color[idx | bg[q | 0x37]];
		data[p + 0x700] = color[idx | bg[q | 0x38]];
		data[p + 0x701] = color[idx | bg[q | 0x39]];
		data[p + 0x702] = color[idx | bg[q | 0x3a]];
		data[p + 0x703] = color[idx | bg[q | 0x3b]];
		data[p + 0x704] = color[idx | bg[q | 0x3c]];
		data[p + 0x705] = color[idx | bg[q | 0x3d]];
		data[p + 0x706] = color[idx | bg[q | 0x3e]];
		data[p + 0x707] = color[idx | bg[q | 0x3f]];
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = src << 6 & 0x3f00;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
		}
		else {
			src = src << 6 & 0x3f00;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
		}
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 256 - 16;
			for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
		}
		else {
			src = (src << 6 & 0x3f00) + 256 - 16;
			for (int i = h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[src++]]) != 0)
						data[dst] = px;
		}
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 16;
			for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
		}
		else {
			src = (src << 6 & 0x3f00) + 16;
			for (int i = h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
		}
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 256;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
		}
		else {
			src = (src << 6 & 0x3f00) + 256;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = color[idx | obj[--src]]) != 0)
						data[dst] = px;
		}
	}

	static void init(int rate) {
		sound0 = new PacManSound(SND, rate);
		Z80::init();
	}
};

#endif //PAC_MAN_H
