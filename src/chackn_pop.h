/*
 *
 *	Chack'n Pop
 *
 */

#ifndef CHACKN_POP_H
#define CHACKN_POP_H

#include <cmath>
#include <array>
#include "z80.h"
#include "mc6805.h"
#include "ay-3-8910.h"
#include "utils.h"
using namespace std;

struct ChacknPop {
	static array<uint8_t, 0xa000> PRG1;
	static array<uint8_t, 0x800> PRG2;
	static array<uint8_t, 0x4000> OBJ, BG;
	static array<uint8_t, 0x400> RGB_L, RGB_H;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static AY_3_8910 *sound0, *sound1;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nLife = 3;
	int nBonus = 20000;

	array<uint8_t, 0xd00> ram = {};
	array<uint8_t, 0x80> ram2 = {};
	array<uint8_t, 6> in = {0x7d, 0xff, 0xff, 0xff, 0, 0x4f};
	struct {
		int addr = 0;
	} psg[2];
	int mcu_command = 0;
	int mcu_result = 0;
	int mcu_flag = 0;

	array<uint8_t, 0x8000> vram = {};
	array<uint8_t, 0x10000> bg;
	array<uint8_t, 0x10000> obj;
	array<int, 0x400> rgb;
	array<int, width * height> bitmap;
	bool updated = false;
	int mode = 0;

	Z80 cpu;
	MC6805 mcu;
	Timer<int> timer;

