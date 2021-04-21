/*
 *
 *	Elevator Action
 *
 */

#ifndef ELEVATOR_ACTION_H
#define ELEVATOR_ACTION_H

#include <algorithm>
#include <array>
#include "z80.h"
#include "mc6805.h"
#include "ay-3-8910.h"
#include "dac.h"
#include "utils.h"
using namespace std;

enum {
	RANK_EASIEST, RANK_EASY, RANK_NORMAL, RANK_HARD,
};

struct ElevatorAction {
	static array<uint8_t, 0x8000> PRG1;
	static array<uint8_t, 0x2000> PRG2;
	static array<uint8_t, 0x800> PRG3;
	static array<uint8_t, 0x8000> GFX;
	static array<uint8_t, 0x100> PRI;

	static const int cxScreen = 224;
	static const int cyScreen = 256;
	static const int width = 256;
	static const int height = 512;
	static const int xOffset = 16;
	static const int yOffset = 16;
	static const bool rotate = true;

	static AY_3_8910 *sound0, *sound1, *sound2, *sound3;
	static Dac1Ch *sound4;

	bool fReset = false;
	bool fTest = false;
	bool fDIPSwitchChanged = true;
	int fCoin = 0;
	int fStart1P = 0;
	int fStart2P = 0;
	int nBonus = 10000;
	int nLife = 3;
	int nRank = RANK_EASIEST;

	bool fNmiEnable = false;

	array<uint8_t, 0x4b00> ram = {};
	array<uint8_t, 0x400> ram2 = {};
	array<uint8_t, 0x80> ram3 = {};
	array<uint8_t, 8> in = {0xff, 0xff, 0x7f, 0xff, 0xef, 0x0f, 0, 0xff};
	struct {
		int addr = 0;
	} psg[4];
	bool cpu2_irq = false;
	bool cpu2_nmi = false;
	bool cpu2_nmi2 = false;
	int cpu2_command = 0;
	int cpu2_flag = 0;
	int cpu2_flag2 = 0;
	int mcu_command = 0;
	int mcu_result = 0;
	int mcu_flag = 0;

	array<uint8_t, 0x8000> bg;
	array<uint8_t, 0x8000> obj;
	array<int, 0x40> rgb;
	array<array<uint8_t, 4>, 32> pri = {};
	array<uint8_t, width * height> layer[4];
	array<int, width * height> bitmap;
	bool updated = false;
	int priority = 0;
	array<uint8_t, 4> collision = {};
	int gfxaddr = 0;
	array<uint8_t, 6> scroll = {};
	array<uint8_t, 2> colorbank = {};
	int mode = 0;

	Z80 cpu, cpu2;
	MC6805 mcu;
	Timer<int> timer;
	Timer<double> timer2;

