/*
 *
 *	Crush Roller
 *
 */

#ifndef CRUSH_ROLLER_H
#define CRUSH_ROLLER_H

#include <array>
#include "z80.h"
#include "pac-man_sound.h"
#include "utils.h"
using namespace std;

struct CrushRoller {
	static array<uint8_t, 0x1000> BG;
	static array<uint8_t, 0x100> COLOR;
	static array<uint8_t, 0x1000> OBJ;
	static array<uint8_t, 0x20> RGB;
	static array<uint8_t, 0x4000> PRG;
	static array<uint8_t, 0x100> SND;

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
	int nBrush = 3;

	bool fInterruptEnable = false;
	array<uint8_t, 0xd00> ram = {};
	array<uint8_t, 3> in = {0xef, 0x6f, 0x31};
	int intvec = 0;
	bool fProtectEnable = false;
	int protect_count = 0;
	int protect_index = 0;

	array<uint8_t, 0x4000> bg;
	array<uint8_t, 0x4000> obj;
	array<int, 0x20> rgb;
	array<int, width * height> bitmap;
	bool updated = false;

	Z80 cpu;
	Timer<int> timer;

	CrushRoller() : cpu(18432000 / 6), timer(60) {
		// CPU周りの初期化
		auto range = [&](int page, int start, int end, int mirror = 0) { return (page & ~mirror) >= start && (page & ~mirror) <= end; };

		for (int page = 0; page < 0x100; page++)
			if (range(page, 0, 0x3f, 0x80))
				cpu.memorymap[page].base = &PRG[(page & 0x3f) << 8];
			else if (range(page, 0x40, 0x47, 0xa0)) {
				cpu.memorymap[page].base = &ram[(page & 7) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x48, 0x48, 0xa3))
				cpu.memorymap[page].read = [&](int addr) { return 0xbf; };
			else if (range(page, 0x4c, 0x4f, 0xa0)) {
				cpu.memorymap[page].base = &ram[(8 | page & 3) << 8];
				cpu.memorymap[page].write = nullptr;
			} else if (range(page, 0x50, 0x50, 0xaf)) {
				cpu.memorymap[page].read = [&](int addr) -> int {
					switch (addr >> 6 & 3) {
					case 0:
						return in[0];
					case 1:
						return in[1];
					case 2:
						if (fProtectEnable)
							return in[2] & ~0xc0 | array<uint8_t, 0x1e>{0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00,
								0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40}[protect_index];
						switch (addr & 0x3f) {
						case 0x01:
						case 0x04:
							return in[2] & ~0xc0 | 0x40;
						case 0x05:
						case 0x0e:
						case 0x10:
							return in[2] & ~0xc0 | 0xc0;
						default:
							return in[2] & ~0xc0;
						}
					case 3:
						if (fProtectEnable)
							return array<uint8_t, 0x1e>{0x1f, 0x3f, 0x2f, 0x2f, 0x0f, 0x0f, 0x0f, 0x3f, 0x0f, 0x0f, 0x1c, 0x3c, 0x2c, 0x2c, 0x0c,
								0x0c, 0x0c, 0x3c, 0x0c, 0x0c, 0x11, 0x31, 0x21, 0x21, 0x01, 0x01, 0x01, 0x31, 0x01, 0x01}[protect_index];
						switch (addr & 0x3f) {
						case 0x00:
							return 0x1f;
						case 0x09:
							return 0x30;
						case 0x0c:
							return 0x00;
						default:
							return 0x20;
						}
					}
					return 0xff;
				};
				cpu.memorymap[page].write = [&](int addr, int data) {
					switch (addr >> 4 & 0xf) {
					case 0:
						switch (addr & 7) {
						case 0:
							return void(fInterruptEnable = (data & 1) != 0);
						case 1:
							return sound0->control(data & 1);
						case 4:
							if (!(fProtectEnable = (data & 1) != 0))
								protect_count = protect_index = 0;
							else if (++protect_count == 0x3c) {
								protect_count = 0;
								if (++protect_index == 0x1e)
									protect_index = 0;
							}
							return;
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
			cpu.iomap[page].write = [&](int addr, int data) { !(addr & 0xff) && (intvec = data); };

		// Videoの初期化
		bg.fill(3), obj.fill(3), bitmap.fill(0xff000000);
		convertGFX(&bg[0], &BG[0], 256, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&obj[0], &OBJ[0], 64, {rseq8(256, 8), rseq8(0, 8)}, {seq4(64, 1), seq4(128, 1), seq4(192, 1), seq4(0, 1)}, {0, 4}, 64);
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | (RGB[i] >> 6) * 255 / 3 << 16 | (RGB[i] >> 3 & 7) * 255 / 7 << 8 | (RGB[i] & 7) * 255 / 7;
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), fInterruptEnable && cpu.interrupt(intvec); });
			sound0->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	CrushRoller *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nBrush) {
			case 3:
				in[2] &= ~0xc;
				break;
			case 4:
				in[2] = in[2] & ~0xc | 4;
				break;
			case 5:
				in[2] = in[2] & ~0xc | 8;
				break;
			case 6:
				in[2] |= 0xc;
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

	CrushRoller *updateInput() {
		in[0] = in[0] & ~(1 << 5) | !fCoin << 5;
		in[1] = in[1] & ~0x60 | !fStart1P << 5 | !fStart2P << 6;
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
		in[0] = in[0] & ~(1 << 0) | fDown << 3 | !fDown << 0;
		in[1] = in[1] & ~(1 << 0) | fDown << 3 | !fDown << 0;
	}

	void right(bool fDown) {
		in[0] = in[0] & ~(1 << 2) | fDown << 1 | !fDown << 2;
		in[1] = in[1] & ~(1 << 2) | fDown << 1 | !fDown << 2;
	}

	void down(bool fDown) {
		in[0] = in[0] & ~(1 << 3) | fDown << 0 | !fDown << 3;
		in[1] = in[1] & ~(1 << 3) | fDown << 0 | !fDown << 3;
	}

	void left(bool fDown) {
		in[0] = in[0] & ~(1 << 1) | fDown << 2 | !fDown << 1;
		in[1] = in[1] & ~(1 << 1) | fDown << 2 | !fDown << 1;
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// bg描画
		int p = 256 * 8 * 4 + 232;
		for (int k = 0x40, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(bitmap.data(), p, k);
		p = 256 * 8 * 36 + 232;
		for (int k = 2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(bitmap.data(), p, k);
		p = 256 * 8 * 37 + 232;
		for (int k = 0x22, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(bitmap.data(), p, k);
		p = 256 * 8 * 2 + 232;
		for (int k = 0x3c2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(bitmap.data(), p, k);
		p = 256 * 8 * 3 + 232;
		for (int k = 0x3e2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(bitmap.data(), p, k);

		// obj描画
		for (int k = 0x0bfe, i = 7; i != 0; k -= 2, --i) {
			const int x = (~ram[k + 0x70] - (i < 3)) & 0xff;
			const int y = (-ram[k + 0x71] & 0xff) + 32;
			const int src = ram[k] | ram[k + 1] << 8;
			switch (ram[k] & 3) {
			case 0: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src);
				break;
			case 1: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src);
				break;
			case 2: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src);
				break;
			case 3: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src);
				break;
			}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0x7c;

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
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = src << 6 & 0x3f00;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		} else {
			src = src << 6 & 0x3f00;
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
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 256 - 16;
			for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[src++]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00) + 256 - 16;
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
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 16;
			for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00) + 16;
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
		const int idx = src >> 6 & 0x7c, h = 288 - (dst >> 8);
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return;
		if (h >= 16) {
			src = (src << 6 & 0x3f00) + 256;
			for (int i = 16; i != 0; dst += 256 - 16, --i)
				for (int j = 16; j != 0; dst++, --j)
					if ((px = COLOR[idx | obj[--src]]))
						data[dst] = px;
		} else {
			src = (src << 6 & 0x3f00) + 256;
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
		sound0 = new PacManSound(SND);
		Z80::init();
	}
};

#endif //CRUSH_ROLLER_H