	ChacknPop() : cpu(18000000 / 6), mcu(18000000 / 6 / 4), timer(60) {
		// CPU周りの初期化
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[i].base = &PRG1[i << 8];
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x80 + i].base = &ram[i << 8];
			cpu.memorymap[0x80 + i].write = nullptr;
		}
		cpu.memorymap[0x88].read = [&](int addr) -> int {
			switch (addr & 0xff) {
			case 0x00:
				return mcu_flag &= ~2, mcu_result;
			case 0x01:
				return mcu_flag ^ 1 | 0xfc;
			case 0x04:
			case 0x05:
				return sound0->read(psg[0].addr);
			case 0x06:
			case 0x07:
				return sound1->read(psg[1].addr);
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
				return in[addr & 3];
			case 0x0c:
				return mode;
			}
			return 0xff;
		};
		cpu.memorymap[0x88].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0x00:
				return mcu_command = data, mcu_flag |= 1, void(mcu.irq = true);
			case 0x04:
				return void(psg[0].addr = data);
			case 0x05:
				if ((psg[0].addr & 0x0f) < 0x0e)
					sound0->write(psg[0].addr, data);
				return;
			case 0x06:
				return void(psg[1].addr = data);
			case 0x07:
				return sound1->write(psg[1].addr, data);
			case 0x0c:
				if ((data ^ mode) & 4) {
					const int bank = data << 4 & 0x40;
					for (int i = 0; i < 0x40; i++)
						cpu.memorymap[0xc0 + i].base = &vram[bank + i << 8];
				}
				return void(mode = data);
			}
		};
		for (int i = 0; i < 4; i++) {
			cpu.memorymap[0x90 + i].base = &ram[8 + i << 8];
			cpu.memorymap[0x90 + i].write = nullptr;
		}
		cpu.memorymap[0x98].base = &ram[0xc00];
		cpu.memorymap[0x98].write = nullptr;
		for (int i = 0; i < 0x20; i++)
			cpu.memorymap[0xa0 + i].base = &PRG1[0x80 + i << 8];
		for (int i = 0; i < 0x40; i++) {
			cpu.memorymap[0xc0 + i].base = &vram[i << 8];
			cpu.memorymap[0xc0 + i].write = nullptr;
		}

		mcu.memorymap[0].fetch = [&](int addr) -> int { return addr >= 0x80 ? PRG2[addr] : ram2[addr]; };
		mcu.memorymap[0].read = [&](int addr) -> int {
			if (addr >= 0x80)
				return PRG2[addr];
			switch (addr) {
			case 0:
				return mcu_command;
			case 2:
				return mcu_flag ^ 2 | 0xfc;
			}
			return ram2[addr];
		};
		mcu.memorymap[0].write = [&](int addr, int data) {
			if (addr >= 0x80)
				return;
			if (addr == 1 && ~ram2[1] & data & 2)
				mcu_flag &= ~1, mcu.irq = false;
			if (addr == 1 && ram2[1] & ~data & 4)
				mcu_result = ram2[0], mcu_flag |= 2;
			ram2[addr] = data;
		};
		for (int i = 1; i < 8; i++)
			mcu.memorymap[i].base = &PRG2[i << 8];

		mcu.check_interrupt = [&]() { return mcu.irq && mcu.interrupt(); };

		// Videoの初期化
		bg.fill(3), obj.fill(3), bitmap.fill(0xff000000);
		convertGFX(&bg[0], &BG[0], 1024, {rseq8(0, 8)}, {seq8(0, 1)}, {0, 0x10000}, 8);
		convertGFX(&obj[0], &OBJ[0], 256, {rseq8(128, 8), rseq8(0, 8)}, {seq8(0, 1), seq8(64, 1)}, {0, 0x10000}, 32);
		for (int i = 0; i < rgb.size(); i++) {
			const int e = RGB_H[i] << 4 | RGB_L[i];
			rgb[i] = 0xff000000 | (e >> 6) * 255 / 3 << 16 | (e >> 3 & 7) * 255 / 7 << 8 | (e & 7) * 255 / 7;
		}
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			mcu.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), cpu.interrupt(); });
			sound0->execute(tick_rate);
			sound1->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	ChacknPop *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 6:
				in[5] &= ~0x18;
				break;
			case 3:
				in[5] = in[5] & ~0x18 | 8;
				break;
			case 2:
				in[5] = in[5] & ~0x18 | 0x10;
				break;
			case 1:
				in[5] |= 0x18;
				break;
			}
			switch (nBonus) {
			case 80000:
				in[5] &= ~3;
				break;
			case 60000:
				in[5] = in[5] & ~3 | 1;
				break;
			case 40000:
				in[5] = in[5] & ~3 | 2;
				break;
			case 20000:
				in[5] |= 3;
				break;
			}
			sound0->write(0x0e, in[4]);
			sound0->write(0x0f, in[5]);
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			mcu.reset();
			mcu_command = 0;
			mcu_result = 0;
			mcu_flag = 0;
		}
		return this;
	}

	ChacknPop *updateInput() {
		in[2] = in[2] & ~0x34 | !fCoin << 2 | !fStart1P << 4 | !fStart2P << 5;
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
		in[1] = in[1] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void right(bool fDown) {
		in[1] = in[1] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void down(bool fDown) {
		in[1] = in[1] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void left(bool fDown) {
		in[1] = in[1] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void triggerA(bool fDown) {
		in[1] = in[1] & ~(1 << 4) | !fDown << 4;
	}

	void triggerB(bool fDown) {
		in[1] = in[1] & ~(1 << 5) | !fDown << 5;
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// bg描画
		int p = 256 * 8 * 2 + 232;
		for (int k = 0x840, i = 0; i < 28; p -= 256 * 8 * 32 + 8, i++)
			for (int j = 0; j < 32; k++, p += 256 * 8, j++)
				xfer8x8(bitmap.data(), p, k);

		// obj描画
		for (int k = 0xc40, i = 48; i != 0; k += 4, --i) {
			const int x = ram[k] - 1 & 0xff, y = ram[k + 3] + 16;
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

		// bitmap描画
		p = 256 * 8 * 33 + 16;
		for (int k = 0x0200, i = 256 >> 3; i != 0; --i) {
			for (int j = 224 >> 2; j != 0; k += 0x80, p += 4, --j) {
				int p0 = vram[k], p1 = vram[0x2000 + k], p2 = vram[0x4000 + k], p3 = vram[0x6000 + k];
				bitmap[p + 7 * 256] = rgb[p0 << 9 & 0x200 | p2 << 8 & 0x100 | p1 << 7 & 0x80 | p3 << 6 & 0x40 | bitmap[p + 7 * 256]];
				bitmap[p + 6 * 256] = rgb[p0 << 8 & 0x200 | p2 << 7 & 0x100 | p1 << 6 & 0x80 | p3 << 5 & 0x40 | bitmap[p + 6 * 256]];
				bitmap[p + 5 * 256] = rgb[p0 << 7 & 0x200 | p2 << 6 & 0x100 | p1 << 5 & 0x80 | p3 << 4 & 0x40 | bitmap[p + 5 * 256]];
				bitmap[p + 4 * 256] = rgb[p0 << 6 & 0x200 | p2 << 5 & 0x100 | p1 << 4 & 0x80 | p3 << 3 & 0x40 | bitmap[p + 4 * 256]];
				bitmap[p + 3 * 256] = rgb[p0 << 5 & 0x200 | p2 << 4 & 0x100 | p1 << 3 & 0x80 | p3 << 2 & 0x40 | bitmap[p + 3 * 256]];
				bitmap[p + 2 * 256] = rgb[p0 << 4 & 0x200 | p2 << 3 & 0x100 | p1 << 2 & 0x80 | p3 << 1 & 0x40 | bitmap[p + 2 * 256]];
				bitmap[p + 256] = rgb[p0 << 3 & 0x200 | p2 << 2 & 0x100 | p1 << 1 & 0x80 | p3 << 0 & 0x40 | bitmap[p + 256]];
				bitmap[p] = rgb[p0 << 2 & 0x200 | p2 << 1 & 0x100 | p1 << 0 & 0x80 | p3 >> 1 & 0x40 | bitmap[p]];
				p0 = vram[0x20 + k], p1 = vram[0x2020 + k], p2 = vram[0x4020 + k], p3 = vram[0x6020 + k];
				bitmap[p + 1 + 7 * 256] = rgb[p0 << 9 & 0x200 | p2 << 8 & 0x100 | p1 << 7 & 0x80 | p3 << 6 & 0x40 | bitmap[p + 1 + 7 * 256]];
				bitmap[p + 1 + 6 * 256] = rgb[p0 << 8 & 0x200 | p2 << 7 & 0x100 | p1 << 6 & 0x80 | p3 << 5 & 0x40 | bitmap[p + 1 + 6 * 256]];
				bitmap[p + 1 + 5 * 256] = rgb[p0 << 7 & 0x200 | p2 << 6 & 0x100 | p1 << 5 & 0x80 | p3 << 4 & 0x40 | bitmap[p + 1 + 5 * 256]];
				bitmap[p + 1 + 4 * 256] = rgb[p0 << 6 & 0x200 | p2 << 5 & 0x100 | p1 << 4 & 0x80 | p3 << 3 & 0x40 | bitmap[p + 1 + 4 * 256]];
				bitmap[p + 1 + 3 * 256] = rgb[p0 << 5 & 0x200 | p2 << 4 & 0x100 | p1 << 3 & 0x80 | p3 << 2 & 0x40 | bitmap[p + 1 + 3 * 256]];
				bitmap[p + 1 + 2 * 256] = rgb[p0 << 4 & 0x200 | p2 << 3 & 0x100 | p1 << 2 & 0x80 | p3 << 1 & 0x40 | bitmap[p + 1 + 2 * 256]];
				bitmap[p + 1 + 256] = rgb[p0 << 3 & 0x200 | p2 << 2 & 0x100 | p1 << 1 & 0x80 | p3 << 0 & 0x40 | bitmap[p + 1 + 256]];
				bitmap[p + 1] = rgb[p0 << 2 & 0x200 | p2 << 1 & 0x100 | p1 << 0 & 0x80 | p3 >> 1 & 0x40 | bitmap[p + 1]];
				p0 = vram[0x40 + k], p1 = vram[0x2040 + k], p2 = vram[0x4040 + k], p3 = vram[0x6040 + k];
				bitmap[p + 2 + 7 * 256] = rgb[p0 << 9 & 0x200 | p2 << 8 & 0x100 | p1 << 7 & 0x80 | p3 << 6 & 0x40 | bitmap[p + 2 + 7 * 256]];
				bitmap[p + 2 + 6 * 256] = rgb[p0 << 8 & 0x200 | p2 << 7 & 0x100 | p1 << 6 & 0x80 | p3 << 5 & 0x40 | bitmap[p + 2 + 6 * 256]];
				bitmap[p + 2 + 5 * 256] = rgb[p0 << 7 & 0x200 | p2 << 6 & 0x100 | p1 << 5 & 0x80 | p3 << 4 & 0x40 | bitmap[p + 2 + 5 * 256]];
				bitmap[p + 2 + 4 * 256] = rgb[p0 << 6 & 0x200 | p2 << 5 & 0x100 | p1 << 4 & 0x80 | p3 << 3 & 0x40 | bitmap[p + 2 + 4 * 256]];
				bitmap[p + 2 + 3 * 256] = rgb[p0 << 5 & 0x200 | p2 << 4 & 0x100 | p1 << 3 & 0x80 | p3 << 2 & 0x40 | bitmap[p + 2 + 3 * 256]];
				bitmap[p + 2 + 2 * 256] = rgb[p0 << 4 & 0x200 | p2 << 3 & 0x100 | p1 << 2 & 0x80 | p3 << 1 & 0x40 | bitmap[p + 2 + 2 * 256]];
				bitmap[p + 2 + 256] = rgb[p0 << 3 & 0x200 | p2 << 2 & 0x100 | p1 << 1 & 0x80 | p3 << 0 & 0x40 | bitmap[p + 2 + 256]];
				bitmap[p + 2] = rgb[p0 << 2 & 0x200 | p2 << 1 & 0x100 | p1 << 0 & 0x80 | p3 >> 1 & 0x40 | bitmap[p + 2]];
				p0 = vram[0x60 + k], p1 = vram[0x2060 + k], p2 = vram[0x4060 + k], p3 = vram[0x6060 + k];
				bitmap[p + 3 + 7 * 256] = rgb[p0 << 9 & 0x200 | p2 << 8 & 0x100 | p1 << 7 & 0x80 | p3 << 6 & 0x40 | bitmap[p + 3 + 7 * 256]];
				bitmap[p + 3 + 6 * 256] = rgb[p0 << 8 & 0x200 | p2 << 7 & 0x100 | p1 << 6 & 0x80 | p3 << 5 & 0x40 | bitmap[p + 3 + 6 * 256]];
				bitmap[p + 3 + 5 * 256] = rgb[p0 << 7 & 0x200 | p2 << 6 & 0x100 | p1 << 5 & 0x80 | p3 << 4 & 0x40 | bitmap[p + 3 + 5 * 256]];
				bitmap[p + 3 + 4 * 256] = rgb[p0 << 6 & 0x200 | p2 << 5 & 0x100 | p1 << 4 & 0x80 | p3 << 3 & 0x40 | bitmap[p + 3 + 4 * 256]];
				bitmap[p + 3 + 3 * 256] = rgb[p0 << 5 & 0x200 | p2 << 4 & 0x100 | p1 << 3 & 0x80 | p3 << 2 & 0x40 | bitmap[p + 3 + 3 * 256]];
				bitmap[p + 3 + 2 * 256] = rgb[p0 << 4 & 0x200 | p2 << 3 & 0x100 | p1 << 2 & 0x80 | p3 << 1 & 0x40 | bitmap[p + 3 + 2 * 256]];
				bitmap[p + 3 + 256] = rgb[p0 << 3 & 0x200 | p2 << 2 & 0x100 | p1 << 1 & 0x80 | p3 << 0 & 0x40 | bitmap[p + 3 + 256]];
				bitmap[p + 3] = rgb[p0 << 2 & 0x200 | p2 << 1 & 0x100 | p1 << 0 & 0x80 | p3 >> 1 & 0x40 | bitmap[p + 3]];
			}
			k -= 0x20 * 224 - 1;
			p -= 224 + 256 * 8;
		}

		return bitmap.data();
	}

	void xfer8x8(int *data, int p, int k) {
		const int q = (ram[k] ^ (mode & 0x20 && ram[k] >= 0xc0 ? 0x140 : 0) | mode << 2 & 0x200) << 6;
		const int idx = (ram[k] == 0x74 ? ram[0xc0b] : ram[0xc01]) << 2 & 0x1c | 0x20;

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
		src = src << 8 & 0x3f00 | src << 5 & 0xc000;
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
		src = (src << 8 & 0x3f00 | src << 5 & 0xc000) + 256 - 16;
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
		src = (src << 8 & 0x3f00 | src << 5 & 0xc000) + 16;
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
		src = (src << 8 & 0x3f00 | src << 5 & 0xc000) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, --i)
			for (int j = 16; j != 0; dst++, --j)
				if ((px = obj[--src]))
					data[dst] = idx | px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(18000000 / 12);
		sound1 = new AY_3_8910(18000000 / 12);
		Z80::init();
	}
};

#endif //CHACKN_POP_H
