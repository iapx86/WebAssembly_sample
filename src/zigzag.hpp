/*
 *
 *	Zig Zag
 *
 */

#ifndef ZIG_ZAG_H
#define ZIG_ZAG_H

#include <cmath>
#include <array>
#include "z80.hpp"
#include "ay-3-8910.hpp"
#include "utils.hpp"
using namespace std;

enum {
	BONUS_10000_60000, BONUS_20000_60000, BONUS_30000_60000, BONUS_40000_60000,
};

struct ZigZag {
	static array<uint8_t, 0x1000> BG, OBJ;
	static array<uint8_t, 0x20> RGB;
	static array<uint8_t, 0x4000> PRG;

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
	int nBonus = BONUS_10000_60000;

	bool fInterruptEnable = false;
	int bank = 0x20;
	array<uint8_t, 0x900> ram = {};
	array<uint8_t, 3> in = {0, 0, 2};
	struct {
		int latch = 0;
		int addr = 0;
	} psg;

	struct {
		int x = 0;
		int y = 0;
		int color = 0;
	} stars[1024];
	bool fStarEnable = false;
	bool fStarMove = false;
	array<uint8_t, 0x4000> bg;
	array<uint8_t, 0x4000> obj;
	array<int, 0x80> rgb = {};
	array<int, width * height> bitmap;
	bool updated = false;

	Z80 cpu;
	Timer<int> timer;

