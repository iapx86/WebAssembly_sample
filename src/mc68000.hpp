/*
 *
 *	MC68000 Emulator
 *
 */

#ifndef MC68000_H
#define MC68000_H

#include <algorithm>
#include "cpu.hpp"

struct MC68000 : Cpu {
	static const unsigned char cc[0x10000], cc_ex[0x100];
	int d[8] = {};
	int a[8] = {};
	int ssp = 0;
	int usp = 0;
	int sr = 0; // sr:t-s--iii ccr:---xnzvc
	Page16 memorymap[0x10000];
	int breakpointmap[0x80000] = {};
	int op = 0;

	MC68000(int clock = 0) : Cpu(clock) {}

	void reset() override {
		Cpu::reset();
		sr = 0x2700;
		a[7] = txread32(0);
		pc = txread32(4);
	}

	bool interrupt() override {
		const int ipl = 1;
		if (ipl <= (sr >> 8 & 7) || !Cpu::interrupt())
			return false;
		return exception(24 + ipl), sr = sr & ~0x0700 | ipl << 8, true;
	}

	virtual bool interrupt(int ipl, int vector = -1) {
		if (ipl < 7 && ipl <= (sr >> 8 & 7) || !Cpu::interrupt())
			return false;
		return exception(vector < 0 ? 24 + ipl : vector), sr = sr & ~0x0700 | ipl << 8, true;
	}

	void exception(int vector) {
		switch (vector) {
		case 4: case 8: case 10: case 11:
			pc = pc - 2;
			break;
		}
		cycle -= cc_ex[vector], ~sr & 0x2000 && (usp = a[7], a[7] = ssp);
		a[7] -= 4, write32(pc, a[7]), a[7] -= 2, write16(sr, a[7]), pc = read32(vector << 2), sr = sr & ~0x8000 | 0x2000;
	}

	void _execute() override {
		if (pc & 1)
			return exception(3);
		op = fetch16();
		cycle -= cc[op];
		switch (op >> 12) {
		case 0x0: // Bit Manipulation/MOVEP/Immediate
			return execute_0();
		case 0x1: // Move Byte
			return execute_1();
		case 0x2: // Move Long
			return execute_2();
		case 0x3: // Move Word
			return execute_3();
		case 0x4: // Miscellaneous
			return execute_4();
		case 0x5: // ADDQ/SUBQ/Scc/DBcc
			return execute_5();
		case 0x6: // Bcc/BSR
			return execute_6();
		case 0x7: // MOVEQ
			return execute_7();
		case 0x8: // OR/DIV/SBCD
			return execute_8();
		case 0x9: // SUB/SUBX
			return execute_9();
		case 0xa: // (Unassigned)
			return exception(10);
		case 0xb: // CMP/EOR
			return execute_b();
		case 0xc: // AND/MUL/ABCD/EXG
			return execute_c();
		case 0xd: // ADD/ADDX
			return execute_d();
		case 0xe: // Shift/Rotate
			return execute_e();
		case 0xf: // (Unassigned)
			return exception(11);
		default:
			return;
		}
	}

