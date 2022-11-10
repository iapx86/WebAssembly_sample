/*
 *
 *	Pac-Land
 *
 */

#ifndef PAC_LAND_H
#define PAC_LAND_H

#include <cmath>
#include <algorithm>
#include <array>
#include "mc6809.hpp"
#include "mc6801.hpp"
#include "c30.hpp"
#include "utils.hpp"
using namespace std;

enum {
	BONUS_A, BONUS_B, BONUS_C, BONUS_D, BONUS_E, BONUS_F, BONUS_G, BONUS_H,
};

enum {
	RANK_A, RANK_B, RANK_C, RANK_D,
};

struct PacLand {
	static array<uint8_t, 0x18000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x1000> PRG2I;
	static array<uint8_t, 0x2000> FG, BG;
	static array<uint8_t, 0x10000> OBJ;
	static array<uint8_t, 0x400> RED, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR;

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
	int nLife = 3;
	bool fAttract = true;
	int nBonus = BONUS_A;
	int nRank = RANK_A;

	bool fInterruptEnable0 = false;
	bool fInterruptEnable1 = false;
	int bank = 0x80;

	array<uint8_t, 0x3800> ram = {};
	array<uint8_t, 0x900> ram2 = {};
	array<uint8_t, 5> in = {0xff, 0xff, 0xff, 0xff, 0xff};
	bool cpu_irq = false;
	bool mcu_irq = false;

	array<uint8_t, 0x8000> fg;
	array<uint8_t, 0x8000> bg;
	array<uint8_t, 0x20000> obj;
	array<int, 0x400> rgb;
	array<int, width * height> bitmap;
	bool updated = false;
	array<uint8_t, 0x100> opaque[3] = {};
	int dwScroll0 = 0;
	int dwScroll1 = 0;
	int palette = 0;
	bool fFlip = false;

	MC6809 cpu;
	MC6801 mcu;
	Timer<int> timer;