	ElevatorAction() : cpu(8000000 / 2), cpu2(6000000 / 2), mcu(3000000 / 4), timer(60), timer2(6000000.0 / 163840) {
		// CPU周りの初期化
		for (int i = 0; i < 0x80; i++)
			cpu.memorymap[i].base = &PRG1[i << 8];
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x80 + i].base = &ram[i << 8];
			cpu.memorymap[0x80 + i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu.memorymap[0x88 + i].read = [&](int addr) { return addr & 1 ? mcu_flag ^ 1 | 0xfc : (mcu_flag &= ~2, mcu_result); };
			cpu.memorymap[0x88 + i].write = [&](int addr, int data) { ~addr & 1 && (mcu_command = data, mcu_flag |= 1, mcu.irq = true); };
		}
		for (int i = 0; i < 0x42; i++) {
			cpu.memorymap[0x90 + i].base = &ram[8 + i << 8];
			cpu.memorymap[0x90 + i].write = nullptr;
		}
		cpu.memorymap[0xd2].read = [&](int addr) -> int { return ram[0x4a00 | addr & 0x7f]; };
		cpu.memorymap[0xd2].write = [&](int addr, int data) { ram[0x4a00 | addr & 0x7f] = data; };
		cpu.memorymap[0xd3].write = [&](int addr, int data) { priority = data; };
		cpu.memorymap[0xd4].read = [&](int addr) -> int {
			int data;
			switch (addr & 0x0f) {
			case 0:
			case 1:
			case 2:
			case 3:
				return collision[addr & 3];
			case 4:
			case 5:
			case 6:
			case 7:
				data = gfxaddr < 0x8000 ? GFX[gfxaddr] : 0;
				gfxaddr = gfxaddr + 1 & 0xffff;
				return data;
			case 8:
			case 9:
			case 0xa:
			case 0xb:
			case 0xc:
			case 0xd:
				return in[addr & 7];
			case 0xf:
				return sound0->read(psg[0].addr);
			}
			return 0xff;
		};
		cpu.memorymap[0xd4].write = [&](int addr, int data) {
			switch (addr & 0xf) {
			case 0xe:
				return void(psg[0].addr = data);
			case 0xf:
				return (psg[0].addr & 0xf) < 0xe ? sound0->write(psg[0].addr, data) : void(0);
			}
		};
		cpu.memorymap[0xd5].write = [&](int addr, int data) {
			switch (addr & 0xf) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				return void(scroll[addr & 7] = data);
			case 6:
			case 7:
				return void(colorbank[addr & 1] = data);
			case 8:
				return collision.fill(0);
			case 9:
				return void(gfxaddr = gfxaddr & 0xff00 | data);
			case 0xa:
				return void(gfxaddr = gfxaddr & 0xff | data << 8);
			case 0xb:
				return cpu2_command = data, cpu2_flag = 1, void(cpu2_nmi = true);
			case 0xc:
				return cpu2_flag2 = data & 1, void(cpu2_nmi2 = (data & 1) != 0);
			}
		};
		cpu.memorymap[0xd6].write = [&](int addr, int data) { mode = data; };

		for (int i = 0; i < 0x20; i++)
			cpu2.memorymap[i].base = &PRG2[i << 8];
		for (int i = 0; i < 4; i++) {
			cpu2.memorymap[0x40 + i].base = &ram2[i << 8];
			cpu2.memorymap[0x40 + i].write = nullptr;
		}
		for (int i = 0; i < 8; i++) {
			cpu2.memorymap[0x48 + i].read = [&](int addr) {
				switch (addr & 7) {
				case 1:
					return sound1->read(psg[1].addr);
				case 3:
					return sound2->read(psg[2].addr);
				case 5:
				case 7:
					return sound3->read(psg[3].addr);
				}
				return 0xff;
			};
			cpu2.memorymap[0x48 + i].write = [&](int addr, int data) {
				switch (addr & 7) {
				case 0:
					return void(psg[1].addr = data);
				case 1:
					(psg[1].addr & 0xf) == 0xe && (sound4->data = (data - 128) / 127.0);
					(psg[1].addr & 0xf) == 0xf && (sound4->vol = (data ^ 255) / 255.0);
					return sound1->write(psg[1].addr, data);
				case 2:
					return void(psg[2].addr = data);
				case 3:
					(psg[2].addr & 0xf) == 0xe && (in[5] = in[5] & 0x0f | data & 0xf0);
					return sound2->write(psg[2].addr, data);
				case 4:
				case 6:
					return void(psg[3].addr = data);
				case 5:
				case 7:
					(psg[3].addr & 0xf) == 0xf && (fNmiEnable = !(data & 1));
					return sound3->write(psg[3].addr, data);
				}
			};
			cpu2.memorymap[0x50 + i].read = [&](int addr) {
				switch (addr & 3) {
				case 0:
					return cpu2_flag = 0, cpu2_command;
				case 1:
					return cpu2_flag << 3 | cpu2_flag2 << 2 | 3;
				}
				return 0xff;
			};
			cpu2.memorymap[0x50 + i].write = [&](int addr, int data) {
				switch (addr & 3) {
				case 0:
					return void(cpu2_command &= 0x7f);
				case 1:
					return void(cpu2_flag2 = 0);
				}
			};
		}

		cpu2.check_interrupt = [&]() {
			if (((fNmiEnable && cpu2_nmi) || cpu2_nmi2) && cpu2.non_maskable_interrupt())
				return cpu2_nmi = cpu2_nmi2 = false, true;
			if (cpu2_irq && cpu2.interrupt())
				return cpu2_irq = false, true;
			return false;
		};

		mcu.memorymap[0].fetch = [&](int addr) -> int { return addr >= 0x80 ? PRG3[addr] : ram3[addr]; };
		mcu.memorymap[0].read = [&](int addr) -> int {
			if (addr >= 0x80)
				return PRG3[addr];
			switch (addr) {
			case 0:
				return mcu_command;
			case 2:
				return mcu_flag ^ 2 | 0xfc;
			}
			return ram3[addr];
		};
		mcu.memorymap[0].write = [&](int addr, int data) {
			if (addr >= 0x80)
				return;
			if (addr == 1 && ~ram3[1] & data & 2)
				mcu_flag &= ~1, mcu.irq = false;
			if (addr == 1 && ram3[1] & ~data & 4)
				mcu_result = ram3[0], mcu_flag |= 2;
			ram3[addr] = data;
		};
		for (int i = 1; i < 8; i++)
			mcu.memorymap[i].base = &PRG3[i << 8];

		mcu.check_interrupt = [&]() { return mcu.irq && mcu.interrupt(); };

		// Videoの初期化
		bitmap.fill(0xff000000);
		for (int i = 0; i < 16; i++)
			for (int mask = 0, j = 3; j >= 0; mask |= 1 << pri[i][j], --j)
				pri[i][j] = PRI[i << 4 & 0xf0 | mask] & 3;
		for (int i = 16; i < 32; i++)
			for (int mask = 0, j = 3; j >= 0; mask |= 1 << pri[i][j], --j)
				pri[i][j] = PRI[i << 4 & 0xf0 | mask] >> 2 & 3;
	}

	void execute(Timer<int>& audio, int length) {
		const int tick_rate = 192000, tick_max = ceil(double(length * tick_rate - audio.frac) / audio.rate);
		auto update = [&]() { makeBitmap(true), updateStatus(), updateInput(); };
		for (int i = 0; !updated && i < tick_max; i++) {
			cpu.execute(tick_rate);
			cpu2.execute(tick_rate);
			mcu.execute(tick_rate);
			timer.execute(tick_rate, [&]() { update(), cpu.interrupt(); });
			timer2.execute(tick_rate, [&]() { cpu2_irq = true; });
			sound0->execute(tick_rate);
			sound1->execute(tick_rate);
			sound2->execute(tick_rate);
			sound3->execute(tick_rate);
			audio.execute(tick_rate);
		}
	}

	void reset() {
		fReset = true;
	}

	ElevatorAction *updateStatus() {
		// DIPスイッチの更新
		if (fDIPSwitchChanged) {
			fDIPSwitchChanged = false;
			switch (nBonus) {
			case 10000:
				in[2] |= 3;
				break;
			case 15000:
				in[2] = in[2] & ~3 | 2;
				break;
			case 20000:
				in[2] = in[2] & ~3 | 1;
				break;
			case 25000:
				in[2] &= ~3;
				break;
			}
			switch (nLife) {
			case 3:
				in[2] |= 0x18;
				break;
			case 4:
				in[2] = in[2] & ~0x18 | 0x10;
				break;
			case 5:
				in[2] = in[2] & ~0x18 | 8;
				break;
			case 6:
				in[2] &= ~0x18;
				break;
			}
			switch (nRank) {
			case RANK_EASIEST:
				in[7] |= 3;
				break;
			case RANK_EASY:
				in[7] = in[7] & ~3 | 2;
				break;
			case RANK_NORMAL:
				in[7] = in[7] & ~3 | 1;
				break;
			case RANK_HARD:
				in[7] &= ~3;
				break;
			}
			sound0->write(0x0e, in[6]);
			sound0->write(0x0f, in[7]);
			if (!fTest)
				fReset = true;
		}

		// リセット処理
		if (fReset) {
			fReset = false;
			cpu.reset();
			cpu2.reset();
			cpu2_irq = false;
			cpu2_nmi = false;
			cpu2_nmi2 = false;
			cpu2_command = 0;
			cpu2_flag = 0;
			cpu2_flag2 = 0;
			mcu.reset();
			mcu_command = 0;
			mcu_result = 0;
			mcu_flag = 0;
		}
		return this;
	}

	ElevatorAction *updateInput() {
		in[3] = in[3] & ~0xe0 | !fCoin << 5 | !fStart1P << 6 | !fStart2P << 7;
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
		in[0] = in[0] & ~(1 << 3) | fDown << 2 | !fDown << 3;
	}

	void right(bool fDown) {
		in[0] = in[0] & ~(1 << 1) | fDown << 0 | !fDown << 1;
	}

	void down(bool fDown) {
		in[0] = in[0] & ~(1 << 2) | fDown << 3 | !fDown << 2;
	}

	void left(bool fDown) {
		in[0] = in[0] & ~(1 << 0) | fDown << 1 | !fDown << 0;
	}

	void triggerA(bool fDown) {
		in[0] = in[0] & ~(1 << 4) | !fDown << 4;
	}

	void triggerB(bool fDown) {
		in[0] = in[0] & ~(1 << 5) | !fDown << 5;
	}

	int *makeBitmap(bool flag) {
		if (!(updated = flag))
			return bitmap.data();

		// 画像データ変換
		auto convertBG = [](uint8_t *dst, uint8_t *src, int n) {
			for (int i = 0; i < n; src += 8, i++)
				for (int j = 0; j < 8; j++)
					for (int k = 7; k >= 0; --k)
						*dst++ = src[k] >> j & 1 | src[k + 0x800] >> j << 1 & 2 | src[k + 0x1000] >> j << 2 & 4;
		};
		convertBG(&bg[0], &ram[0x800], 256);
		convertBG(&bg[0x4000], &ram[0x2000], 256);
		auto convertOBJ = [](uint8_t *dst, uint8_t *src, int n) {
			for (int i = 0; i < n; src += 32, i++) {
				for (int j = 0; j < 8; j++) {
					for (int k = 7; k >= 0; --k)
						*dst++ = src[k + 16] >> j & 1 | src[k + 0x800 + 16] >> j << 1 & 2 | src[k + 0x1000 + 16] >> j << 2 & 4;
					for (int k = 7; k >= 0; --k)
						*dst++ = src[k] >> j & 1 | src[k + 0x800] >> j << 1 & 2 | src[k + 0x1000] >> j << 2 & 4;
				}
				for (int j = 0; j < 8; j++) {
					for (int k = 7; k >= 0; --k)
						*dst++ = src[k + 24] >> j & 1 | src[k + 0x800 + 24] >> j << 1 & 2 | src[k + 0x1000 + 24] >> j << 2 & 4;
					for (int k = 7; k >= 0; --k)
						*dst++ = src[k + 8] >> j & 1 | src[k + 0x800 + 8] >> j << 1 & 2 | src[k + 0x1000 + 8] >> j << 2 & 4;
				}
			}
		};
		convertOBJ(&obj[0], &ram[0x800], 64);
		convertOBJ(&obj[0x4000], &ram[0x2000], 64);
		for (int k = 0x4a00, i = 0; i < rgb.size(); k += 2, i++) {
			const int e = ~(ram[k] << 8 | ram[k + 1]);
			rgb[i] = 0xff000000 | (e & 7) * 255 / 7 << 16 | (e >> 3 & 7) * 255 / 7 << 8 | (e >> 6 & 7) * 255 / 7;
		}

		// 画面クリア
		int p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256, i++)
			fill_n(&bitmap[p], 224, colorbank[1] << 3 & 0x38);

		// bg描画
		if (mode & 0x10) {
			const int color = colorbank[0];
			const int _scroll = -(scroll[0] & 0xf8) + (scroll[0] + 3 & 7) + 8;
			for (int k = 0x3c00; k < 0x4000; k++) {
				const int x = (~k >> 2 & 0xf8) + scroll[1] + ram[0x4800 | k & 0x1f] & 0xff;
				const int y = ((k << 3 & 0xf8) + _scroll & 0xff) + 16;
				if (x > 8 && x < 240)
					xfer8x8(layer[1].data(), color, x | y << 8, k);
			}
		}
		if (mode & 0x20) {
			const int color = colorbank[0] >> 4;
			const int _scroll = -(scroll[2] & 0xf8) + (scroll[2] + 1 & 7) + 10;
			for (int k = 0x4000; k < 0x4400; k++) {
				const int x = (~k >> 2 & 0xf8) + scroll[3] + ram[0x4820 | k & 0x1f] & 0xff;
				const int y = ((k << 3 & 0xf8) + _scroll & 0xff) + 16;
				if (x > 8 && x < 240)
					xfer8x8(layer[2].data(), color, x | y << 8, k);
			}
		}
		if (mode & 0x40) {
			const int color = colorbank[1];
			const int _scroll = -(scroll[4] & 0xf8) + (scroll[4] - 1 & 7) + 12;
			for (int k = 0x4400; k < 0x4800; k++) {
				const int x = (~k >> 2 & 0xf8) + scroll[5] + ram[0x4840 | k & 0x1f] & 0xff;
				const int y = ((k << 3 & 0xf8) + _scroll & 0xff) + 16;
				if (x > 8 && x < 240)
					xfer8x8(layer[3].data(), color, x | y << 8, k);
			}
		}

		// obj描画
		if (mode & 0x80) {
			layer[0].fill(0);
			drawObj(0x497c | mode << 5 & 0x80);
			for (int k = 0x4900 | mode << 5 & 0x80, i = 0; i < 31; k += 4, i++) {
				if (i >= 16 && i < 24)
					continue;
				const int _collision = drawObj(k);
				if (_collision & 8)
					collision[i >= 16 ? 2 : i >> 3] |= 1 << (i & 7);
				collision[3] |= _collision & 7;
			}
		}

		// layer合成
		for (int i = 0; i < 4; i++) {
			const int index = pri[priority & 0x1f][i];
			auto& _layer = layer[index];
			if (~mode & array<int, 4>{0x80, 0x10, 0x20, 0x40}[index])
				continue;
			p = 256 * 16 + 16;
			for (int j = 0; j < 256; p += 256 - 224, j++)
				for (int k = 0; k < 7; p += 32, k++) {
					int px;
					(px = _layer[p]) & 7 && (bitmap[p] = px);
					(px = _layer[p + 0x01]) & 7 && (bitmap[p + 0x01] = px);
					(px = _layer[p + 0x02]) & 7 && (bitmap[p + 0x02] = px);
					(px = _layer[p + 0x03]) & 7 && (bitmap[p + 0x03] = px);
					(px = _layer[p + 0x04]) & 7 && (bitmap[p + 0x04] = px);
					(px = _layer[p + 0x05]) & 7 && (bitmap[p + 0x05] = px);
					(px = _layer[p + 0x06]) & 7 && (bitmap[p + 0x06] = px);
					(px = _layer[p + 0x07]) & 7 && (bitmap[p + 0x07] = px);
					(px = _layer[p + 0x08]) & 7 && (bitmap[p + 0x08] = px);
					(px = _layer[p + 0x09]) & 7 && (bitmap[p + 0x09] = px);
					(px = _layer[p + 0x0a]) & 7 && (bitmap[p + 0x0a] = px);
					(px = _layer[p + 0x0b]) & 7 && (bitmap[p + 0x0b] = px);
					(px = _layer[p + 0x0c]) & 7 && (bitmap[p + 0x0c] = px);
					(px = _layer[p + 0x0d]) & 7 && (bitmap[p + 0x0d] = px);
					(px = _layer[p + 0x0e]) & 7 && (bitmap[p + 0x0e] = px);
					(px = _layer[p + 0x0f]) & 7 && (bitmap[p + 0x0f] = px);
					(px = _layer[p + 0x10]) & 7 && (bitmap[p + 0x10] = px);
					(px = _layer[p + 0x11]) & 7 && (bitmap[p + 0x11] = px);
					(px = _layer[p + 0x12]) & 7 && (bitmap[p + 0x12] = px);
					(px = _layer[p + 0x13]) & 7 && (bitmap[p + 0x13] = px);
					(px = _layer[p + 0x14]) & 7 && (bitmap[p + 0x14] = px);
					(px = _layer[p + 0x15]) & 7 && (bitmap[p + 0x15] = px);
					(px = _layer[p + 0x16]) & 7 && (bitmap[p + 0x16] = px);
					(px = _layer[p + 0x17]) & 7 && (bitmap[p + 0x17] = px);
					(px = _layer[p + 0x18]) & 7 && (bitmap[p + 0x18] = px);
					(px = _layer[p + 0x19]) & 7 && (bitmap[p + 0x19] = px);
					(px = _layer[p + 0x1a]) & 7 && (bitmap[p + 0x1a] = px);
					(px = _layer[p + 0x1b]) & 7 && (bitmap[p + 0x1b] = px);
					(px = _layer[p + 0x1c]) & 7 && (bitmap[p + 0x1c] = px);
					(px = _layer[p + 0x1d]) & 7 && (bitmap[p + 0x1d] = px);
					(px = _layer[p + 0x1e]) & 7 && (bitmap[p + 0x1e] = px);
					(px = _layer[p + 0x1f]) & 7 && (bitmap[p + 0x1f] = px);
				}
		}

		// palette変換
		p = 256 * 16 + 16;
		for (int i = 0; i < 256; p += 256 - 224, i++)
			for (int j = 0; j < 224; p++, j++)
				bitmap[p] = rgb[bitmap[p]];

		return bitmap.data();
	}

	void xfer8x8(uint8_t *data, int color, int p, int k) {
		int q = (ram[k] | color << 5 & 0x100) << 6;
		const int idx = color << 3 & 0x38;

		if (p < 0x10900) {
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
			return;
		}
		for (int i = 0; i < 8; p += 256, p -= (p >= 0x11000) * 0x10000, q += 8, i++) {
			data[p + 0x000] = idx | bg[q | 0x00];
			data[p + 0x001] = idx | bg[q | 0x01];
			data[p + 0x002] = idx | bg[q | 0x02];
			data[p + 0x003] = idx | bg[q | 0x03];
			data[p + 0x004] = idx | bg[q | 0x04];
			data[p + 0x005] = idx | bg[q | 0x05];
			data[p + 0x006] = idx | bg[q | 0x06];
			data[p + 0x007] = idx | bg[q | 0x07];
		}
	}

	int drawObj(int k) {
		const int x = ram[k + 1];
		const int y = (ram[k] - 1 & 0xff) + 16;
		const int src = ram[k + 3] & 0x7f | ram[k + 2] << 5 & 0x80 | colorbank[1] << 4 & 0x300;

		switch (ram[k + 2] & 3) {
		case 0: // ノーマル
			return xfer16x16(x | y << 8, src);
		case 1: // V反転
			return xfer16x16V(x | y << 8, src);
		case 2: // H反転
			return xfer16x16H(x | y << 8, src);
		case 3: // HV反転
			return xfer16x16HV(x | y << 8, src);
		}
		return 0;
	}

	int xfer16x16(int dst, int src) {
		const int idx = src >> 4 & 0x38;
		int px, _collision = 0;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return _collision;
		src = src << 8 & 0x7f00;
		for (int i = 16; i != 0; dst += 256 - 16, dst -= (dst >= 0x11000) * 0x10000, --i)
			for (int j = 16; j != 0; dst++, --j) {
				if (!(px = obj[src++]) || (dst & 0xff) < 16 || (dst & 0xff) >= 240)
					continue;
				layer[0][dst] ? (_collision |= 8) : (layer[0][dst] = idx | px);
				mode & 0x10 && layer[1][dst] & 7 && (_collision |= 1);
				mode & 0x20 && layer[2][dst] & 7 && (_collision |= 2);
				mode & 0x40 && layer[3][dst] & 7 && (_collision |= 4);
			}
		return _collision;
	}

	int xfer16x16V(int dst, int src) {
		const int idx = src >> 4 & 0x38;
		int px, _collision = 0;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return _collision;
		src = (src << 8 & 0x7f00) + 256 - 16;
		for (int i = 16; i != 0; dst += 256 - 16, src -= 32, dst -= (dst >= 0x11000) * 0x10000, --i)
			for (int j = 16; j != 0; dst++, --j) {
				if (!(px = obj[src++]) || (dst & 0xff) < 16 || (dst & 0xff) >= 240)
					continue;
				layer[0][dst] ? (_collision |= 8) : (layer[0][dst] = idx | px);
				mode & 0x10 && layer[1][dst] & 7 && (_collision |= 1);
				mode & 0x20 && layer[2][dst] & 7 && (_collision |= 2);
				mode & 0x40 && layer[3][dst] & 7 && (_collision |= 4);
			}
		return _collision;
	}

	int xfer16x16H(int dst, int src) {
		const int idx = src >> 4 & 0x38;
		int px, _collision = 0;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return _collision;
		src = (src << 8 & 0x7f00) + 16;
		for (int i = 16; i != 0; dst += 256 - 16, src += 32, dst -= (dst >= 0x11000) * 0x10000, --i)
			for (int j = 16; j != 0; dst++, --j) {
				if (!(px = obj[--src]) || (dst & 0xff) < 16 || (dst & 0xff) >= 240)
					continue;
				layer[0][dst] ? (_collision |= 8) : (layer[0][dst] = idx | px);
				mode & 0x10 && layer[1][dst] & 7 && (_collision |= 1);
				mode & 0x20 && layer[2][dst] & 7 && (_collision |= 2);
				mode & 0x40 && layer[3][dst] & 7 && (_collision |= 4);
			}
		return _collision;
	}

	int xfer16x16HV(int dst, int src) {
		const int idx = src >> 4 & 0x38;
		int px, _collision = 0;

		if ((dst & 0xff) == 0 || (dst & 0xff) >= 240)
			return _collision;
		src = (src << 8 & 0x7f00) + 256;
		for (int i = 16; i != 0; dst += 256 - 16, dst -= (dst >= 0x11000) * 0x10000, --i)
			for (int j = 16; j != 0; dst++, --j) {
				if (!(px = obj[--src]) || (dst & 0xff) < 16 || (dst & 0xff) >= 240)
					continue;
				layer[0][dst] ? (_collision |= 8) : (layer[0][dst] = idx | px);
				mode & 0x10 && layer[1][dst] & 7 && (_collision |= 1);
				mode & 0x20 && layer[2][dst] & 7 && (_collision |= 2);
				mode & 0x40 && layer[3][dst] & 7 && (_collision |= 4);
			}
		return _collision;
	}

	void init(int rate) {
		sound0 = new AY_3_8910(6000000 / 4);
		sound1 = new AY_3_8910(6000000 / 4);
		sound2 = new AY_3_8910(6000000 / 4);
		sound3 = new AY_3_8910(6000000 / 4);
		sound4 = new Dac1Ch(0.2);
		Z80::init();
	}
};

#endif //ELEVATOR_ACTION_H
