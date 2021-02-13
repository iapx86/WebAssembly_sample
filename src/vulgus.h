/*
 *
 *	Vulgus
 *
 */

#ifndef VULGUS_H
#define VULGUS_H

#include <array>
#include "z80.h"
#include "ay-3-8910.h"
#include "utils.h"
using namespace std;

enum {
	BONUS_10000_50000, BONUS_10000_60000, BONUS_10000_70000, BONUS_20000_60000,
	BONUS_20000_70000, BONUS_20000_80000, BONUS_30000_70000, BONUS_NONE,
};

struct Vulgus {
	static array<uint8_t, 0xa000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x2000> FG;
	static array<uint8_t, 0xc000> BG;
	static array<uint8_t, 0x8000> OBJ;
	static array<uint8_t, 0x100> RED, GREEN, BLUE, FGCOLOR, BGCOLOR, OBJCOLOR;

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
	bool fTurbo = false;
	int nBonus = BONUS_20000_60000;
	int nLife = 3;

	array<uint8_t, 0x2100> ram = {};
	array<uint8_t, 0x800> ram2 = {};
	array<uint8_t, 5> in = {0xff, 0xff, 0xff, 0xff, 0x7f};
	struct {
		int addr = 0;
	} psg[2];
	int command = 0;
	bool cpu_irq = false;

	array<uint8_t, 0x8000> fg;
	array<uint8_t, 0x20000> bg;
	array<uint8_t, 0x10000> obj;
	array<uint8_t, 0x100> fgcolor;
	array<uint8_t, 0x100> objcolor;
	array<int, 0x100> rgb;
	int hScroll = 0;
	int vScroll = 0;
	int palette = 0;
	int frame = 0;

	Z80 cpu, cpu2;
	IntTimer timer;

