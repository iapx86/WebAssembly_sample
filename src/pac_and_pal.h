/*
 *
 *	Pac & Pal
 *
 */

#ifndef PAC_AND_PAL_H
#define PAC_AND_PAL_H

#include <algorithm>
#include <array>
#include "mc6809.h"
#include "mappy_sound.h"
#include "utils.h"
using namespace std;

enum {
	RANK_A, RANK_B, RANK_C, RANK_D,
};

enum {
	BONUS_A, BONUS_B, BONUS_C, BONUS_D, BONUS_E, BONUS_F, BONUS_G, BONUS_NONE, 
};

struct PacAndPal {
	static array<uint8_t, 0x100> SND;
	static array<uint8_t, 0x1000> BG;
	static array<uint8_t, 0x2000> OBJ;
	static array<uint8_t, 0x100> BGCOLOR, OBJCOLOR;
	static array<uint8_t, 0x20> RGB;
	static array<uint8_t, 0x6000> PRG1;
	static array<uint8_t, 0x1000> PRG2;

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
	int nRank = RANK_B;
	int nBonus = BONUS_E;

	bool fPortTest = false;
	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
	array<uint8_t, 0x2000> ram = {};
	array<uint8_t, 0x40> port = {};
	array<uint8_t, 10> in = {};

	array<uint8_t, 0x4000> bg;
	array<uint8_t, 0x10000> obj;
	array<uint8_t, 0x100> bgcolor;
	array<int, 0x20> rgb;
	array<int, width * height> bitmap;
	bool updated = false;

	MC6809 cpu, cpu2;
	Timer<int> timer;

