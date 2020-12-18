/*
 *
 *	Pengo
 *
 */

#ifndef PENGO_H
#define PENGO_H

#include <array>
#include <vector>
#include "z80.h"
#include "pac-man_sound.h"
#include "utils.h"
using namespace std;

enum {
	RANK_EASY, RANK_MEDIUM, RANK_HARD, RANK_HARDEST,
};

struct Pengo {
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
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nLife = 3;
	int nBonus = 30000;
	int nRank = RANK_MEDIUM;

	bool fInterruptEnable = false;
	bool fSoundEnable = false;
	uint8_t ram[0x1100] = {};
	uint8_t in[4] = {0xcc, 0xb0, 0xff, 0xff};

	array<uint8_t, 0x8000> bg;
	array<uint8_t, 0x8000> obj;
	array<int, 0x20> rgb;

	Z80 cpu;

	Pengo() {
		// CPU周りの初期化
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[i].base = PRG + i * 0x100;
		for (int i = 0; i < 0x10; i++) {
			cpu.memorymap[0x80 + i].base = ram + i * 0x100;
			cpu.memorymap[0x80 + i].write = nullptr;
		}
		cpu.memorymap[0x90].read = [&](int addr) -> int { return in[addr >> 6 & 3]; };
		cpu.memorymap[0x90].write = [&](int addr, int data) {
			switch (addr >> 4 & 0xf) {
			case 0:
			case 1:
				return sound0->write(addr, data);
			case 2:
				return void(ram[0x1020 | addr & 0xf] = data);
			case 4:
				switch (addr & 7) {
				case 0:
					return void(fInterruptEnable = (data & 1) != 0);
				case 1:
					return void(fSoundEnable = (data & 1) != 0);
				default:
					return void(ram[0x1040 | addr & 7] = data & 1);
				}
			}
		};

		// Videoの初期化
		bg.fill(3), obj.fill(3);
		convertGFX(&bg[0], BG, 512, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&obj[0], OBJ, 128, {rseq8(256, 8), rseq8(0, 8)}, {seq4(64, 1), seq4(128, 1), seq4(192, 1), seq4(0, 1)}, {0, 4}, 64);
		for (int i = 0; i < 0x20; i++)
			rgb[i] = 0xff000000 | (RGB[i] >> 6) * 255 / 3 << 16 | (RGB[i] >> 3 & 7) * 255 / 7 << 8 | (RGB[i] & 7) * 255 / 7;
	}

	Pengo *execute() {
		sound0->mute(!fSoundEnable);
		fInterruptEnable && cpu.interrupt(), cpu.execute(0x1600);
		return this;
	}

	void reset() {
		fReset = true;
	}

