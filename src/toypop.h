/*
 *
 *	Toypop
 *
 */

#ifndef TOYPOP_H
#define TOYPOP_H

#include <array>
#include "mc6809.h"
#include "mc68000.h"
#include "mappy_sound.h"
#include "utils.h"
using namespace std;

enum {
	BONUS_A, BONUS_B,
};

enum {
	RANK_EASY, RANK_NORMAL, RANK_HARD, RANK_VERY_HARD,
};

struct Toypop {
	static array<uint8_t, 0x8000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x8000> PRG3;
	static array<uint8_t, 0x2000> BG;
	static array<uint8_t, 0x4000> OBJ;
	static array<uint8_t, 0x100> RED, GREEN, BLUE, BGCOLOR;
	static array<uint8_t, 0x200> OBJCOLOR;
	static array<uint8_t, 0x100> SND;

	static const int cxScreen = 224;
	static const int cyScreen = 288;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static MappySound *sound0;

	bool fReset = true;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nPino = 3;
	int nBonus = BONUS_A;
	int nRank = RANK_NORMAL;
	bool fAttract = true;
	bool fRound = false;

	bool fInterruptEnable = false;
	bool fInterruptEnable2 = false;
	array<uint8_t, 0x2000> ram = {};
	array<uint8_t, 0x800> ram2 = {};
	array<uint8_t, 0x40000> ram3 = {};
	array<uint8_t, 0x40> port = {};
	array<uint8_t, 15> in = {};

	array<uint8_t, 0x10000> vram = {};
	array<uint8_t, 0x8000> bg;
	array<uint8_t, 0x10000> obj;
	array<int, 0x100> rgb;
	array<int, width * height> bitmap;
	bool updated = false;
	int palette = 0;

	MC6809 cpu, cpu2;
	MC68000 cpu3;
	Timer<int> timer;