	PacAndPal() : cpu(18432000 / 12), cpu2(18432000 / 12), timer(60) {
		// CPU周りの初期化
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[i].base = &ram[i << 8];
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
			case 0x06: // SND STOP
				return sound0->control(false);
			case 0x07: // SND START
				return sound0->control(true);
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
		for (int i = 0; i < 0x60; i++)
			cpu.memorymap[0xa0 + i].base = &PRG1[i << 8];

		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[i].read = [&](int addr) { return sound0->read(addr); };
			cpu2.memorymap[i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		cpu2.memorymap[0x20].write = cpu.memorymap[0x50].write;
		for (int i = 0; i < 0x10; i++)
			cpu2.memorymap[0xf0 + i].base = &PRG2[i << 8];

		// Videoの初期化
		bg.fill(3), obj.fill(3), bitmap.fill(0xff000000);
		convertGFX(&bg[0], &BG[0], 256, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&obj[0], &OBJ[0], 128, {rseq8(256, 8), rseq8(0, 8)}, {seq4(0, 1), seq4(64, 1), seq4(128, 1), seq4(192, 1)}, {0, 4}, 64);
		for (int i = 0; i < bgcolor.size(); i++)
			bgcolor[i] = 0x10 | ~BGCOLOR[i] & 0xf;
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | (RGB[i] >> 6) * 255 / 3 << 16 | (RGB[i] >> 3 & 7) * 255 / 7 << 8 | (RGB[i] & 7) * 255 / 7;
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), fInterruptEnable0 && cpu.interrupt(), fInterruptEnable1 && cpu2.interrupt(); });
			sound0->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	PacAndPal *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nBonus) {
			case BONUS_A:
				in[7] &= ~3, in[6] |= 8;
				break;
			case BONUS_B:
				in[7] = in[7] & ~3 | 1, in[6] &= ~8;
				break;
			case BONUS_C:
				in[7] = in[7] & ~3 | 1, in[6] |= 8;
				break;
			case BONUS_D:
				in[7] = in[7] & ~3 | 2, in[6] &= ~8;
				break;
			case BONUS_E:
				in[7] = in[7] & ~3 | 2, in[6] |= 8;
				break;
			case BONUS_F:
				in[7] |= 3, in[6] &= ~8;
				break;
			case BONUS_G:
				in[7] |= 3, in[6] |= 8;
				break;
			case BONUS_NONE:
				in[7] &= ~3, in[6] &= ~8;
				break;
			}
			switch (nPacman) {
			case 1:
				in[7] &= ~0xc;
				break;
			case 2:
				in[7] = in[7] & ~0xc | 4;
				break;
			case 3:
				in[7] = in[7] & ~0xc | 8;
				break;
			case 5:
				in[7] |= 0xc;
				break;
			}
			switch (nRank) {
			case RANK_A:
				in[5] &= ~0xc;
				break;
			case RANK_B:
				in[5] = in[5] & ~0xc | 4;
				break;
			case RANK_C:
				in[5] = in[5] & ~0xc | 8;
				break;
			case RANK_D:
				in[5] |= 0xc;
				break;
			}
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
			cpu.reset();
			cpu2.disable();
		}
		return this;
	}

	PacAndPal *updateInput() {
		in[0] = (fCoin != 0) << 3, in[3] = in[3] & 3 | (fStart1P != 0) << 2 | (fStart2P != 0) << 3;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		if (fPortTest)
			return this;
		if (port[8] == 1)
			copy_n(&in[0], 4, &port[0]);
		if (port[0x18] == 3)
			copy_n(&array<uint8_t, 4>{in[5], in[7], in[6], in[8]}[0], 4, &port[0x14]);
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

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256, i++)
			fill_n(&bitmap[p], 224, 0x1f);

		// bg描画
		drawBG(bitmap.data(), 0);

		// obj描画
		for (int k = 0x0f80, i = 64; i != 0; k += 2, --i) {
			const int x = ram[k + 0x800] - 1 & 0xff;
			const int y = (ram[k + 0x801] | ram[k + 0x1001] << 8) - 24 & 0x1ff;
			const int src = ram[k] | ram[k + 1] << 8;
			switch (ram[k + 0x1000] & 0x0f) {
			case 0x00: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src);
				break;
			case 0x01: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src);
				break;
			case 0x02: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src);
				break;
			case 0x03: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src);
				break;
			case 0x04: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src & ~1);
				xfer16x16(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x05: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src | 1);
				xfer16x16V(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x06: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src & ~1);
				xfer16x16H(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x07: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src | 1);
				xfer16x16HV(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x08: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src | 2);
				xfer16x16(bitmap.data(), x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x09: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src | 2);
				xfer16x16V(bitmap.data(), x + 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0a: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src & ~2);
				xfer16x16H(bitmap.data(), x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0b: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src & ~2);
				xfer16x16HV(bitmap.data(), x + 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0c: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src & ~3 | 2);
				xfer16x16(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src | 3);
				xfer16x16(bitmap.data(), x + 16 & 0xff | y << 8, src & ~3);
				xfer16x16(bitmap.data(), x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				break;
			case 0x0d: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src | 3);
				xfer16x16V(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				xfer16x16V(bitmap.data(), x + 16 & 0xff | y << 8, src & ~3 | 1);
				xfer16x16V(bitmap.data(), x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3);
				break;
			case 0x0e: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src & ~3);
				xfer16x16H(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src & ~3 | 1);
				xfer16x16H(bitmap.data(), x + 16 & 0xff | y << 8, src & ~3 | 2);
				xfer16x16H(bitmap.data(), x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src | 3);
				break;
			case 0x0f: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src & ~3 | 1);
				xfer16x16HV(bitmap.data(), x | (y + 16 & 0x1ff) << 8, src & ~3);
				xfer16x16HV(bitmap.data(), x + 16 & 0xff | y << 8, src | 3);
				xfer16x16HV(bitmap.data(), x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 2);
				break;
			}
		}

		// bg描画
		drawBG(bitmap.data(), 1);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void drawBG(int *data, int pri) {
		int p = 256 * 8 * 4 + 232;
		for (int k = 0x40, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(data, p, k, pri);
		p = 256 * 8 * 36 + 232;
		for (int k = 2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 37 + 232;
		for (int k = 0x22, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 2 + 232;
		for (int k = 0x3c2, i = 0; i < 28; p -= 8, k++, i++)
			xfer8x8(data, p, k, pri);
		p = 256 * 8 * 3 + 232;
		for (int k = 0x3e2, i = 0; i < 28; p -= 8, k++, i++)
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
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xf)
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
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xf)
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
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xf)
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
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xf)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new MappySound(SND);
	}
};

#endif //PAC_AND_PAL_H