	void execute_0() { // Bit Manipulation/MOVEP/Immediate
		const int x = op >> 9 & 7, y = op & 7;
		int ea, data;
		switch (op >> 3 & 077) {
		case 000:
			switch (x) {
			case 0: // ORI.B #<data>,Dy
				return void(d[y] = d[y] & ~0xff | or8(fetch16(), d[y]));
			case 1: // ANDI.B #<data>,Dy
				return void(d[y] = d[y] & ~0xff | and8(fetch16(), d[y]));
			case 2: // SUBI.B #<data>,Dy
				return void(d[y] = d[y] & ~0xff | sub8(fetch16(), d[y]));
			case 3: // ADDI.B #<data>,Dy
				return void(d[y] = d[y] & ~0xff | add8(fetch16(), d[y]));
			case 4: // BTST #<data>,Dy
				return btst32(fetch16(), d[y]);
			case 5: // EORI.B #<data>,Dy
				return void(d[y] = d[y] & ~0xff | eor8(fetch16(), d[y]));
			case 6: // CMPI.B #<data>,Dy
				return cmp8(fetch16(), d[y]);
			}
			return exception(4);
		case 002:
			switch (x) {
			case 0: // ORI.B #<data>,(Ay)
				return write8(or8(fetch16(), read8(a[y])), a[y]);
			case 1: // ANDI.B #<data>,(Ay)
				return write8(and8(fetch16(), read8(a[y])), a[y]);
			case 2: // SUBI.B #<data>,(Ay)
				return write8(sub8(fetch16(), read8(a[y])), a[y]);
			case 3: // ADDI.B #<data>,(Ay)
				return write8(add8(fetch16(), read8(a[y])), a[y]);
			case 4: // BTST #<data>,(Ay)
				return btst8(fetch16(), read8(a[y]));
			case 5: // EORI.B #<data>,(Ay)
				return write8(eor8(fetch16(), read8(a[y])), a[y]);
			case 6: // CMPI.B #<data>,(Ay)
				return cmp8(fetch16(), read8(a[y]));
			}
			return exception(4);
		case 003:
			switch (x) {
			case 0: // ORI.B #<data>,(Ay)+
				return write8(or8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 1: // ANDI.B #<data>,(Ay)+
				return write8(and8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 2: // SUBI.B #<data>,(Ay)+
				return write8(sub8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 3: // ADDI.B #<data>,(Ay)+
				return write8(add8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 4: // BTST #<data>,(Ay)+
				return btst8(fetch16(), read8(a[y])), void(a[y] += y < 7 ? 1 : 2);
			case 5: // EORI.B #<data>,(Ay)+
				return write8(eor8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 6: // CMPI.B #<data>,(Ay)+
				return cmp8(fetch16(), read8(a[y])), void(a[y] += y < 7 ? 1 : 2);
			}
			return exception(4);
		case 004:
			switch (x) {
			case 0: // ORI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(or8(fetch16(), read8(a[y])), a[y]);
			case 1: // ANDI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(and8(fetch16(), read8(a[y])), a[y]);
			case 2: // SUBI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(sub8(fetch16(), read8(a[y])), a[y]);
			case 3: // ADDI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(add8(fetch16(), read8(a[y])), a[y]);
			case 4: // BTST #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, btst8(fetch16(), read8(a[y]));
			case 5: // EORI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(eor8(fetch16(), read8(a[y])), a[y]);
			case 6: // CMPI.B #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, cmp8(fetch16(), read8(a[y]));
			}
			return exception(4);
		case 005:
			switch (x) {
			case 0: // ORI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(or8(data, read8(ea)), ea);
			case 1: // ANDI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(and8(data, read8(ea)), ea);
			case 2: // SUBI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(sub8(data, read8(ea)), ea);
			case 3: // ADDI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(add8(data, read8(ea)), ea);
			case 4: // BTST #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), btst8(data, read8(ea));
			case 5: // EORI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(eor8(data, read8(ea)), ea);
			case 6: // CMPI.B #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), cmp8(data, read8(ea));
			}
			return exception(4);
		case 006:
			switch (x) {
			case 0: // ORI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(or8(data, read8(ea)), ea);
			case 1: // ANDI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(and8(data, read8(ea)), ea);
			case 2: // SUBI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(sub8(data, read8(ea)), ea);
			case 3: // ADDI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(add8(data, read8(ea)), ea);
			case 4: // BTST #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), btst8(data, read8(ea));
			case 5: // EORI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(eor8(data, read8(ea)), ea);
			case 6: // CMPI.B #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), cmp8(data, read8(ea));
			}
			return exception(4);
		case 007:
			switch (x << 3 | y) {
			case 000: // ORI.B #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(or8(data, read8(ea)), ea);
			case 001: // ORI.B #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(or8(data, read8(ea)), ea);
			case 004: // ORI #<data>,CCR
				return void(sr |= fetch16() & 0xff);
			case 010: // ANDI.B #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(and8(data, read8(ea)), ea);
			case 011: // ANDI.B #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(and8(data, read8(ea)), ea);
			case 014: // ANDI #<data>,CCR
				return void(sr &= fetch16() | ~0xff);
			case 020: // SUBI.B #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(sub8(data, read8(ea)), ea);
			case 021: // SUBI.B #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(sub8(data, read8(ea)), ea);
			case 030: // ADDI.B #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(add8(data, read8(ea)), ea);
			case 031: // ADDI.B #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(add8(data, read8(ea)), ea);
			case 040: // BTST #<data>,Abs.W
				return data = fetch16(), btst8(data, read8(fetch16s()));
			case 041: // BTST #<data>,Abs.L
				return data = fetch16(), btst8(data, read8(fetch32()));
			case 042: // BTST #<data>,d(PC)
				return data = fetch16(), btst8(data, read8(disp(pc)));
			case 043: // BTST #<data>,d(PC,Xi)
				return data = fetch16(), btst8(data, read8(index(pc)));
			case 050: // EORI.B #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(eor8(data, read8(ea)), ea);
			case 051: // EORI.B #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(eor8(data, read8(ea)), ea);
			case 054: // EORI #<data>,CCR
				return void(sr ^= fetch16() & 0xff);
			case 060: // CMPI.B #<data>,Abs.W
				return data = fetch16(), cmp8(data, read8(fetch16s()));
			case 061: // CMPI.B #<data>,Abs.L
				return data = fetch16(), cmp8(data, read8(fetch32()));
			}
			return exception(4);
		case 010:
			switch (x) {
			case 0: // ORI.W #<data>,Dy
				return void(d[y] = d[y] & ~0xffff | or16(fetch16(), d[y]));
			case 1: // ANDI.W #<data>,Dy
				return void(d[y] = d[y] & ~0xffff | and16(fetch16(), d[y]));
			case 2: // SUBI.W #<data>,Dy
				return void(d[y] = d[y] & ~0xffff | sub16(fetch16(), d[y]));
			case 3: // ADDI.W #<data>,Dy
				return void(d[y] = d[y] & ~0xffff | add16(fetch16(), d[y]));
			case 4: // BCHG #<data>,Dy
				return void(d[y] = bchg32(fetch16(), d[y]));
			case 5: // EORI.W #<data>,Dy
				return void(d[y] = d[y] & ~0xffff | eor16(fetch16(), d[y]));
			case 6: // CMPI.W #<data>,Dy
				return cmp16(fetch16(), d[y]);
			}
			return exception(4);
		case 012:
			switch (x) {
			case 0: // ORI.W #<data>,(Ay)
				return write16(or16(fetch16(), read16(a[y])), a[y]);
			case 1: // ANDI.W #<data>,(Ay)
				return write16(and16(fetch16(), read16(a[y])), a[y]);
			case 2: // SUBI.W #<data>,(Ay)
				return write16(sub16(fetch16(), read16(a[y])), a[y]);
			case 3: // ADDI.W #<data>,(Ay)
				return write16(add16(fetch16(), read16(a[y])), a[y]);
			case 4: // BCHG #<data>,(Ay)
				return write8(bchg8(fetch16(), read8(a[y])), a[y]);
			case 5: // EORI.W #<data>,(Ay)
				return write16(eor16(fetch16(), read16(a[y])), a[y]);
			case 6: // CMPI.W #<data>,(Ay)
				return cmp16(fetch16(), read16(a[y]));
			}
			return exception(4);
		case 013:
			switch (x) {
			case 0: // ORI.W #<data>,(Ay)+
				return write16(or16(fetch16(), read16(a[y])), a[y]), void(a[y] += 2);
			case 1: // ANDI.W #<data>,(Ay)+
				return write16(and16(fetch16(), read16(a[y])), a[y]), void(a[y] += 2);
			case 2: // SUBI.W #<data>,(Ay)+
				return write16(sub16(fetch16(), read16(a[y])), a[y]), void(a[y] += 2);
			case 3: // ADDI.W #<data>,(Ay)+
				return write16(add16(fetch16(), read16(a[y])), a[y]), void(a[y] += 2);
			case 4: // BCHG #<data>,(Ay)+
				return write8(bchg8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 5: // EORI.W #<data>,(Ay)+
				return write16(eor16(fetch16(), read16(a[y])), a[y]), void(a[y] += 2);
			case 6: // CMPI.W #<data>,(Ay)+
				return cmp16(fetch16(), read16(a[y])), void(a[y] += 2);
			}
			return exception(4);
		case 014:
			switch (x) {
			case 0: // ORI.W #<data>,-(Ay)
				return a[y] -= 2, write16(or16(fetch16(), read16(a[y])), a[y]);
			case 1: // ANDI.W #<data>,-(Ay)
				return a[y] -= 2, write16(and16(fetch16(), read16(a[y])), a[y]);
			case 2: // SUBI.W #<data>,-(Ay)
				return a[y] -= 2, write16(sub16(fetch16(), read16(a[y])), a[y]);
			case 3: // ADDI.W #<data>,-(Ay)
				return a[y] -= 2, write16(add16(fetch16(), read16(a[y])), a[y]);
			case 4: // BCHG #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(bchg8(fetch16(), read8(a[y])), a[y]);
			case 5: // EORI.W #<data>,-(Ay)
				return a[y] -= 2, write16(eor16(fetch16(), read16(a[y])), a[y]);
			case 6: // CMPI.W #<data>,-(Ay)
				return a[y] -= 2, cmp16(fetch16(), read16(a[y]));
			}
			return exception(4);
		case 015:
			switch (x) {
			case 0: // ORI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write16(or16(data, read16(ea)), ea);
			case 1: // ANDI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write16(and16(data, read16(ea)), ea);
			case 2: // SUBI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write16(sub16(data, read16(ea)), ea);
			case 3: // ADDI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write16(add16(data, read16(ea)), ea);
			case 4: // BCHG #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(bchg8(data, read8(ea)), ea);
			case 5: // EORI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write16(eor16(data, read16(ea)), ea);
			case 6: // CMPI.W #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), cmp16(data, read16(ea));
			}
			return exception(4);
		case 016:
			switch (x) {
			case 0: // ORI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write16(or16(data, read16(ea)), ea);
			case 1: // ANDI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write16(and16(data, read16(ea)), ea);
			case 2: // SUBI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write16(sub16(data, read16(ea)), ea);
			case 3: // ADDI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write16(add16(data, read16(ea)), ea);
			case 4: // BCHG #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(bchg8(data, read8(ea)), ea);
			case 5: // EORI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write16(eor16(data, read16(ea)), ea);
			case 6: // CMPI.W #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), cmp16(data, read16(ea));
			}
			return exception(4);
		case 017:
			switch (x << 3 | y) {
			case 000: // ORI.W #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write16(or16(data, read16(ea)), ea);
			case 001: // ORI.W #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write16(or16(data, read16(ea)), ea);
			case 004: // ORI #<data>,SR
				return ~sr & 0x2000 ? exception(8) : void(sr |= fetch16());
			case 010: // ANDI.W #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write16(and16(data, read16(ea)), ea);
			case 011: // ANDI.W #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write16(and16(data, read16(ea)), ea);
			case 014: // ANDI #<data>,SR
				return ~sr & 0x2000 ? exception(8) : (sr &= fetch16(), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 020: // SUBI.W #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write16(sub16(data, read16(ea)), ea);
			case 021: // SUBI.W #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write16(sub16(data, read16(ea)), ea);
			case 030: // ADDI.W #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write16(add16(data, read16(ea)), ea);
			case 031: // ADDI.W #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write16(add16(data, read16(ea)), ea);
			case 040: // BCHG #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(bchg8(data, read8(ea)), ea);
			case 041: // BCHG #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(bchg8(data, read8(ea)), ea);
			case 050: // EORI.W #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write16(eor16(data, read16(ea)), ea);
			case 051: // EORI.W #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write16(eor16(data, read16(ea)), ea);
			case 054: // EORI #<data>,SR
				return ~sr & 0x2000 ? exception(8) : (sr ^= fetch16(), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 060: // CMPI.W #<data>,Abs.W
				return data = fetch16(), cmp16(data, read16(fetch16s()));
			case 061: // CMPI.W #<data>,Abs.L
				return data = fetch16(), cmp16(data, read16(fetch32()));
			}
			return exception(4);
		case 020:
			switch (x) {
			case 0: // ORI.L #<data>,Dy
				return void(d[y] = or32(fetch32(), d[y]));
			case 1: // ANDI.L #<data>,Dy
				return void(d[y] = and32(fetch32(), d[y]));
			case 2: // SUBI.L #<data>,Dy
				return void(d[y] = sub32(fetch32(), d[y]));
			case 3: // ADDI.L #<data>,Dy
				return void(d[y] = add32(fetch32(), d[y]));
			case 4: // BCLR #<data>,Dy
				return void(d[y] = bclr32(fetch16(), d[y]));
			case 5: // EORI.L #<data>,Dy
				return void(d[y] = eor32(fetch32(), d[y]));
			case 6: // CMPI.L #<data>,Dy
				return cmp32(fetch32(), d[y]);
			}
			return exception(4);
		case 022:
			switch (x) {
			case 0: // ORI.L #<data>,(Ay)
				return write32(or32(fetch32(), read32(a[y])), a[y]);
			case 1: // ANDI.L #<data>,(Ay)
				return write32(and32(fetch32(), read32(a[y])), a[y]);
			case 2: // SUBI.L #<data>,(Ay)
				return write32(sub32(fetch32(), read32(a[y])), a[y]);
			case 3: // ADDI.L #<data>,(Ay)
				return write32(add32(fetch32(), read32(a[y])), a[y]);
			case 4: // BCLR #<data>,(Ay)
				return write8(bclr8(fetch16(), read8(a[y])), a[y]);
			case 5: // EORI.L #<data>,(Ay)
				return write32(eor32(fetch32(), read32(a[y])), a[y]);
			case 6: // CMPI.L #<data>,(Ay)
				return cmp32(fetch32(), read32(a[y]));
			}
			return exception(4);
		case 023:
			switch (x) {
			case 0: // ORI.L #<data>,(Ay)+
				return write32(or32(fetch32(), read32(a[y])), a[y]), void(a[y] += 4);
			case 1: // ANDI.L #<data>,(Ay)+
				return write32(and32(fetch32(), read32(a[y])), a[y]), void(a[y] += 4);
			case 2: // SUBI.L #<data>,(Ay)+
				return write32(sub32(fetch32(), read32(a[y])), a[y]), void(a[y] += 4);
			case 3: // ADDI.L #<data>,(Ay)+
				return write32(add32(fetch32(), read32(a[y])), a[y]), void(a[y] += 4);
			case 4: // BCLR #<data>,(Ay)+
				return write8(bclr8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 5: // EORI.L #<data>,(Ay)+
				return write32(eor32(fetch32(), read32(a[y])), a[y]), void(a[y] += 4);
			case 6: // CMPI.L #<data>,(Ay)+
				return cmp32(fetch32(), read32(a[y])), void(a[y] += 4);
			}
			return exception(4);
		case 024:
			switch (x) {
			case 0: // ORI.L #<data>,-(Ay)
				return a[y] -= 4, write32(or32(fetch32(), read32(a[y])), a[y]);
			case 1: // ANDI.L #<data>,-(Ay)
				return a[y] -= 4, write32(and32(fetch32(), read32(a[y])), a[y]);
			case 2: // SUBI.L #<data>,-(Ay)
				return a[y] -= 4, write32(sub32(fetch32(), read32(a[y])), a[y]);
			case 3: // ADDI.L #<data>,-(Ay)
				return a[y] -= 4, write32(add32(fetch32(), read32(a[y])), a[y]);
			case 4: // BCLR #<data>,-(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(bclr8(fetch16(), read8(a[y])), a[y]);
			case 5: // EORI.L #<data>,-(Ay)
				return a[y] -= 4, write32(eor32(fetch32(), read32(a[y])), a[y]);
			case 6: // CMPI.L #<data>,-(Ay)
				return a[y] -= 4, cmp32(fetch32(), read32(a[y]));
			}
			return exception(4);
		case 025:
			switch (x) {
			case 0: // ORI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), write32(or32(data, read32(ea)), ea);
			case 1: // ANDI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), write32(and32(data, read32(ea)), ea);
			case 2: // SUBI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), write32(sub32(data, read32(ea)), ea);
			case 3: // ADDI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), write32(add32(data, read32(ea)), ea);
			case 4: // BCLR #<data>,d(Ay)
				return data = fetch16(), ea = disp(a[y]), write8(bclr8(data, read8(ea)), ea);
			case 5: // EORI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), write32(eor32(data, read32(ea)), ea);
			case 6: // CMPI.L #<data>,d(Ay)
				return data = fetch32(), ea = disp(a[y]), cmp32(data, read32(ea));
			}
			return exception(4);
		case 026:
			switch (x) {
			case 0: // ORI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), write32(or32(data, read32(ea)), ea);
			case 1: // ANDI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), write32(and32(data, read32(ea)), ea);
			case 2: // SUBI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), write32(sub32(data, read32(ea)), ea);
			case 3: // ADDI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), write32(add32(data, read32(ea)), ea);
			case 4: // BCLR #<data>,d(Ay,Xi)
				return data = fetch16(), ea = index(a[y]), write8(bclr8(data, read8(ea)), ea);
			case 5: // EORI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), write32(eor32(data, read32(ea)), ea);
			case 6: // CMPI.L #<data>,d(Ay,Xi)
				return data = fetch32(), ea = index(a[y]), cmp32(data, read32(ea));
			}
			return exception(4);
		case 027:
			switch (x << 3 | y) {
			case 000: // ORI.L #<data>,Abs.W
				return data = fetch32(), ea = fetch16s(), write32(or32(data, read32(ea)), ea);
			case 001: // ORI.L #<data>,Abs.L
				return data = fetch32(), ea = fetch32(), write32(or32(data, read32(ea)), ea);
			case 010: // ANDI.L #<data>,Abs.W
				return data = fetch32(), ea = fetch16s(), write32(and32(data, read32(ea)), ea);
			case 011: // ANDI.L #<data>,Abs.L
				return data = fetch32(), ea = fetch32(), write32(and32(data, read32(ea)), ea);
			case 020: // SUBI.L #<data>,Abs.W
				return data = fetch32(), ea = fetch16s(), write32(sub32(data, read32(ea)), ea);
			case 021: // SUBI.L #<data>,Abs.L
				return data = fetch32(), ea = fetch32(), write32(sub32(data, read32(ea)), ea);
			case 030: // ADDI.L #<data>,Abs.W
				return data = fetch32(), ea = fetch16s(), write32(add32(data, read32(ea)), ea);
			case 031: // ADDI.L #<data>,Abs.L
				return data = fetch32(), ea = fetch32(), write32(add32(data, read32(ea)), ea);
			case 040: // BCLR #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(bclr8(data, read8(ea)), ea);
			case 041: // BCLR #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(bclr8(data, read8(ea)), ea);
			case 050: // EORI.L #<data>,Abs.W
				return data = fetch32(), ea = fetch16s(), write32(eor32(data, read32(ea)), ea);
			case 051: // EORI.L #<data>,Abs.L
				return data = fetch32(), ea = fetch32(), write32(eor32(data, read32(ea)), ea);
			case 060: // CMPI.L #<data>,Abs.W
				return data = fetch32(), cmp32(data, read32(fetch16s()));
			case 061: // CMPI.L #<data>,Abs.L
				return data = fetch32(), cmp32(data, read32(fetch32()));
			}
			return exception(4);
		case 030: // BSET #<data>,Dy
			return x != 4 ? exception(4) : void(d[y] = bset32(fetch16(), d[y]));
		case 032: // BSET #<data>,(Ay)
			return x != 4 ? exception(4) : write8(bset8(fetch16(), read8(a[y])), a[y]);
		case 033: // BSET #<data>,(Ay)+
			return x != 4 ? exception(4) : (write8(bset8(fetch16(), read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2));
		case 034: // BSET #<data>,-(Ay)
			return x != 4 ? exception(4) : (a[y] -= y < 7 ? 1 : 2, write8(bset8(fetch16(), read8(a[y])), a[y]));
		case 035: // BSET #<data>,d(Ay)
			return x != 4 ? exception(4) : (data = fetch16(), ea = disp(a[y]), write8(bset8(data, read8(ea)), ea));
		case 036: // BSET #<data>,d(Ay,Xi)
			return x != 4 ? exception(4) : (data = fetch16(), ea = index(a[y]), write8(bset8(data, read8(ea)), ea));
		case 037:
			switch (x << 3 | y) {
			case 040: // BSET #<data>,Abs.W
				return data = fetch16(), ea = fetch16s(), write8(bset8(data, read8(ea)), ea);
			case 041: // BSET #<data>,Abs.L
				return data = fetch16(), ea = fetch32(), write8(bset8(data, read8(ea)), ea);
			}
			return exception(4);
		case 040: // BTST Dx,Dy
			return btst32(d[x], d[y]);
		case 041: // MOVEP.W d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xffff | read8(ea) << 8 | read8(ea + 2));
		case 042: // BTST Dx,(Ay)
			return btst8(d[x], read8(a[y]));
		case 043: // BTST Dx,(Ay)+
			return btst8(d[x], read8(a[y])), void(a[y] += y < 7 ? 1 : 2);
		case 044: // BTST Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, btst8(d[x], read8(a[y]));
		case 045: // BTST Dx,d(Ay)
			return ea = disp(a[y]), btst8(d[x], read8(ea));
		case 046: // BTST Dx,d(Ay,Xi)
			return ea = index(a[y]), btst8(d[x], read8(ea));
		case 047: // BTST Dx,Abs...
			return y >= 5 ? exception(4) : btst8(d[x], rop8());
		case 050: // BCHG Dx,Dy
			return void(d[y] = bchg32(d[x], d[y]));
		case 051: // MOVEP.L d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = read8(ea) << 24 | read8(ea + 2) << 16 | read8(ea + 4) << 8 | read8(ea + 6));
		case 052: // BCHG Dx,(Ay)
			return write8(bchg8(d[x], read8(a[y])), a[y]);
		case 053: // BCHG Dx,(Ay)+
			return write8(bchg8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 054: // BCHG Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(bchg8(d[x], read8(a[y])), a[y]);
		case 055: // BCHG Dx,d(Ay)
			return ea = disp(a[y]), write8(bchg8(d[x], read8(ea)), ea);
		case 056: // BCHG Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(bchg8(d[x], read8(ea)), ea);
		case 057:
			switch (y) {
			case 0: // BCHG Dx,Abs.W
				return ea = fetch16s(), write8(bchg8(d[x], read8(ea)), ea);
			case 1: // BCHG Dx,Abs.L
				return ea = fetch32(), write8(bchg8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 060: // BCLR Dx,Dy
			return void(d[y] = bclr32(d[x], d[y]));
		case 061: // MOVEP.W Dx,d(Ay)
			return ea = disp(a[y]), write8(d[x] >> 8, ea), write8(d[x], ea + 2);
		case 062: // BCLR Dx,(Ay)
			return write8(bclr8(d[x], read8(a[y])), a[y]);
		case 063: // BCLR Dx,(Ay)+
			return write8(bclr8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 064: // BCLR Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(bclr8(d[x], read8(a[y])), a[y]);
		case 065: // BCLR Dx,d(Ay)
			return ea = disp(a[y]), write8(bclr8(d[x], read8(ea)), ea);
		case 066: // BCLR Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(bclr8(d[x], read8(ea)), ea);
		case 067:
			switch (y) {
			case 0: // BCLR Dx,Abs.W
				return ea = fetch16s(), write8(bclr8(d[x], read8(ea)), ea);
			case 1: // BCLR Dx,Abs.L
				return ea = fetch32(), write8(bclr8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 070: // BSET Dx,Dy
			return void(d[y] = bset32(d[x], d[y]));
		case 071: // MOVEP.L Dx,d(Ay)
			return ea = disp(a[y]), write8(d[x] >> 24, ea), write8(d[x] >> 16, ea + 2), write8(d[x] >> 8, ea + 4), write8(d[x], ea + 6);
		case 072: // BSET Dx,(Ay)
			return write8(bset8(d[x], read8(a[y])), a[y]);
		case 073: // BSET Dx,(Ay)+
			return write8(bset8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 074: // BSET Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(bset8(d[x], read8(a[y])), a[y]);
		case 075: // BSET Dx,d(Ay)
			return ea = disp(a[y]), write8(bset8(d[x], read8(ea)), ea);
		case 076: // BSET Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(bset8(d[x], read8(ea)), ea);
		case 077:
			switch (y) {
			case 0: // BSET Dx,Abs.W
				return ea = fetch16s(), write8(bset8(d[x], read8(ea)), ea);
			case 1: // BSET Dx,Abs.L
				return ea = fetch32(), write8(bset8(d[x], read8(ea)), ea);
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	void execute_1() { // Move Byte
		const int x = op >> 9 & 7, y = op & 7;
		int src;
		switch (op >> 3 & 077) {
		case 000: // MOVE.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | or8(0, d[y]));
		case 002: // MOVE.B (Ay),Dx
			return src = read8(a[y]), void(d[x] = d[x] & ~0xff | or8(0, src));
		case 003: // MOVE.B (Ay)+,Dx
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, void(d[x] = d[x] & ~0xff | or8(0, src));
		case 004: // MOVE.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), void(d[x] = d[x] & ~0xff | or8(0, src));
		case 005: // MOVE.B d(Ay),Dx
			return src = read8(disp(a[y])), void(d[x] = d[x] & ~0xff | or8(0, src));
		case 006: // MOVE.B d(Ay,Xi),Dx
			return src = read8(index(a[y])), void(d[x] = d[x] & ~0xff | or8(0, src));
		case 007: // MOVE.B Abs...,Dx
			return y >= 5 ? exception(4) : (src = rop8(), void(d[x] = d[x] & ~0xff | or8(0, src)));
		case 020: // MOVE.B Dy,(Ax)
			return src = d[y], write8(or8(0, src), a[x]);
		case 022: // MOVE.B (Ay),(Ax)
			return src = read8(a[y]), write8(or8(0, src), a[x]);
		case 023: // MOVE.B (Ay)+,(Ax)
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 024: // MOVE.B -(Ay),(Ax)
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), a[x]);
		case 025: // MOVE.B d(Ay),(Ax)
			return src = read8(disp(a[y])), write8(or8(0, src), a[x]);
		case 026: // MOVE.B d(Ay,Xi),(Ax)
			return src = read8(index(a[y])), write8(or8(0, src), a[x]);
		case 027: // MOVE.B Abs...,(Ax)
			return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), a[x]));
		case 030: // MOVE.B Dy,(Ax)+
			return src = d[y], write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 032: // MOVE.B (Ay),(Ax)+
			return src = read8(a[y]), write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 033: // MOVE.B (Ay)+,(Ax)+
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 034: // MOVE.B -(Ay),(Ax)+
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 035: // MOVE.B d(Ay),(Ax)+
			return src = read8(disp(a[y])), write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 036: // MOVE.B d(Ay,Xi),(Ax)+
			return src = read8(index(a[y])), write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2);
		case 037: // MOVE.B Abs...,(Ax)+
			return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), a[x]), void(a[x] += x < 7 ? 1 : 2));
		case 040: // MOVE.B Dy,-(Ax)
			return src = d[y], a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 042: // MOVE.B (Ay),-(Ax)
			return src = read8(a[y]), a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 043: // MOVE.B (Ay)+,-(Ax)
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 044: // MOVE.B -(Ay),-(Ax)
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 045: // MOVE.B d(Ay),-(Ax)
			return src = read8(disp(a[y])), a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 046: // MOVE.B d(Ay,Xi),-(Ax)
			return src = read8(index(a[y])), a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]);
		case 047: // MOVE.B Abs...,-(Ax)
			return y >= 5 ? exception(4) : (src = rop8(), a[x] -= x < 7 ? 1 : 2, write8(or8(0, src), a[x]));
		case 050: // MOVE.B Dy,d(Ax)
			return src = d[y], write8(or8(0, src), disp(a[x]));
		case 052: // MOVE.B (Ay),d(Ax)
			return src = read8(a[y]), write8(or8(0, src), disp(a[x]));
		case 053: // MOVE.B (Ay)+,d(Ax)
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), disp(a[x]));
		case 054: // MOVE.B -(Ay),d(Ax)
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), disp(a[x]));
		case 055: // MOVE.B d(Ay),d(Ax)
			return src = read8(disp(a[y])), write8(or8(0, src), disp(a[x]));
		case 056: // MOVE.B d(Ay,Xi),d(Ax)
			return src = read8(index(a[y])), write8(or8(0, src), disp(a[x]));
		case 057: // MOVE.B Abs...,d(Ax)
			return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), disp(a[x])));
		case 060: // MOVE.B Dy,d(Ax,Xi)
			return src = d[y], write8(or8(0, src), index(a[x]));
		case 062: // MOVE.B (Ay),d(Ax,Xi)
			return src = read8(a[y]), write8(or8(0, src), index(a[x]));
		case 063: // MOVE.B (Ay)+,d(Ax,Xi)
			return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), index(a[x]));
		case 064: // MOVE.B -(Ay),d(Ax,Xi)
			return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), index(a[x]));
		case 065: // MOVE.B d(Ay),d(Ax,Xi)
			return src = read8(disp(a[y])), write8(or8(0, src), index(a[x]));
		case 066: // MOVE.B d(Ay,Xi),d(Ax,Xi)
			return src = read8(index(a[y])), write8(or8(0, src), index(a[x]));
		case 067: // MOVE.B Abs...,d(Ax,Xi)
			return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), index(a[x])));
		case 070:
			switch (x) {
			case 0: // MOVE.B Dy,Abs.W
				return src = d[y], write8(or8(0, src), fetch16s());
			case 1: // MOVE.B Dy,Abs.L
				return src = d[y], write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 072:
			switch (x) {
			case 0: // MOVE.B (Ay),Abs.W
				return src = read8(a[y]), write8(or8(0, src), fetch16s());
			case 1: // MOVE.B (Ay),Abs.L
				return src = read8(a[y]), write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 073:
			switch (x) {
			case 0: // MOVE.B (Ay)+,Abs.W
				return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), fetch16s());
			case 1: // MOVE.B (Ay)+,Abs.L
				return src = read8(a[y]), a[y] += y < 7 ? 1 : 2, write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 074:
			switch (x) {
			case 0: // MOVE.B -(Ay),Abs.W
				return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), fetch16s());
			case 1: // MOVE.B -(Ay),Abs.L
				return a[y] -= y < 7 ? 1 : 2, src = read8(a[y]), write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 075:
			switch (x) {
			case 0: // MOVE.B d(Ay),Abs.W
				return src = read8(disp(a[y])), write8(or8(0, src), fetch16s());
			case 1: // MOVE.B d(Ay),Abs.L
				return src = read8(disp(a[y])), write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 076:
			switch (x) {
			case 0: // MOVE.B d(Ay,Xi),Abs.W
				return src = read8(index(a[y])), write8(or8(0, src), fetch16s());
			case 1: // MOVE.B d(Ay,Xi),Abs.L
				return src = read8(index(a[y])), write8(or8(0, src), fetch32());
			}
			return exception(4);
		case 077:
			switch (x) {
			case 0: // MOVE.B Abs...,Abs.W
				return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), fetch16s()));
			case 1: // MOVE.B Abs...,Abs.L
				return y >= 5 ? exception(4) : (src = rop8(), write8(or8(0, src), fetch32()));
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	void execute_2() { // Move Long
		const int x = op >> 9 & 7, y = op & 7;
		int src;
		switch (op >> 3 & 077) {
		case 000: // MOVE.L Dy,Dx
			return void(d[x] = or32(0, d[y]));
		case 001: // MOVE.L Ay,Dx
			return void(d[x] = or32(0, a[y]));
		case 002: // MOVE.L (Ay),Dx
			return src = read32(a[y]), void(d[x] = or32(0, src));
		case 003: // MOVE.L (Ay)+,Dx
			return src = read32(a[y]), a[y] += 4, void(d[x] = or32(0, src));
		case 004: // MOVE.L -(Ay),Dx
			return a[y] -= 4, src = read32(a[y]), void(d[x] = or32(0, src));
		case 005: // MOVE.L d(Ay),Dx
			return src = read32(disp(a[y])), void(d[x] = or32(0, src));
		case 006: // MOVE.L d(Ay,Xi),Dx
			return src = read32(index(a[y])), void(d[x] = or32(0, src));
		case 007: // MOVE.L Abs...,Dx
			return y >= 5 ? exception(4) : (src = rop32(), void(d[x] = or32(0, src)));
		case 010: // MOVEA.L Dy,Ax
			return void(a[x] = d[y]);
		case 011: // MOVEA.L Ay,Ax
			return void(a[x] = a[y]);
		case 012: // MOVEA.L (Ay),Ax
			return src = read32(a[y]), void(a[x] = src);
		case 013: // MOVEA.L (Ay)+,Ax
			return src = read32(a[y]), a[y] += 4, void(a[x] = src);
		case 014: // MOVEA.L -(Ay),Ax
			return a[y] -= 4, src = read32(a[y]), void(a[x] = src);
		case 015: // MOVEA.L d(Ay),Ax
			return src = read32(disp(a[y])), void(a[x] = src);
		case 016: // MOVEA.L d(Ay,Xi),Ax
			return src = read32(index(a[y])), void(a[x] = src);
		case 017: // MOVEA.L Abs...,Ax
			return y >= 5 ? exception(4) : (src = rop32(), void(a[x] = src));
		case 020: // MOVE.L Dy,(Ax)
			return src = d[y], write32(or32(0, src), a[x]);
		case 021: // MOVE.L Ay,(Ax)
			return src = a[y], write32(or32(0, src), a[x]);
		case 022: // MOVE.L (Ay),(Ax)
			return src = read32(a[y]), write32(or32(0, src), a[x]);
		case 023: // MOVE.L (Ay)+,(Ax)
			return src = read32(a[y]), a[y] += 4, write32(or32(0, src), a[x]);
		case 024: // MOVE.L -(Ay),(Ax)
			return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), a[x]);
		case 025: // MOVE.L d(Ay),(Ax)
			return src = read32(disp(a[y])), write32(or32(0, src), a[x]);
		case 026: // MOVE.L d(Ay,Xi),(Ax)
			return src = read32(index(a[y])), write32(or32(0, src), a[x]);
		case 027: // MOVE.L Abs...,(Ax)
			return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), a[x]));
		case 030: // MOVE.L Dy,(Ax)+
			return src = d[y], write32(or32(0, src), a[x]), void(a[x] += 4);
		case 031: // MOVE.L Ay,(Ax)+
			return src = a[y], write32(or32(0, src), a[x]), void(a[x] += 4);
		case 032: // MOVE.L (Ay),(Ax)+
			return src = read32(a[y]), write32(or32(0, src), a[x]), void(a[x] += 4);
		case 033: // MOVE.L (Ay)+,(Ax)+
			return src = read32(a[y]), a[y] += 4, write32(or32(0, src), a[x]), void(a[x] += 4);
		case 034: // MOVE.L -(Ay),(Ax)+
			return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), a[x]), void(a[x] += 4);
		case 035: // MOVE.L d(Ay),(Ax)+
			return src = read32(disp(a[y])), write32(or32(0, src), a[x]), void(a[x] += 4);
		case 036: // MOVE.L d(Ay,Xi),(Ax)+
			return src = read32(index(a[y])), write32(or32(0, src), a[x]), void(a[x] += 4);
		case 037: // MOVE.L Abs...,(Ax)+
			return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), a[x]), void(a[x] += 4));
		case 040: // MOVE.L Dy,-(Ax)
			return src = d[y], a[x] -= 4, write32(or32(0, src), a[x]);
		case 041: // MOVE.L Ay,-(Ax)
			return src = a[y], a[x] -= 4, write32(or32(0, src), a[x]);
		case 042: // MOVE.L (Ay),-(Ax)
			return src = read32(a[y]), a[x] -= 4, write32(or32(0, src), a[x]);
		case 043: // MOVE.L (Ay)+,-(Ax)
			return src = read32(a[y]), a[y] += 4, a[x] -= 4, write32(or32(0, src), a[x]);
		case 044: // MOVE.L -(Ay),-(Ax)
			return a[y] -= 4, src = read32(a[y]), a[x] -= 4, write32(or32(0, src), a[x]);
		case 045: // MOVE.L d(Ay),-(Ax)
			return src = read32(disp(a[y])), a[x] -= 4, write32(or32(0, src), a[x]);
		case 046: // MOVE.L d(Ay,Xi),-(Ax)
			return src = read32(index(a[y])), a[x] -= 4, write32(or32(0, src), a[x]);
		case 047: // MOVE.L Abs...,-(Ax)
			return y >= 5 ? exception(4) : (src = rop32(), a[x] = a[x] - 4, write32(or32(0, src), a[x]));
		case 050: // MOVE.L Dy,d(Ax)
			return src = d[y], write32(or32(0, src), disp(a[x]));
		case 051: // MOVE.L Ay,d(Ax)
			return src = a[y], write32(or32(0, src), disp(a[x]));
		case 052: // MOVE.L (Ay),d(Ax)
			return src = read32(a[y]), write32(or32(0, src), disp(a[x]));
		case 053: // MOVE.L (Ay)+,d(Ax)
			return src = read32(a[y]), a[y] += 4, write32(or32(0, src), disp(a[x]));
		case 054: // MOVE.L -(Ay),d(Ax)
			return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), disp(a[x]));
		case 055: // MOVE.L d(Ay),d(Ax)
			return src = read32(disp(a[y])), write32(or32(0, src), disp(a[x]));
		case 056: // MOVE.L d(Ay,Xi),d(Ax)
			return src = read32(index(a[y])), write32(or32(0, src), disp(a[x]));
		case 057: // MOVE.L Abs...,d(Ax)
			return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), disp(a[x])));
		case 060: // MOVE.L Dy,d(Ax,Xi)
			return src = d[y], write32(or32(0, src), index(a[x]));
		case 061: // MOVE.L Ay,d(Ax,Xi)
			return src = a[y], write32(or32(0, src), index(a[x]));
		case 062: // MOVE.L (Ay),d(Ax,Xi)
			return src = read32(a[y]), write32(or32(0, src), index(a[x]));
		case 063: // MOVE.L (Ay)+,d(Ax,Xi)
			return src = read32(a[y]), a[y] += 4, write32(or32(0, src), index(a[x]));
		case 064: // MOVE.L -(Ay),d(Ax,Xi)
			return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), index(a[x]));
		case 065: // MOVE.L d(Ay),d(Ax,Xi)
			return src = read32(disp(a[y])), write32(or32(0, src), index(a[x]));
		case 066: // MOVE.L d(Ay,Xi),d(Ax,Xi)
			return src = read32(index(a[y])), write32(or32(0, src), index(a[x]));
		case 067: // MOVE.L Abs...,d(Ax,Xi)
			return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), index(a[x])));
		case 070:
			switch (x) {
			case 0: // MOVE.L Dy,Abs.W
				return src = d[y], write32(or32(0, src), fetch16s());
			case 1: // MOVE.L Dy,Abs.L
				return src = d[y], write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 071:
			switch (x) {
			case 0: // MOVE.L Ay,Abs.W
				return src = a[y], write32(or32(0, src), fetch16s());
			case 1: // MOVE.L Ay,Abs.L
				return src = a[y], write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 072:
			switch (x) {
			case 0: // MOVE.L (Ay),Abs.W
				return src = read32(a[y]), write32(or32(0, src), fetch16s());
			case 1: // MOVE.L (Ay),Abs.L
				return src = read32(a[y]), write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 073:
			switch (x) {
			case 0: // MOVE.L (Ay)+,Abs.W
				return src = read32(a[y]), a[y] += 4, write32(or32(0, src), fetch16s());
			case 1: // MOVE.L (Ay)+,Abs.L
				return src = read32(a[y]), a[y] += 4, write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 074:
			switch (x) {
			case 0: // MOVE.L -(Ay),Abs.W
				return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), fetch16s());
			case 1: // MOVE.L -(Ay),Abs.L
				return a[y] -= 4, src = read32(a[y]), write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 075:
			switch (x) {
			case 0: // MOVE.L d(Ay),Abs.W
				return src = read32(disp(a[y])), write32(or32(0, src), fetch16s());
			case 1: // MOVE.L d(Ay),Abs.L
				return src = read32(disp(a[y])), write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 076:
			switch (x) {
			case 0: // MOVE.L d(Ay,Xi),Abs.W
				return src = read32(index(a[y])), write32(or32(0, src), fetch16s());
			case 1: // MOVE.L d(Ay,Xi),Abs.L
				return src = read32(index(a[y])), write32(or32(0, src), fetch32());
			}
			return exception(4);
		case 077:
			switch (x) {
			case 0: // MOVE.L Abs...,Abs.W
				return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), fetch16s()));
			case 1: // MOVE.L Abs...,Abs.L
				return y >= 5 ? exception(4) : (src = rop32(), write32(or32(0, src), fetch32()));
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	void execute_3() { // Move Word
		const int x = op >> 9 & 7, y = op & 7;
		int src;
		switch (op >> 3 & 077) {
		case 000: // MOVE.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | or16(0, d[y]));
		case 001: // MOVE.W Ay,Dx
			return void(d[x] = d[x] & ~0xffff | or16(0, a[y]));
		case 002: // MOVE.W (Ay),Dx
			return src = read16(a[y]), void(d[x] = d[x] & ~0xffff | or16(0, src));
		case 003: // MOVE.W (Ay)+,Dx
			return src = read16(a[y]), a[y] += 2, void(d[x] = d[x] & ~0xffff | or16(0, src));
		case 004: // MOVE.W -(Ay),Dx
			return a[y] -= 2, src = read16(a[y]), void(d[x] = d[x] & ~0xffff | or16(0, src));
		case 005: // MOVE.W d(Ay),Dx
			return src = read16(disp(a[y])), void(d[x] = d[x] & ~0xffff | or16(0, src));
		case 006: // MOVE.W d(Ay,Xi),Dx
			return src = read16(index(a[y])), void(d[x] = d[x] & ~0xffff | or16(0, src));
		case 007: // MOVE.W Abs...,Dx
			return y >= 5 ? exception(4) : (src = rop16(), void(d[x] = d[x] & ~0xffff | or16(0, src)));
		case 010: // MOVEA.W Dy,Ax
			return void(a[x] = d[y] << 16 >> 16);
		case 011: // MOVEA.W Ay,Ax
			return void(a[x] = a[y] << 16 >> 16);
		case 012: // MOVEA.W (Ay),Ax
			return src = read16(a[y]), void(a[x] = src << 16 >> 16);
		case 013: // MOVEA.W (Ay)+,Ax
			return src = read16(a[y]), a[y] += 2, void(a[x] = src << 16 >> 16);
		case 014: // MOVEA.W -(Ay),Ax
			return a[y] -= 2, src = read16(a[y]), void(a[x] = src << 16 >> 16);
		case 015: // MOVEA.W d(Ay),Ax
			return src = read16(disp(a[y])), void(a[x] = src << 16 >> 16);
		case 016: // MOVEA.W d(Ay,Xi),Ax
			return src = read16(index(a[y])), void(a[x] = src << 16 >> 16);
		case 017: // MOVEA.W Abs...,Ax
			return y >= 5 ? exception(4) : (src = rop16(), void(a[x] = src << 16 >> 16));
		case 020: // MOVE.W Dy,(Ax)
			return src = d[y], write16(or16(0, src), a[x]);
		case 021: // MOVE.W Ay,(Ax)
			return src = a[y], write16(or16(0, src), a[x]);
		case 022: // MOVE.W (Ay),(Ax)
			return src = read16(a[y]), write16(or16(0, src), a[x]);
		case 023: // MOVE.W (Ay)+,(Ax)
			return src = read16(a[y]), a[y] += 2, write16(or16(0, src), a[x]);
		case 024: // MOVE.W -(Ay),(Ax)
			return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), a[x]);
		case 025: // MOVE.W d(Ay),(Ax)
			return src = read16(disp(a[y])), write16(or16(0, src), a[x]);
		case 026: // MOVE.W d(Ay,Xi),(Ax)
			return src = read16(index(a[y])), write16(or16(0, src), a[x]);
		case 027: // MOVE.W Abs...,(Ax)
			return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), a[x]));
		case 030: // MOVE.W Dy,(Ax)+
			return src = d[y], write16(or16(0, src), a[x]), void(a[x] += 2);
		case 031: // MOVE.W Ay,(Ax)+
			return src = a[y], write16(or16(0, src), a[x]), void(a[x] += 2);
		case 032: // MOVE.W (Ay),(Ax)+
			return src = read16(a[y]), write16(or16(0, src), a[x]), void(a[x] += 2);
		case 033: // MOVE.W (Ay)+,(Ax)+
			return src = read16(a[y]), a[y] += 2, write16(or16(0, src), a[x]), void(a[x] += 2);
		case 034: // MOVE.W -(Ay),(Ax)+
			return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), a[x]), void(a[x] += 2);
		case 035: // MOVE.W d(Ay),(Ax)+
			return src = read16(disp(a[y])), write16(or16(0, src), a[x]), void(a[x] += 2);
		case 036: // MOVE.W d(Ay,Xi),(Ax)+
			return src = read16(index(a[y])), write16(or16(0, src), a[x]), void(a[x] += 2);
		case 037: // MOVE.W Abs...,(Ax)+
			return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), a[x]), void(a[x] += 2));
		case 040: // MOVE.W Dy,-(Ax)
			return src = d[y], a[x] -= 2, write16(or16(0, src), a[x]);
		case 041: // MOVE.W Ay,-(Ax)
			return src = a[y], a[x] -= 2, write16(or16(0, src), a[x]);
		case 042: // MOVE.W (Ay),-(Ax)
			return src = read16(a[y]), a[x] -= 2, write16(or16(0, src), a[x]);
		case 043: // MOVE.W (Ay)+,-(Ax)
			return src = read16(a[y]), a[y] += 2, a[x] -= 2, write16(or16(0, src), a[x]);
		case 044: // MOVE.W -(Ay),-(Ax)
			return a[y] -= 2, src = read16(a[y]), a[x] -= 2, write16(or16(0, src), a[x]);
		case 045: // MOVE.W d(Ay),-(Ax)
			return src = read16(disp(a[y])), a[x] -= 2, write16(or16(0, src), a[x]);
		case 046: // MOVE.W d(Ay,Xi),-(Ax)
			return src = read16(index(a[y])), a[x] -= 2, write16(or16(0, src), a[x]);
		case 047: // MOVE.W Abs...,-(Ax)
			return y >= 5 ? exception(4) : (src = rop16(), a[x] -= 2, write16(or16(0, src), a[x]));
		case 050: // MOVE.W Dy,d(Ax)
			return src = d[y], write16(or16(0, src), disp(a[x]));
		case 051: // MOVE.W Ay,d(Ax)
			return src = a[y], write16(or16(0, src), disp(a[x]));
		case 052: // MOVE.W (Ay),d(Ax)
			return src = read16(a[y]), write16(or16(0, src), disp(a[x]));
		case 053: // MOVE.W (Ay)+,d(Ax)
			return src = read16(a[y]), a[y] += 2, write16(or16(0, src), disp(a[x]));
		case 054: // MOVE.W -(Ay),d(Ax)
			return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), disp(a[x]));
		case 055: // MOVE.W d(Ay),d(Ax)
			return src = read16(disp(a[y])), write16(or16(0, src), disp(a[x]));
		case 056: // MOVE.W d(Ay,Xi),d(Ax)
			return src = read16(index(a[y])), write16(or16(0, src), disp(a[x]));
		case 057: // MOVE.W Abs...,d(Ax)
			return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), disp(a[x])));
		case 060: // MOVE.W Dy,d(Ax,Xi)
			return src = d[y], write16(or16(0, src), index(a[x]));
		case 061: // MOVE.W Ay,d(Ax,Xi)
			return src = a[y], write16(or16(0, src), index(a[x]));
		case 062: // MOVE.W (Ay),d(Ax,Xi)
			return src = read16(a[y]), write16(or16(0, src), index(a[x]));
		case 063: // MOVE.W (Ay)+,d(Ax,Xi)
			return src = read16(a[y]), a[y] += 2, write16(or16(0, src), index(a[x]));
		case 064: // MOVE.W -(Ay),d(Ax,Xi)
			return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), index(a[x]));
		case 065: // MOVE.W d(Ay),d(Ax,Xi)
			return src = read16(disp(a[y])), write16(or16(0, src), index(a[x]));
		case 066: // MOVE.W d(Ay,Xi),d(Ax,Xi)
			return src = read16(index(a[y])), write16(or16(0, src), index(a[x]));
		case 067: // MOVE.W Abs...,d(Ax,Xi)
			return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), index(a[x])));
		case 070:
			switch (x) {
			case 0: // MOVE.W Dy,Abs.W
				return src = d[y], write16(or16(0, src), fetch16s());
			case 1: // MOVE.W Dy,Abs.L
				return src = d[y], write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 071:
			switch (x) {
			case 0: // MOVE.W Ay,Abs.W
				return src = a[y], write16(or16(0, src), fetch16s());
			case 1: // MOVE.W Ay,Abs.L
				return src = a[y], write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 072:
			switch (x) {
			case 0: // MOVE.W (Ay),Abs.W
				return src = read16(a[y]), write16(or16(0, src), fetch16s());
			case 1: // MOVE.W (Ay),Abs.L
				return src = read16(a[y]), write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 073:
			switch (x) {
			case 0: // MOVE.W (Ay)+,Abs.W
				return src = read16(a[y]), a[y] += 2, write16(or16(0, src), fetch16s());
			case 1: // MOVE.W (Ay)+,Abs.L
				return src = read16(a[y]), a[y] += 2, write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 074:
			switch (x) {
			case 0: // MOVE.W -(Ay),Abs.W
				return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), fetch16s());
			case 1: // MOVE.W -(Ay),Abs.L
				return a[y] -= 2, src = read16(a[y]), write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 075:
			switch (x) {
			case 0: // MOVE.W d(Ay),Abs.W
				return src = read16(disp(a[y])), write16(or16(0, src), fetch16s());
			case 1: // MOVE.W d(Ay),Abs.L
				return src = read16(disp(a[y])), write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 076:
			switch (x) {
			case 0: // MOVE.W d(Ay,Xi),Abs.W
				return src = read16(index(a[y])), write16(or16(0, src), fetch16s());
			case 1: // MOVE.W d(Ay,Xi),Abs.L
				return src = read16(index(a[y])), write16(or16(0, src), fetch32());
			}
			return exception(4);
		case 077:
			switch (x) {
			case 0: // MOVE.W Abs...,Abs.W
				return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), fetch16s()));
			case 1: // MOVE.W Abs...,Abs.L
				return y >= 5 ? exception(4) : (src = rop16(), write16(or16(0, src), fetch32()));
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	void execute_4() { // Miscellaneous
		const int x = op >> 9 & 7, y = op & 7;
		int ea, data;
		switch (op >> 3 & 077) {
		case 000:
			switch (x) {
			case 0: // NEGX.B Dy
				return void(d[y] = d[y] & ~0xff | negx8(d[y]));
			case 1: // CLR.B Dy
				return void(d[y] = d[y] & ~0xff | clr());
			case 2: // NEG.B Dy
				return void(d[y] = d[y] & ~0xff | neg8(d[y]));
			case 3: // NOT.B Dy
				return void(d[y] = d[y] & ~0xff | or8(0, ~d[y]));
			case 4: // NBCD Dy
				return void(d[y] = d[y] & ~0xff | nbcd(d[y]));
			case 5: // TST.B Dy
				return void(or8(0, d[y]));
			}
			return exception(4);
		case 002:
			switch (x) {
			case 0: // NEGX.B (Ay)
				return write8(negx8(read8(a[y])), a[y]);
			case 1: // CLR.B (Ay)
				return read8(a[y]), write8(clr(), a[y]);
			case 2: // NEG.B (Ay)
				return write8(neg8(read8(a[y])), a[y]);
			case 3: // NOT.B (Ay)
				return write8(or8(0, ~read8(a[y])), a[y]);
			case 4: // NBCD (Ay)
				return write8(nbcd(read8(a[y])), a[y]);
			case 5: // TST.B (Ay)
				return void(or8(0, read8(a[y])));
			}
			return exception(4);
		case 003:
			switch (x) {
			case 0: // NEGX.B (Ay)+
				return write8(negx8(read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 1: // CLR.B (Ay)+
				return read8(a[y]), write8(clr(), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 2: // NEG.B (Ay)+
				return write8(neg8(read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 3: // NOT.B (Ay)+
				return write8(or8(0, ~read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 4: // NBCD (Ay)+
				return write8(nbcd(read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 5: // TST.B (Ay)+
				return or8(0, read8(a[y])), void(a[y] += y < 7 ? 1 : 2);
			}
			return exception(4);
		case 004:
			switch (x) {
			case 0: // NEGX.B -(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(negx8(read8(a[y])), a[y]);
			case 1: // CLR.B -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(clr(), a[y]);
			case 2: // NEG.B -(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(neg8(read8(a[y])), a[y]);
			case 3: // NOT.B -(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(or8(0, ~read8(a[y])), a[y]);
			case 4: // NBCD -(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(nbcd(read8(a[y])), a[y]);
			case 5: // TST.B -(Ay)
				return a[y] -= y < 7 ? 1 : 2, void(or8(0, read8(a[y])));
			}
			return exception(4);
		case 005:
			switch (x) {
			case 0: // NEGX.B d(Ay)
				return ea = disp(a[y]), write8(negx8(read8(ea)), ea);
			case 1: // CLR.B d(Ay)
				return ea = disp(a[y]), read8(ea), write8(clr(), ea);
			case 2: // NEG.B d(Ay)
				return ea = disp(a[y]), write8(neg8(read8(ea)), ea);
			case 3: // NOT.B d(Ay)
				return ea = disp(a[y]), write8(or8(0, ~read8(ea)), ea);
			case 4: // NBCD d(Ay)
				return ea = disp(a[y]), write8(nbcd(read8(ea)), ea);
			case 5: // TST.B d(Ay)
				return ea = disp(a[y]), void(or8(0, read8(ea)));
			}
			return exception(4);
		case 006:
			switch (x) {
			case 0: // NEGX.B d(Ay,Xi)
				return ea = index(a[y]), write8(negx8(read8(ea)), ea);
			case 1: // CLR.B d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(clr(), ea);
			case 2: // NEG.B d(Ay,Xi)
				return ea = index(a[y]), write8(neg8(read8(ea)), ea);
			case 3: // NOT.B d(Ay,Xi)
				return ea = index(a[y]), write8(or8(0, ~read8(ea)), ea);
			case 4: // NBCD d(Ay,Xi)
				return ea = index(a[y]), write8(nbcd(read8(ea)), ea);
			case 5: // TST.B d(Ay,Xi)
				return ea = index(a[y]), void(or8(0, read8(ea)));
			}
			return exception(4);
		case 007:
			switch (x << 3 | y) {
			case 000: // NEGX.B Abs.W
				return ea = fetch16s(), write8(negx8(read8(ea)), ea);
			case 001: // NEGX.B Abs.L
				return ea = fetch32(), write8(negx8(read8(ea)), ea);
			case 010: // CLR.B Abs.W
				return ea = fetch16s(), read8(ea), write8(clr(), ea);
			case 011: // CLR.B Abs.L
				return ea = fetch32(), read8(ea), write8(clr(), ea);
			case 020: // NEG.B Abs.W
				return ea = fetch16s(), write8(neg8(read8(ea)), ea);
			case 021: // NEG.B Abs.L
				return ea = fetch32(), write8(neg8(read8(ea)), ea);
			case 030: // NOT.B Abs.W
				return ea = fetch16s(), write8(or8(0, ~read8(ea)), ea);
			case 031: // NOT.B Abs.L
				return ea = fetch32(), write8(or8(0, ~read8(ea)), ea);
			case 040: // NBCD Abs.W
				return ea = fetch16s(), write8(nbcd(read8(ea)), ea);
			case 041: // NBCD Abs.L
				return ea = fetch32(), write8(nbcd(read8(ea)), ea);
			case 050: // TST.B Abs.W
				return void(or8(0, read8(fetch16s())));
			case 051: // TST.B Abs.L
				return void(or8(0, read8(fetch32())));
			}
			return exception(4);
		case 010:
			switch (x) {
			case 0: // NEGX.W Dy
				return void(d[y] = d[y] & ~0xffff | negx16(d[y]));
			case 1: // CLR.W Dy
				return void(d[y] = d[y] & ~0xffff | clr());
			case 2: // NEG.W Dy
				return void(d[y] = d[y] & ~0xffff | neg16(d[y]));
			case 3: // NOT.W Dy
				return void(d[y] = d[y] & ~0xffff | or16(0, ~d[y]));
			case 4: // SWAP Dy
				return void(d[y] = or32(0, d[y] << 16 | (unsigned int)d[y] >> 16));
			case 5: // TST.W Dy
				return void(or16(0, d[y]));
			case 7: // TRAP #<vector>
				return exception(y | 0x20);
			}
			return exception(4);
		case 011: // TRAP #<vector>
			return x != 7 ? exception(4) : exception(y | 0x28);
		case 012:
			switch (x) {
			case 0: // NEGX.W (Ay)
				return write16(negx16(read16(a[y])), a[y]);
			case 1: // CLR.W (Ay)
				return read16(a[y]), write16(clr(), a[y]);
			case 2: // NEG.W (Ay)
				return write16(neg16(read16(a[y])), a[y]);
			case 3: // NOT.W (Ay)
				return write16(or16(0, ~read16(a[y])), a[y]);
			case 4: // PEA (Ay)
				return ea = a[y], a[7] -= 4, write32(ea, a[7]);
			case 5: // TST.W (Ay)
				return void(or16(0, read16(a[y])));
			case 7: // LINK Ay,#<displacement>
				return a[7] -= 4, write32(a[y], a[7]), a[y] = a[7], void(a[7] = disp(a[7]));
			}
			return exception(4);
		case 013:
			switch (x) {
			case 0: // NEGX.W (Ay)+
				return write16(negx16(read16(a[y])), a[y]), void(a[y] += 2);
			case 1: // CLR.W (Ay)+
				return read16(a[y]), write16(clr(), a[y]), void(a[y] += 2);
			case 2: // NEG.W (Ay)+
				return write16(neg16(read16(a[y])), a[y]), void(a[y] += 2);
			case 3: // NOT.W (Ay)+
				return write16(or16(0, ~read16(a[y])), a[y]), void(a[y] += 2);
			case 5: // TST.W (Ay)+
				return or16(0, read16(a[y])), void(a[y] += 2);
			case 7: // UNLK Ay
				return a[7] = a[y], a[y] = read32(a[7]), void(a[7] += 4);
			}
			return exception(4);
		case 014:
			switch (x) {
			case 0: // NEGX.W -(Ay)
				return a[y] -= 2, write16(negx16(read16(a[y])), a[y]);
			case 1: // CLR.W -(Ay)
				return a[y] -= 2, read16(a[y]), write16(clr(), a[y]);
			case 2: // NEG.W -(Ay)
				return a[y] -= 2, write16(neg16(read16(a[y])), a[y]);
			case 3: // NOT.W -(Ay)
				return a[y] -= 2, write16(or16(0, ~read16(a[y])), a[y]);
			case 5: // TST.W -(Ay)
				return a[y] -= 2, void(or16(0, read16(a[y])));
			case 7: // MOVE Ay,USP
				return ~sr & 0x2000 ? exception(8) : void(usp = a[y]);
			}
			return exception(4);
		case 015:
			switch (x) {
			case 0: // NEGX.W d(Ay)
				return ea = disp(a[y]), write16(negx16(read16(ea)), ea);
			case 1: // CLR.W d(Ay)
				return ea = disp(a[y]), read16(ea), write16(clr(), ea);
			case 2: // NEG.W d(Ay)
				return ea = disp(a[y]), write16(neg16(read16(ea)), ea);
			case 3: // NOT.W d(Ay)
				return ea = disp(a[y]), write16(or16(0, ~read16(ea)), ea);
			case 4: // PEA d(Ay)
				return ea = disp(a[y]), a[7] -= 4, write32(ea, a[7]);
			case 5: // TST.W d(Ay)
				return ea = disp(a[y]), void(or16(0, read16(ea)));
			case 7: // MOVE USP,Ay
				return ~sr & 0x2000 ? exception(8) : void(a[y] = usp);
			}
			return exception(4);
		case 016:
			switch (x) {
			case 0: // NEGX.W d(Ay,Xi)
				return ea = index(a[y]), write16(negx16(read16(ea)), ea);
			case 1: // CLR.W d(Ay,Xi)
				return ea = index(a[y]), read16(ea), write16(clr(), ea);
			case 2: // NEG.W d(Ay,Xi)
				return ea = index(a[y]), write16(neg16(read16(ea)), ea);
			case 3: // NOT.W d(Ay,Xi)
				return ea = index(a[y]), write16(or16(0, ~read16(ea)), ea);
			case 4: // PEA d(Ay,Xi)
				return ea = index(a[y]), a[7] -= 4, write32(ea, a[7]);
			case 5: // TST.W d(Ay,Xi)
				return ea = index(a[y]), void(or16(0, read16(ea)));
			case 7:
				switch(y) {
				case 0: // RESET
					return ~sr & 0x2000 ? exception(8) : void(0);
				case 1: // NOP
					return;
				case 2: // STOP #<data>
					return ~sr & 0x2000 || ~(data = fetch16()) & 0x2000 ? exception(8) : (sr = data, void(fSuspend = true));
				case 3: // RTE
					return ~sr & 0x2000 ? exception(8) : rte();
				case 4: // RTD 68010
					return exception(4);
				case 5: // RTS
					return pc = read32(a[7]), void(a[7] += 4);
				case 6: // TRAPV
					return sr & 2 ? exception(7) : void(0);
				case 7: // RTR
					return sr = sr & ~0xff | read16(a[7]) & 0xff, a[7] += 2, pc = read32(a[7]), void(a[7] += 4);
				}
				return;
			}
			return exception(4);
		case 017:
			switch (x << 3 | y) {
			case 000: // NEGX.W Abs.W
				return ea = fetch16s(), write16(negx16(read16(ea)), ea);
			case 001: // NEGX.W Abs.L
				return ea = fetch32(), write16(negx16(read16(ea)), ea);
			case 010: // CLR.W Abs.W
				return ea = fetch16s(), read16(ea), write16(clr(), ea);
			case 011: // CLR.W Abs.L
				return ea = fetch32(), read16(ea), write16(clr(), ea);
			case 020: // NEG.W Abs.W
				return ea = fetch16s(), write16(neg16(read16(ea)), ea);
			case 021: // NEG.W Abs.L
				return ea = fetch32(), write16(neg16(read16(ea)), ea);
			case 030: // NOT.W Abs.W
				return ea = fetch16s(), write16(or16(0, ~read16(ea)), ea);
			case 031: // NOT.W Abs.L
				return ea = fetch32(), write16(or16(0, ~read16(ea)), ea);
			case 040: // PEA Abs.W
				return ea = fetch16s(), a[7] -= 4, write32(ea, a[7]);
			case 041: // PEA Abs.L
				return ea = fetch32(), a[7] -= 4, write32(ea, a[7]);
			case 042: // PEA d(PC)
				return ea = disp(pc), a[7] -= 4, write32(ea, a[7]);
			case 043: // PEA d(PC,Xi)
				return ea = index(pc), a[7] -= 4, write32(ea, a[7]);
			case 050: // TST.W Abs.W
				return void(or16(0, read16(fetch16s())));
			case 051: // TST.W Abs.L
				return void(or16(0, read16(fetch32())));
			}
			return exception(4);
		case 020:
			switch (x) {
			case 0: // NEGX.L Dy
				return void(d[y] = negx32(d[y]));
			case 1: // CLR.L Dy
				return void(d[y] = clr());
			case 2: // NEG.L Dy
				return void(d[y] = neg32(d[y]));
			case 3: // NOT.L Dy
				return void(d[y] = or32(0, ~d[y]));
			case 4: // EXT.W Dy
				return void(d[y] = d[y] & ~0xffff | or16(0, d[y] << 24 >> 24));
			case 5: // TST.L Dy
				return void(or32(0, d[y]));
			}
			return exception(4);
		case 022:
			switch (x) {
			case 0: // NEGX.L (Ay)
				return write32(negx32(read32(a[y])), a[y]);
			case 1: // CLR.L (Ay)
				return read32(a[y]), write32(clr(), a[y]);
			case 2: // NEG.L (Ay)
				return write32(neg32(read32(a[y])), a[y]);
			case 3: // NOT.L (Ay)
				return write32(or32(0, ~read32(a[y])), a[y]);
			case 4: // MOVEM.W <register list>,(Ay)
				return movem16rm();
			case 5: // TST.L (Ay)
				return void(or32(0, read32(a[y])));
			case 6: // MOVEM.W (Ay),<register list>
				return movem16mr();
			case 7: // JSR (Ay)
				return ea = a[y], a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			}
			return;
		case 023:
			switch (x) {
			case 0: // NEGX.L (Ay)+
				return write32(negx32(read32(a[y])), a[y]), void(a[y] += 4);
			case 1: // CLR.L (Ay)+
				return read32(a[y]), write32(clr(), a[y]), void(a[y] += 4);
			case 2: // NEG.L (Ay)+
				return write32(neg32(read32(a[y])), a[y]), void(a[y] += 4);
			case 3: // NOT.L (Ay)+
				return write32(or32(0, ~read32(a[y])), a[y]), void(a[y] += 4);
			case 5: // TST.L (Ay)+
				return or32(0, read32(a[y])), void(a[y] += 4);
			case 6: // MOVEM.W (Ay)+,<register list>
				return movem16mr();
			}
			return exception(4);
		case 024:
			switch (x) {
			case 0: // NEGX.L -(Ay)
				return a[y] -= 4, write32(negx32(read32(a[y])), a[y]);
			case 1: // CLR.L -(Ay)
				return a[y] -= 4, read32(a[y]), write32(clr(), a[y]);
			case 2: // NEG.L -(Ay)
				return a[y] -= 4, write32(neg32(read32(a[y])), a[y]);
			case 3: // NOT.L -(Ay)
				return a[y] -= 4, write32(or32(0, ~read32(a[y])), a[y]);
			case 4: // MOVEM.W <register list>,-(Ay)
				return movem16rm();
			case 5: // TST.L -(Ay)
				return a[y] -= 4, void(or32(0, read32(a[y])));
			}
			return exception(4);
		case 025:
			switch (x) {
			case 0: // NEGX.L d(Ay)
				return ea = disp(a[y]), write32(negx32(read32(ea)), ea);
			case 1: // CLR.L d(Ay)
				return ea = disp(a[y]), read32(ea), write32(clr(), ea);
			case 2: // NEG.L d(Ay)
				return ea = disp(a[y]), write32(neg32(read32(ea)), ea);
			case 3: // NOT.L d(Ay)
				return ea = disp(a[y]), write32(or32(0, ~read32(ea)), ea);
			case 4: // MOVEM.W <register list>,d(Ay)
				return movem16rm();
			case 5: // TST.L d(Ay)
				return ea = disp(a[y]), void(or32(0, read32(ea)));
			case 6: // MOVEM.W d(Ay),<register list>
				return movem16mr();
			case 7: // JSR d(Ay)
				return ea = disp(a[y]), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			}
			return;
		case 026:
			switch (x) {
			case 0: // NEGX.L d(Ay,Xi)
				return ea = index(a[y]), write32(negx32(read32(ea)), ea);
			case 1: // CLR.L d(Ay,Xi)
				return ea = index(a[y]), read32(ea), write32(clr(), ea);
			case 2: // NEG.L d(Ay,Xi)
				return ea = index(a[y]), write32(neg32(read32(ea)), ea);
			case 3: // NOT.L d(Ay,Xi)
				return ea = index(a[y]), write32(or32(0, ~read32(ea)), ea);
			case 4: // MOVEM.W <register list>,d(Ay,Xi)
				return movem16rm();
			case 5: // TST.L d(Ay,Xi)
				return ea = index(a[y]), void(or32(0, read32(ea)));
			case 6: // MOVEM.W d(Ay,Xi),<register list>
				return movem16mr();
			case 7: // JSR d(Ay,Xi)
				return ea = index(a[y]), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			}
			return;
		case 027:
			switch (x << 3 | y) {
			case 000: // NEGX.L Abs.W
				return ea = fetch16s(), write32(negx32(read32(ea)), ea);
			case 001: // NEGX.L Abs.L
				return ea = fetch32(), write32(negx32(read32(ea)), ea);
			case 010: // CLR.L Abs.W
				return ea = fetch16s(), read32(ea), write32(clr(), ea);
			case 011: // CLR.L Abs.L
				return ea = fetch32(), read32(ea), write32(clr(), ea);
			case 020: // NEG.L Abs.W
				return ea = fetch16s(), write32(neg32(read32(ea)), ea);
			case 021: // NEG.L Abs.L
				return ea = fetch32(), write32(neg32(read32(ea)), ea);
			case 030: // NOT.L Abs.W
				return ea = fetch16s(), write32(or32(0, ~read32(ea)), ea);
			case 031: // NOT.L Abs.L
				return ea = fetch32(), write32(or32(0, ~read32(ea)), ea);
			case 040: // MOVEM.W <register list>,Abs.W
			case 041: // MOVEM.W <register list>,Abs.L
				return movem16rm();
			case 050: // TST.L Abs.W
				return void(or32(0, read32(fetch16s())));
			case 051: // TST.L Abs.L
				return void(or32(0, read32(fetch32())));
			case 060: // MOVEM.W Abs.W,<register list>
			case 061: // MOVEM.W Abs.L,<register list>
			case 062: // MOVEM.W d(PC),<register list>
			case 063: // MOVEM.W d(PC,Xi),<register list>
				return movem16mr();
			case 070: // JSR Abs.W
				return ea = fetch16s(), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			case 071: // JSR Abs.L
				return ea = fetch32(), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			case 072: // JSR d(PC)
				return ea = disp(pc), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			case 073: // JSR d(PC,Xi)
				return ea = index(pc), a[7] -= 4, write32(pc, a[7]), void(pc = ea);
			}
			return exception(4);
		case 030:
			switch (x) {
			case 0: // MOVE SR,Dy
				return void(d[y] = d[y] & ~0xffff | sr);
			case 2: // MOVE Dy,CCR
				return void(sr = sr & ~0xff | d[y] & 0xff);
			case 3: // MOVE Dy,SR
				return ~sr & 0x2000 ? exception(8) : (sr = d[y] & 0xffff, void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 4: // EXT.L Dy
				return void(d[y] = or32(0, d[y] << 16 >> 16));
			case 5: // TAS Dy
				return void(d[y] = d[y] & ~0xff | or8(0, d[y]) | 0x80);
			}
			return exception(4);
		case 032:
			switch (x) {
			case 0: // MOVE SR,(Ay)
				return read16(a[y]), write16(sr, a[y]);
			case 2: // MOVE (Ay),CCR
				return void(sr = sr & ~0xff | read16(a[y]) & 0xff);
			case 3: // MOVE (Ay),SR
				return ~sr & 0x2000 ? exception(8) : (sr = read16(a[y]), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 4: // MOVEM.L <register list>,(Ay)
				return movem32rm();
			case 5: // TAS (Ay)
				return write8(or8(0, read8(a[y])) | 0x80, a[y]);
			case 6: // MOVEM.L (Ay),<register list>
				return movem32mr();
			case 7: // JMP (Ay)
				return void(pc = a[y]);
			}
			return exception(4);
		case 033:
			switch (x) {
			case 0: // MOVE SR,(Ay)+
				return read16(a[y]), write16(sr, a[y]), void(a[y] += 2);
			case 2: // MOVE (Ay)+,CCR
				return sr = sr & ~0xff | read16(a[y]) & 0xff, void(a[y] += 2);
			case 3: // MOVE (Ay)+,SR
				return ~sr & 0x2000 ? exception(8) : (sr = read16(a[y]), a[y] += 2, void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 5: // TAS (Ay)+
				return write8(or8(0, read8(a[y])) | 0x80, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 6: // MOVEM.L (Ay)+,<register list>
				return movem32mr();
			}
			return exception(4);
		case 034:
			switch (x) {
			case 0: // MOVE SR,-(Ay)
				return a[y] -= 2, read16(a[y]), write16(sr, a[y]);
			case 2: // MOVE -(Ay),CCR
				return a[y] -= 2, void(sr = sr & ~0xff | read16(a[y]) & 0xff);
			case 3: // MOVE -(Ay),SR
				return ~sr & 0x2000 ? exception(8) : (a[y] -= 2, sr = read16(a[y]), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 4: // MOVEM.L <register list>,-(Ay)
				return movem32rm();
			case 5: // TAS -(Ay)
				return a[y] -= y < 7 ? 1 : 2, write8(or8(0, read8(a[y])) | 0x80, a[y]);
			}
			return exception(4);
		case 035:
			switch (x) {
			case 0: // MOVE SR,d(Ay)
				return ea = disp(a[y]), read16(ea), write16(sr, ea);
			case 2: // MOVE d(Ay),CCR
				return ea = disp(a[y]), void(sr = sr & ~0xff | read16(ea) & 0xff);
			case 3: // MOVE d(Ay),SR
				return ~sr & 0x2000 ? exception(8) : (ea = disp(a[y]), sr = read16(ea), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 4: // MOVEM.L <register list>,d(Ay)
				return movem32rm();
			case 5: // TAS d(Ay)
				return ea = disp(a[y]), write8(or8(0, read8(ea)) | 0x80, ea);
			case 6: // MOVEM.L d(Ay),<register list>
				return movem32mr();
			case 7: // JMP d(Ay)
				return void(pc = disp(a[y]));
			}
			return exception(4);
		case 036:
			switch (x) {
			case 0: // MOVE SR,d(Ay,Xi)
				return ea = index(a[y]), read16(ea), write16(sr, ea);
			case 2: // MOVE d(Ay,Xi),CCR
				return ea = index(a[y]), void(sr = sr & ~0xff | read16(ea) & 0xff);
			case 3: // MOVE d(Ay,Xi),SR
				return ~sr & 0x2000 ? exception(8) : (ea = index(a[y]), sr = read16(ea), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 4: // MOVEM.L <register list>,d(Ay,Xi)
				return movem32rm();
			case 5: // TAS d(Ay,Xi)
				return ea = index(a[y]), write8(or8(0, read8(ea)) | 0x80, ea);
			case 6: // MOVEM.L d(Ay,Xi),<register list>
				return movem32mr();
			case 7: // JMP d(Ay,Xi)
				return void(pc = index(a[y]));
			}
			return exception(4);
		case 037:
			switch (x << 3 | y) {
			case 000: // MOVE SR,Abs.W
				return ea = fetch16s(), read16(ea), write16(sr, ea);
			case 001: // MOVE SR,Abs.L
				return ea = fetch32(), read16(ea), write16(sr, ea);
			case 020: // MOVE Abs.W,CCR
			case 021: // MOVE Abs.L,CCR
			case 022: // MOVE d(PC),CCR
			case 023: // MOVE d(PC,Xi),CCR
			case 024: // MOVE #<data>,CCR
				return void(sr = sr & ~0xff | rop16() & 0xff);
			case 030: // MOVE Abs.W,SR
			case 031: // MOVE Abs.L,SR
			case 032: // MOVE d(PC),SR
			case 033: // MOVE d(PC,Xi),SR
			case 034: // MOVE #<data>,SR
				return ~sr & 0x2000 ? exception(8) : (sr = rop16(), void(~sr & 0x2000 && (ssp = a[7], a[7] = usp)));
			case 040: // MOVEM.L <register list>,Abs.W
			case 041: // MOVEM.L <register list>,Abs.L
				return movem32rm();
			case 050: // TAS Abs.W
				return ea = fetch16s(), write8(or8(0, read8(ea)) | 0x80, ea);
			case 051: // TAS Abs.L
				return ea = fetch32(), write8(or8(0, read8(ea)) | 0x80, ea);
			case 054: // ILLEGAL
				return exception(4);
			case 060: // MOVEM.L Abs.W,<register list>
			case 061: // MOVEM.L Abs.L,<register list>
			case 062: // MOVEM.L d(PC),<register list>
			case 063: // MOVEM.L d(PC,Xi),<register list>
				return movem32mr();
			case 070: // JMP Abs.W
				return void(pc = fetch16s());
			case 071: // JMP Abs.L
				return void(pc = fetch32());
			case 072: // JMP d(PC)
				return void(pc = disp(pc));
			case 073: // JMP d(PC,Xi)
				return void(pc = index(pc));
			}
			return exception(4);
		case 060: // CHK Dy,Dx
			return chk(d[y], d[x]);
		case 062: // CHK (Ay),Dx
			return chk(read16(a[y]), d[x]);
		case 063: // CHK (Ay)+,Dx
			return chk(read16(a[y]), d[x]), void(a[y] += 2);
		case 064: // CHK -(Ay),Dx
			return a[y] -= 2, chk(read16(a[y]), d[x]);
		case 065: // CHK d(Ay),Dx
			return ea = disp(a[y]), chk(read16(ea), d[x]);
		case 066: // CHK d(Ay,Xi),Dx
			return ea = index(a[y]), chk(read16(ea), d[x]);
		case 067: // CHK Abs...,Dx
			return y >= 5 ? exception(4) : chk(rop16(), d[x]);
		case 072: // LEA (Ay),Ax
			return void(a[x] = a[y]);
		case 075: // LEA d(Ay),Ax
			return void(a[x] = disp(a[y]));
		case 076: // LEA d(Ay,Xi),Ax
			return void(a[x] = index(a[y]));
		case 077: // LEA Abs...,Ax
			return y >= 4 ? exception(4) : void(a[x] = lea());
		default:
			return exception(4);
		}
	}

	void execute_5() { // ADDQ/SUBQ/Scc/DBcc
		const int x = op >> 9 & 7, y = op & 7, data = (x - 1 & 7) + 1;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // ADDQ.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | add8(data, d[y]));
		case 002: // ADDQ.B #<data>,(Ay)
			return write8(add8(data, read8(a[y])), a[y]);
		case 003: // ADDQ.B #<data>,(Ay)+
			return write8(add8(data, read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // ADDQ.B #<data>,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(add8(data, read8(a[y])), a[y]);
		case 005: // ADDQ.B #<data>,d(Ay)
			return ea = disp(a[y]), write8(add8(data, read8(ea)), ea);
		case 006: // ADDQ.B #<data>,d(Ay,Xi)
			return ea = index(a[y]), write8(add8(data, read8(ea)), ea);
		case 007:
			switch (y) {
			case 0: // ADDQ.B #<data>,Abs.W
				return ea = fetch16s(), write8(add8(data, read8(ea)), ea);
			case 1: // ADDQ.B #<data>,Abs.L
				return ea = fetch32(), write8(add8(data, read8(ea)), ea);
			}
			return exception(4);
		case 010: // ADDQ.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | add16(data, d[y]));
		case 011: // ADDQ.W #<data>,Ay
			return void(a[y] += data);
		case 012: // ADDQ.W #<data>,(Ay)
			return write16(add16(data, read16(a[y])), a[y]);
		case 013: // ADDQ.W #<data>,(Ay)+
			return write16(add16(data, read16(a[y])), a[y]), void(a[y] += 2);
		case 014: // ADDQ.W #<data>,-(Ay)
			return a[y] -= 2, write16(add16(data, read16(a[y])), a[y]);
		case 015: // ADDQ.W #<data>,d(Ay)
			return ea = disp(a[y]), write16(add16(data, read16(ea)), ea);
		case 016: // ADDQ.W #<data>,d(Ay,Xi)
			return ea = index(a[y]), write16(add16(data, read16(ea)), ea);
		case 017:
			switch (y) {
			case 0: // ADDQ.W #<data>,Abs.W
				return ea = fetch16s(), write16(add16(data, read16(ea)), ea);
			case 1: // ADDQ.W #<data>,Abs.L
				return ea = fetch32(), write16(add16(data, read16(ea)), ea);
			}
			return exception(4);
		case 020: // ADDQ.L #<data>,Dy
			return void(d[y] = add32(data, d[y]));
		case 021: // ADDQ.L #<data>,Ay
			return void(a[y] += data);
		case 022: // ADDQ.L #<data>,(Ay)
			return write32(add32(data, read32(a[y])), a[y]);
		case 023: // ADDQ.L #<data>,(Ay)+
			return write32(add32(data, read32(a[y])), a[y]), void(a[y] += 4);
		case 024: // ADDQ.L #<data>,-(Ay)
			return a[y] -= 4, write32(add32(data, read32(a[y])), a[y]);
		case 025: // ADDQ.L #<data>,d(Ay)
			return ea = disp(a[y]), write32(add32(data, read32(ea)), ea);
		case 026: // ADDQ.L #<data>,d(Ay,Xi)
			return ea = index(a[y]), write32(add32(data, read32(ea)), ea);
		case 027:
			switch (y) {
			case 0: // ADDQ.L #<data>,Abs.W
				return ea = fetch16s(), write32(add32(data, read32(ea)), ea);
			case 1: // ADDQ.L #<data>,Abs.L
				return ea = fetch32(), write32(add32(data, read32(ea)), ea);
			}
			return exception(4);
		case 030: // Scc Dy
			switch (x) {
			case 0: // ST Dy
				return void(d[y] |= 0xff);
			case 1: // SHI Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 | sr) & 1 ? 0 : 0xff));
			case 2: // SCC Dy
				return void(d[y] = d[y] & ~0xff | (sr & 1 ? 0 : 0xff));
			case 3: // SNE Dy
				return void(d[y] = d[y] & ~0xff | (sr & 4 ? 0 : 0xff));
			case 4: // SVC Dy
				return void(d[y] = d[y] & ~0xff | (sr & 2 ? 0 : 0xff));
			case 5: // SPL Dy
				return void(d[y] = d[y] & ~0xff | (sr & 8 ? 0 : 0xff));
			case 6: // SGE Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 ^ sr) & 2 ? 0 : 0xff));
			case 7: // SGT Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff));
			}
			return;
		case 031: // DBcc Dy,<label>
			switch (x) {
			case 0: // DBT Dy,<label>
				return dbcc(true);
			case 1: // DBHI Dy,<label>
				return dbcc(!((sr >> 2 | sr) & 1));
			case 2: // DBCC Dy,<label>
				return dbcc(!(sr & 1));
			case 3: // DBNE Dy,<label>
				return dbcc(!(sr & 4));
			case 4: // DBVC Dy,<label>
				return dbcc(!(sr & 2));
			case 5: // DBPL Dy,<label>
				return dbcc(!(sr & 8));
			case 6: // DBGE Dy,<label>
				return dbcc(!((sr >> 2 ^ sr) & 2));
			case 7: // DBGT Dy,<label>
				return dbcc(!((sr >> 2 ^ sr | sr >> 1) & 2));
			}
			return;
		case 032: // Scc (Ay)
			switch (x) {
			case 0: // ST (Ay)
				return read8(a[y]), write8(0xff, a[y]);
			case 1: // SHI (Ay)
				return read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, a[y]);
			case 2: // SCC (Ay)
				return read8(a[y]), write8(sr & 1 ? 0 : 0xff, a[y]);
			case 3: // SNE (Ay)
				return read8(a[y]), write8(sr & 4 ? 0 : 0xff, a[y]);
			case 4: // SVC (Ay)
				return read8(a[y]), write8(sr & 2 ? 0 : 0xff, a[y]);
			case 5: // SPL (Ay)
				return read8(a[y]), write8(sr & 8 ? 0 : 0xff, a[y]);
			case 6: // SGE (Ay)
				return read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, a[y]);
			case 7: // SGT (Ay)
				return read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, a[y]);
			}
			return;
		case 033: // Scc (Ay)+
			switch (x) {
			case 0: // ST (Ay)+
				return read8(a[y]), write8(0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 1: // SHI (Ay)+
				return read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 2: // SCC (Ay)+
				return read8(a[y]), write8(sr & 1 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 3: // SNE (Ay)+
				return read8(a[y]), write8(sr & 4 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 4: // SVC (Ay)+
				return read8(a[y]), write8(sr & 2 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 5: // SPL (Ay)+
				return read8(a[y]), write8(sr & 8 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 6: // SGE (Ay)+
				return read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 7: // SGT (Ay)+
				return read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, a[y]), void(a[y] += y < 7 ? 1 : 2);
			}
			return;
		case 034: // Scc -(Ay)
			switch (x) {
			case 0: // ST -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(0xff, a[y]);
			case 1: // SHI -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, a[y]);
			case 2: // SCC -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 1 ? 0 : 0xff, a[y]);
			case 3: // SNE -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 4 ? 0 : 0xff, a[y]);
			case 4: // SVC -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 2 ? 0 : 0xff, a[y]);
			case 5: // SPL -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 8 ? 0 : 0xff, a[y]);
			case 6: // SGE -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, a[y]);
			case 7: // SGT -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, a[y]);
			}
			return;
		case 035: // Scc d(Ay)
			switch (x) {
			case 0: // ST d(Ay)
				return ea = disp(a[y]), read8(ea), write8(0xff, ea);
			case 1: // SHI d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, ea);
			case 2: // SCC d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 1 ? 0 : 0xff, ea);
			case 3: // SNE d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 4 ? 0 : 0xff, ea);
			case 4: // SVC d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 2 ? 0 : 0xff, ea);
			case 5: // SPL d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 8 ? 0 : 0xff, ea);
			case 6: // SGE d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, ea);
			case 7: // SGT d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, ea);
			}
			return;
		case 036: // Scc d(Ay,Xi)
			switch (x) {
			case 0: // ST d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(0xff, ea);
			case 1: // SHI d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, ea);
			case 2: // SCC d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 1 ? 0 : 0xff, ea);
			case 3: // SNE d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 4 ? 0 : 0xff, ea);
			case 4: // SVC d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 2 ? 0 : 0xff, ea);
			case 5: // SPL d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 8 ? 0 : 0xff, ea);
			case 6: // SGE d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, ea);
			case 7: // SGT d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, ea);
			}
			return;
		case 037: // Scc Abs...
			switch (x << 3 | y) {
			case 000: // ST Abs.W
				return ea = fetch16s(), read8(ea), write8(0xff, ea);
			case 001: // ST Abs.L
				return ea = fetch32(), read8(ea), write8(0xff, ea);
			case 010: // SHI Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, ea);
			case 011: // SHI Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 | sr) & 1 ? 0 : 0xff, ea);
			case 020: // SCC Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 1 ? 0 : 0xff, ea);
			case 021: // SCC Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 1 ? 0 : 0xff, ea);
			case 030: // SNE Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 4 ? 0 : 0xff, ea);
			case 031: // SNE Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 4 ? 0 : 0xff, ea);
			case 040: // SVC Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 2 ? 0 : 0xff, ea);
			case 041: // SVC Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 2 ? 0 : 0xff, ea);
			case 050: // SPL Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 8 ? 0 : 0xff, ea);
			case 051: // SPL Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 8 ? 0 : 0xff, ea);
			case 060: // SGE Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, ea);
			case 061: // SGE Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0 : 0xff, ea);
			case 070: // SGT Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, ea);
			case 071: // SGT Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0 : 0xff, ea);
			}
			return exception(4);
		case 040: // SUBQ.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | sub8(data, d[y]));
		case 042: // SUBQ.B #<data>,(Ay)
			return write8(sub8(data, read8(a[y])), a[y]);
		case 043: // SUBQ.B #<data>,(Ay)+
			return write8(sub8(data, read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // SUBQ.B #<data>,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(sub8(data, read8(a[y])), a[y]);
		case 045: // SUBQ.B #<data>,d(Ay)
			return ea = disp(a[y]), write8(sub8(data, read8(ea)), ea);
		case 046: // SUBQ.B #<data>,d(Ay,Xi)
			return ea = index(a[y]), write8(sub8(data, read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // SUBQ.B #<data>,Abs.W
				return ea = fetch16s(), write8(sub8(data, read8(ea)), ea);
			case 1: // SUBQ.B #<data>,Abs.L
				return ea = fetch32(), write8(sub8(data, read8(ea)), ea);
			}
			return exception(4);
		case 050: // SUBQ.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | sub16(data, d[y]));
		case 051: // SUBQ.W #<data>,Ay
			return void(a[y] -= data);
		case 052: // SUBQ.W #<data>,(Ay)
			return write16(sub16(data, read16(a[y])), a[y]);
		case 053: // SUBQ.W #<data>,(Ay)+
			return write16(sub16(data, read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // SUBQ.W #<data>,-(Ay)
			return a[y] -= 2, write16(sub16(data, read16(a[y])), a[y]);
		case 055: // SUBQ.W #<data>,d(Ay)
			return ea = disp(a[y]), write16(sub16(data, read16(ea)), ea);
		case 056: // SUBQ.W #<data>,d(Ay,Xi)
			return ea = index(a[y]), write16(sub16(data, read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // SUBQ.W #<data>,Abs.W
				return ea = fetch16s(), write16(sub16(data, read16(ea)), ea);
			case 1: // SUBQ.W #<data>,Abs.L
				return ea = fetch32(), write16(sub16(data, read16(ea)), ea);
			}
			return exception(4);
		case 060: // SUBQ.L #<data>,Dy
			return void(d[y] = sub32(data, d[y]));
		case 061: // SUBQ.L #<data>,Ay
			return void(a[y] -= data);
		case 062: // SUBQ.L #<data>,(Ay)
			return write32(sub32(data, read32(a[y])), a[y]);
		case 063: // SUBQ.L #<data>,(Ay)+
			return write32(sub32(data, read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // SUBQ.L #<data>,-(Ay)
			return a[y] -= 4, write32(sub32(data, read32(a[y])), a[y]);
		case 065: // SUBQ.L #<data>,d(Ay)
			return ea = disp(a[y]), write32(sub32(data, read32(ea)), ea);
		case 066: // SUBQ.L #<data>,d(Ay,Xi)
			return ea = index(a[y]), write32(sub32(data, read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // SUBQ.L #<data>,Abs.W
				return ea = fetch16s(), write32(sub32(data, read32(ea)), ea);
			case 1: // SUBQ.L #<data>,Abs.L
				return ea = fetch32(), write32(sub32(data, read32(ea)), ea);
			}
			return exception(4);
		case 070: // Scc Dy
			switch (x) {
			case 0: // SF Dy
				return void(d[y] &= ~0xff);
			case 1: // SLS Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 | sr) & 1 ? 0xff : 0));
			case 2: // SCS Dy
				return void(d[y] = d[y] & ~0xff | (sr & 1 ? 0xff : 0));
			case 3: // SEQ Dy
				return void(d[y] = d[y] & ~0xff | (sr & 4 ? 0xff : 0));
			case 4: // SVS Dy
				return void(d[y] = d[y] & ~0xff | (sr & 2 ? 0xff : 0));
			case 5: // SMI Dy
				return void(d[y] = d[y] & ~0xff | (sr & 8 ? 0xff : 0));
			case 6: // SLT Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 ^ sr) & 2 ? 0xff : 0));
			case 7: // SLE Dy
				return void(d[y] = d[y] & ~0xff | ((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0));
			}
			return;
		case 071: // DBcc Dy,<label>
			switch (x) {
			case 0: // DBRA Dy,<label>
				return dbcc(false);
			case 1: // DBLS Dy,<label>
				return dbcc(((sr >> 2 | sr) & 1) != 0);
			case 2: // DBCS Dy,<label>
				return dbcc((sr & 1) != 0);
			case 3: // DBEQ Dy,<label>
				return dbcc((sr & 4) != 0);
			case 4: // DBVS Dy,<label>
				return dbcc((sr & 2) != 0);
			case 5: // DBMI Dy,<label>
				return dbcc((sr & 8) != 0);
			case 6: // DBLT Dy,<label>
				return dbcc(((sr >> 2 ^ sr) & 2) != 0);
			case 7: // DBLE Dy,<label>
				return dbcc(((sr >> 2 ^ sr | sr >> 1) & 2) != 0);
			}
			return;
		case 072: // Scc (Ay)
			switch (x) {
			case 0: // SF (Ay)
				return read8(a[y]), write8(0, a[y]);
			case 1: // SLS (Ay)
				return read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0xff : 0, a[y]);
			case 2: // SCS (Ay)
				return read8(a[y]), write8(sr & 1 ? 0xff : 0, a[y]);
			case 3: // SEQ (Ay)
				return read8(a[y]), write8(sr & 4 ? 0xff : 0, a[y]);
			case 4: // SVS (Ay)
				return read8(a[y]), write8(sr & 2 ? 0xff : 0, a[y]);
			case 5: // SMI (Ay)
				return read8(a[y]), write8(sr & 8 ? 0xff : 0, a[y]);
			case 6: // SLT (Ay)
				return read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, a[y]);
			case 7: // SLE (Ay)
				return read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, a[y]);
			}
			return;
		case 073: // Scc (Ay)+
			switch (x) {
			case 0: // SF (Ay)+
				return read8(a[y]), write8(0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 1: // SLS (Ay)+
				return read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 2: // SCS (Ay)+
				return read8(a[y]), write8(sr & 1 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 3: // SEQ (Ay)+
				return read8(a[y]), write8(sr & 4 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 4: // SVS (Ay)+
				return read8(a[y]), write8(sr & 2 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 5: // SMI (Ay)+
				return read8(a[y]), write8(sr & 8 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 6: // SLT (Ay)+
				return read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			case 7: // SLE (Ay)+
				return read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, a[y]), void(a[y] += y < 7 ? 1 : 2);
			}
			return;
		case 074: // Scc -(Ay)
			switch (x) {
			case 0: // SF -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(0, a[y]);
			case 1: // SLS -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 | sr) & 1 ? 0xff : 0, a[y]);
			case 2: // SCS -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 1 ? 0xff : 0, a[y]);
			case 3: // SEQ -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 4 ? 0xff : 0, a[y]);
			case 4: // SVS -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 2 ? 0xff : 0, a[y]);
			case 5: // SMI -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8(sr & 8 ? 0xff : 0, a[y]);
			case 6: // SLT -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, a[y]);
			case 7: // SLE -(Ay)
				return a[y] -= y < 7 ? 1 : 2, read8(a[y]), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, a[y]);
			}
			return;
		case 075: // Scc d(Ay)
			switch (x) {
			case 0: // SF d(Ay)
				return ea = disp(a[y]), read8(ea), write8(0, ea);
			case 1: // SLS d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 | sr) & 1 ? 0xff : 0, ea);
			case 2: // SCS d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 1 ? 0xff : 0, ea);
			case 3: // SEQ d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 4 ? 0xff : 0, ea);
			case 4: // SVS d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 2 ? 0xff : 0, ea);
			case 5: // SMI d(Ay)
				return ea = disp(a[y]), read8(ea), write8(sr & 8 ? 0xff : 0, ea);
			case 6: // SLT d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, ea);
			case 7: // SLE d(Ay)
				return ea = disp(a[y]), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, ea);
			}
			return;
		case 076: // Scc d(Ay,Xi)
			switch (x) {
			case 0: // SF d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(0, ea);
			case 1: // SLS d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 | sr) & 1 ? 0xff : 0, ea);
			case 2: // SCS d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 1 ? 0xff : 0, ea);
			case 3: // SEQ d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 4 ? 0xff : 0, ea);
			case 4: // SVS d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 2 ? 0xff : 0, ea);
			case 5: // SMI d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8(sr & 8 ? 0xff : 0, ea);
			case 6: // SLT d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, ea);
			case 7: // SLE d(Ay,Xi)
				return ea = index(a[y]), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, ea);
			}
			return;
		case 077: // Scc Abs...
			switch (x << 3 | y) {
			case 000: // SF Abs.W
				return ea = fetch16s(), read8(ea), write8(0, ea);
			case 001: // SF Abs.L
				return ea = fetch32(), read8(ea), write8(0, ea);
			case 010: // SLS Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 | sr) & 1 ? 0xff : 0, ea);
			case 011: // SLS Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 | sr) & 1 ? 0xff : 0, ea);
			case 020: // SCS Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 1 ? 0xff : 0, ea);
			case 021: // SCS Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 1 ? 0xff : 0, ea);
			case 030: // SEQ Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 4 ? 0xff : 0, ea);
			case 031: // SEQ Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 4 ? 0xff : 0, ea);
			case 040: // SVS Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 2 ? 0xff : 0, ea);
			case 041: // SVS Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 2 ? 0xff : 0, ea);
			case 050: // SMI Abs.W
				return ea = fetch16s(), read8(ea), write8(sr & 8 ? 0xff : 0, ea);
			case 051: // SMI Abs.L
				return ea = fetch32(), read8(ea), write8(sr & 8 ? 0xff : 0, ea);
			case 060: // SLT Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, ea);
			case 061: // SLT Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 ^ sr) & 2 ? 0xff : 0, ea);
			case 070: // SLE Abs.W
				return ea = fetch16s(), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, ea);
			case 071: // SLE Abs.L
				return ea = fetch32(), read8(ea), write8((sr >> 2 ^ sr | sr >> 1) & 2 ? 0xff : 0, ea);
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	void execute_6() { // Bcc/BSR
		const int addr = op & 0xff ? pc + (op << 24 >> 24) : disp(pc);
		switch (op >> 8 & 0xf) {
		case 0x0: // BRA <label>
			return void(pc = addr);
		case 0x1: // BSR <label>
			return a[7] -= 4, write32(pc, a[7]), void(pc = addr);
		case 0x2: // BHI <label>
			return void(~(sr >> 2 | sr) & 1 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x3: // BLS <label>
			return void((sr >> 2 | sr) & 1 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x4: // BCC <label>
			return void(~sr & 1 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x5: // BCS <label>
			return void(sr & 1 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x6: // BNE <label>
			return void(~sr & 4 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x7: // BEQ <label>
			return void(sr & 4 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x8: // BVC <label>
			return void(~sr & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0x9: // BVS <label>
			return void(sr & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xa: // BPL <label>
			return void(~sr & 8 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xb: // BMI <label>
			return void(sr & 8 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xc: // BGE <label>
			return void(~(sr >> 2 ^ sr) & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xd: // BLT <label>
			return void((sr >> 2 ^ sr) & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xe: // BGT <label>
			return void(~(sr >> 2 ^ sr | sr >> 1) & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		case 0xf: // BLE <label>
			return void((sr >> 2 ^ sr | sr >> 1) & 2 ? (pc = addr) : (cycle -= op & 0xff ? -2 : 2));
		}
	}

	void execute_7() { // MOVEQ
		const int n = op >> 9 & 7, data = op << 24 >> 24;
		op & 0x100 ? exception(4) : (d[n] = data, void(sr = sr & ~0x0f | data >> 28 & 8 | !data << 2));
	}

	void execute_8() { // OR/DIV/SBCD
		const int x = op >> 9 & 7, y = op & 7;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // OR.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | or8(d[y], d[x]));
		case 002: // OR.B (Ay),Dx
			return void(d[x] = d[x] & ~0xff | or8(read8(a[y]), d[x]));
		case 003: // OR.B (Ay)+,Dx
			return d[x] = d[x] & ~0xff | or8(read8(a[y]), d[x]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // OR.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, void(d[x] = d[x] & ~0xff | or8(read8(a[y]), d[x]));
		case 005: // OR.B d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xff | or8(read8(ea), d[x]));
		case 006: // OR.B d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xff | or8(read8(ea), d[x]));
		case 007: // OR.B Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xff | or8(rop8(), d[x]));
		case 010: // OR.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | or16(d[y], d[x]));
		case 012: // OR.W (Ay),Dx
			return void(d[x] = d[x] & ~0xffff | or16(read16(a[y]), d[x]));
		case 013: // OR.W (Ay)+,Dx
			return d[x] = d[x] & ~0xffff | or16(read16(a[y]), d[x]), void(a[y] += 2);
		case 014: // OR.W -(Ay),Dx
			return a[y] -= 2, void(d[x] = d[x] & ~0xffff | or16(read16(a[y]), d[x]));
		case 015: // OR.W d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xffff | or16(read16(ea), d[x]));
		case 016: // OR.W d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xffff | or16(read16(ea), d[x]));
		case 017: // OR.W Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xffff | or16(rop16(), d[x]));
		case 020: // OR.L Dy,Dx
			return void(d[x] = or32(d[y], d[x]));
		case 022: // OR.L (Ay),Dx
			return void(d[x] = or32(read32(a[y]), d[x]));
		case 023: // OR.L (Ay)+,Dx
			return d[x] = or32(read32(a[y]), d[x]), void(a[y] += 4);
		case 024: // OR.L -(Ay),Dx
			return a[y] -= 4, void(d[x] = or32(read32(a[y]), d[x]));
		case 025: // OR.L d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = or32(read32(ea), d[x]));
		case 026: // OR.L d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = or32(read32(ea), d[x]));
		case 027: // OR.L Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = or32(rop32(), d[x]));
		case 030: // DIVU Dy,Dx
			return void(d[x] = divu(d[y], d[x]));
		case 032: // DIVU (Ay),Dx
			return void(d[x] = divu(read16(a[y]), d[x]));
		case 033: // DIVU (Ay)+,Dx
			return d[x] = divu(read16(a[y]), d[x]), void(a[y] += 2);
		case 034: // DIVU -(Ay),Dx
			return a[y] -= 2, void(d[x] = divu(read16(a[y]), d[x]));
		case 035: // DIVU d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = divu(read16(ea), d[x]));
		case 036: // DIVU d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = divu(read16(ea), d[x]));
		case 037: // DIVU Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = divu(rop16(), d[x]));
		case 040: // SBCD Dy,Dx
			return void(d[x] = d[x] & ~0xff | sbcd(d[y], d[x]));
		case 041: // SBCD -(Ay),-(Ax)
			return a[y] -= y < 7 ? 1 : 2, a[x] -= x < 7 ? 1 : 2, write8(sbcd(read8(a[y]), read8(a[x])), a[x]);
		case 042: // OR.B Dx,(Ay)
			return write8(or8(d[x], read8(a[y])), a[y]);
		case 043: // OR.B Dx,(Ay)+
			return write8(or8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // OR.B Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(or8(d[x], read8(a[y])), a[y]);
		case 045: // OR.B Dx,d(Ay)
			return ea = disp(a[y]), write8(or8(d[x], read8(ea)), ea);
		case 046: // OR.B Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(or8(d[x], read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // OR.B Dx,Abs.W
				return ea = fetch16s(), write8(or8(d[x], read8(ea)), ea);
			case 1: // OR.B Dx,Abs.L
				return ea = fetch32(), write8(or8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 052: // OR.W Dx,(Ay)
			return write16(or16(d[x], read16(a[y])), a[y]);
		case 053: // OR.W Dx,(Ay)+
			return write16(or16(d[x], read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // OR.W Dx,-(Ay)
			return a[y] -= 2, write16(or16(d[x], read16(a[y])), a[y]);
		case 055: // OR.W Dx,d(Ay)
			return ea = disp(a[y]), write16(or16(d[x], read16(ea)), ea);
		case 056: // OR.W Dx,d(Ay,Xi)
			return ea = index(a[y]), write16(or16(d[x], read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // OR.W Dx,Abs.W
				return ea = fetch16s(), write16(or16(d[x], read16(ea)), ea);
			case 1: // OR.W Dx,Abs.L
				return ea = fetch32(), write16(or16(d[x], read16(ea)), ea);
			}
			return exception(4);
		case 062: // OR.L Dx,(Ay)
			return write32(or32(d[x], read32(a[y])), a[y]);
		case 063: // OR.L Dx,(Ay)+
			return write32(or32(d[x], read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // OR.L Dx,-(Ay)
			return a[y] -= 4, write32(or32(d[x], read32(a[y])), a[y]);
		case 065: // OR.L Dx,d(Ay)
			return ea = disp(a[y]), write32(or32(d[x], read32(ea)), ea);
		case 066: // OR.L Dx,d(Ay,Xi)
			return ea = index(a[y]), write32(or32(d[x], read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // OR.L Dx,Abs.W
				return ea = fetch16s(), write32(or32(d[x], read32(ea)), ea);
			case 1: // OR.L Dx,Abs.L
				return ea = fetch32(), write32(or32(d[x], read32(ea)), ea);
			}
			return exception(4);
		case 070: // DIVS Dy,Dx
			return void(d[x] = divs(d[y], d[x]));
		case 072: // DIVS (Ay),Dx
			return void(d[x] = divs(read16(a[y]), d[x]));
		case 073: // DIVS (Ay)+,Dx
			return d[x] = divs(read16(a[y]), d[x]), void(a[y] += 2);
		case 074: // DIVS -(Ay),Dx
			return a[y] -= 2, void(d[x] = divs(read16(a[y]), d[x]));
		case 075: // DIVS d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = divs(read16(ea), d[x]));
		case 076: // DIVS d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = divs(read16(ea), d[x]));
		case 077: // DIVS Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = divs(rop16(), d[x]));
		default:
			return exception(4);
		}
	}

	void execute_9() { // SUB/SUBX
		const int x = op >> 9 & 7, y = op & 7;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // SUB.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | sub8(d[y], d[x]));
		case 002: // SUB.B (Ay),Dx
			return void(d[x] = d[x] & ~0xff | sub8(read8(a[y]), d[x]));
		case 003: // SUB.B (Ay)+,Dx
			return d[x] = d[x] & ~0xff | sub8(read8(a[y]), d[x]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // SUB.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, void(d[x] = d[x] & ~0xff | sub8(read8(a[y]), d[x]));
		case 005: // SUB.B d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xff | sub8(read8(ea), d[x]));
		case 006: // SUB.B d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xff | sub8(read8(ea), d[x]));
		case 007: // SUB.B Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xff | sub8(rop8(), d[x]));
		case 010: // SUB.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | sub16(d[y], d[x]));
		case 011: // SUB.W Ay,Dx
			return void(d[x] = d[x] & ~0xffff | sub16(a[y], d[x]));
		case 012: // SUB.W (Ay),Dx
			return void(d[x] = d[x] & ~0xffff | sub16(read16(a[y]), d[x]));
		case 013: // SUB.W (Ay)+,Dx
			return d[x] = d[x] & ~0xffff | sub16(read16(a[y]), d[x]), void(a[y] += 2);
		case 014: // SUB.W -(Ay),Dx
			return a[y] -= 2, void(d[x] = d[x] & ~0xffff | sub16(read16(a[y]), d[x]));
		case 015: // SUB.W d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xffff | sub16(read16(ea), d[x]));
		case 016: // SUB.W d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xffff | sub16(read16(ea), d[x]));
		case 017: // SUB.W Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xffff | sub16(rop16(), d[x]));
		case 020: // SUB.L Dy,Dx
			return void(d[x] = sub32(d[y], d[x]));
		case 021: // SUB.L Ay,Dx
			return void(d[x] = sub32(a[y], d[x]));
		case 022: // SUB.L (Ay),Dx
			return void(d[x] = sub32(read32(a[y]), d[x]));
		case 023: // SUB.L (Ay)+,Dx
			return d[x] = sub32(read32(a[y]), d[x]), void(a[y] += 4);
		case 024: // SUB.L -(Ay),Dx
			return a[y] -= 4, void(d[x] = sub32(read32(a[y]), d[x]));
		case 025: // SUB.L d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = sub32(read32(ea), d[x]));
		case 026: // SUB.L d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = sub32(read32(ea), d[x]));
		case 027: // SUB.L Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = sub32(rop32(), d[x]));
		case 030: // SUBA.W Dy,Ax
			return void(a[x] -= d[y] << 16 >> 16);
		case 031: // SUBA.W Ay,Ax
			return void(a[x] -= a[y] << 16 >> 16);
		case 032: // SUBA.W (Ay),Ax
			return void(a[x] -= read16(a[y]) << 16 >> 16);
		case 033: // SUBA.W (Ay)+,Ax
			return a[x] -= read16(a[y]) << 16 >> 16, void(a[y] += 2);
		case 034: // SUBA.W -(Ay),Ax
			return a[y] -= 2, void(a[x] -= read16(a[y]) << 16 >> 16);
		case 035: // SUBA.W d(Ay),Ax
			return ea = disp(a[y]), void(a[x] -= read16(ea) << 16 >> 16);
		case 036: // SUBA.W d(Ay,Xi),Ax
			return ea = index(a[y]), void(a[x] -= read16(ea) << 16 >> 16);
		case 037: // SUBA.W Abs...,Ax
			return y >= 5 ? exception(4) : void(a[x] -= rop16() << 16 >> 16);
		case 040: // SUBX.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | subx8(d[y], d[x]));
		case 041: // SUBX.B -(Ay),-(Ax)
			return a[y] -= y < 7 ? 1 : 2, a[x] -= x < 7 ? 1 : 2, write8(subx8(read8(a[y]), read8(a[x])), a[x]);
		case 042: // SUB.B Dx,(Ay)
			return write8(sub8(d[x], read8(a[y])), a[y]);
		case 043: // SUB.B Dx,(Ay)+
			return write8(sub8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // SUB.B Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(sub8(d[x], read8(a[y])), a[y]);
		case 045: // SUB.B Dx,d(Ay)
			return ea = disp(a[y]), write8(sub8(d[x], read8(ea)), ea);
		case 046: // SUB.B Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(sub8(d[x], read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // SUB.B Dx,Abs.W
				return ea = fetch16s(), write8(sub8(d[x], read8(ea)), ea);
			case 1: // SUB.B Dx,Abs.L
				return ea = fetch32(), write8(sub8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 050: // SUBX.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | subx16(d[y], d[x]));
		case 051: // SUBX.W -(Ay),-(Ax)
			return a[y] -= 2, a[x] -= 2, write16(subx16(read16(a[y]), read16(a[x])), a[x]);
		case 052: // SUB.W Dx,(Ay)
			return write16(sub16(d[x], read16(a[y])), a[y]);
		case 053: // SUB.W Dx,(Ay)+
			return write16(sub16(d[x], read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // SUB.W Dx,-(Ay)
			return a[y] -= 2, write16(sub16(d[x], read16(a[y])), a[y]);
		case 055: // SUB.W Dx,d(Ay)
			return ea = disp(a[y]), write16(sub16(d[x], read16(ea)), ea);
		case 056: // SUB.W Dx,d(Ay,Xi)
			return ea = index(a[y]), write16(sub16(d[x], read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // SUB.W Dx,Abs.W
				return ea = fetch16s(), write16(sub16(d[x], read16(ea)), ea);
			case 1: // SUB.W Dx,Abs.L
				return ea = fetch32(), write16(sub16(d[x], read16(ea)), ea);
			}
			return exception(4);
		case 060: // SUBX.L Dy,Dx
			return void(d[x] = subx32(d[y], d[x]));
		case 061: // SUBX.L -(Ay),-(Ax)
			return a[y] -= 4, a[x] -= 4, write32(subx32(read32(a[y]), read32(a[x])), a[x]);
		case 062: // SUB.L Dx,(Ay)
			return write32(sub32(d[x], read32(a[y])), a[y]);
		case 063: // SUB.L Dx,(Ay)+
			return write32(sub32(d[x], read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // SUB.L Dx,-(Ay)
			return a[y] -= 4, write32(sub32(d[x], read32(a[y])), a[y]);
		case 065: // SUB.L Dx,d(Ay)
			return ea = disp(a[y]), write32(sub32(d[x], read32(ea)), ea);
		case 066: // SUB.L Dx,d(Ay,Xi)
			return ea = index(a[y]), write32(sub32(d[x], read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // SUB.L Dx,Abs.W
				return ea = fetch16s(), write32(sub32(d[x], read32(ea)), ea);
			case 1: // SUB.L Dx,Abs.L
				return ea = fetch32(), write32(sub32(d[x], read32(ea)), ea);
			}
			return exception(4);
		case 070: // SUBA.L Dy,Ax
			return void(a[x] -= d[y]);
		case 071: // SUBA.L Ay,Ax
			return void(a[x] -= a[y]);
		case 072: // SUBA.L (Ay),Ax
			return void(a[x] -= read32(a[y]));
		case 073: // SUBA.L (Ay)+,Ax
			return a[x] -= read32(a[y]), void(a[y] += 4);
		case 074: // SUBA.L -(Ay),Ax
			return a[y] -= 4, void(a[x] -= read32(a[y]));
		case 075: // SUBA.L d(Ay),Ax
			return ea = disp(a[y]), void(a[x] -= read32(ea));
		case 076: // SUBA.L d(Ay,Xi),Ax
			return ea = index(a[y]), void(a[x] -= read32(ea));
		case 077: // SUBA.L Abs...,Ax
			return y >= 5 ? exception(4) : void(a[x] -= rop32());
		default:
			return exception(4);
		}
	}

	void execute_b() { // CMP/EOR
		const int x = op >> 9 & 7, y = op & 7;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // CMP.B Dy,Dx
			return cmp8(d[y], d[x]);
		case 002: // CMP.B (Ay),Dx
			return cmp8(read8(a[y]), d[x]);
		case 003: // CMP.B (Ay)+,Dx
			return cmp8(read8(a[y]), d[x]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // CMP.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, cmp8(read8(a[y]), d[x]);
		case 005: // CMP.B d(Ay),Dx
			return ea = disp(a[y]), cmp8(read8(ea), d[x]);
		case 006: // CMP.B d(Ay,Xi),Dx
			return ea = index(a[y]), cmp8(read8(ea), d[x]);
		case 007: // CMP.B Abs...,Dx
			return y >= 5 ? exception(4) : cmp8(rop8(), d[x]);
		case 010: // CMP.W Dy,Dx
			return cmp16(d[y], d[x]);
		case 011: // CMP.W Ay,Dx
			return cmp16(a[y], d[x]);
		case 012: // CMP.W (Ay),Dx
			return cmp16(read16(a[y]), d[x]);
		case 013: // CMP.W (Ay)+,Dx
			return cmp16(read16(a[y]), d[x]), void(a[y] += 2);
		case 014: // CMP.W -(Ay),Dx
			return a[y] -= 2, cmp16(read16(a[y]), d[x]);
		case 015: // CMP.W d(Ay),Dx
			return ea = disp(a[y]), cmp16(read16(ea), d[x]);
		case 016: // CMP.W d(Ay,Xi),Dx
			return ea = index(a[y]), cmp16(read16(ea), d[x]);
		case 017: // CMP.W Abs...,Dx
			return y >= 5 ? exception(4) : cmp16(rop16(), d[x]);
		case 020: // CMP.L Dy,Dx
			return cmp32(d[y], d[x]);
		case 021: // CMP.L Ay,Dx
			return cmp32(a[y], d[x]);
		case 022: // CMP.L (Ay),Dx
			return cmp32(read32(a[y]), d[x]);
		case 023: // CMP.L (Ay)+,Dx
			return cmp32(read32(a[y]), d[x]), void(a[y] += 4);
		case 024: // CMP.L -(Ay),Dx
			return a[y] -= 4, cmp32(read32(a[y]), d[x]);
		case 025: // CMP.L d(Ay),Dx
			return ea = disp(a[y]), cmp32(read32(ea), d[x]);
		case 026: // CMP.L d(Ay,Xi),Dx
			return ea = index(a[y]), cmp32(read32(ea), d[x]);
		case 027: // CMP.L Abs...,Dx
			return y >= 5 ? exception(4) : cmp32(rop32(), d[x]);
		case 030: // CMPA.W Dy,Ax
			return cmpa16(d[y], a[x]);
		case 031: // CMPA.W Ay,Ax
			return cmpa16(a[y], a[x]);
		case 032: // CMPA.W (Ay),Ax
			return cmpa16(read16(a[y]), a[x]);
		case 033: // CMPA.W (Ay)+,Ax
			return cmpa16(read16(a[y]), a[x]), void(a[y] += 2);
		case 034: // CMPA.W -(Ay),Ax
			return a[y] -= 2, cmpa16(read16(a[y]), a[x]);
		case 035: // CMPA.W d(Ay),Ax
			return ea = disp(a[y]), cmpa16(read16(ea), a[x]);
		case 036: // CMPA.W d(Ay,Xi),Ax
			return ea = index(a[y]), cmpa16(read16(ea), a[x]);
		case 037: // CMPA.W Abs...,Ax
			return y >= 5 ? exception(4) : cmpa16(rop16(), a[x]);
		case 040: // EOR.B Dx,Dy
			return void(d[y] = d[y] & ~0xff | eor8(d[x], d[y]));
		case 041: // CMPM.B (Ay)+,(Ax)+
			return cmp8(read8(a[y]), read8(a[x])), a[y] += y < 7 ? 1 : 2, void(a[x] += x < 7 ? 1 : 2);
		case 042: // EOR.B Dx,(Ay)
			return write8(eor8(d[x], read8(a[y])), a[y]);
		case 043: // EOR.B Dx,(Ay)+
			return write8(eor8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // EOR.B Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(eor8(d[x], read8(a[y])), a[y]);
		case 045: // EOR.B Dx,d(Ay)
			return ea = disp(a[y]), write8(eor8(d[x], read8(ea)), ea);
		case 046: // EOR.B Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(eor8(d[x], read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // EOR.B Dx,Abs.W
				return ea = fetch16s(), write8(eor8(d[x], read8(ea)), ea);
			case 1: // EOR.B Dx,Abs.L
				return ea = fetch32(), write8(eor8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 050: // EOR.W Dx,Dy
			return void(d[y] = d[y] & ~0xffff | eor16(d[x], d[y]));
		case 051: // CMPM.W (Ay)+,(Ax)+
			return cmp16(read16(a[y]), read16(a[x])), a[y] += 2, void(a[x] += 2);
		case 052: // EOR.W Dx,(Ay)
			return write16(eor16(d[x], read16(a[y])), a[y]);
		case 053: // EOR.W Dx,(Ay)+
			return write16(eor16(d[x], read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // EOR.W Dx,-(Ay)
			return a[y] -= 2, write16(eor16(d[x], read16(a[y])), a[y]);
		case 055: // EOR.W Dx,d(Ay)
			return ea = disp(a[y]), write16(eor16(d[x], read16(ea)), ea);
		case 056: // EOR.W Dx,d(Ay,Xi)
			return ea = index(a[y]), write16(eor16(d[x], read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // EOR.W Dx,Abs.W
				return ea = fetch16s(), write16(eor16(d[x], read16(ea)), ea);
			case 1: // EOR.W Dx,Abs.L
				return ea = fetch32(), write16(eor16(d[x], read16(ea)), ea);
			}
			return exception(4);
		case 060: // EOR.L Dx,Dy
			return void(d[y] = eor32(d[x], d[y]));
		case 061: // CMPM.L (Ay)+,(Ax)+
			return cmp32(read32(a[y]), read32(a[x])), a[y] += 4, void(a[x] += 4);
		case 062: // EOR.L Dx,(Ay)
			return write32(eor32(d[x], read32(a[y])), a[y]);
		case 063: // EOR.L Dx,(Ay)+
			return write32(eor32(d[x], read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // EOR.L Dx,-(Ay)
			return a[y] -= 4, write32(eor32(d[x], read32(a[y])), a[y]);
		case 065: // EOR.L Dx,d(Ay)
			return ea = disp(a[y]), write32(eor32(d[x], read32(ea)), ea);
		case 066: // EOR.L Dx,d(Ay,Xi)
			return ea = index(a[y]), write32(eor32(d[x], read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // EOR.L Dx,Abs.W
				return ea = fetch16s(), write32(eor32(d[x], read32(ea)), ea);
			case 1: // EOR.L Dx,Abs.L
				return ea = fetch32(), write32(eor32(d[x], read32(ea)), ea);
			}
			return exception(4);
		case 070: // CMPA.L Dy,Ax
			return cmpa32(d[y], a[x]);
		case 071: // CMPA.L Ay,Ax
			return cmpa32(a[y], a[x]);
		case 072: // CMPA.L (Ay),Ax
			return cmpa32(read32(a[y]), a[x]);
		case 073: // CMPA.L (Ay)+,Ax
			return cmpa32(read32(a[y]), a[x]), void(a[y] += 4);
		case 074: // CMPA.L -(Ay),Ax
			return a[y] -= 4, cmpa32(read32(a[y]), a[x]);
		case 075: // CMPA.L d(Ay),Ax
			return ea = disp(a[y]), cmpa32(read32(ea), a[x]);
		case 076: // CMPA.L d(Ay,Xi),Ax
			return ea = index(a[y]), cmpa32(read32(ea), a[x]);
		case 077: // CMPA.L Abs...,Ax
			return y >= 5 ? exception(4) : cmpa32(rop32(), a[x]);
		default:
			return exception(4);
		}
	}

	void execute_c() { // AND/MUL/ABCD/EXG
		const int x = op >> 9 & 7, y = op & 7;
		int ea, src;
		switch (op >> 3 & 077) {
		case 000: // AND.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | and8(d[y], d[x]));
		case 002: // AND.B (Ay),Dx
			return void(d[x] = d[x] & ~0xff | and8(read8(a[y]), d[x]));
		case 003: // AND.B (Ay)+,Dx
			return d[x] = d[x] & ~0xff | and8(read8(a[y]), d[x]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // AND.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, void(d[x] = d[x] & ~0xff | and8(read8(a[y]), d[x]));
		case 005: // AND.B d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xff | and8(read8(ea), d[x]));
		case 006: // AND.B d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xff | and8(read8(ea), d[x]));
		case 007: // AND.B Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xff | and8(rop8(), d[x]));
		case 010: // AND.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | and16(d[y], d[x]));
		case 012: // AND.W (Ay),Dx
			return void(d[x] = d[x] & ~0xffff | and16(read16(a[y]), d[x]));
		case 013: // AND.W (Ay)+,Dx
			return d[x] = d[x] & ~0xffff | and16(read16(a[y]), d[x]), void(a[y] += 2);
		case 014: // AND.W -(Ay),Dx
			return a[y] -= 2, void(d[x] = d[x] & ~0xffff | and16(read16(a[y]), d[x]));
		case 015: // AND.W d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xffff | and16(read16(ea), d[x]));
		case 016: // AND.W d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xffff | and16(read16(ea), d[x]));
		case 017: // AND.W Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xffff | and16(rop16(), d[x]));
		case 020: // AND.L Dy,Dx
			return void(d[x] = and32(d[y], d[x]));
		case 022: // AND.L (Ay),Dx
			return void(d[x] = and32(read32(a[y]), d[x]));
		case 023: // AND.L (Ay)+,Dx
			return d[x] = and32(read32(a[y]), d[x]), void(a[y] += 4);
		case 024: // AND.L -(Ay),Dx
			return a[y] -= 4, void(d[x] = and32(read32(a[y]), d[x]));
		case 025: // AND.L d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = and32(read32(ea), d[x]));
		case 026: // AND.L d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = and32(read32(ea), d[x]));
		case 027: // AND.L Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = and32(rop32(), d[x]));
		case 030: // MULU Dy,Dx
			return void(d[x] = mulu(d[y], d[x]));
		case 032: // MULU (Ay),Dx
			return void(d[x] = mulu(read16(a[y]), d[x]));
		case 033: // MULU (Ay)+,Dx
			return d[x] = mulu(read16(a[y]), d[x]), void(a[y] += 2);
		case 034: // MULU -(Ay),Dx
			return a[y] -= 2, void(d[x] = mulu(read16(a[y]), d[x]));
		case 035: // MULU d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = mulu(read16(ea), d[x]));
		case 036: // MULU d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = mulu(read16(ea), d[x]));
		case 037: // MULU Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = mulu(rop16(), d[x]));
		case 040: // ABCD Dy,Dx
			return void(d[x] = d[x] & ~0xff | abcd(d[y], d[x]));
		case 041: // ABCD -(Ay),-(Ax)
			return a[y] -= y < 7 ? 1 : 2, a[x] -= x < 7 ? 1 : 2, write8(abcd(read8(a[y]), read8(a[x])), a[x]);
		case 042: // AND.B Dx,(Ay)
			return write8(and8(d[x], read8(a[y])), a[y]);
		case 043: // AND.B Dx,(Ay)+
			return write8(and8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // AND.B Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(and8(d[x], read8(a[y])), a[y]);
		case 045: // AND.B Dx,d(Ay)
			return ea = disp(a[y]), write8(and8(d[x], read8(ea)), ea);
		case 046: // AND.B Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(and8(d[x], read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // AND.B Dx,Abs.W
				return ea = fetch16s(), write8(and8(d[x], read8(ea)), ea);
			case 1: // AND.B Dx,Abs.L
				return ea = fetch32(), write8(and8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 050: // EXG Dx,Dy
			return src = d[x], d[x] = d[y], void(d[y] = src);
		case 051: // EXG Ax,Ay
			return src = a[x], a[x] = a[y], void(a[y] = src);
		case 052: // AND.W Dx,(Ay)
			return write16(and16(d[x], read16(a[y])), a[y]);
		case 053: // AND.W Dx,(Ay)+
			return write16(and16(d[x], read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // AND.W Dx,-(Ay)
			return a[y] -= 2, write16(and16(d[x], read16(a[y])), a[y]);
		case 055: // AND.W Dx,d(Ay)
			return ea = disp(a[y]), write16(and16(d[x], read16(ea)), ea);
		case 056: // AND.W Dx,d(Ay,Xi)
			return ea = index(a[y]), write16(and16(d[x], read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // AND.W Dx,Abs.W
				return ea = fetch16s(), write16(and16(d[x], read16(ea)), ea);
			case 1: // AND.W Dx,Abs.L
				return ea = fetch32(), write16(and16(d[x], read16(ea)), ea);
			}
			return exception(4);
		case 061: // EXG Dx,Ay
			return src = d[x], d[x] = a[y], void(a[y] = src);
		case 062: // AND.L Dx,(Ay)
			return write32(and32(d[x], read32(a[y])), a[y]);
		case 063: // AND.L Dx,(Ay)+
			return write32(and32(d[x], read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // AND.L Dx,-(Ay)
			return a[y] -= 4, write32(and32(d[x], read32(a[y])), a[y]);
		case 065: // AND.L Dx,d(Ay)
			return ea = disp(a[y]), write32(and32(d[x], read32(ea)), ea);
		case 066: // AND.L Dx,d(Ay,Xi)
			return ea = index(a[y]), write32(and32(d[x], read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // AND.L Dx,Abs.W
				return ea = fetch16s(), write32(and32(d[x], read32(ea)), ea);
			case 1: // AND.L Dx,Abs.L
				return ea = fetch32(), write32(and32(d[x], read32(ea)), ea);
			}
			return exception(4);
		case 070: // MULS Dy,Dx
			return void(d[x] = muls(d[y], d[x]));
		case 072: // MULS (Ay),Dx
			return void(d[x] = muls(read16(a[y]), d[x]));
		case 073: // MULS (Ay)+,Dx
			return d[x] = muls(read16(a[y]), d[x]), void(a[y] += 2);
		case 074: // MULS -(Ay),Dx
			return a[y] -= 2, void(d[x] = muls(read16(a[y]), d[x]));
		case 075: // MULS d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = muls(read16(ea), d[x]));
		case 076: // MULS d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = muls(read16(ea), d[x]));
		case 077: // MULS Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = muls(rop16(), d[x]));
		default:
			return exception(4);
		}
	}

	void execute_d() { // ADD/ADDX
		const int x = op >> 9 & 7, y = op & 7;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // ADD.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | add8(d[y], d[x]));
		case 002: // ADD.B (Ay),Dx
			return void(d[x] = d[x] & ~0xff | add8(read8(a[y]), d[x]));
		case 003: // ADD.B (Ay)+,Dx
			return d[x] = d[x] & ~0xff | add8(read8(a[y]), d[x]), void(a[y] += y < 7 ? 1 : 2);
		case 004: // ADD.B -(Ay),Dx
			return a[y] -= y < 7 ? 1 : 2, void(d[x] = d[x] & ~0xff | add8(read8(a[y]), d[x]));
		case 005: // ADD.B d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xff | add8(read8(ea), d[x]));
		case 006: // ADD.B d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xff | add8(read8(ea), d[x]));
		case 007: // ADD.B Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xff | add8(rop8(), d[x]));
		case 010: // ADD.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | add16(d[y], d[x]));
		case 011: // ADD.W Ay,Dx
			return void(d[x] = d[x] & ~0xffff | add16(a[y], d[x]));
		case 012: // ADD.W (Ay),Dx
			return void(d[x] = d[x] & ~0xffff | add16(read16(a[y]), d[x]));
		case 013: // ADD.W (Ay)+,Dx
			return d[x] = d[x] & ~0xffff | add16(read16(a[y]), d[x]), void(a[y] += 2);
		case 014: // ADD.W -(Ay),Dx
			return a[y] -= 2, void(d[x] = d[x] & ~0xffff | add16(read16(a[y]), d[x]));
		case 015: // ADD.W d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = d[x] & ~0xffff | add16(read16(ea), d[x]));
		case 016: // ADD.W d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = d[x] & ~0xffff | add16(read16(ea), d[x]));
		case 017: // ADD.W Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = d[x] & ~0xffff | add16(rop16(), d[x]));
		case 020: // ADD.L Dy,Dx
			return void(d[x] = add32(d[y], d[x]));
		case 021: // ADD.L Ay,Dx
			return void(d[x] = add32(a[y], d[x]));
		case 022: // ADD.L (Ay),Dx
			return void(d[x] = add32(read32(a[y]), d[x]));
		case 023: // ADD.L (Ay)+,Dx
			return d[x] = add32(read32(a[y]), d[x]), void(a[y] += 4);
		case 024: // ADD.L -(Ay),Dx
			return a[y] -= 4, void(d[x] = add32(read32(a[y]), d[x]));
		case 025: // ADD.L d(Ay),Dx
			return ea = disp(a[y]), void(d[x] = add32(read32(ea), d[x]));
		case 026: // ADD.L d(Ay,Xi),Dx
			return ea = index(a[y]), void(d[x] = add32(read32(ea), d[x]));
		case 027: // ADD.L Abs...,Dx
			return y >= 5 ? exception(4) : void(d[x] = add32(rop32(), d[x]));
		case 030: // ADDA.W Dy,Ax
			return void(a[x] += d[y] << 16 >> 16);
		case 031: // ADDA.W Ay,Ax
			return void(a[x] += a[y] << 16 >> 16);
		case 032: // ADDA.W (Ay),Ax
			return void(a[x] += read16(a[y]) << 16 >> 16);
		case 033: // ADDA.W (Ay)+,Ax
			return a[x] += read16(a[y]) << 16 >> 16, void(a[y] += 2);
		case 034: // ADDA.W -(Ay),Ax
			return a[y] -= 2, void(a[x] += read16(a[y]) << 16 >> 16);
		case 035: // ADDA.W d(Ay),Ax
			return ea = disp(a[y]), void(a[x] += read16(ea) << 16 >> 16);
		case 036: // ADDA.W d(Ay,Xi),Ax
			return ea = index(a[y]), void(a[x] += read16(ea) << 16 >> 16);
		case 037: // ADDA.W Abs...,Ax
			return y >= 5 ? exception(4) : void(a[x] += rop16() << 16 >> 16);
		case 040: // ADDX.B Dy,Dx
			return void(d[x] = d[x] & ~0xff | addx8(d[y], d[x]));
		case 041: // ADDX.B -(Ay),-(Ax)
			return a[y] -= y < 7 ? 1 : 2, a[x] -= x < 7 ? 1 : 2, write8(addx8(read8(a[y]), read8(a[x])), a[x]);
		case 042: // ADD.B Dx,(Ay)
			return write8(add8(d[x], read8(a[y])), a[y]);
		case 043: // ADD.B Dx,(Ay)+
			return write8(add8(d[x], read8(a[y])), a[y]), void(a[y] += y < 7 ? 1 : 2);
		case 044: // ADD.B Dx,-(Ay)
			return a[y] -= y < 7 ? 1 : 2, write8(add8(d[x], read8(a[y])), a[y]);
		case 045: // ADD.B Dx,d(Ay)
			return ea = disp(a[y]), write8(add8(d[x], read8(ea)), ea);
		case 046: // ADD.B Dx,d(Ay,Xi)
			return ea = index(a[y]), write8(add8(d[x], read8(ea)), ea);
		case 047:
			switch (y) {
			case 0: // ADD.B Dx,Abs.W
				return ea = fetch16s(), write8(add8(d[x], read8(ea)), ea);
			case 1: // ADD.B Dx,Abs.L
				return ea = fetch32(), write8(add8(d[x], read8(ea)), ea);
			}
			return exception(4);
		case 050: // ADDX.W Dy,Dx
			return void(d[x] = d[x] & ~0xffff | addx16(d[y], d[x]));
		case 051: // ADDX.W -(Ay),-(Ax)
			return a[y] -= 2, a[x] -= 2, write16(addx16(read16(a[y]), read16(a[x])), a[x]);
		case 052: // ADD.W Dx,(Ay)
			return write16(add16(d[x], read16(a[y])), a[y]);
		case 053: // ADD.W Dx,(Ay)+
			return write16(add16(d[x], read16(a[y])), a[y]), void(a[y] += 2);
		case 054: // ADD.W Dx,-(Ay)
			return a[y] -= 2, write16(add16(d[x], read16(a[y])), a[y]);
		case 055: // ADD.W Dx,d(Ay)
			return ea = disp(a[y]), write16(add16(d[x], read16(ea)), ea);
		case 056: // ADD.W Dx,d(Ay,Xi)
			return ea = index(a[y]), write16(add16(d[x], read16(ea)), ea);
		case 057:
			switch (y) {
			case 0: // ADD.W Dx,Abs.W
				return ea = fetch16s(), write16(add16(d[x], read16(ea)), ea);
			case 1: // ADD.W Dx,Abs.L
				return ea = fetch32(), write16(add16(d[x], read16(ea)), ea);
			}
			return exception(4);
		case 060: // ADDX.L Dy,Dx
			return void(d[x] = addx32(d[y], d[x]));
		case 061: // ADDX.L -(Ay),-(Ax)
			return a[y] -= 4, a[x] -= 4, write32(addx32(read32(a[y]), read32(a[x])), a[x]);
		case 062: // ADD.L Dx,(Ay)
			return write32(add32(d[x], read32(a[y])), a[y]);
		case 063: // ADD.L Dx,(Ay)+
			return write32(add32(d[x], read32(a[y])), a[y]), void(a[y] += 4);
		case 064: // ADD.L Dx,-(Ay)
			return a[y] -= 4, write32(add32(d[x], read32(a[y])), a[y]);
		case 065: // ADD.L Dx,d(Ay)
			return ea = disp(a[y]), write32(add32(d[x], read32(ea)), ea);
		case 066: // ADD.L Dx,d(Ay,Xi)
			return ea = index(a[y]), write32(add32(d[x], read32(ea)), ea);
		case 067:
			switch (y) {
			case 0: // ADD.L Dx,Abs.W
				return ea = fetch16s(), write32(add32(d[x], read32(ea)), ea);
			case 1: // ADD.L Dx,Abs.L
				return ea = fetch32(), write32(add32(d[x], read32(ea)), ea);
			}
			return exception(4);
		case 070: // ADDA.L Dy,Ax
			return void(a[x] += d[y]);
		case 071: // ADDA.L Ay,Ax
			return void(a[x] += a[y]);
		case 072: // ADDA.L (Ay),Ax
			return void(a[x] += read32(a[y]));
		case 073: // ADDA.L (Ay)+,Ax
			return a[x] += read32(a[y]), void(a[y] += 4);
		case 074: // ADDA.L -(Ay),Ax
			return a[y] -= 4, void(a[x] += read32(a[y]));
		case 075: // ADDA.L d(Ay),Ax
			return ea = disp(a[y]), void(a[x] += read32(ea));
		case 076: // ADDA.L d(Ay,Xi),Ax
			return ea = index(a[y]), void(a[x] += read32(ea));
		case 077: // ADDA.L Abs...,Ax
			return y >= 5 ? exception(4) : void(a[x] += rop32());
		default:
			return exception(4);
		}
	}

	void execute_e() { // Shift/Rotate
		const int x = op >> 9 & 7, y = op & 7, data = (x - 1 & 7) + 1;
		int ea;
		switch (op >> 3 & 077) {
		case 000: // ASR.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | asr8(data, d[y]));
		case 001: // LSR.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | lsr8(data, d[y]));
		case 002: // ROXR.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | roxr8(data, d[y]));
		case 003: // ROR.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | ror8(data, d[y]));
		case 004: // ASR.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | asr8(d[x], d[y]));
		case 005: // LSR.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | lsr8(d[x], d[y]));
		case 006: // ROXR.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | roxr8(d[x], d[y]));
		case 007: // ROR.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | ror8(d[x], d[y]));
		case 010: // ASR.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | asr16(data, d[y]));
		case 011: // LSR.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | lsr16(data, d[y]));
		case 012: // ROXR.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | roxr16(data, d[y]));
		case 013: // ROR.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | ror16(data, d[y]));
		case 014: // ASR.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | asr16(d[x], d[y]));
		case 015: // LSR.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | lsr16(d[x], d[y]));
		case 016: // ROXR.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | roxr16(d[x], d[y]));
		case 017: // ROR.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | ror16(d[x], d[y]));
		case 020: // ASR.L #<data>,Dy
			return void(d[y] = asr32(data, d[y]));
		case 021: // LSR.L #<data>,Dy
			return void(d[y] = lsr32(data, d[y]));
		case 022: // ROXR.L #<data>,Dy
			return void(d[y] = roxr32(data, d[y]));
		case 023: // ROR.L #<data>,Dy
			return void(d[y] = ror32(data, d[y]));
		case 024: // ASR.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = asr32(d[x], d[y]));
		case 025: // LSR.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = lsr32(d[x], d[y]));
		case 026: // ROXR.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = roxr32(d[x], d[y]));
		case 027: // ROR.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = ror32(d[x], d[y]));
		case 032:
			switch (x) {
			case 0: // ASR.W (Ay)
				return write16(asr16(1, read16(a[y])), a[y]);
			case 1: // LSR.W (Ay)
				return write16(lsr16(1, read16(a[y])), a[y]);
			case 2: // ROXR.W (Ay)
				return write16(roxr16(1, read16(a[y])), a[y]);
			case 3: // ROR.W (Ay)
				return write16(ror16(1, read16(a[y])), a[y]);
			}
			return exception(4);
		case 033:
			switch (x) {
			case 0: // ASR.W (Ay)+
				return write16(asr16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 1: // LSR.W (Ay)+
				return write16(lsr16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 2: // ROXR.W (Ay)+
				return write16(roxr16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 3: // ROR.W (Ay)+
				return write16(ror16(1, read16(a[y])), a[y]), void(a[y] += 2);
			}
			return exception(4);
		case 034:
			switch (x) {
			case 0: // ASR.W -(Ay)
				return a[y] -= 2, write16(asr16(1, read16(a[y])), a[y]);
			case 1: // LSR.W -(Ay)
				return a[y] -= 2, write16(lsr16(1, read16(a[y])), a[y]);
			case 2: // ROXR.W -(Ay)
				return a[y] -= 2, write16(roxr16(1, read16(a[y])), a[y]);
			case 3: // ROR.W -(Ay)
				return a[y] -= 2, write16(ror16(1, read16(a[y])), a[y]);
			}
			return exception(4);
		case 035:
			switch (x) {
			case 0: // ASR.W d(Ay)
				return ea = disp(a[y]), write16(asr16(1, read16(ea)), ea);
			case 1: // LSR.W d(Ay)
				return ea = disp(a[y]), write16(lsr16(1, read16(ea)), ea);
			case 2: // ROXR.W d(Ay)
				return ea = disp(a[y]), write16(roxr16(1, read16(ea)), ea);
			case 3: // ROR.W d(Ay)
				return ea = disp(a[y]), write16(ror16(1, read16(ea)), ea);
			}
			return exception(4);
		case 036:
			switch (x) {
			case 0: // ASR.W d(Ay,Xi)
				return ea = index(a[y]), write16(asr16(1, read16(ea)), ea);
			case 1: // LSR.W d(Ay,Xi)
				return ea = index(a[y]), write16(lsr16(1, read16(ea)), ea);
			case 2: // ROXR.W d(Ay,Xi)
				return ea = index(a[y]), write16(roxr16(1, read16(ea)), ea);
			case 3: // ROR.W d(Ay,Xi)
				return ea = index(a[y]), write16(ror16(1, read16(ea)), ea);
			}
			return exception(4);
		case 037:
			switch (x << 3 | y) {
			case 000: // ASR.W Abs.W
				return ea = fetch16s(), write16(asr16(1, read16(ea)), ea);
			case 001: // ASR.W Abs.L
				return ea = fetch32(), write16(asr16(1, read16(ea)), ea);
			case 010: // LSR.W Abs.W
				return ea = fetch16s(), write16(lsr16(1, read16(ea)), ea);
			case 011: // LSR.W Abs.L
				return ea = fetch32(), write16(lsr16(1, read16(ea)), ea);
			case 020: // ROXR.W Abs.W
				return ea = fetch16s(), write16(roxr16(1, read16(ea)), ea);
			case 021: // ROXR.W Abs.L
				return ea = fetch32(), write16(roxr16(1, read16(ea)), ea);
			case 030: // ROR.W Abs.W
				return ea = fetch16s(), write16(ror16(1, read16(ea)), ea);
			case 031: // ROR.W Abs.L
				return ea = fetch32(), write16(ror16(1, read16(ea)), ea);
			}
			return exception(4);
		case 040: // ASL.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | asl8(data, d[y]));
		case 041: // LSL.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | lsl8(data, d[y]));
		case 042: // ROXL.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | roxl8(data, d[y]));
		case 043: // ROL.B #<data>,Dy
			return void(d[y] = d[y] & ~0xff | rol8(data, d[y]));
		case 044: // ASL.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | asl8(d[x], d[y]));
		case 045: // LSL.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | lsl8(d[x], d[y]));
		case 046: // ROXL.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | roxl8(d[x], d[y]));
		case 047: // ROL.B Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xff | rol8(d[x], d[y]));
		case 050: // ASL.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | asl16(data, d[y]));
		case 051: // LSL.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | lsl16(data, d[y]));
		case 052: // ROXL.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | roxl16(data, d[y]));
		case 053: // ROL.W #<data>,Dy
			return void(d[y] = d[y] & ~0xffff | rol16(data, d[y]));
		case 054: // ASL.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | asl16(d[x], d[y]));
		case 055: // LSL.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | lsl16(d[x], d[y]));
		case 056: // ROXL.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | roxl16(d[x], d[y]));
		case 057: // ROL.W Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = d[y] & ~0xffff | rol16(d[x], d[y]));
		case 060: // ASL.L #<data>,Dy
			return void(d[y] = asl32(data, d[y]));
		case 061: // LSL.L #<data>,Dy
			return void(d[y] = lsl32(data, d[y]));
		case 062: // ROXL.L #<data>,Dy
			return void(d[y] = roxl32(data, d[y]));
		case 063: // ROL.L #<data>,Dy
			return void(d[y] = rol32(data, d[y]));
		case 064: // ASL.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = asl32(d[x], d[y]));
		case 065: // LSL.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = lsl32(d[x], d[y]));
		case 066: // ROXL.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = roxl32(d[x], d[y]));
		case 067: // ROL.L Dx,Dy
			return cycle -= d[x] << 1 & 0x7e, void(d[y] = rol32(d[x], d[y]));
		case 072:
			switch (x) {
			case 0: // ASL.W (Ay)
				return write16(asl16(1, read16(a[y])), a[y]);
			case 1: // LSL.W (Ay)
				return write16(lsl16(1, read16(a[y])), a[y]);
			case 2: // ROXL.W (Ay)
				return write16(roxl16(1, read16(a[y])), a[y]);
			case 3: // ROL.W (Ay)
				return write16(rol16(1, read16(a[y])), a[y]);
			}
			return exception(4);
		case 073:
			switch (x) {
			case 0: // ASL.W (Ay)+
				return write16(asl16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 1: // LSL.W (Ay)+
				return write16(lsl16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 2: // ROXL.W (Ay)+
				return write16(roxl16(1, read16(a[y])), a[y]), void(a[y] += 2);
			case 3: // ROL.W (Ay)+
				return write16(rol16(1, read16(a[y])), a[y]), void(a[y] += 2);
			}
			return exception(4);
		case 074:
			switch (x) {
			case 0: // ASL.W -(Ay)
				return a[y] -= 2, write16(asl16(1, read16(a[y])), a[y]);
			case 1: // LSL.W -(Ay)
				return a[y] -= 2, write16(lsl16(1, read16(a[y])), a[y]);
			case 2: // ROXL.W -(Ay)
				return a[y] -= 2, write16(roxl16(1, read16(a[y])), a[y]);
			case 3: // ROL.W -(Ay)
				return a[y] -= 2, write16(rol16(1, read16(a[y])), a[y]);
			}
			return exception(4);
		case 075:
			switch (x) {
			case 0: // ASL.W d(Ay)
				return ea = disp(a[y]), write16(asl16(1, read16(ea)), ea);
			case 1: // LSL.W d(Ay)
				return ea = disp(a[y]), write16(lsl16(1, read16(ea)), ea);
			case 2: // ROXL.W d(Ay)
				return ea = disp(a[y]), write16(roxl16(1, read16(ea)), ea);
			case 3: // ROL.W d(Ay)
				return ea = disp(a[y]), write16(rol16(1, read16(ea)), ea);
			}
			return exception(4);
		case 076:
			switch (x) {
			case 0: // ASL.W d(Ay,Xi)
				return ea = index(a[y]), write16(asl16(1, read16(ea)), ea);
			case 1: // LSL.W d(Ay,Xi)
				return ea = index(a[y]), write16(lsl16(1, read16(ea)), ea);
			case 2: // ROXL.W d(Ay,Xi)
				return ea = index(a[y]), write16(roxl16(1, read16(ea)), ea);
			case 3: // ROL.W d(Ay,Xi)
				return ea = index(a[y]), write16(rol16(1, read16(ea)), ea);
			}
			return exception(4);
		case 077:
			switch (x << 3 | y) {
			case 000: // ASL.W Abs.W
				return ea = fetch16s(), write16(asl16(1, read16(ea)), ea);
			case 001: // ASL.W Abs.L
				return ea = fetch32(), write16(asl16(1, read16(ea)), ea);
			case 010: // LSL.W Abs.W
				return ea = fetch16s(), write16(lsl16(1, read16(ea)), ea);
			case 011: // LSL.W Abs.L
				return ea = fetch32(), write16(lsl16(1, read16(ea)), ea);
			case 020: // ROXL.W Abs.W
				return ea = fetch16s(), write16(roxl16(1, read16(ea)), ea);
			case 021: // ROXL.W Abs.L
				return ea = fetch32(), write16(roxl16(1, read16(ea)), ea);
			case 030: // ROL.W Abs.W
				return ea = fetch16s(), write16(rol16(1, read16(ea)), ea);
			case 031: // ROL.W Abs.L
				return ea = fetch32(), write16(rol16(1, read16(ea)), ea);
			}
			return exception(4);
		default:
			return exception(4);
		}
	}

	int rop8() {
		switch (op & 7) {
		case 0: // B Abs.W
			return read8(fetch16s());
		case 1: // B Abs.L
			return read8(fetch32());
		case 2: // B d(PC)
			return txread8(disp(pc));
		case 3: // B d(PC,Xi)
			return txread8(index(pc));
		case 4: // B #<data>
			return fetch16() & 0xff;
		}
		return -1;
	}

	int rop16() {
		switch (op & 7) {
		case 0: // W Abs.W
			return read16(fetch16s());
		case 1: // W Abs.L
			return read16(fetch32());
		case 2: // W d(PC)
			return txread16(disp(pc));
		case 3: // W d(PC,Xi)
			return txread16(index(pc));
		case 4: // W #<data>
			return fetch16();
		}
		return -1;
	}

	int rop32() {
		switch (op & 7) {
		case 0: // L Abs.W
			return read32(fetch16s());
		case 1: // L Abs.L
			return read32(fetch32());
		case 2: // L d(PC)
			return txread32(disp(pc));
		case 3: // L d(PC,Xi)
			return txread32(index(pc));
		case 4: // L #<data>
			return fetch32();
		}
		return -1;
	}

	void movem16rm() {
		const int n = op & 7, list = fetch16();
		int ea = lea();
		if ((op & 070) == 040) {
			for (int i = 0; i < 8; i++)
				list & 1 << i ? (cycle -= 4, ea = ea - 2, write16(a[7 - i], ea)) : void(0);
			for (int i = 0; i < 8; i++)
				list & 0x100 << i ? (cycle -= 4, ea = ea - 2, write16(d[7 - i], ea)) : void(0);
			a[n] = ea;
		} else {
			for (int i = 0; i < 8; i++)
				list & 1 << i && (cycle -= 4, write16(d[i], ea), ea = ea + 2);
			for (int i = 0; i < 8; i++)
				list & 0x100 << i && (cycle -= 4, write16(a[i], ea), ea = ea + 2);
		}
	}

	void movem32rm() {
		const int n = op & 7, list = fetch16();
		int ea = lea();
		if ((op & 070) == 040) {
			for (int i = 0; i < 8; i++)
				list & 1 << i ? (cycle -= 8, ea = ea - 4, write32(a[7 - i], ea)) : void(0);
			for (int i = 0; i < 8; i++)
				list & 0x100 << i ? (cycle -= 8, ea = ea - 4, write32(d[7 - i], ea)) : void(0);
			a[n] = ea;
		} else {
			for (int i = 0; i < 8; i++)
				list & 1 << i && (cycle -= 8, write32(d[i], ea), ea = ea + 4);
			for (int i = 0; i < 8; i++)
				list & 0x100 << i && (cycle -= 8, write32(a[i], ea), ea = ea + 4);
		}
	}

	void movem16mr() {
		const int n = op & 7, list = fetch16();
		int ea = lea();
		for (int i = 0; i < 8; i++)
			list & 1 << i && (cycle -= 4, d[i] = read16s(ea), ea = ea + 2);
		for (int i = 0; i < 8; i++)
			list & 0x100 << i && (cycle -= 4, a[i] = read16s(ea), ea = ea + 2);
		(op & 070) == 030 && (a[n] = ea);
	}

	void movem32mr() {
		const int n = op & 7, list = fetch16();
		int ea = lea();
		for (int i = 0; i < 8; i++)
			list & 1 << i && (cycle -= 8, d[i] = read32(ea), ea = ea + 4);
		for (int i = 0; i < 8; i++)
			list & 0x100 << i && (cycle -= 8, a[i] = read32(ea), ea = ea + 4);
		(op & 070) == 030 && (a[n] = ea);
	}

	virtual void rte() {
		sr = read16(a[7]), a[7] += 2, pc = read32(a[7]), a[7] += 4, ~sr & 0x2000 && (ssp = a[7], a[7] = usp);
	}

	void chk(int src, int dst) {
		dst = dst << 16 >> 16, src = src << 16 >> 16, dst >= 0 && dst <= src ? void(0) : (sr = sr & ~8 | dst >> 12 & 8, exception(6));
	}

	int lea() {
		const int n = op & 7;
		switch(op >> 3 & 7) {
		case 2: // (An)
		case 3: // (An)+
		case 4: // -(An)
			return a[n];
		case 5: // d(An)
			return disp(a[n]);
		case 6: // d(An,Xi)
			return index(a[n]);
		case 7:
			switch (n) {
			case 0: // Abs.W
				return fetch16s();
			case 1: // Abs.L
				return fetch32();
			case 2: // d(PC)
				return disp(pc);
			case 3: // d(PC,Xi)
				return index(pc);
			}
		}
		return -1;
	}

	int or8(int src, int dst) {
		const int r = (dst | src) & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	int or16(int src, int dst) {
		const int r = (dst | src) & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	int or32(int src, int dst) {
		const int r = dst | src;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	int and8(int src, int dst) {
		const int r = dst & src & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	int and16(int src, int dst) {
		const int r = dst & src & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	int and32(int src, int dst) {
		const int r = dst & src;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	int sub8(int src, int dst) {
		const int r = dst - src & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int sub16(int src, int dst) {
		const int r = dst - src & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int sub32(int src, int dst) {
		const int r = dst - src, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	int add8(int src, int dst) {
		const int r = dst + src & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int add16(int src, int dst) {
		const int r = dst + src & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int add32(int src, int dst) {
		const int r = dst + src, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	void btst8(int src, int dst) {
		sr = sr & ~4 | ~dst >> (src & 7) << 2 & 4;
	}

	void btst32(int src, int dst) {
		sr = sr & ~4 | ~dst >> (src & 31) << 2 & 4;
	}

	int bchg8(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, (dst ^ 1 << src) & 0xff;
	}

	int bchg32(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst ^ 1 << src;
	}

	int bclr8(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, dst & ~(1 << src) & 0xff;
	}

	int bclr32(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst & ~(1 << src);
	}

	int bset8(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, (dst | 1 << src) & 0xff;
	}

	int bset32(int src, int dst) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst | 1 << src;
	}

	int eor8(int src, int dst) {
		const int r = (dst ^ src) & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	int eor16(int src, int dst) {
		const int r = (dst ^ src) & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	int eor32(int src, int dst) {
		const int r = dst ^ src;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	void cmp8(int src, int dst) {
		const int r = dst - src & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1;
	}

	void cmp16(int src, int dst) {
		const int r = dst - src & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1;
	}

	virtual void cmp32(int src, int dst) {
		const int r = dst - src, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1;
	}

	int divu(int src, int dst) {
		if (!(src &= 0xffff))
			return exception(5), dst;
		const int r = (unsigned int)dst / src;
		return r > 0xffff || r < 0 ? (sr |= 2, dst) : (sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r | (unsigned int)dst % src << 16);
	}

	int sbcd(int src, int dst) {
		const int h = (dst & 0x0f) - (src & 0x0f) - (sr >> 4 & 1) < 0, c = (dst >> 4 & 0x0f) - (src >> 4 & 0x0f) - h < 0;
		const int r = dst - src - (sr >> 4 & 1) - h * 6 - c * 0x60 & 0xff;
		return sr = sr & ~0x15 | c << 4 | sr & !r << 2 | c, r;
	}

	int divs(int src, int dst) {
		if (!(src = src << 16 >> 16))
			return exception(5), dst;
		const int r = dst / src;
		return r > 0x7fff || r < -0x8000 ? (sr |= 2, dst) : (sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r & 0xffff | dst % src << 16);
	}

	int subx8(int src, int dst) {
		const int r = dst - src - (sr >> 4 & 1) & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int subx16(int src, int dst) {
		const int r = dst - src - (sr >> 4 & 1) & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int subx32(int src, int dst) {
		const int r = dst - src - (sr >> 4 & 1), v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | sr & !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	void cmpa16(int src, int dst) {
		const int r = dst - (src = src << 16 >> 16), v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1;
	}

	void cmpa32(int src, int dst) {
		const int r = dst - src, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1;
	}

	int mulu(int src, int dst) {
		const int r = (src & 0xffff) * (dst & 0xffff);
		return cycle -= __builtin_popcount(src & 0xffff) << 1, sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	int abcd(int src, int dst) {
		const int h = (dst & 0x0f) + (src & 0x0f) + (sr >> 4 & 1) > 9, c = (dst >> 4 & 0x0f) + (src >> 4 & 0x0f) + h > 9;
		const int r = dst + src + (sr >> 4 & 1) + h * 6 + c * 0x60 & 0xff;
		return sr = sr & ~0x15 | c << 4 | sr & !r << 2 | c, r;
	}

	int muls(int src, int dst) {
		const int r = (dst << 16 >> 16) * (src << 16 >> 16);
		return cycle -= __builtin_popcount((src ^ src << 1) & 0xffff) << 1, sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	int addx8(int src, int dst) {
		const int r = dst + src + (sr >> 4 & 1) & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int addx16(int src, int dst) {
		const int r = dst + src + (sr >> 4 & 1) & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int addx32(int src, int dst) {
		const int r = dst + src + (sr >> 4 & 1), v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | sr & !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	int asr8(int src, int dst) {
		src &= 63, dst = dst << 24 >> 24;
		const int r = dst >> min(src, 7) & 0xff, c = (src > 0) & dst >> min(src - 1, 7), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	int asr16(int src, int dst) {
		src &= 63, dst = dst << 16 >> 16;
		const int r = dst >> min(src, 15) & 0xffff, c = (src > 0) & dst >> min(src - 1, 15), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	int asr32(int src, int dst) {
		src &= 63;
		const int r = dst >> min(src, 31), c = (src > 0) & dst >> min(src - 1, 31), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	int lsr8(int src, int dst) {
		src &= 63, dst &= 0xff;
		const int r = -(src < 8) & dst >> src, c = (src > 0 && src < 9) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	int lsr16(int src, int dst) {
		src &= 63, dst &= 0xffff;
		const int r = -(src < 16) & dst >> src, c = (src > 0 && src < 17) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	int lsr32(int src, int dst) {
		src &= 63;
		const int r = -(src < 32) & (unsigned int)dst >> src, c = (src > 0 && src < 33) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	int roxr8(int src, int dst) {
		src = (src & 63) % 9, dst &= 0xff;
		int x = sr >> 4 & 1, r = (dst | x << 8 | dst << 9) >> src & 0xff;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | x, r;
	}

	int roxr16(int src, int dst) {
		src = (src & 63) % 17, dst &= 0xffff;
		int x = sr >> 4 & 1, r = (dst | x << 16 | dst << 17) >> src & 0xffff;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | x, r;
	}

	int roxr32(int src, int dst) {
		src = (src & 63) % 33;
		int x = sr >> 4 & 1, r = -(src > 1) & dst << 33 - src | -(src > 0) & x << 32 - src | -(src < 32) & (unsigned int)dst >> src;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | x, r;
	}

	int ror8(int src, int dst) {
		dst &= 0xff;
		const int r = (dst | dst << 8) >> (src & 7) & 0xff, c = (src > 0) & r >> 7;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2 | c, r;
	}

	int ror16(int src, int dst) {
		dst &= 0xffff;
		const int r = (dst | dst << 16) >> (src & 15) & 0xffff, c = (src > 0) & r >> 15;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2 | c, r;
	}

	int ror32(int src, int dst) {
		const int r = dst << (-src & 31) | (unsigned int)dst >> (src & 31), c = (src > 0) & r >> 31;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | c, r;
	}

	int asl8(int src, int dst) {
		src &= 63;
		const int r = -(src < 8) & dst << src & 0xff, c = (src > 0 && src < 9) & dst << src - 1 >> 7, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7f >> min(src - 1, 7) & 0xff, v = src > 0 ? dst >> 7 & ((~(dst << 1) & m) != 0) | ~dst >> 7 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int asl16(int src, int dst) {
		src &= 63;
		const int r = -(src < 16) & dst << src & 0xffff, c = (src > 0 && src < 17) & dst << src - 1 >> 15, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7fff >> min(src - 1, 15) & 0xffff, v = src > 0 ? dst >> 15 & ((~(dst << 1) & m) != 0) | ~dst >> 15 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | v << 1 | c, r;
	}

	int asl32(int src, int dst) {
		src &= 63;
		const int r = -(src < 32) & dst << src, c = (src > 0 && src < 33) & dst << src - 1 >> 31, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7fffffff >> min(src - 1, 31), v = src > 0 ? dst >> 31 & ((~(dst << 1) & m) != 0) | ~dst >> 31 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | v << 1 | c, r;
	}

	int lsl8(int src, int dst) {
		src &= 63;
		const int r = -(src < 8) & dst << src & 0xff, c = (src > 0 && src < 9) & dst << src - 1 >> 7, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	int lsl16(int src, int dst) {
		src &= 63;
		const int r = -(src < 16) & dst << src & 0xffff, c = (src > 0 && src < 17) & dst << src - 1 >> 15, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	int lsl32(int src, int dst) {
		src &= 63;
		const int r = -(src < 32) & dst << src, c = (src > 0 && src < 33) & dst << src - 1 >> 31, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	int roxl8(int src, int dst) {
		src = (src & 63) % 9, dst &= 0xff;
		int x = sr >> 4 & 1, r = (dst >> 1 | x << 7 | dst << 8) >> 8 - src & 0xff;
		x = src > 0 ? dst >> 8 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | x, r;
	}

	int roxl16(int src, int dst) {
		src = (src & 63) % 17, dst &= 0xffff;
		int x = sr >> 4 & 1, r = (dst >> 1 | x << 15 | dst << 16) >> 16 - src & 0xffff;
		x = src > 0 ? dst >> 16 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | x, r;
	}

	int roxl32(int src, int dst) {
		src = (src & 63) % 33;
		int x = sr >> 4 & 1, r = -(src < 32) & dst << src | -(src > 0) & x << src - 1 | -(src > 1) & (unsigned int)dst >> 33 - src;
		x = src > 0 ? dst >> 32 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | x, r;
	}

	int rol8(int src, int dst) {
		dst &= 0xff;
		const int r = (dst | dst << 8) >> (-src & 7) & 0xff, c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2 | c, r;
	}

	int rol16(int src, int dst) {
		dst &= 0xffff;
		const int r = (dst | dst << 16) >> (-src & 15) & 0xffff, c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2 | c, r;
	}

	int rol32(int src, int dst) {
		const int r = dst << (src & 31) | (unsigned int)dst >> (-src & 31), c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | c, r;
	}

	int negx8(int dst) {
		const int r = -dst - (sr >> 4 & 1) & 0xff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int negx16(int dst) {
		const int r = -dst - (sr >> 4 & 1) & 0xffff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int negx32(int dst) {
		const int r = -dst - (sr >> 4 & 1), v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | sr & !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	int clr() {
		return sr = sr & ~0x0f | 4, 0;
	}

	int neg8(int dst) {
		const int r = -dst & 0xff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int neg16(int dst) {
		const int r = -dst & 0xffff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int neg32(int dst) {
		const int r = -dst, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	int nbcd(int dst) {
		const int h = -(dst & 0x0f) - (sr >> 4 & 1) < 0, c = -(dst >> 4 & 0x0f) - h < 0, r = -dst - (sr >> 4 & 1) - h * 6 - c * 0x60 & 0xff;
		return sr = sr & ~0x15 | c << 4 | sr & !r << 2 | c, r;
	}

	void dbcc(bool cond) {
		const int n = op & 7, addr = disp(pc);
		cond ? (cycle -= 2) : ((d[n] = d[n] & ~0xffff | d[n] - 1 & 0xffff) & 0xffff) != 0xffff ? (pc = addr) : (cycle -= 4);
	}

	int disp(int base) {
		return base + fetch16s();
	}

	int index(int base) {
		const int word = fetch16(), i = word >> 12 & 7, disp = word << 24 >> 24;
		return word & 0x800 ? base + (word & 0x8000 ? a[i] : d[i]) + disp : base + ((word & 0x8000 ? a[i] : d[i]) << 16 >> 16) + disp;
	}

	int txread8(int addr) {
		const int data = txread16(addr & ~1);
		return addr & 1 ? data & 0xff : data >> 8;
	}

	virtual int txread16(int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		return page.base[addr & 0xff] << 8 | page.base[addr + 1 & 0xff];
	}

	int txread32(int addr) {
		const int data = txread16(addr) << 16;
		return data | txread16(addr + 2);
	}

	int fetch16() {
		const int data = txread16(pc);
		return pc = pc + 2, data;
	}

	int fetch16s() {
		const int data = txread16(pc) << 16 >> 16;
		return pc = pc + 2, data;
	}

	int fetch32() {
		const int data = txread32(pc);
		return pc = pc + 4, data;
	}

	int read8(int addr) {
		const int data = read16(addr & ~1);
		return addr & 1 ? data & 0xff : data >> 8;
	}

	virtual int read16(int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		return page.read16 ? page.read16(addr) : page.read ? page.read(addr) << 8 | page.read(addr + 1) : page.base[addr & 0xff] << 8 | page.base[addr + 1 & 0xff];
	}

	int read16s(int addr) {
		return read16(addr) << 16 >> 16;
	}

	int read32(int addr) {
		const int data = read16(addr) << 16;
		return data | read16(addr + 2);
	}

	void write8(int data, int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		page.write ? page.write(addr, data & 0xff) : void(page.base[addr & 0xff] = data);
	}

	void write16(int data, int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		if (page.write16)
			page.write16(addr, data & 0xffff);
		else if (page.write)
			page.write(addr, data >> 8 & 0xff), page.write(addr + 1, data & 0xff);
		else
			page.base[addr & 0xff] = data >> 8, page.base[addr + 1 & 0xff] = data;
	}

	void write32(int data, int addr) {
		write16(data >> 16 & 0xffff, addr), write16(data & 0xffff, addr + 2);
	}
};

#endif //MC68000