	Vulgus() : cpu(12000000 / 4), cpu2(12000000 / 4), timer(8 * 60) {
		// CPU周りの初期化
		for (int i = 0; i < 0xa0; i++)
			cpu.memorymap[i].base = &PRG1[i << 8];
		cpu.memorymap[0xc0].read = [&](int addr) -> int { return (addr &= 0xff) < 5 ? in[addr] : 0xff; };
		cpu.memorymap[0xc8].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(command = data);
			case 2:
				return void(hScroll = hScroll & 0xff00 | data);
			case 3:
				return void(vScroll = vScroll & 0xff00 | data);
			case 4:
				return data & 0x10 ? cpu2.disable() : cpu2.enable();
			case 5:
				return void(palette = data << 6 & 0xc0);
			}
		};
		cpu.memorymap[0xc9].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 2:
				return void(hScroll = hScroll & 0xff | data << 8);
			case 3:
				return void(vScroll = vScroll & 0xff | data << 8);
			}
		};
		cpu.memorymap[0xcc].base = &ram[0];
		cpu.memorymap[0xcc].write = nullptr;
		for (int i = 0; i < 0x20; i++) {
			cpu.memorymap[0xd0 + i].base = &ram[1 + i << 8];
			cpu.memorymap[0xd0 + i].write = nullptr;
		}

		cpu.check_interrupt = [&]() { return cpu_irq && cpu.interrupt(0xd7) && (cpu_irq = false, true); };

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = &PRG2[i << 8];
		for (int i = 0; i < 8; i++) {
			cpu2.memorymap[0x40 + i].base = &ram2[i << 8];
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		cpu2.memorymap[0x60].read = [&](int addr) { return addr & 0xff ? 0xff : command; };
		cpu2.memorymap[0x80].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(psg[0].addr = data);
			case 1:
				return sound0->write(psg[0].addr, data);
			}
		};
		cpu2.memorymap[0xc0].write = [&](int addr, int data) {
			switch (addr & 0xff) {
			case 0:
				return void(psg[1].addr = data);
			case 1:
				return sound1->write(psg[1].addr, data);
			}
		};

		// Videoの初期化
		fg.fill(3), bg.fill(7), obj.fill(15);
		convertGFX(&fg[0], &FG[0], 512, {seq8(0, 16)}, {rseq4(8, 1), rseq4(0, 1)}, {4, 0}, 16);
		convertGFX(&bg[0], &BG[0], 512, {seq16(0, 8)}, {rseq8(128, 1), rseq8(0, 1)}, {0, 0x20000, 0x40000}, 32);
		convertGFX(&obj[0], &OBJ[0], 256, {seq16(0, 16)}, {rseq4(264, 1), rseq4(256, 1), rseq4(8, 1), rseq4(0, 1)}, {0x20004, 0x20000, 4, 0}, 64);
		for (int i = 0; i < fgcolor.size(); i++)
			fgcolor[i] = 0x20 | FGCOLOR[i];
		for (int i = 0; i < objcolor.size(); i++)
			objcolor[i] = 0x10 | OBJCOLOR[i];
		for (int i = 0; i < rgb.size(); i++)
			rgb[i] = 0xff000000 | BLUE[i] * 255 / 15 << 16 | GREEN[i] * 255 / 15 << 8 | RED[i] * 255 / 15;
	}

	Vulgus *execute(DoubleTimer& audio, double rate_correction) {
		constexpr int tick_rate = 384000, tick_max = tick_rate / 60;
		cpu_irq = true;
		for (int i = 0; i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			timer.execute(tick_rate, [&]() {
				cpu2.interrupt();
			});
			sound0->execute(tick_rate, rate_correction);
			sound1->execute(tick_rate, rate_correction);
			audio.execute(tick_rate, rate_correction);
		}
		return this;
	}

	void reset() {
		fReset = true;
	}

	Vulgus *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nLife) {
			case 1:
				in[3] = in[3] & ~3 | 1;
				break;
			case 2:
				in[3] = in[3] & ~3 | 2;
				break;
			case 3:
				in[3] |= 3;
				break;
			case 5:
				in[3] &= ~3;
				break;
			}
			switch (nBonus) {
			case BONUS_10000_50000:
				in[4] = in[4] & ~0x70 | 0x30;
				break;
			case BONUS_10000_60000:
				in[4] = in[4] & ~0x70 | 0x50;
				break;
			case BONUS_10000_70000:
				in[4] = in[4] & ~0x70 | 0x10;
				break;
			case BONUS_20000_60000:
				in[4] |= 0x70;
				break;
			case BONUS_20000_70000:
				in[4] = in[4] & ~0x70 | 0x60;
				break;
			case BONUS_20000_80000:
				in[4] = in[4] & ~0x70 | 0x20;
				break;
			case BONUS_30000_70000:
				in[4] = in[4] & ~0x70 | 0x40;
				break;
			case BONUS_NONE:
				in[4] &= ~0x70;
				break;
			}
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu_irq = false;
			cpu.reset();
			cpu2.disable();
		}
		return this;
	}

	Vulgus *updateInput() {
		in[0] = in[0] & ~0x13 | !fCoin << 4 | !fStart1P << 0 | !fStart2P << 1;
		fCoin -= fCoin != 0, fStart1P -= fStart1P != 0, fStart2P -= fStart2P != 0;
		fTurbo && frame & 1 && (in[1] ^= 1 << 4);
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
		in[1] = in[1] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void right(bool fDown) {
		in[1] = in[1] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void down(bool fDown) {
		in[1] = in[1] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void left(bool fDown) {
		in[1] = in[1] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void triggerA(bool fDown) {
		in[1] = in[1] & ~(1 << 4) | !fDown << 4;
	}

	void triggerB(bool fDown) {
		in[1] = in[1] & ~(1 << 5) | !fDown << 5;
	}

	void triggerY(bool fDown) {
		!(fTurbo = fDown) && (in[1] |= 1 << 4);
	}

	void makeBitmap(int *data) {
		frame++;

		// bg描画
		int p = 256 * 256 + 16 - (16 + hScroll & 0x0f) + (vScroll & 0x0f) * 256;
		for (int k = 16 + hScroll >> 4 & 0x1f | vScroll << 1 & 0x3e0, i = 0; i < 17; k = k + 0x11 & 0x1f | k + 0x20 & 0x3e0, p -= 15 * 16 + 256 * 16, i++)
			for (int j = 0; j < 15; k = k + 1 & 0x1f | k & 0x3e0, p += 16, j++)
				xfer16x16x3(data, p, 0x900 + k);

		// obj描画
		for (int k = 0x7c, i = 32; i != 0; k -= 4, --i) {
			const int x = ram[k + 2];
			const int y = 256 - ram[k + 3];
			const int src = ram[k] | ram[k + 1] << 8;
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
		for (int k = 0x140, i = 0; i < 28; p += 256 * 8 * 32 + 8, i++)
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

		(px = fgcolor[idx | fg[q | 0x00]]) != 0x2f && (data[p + 0x000] = px);
		(px = fgcolor[idx | fg[q | 0x01]]) != 0x2f && (data[p + 0x001] = px);
		(px = fgcolor[idx | fg[q | 0x02]]) != 0x2f && (data[p + 0x002] = px);
		(px = fgcolor[idx | fg[q | 0x03]]) != 0x2f && (data[p + 0x003] = px);
		(px = fgcolor[idx | fg[q | 0x04]]) != 0x2f && (data[p + 0x004] = px);
		(px = fgcolor[idx | fg[q | 0x05]]) != 0x2f && (data[p + 0x005] = px);
		(px = fgcolor[idx | fg[q | 0x06]]) != 0x2f && (data[p + 0x006] = px);
		(px = fgcolor[idx | fg[q | 0x07]]) != 0x2f && (data[p + 0x007] = px);
		(px = fgcolor[idx | fg[q | 0x08]]) != 0x2f && (data[p + 0x100] = px);
		(px = fgcolor[idx | fg[q | 0x09]]) != 0x2f && (data[p + 0x101] = px);
		(px = fgcolor[idx | fg[q | 0x0a]]) != 0x2f && (data[p + 0x102] = px);
		(px = fgcolor[idx | fg[q | 0x0b]]) != 0x2f && (data[p + 0x103] = px);
		(px = fgcolor[idx | fg[q | 0x0c]]) != 0x2f && (data[p + 0x104] = px);
		(px = fgcolor[idx | fg[q | 0x0d]]) != 0x2f && (data[p + 0x105] = px);
		(px = fgcolor[idx | fg[q | 0x0e]]) != 0x2f && (data[p + 0x106] = px);
		(px = fgcolor[idx | fg[q | 0x0f]]) != 0x2f && (data[p + 0x107] = px);
		(px = fgcolor[idx | fg[q | 0x10]]) != 0x2f && (data[p + 0x200] = px);
		(px = fgcolor[idx | fg[q | 0x11]]) != 0x2f && (data[p + 0x201] = px);
		(px = fgcolor[idx | fg[q | 0x12]]) != 0x2f && (data[p + 0x202] = px);
		(px = fgcolor[idx | fg[q | 0x13]]) != 0x2f && (data[p + 0x203] = px);
		(px = fgcolor[idx | fg[q | 0x14]]) != 0x2f && (data[p + 0x204] = px);
		(px = fgcolor[idx | fg[q | 0x15]]) != 0x2f && (data[p + 0x205] = px);
		(px = fgcolor[idx | fg[q | 0x16]]) != 0x2f && (data[p + 0x206] = px);
		(px = fgcolor[idx | fg[q | 0x17]]) != 0x2f && (data[p + 0x207] = px);
		(px = fgcolor[idx | fg[q | 0x18]]) != 0x2f && (data[p + 0x300] = px);
		(px = fgcolor[idx | fg[q | 0x19]]) != 0x2f && (data[p + 0x301] = px);
		(px = fgcolor[idx | fg[q | 0x1a]]) != 0x2f && (data[p + 0x302] = px);
		(px = fgcolor[idx | fg[q | 0x1b]]) != 0x2f && (data[p + 0x303] = px);
		(px = fgcolor[idx | fg[q | 0x1c]]) != 0x2f && (data[p + 0x304] = px);
		(px = fgcolor[idx | fg[q | 0x1d]]) != 0x2f && (data[p + 0x305] = px);
		(px = fgcolor[idx | fg[q | 0x1e]]) != 0x2f && (data[p + 0x306] = px);
		(px = fgcolor[idx | fg[q | 0x1f]]) != 0x2f && (data[p + 0x307] = px);
		(px = fgcolor[idx | fg[q | 0x20]]) != 0x2f && (data[p + 0x400] = px);
		(px = fgcolor[idx | fg[q | 0x21]]) != 0x2f && (data[p + 0x401] = px);
		(px = fgcolor[idx | fg[q | 0x22]]) != 0x2f && (data[p + 0x402] = px);
		(px = fgcolor[idx | fg[q | 0x23]]) != 0x2f && (data[p + 0x403] = px);
		(px = fgcolor[idx | fg[q | 0x24]]) != 0x2f && (data[p + 0x404] = px);
		(px = fgcolor[idx | fg[q | 0x25]]) != 0x2f && (data[p + 0x405] = px);
		(px = fgcolor[idx | fg[q | 0x26]]) != 0x2f && (data[p + 0x406] = px);
		(px = fgcolor[idx | fg[q | 0x27]]) != 0x2f && (data[p + 0x407] = px);
		(px = fgcolor[idx | fg[q | 0x28]]) != 0x2f && (data[p + 0x500] = px);
		(px = fgcolor[idx | fg[q | 0x29]]) != 0x2f && (data[p + 0x501] = px);
		(px = fgcolor[idx | fg[q | 0x2a]]) != 0x2f && (data[p + 0x502] = px);
		(px = fgcolor[idx | fg[q | 0x2b]]) != 0x2f && (data[p + 0x503] = px);
		(px = fgcolor[idx | fg[q | 0x2c]]) != 0x2f && (data[p + 0x504] = px);
		(px = fgcolor[idx | fg[q | 0x2d]]) != 0x2f && (data[p + 0x505] = px);
		(px = fgcolor[idx | fg[q | 0x2e]]) != 0x2f && (data[p + 0x506] = px);
		(px = fgcolor[idx | fg[q | 0x2f]]) != 0x2f && (data[p + 0x507] = px);
		(px = fgcolor[idx | fg[q | 0x30]]) != 0x2f && (data[p + 0x600] = px);
		(px = fgcolor[idx | fg[q | 0x31]]) != 0x2f && (data[p + 0x601] = px);
		(px = fgcolor[idx | fg[q | 0x32]]) != 0x2f && (data[p + 0x602] = px);
		(px = fgcolor[idx | fg[q | 0x33]]) != 0x2f && (data[p + 0x603] = px);
		(px = fgcolor[idx | fg[q | 0x34]]) != 0x2f && (data[p + 0x604] = px);
		(px = fgcolor[idx | fg[q | 0x35]]) != 0x2f && (data[p + 0x605] = px);
		(px = fgcolor[idx | fg[q | 0x36]]) != 0x2f && (data[p + 0x606] = px);
		(px = fgcolor[idx | fg[q | 0x37]]) != 0x2f && (data[p + 0x607] = px);
		(px = fgcolor[idx | fg[q | 0x38]]) != 0x2f && (data[p + 0x700] = px);
		(px = fgcolor[idx | fg[q | 0x39]]) != 0x2f && (data[p + 0x701] = px);
		(px = fgcolor[idx | fg[q | 0x3a]]) != 0x2f && (data[p + 0x702] = px);
		(px = fgcolor[idx | fg[q | 0x3b]]) != 0x2f && (data[p + 0x703] = px);
		(px = fgcolor[idx | fg[q | 0x3c]]) != 0x2f && (data[p + 0x704] = px);
		(px = fgcolor[idx | fg[q | 0x3d]]) != 0x2f && (data[p + 0x705] = px);
		(px = fgcolor[idx | fg[q | 0x3e]]) != 0x2f && (data[p + 0x706] = px);
		(px = fgcolor[idx | fg[q | 0x3f]]) != 0x2f && (data[p + 0x707] = px);
	}

	void xfer16x16x3(int *data, int p, int k) {
		const int idx = ram[k + 0x400] << 3 & 0xf8;
		int i, j, q = (ram[k] | ram[k + 0x400] << 1 & 0x100) << 8;

		switch (ram[k + 0x400] >> 5 & 3) {
		case 0:
			for (i = 16; i != 0; p += 256 - 16, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | BGCOLOR[idx | bg[q++]];
			break;
		case 1:
			for (q += 256 - 16, i = 16; i != 0; p += 256 - 16, q -= 32, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | BGCOLOR[idx | bg[q++]];
			break;
		case 2:
			for (q += 16, i = 16; i != 0; p += 256 - 16, q += 32, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | BGCOLOR[idx | bg[--q]];
			break;
		case 3:
			for (q += 256, i = 16; i != 0; p += 256 - 16, --i)
				for (j = 16; j != 0; --j)
					data[p++] = palette | BGCOLOR[idx | bg[--q]];
			break;
		}
	}

	void xfer16x16x4(int *data, int dst, int src) {
		const int idx = src >> 4 & 0xf0;
		int px, i, j;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240 || (dst & 0x1ff00) == 0 || dst >= 272 * 0x100)
			return;
		for (src = src << 8 & 0xff00, i = 16; i != 0; dst += 256 - 16, --i)
			for (j = 16; j != 0; dst++, --j)
				if ((px = objcolor[idx | obj[src++]]) != 0x1f)
					data[dst] = px;
	}

	static void init(int rate) {
		sound0 = new AY_3_8910(12000000 / 8);
		sound1 = new AY_3_8910(12000000 / 8);
		Z80::init();
	}
};

#endif //VULGUS_H