	PacLand() : cpu(49152000 / 32), mcu(49152000 / 8 / 4), timer(60) {
		// CPU周りの初期化
		for (int i = 0; i < 0x38; i++) {
			cpu.memorymap[i].base = &ram[i << 8];
			cpu.memorymap[i].write = nullptr;
		}
		cpu.memorymap[0x38].write = [&](int addr, int data) { dwScroll0 = data | addr << 8 & 0x100; };
		cpu.memorymap[0x3a].write = [&](int addr, int data) { dwScroll1 = data | addr << 8 & 0x100; };
		cpu.memorymap[0x3c].write = [&](int addr, int data) {
			const int _bank = (data << 5 & 0xe0) + 0x80;
			if (addr & 0xff)
				return;
			if (_bank != bank) {
				for (int i = 0; i < 0x20; i++)
					cpu.memorymap[0x40 + i].base = &PRG1[_bank + i << 8];
				bank = _bank;
			}
			palette = data << 5 & 0x300;
		};
		for (int i = 0; i < 0x20; i++)
			cpu.memorymap[0x40 + i].base = &PRG1[0x80 + i << 8];
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x68 + i].read = [&](int addr) { return sound0->read(addr); };
			cpu.memorymap[0x68 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x70 + i].write = [&](int addr, int data) { fInterruptEnable0 = !(addr & 0x800); };
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[0x80 + i].base = &PRG1[i << 8];
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x80 + i].write = [&](int addr, int data) { addr & 0x800 ? mcu.disable() : mcu.enable(); };
		for (int i = 0; i < 0x10; i++)
			cpu.memorymap[0x90 + i].write = [&](int addr, int data) { fFlip = !(addr & 0x800); };

		cpu.check_interrupt = [&]() { return cpu_irq && cpu.interrupt() && (cpu_irq = false, true); };

		mcu.memorymap[0].base = &ram2[0];
		mcu.memorymap[0].read = [&](int addr) -> int {
			int data;
			switch (addr) {
			case 2:
				return in[4];
			case 8:
				return data = ram2[8], ram2[8] &= ~0xe0, data;
			}
			return ram2[addr];
		};
		mcu.memorymap[0].write = nullptr;
		for (int i = 0; i < 4; i++) {
			mcu.memorymap[0x10 + i].read = [&](int addr) { return sound0->read(addr); };
			mcu.memorymap[0x10 + i].write = [&](int addr, int data) { sound0->write(addr, data); };
		}
		for (int i = 0; i < 0x40; i++)
			mcu.memorymap[0x40 + i].write = [&](int addr, int data) { fInterruptEnable1 = !(addr & 0x2000); };
		for (int i = 0; i < 0x20; i++)
			mcu.memorymap[0x80 + i].base = &PRG2[i << 8];
		for (int i = 0; i < 8; i++) {
			mcu.memorymap[0xc0 + i].base = &ram2[1 + i << 8];
			mcu.memorymap[0xc0 + i].write = nullptr;
		}
		mcu.memorymap[0xd0].read = [&](int addr) -> int { return addr & 0xfc ? 0xff : in[addr & 3]; };
		for (int i = 0; i < 0x10; i++)
			mcu.memorymap[0xf0 + i].base = &PRG2I[i << 8];

		mcu.check_interrupt = [&]() { return mcu_irq && mcu.interrupt() ? (mcu_irq = false, true) : (ram2[8] & 0x48) == 0x48 && mcu.interrupt(MC6801_OCF); };

		// Videoの初期化
		fg.fill(3), bg.fill(3), obj.fill(15), bitmap.fill(0xff000000);
		convertGFX(&fg[0], &FG[0], 512, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&bg[0], &BG[0], 512, {rseq8(0, 8)}, {seq4(64, 1), seq4(0, 1)}, {0, 4}, 16);
		convertGFX(&obj[0], &OBJ[0], 512, {rseq8(256, 8), rseq8(0, 8)}, {seq4(0, 1), seq4(64, 1), seq4(128, 1), seq4(192, 1)}, {0, 4, 0x40000, 0x40004}, 64);
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | BLUE[i] * 255 / 15 << 16 | (RED[i] >> 4) * 255 / 15 << 8 | (RED[i] & 15) * 255 / 15;
		fill_n(&opaque[0][0], 0x80, 1), fill_n(&opaque[1][0], 0x7f, 1), fill_n(&opaque[1][0x80], 0x7f, 1), fill_n(&opaque[2][0xf0], 0xf, 1);
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			mcu.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), cpu_irq = fInterruptEnable0, mcu_irq = fInterruptEnable1, ram2[8] |= ram2[8] << 3 & 0x40; });
			sound0->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	PacLand *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 3:
				in[0] |= 0x60;
				break;
			case 2:
				in[0] = in[0] & ~0x60 | 0x40;
				break;
			case 4:
				in[0] = in[0] & ~0x60 | 0x20;
				break;
			case 5:
				in[0] &= ~0x60;
				break;
			}
			if (fAttract)
				in[1] |= 0x40;
			else
				in[1] &= ~0x40;
			switch (nBonus) {
			case BONUS_A: // 30000 80000 150000 300000 500000 1000000
				in[0] |= 0x0e;
				break;
			case BONUS_B: // 30000 100000 200000 400000 600000 1000000
				in[0] = in[0] & ~0xe | 0xc;
				break;
			case BONUS_C: // 40000 100000 180000 300000 500000 1000000
				in[0] = in[0] & ~0xe | 0xa;
				break;
			case BONUS_D: // 30000 80000 every 100000
				in[0] = in[0] & ~0xe | 8;
				break;
			case BONUS_E: // 50000 150000 every 200000
				in[0] = in[0] & ~0xe | 6;
				break;
			case BONUS_F: // 30000 80000 150000
				in[0] = in[0] & ~0xe | 4;
				break;
			case BONUS_G: // 40000 100000 200000
				in[0] = in[0] & ~0xe | 2;
				break;
			case BONUS_H: // 40000
				in[0] &= ~0xe;
				break;
			}
			switch (nRank) {
			case RANK_A: // Normal
				in[0] |= 1, in[1] |= 8;
				break;
			case RANK_B: // Easy
				in[0] |= 1, in[1] &= ~8;
				break;
			case RANK_C: // Hard
				in[0] &= ~1, in[1] |= 8;
				break;
			case RANK_D: // Very Hard
				in[0] &= ~1, in[1] &= ~8;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		if (fTest)
			in[0] &= ~0x80;
		else
			in[0] |= 0x80;

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu_irq = mcu_irq = false;
			cpu.reset();
			mcu.disable();
		}
		return this;
	}

	PacLand *updateInput() {
		in[3] = in[3] & ~(1 << 5) | !fCoin << 5;
		in[2] = in[2] & ~0x30 | !fStart1P << 4 | !fStart2P << 5;
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

	void right(bool fDown) {
		in[4] = in[4] & ~(1 << 5) | !fDown << 5;
	}

	void left(bool fDown) {
		in[4] = in[4] & ~(1 << 4) | !fDown << 4;
	}

	void triggerA(bool fDown) {
		in[4] = in[4] & ~(1 << 3) | !fDown << 3;
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256, i++)
			fill_n(&bitmap[p], 224, 0);

		// obj描画
		drawObj(bitmap.data(), 0);

		// bg描画
		p = 256 * 8 * 2 + 232 - (fFlip ? 4 + dwScroll1 & 7 : 5 + dwScroll1 & 7) * 256;
		int k = 0x1100 | (fFlip ? (4 + dwScroll1 >> 2) + 0x30 : (5 + dwScroll1 >> 2) + 4) & 0x7e;
		for (int i = 0; i < 28; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				xfer8x8(bitmap.data(), &BGCOLOR[0], &bg[0], ram[k + 1] << 1 & 0x7c | ram[k] << 1 & 0x180 | ram[k + 1] << 9 & 0x200, p, k);

		// fg描画
		p = 256 * 8 * 2 + 208 - (fFlip ? 1 + dwScroll0 & 7 : dwScroll0 & 7) * 256;
		k = 0x280 | (fFlip ? (1 + dwScroll0 >> 2) + 0x30 : (dwScroll0 >> 2) + 6) & 0x7e;
		for (int i = 0; i < 24; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				if (~ram[k + 1] & 0x20)
					xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 232;
		k = fFlip ? 0x132 : 0x106;
		for (int i = 0; i < 3; p -= 256 * 8 * 36 + 8, k += 56, i++)
			for (int j = 0; j < 36; p += 256 * 8, k += 2, j++)
				if (~ram[k + 1] & 0x20)
					xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 16;
		k = fFlip ? 0xeb2 : 0xe86;
		for (int i = 0; i < 36; p += 256 * 8, k += 2, i++)
			if (~ram[k + 1] & 0x20)
				xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);

		// obj描画
		drawObj(bitmap.data(), 1);

		// fg描画
		p = 256 * 8 * 2 + 208 - (fFlip ? 1 + dwScroll0 & 7 : dwScroll0 & 7) * 256;
		k = 0x280 | (fFlip ? (1 + dwScroll0 >> 2) + 0x30 : (dwScroll0 >> 2) + 6) & 0x7e;
		for (int i = 0; i < 24; k = k + 54 & 0x7e | k + 0x80 & 0x1f80, p -= 256 * 8 * 37 + 8, i++)
			for (int j = 0; j < 37; k = k + 2 & 0x7e | k & 0x1f80, p += 256 * 8, j++)
				if (ram[k + 1] & 0x20)
					xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 232;
		k = fFlip ? 0x132 : 0x106;
		for (int i = 0; i < 3; p -= 256 * 8 * 36 + 8, k += 56, i++)
			for (int j = 0; j < 36; p += 256 * 8, k += 2, j++)
				if (ram[k + 1] & 0x20)
					xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);
		p = 256 * 8 * 2 + 16;
		k = fFlip ? 0xeb2 : 0xe86;
		for (int i = 0; i < 36; p += 256 * 8, k += 2, i++)
			if (ram[k + 1] & 0x20)
				xfer8x8(bitmap.data(), &FGCOLOR[0], &fg[0], ram[k + 1] << 1 & 0x3c | ram[k] << 1 & 0x1c0 | ram[k + 1] << 9 & 0x200, p, k);

		// obj描画
		drawObj(bitmap.data(), 2);

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 288; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[palette | bitmap[p]];

		return bitmap.data();
	}

	void drawObj(int *data, int cat) {
		if (fFlip) {
			for (int k = 0x2780, i = 64; i != 0; k += 2, --i) {
				const int x = 0xe9 - ram[k + 0x800] & 0xff;
				const int y = 0x167 - ram[k + 0x801] - ram[k + 0x1001] * 0x100 & 0x1ff;
				const int src = ram[k + 1] << 9 | ram[k + 0x1000] << 1 & 0x100 | ram[k];
				switch (ram[k + 0x1000] & 0x0f) {
				case 0x00: // ノーマル
					xfer16x16(data, x | y << 8, src, cat);
					break;
				case 0x01: // V反転
					xfer16x16V(data, x | y << 8, src, cat);
					break;
				case 0x02: // H反転
					xfer16x16H(data, x | y << 8, src, cat);
					break;
				case 0x03: // HV反転
					xfer16x16HV(data, x | y << 8, src, cat);
					break;
				case 0x04: // ノーマル
					xfer16x16(data, x | y << 8, src | 1, cat);
					xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x05: // V反転
					xfer16x16V(data, x | y << 8, src & ~1, cat);
					xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x06: // H反転
					xfer16x16H(data, x | y << 8, src | 1, cat);
					xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x07: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~1, cat);
					xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x08: // ノーマル
					xfer16x16(data, x | y << 8, src & ~2, cat);
					xfer16x16(data, x - 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x09: // V反転
					xfer16x16V(data, x | y << 8, src & ~2, cat);
					xfer16x16V(data, x - 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0a: // H反転
					xfer16x16H(data, x | y << 8, src | 2, cat);
					xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0b: // HV反転
					xfer16x16HV(data, x | y << 8, src | 2, cat);
					xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0c: // ノーマル
					xfer16x16(data, x | y << 8, src & ~3 | 1, cat);
					xfer16x16(data, x | (y - 16 & 0x1ff) << 8, src & ~3, cat);
					xfer16x16(data, x - 16 & 0xff | y << 8, src | 3, cat);
					xfer16x16(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					break;
				case 0x0d: // V反転
					xfer16x16V(data, x | y << 8, src & ~3, cat);
					xfer16x16V(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					xfer16x16V(data, x - 16 & 0xff | y << 8, src & ~3 | 2, cat);
					xfer16x16V(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src | 3, cat);
					break;
				case 0x0e: // H反転
					xfer16x16H(data, x | y << 8, src | 3, cat);
					xfer16x16H(data, x | (y - 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					xfer16x16H(data, x - 16 & 0xff | y << 8, src & ~3 | 1, cat);
					xfer16x16H(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3, cat);
					break;
				case 0x0f: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~3 | 2, cat);
					xfer16x16HV(data, x | (y - 16 & 0x1ff) << 8, src | 3, cat);
					xfer16x16HV(data, x - 16 & 0xff | y << 8, src & ~3, cat);
					xfer16x16HV(data, x - 16 & 0xff | (y - 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					break;
				}
			}
		} else {
			for (int k = 0x2780, i = 64; i != 0; k += 2, --i) {
				const int x = ram[k + 0x800] + 7 & 0xff;
				const int y = (ram[k + 0x801] | ram[k + 0x1001] << 8) - 55 & 0x1ff;
				const int src = ram[k + 1] << 9 | ram[k + 0x1000] << 1 & 0x100 | ram[k];
				switch (ram[k + 0x1000] & 0x0f) {
				case 0x00: // ノーマル
					xfer16x16(data, x | y << 8, src, cat);
					break;
				case 0x01: // V反転
					xfer16x16V(data, x | y << 8, src, cat);
					break;
				case 0x02: // H反転
					xfer16x16H(data, x | y << 8, src, cat);
					break;
				case 0x03: // HV反転
					xfer16x16HV(data, x | y << 8, src, cat);
					break;
				case 0x04: // ノーマル
					xfer16x16(data, x | y << 8, src & ~1, cat);
					xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x05: // V反転
					xfer16x16V(data, x | y << 8, src | 1, cat);
					xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x06: // H反転
					xfer16x16H(data, x | y << 8, src & ~1, cat);
					xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src | 1, cat);
					break;
				case 0x07: // HV反転
					xfer16x16HV(data, x | y << 8, src | 1, cat);
					xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~1, cat);
					break;
				case 0x08: // ノーマル
					xfer16x16(data, x | y << 8, src | 2, cat);
					xfer16x16(data, x + 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x09: // V反転
					xfer16x16V(data, x | y << 8, src | 2, cat);
					xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~2, cat);
					break;
				case 0x0a: // H反転
					xfer16x16H(data, x | y << 8, src & ~2, cat);
					xfer16x16H(data, x + 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0b: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~2, cat);
					xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 2, cat);
					break;
				case 0x0c: // ノーマル
					xfer16x16(data, x | y << 8, src & ~3 | 2, cat);
					xfer16x16(data, x | (y + 16 & 0x1ff) << 8, src | 3, cat);
					xfer16x16(data, x + 16 & 0xff | y << 8, src & ~3, cat);
					xfer16x16(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					break;
				case 0x0d: // V反転
					xfer16x16V(data, x | y << 8, src | 3, cat);
					xfer16x16V(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					xfer16x16V(data, x + 16 & 0xff | y << 8, src & ~3 | 1, cat);
					xfer16x16V(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3, cat);
					break;
				case 0x0e: // H反転
					xfer16x16H(data, x | y << 8, src & ~3, cat);
					xfer16x16H(data, x | (y + 16 & 0x1ff) << 8, src & ~3 | 1, cat);
					xfer16x16H(data, x + 16 & 0xff | y << 8, src & ~3 | 2, cat);
					xfer16x16H(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src | 3, cat);
					break;
				case 0x0f: // HV反転
					xfer16x16HV(data, x | y << 8, src & ~3 | 1, cat);
					xfer16x16HV(data, x | (y + 16 & 0x1ff) << 8, src & ~3, cat);
					xfer16x16HV(data, x + 16 & 0xff | y << 8, src | 3, cat);
					xfer16x16HV(data, x + 16 & 0xff | (y + 16 & 0x1ff) << 8, src & ~3 | 2, cat);
					break;
				}
			}
		}
	}

	void xfer8x8(int *data, uint8_t *color, uint8_t *pattern, int idx, int p, int k) {
		const int q = (ram[k] | ram[k + 1] << 8) << 6 & 0x7fc0;
		int px;

		switch (ram[k + 1] >> 6) {
		case 0: // ノーマル
			(px = color[idx | pattern[q | 0x00]]) != 0xff && (data[p + 0x000] = px);
			(px = color[idx | pattern[q | 0x01]]) != 0xff && (data[p + 0x001] = px);
			(px = color[idx | pattern[q | 0x02]]) != 0xff && (data[p + 0x002] = px);
			(px = color[idx | pattern[q | 0x03]]) != 0xff && (data[p + 0x003] = px);
			(px = color[idx | pattern[q | 0x04]]) != 0xff && (data[p + 0x004] = px);
			(px = color[idx | pattern[q | 0x05]]) != 0xff && (data[p + 0x005] = px);
			(px = color[idx | pattern[q | 0x06]]) != 0xff && (data[p + 0x006] = px);
			(px = color[idx | pattern[q | 0x07]]) != 0xff && (data[p + 0x007] = px);
			(px = color[idx | pattern[q | 0x08]]) != 0xff && (data[p + 0x100] = px);
			(px = color[idx | pattern[q | 0x09]]) != 0xff && (data[p + 0x101] = px);
			(px = color[idx | pattern[q | 0x0a]]) != 0xff && (data[p + 0x102] = px);
			(px = color[idx | pattern[q | 0x0b]]) != 0xff && (data[p + 0x103] = px);
			(px = color[idx | pattern[q | 0x0c]]) != 0xff && (data[p + 0x104] = px);
			(px = color[idx | pattern[q | 0x0d]]) != 0xff && (data[p + 0x105] = px);
			(px = color[idx | pattern[q | 0x0e]]) != 0xff && (data[p + 0x106] = px);
			(px = color[idx | pattern[q | 0x0f]]) != 0xff && (data[p + 0x107] = px);
			(px = color[idx | pattern[q | 0x10]]) != 0xff && (data[p + 0x200] = px);
			(px = color[idx | pattern[q | 0x11]]) != 0xff && (data[p + 0x201] = px);
			(px = color[idx | pattern[q | 0x12]]) != 0xff && (data[p + 0x202] = px);
			(px = color[idx | pattern[q | 0x13]]) != 0xff && (data[p + 0x203] = px);
			(px = color[idx | pattern[q | 0x14]]) != 0xff && (data[p + 0x204] = px);
			(px = color[idx | pattern[q | 0x15]]) != 0xff && (data[p + 0x205] = px);
			(px = color[idx | pattern[q | 0x16]]) != 0xff && (data[p + 0x206] = px);
			(px = color[idx | pattern[q | 0x17]]) != 0xff && (data[p + 0x207] = px);
			(px = color[idx | pattern[q | 0x18]]) != 0xff && (data[p + 0x300] = px);
			(px = color[idx | pattern[q | 0x19]]) != 0xff && (data[p + 0x301] = px);
			(px = color[idx | pattern[q | 0x1a]]) != 0xff && (data[p + 0x302] = px);
			(px = color[idx | pattern[q | 0x1b]]) != 0xff && (data[p + 0x303] = px);
			(px = color[idx | pattern[q | 0x1c]]) != 0xff && (data[p + 0x304] = px);
			(px = color[idx | pattern[q | 0x1d]]) != 0xff && (data[p + 0x305] = px);
			(px = color[idx | pattern[q | 0x1e]]) != 0xff && (data[p + 0x306] = px);
			(px = color[idx | pattern[q | 0x1f]]) != 0xff && (data[p + 0x307] = px);
			(px = color[idx | pattern[q | 0x20]]) != 0xff && (data[p + 0x400] = px);
			(px = color[idx | pattern[q | 0x21]]) != 0xff && (data[p + 0x401] = px);
			(px = color[idx | pattern[q | 0x22]]) != 0xff && (data[p + 0x402] = px);
			(px = color[idx | pattern[q | 0x23]]) != 0xff && (data[p + 0x403] = px);
			(px = color[idx | pattern[q | 0x24]]) != 0xff && (data[p + 0x404] = px);
			(px = color[idx | pattern[q | 0x25]]) != 0xff && (data[p + 0x405] = px);
			(px = color[idx | pattern[q | 0x26]]) != 0xff && (data[p + 0x406] = px);
			(px = color[idx | pattern[q | 0x27]]) != 0xff && (data[p + 0x407] = px);
			(px = color[idx | pattern[q | 0x28]]) != 0xff && (data[p + 0x500] = px);
			(px = color[idx | pattern[q | 0x29]]) != 0xff && (data[p + 0x501] = px);
			(px = color[idx | pattern[q | 0x2a]]) != 0xff && (data[p + 0x502] = px);
			(px = color[idx | pattern[q | 0x2b]]) != 0xff && (data[p + 0x503] = px);
			(px = color[idx | pattern[q | 0x2c]]) != 0xff && (data[p + 0x504] = px);
			(px = color[idx | pattern[q | 0x2d]]) != 0xff && (data[p + 0x505] = px);
			(px = color[idx | pattern[q | 0x2e]]) != 0xff && (data[p + 0x506] = px);
			(px = color[idx | pattern[q | 0x2f]]) != 0xff && (data[p + 0x507] = px);
			(px = color[idx | pattern[q | 0x30]]) != 0xff && (data[p + 0x600] = px);
			(px = color[idx | pattern[q | 0x31]]) != 0xff && (data[p + 0x601] = px);
			(px = color[idx | pattern[q | 0x32]]) != 0xff && (data[p + 0x602] = px);
			(px = color[idx | pattern[q | 0x33]]) != 0xff && (data[p + 0x603] = px);
			(px = color[idx | pattern[q | 0x34]]) != 0xff && (data[p + 0x604] = px);
			(px = color[idx | pattern[q | 0x35]]) != 0xff && (data[p + 0x605] = px);
			(px = color[idx | pattern[q | 0x36]]) != 0xff && (data[p + 0x606] = px);
			(px = color[idx | pattern[q | 0x37]]) != 0xff && (data[p + 0x607] = px);
			(px = color[idx | pattern[q | 0x38]]) != 0xff && (data[p + 0x700] = px);
			(px = color[idx | pattern[q | 0x39]]) != 0xff && (data[p + 0x701] = px);
			(px = color[idx | pattern[q | 0x3a]]) != 0xff && (data[p + 0x702] = px);
			(px = color[idx | pattern[q | 0x3b]]) != 0xff && (data[p + 0x703] = px);
			(px = color[idx | pattern[q | 0x3c]]) != 0xff && (data[p + 0x704] = px);
			(px = color[idx | pattern[q | 0x3d]]) != 0xff && (data[p + 0x705] = px);
			(px = color[idx | pattern[q | 0x3e]]) != 0xff && (data[p + 0x706] = px);
			(px = color[idx | pattern[q | 0x3f]]) != 0xff && (data[p + 0x707] = px);
			break;
		case 1: // V反転
			(px = color[idx | pattern[q | 0x38]]) != 0xff && (data[p + 0x000] = px);
			(px = color[idx | pattern[q | 0x39]]) != 0xff && (data[p + 0x001] = px);
			(px = color[idx | pattern[q | 0x3a]]) != 0xff && (data[p + 0x002] = px);
			(px = color[idx | pattern[q | 0x3b]]) != 0xff && (data[p + 0x003] = px);
			(px = color[idx | pattern[q | 0x3c]]) != 0xff && (data[p + 0x004] = px);
			(px = color[idx | pattern[q | 0x3d]]) != 0xff && (data[p + 0x005] = px);
			(px = color[idx | pattern[q | 0x3e]]) != 0xff && (data[p + 0x006] = px);
			(px = color[idx | pattern[q | 0x3f]]) != 0xff && (data[p + 0x007] = px);
			(px = color[idx | pattern[q | 0x30]]) != 0xff && (data[p + 0x100] = px);
			(px = color[idx | pattern[q | 0x31]]) != 0xff && (data[p + 0x101] = px);
			(px = color[idx | pattern[q | 0x32]]) != 0xff && (data[p + 0x102] = px);
			(px = color[idx | pattern[q | 0x33]]) != 0xff && (data[p + 0x103] = px);
			(px = color[idx | pattern[q | 0x34]]) != 0xff && (data[p + 0x104] = px);
			(px = color[idx | pattern[q | 0x35]]) != 0xff && (data[p + 0x105] = px);
			(px = color[idx | pattern[q | 0x36]]) != 0xff && (data[p + 0x106] = px);
			(px = color[idx | pattern[q | 0x37]]) != 0xff && (data[p + 0x107] = px);
			(px = color[idx | pattern[q | 0x28]]) != 0xff && (data[p + 0x200] = px);
			(px = color[idx | pattern[q | 0x29]]) != 0xff && (data[p + 0x201] = px);
			(px = color[idx | pattern[q | 0x2a]]) != 0xff && (data[p + 0x202] = px);
			(px = color[idx | pattern[q | 0x2b]]) != 0xff && (data[p + 0x203] = px);
			(px = color[idx | pattern[q | 0x2c]]) != 0xff && (data[p + 0x204] = px);
			(px = color[idx | pattern[q | 0x2d]]) != 0xff && (data[p + 0x205] = px);
			(px = color[idx | pattern[q | 0x2e]]) != 0xff && (data[p + 0x206] = px);
			(px = color[idx | pattern[q | 0x2f]]) != 0xff && (data[p + 0x207] = px);
			(px = color[idx | pattern[q | 0x20]]) != 0xff && (data[p + 0x300] = px);
			(px = color[idx | pattern[q | 0x21]]) != 0xff && (data[p + 0x301] = px);
			(px = color[idx | pattern[q | 0x22]]) != 0xff && (data[p + 0x302] = px);
			(px = color[idx | pattern[q | 0x23]]) != 0xff && (data[p + 0x303] = px);
			(px = color[idx | pattern[q | 0x24]]) != 0xff && (data[p + 0x304] = px);
			(px = color[idx | pattern[q | 0x25]]) != 0xff && (data[p + 0x305] = px);
			(px = color[idx | pattern[q | 0x26]]) != 0xff && (data[p + 0x306] = px);
			(px = color[idx | pattern[q | 0x27]]) != 0xff && (data[p + 0x307] = px);
			(px = color[idx | pattern[q | 0x18]]) != 0xff && (data[p + 0x400] = px);
			(px = color[idx | pattern[q | 0x19]]) != 0xff && (data[p + 0x401] = px);
			(px = color[idx | pattern[q | 0x1a]]) != 0xff && (data[p + 0x402] = px);
			(px = color[idx | pattern[q | 0x1b]]) != 0xff && (data[p + 0x403] = px);
			(px = color[idx | pattern[q | 0x1c]]) != 0xff && (data[p + 0x404] = px);
			(px = color[idx | pattern[q | 0x1d]]) != 0xff && (data[p + 0x405] = px);
			(px = color[idx | pattern[q | 0x1e]]) != 0xff && (data[p + 0x406] = px);
			(px = color[idx | pattern[q | 0x1f]]) != 0xff && (data[p + 0x407] = px);
			(px = color[idx | pattern[q | 0x10]]) != 0xff && (data[p + 0x500] = px);
			(px = color[idx | pattern[q | 0x11]]) != 0xff && (data[p + 0x501] = px);
			(px = color[idx | pattern[q | 0x12]]) != 0xff && (data[p + 0x502] = px);
			(px = color[idx | pattern[q | 0x13]]) != 0xff && (data[p + 0x503] = px);
			(px = color[idx | pattern[q | 0x14]]) != 0xff && (data[p + 0x504] = px);
			(px = color[idx | pattern[q | 0x15]]) != 0xff && (data[p + 0x505] = px);
			(px = color[idx | pattern[q | 0x16]]) != 0xff && (data[p + 0x506] = px);
			(px = color[idx | pattern[q | 0x17]]) != 0xff && (data[p + 0x507] = px);
			(px = color[idx | pattern[q | 0x08]]) != 0xff && (data[p + 0x600] = px);
			(px = color[idx | pattern[q | 0x09]]) != 0xff && (data[p + 0x601] = px);
			(px = color[idx | pattern[q | 0x0a]]) != 0xff && (data[p + 0x602] = px);
			(px = color[idx | pattern[q | 0x0b]]) != 0xff && (data[p + 0x603] = px);
			(px = color[idx | pattern[q | 0x0c]]) != 0xff && (data[p + 0x604] = px);
			(px = color[idx | pattern[q | 0x0d]]) != 0xff && (data[p + 0x605] = px);
			(px = color[idx | pattern[q | 0x0e]]) != 0xff && (data[p + 0x606] = px);
			(px = color[idx | pattern[q | 0x0f]]) != 0xff && (data[p + 0x607] = px);
			(px = color[idx | pattern[q | 0x00]]) != 0xff && (data[p + 0x700] = px);
			(px = color[idx | pattern[q | 0x01]]) != 0xff && (data[p + 0x701] = px);
			(px = color[idx | pattern[q | 0x02]]) != 0xff && (data[p + 0x702] = px);
			(px = color[idx | pattern[q | 0x03]]) != 0xff && (data[p + 0x703] = px);
			(px = color[idx | pattern[q | 0x04]]) != 0xff && (data[p + 0x704] = px);
			(px = color[idx | pattern[q | 0x05]]) != 0xff && (data[p + 0x705] = px);
			(px = color[idx | pattern[q | 0x06]]) != 0xff && (data[p + 0x706] = px);
			(px = color[idx | pattern[q | 0x07]]) != 0xff && (data[p + 0x707] = px);
			break;
		case 2: // H反転
			(px = color[idx | pattern[q | 0x07]]) != 0xff && (data[p + 0x000] = px);
			(px = color[idx | pattern[q | 0x06]]) != 0xff && (data[p + 0x001] = px);
			(px = color[idx | pattern[q | 0x05]]) != 0xff && (data[p + 0x002] = px);
			(px = color[idx | pattern[q | 0x04]]) != 0xff && (data[p + 0x003] = px);
			(px = color[idx | pattern[q | 0x03]]) != 0xff && (data[p + 0x004] = px);
			(px = color[idx | pattern[q | 0x02]]) != 0xff && (data[p + 0x005] = px);
			(px = color[idx | pattern[q | 0x01]]) != 0xff && (data[p + 0x006] = px);
			(px = color[idx | pattern[q | 0x00]]) != 0xff && (data[p + 0x007] = px);
			(px = color[idx | pattern[q | 0x0f]]) != 0xff && (data[p + 0x100] = px);
			(px = color[idx | pattern[q | 0x0e]]) != 0xff && (data[p + 0x101] = px);
			(px = color[idx | pattern[q | 0x0d]]) != 0xff && (data[p + 0x102] = px);
			(px = color[idx | pattern[q | 0x0c]]) != 0xff && (data[p + 0x103] = px);
			(px = color[idx | pattern[q | 0x0b]]) != 0xff && (data[p + 0x104] = px);
			(px = color[idx | pattern[q | 0x0a]]) != 0xff && (data[p + 0x105] = px);
			(px = color[idx | pattern[q | 0x09]]) != 0xff && (data[p + 0x106] = px);
			(px = color[idx | pattern[q | 0x08]]) != 0xff && (data[p + 0x107] = px);
			(px = color[idx | pattern[q | 0x17]]) != 0xff && (data[p + 0x200] = px);
			(px = color[idx | pattern[q | 0x16]]) != 0xff && (data[p + 0x201] = px);
			(px = color[idx | pattern[q | 0x15]]) != 0xff && (data[p + 0x202] = px);
			(px = color[idx | pattern[q | 0x14]]) != 0xff && (data[p + 0x203] = px);
			(px = color[idx | pattern[q | 0x13]]) != 0xff && (data[p + 0x204] = px);
			(px = color[idx | pattern[q | 0x12]]) != 0xff && (data[p + 0x205] = px);
			(px = color[idx | pattern[q | 0x11]]) != 0xff && (data[p + 0x206] = px);
			(px = color[idx | pattern[q | 0x10]]) != 0xff && (data[p + 0x207] = px);
			(px = color[idx | pattern[q | 0x1f]]) != 0xff && (data[p + 0x300] = px);
			(px = color[idx | pattern[q | 0x1e]]) != 0xff && (data[p + 0x301] = px);
			(px = color[idx | pattern[q | 0x1d]]) != 0xff && (data[p + 0x302] = px);
			(px = color[idx | pattern[q | 0x1c]]) != 0xff && (data[p + 0x303] = px);
			(px = color[idx | pattern[q | 0x1b]]) != 0xff && (data[p + 0x304] = px);
			(px = color[idx | pattern[q | 0x1a]]) != 0xff && (data[p + 0x305] = px);
			(px = color[idx | pattern[q | 0x19]]) != 0xff && (data[p + 0x306] = px);
			(px = color[idx | pattern[q | 0x18]]) != 0xff && (data[p + 0x307] = px);
			(px = color[idx | pattern[q | 0x27]]) != 0xff && (data[p + 0x400] = px);
			(px = color[idx | pattern[q | 0x26]]) != 0xff && (data[p + 0x401] = px);
			(px = color[idx | pattern[q | 0x25]]) != 0xff && (data[p + 0x402] = px);
			(px = color[idx | pattern[q | 0x24]]) != 0xff && (data[p + 0x403] = px);
			(px = color[idx | pattern[q | 0x23]]) != 0xff && (data[p + 0x404] = px);
			(px = color[idx | pattern[q | 0x22]]) != 0xff && (data[p + 0x405] = px);
			(px = color[idx | pattern[q | 0x21]]) != 0xff && (data[p + 0x406] = px);
			(px = color[idx | pattern[q | 0x20]]) != 0xff && (data[p + 0x407] = px);
			(px = color[idx | pattern[q | 0x2f]]) != 0xff && (data[p + 0x500] = px);
			(px = color[idx | pattern[q | 0x2e]]) != 0xff && (data[p + 0x501] = px);
			(px = color[idx | pattern[q | 0x2d]]) != 0xff && (data[p + 0x502] = px);
			(px = color[idx | pattern[q | 0x2c]]) != 0xff && (data[p + 0x503] = px);
			(px = color[idx | pattern[q | 0x2b]]) != 0xff && (data[p + 0x504] = px);
			(px = color[idx | pattern[q | 0x2a]]) != 0xff && (data[p + 0x505] = px);
			(px = color[idx | pattern[q | 0x29]]) != 0xff && (data[p + 0x506] = px);
			(px = color[idx | pattern[q | 0x28]]) != 0xff && (data[p + 0x507] = px);
			(px = color[idx | pattern[q | 0x37]]) != 0xff && (data[p + 0x600] = px);
			(px = color[idx | pattern[q | 0x36]]) != 0xff && (data[p + 0x601] = px);
			(px = color[idx | pattern[q | 0x35]]) != 0xff && (data[p + 0x602] = px);
			(px = color[idx | pattern[q | 0x34]]) != 0xff && (data[p + 0x603] = px);
			(px = color[idx | pattern[q | 0x33]]) != 0xff && (data[p + 0x604] = px);
			(px = color[idx | pattern[q | 0x32]]) != 0xff && (data[p + 0x605] = px);
			(px = color[idx | pattern[q | 0x31]]) != 0xff && (data[p + 0x606] = px);
			(px = color[idx | pattern[q | 0x30]]) != 0xff && (data[p + 0x607] = px);
			(px = color[idx | pattern[q | 0x3f]]) != 0xff && (data[p + 0x700] = px);
			(px = color[idx | pattern[q | 0x3e]]) != 0xff && (data[p + 0x701] = px);
			(px = color[idx | pattern[q | 0x3d]]) != 0xff && (data[p + 0x702] = px);
			(px = color[idx | pattern[q | 0x3c]]) != 0xff && (data[p + 0x703] = px);
			(px = color[idx | pattern[q | 0x3b]]) != 0xff && (data[p + 0x704] = px);
			(px = color[idx | pattern[q | 0x3a]]) != 0xff && (data[p + 0x705] = px);
			(px = color[idx | pattern[q | 0x39]]) != 0xff && (data[p + 0x706] = px);
			(px = color[idx | pattern[q | 0x38]]) != 0xff && (data[p + 0x707] = px);
			break;
		case 3: // HV反転
			(px = color[idx | pattern[q | 0x3f]]) != 0xff && (data[p + 0x000] = px);
			(px = color[idx | pattern[q | 0x3e]]) != 0xff && (data[p + 0x001] = px);
			(px = color[idx | pattern[q | 0x3d]]) != 0xff && (data[p + 0x002] = px);
			(px = color[idx | pattern[q | 0x3c]]) != 0xff && (data[p + 0x003] = px);
			(px = color[idx | pattern[q | 0x3b]]) != 0xff && (data[p + 0x004] = px);
			(px = color[idx | pattern[q | 0x3a]]) != 0xff && (data[p + 0x005] = px);
			(px = color[idx | pattern[q | 0x39]]) != 0xff && (data[p + 0x006] = px);
			(px = color[idx | pattern[q | 0x38]]) != 0xff && (data[p + 0x007] = px);
			(px = color[idx | pattern[q | 0x37]]) != 0xff && (data[p + 0x100] = px);
			(px = color[idx | pattern[q | 0x36]]) != 0xff && (data[p + 0x101] = px);
			(px = color[idx | pattern[q | 0x35]]) != 0xff && (data[p + 0x102] = px);
			(px = color[idx | pattern[q | 0x34]]) != 0xff && (data[p + 0x103] = px);
			(px = color[idx | pattern[q | 0x33]]) != 0xff && (data[p + 0x104] = px);
			(px = color[idx | pattern[q | 0x32]]) != 0xff && (data[p + 0x105] = px);
			(px = color[idx | pattern[q | 0x31]]) != 0xff && (data[p + 0x106] = px);
			(px = color[idx | pattern[q | 0x30]]) != 0xff && (data[p + 0x107] = px);
			(px = color[idx | pattern[q | 0x2f]]) != 0xff && (data[p + 0x200] = px);
			(px = color[idx | pattern[q | 0x2e]]) != 0xff && (data[p + 0x201] = px);
			(px = color[idx | pattern[q | 0x2d]]) != 0xff && (data[p + 0x202] = px);
			(px = color[idx | pattern[q | 0x2c]]) != 0xff && (data[p + 0x203] = px);
			(px = color[idx | pattern[q | 0x2b]]) != 0xff && (data[p + 0x204] = px);
			(px = color[idx | pattern[q | 0x2a]]) != 0xff && (data[p + 0x205] = px);
			(px = color[idx | pattern[q | 0x29]]) != 0xff && (data[p + 0x206] = px);
			(px = color[idx | pattern[q | 0x28]]) != 0xff && (data[p + 0x207] = px);
			(px = color[idx | pattern[q | 0x27]]) != 0xff && (data[p + 0x300] = px);
			(px = color[idx | pattern[q | 0x26]]) != 0xff && (data[p + 0x301] = px);
			(px = color[idx | pattern[q | 0x25]]) != 0xff && (data[p + 0x302] = px);
			(px = color[idx | pattern[q | 0x24]]) != 0xff && (data[p + 0x303] = px);
			(px = color[idx | pattern[q | 0x23]]) != 0xff && (data[p + 0x304] = px);
			(px = color[idx | pattern[q | 0x22]]) != 0xff && (data[p + 0x305] = px);
			(px = color[idx | pattern[q | 0x21]]) != 0xff && (data[p + 0x306] = px);
			(px = color[idx | pattern[q | 0x20]]) != 0xff && (data[p + 0x307] = px);
			(px = color[idx | pattern[q | 0x1f]]) != 0xff && (data[p + 0x400] = px);
			(px = color[idx | pattern[q | 0x1e]]) != 0xff && (data[p + 0x401] = px);
			(px = color[idx | pattern[q | 0x1d]]) != 0xff && (data[p + 0x402] = px);
			(px = color[idx | pattern[q | 0x1c]]) != 0xff && (data[p + 0x403] = px);
			(px = color[idx | pattern[q | 0x1b]]) != 0xff && (data[p + 0x404] = px);
			(px = color[idx | pattern[q | 0x1a]]) != 0xff && (data[p + 0x405] = px);
			(px = color[idx | pattern[q | 0x19]]) != 0xff && (data[p + 0x406] = px);
			(px = color[idx | pattern[q | 0x18]]) != 0xff && (data[p + 0x407] = px);
			(px = color[idx | pattern[q | 0x17]]) != 0xff && (data[p + 0x500] = px);
			(px = color[idx | pattern[q | 0x16]]) != 0xff && (data[p + 0x501] = px);
			(px = color[idx | pattern[q | 0x15]]) != 0xff && (data[p + 0x502] = px);
			(px = color[idx | pattern[q | 0x14]]) != 0xff && (data[p + 0x503] = px);
			(px = color[idx | pattern[q | 0x13]]) != 0xff && (data[p + 0x504] = px);
			(px = color[idx | pattern[q | 0x12]]) != 0xff && (data[p + 0x505] = px);
			(px = color[idx | pattern[q | 0x11]]) != 0xff && (data[p + 0x506] = px);
			(px = color[idx | pattern[q | 0x10]]) != 0xff && (data[p + 0x507] = px);
			(px = color[idx | pattern[q | 0x0f]]) != 0xff && (data[p + 0x600] = px);
			(px = color[idx | pattern[q | 0x0e]]) != 0xff && (data[p + 0x601] = px);
			(px = color[idx | pattern[q | 0x0d]]) != 0xff && (data[p + 0x602] = px);
			(px = color[idx | pattern[q | 0x0c]]) != 0xff && (data[p + 0x603] = px);
			(px = color[idx | pattern[q | 0x0b]]) != 0xff && (data[p + 0x604] = px);
			(px = color[idx | pattern[q | 0x0a]]) != 0xff && (data[p + 0x605] = px);
			(px = color[idx | pattern[q | 0x09]]) != 0xff && (data[p + 0x606] = px);
			(px = color[idx | pattern[q | 0x08]]) != 0xff && (data[p + 0x607] = px);
			(px = color[idx | pattern[q | 0x07]]) != 0xff && (data[p + 0x700] = px);
			(px = color[idx | pattern[q | 0x06]]) != 0xff && (data[p + 0x701] = px);
			(px = color[idx | pattern[q | 0x05]]) != 0xff && (data[p + 0x702] = px);
			(px = color[idx | pattern[q | 0x04]]) != 0xff && (data[p + 0x703] = px);
			(px = color[idx | pattern[q | 0x03]]) != 0xff && (data[p + 0x704] = px);
			(px = color[idx | pattern[q | 0x02]]) != 0xff && (data[p + 0x705] = px);
			(px = color[idx | pattern[q | 0x01]]) != 0xff && (data[p + 0x706] = px);
			(px = color[idx | pattern[q | 0x00]]) != 0xff && (data[p + 0x707] = px);
			break;
		}
	}

	void xfer16x16(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = src << 8 & 0x1ff00;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[src++]]])
					data[dst] = px;
	}

	void xfer16x16V(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[src++]]])
					data[dst] = px;
	}

	void xfer16x16H(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[--src]]])
					data[dst] = px;
	}

	void xfer16x16HV(int *data, int dst, int src, int cat) {
		const int idx = src >> 5 & 0x3f0;
		int px;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 304 * 0x100)
			return;
		src = (src << 8 & 0x1ff00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if (opaque[cat][px = OBJCOLOR[idx | obj[--src]]])
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new C30(49152000 / 1024);
	}
};

#endif //PAC_LAND_H