	Pengo *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 2:
				in[1] |= 0x18;
				break;
			case 3:
				in[1] = in[1] & ~0x18 | 0x10;
				break;
			case 4:
				in[1] = in[1] & ~0x18 | 8;
				break;
			case 5:
				in[1] &= ~0x18;
				break;
			}
			switch (nBonus) {
			case 30000:
				in[1] &= ~1;
				break;
			case 50000:
				in[1] |= 1;
				break;
			}
			switch (nRank) {
			case RANK_EASY:
				in[1] |= 0xc0;
				break;
			case RANK_MEDIUM:
				in[1] = in[1] & ~0xc0 | 0x80;
				break;
			case RANK_HARD:
				in[1] = in[1] & ~0xc0 | 0x40;
				break;
			case RANK_HARDEST:
				in[1] &= ~0xc0;
				break;
			}
			fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
		}
		return this;
	}

	Pengo *updateInput() {
		in[3] = in[3] & ~(1 << 4) | !fCoin << 4;
		in[2] = in[2] & ~0x60 | !fStart1P << 5 | !fStart2P << 6;
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
		in[3] = in[3] & ~(1 << 0) | fDown << 1 | !fDown << 0;
		in[2] = in[2] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void right(bool fDown) {
		in[3] = in[3] & ~(1 << 3) | fDown << 2 | !fDown << 3;
		in[2] = in[2] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void down(bool fDown) {
		in[3] = in[3] & ~(1 << 1) | fDown << 0 | !fDown << 1;
		in[2] = in[2] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void left(bool fDown) {
		in[3] = in[3] & ~(1 << 2) | fDown << 3 | !fDown << 2;
		in[2] = in[2] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void triggerA(bool fDown) {
		in[3] = in[3] & ~(1 << 7) | !fDown << 7;
		in[2] = in[2] & ~(1 << 7) | !fDown << 7;
	}

	void makeBitmap(int *data) {
		// bg描画
		int p = 256 * 8 * 4 + 232;
		for (int k = 0x40, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k);
		p = 256 * 8 * 36 + 232;
		for (int k = 2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 37 + 232;
		for (int k = 0x22, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 2 + 232;
		for (int k = 0x3c2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);
		p = 256 * 8 * 3 + 232;
		for (int k = 0x3e2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k);

		// obj描画
		for (int k = 0x0ffe, i = 7; i != 0; k -= 2, --i) {
			const int x = (~ram[k + 0x30] - (i < 3)) & 0xff;
			const int y = (-ram[k + 0x31] & 0xff) + 32;
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
				data[p] = rgb[data[p] | ram[0x1042] << 4];
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = ram[k] << 6 | ram[0x1047] << 14, idx = ram[k + 0x400] << 2 & 0x7c | ram[0x1046] << 7;

		data[p + 0x000] = COLOR[idx | bg[q | 0x00]];
		data[p + 0x001] = COLOR[idx | bg[q | 0x01]];
		data[p + 0x002] = COLOR[idx | bg[q | 0x02]];
		data[p + 0x003] = COLOR[idx | bg[q | 0x03]];
		data[p + 0x004] = COLOR[idx | bg[q | 0x04]];
		data[p + 0x005] = COLOR[idx | bg[q | 0x05]];
		data[p + 0x006] = COLOR[idx | bg[q | 0x06]];
		data[p + 0x007] = COLOR[idx | bg[q | 0x07]];
		data[p + 0x100] = COLOR[idx | bg[q | 0x08]];
		data[p + 0x101] = COLOR[idx | bg[q | 0x09]];
		data[p + 0x102] = COLOR[idx | bg[q | 0x0a]];
		data[p + 0x103] = COLOR[idx | bg[q | 0x0b]];
		data[p + 0x104] = COLOR[idx | bg[q | 0x0c]];
		data[p + 0x105] = COLOR[idx | bg[q | 0x0d]];
		data[p + 0x106] = COLOR[idx | bg[q | 0x0e]];
		data[p + 0x107] = COLOR[idx | bg[q | 0x0f]];
		data[p + 0x200] = COLOR[idx | bg[q | 0x10]];
		data[p + 0x201] = COLOR[idx | bg[q | 0x11]];
		data[p + 0x202] = COLOR[idx | bg[q | 0x12]];
		data[p + 0x203] = COLOR[idx | bg[q | 0x13]];
		data[p + 0x204] = COLOR[idx | bg[q | 0x14]];
		data[p + 0x205] = COLOR[idx | bg[q | 0x15]];
		data[p + 0x206] = COLOR[idx | bg[q | 0x16]];
		data[p + 0x207] = COLOR[idx | bg[q | 0x17]];
		data[p + 0x300] = COLOR[idx | bg[q | 0x18]];
		data[p + 0x301] = COLOR[idx | bg[q | 0x19]];
		data[p + 0x302] = COLOR[idx | bg[q | 0x1a]];
		data[p + 0x303] = COLOR[idx | bg[q | 0x1b]];
		data[p + 0x304] = COLOR[idx | bg[q | 0x1c]];
		data[p + 0x305] = COLOR[idx | bg[q | 0x1d]];
		data[p + 0x306] = COLOR[idx | bg[q | 0x1e]];
		data[p + 0x307] = COLOR[idx | bg[q | 0x1f]];
		data[p + 0x400] = COLOR[idx | bg[q | 0x20]];
		data[p + 0x401] = COLOR[idx | bg[q | 0x21]];
		data[p + 0x402] = COLOR[idx | bg[q | 0x22]];
		data[p + 0x403] = COLOR[idx | bg[q | 0x23]];
		data[p + 0x404] = COLOR[idx | bg[q | 0x24]];
		data[p + 0x405] = COLOR[idx | bg[q | 0x25]];
		data[p + 0x406] = COLOR[idx | bg[q | 0x26]];
		data[p + 0x407] = COLOR[idx | bg[q | 0x27]];
		data[p + 0x500] = COLOR[idx | bg[q | 0x28]];
		data[p + 0x501] = COLOR[idx | bg[q | 0x29]];
		data[p + 0x502] = COLOR[idx | bg[q | 0x2a]];
		data[p + 0x503] = COLOR[idx | bg[q | 0x2b]];
		data[p + 0x504] = COLOR[idx | bg[q | 0x2c]];
		data[p + 0x505] = COLOR[idx | bg[q | 0x2d]];
		data[p + 0x506] = COLOR[idx | bg[q | 0x2e]];
		data[p + 0x507] = COLOR[idx | bg[q | 0x2f]];
		data[p + 0x600] = COLOR[idx | bg[q | 0x30]];
		data[p + 0x601] = COLOR[idx | bg[q | 0x31]];
		data[p + 0x602] = COLOR[idx | bg[q | 0x32]];
		data[p + 0x603] = COLOR[idx | bg[q | 0x33]];
		data[p + 0x604] = COLOR[idx | bg[q | 0x34]];
		data[p + 0x605] = COLOR[idx | bg[q | 0x35]];
		data[p + 0x606] = COLOR[idx | bg[q | 0x36]];
		data[p + 0x607] = COLOR[idx | bg[q | 0x37]];
		data[p + 0x700] = COLOR[idx | bg[q | 0x38]];
		data[p + 0x701] = COLOR[idx | bg[q | 0x39]];
		data[p + 0x702] = COLOR[idx | bg[q | 0x3a]];
		data[p + 0x703] = COLOR[idx | bg[q | 0x3b]];
		data[p + 0x704] = COLOR[idx | bg[q | 0x3c]];
		data[p + 0x705] = COLOR[idx | bg[q | 0x3d]];
		data[p + 0x706] = COLOR[idx | bg[q | 0x3e]];
		data[p + 0x707] = COLOR[idx | bg[q | 0x3f]];
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c | ram[0x1046] << 7, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = src << 6 & 0x3f00 | ram[0x1047] << 14;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		} else {
			src = src << 6 & 0x3f00 | ram[0x1047] << 14;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		}
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c | ram[0x1046] << 7, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 256 - 16;
			for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 256 - 16;
			for (int i = h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		}
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c | ram[0x1046] << 7, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 16;
			for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 16;
			for (int i = h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		}
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0x7c | ram[0x1046] << 7, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 256;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00 | ram[0x1047] << 14) + 256;
			for (int i = h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
			dst -= 0x10000;
			for (int i = 16 - h; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		}
	}

	static void init(int rate) {
		sound0 = new PacManSound(SND, rate);
		Z80::init();
	}
};

#endif //PENGO_H