	Toypop() : cpu(6144000 / 4), cpu2(6144000 / 4), cpu3(6144000), timer(60) {
		// CPU周りの初期化
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[i].base = &ram[i << 8];
			cpu.memorymap[i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x28 + i].base = &ram2[i << 8];
			cpu.memorymap[0x28 + i].write = nullptr;
		}
		cpu.memorymap[0x60].read = [&](int addr) { return port[addr & 0x3f] | 0xf0; };
		cpu.memorymap[0x60].write = [&](int addr, int data) { port[addr & 0x3f] = data & 0xf; };
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x68 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x68 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		cpu.memorymap[0x70].read = [&](int addr) { return fInterruptEnable = true, 0; };
		cpu.memorymap[0x70].write = [&](int addr, int data) { fInterruptEnable = false; };
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[0x80 + i].base = &PRG1[i << 8];
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x80 + i].write = [&](int addr, int data) { addr & 0x800 ? cpu3.disable() : cpu3.enable(); };
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x90 + i].write = [&](int addr, int data) { addr & 0x800 ? cpu2.disable() : cpu2.enable(); };
		cpu.memorymap[0xa0].write = [&](int addr, int data) { palette = addr << 7 & 0x80; };

		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[i].read = [&](int addr) { return sound0->read(addr); };
			cpu2.memorymap[i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[0xe0 + i].base = &PRG2[i << 8];

		for (int i = 0; i < 0x80; i++)
			cpu3.memorymap[i].base = &PRG3[i << 8];
		for (int i = 0; i < 0x400; i++) {
			cpu3.memorymap[0x800 + i].base = &ram3[i << 8];
			cpu3.memorymap[0x800 + i].write = nullptr;
		}
		for (int i = 0; i < 0x10; i++) {
			cpu3.memorymap[0x1000 + i].read = [&](int addr) -> int { return ram2[addr >> 1 & 0x7ff]; };
			cpu3.memorymap[0x1000 + i].write = [&](int addr, int data) { ram2[addr >> 1 & 0x7ff] = data; };
		}
		for (int i = 0; i < 0x80; i++) {
			cpu3.memorymap[0x1800 + i].read = [&](int addr) -> int { return addr = addr << 1 & 0xfffe, vram[addr] << 4 | vram[addr | 1] & 0xf; };
			cpu3.memorymap[0x1800 + i].write = [&](int addr, int data) { addr = addr << 1 & 0xfffe, vram[addr] = data >> 4, vram[addr | 1] = data & 0xf; };
		}
		for (int i = 0; i < 0x500; i++) {
			cpu3.memorymap[0x1900 + i].base = &vram[(i & 0xff) << 8];
			cpu3.memorymap[0x1900 + i].write = nullptr;
		}
		for (int i = 0; i < 0x1000; i++)
			cpu3.memorymap[0x3000 + i].write16 = [&](int addr, int data) { fInterruptEnable2 = !(addr & 0x80000); };

		// Videoの初期化
		bg.fill(3), obj.fill(3), bitmap.fill(0xff000000);
		convertGFX(&bg[0], &BG[0], 512, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&obj[0], &OBJ[0], 256, {rseq8(256, 8), rseq8(0, 8)}, {seq4(0, 1), seq4(64, 1), seq4(128, 1), seq4(192, 1)}, {0, 4}, 64);
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | BLUE[i] * 255 / 15 << 16 | GREEN[i] * 255 / 15 << 8 | RED[i] * 255 / 15;
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			cpu3.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), fInterruptEnable && cpu.interrupt(), cpu2.interrupt(), fInterruptEnable2 && cpu3.interrupt(6); });
			sound0->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	Toypop *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nPino) {
			case 1:
				in[8] = in[8] & ~3 | 1;
				break;
			case 2:
				in[8] = in[8] & ~3 | 2;
				break;
			case 3:
				in[8] &= ~3;
				break;
			case 5:
				in[8] |= 3;
				break;
			}
			switch (nBonus) {
			case BONUS_A:
				in[7] &= ~8;
				break;
			case BONUS_B:
				in[7] |= 8;
				break;
			}
			switch (nRank) {
			case RANK_EASY:
				in[7] = in[7] & ~6 | 2;
				break;
			case RANK_NORMAL:
				in[7] &= ~6;
				break;
			case RANK_HARD:
				in[7] = in[7] & ~6 | 4;
				break;
			case RANK_VERY_HARD:
				in[7] |= 6;
				break;
			}
			if (fAttract)
				in[6] &= ~8;
			else
				in[6] |= 8;
			if (fRound)
				in[6] |= 2;
			else
				in[6] &= ~2;
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[5] |= 8;
		else
			in[5] &= ~8;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			cpu2.disable();
			cpu3.disable();
		}
		return this;
	}

	Toypop *updateInput() {
		in[0] = (fCoin != 0) << 3, in[3] = in[3] & 3 | (fStart1P != 0) << 2 | (fStart2P != 0) << 3;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		if (port[8] == 1)
			copy_n(&in[0], 4, &port[4]);
		else if (port[8] == 5)
			port[2] = 0xf, port[6] = 0xc;
		if (port[0x18] == 1)
			copy_n(&in[5], 4, &port[0x10]);
		else if (port[0x18] == 8)
			port[0x10] = 6, port[0x11] = 9;
		if (port[0x28] == 8)
			port[0x20] = 6, port[0x21] = 9;
		else if (port[0x28] == 9)
			copy_n(&array<uint8_t, 8>{in[10], in[14], in[11], in[11], in[12], in[12], in[13], in[13]}[0], 8, &port[0x20]);
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

		// graphic描画
		int p = 256 * 8 * 2 + 239;
		int idx = 0x60 | palette;
		for (int k = 0x200, i = 0; i < 224; p -= 256 * 288 + 1, i++)
			for (int j = 0; j < 288; k++, p += 256, j++)
				bitmap[p] = idx | vram[k];

		// bg描画
		p = 256 * 8 * 4 + 232;
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
		for (int k = 0xf80, i = 64; i != 0; k += 2, --i) {
			const int x = 0xe9 - ram[k + 0x800] & 0xff;
			const int y = 0x167 - (ram[k + 0x801] | ram[k + 0x1001] << 8) & 0x1ff;
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
				xfer16x16(bitmap.data(), x | y << 8, src | 1);
				xfer16x16(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x05: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src & ~1);
				xfer16x16V(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x06: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src | 1);
				xfer16x16H(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src & ~1);
				break;
			case 0x07: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src & ~1);
				xfer16x16HV(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src | 1);
				break;
			case 0x08: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src & ~2);
				xfer16x16(bitmap.data(), x - 16 & 0xff | y << 8, src | 2);
				break;
			case 0x09: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src & ~2);
				xfer16x16V(bitmap.data(), x - 16 & 0xff | y << 8, src | 2);
				break;
			case 0x0a: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src | 2);
				xfer16x16H(bitmap.data(), x - 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0b: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src | 2);
				xfer16x16HV(bitmap.data(), x - 16 & 0xff | y << 8, src & ~2);
				break;
			case 0x0c: // ノーマル
				xfer16x16(bitmap.data(), x | y << 8, src & ~3 | 1);
				xfer16x16(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src & ~3);
				xfer16x16(bitmap.data(), x - 16 & 0xff | y << 8, src | 3);
				xfer16x16(bitmap.data(), x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 2);
				break;
			case 0x0d: // V反転
				xfer16x16V(bitmap.data(), x | y << 8, src & ~3);
				xfer16x16V(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src & ~3 | 1);
				xfer16x16V(bitmap.data(), x - 16 & 0xff | y << 8, src & ~3 | 2);
				xfer16x16V(bitmap.data(), x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src | 3);
				break;
			case 0x0e: // H反転
				xfer16x16H(bitmap.data(), x | y << 8, src | 3);
				xfer16x16H(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src & ~3 | 2);
				xfer16x16H(bitmap.data(), x - 16 & 0xff | y << 8, src & ~3 | 1);
				xfer16x16H(bitmap.data(), x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3);
				break;
			case 0x0f: // HV反転
				xfer16x16HV(bitmap.data(), x | y << 8, src & ~3 | 2);
				xfer16x16HV(bitmap.data(), x | (y - 16 & 0x1ff) << 8, src | 3);
				xfer16x16HV(bitmap.data(), x - 16 & 0xff | y << 8, src & ~3);
				xfer16x16HV(bitmap.data(), x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 1);
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
		const int q = ram[k] << 6, idx = ram[k + 0x400] << 2 & 0xfc, idx2 = 0x70 | palette;
		int px;

		(px = BGCOLOR[idx | bg[q | 0x00]]) != 0xf && (data[p + 0x000] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x01]]) != 0xf && (data[p + 0x001] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x02]]) != 0xf && (data[p + 0x002] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x03]]) != 0xf && (data[p + 0x003] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x04]]) != 0xf && (data[p + 0x004] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x05]]) != 0xf && (data[p + 0x005] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x06]]) != 0xf && (data[p + 0x006] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x07]]) != 0xf && (data[p + 0x007] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x08]]) != 0xf && (data[p + 0x100] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x09]]) != 0xf && (data[p + 0x101] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0a]]) != 0xf && (data[p + 0x102] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0b]]) != 0xf && (data[p + 0x103] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0c]]) != 0xf && (data[p + 0x104] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0d]]) != 0xf && (data[p + 0x105] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0e]]) != 0xf && (data[p + 0x106] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x0f]]) != 0xf && (data[p + 0x107] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x10]]) != 0xf && (data[p + 0x200] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x11]]) != 0xf && (data[p + 0x201] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x12]]) != 0xf && (data[p + 0x202] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x13]]) != 0xf && (data[p + 0x203] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x14]]) != 0xf && (data[p + 0x204] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x15]]) != 0xf && (data[p + 0x205] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x16]]) != 0xf && (data[p + 0x206] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x17]]) != 0xf && (data[p + 0x207] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x18]]) != 0xf && (data[p + 0x300] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x19]]) != 0xf && (data[p + 0x301] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1a]]) != 0xf && (data[p + 0x302] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1b]]) != 0xf && (data[p + 0x303] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1c]]) != 0xf && (data[p + 0x304] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1d]]) != 0xf && (data[p + 0x305] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1e]]) != 0xf && (data[p + 0x306] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x1f]]) != 0xf && (data[p + 0x307] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x20]]) != 0xf && (data[p + 0x400] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x21]]) != 0xf && (data[p + 0x401] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x22]]) != 0xf && (data[p + 0x402] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x23]]) != 0xf && (data[p + 0x403] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x24]]) != 0xf && (data[p + 0x404] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x25]]) != 0xf && (data[p + 0x405] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x26]]) != 0xf && (data[p + 0x406] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x27]]) != 0xf && (data[p + 0x407] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x28]]) != 0xf && (data[p + 0x500] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x29]]) != 0xf && (data[p + 0x501] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2a]]) != 0xf && (data[p + 0x502] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2b]]) != 0xf && (data[p + 0x503] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2c]]) != 0xf && (data[p + 0x504] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2d]]) != 0xf && (data[p + 0x505] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2e]]) != 0xf && (data[p + 0x506] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x2f]]) != 0xf && (data[p + 0x507] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x30]]) != 0xf && (data[p + 0x600] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x31]]) != 0xf && (data[p + 0x601] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x32]]) != 0xf && (data[p + 0x602] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x33]]) != 0xf && (data[p + 0x603] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x34]]) != 0xf && (data[p + 0x604] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x35]]) != 0xf && (data[p + 0x605] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x36]]) != 0xf && (data[p + 0x606] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x37]]) != 0xf && (data[p + 0x607] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x38]]) != 0xf && (data[p + 0x700] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x39]]) != 0xf && (data[p + 0x701] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3a]]) != 0xf && (data[p + 0x702] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3b]]) != 0xf && (data[p + 0x703] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3c]]) != 0xf && (data[p + 0x704] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3d]]) != 0xf && (data[p + 0x705] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3e]]) != 0xf && (data[p + 0x706] = idx2 | px);
		(px = BGCOLOR[idx | bg[q | 0x3f]]) != 0xf && (data[p + 0x707] = idx2 | px);
	}

	void xfer16x16(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0xff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[src++]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src) {
		const int idx = src >> 6 & 0xfc | 0x100;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0xff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = OBJCOLOR[idx | obj[--src]]) != 0xff)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new MappySound(SND);
	}
};

#endif //TOYPOP_H