	ZigZag() : cpu(18432000 / 6), timer(60) {
		// CPU周りの初期化
		auto range = [](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x3f))
				cpu.memorymap[page].base = &PRG[(page & 0x3f) << 8];
			else if (range(page, 0x40, 0x43, 0x04)) {
				cpu.memorymap[page].base = &ram[(page & 3) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x48, 0x48, 0x07))
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr & 0x0300) {
					case 0x0000:
						if (~addr & 1)
							return;
						if (~addr & 2)
							return sound0->write(psg.addr, psg.latch);
						return void(psg.addr = psg.latch);
					case 0x0100:
						return void(psg.latch = addr);
					}
				};
			else if (range(page, 0x50, 0x53, 0x04)) {
				cpu.memorymap[page].base = &ram[(4 | page & 3) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x58, 0x58, 0x07)) {
				cpu.memorymap[page].base = &ram[0x800];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x60, 0x60, 0x07))
				cpu.memorymap[page].read = [&](int addr) -> int { return in[0]; };
			else if (range(page, 0x68, 0x68, 0x07))
				cpu.memorymap[page].read = [&](int addr) -> int { return in[1]; };
			else if (range(page, 0x70, 0x70, 0x07)) {
				cpu.memorymap[page].read = [&](int addr) -> int { return in[2]; };
				cpu.memorymap[page].write = [&](int addr, int data) {
					int _bank;
					switch (addr & 7) {
					case 1:
						return void(fInterruptEnable = (data & 1) != 0);
					case 2:
						_bank = data << 4 & 0x10 | 0x20;
						if (_bank == bank)
							return;
						for (int i = 0; i < 0x10; i++) {
							cpu.memorymap[0x20 + i].base = &PRG[_bank + i << 8];
							cpu.memorymap[0x30 + i].base = &PRG[(_bank ^ 0x10) + i << 8];
						}
						return void(bank = _bank);
					case 4:
						return void(fStarEnable = (data & 1) != 0);
					}
				};
			}

		// Videoの初期化
		bg.fill(3), obj.fill(3), bitmap.fill(0xff000000);
		convertGFX(&bg[0], &BG[0], 256, {rseq8(0, 8)}, {seq8(0, 1)}, {0, 0x4000}, 8);
		convertGFX(&obj[0], &OBJ[0], 64, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x4000}, 32);
		for (int i = 0; i < 0x20; i++)
			rgb[i] = 0xff000000 | (RGB[i] >> 6) * 255 / 3 << 16 | (RGB[i] >> 3 & 7) * 255 / 7 << 8 | (RGB[i] & 7) * 255 / 7;
		const int starColors[4] = {0xd0, 0x70, 0x40, 0x00};
		for (int i = 0; i < 0x40; i++)
			rgb[0x40 | i] = 0xff000000 | starColors[i >> 4 & 3] << 16 | starColors[i >> 2 & 3] << 8 | starColors[i & 3];
		initializeStar();
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			timer.execute(tick_rate, [&]() { moveStars(), update(), fInterruptEnable && cpu.non_maskable_interrupt(); });
			sound0->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	ZigZag *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[2] &= ~1;
				break;
			case 4:
				in[2] |= 1;
				break;
			}
			switch (nBonus) {
			case BONUS_10000_60000:
				in[2] &= ~0x0c;
				break;
			case BONUS_20000_60000:
				in[2] = in[2] & ~0xc | 4;
				break;
			case BONUS_30000_60000:
				in[2] = in[2] & ~0xc | 8;
				break;
			case BONUS_40000_60000:
				in[2] |= 0x0c;
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
		}
		return this;
	}

	ZigZag *updateInput() {
		in[0] = in[0] & ~(1 << 0) | (fCoin != 0) << 0;
		in[1] = in[1] & ~3 | (fStart1P != 0) << 0 | (fStart2P != 0) << 1;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		return this;
	}

	void coin(bool fDown) {
		fDown && (fCoin = 2);
	}

	void start1P(bool fDown) {
		fDown && (fStart1P = 2);
	}

	void start2P(bool fDown) {
		fDown && (fStart2P = 2);
	}

	void up(bool fDown) {
		in[0] = in[0] & ~(1 << 5 | fDown << 6) | fDown << 5;
	}

	void right(bool fDown) {
		in[0] = in[0] & ~(1 << 3 | fDown << 2) | fDown << 3;
	}

	void down(bool fDown) {
		in[0] = in[0] & ~(1 << 6 | fDown << 5) | fDown << 6;
	}

	void left(bool fDown) {
		in[0] = in[0] & ~(1 << 2 | fDown << 3) | fDown << 2;
	}

	void triggerA(bool fDown) {
		in[0] = in[0] & ~(1 << 4) | fDown << 4;
	}

	void initializeStar() {
		int color;

		for (int sr = 0, i = 0, x = 255; x >= 0; --x) {
			for (int y = 0; y < 256; y++) {
				const int cy = sr >> 4 ^ ~sr >> 16;
				sr = cy & 1 | sr << 1;
				if ((sr & 0x100ff) == 0xff && (color = sr >> 8 & 0x3f) && color != 0x3f) {
					stars[i].x = x & 0xff;
					stars[i].y = y;
					stars[i].color = color;
					if (++i >= 1024)
						return;
				}
			}
		}
	}

	void moveStars() {
		if (fStarEnable && (fStarMove = !fStarMove))
			for (int i = 0; i < 256 && stars[i].color; i++)
				if (++stars[i].y >= 0x100) {
					stars[i].y &= 0xff;
					stars[i].x = stars[i].x - 1 & 0xff;
				}
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// bg描画
		int p = 256 * 32;
		for (int k = 0x7e2, i = 2; i < 32; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0x800 + i * 2];
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(bitmap.data(), p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// obj描画
		for (int k = 0x840, i = 16; i != 0; k += 4, --i) {
			const int x = ram[k], y = ram[k + 3] + 16;
			const int src = ram[k + 1] & 0x3f | ram[k + 2] << 6;
			switch (ram[k + 1] & 0xc0) {
			case 0x00: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src);
				break;
			case 0x40: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src);
				break;
			case 0x80: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src);
				break;
			case 0xc0: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src);
				break;
			}
		}

		// bg描画
		p = 256 * 16;
		for (int k = 0x7e0, i = 0; i < 2; p += 256 * 8, k += 0x401, i++) {
			int dwScroll = ram[0x800 + i * 2];
			for (int j = 0; j < 32; k -= 0x20, j++) {
				xfer8x8(bitmap.data(), p + dwScroll, k, i);
				dwScroll = dwScroll + 8 & 0xff;
			}
		}

		// star描画
		if (fStarEnable) {
			p = 256 * 16;
			for (int i = 0; i < 256; i++) {
				const int px = stars[i].color;
				if (!px)
					break;
				const int x = stars[i].x, y = stars[i].y;
				if (x & 1 && ~y & 8 && !(bitmap[p + (x | y << 8)] & 3))
					bitmap[p + (x | y << 8)] = 0x40 | px;
				else if (~x & 1 && y & 8 && !(bitmap[p + (x | y << 8)] & 3))
					bitmap[p + (x | y << 8)] = 0x40 | px;
			}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void xfer8x8(int *data, int p, int k, int i) {
		const int q = ram[k] << 6, idx = ram[0x801 + i * 2] << 2 & 0x1c;

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
				if ((px = obj[src++]))
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
				if ((px = obj[src++]))
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
				if ((px = obj[--src]))
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
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(18432000 / 6);
		Z80::init();
	}
};

#endif //ZIG_ZAG_H
