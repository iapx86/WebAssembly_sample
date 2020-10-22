/*
 *
 *	MC68000 Emulator
 *
 */

#ifndef MC68000_H
#define MC68000_H

#include "cpu.h"

struct MC68000 : Cpu {
	int d0 = 0;
	int d1 = 0;
	int d2 = 0;
	int d3 = 0;
	int d4 = 0;
	int d5 = 0;
	int d6 = 0;
	int d7 = 0;
	int a0 = 0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int a4 = 0;
	int a5 = 0;
	int a6 = 0;
	int a7 = 0;
	int ssp = 0;
	int usp = 0;
	int sr = 0; // sr:t-s--iii ccr:---xnzvc
	Page16 memorymap[0x10000];
	int breakpointmap[0x80000] = {};

	MC68000() {}

	void reset() {
		Cpu::reset();
		sr = 0x2700;
		a7 = read32(0);
		pc = read32(4);
	}

	bool interrupt() {
		const int ipl = 1;
		if (!Cpu::interrupt() || ipl <= (sr >> 8 & 7))
			return false;
		return exception(24 + ipl), sr = sr & ~0x0700 | ipl << 8, true;
	}

	bool interrupt(int ipl) {
		if (!Cpu::interrupt() || ipl < 7 && ipl <= (sr >> 8 & 7))
			return false;
		return exception(24 + ipl), sr = sr & ~0x0700 | ipl << 8, true;
	}

	void exception(int vector) {
		if ((sr & 0x2000) == 0)
			usp = a7, a7 = ssp;
		a7 = a7 - 4, write32(pc, a7), a7 = a7 - 2, write16(sr, a7), pc = read32(vector << 2), sr = sr & ~0x8000 | 0x2000;
	}

	void _execute() {
		const int op = fetch16();
		switch (op >> 12) {
		case 0x0: // Bit Manipulation/MOVEP/Immediate
			return execute_0(op);
		case 0x1: // Move Byte
			return execute_1(op);
		case 0x2: // Move Long
			return execute_2(op);
		case 0x3: // Move Word
			return execute_3(op);
		case 0x4: // Miscellaneous
			return execute_4(op);
		case 0x5: // ADDQ/SUBQ/Scc/DBcc
			return execute_5(op);
		case 0x6: // Bcc/BSR
			return execute_6(op);
		case 0x7: // MOVEQ
			return execute_7(op);
		case 0x8: // OR/DIV/SBCD
			return execute_8(op);
		case 0x9: // SUB/SUBX
			return execute_9(op);
		case 0xa: // (Unassigned)
			return exception(10);
		case 0xb: // CMP/EOR
			return execute_b(op);
		case 0xc: // AND/MUL/ABCD/EXG
			return execute_c(op);
		case 0xd: // ADD/ADDX
			return execute_d(op);
		case 0xe: // Shift/Rotate
			return execute_e(op);
		case 0xf: // (Unassigned)
			return exception(11);
		default:
			return;
		}
	}

	void execute_0(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // ORI.B #<data>,Dn
		case 0002: // ORI.B #<data>,(An)
		case 0003: // ORI.B #<data>,(An)+
		case 0004: // ORI.B #<data>,-(An)
		case 0005: // ORI.B #<data>,d(An)
		case 0006: // ORI.B #<data>,d(An,Xi)
			return rwop8(op, or8, fetch16());
		case 0007:
			switch (op & 7) {
			case 0: // ORI.B #<data>,Abs.W
			case 1: // ORI.B #<data>,Abs.L
				return rwop8(op, or8, fetch16());
			case 4: // ORI #<data>,CCR
				return void(sr |= fetch16() & 0xff);
			default:
				return exception(4);
			}
		case 0010: // ORI.W #<data>,Dn
		case 0012: // ORI.W #<data>,(An)
		case 0013: // ORI.W #<data>,(An)+
		case 0014: // ORI.W #<data>,-(An)
		case 0015: // ORI.W #<data>,d(An)
		case 0016: // ORI.W #<data>,d(An,Xi)
			return rwop16(op, or16, fetch16());
		case 0017:
			switch (op & 7) {
			case 0: // ORI.W #<data>,Abs.W
			case 1: // ORI.W #<data>,Abs.L
				return rwop16(op, or16, fetch16());
			case 4: // ORI #<data>,SR
				if ((sr & 0x2000) == 0)
					return exception(8);
				return void(sr |= fetch16());
			default:
				return exception(4);
			}
		case 0020: // ORI.L #<data>,Dn
		case 0022: // ORI.L #<data>,(An)
		case 0023: // ORI.L #<data>,(An)+
		case 0024: // ORI.L #<data>,-(An)
		case 0025: // ORI.L #<data>,d(An)
		case 0026: // ORI.L #<data>,d(An,Xi)
			return rwop32(op, or32, fetch32());
		case 0027: // ORI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, fetch32());
		case 0040: // BTST D0,Dn
			return btst32(d0, rop32(op));
		case 0041: // MOVEP.W d(Ay),D0
			return void(d0 = d0 & ~0xffff | movep(op));
		case 0042: // BTST D0,(An)
		case 0043: // BTST D0,(An)+
		case 0044: // BTST D0,-(An)
		case 0045: // BTST D0,d(An)
		case 0046: // BTST D0,d(An,Xi)
			return btst8(d0, rop8(op));
		case 0047: // BTST D0,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d0, rop8(op));
		case 0050: // BCHG D0,Dn
			return rwop32(op, bchg32, d0);
		case 0051: // MOVEP.L d(Ay),D0
			return void(d0 = movep(op));
		case 0052: // BCHG D0,(An)
		case 0053: // BCHG D0,(An)+
		case 0054: // BCHG D0,-(An)
		case 0055: // BCHG D0,d(An)
		case 0056: // BCHG D0,d(An,Xi)
			return rwop8(op, bchg8, d0);
		case 0057: // BCHG D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d0);
		case 0060: // BCLR D0,Dn
			return rwop32(op, bclr32, d0);
		case 0061: // MOVEP.W D0,d(Ay)
			return void(movep(op, d0));
		case 0062: // BCLR D0,(An)
		case 0063: // BCLR D0,(An)+
		case 0064: // BCLR D0,-(An)
		case 0065: // BCLR D0,d(An)
		case 0066: // BCLR D0,d(An,Xi)
			return rwop8(op, bclr8, d0);
		case 0067: // BCLR D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d0);
		case 0070: // BSET D0,Dn
			return rwop32(op, bset32, d0);
		case 0071: // MOVEP.L D0,d(Ay)
			return void(movep(op, d0));
		case 0072: // BSET D0,(An)
		case 0073: // BSET D0,(An)+
		case 0074: // BSET D0,-(An)
		case 0075: // BSET D0,d(An)
		case 0076: // BSET D0,d(An,Xi)
			return rwop8(op, bset8, d0);
		case 0077: // BSET D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d0);
		case 0100: // ANDI.B #<data>,Dn
		case 0102: // ANDI.B #<data>,(An)
		case 0103: // ANDI.B #<data>,(An)+
		case 0104: // ANDI.B #<data>,-(An)
		case 0105: // ANDI.B #<data>,d(An)
		case 0106: // ANDI.B #<data>,d(An,Xi)
			return rwop8(op, and8, fetch16());
		case 0107:
			switch (op & 7) {
			case 0: // ANDI.B #<data>,Abs.W
			case 1: // ANDI.B #<data>,Abs.L
				return rwop8(op, and8, fetch16());
			case 4: // ANDI #<data>,CCR
				return void(sr &= fetch16() | ~0xff);
			default:
				return exception(4);
			}
		case 0110: // ANDI.W #<data>,Dn
		case 0112: // ANDI.W #<data>,(An)
		case 0113: // ANDI.W #<data>,(An)+
		case 0114: // ANDI.W #<data>,-(An)
		case 0115: // ANDI.W #<data>,d(An)
		case 0116: // ANDI.W #<data>,d(An,Xi)
			return rwop16(op, and16, fetch16());
		case 0117:
			switch (op & 7) {
			case 0: // ANDI.W #<data>,Abs.W
			case 1: // ANDI.W #<data>,Abs.L
				return rwop16(op, and16, fetch16());
			case 4: // ANDI #<data>,SR
				if ((sr & 0x2000) == 0)
					return exception(8);
				sr &= fetch16();
				if ((sr & 0x2000) == 0)
					ssp = a7, a7 = usp;
				return;
			default:
				return exception(4);
			}
		case 0120: // ANDI.L #<data>,Dn
		case 0122: // ANDI.L #<data>,(An)
		case 0123: // ANDI.L #<data>,(An)+
		case 0124: // ANDI.L #<data>,-(An)
		case 0125: // ANDI.L #<data>,d(An)
		case 0126: // ANDI.L #<data>,d(An,Xi)
			return rwop32(op, and32, fetch32());
		case 0127: // ANDI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, fetch32());
		case 0140: // BTST D1,Dn
			return btst32(d1, rop32(op));
		case 0141: // MOVEP.W d(Ay),D1
			return void(d1 = d1 & ~0xffff | movep(op));
		case 0142: // BTST D1,(An)
		case 0143: // BTST D1,(An)+
		case 0144: // BTST D1,-(An)
		case 0145: // BTST D1,d(An)
		case 0146: // BTST D1,d(An,Xi)
			return btst8(d1, rop8(op));
		case 0147: // BTST D1,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d1, rop8(op));
		case 0150: // BCHG D1,Dn
			return rwop32(op, bchg32, d1);
		case 0151: // MOVEP.L d(Ay),D1
			return void(d1 = movep(op));
		case 0152: // BCHG D1,(An)
		case 0153: // BCHG D1,(An)+
		case 0154: // BCHG D1,-(An)
		case 0155: // BCHG D1,d(An)
		case 0156: // BCHG D1,d(An,Xi)
			return rwop8(op, bchg8, d1);
		case 0157: // BCHG D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d1);
		case 0160: // BCLR D1,Dn
			return rwop32(op, bclr32, d1);
		case 0161: // MOVEP.W D1,d(Ay)
			return void(movep(op, d1));
		case 0162: // BCLR D1,(An)
		case 0163: // BCLR D1,(An)+
		case 0164: // BCLR D1,-(An)
		case 0165: // BCLR D1,d(An)
		case 0166: // BCLR D1,d(An,Xi)
			return rwop8(op, bclr8, d1);
		case 0167: // BCLR D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d1);
		case 0170: // BSET D1,Dn
			return rwop32(op, bset32, d1);
		case 0171: // MOVEP.L D1,d(Ay)
			return void(movep(op, d1));
		case 0172: // BSET D1,(An)
		case 0173: // BSET D1,(An)+
		case 0174: // BSET D1,-(An)
		case 0175: // BSET D1,d(An)
		case 0176: // BSET D1,d(An,Xi)
			return rwop8(op, bset8, d1);
		case 0177: // BSET D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d1);
		case 0200: // SUBI.B #<data>,Dn
		case 0202: // SUBI.B #<data>,(An)
		case 0203: // SUBI.B #<data>,(An)+
		case 0204: // SUBI.B #<data>,-(An)
		case 0205: // SUBI.B #<data>,d(An)
		case 0206: // SUBI.B #<data>,d(An,Xi)
			return rwop8(op, sub8, fetch16());
		case 0207: // SUBI.B #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, fetch16());
		case 0210: // SUBI.W #<data>,Dn
		case 0212: // SUBI.W #<data>,(An)
		case 0213: // SUBI.W #<data>,(An)+
		case 0214: // SUBI.W #<data>,-(An)
		case 0215: // SUBI.W #<data>,d(An)
		case 0216: // SUBI.W #<data>,d(An,Xi)
			return rwop16(op, sub16, fetch16());
		case 0217: // SUBI.W #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, fetch16());
		case 0220: // SUBI.L #<data>,Dn
		case 0222: // SUBI.L #<data>,(An)
		case 0223: // SUBI.L #<data>,(An)+
		case 0224: // SUBI.L #<data>,-(An)
		case 0225: // SUBI.L #<data>,d(An)
		case 0226: // SUBI.L #<data>,d(An,Xi)
			return rwop32(op, sub32, fetch32());
		case 0227: // SUBI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, fetch32());
		case 0240: // BTST D2,Dn
			return btst32(d2, rop32(op));
		case 0241: // MOVEP.W d(Ay),D2
			return void(d2 = d2 & ~0xffff | movep(op));
		case 0242: // BTST D2,(An)
		case 0243: // BTST D2,(An)+
		case 0244: // BTST D2,-(An)
		case 0245: // BTST D2,d(An)
		case 0246: // BTST D2,d(An,Xi)
			return btst8(d2, rop8(op));
		case 0247: // BTST D2,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d2, rop8(op));
		case 0250: // BCHG D2,Dn
			return rwop32(op, bchg32, d2);
		case 0251: // MOVEP.L d(Ay),D2
			return void(d2 = movep(op));
		case 0252: // BCHG D2,(An)
		case 0253: // BCHG D2,(An)+
		case 0254: // BCHG D2,-(An)
		case 0255: // BCHG D2,d(An)
		case 0256: // BCHG D2,d(An,Xi)
			return rwop8(op, bchg8, d2);
		case 0257: // BCHG D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d2);
		case 0260: // BCLR D2,Dn
			return rwop32(op, bclr32, d2);
		case 0261: // MOVEP.W D2,d(Ay)
			return void(movep(op, d2));
		case 0262: // BCLR D2,(An)
		case 0263: // BCLR D2,(An)+
		case 0264: // BCLR D2,-(An)
		case 0265: // BCLR D2,d(An)
		case 0266: // BCLR D2,d(An,Xi)
			return rwop8(op, bclr8, d2);
		case 0267: // BCLR D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d2);
		case 0270: // BSET D2,Dn
			return rwop32(op, bset32, d2);
		case 0271: // MOVEP.L D2,d(Ay)
			return void(movep(op, d2));
		case 0272: // BSET D2,(An)
		case 0273: // BSET D2,(An)+
		case 0274: // BSET D2,-(An)
		case 0275: // BSET D2,d(An)
		case 0276: // BSET D2,d(An,Xi)
			return rwop8(op, bset8, d2);
		case 0277: // BSET D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d2);
		case 0300: // ADDI.B #<data>,Dn
		case 0302: // ADDI.B #<data>,(An)
		case 0303: // ADDI.B #<data>,(An)+
		case 0304: // ADDI.B #<data>,-(An)
		case 0305: // ADDI.B #<data>,d(An)
		case 0306: // ADDI.B #<data>,d(An,Xi)
			return rwop8(op, add8, fetch16());
		case 0307: // ADDI.B #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, fetch16());
		case 0310: // ADDI.W #<data>,Dn
		case 0312: // ADDI.W #<data>,(An)
		case 0313: // ADDI.W #<data>,(An)+
		case 0314: // ADDI.W #<data>,-(An)
		case 0315: // ADDI.W #<data>,d(An)
		case 0316: // ADDI.W #<data>,d(An,Xi)
			return rwop16(op, add16, fetch16());
		case 0317: // ADDI.W #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, fetch16());
		case 0320: // ADDI.L #<data>,Dn
		case 0322: // ADDI.L #<data>,(An)
		case 0323: // ADDI.L #<data>,(An)+
		case 0324: // ADDI.L #<data>,-(An)
		case 0325: // ADDI.L #<data>,d(An)
		case 0326: // ADDI.L #<data>,d(An,Xi)
			return rwop32(op, add32, fetch32());
		case 0327: // ADDI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, fetch32());
		case 0340: // BTST D3,Dn
			return btst32(d3, rop32(op));
		case 0341: // MOVEP.W d(Ay),D3
			return void(d3 = d3 & ~0xffff | movep(op));
		case 0342: // BTST D3,(An)
		case 0343: // BTST D3,(An)+
		case 0344: // BTST D3,-(An)
		case 0345: // BTST D3,d(An)
		case 0346: // BTST D3,d(An,Xi)
			return btst8(d3, rop8(op));
		case 0347: // BTST D3,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d3, rop8(op));
		case 0350: // BCHG D3,Dn
			return rwop32(op, bchg32, d3);
		case 0351: // MOVEP.L d(Ay),D3
			return void(d3 = movep(op));
		case 0352: // BCHG D3,(An)
		case 0353: // BCHG D3,(An)+
		case 0354: // BCHG D3,-(An)
		case 0355: // BCHG D3,d(An)
		case 0356: // BCHG D3,d(An,Xi)
			return rwop8(op, bchg8, d3);
		case 0357: // BCHG D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d3);
		case 0360: // BCLR D3,Dn
			return rwop32(op, bclr32, d3);
		case 0361: // MOVEP.W D3,d(Ay)
			return void(movep(op, d3));
		case 0362: // BCLR D3,(An)
		case 0363: // BCLR D3,(An)+
		case 0364: // BCLR D3,-(An)
		case 0365: // BCLR D3,d(An)
		case 0366: // BCLR D3,d(An,Xi)
			return rwop8(op, bclr8, d3);
		case 0367: // BCLR D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d3);
		case 0370: // BSET D3,Dn
			return rwop32(op, bset32, d3);
		case 0371: // MOVEP.L D3,d(Ay)
			return void(movep(op, d3));
		case 0372: // BSET D3,(An)
		case 0373: // BSET D3,(An)+
		case 0374: // BSET D3,-(An)
		case 0375: // BSET D3,d(An)
		case 0376: // BSET D3,d(An,Xi)
			return rwop8(op, bset8, d3);
		case 0377: // BSET D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d3);
		case 0400: // BTST #<data>,Dn
			return src = fetch16(), btst32(src, rop32(op));
		case 0402: // BTST #<data>,(An)
		case 0403: // BTST #<data>,(An)+
		case 0404: // BTST #<data>,-(An)
		case 0405: // BTST #<data>,d(An)
		case 0406: // BTST #<data>,d(An,Xi)
			return src = fetch16(), btst8(src, rop8(op));
		case 0407: // BTST #<data>,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return src = fetch16(), btst8(src, rop8(op));
		case 0410: // BCHG #<data>,Dn
			return rwop32(op, bchg32, fetch16());
		case 0412: // BCHG #<data>,(An)
		case 0413: // BCHG #<data>,(An)+
		case 0414: // BCHG #<data>,-(An)
		case 0415: // BCHG #<data>,d(An)
		case 0416: // BCHG #<data>,d(An,Xi)
			return rwop8(op, bchg8, fetch16());
		case 0417: // BCHG #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, fetch16());
		case 0420: // BCLR #<data>,Dn
			return rwop32(op, bclr32, fetch16());
		case 0422: // BCLR #<data>,(An)
		case 0423: // BCLR #<data>,(An)+
		case 0424: // BCLR #<data>,-(An)
		case 0425: // BCLR #<data>,d(An)
		case 0426: // BCLR #<data>,d(An,Xi)
			return rwop8(op, bclr8, fetch16());
		case 0427: // BCLR #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, fetch16());
		case 0430: // BSET #<data>,Dn
			return rwop32(op, bset32, fetch16());
		case 0432: // BSET #<data>,(An)
		case 0433: // BSET #<data>,(An)+
		case 0434: // BSET #<data>,-(An)
		case 0435: // BSET #<data>,d(An)
		case 0436: // BSET #<data>,d(An,Xi)
			return rwop8(op, bset8, fetch16());
		case 0437: // BSET #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, fetch16());
		case 0440: // BTST D4,Dn
			return btst32(d4, rop32(op));
		case 0441: // MOVEP.W d(Ay),D4
			return void(d4 = d4 & ~0xffff | movep(op));
		case 0442: // BTST D4,(An)
		case 0443: // BTST D4,(An)+
		case 0444: // BTST D4,-(An)
		case 0445: // BTST D4,d(An)
		case 0446: // BTST D4,d(An,Xi)
			return btst8(d4, rop8(op));
		case 0447: // BTST D4,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d4, rop8(op));
		case 0450: // BCHG D4,Dn
			return rwop32(op, bchg32, d4);
		case 0451: // MOVEP.L d(Ay),D4
			return void(d4 = movep(op));
		case 0452: // BCHG D4,(An)
		case 0453: // BCHG D4,(An)+
		case 0454: // BCHG D4,-(An)
		case 0455: // BCHG D4,d(An)
		case 0456: // BCHG D4,d(An,Xi)
			return rwop8(op, bchg8, d4);
		case 0457: // BCHG D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d4);
		case 0460: // BCLR D4,Dn
			return rwop32(op, bclr32, d4);
		case 0461: // MOVEP.W D4,d(Ay)
			return void(movep(op, d4));
		case 0462: // BCLR D4,(An)
		case 0463: // BCLR D4,(An)+
		case 0464: // BCLR D4,-(An)
		case 0465: // BCLR D4,d(An)
		case 0466: // BCLR D4,d(An,Xi)
			return rwop8(op, bclr8, d4);
		case 0467: // BCLR D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d4);
		case 0470: // BSET D4,Dn
			return rwop32(op, bset32, d4);
		case 0471: // MOVEP.L D4,d(Ay)
			return void(movep(op, d4));
		case 0472: // BSET D4,(An)
		case 0473: // BSET D4,(An)+
		case 0474: // BSET D4,-(An)
		case 0475: // BSET D4,d(An)
		case 0476: // BSET D4,d(An,Xi)
			return rwop8(op, bset8, d4);
		case 0477: // BSET D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d4);
		case 0500: // EORI.B #<data>,Dn
		case 0502: // EORI.B #<data>,(An)
		case 0503: // EORI.B #<data>,(An)+
		case 0504: // EORI.B #<data>,-(An)
		case 0505: // EORI.B #<data>,d(An)
		case 0506: // EORI.B #<data>,d(An,Xi)
			return rwop8(op, eor8, fetch16());
		case 0507:
			switch (op & 7) {
			case 0: // EORI.B #<data>,Abs.W
			case 1: // EORI.B #<data>,Abs.L
				return rwop8(op, eor8, fetch16());
			case 4: // EORI #<data>,CCR
				return void(sr ^= fetch16() & 0xff);
			default:
				return exception(4);
			}
		case 0510: // EORI.W #<data>,Dn
		case 0512: // EORI.W #<data>,(An)
		case 0513: // EORI.W #<data>,(An)+
		case 0514: // EORI.W #<data>,-(An)
		case 0515: // EORI.W #<data>,d(An)
		case 0516: // EORI.W #<data>,d(An,Xi)
			return rwop16(op, eor16, fetch16());
		case 0517:
			switch (op & 7) {
			case 0: // EORI.W #<data>,Abs.W
			case 1: // EORI.W #<data>,Abs.L
				return rwop16(op, eor16, fetch16());
			case 4: // EORI #<data>,SR
				if ((sr & 0x2000) == 0)
					return exception(8);
				sr ^= fetch16();
				if ((sr & 0x2000) == 0)
					ssp = a7, a7 = usp;
				return;
			default:
				return exception(4);
			}
		case 0520: // EORI.L #<data>,Dn
		case 0522: // EORI.L #<data>,(An)
		case 0523: // EORI.L #<data>,(An)+
		case 0524: // EORI.L #<data>,-(An)
		case 0525: // EORI.L #<data>,d(An)
		case 0526: // EORI.L #<data>,d(An,Xi)
			return rwop32(op, eor32, fetch32());
		case 0527: // EORI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, fetch32());
		case 0540: // BTST D5,Dn
			return btst32(d5, rop32(op));
		case 0541: // MOVEP.W d(Ay),D5
			return void(d5 = d5 & ~0xffff | movep(op));
		case 0542: // BTST D5,(An)
		case 0543: // BTST D5,(An)+
		case 0544: // BTST D5,-(An)
		case 0545: // BTST D5,d(An)
		case 0546: // BTST D5,d(An,Xi)
			return btst8(d5, rop8(op));
		case 0547: // BTST D5,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d5, rop8(op));
		case 0550: // BCHG D5,Dn
			return rwop32(op, bchg32, d5);
		case 0551: // MOVEP.L d(Ay),D5
			return void(d5 = movep(op));
		case 0552: // BCHG D5,(An)
		case 0553: // BCHG D5,(An)+
		case 0554: // BCHG D5,-(An)
		case 0555: // BCHG D5,d(An)
		case 0556: // BCHG D5,d(An,Xi)
			return rwop8(op, bchg8, d5);
		case 0557: // BCHG D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d5);
		case 0560: // BCLR D5,Dn
			return rwop32(op, bclr32, d5);
		case 0561: // MOVEP.W D5,d(Ay)
			return void(movep(op, d5));
		case 0562: // BCLR D5,(An)
		case 0563: // BCLR D5,(An)+
		case 0564: // BCLR D5,-(An)
		case 0565: // BCLR D5,d(An)
		case 0566: // BCLR D5,d(An,Xi)
			return rwop8(op, bclr8, d5);
		case 0567: // BCLR D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d5);
		case 0570: // BSET D5,Dn
			return rwop32(op, bset32, d5);
		case 0571: // MOVEP.L D5,d(Ay)
			return void(movep(op, d5));
		case 0572: // BSET D5,(An)
		case 0573: // BSET D5,(An)+
		case 0574: // BSET D5,-(An)
		case 0575: // BSET D5,d(An)
		case 0576: // BSET D5,d(An,Xi)
			return rwop8(op, bset8, d5);
		case 0577: // BSET D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d5);
		case 0600: // CMPI.B #<data>,Dn
		case 0602: // CMPI.B #<data>,(An)
		case 0603: // CMPI.B #<data>,(An)+
		case 0604: // CMPI.B #<data>,-(An)
		case 0605: // CMPI.B #<data>,d(An)
		case 0606: // CMPI.B #<data>,d(An,Xi)
			return src = fetch16(), cmp8(src, rop8(op));
		case 0607: // CMPI.B #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return src = fetch16(), cmp8(src, rop8(op));
		case 0610: // CMPI.W #<data>,Dn
		case 0612: // CMPI.W #<data>,(An)
		case 0613: // CMPI.W #<data>,(An)+
		case 0614: // CMPI.W #<data>,-(An)
		case 0615: // CMPI.W #<data>,d(An)
		case 0616: // CMPI.W #<data>,d(An,Xi)
			return src = fetch16(), cmp16(src, rop16(op));
		case 0617: // CMPI.W #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return src = fetch16(), cmp16(src, rop16(op));
		case 0620: // CMPI.L #<data>,Dn
		case 0622: // CMPI.L #<data>,(An)
		case 0623: // CMPI.L #<data>,(An)+
		case 0624: // CMPI.L #<data>,-(An)
		case 0625: // CMPI.L #<data>,d(An)
		case 0626: // CMPI.L #<data>,d(An,Xi)
			return src = fetch32(), cmp32(src, rop32(op));
		case 0627: // CMPI.L #<data>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return src = fetch32(), cmp32(src, rop32(op));
		case 0640: // BTST D6,Dn
			return btst32(d6, rop32(op));
		case 0641: // MOVEP.W d(Ay),D6
			return void(d6 = d6 & ~0xffff | movep(op));
		case 0642: // BTST D6,(An)
		case 0643: // BTST D6,(An)+
		case 0644: // BTST D6,-(An)
		case 0645: // BTST D6,d(An)
		case 0646: // BTST D6,d(An,Xi)
			return btst8(d6, rop8(op));
		case 0647: // BTST D6,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d6, rop8(op));
		case 0650: // BCHG D6,Dn
			return rwop32(op, bchg32, d6);
		case 0651: // MOVEP.L d(Ay),D6
			return void(d6 = movep(op));
		case 0652: // BCHG D6,(An)
		case 0653: // BCHG D6,(An)+
		case 0654: // BCHG D6,-(An)
		case 0655: // BCHG D6,d(An)
		case 0656: // BCHG D6,d(An,Xi)
			return rwop8(op, bchg8, d6);
		case 0657: // BCHG D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d6);
		case 0660: // BCLR D6,Dn
			return rwop32(op, bclr32, d6);
		case 0661: // MOVEP.W D6,d(Ay)
			return void(movep(op, d6));
		case 0662: // BCLR D6,(An)
		case 0663: // BCLR D6,(An)+
		case 0664: // BCLR D6,-(An)
		case 0665: // BCLR D6,d(An)
		case 0666: // BCLR D6,d(An,Xi)
			return rwop8(op, bclr8, d6);
		case 0667: // BCLR D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d6);
		case 0670: // BSET D6,Dn
			return rwop32(op, bset32, d6);
		case 0671: // MOVEP.L D6,d(Ay)
			return void(movep(op, d6));
		case 0672: // BSET D6,(An)
		case 0673: // BSET D6,(An)+
		case 0674: // BSET D6,-(An)
		case 0675: // BSET D6,d(An)
		case 0676: // BSET D6,d(An,Xi)
			return rwop8(op, bset8, d6);
		case 0677: // BSET D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d6);
		case 0740: // BTST D7,Dn
			return btst32(d7, rop32(op));
		case 0741: // MOVEP.W d(Ay),D7
			return void(d7 = d7 & ~0xffff | movep(op));
		case 0742: // BTST D7,(An)
		case 0743: // BTST D7,(An)+
		case 0744: // BTST D7,-(An)
		case 0745: // BTST D7,d(An)
		case 0746: // BTST D7,d(An,Xi)
			return btst8(d7, rop8(op));
		case 0747: // BTST D7,Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return btst8(d7, rop8(op));
		case 0750: // BCHG D7,Dn
			return rwop32(op, bchg32, d7);
		case 0751: // MOVEP.L d(Ay),D7
			return void(d7 = movep(op));
		case 0752: // BCHG D7,(An)
		case 0753: // BCHG D7,(An)+
		case 0754: // BCHG D7,-(An)
		case 0755: // BCHG D7,d(An)
		case 0756: // BCHG D7,d(An,Xi)
			return rwop8(op, bchg8, d7);
		case 0757: // BCHG D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bchg8, d7);
		case 0760: // BCLR D7,Dn
			return rwop32(op, bclr32, d7);
		case 0761: // MOVEP.W D7,d(Ay)
			return void(movep(op, d7));
		case 0762: // BCLR D7,(An)
		case 0763: // BCLR D7,(An)+
		case 0764: // BCLR D7,-(An)
		case 0765: // BCLR D7,d(An)
		case 0766: // BCLR D7,d(An,Xi)
			return rwop8(op, bclr8, d7);
		case 0767: // BCLR D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bclr8, d7);
		case 0770: // BSET D7,Dn
			return rwop32(op, bset32, d7);
		case 0771: // MOVEP.L D7,d(Ay)
			return void(movep(op, d7));
		case 0772: // BSET D7,(An)
		case 0773: // BSET D7,(An)+
		case 0774: // BSET D7,-(An)
		case 0775: // BSET D7,d(An)
		case 0776: // BSET D7,d(An,Xi)
			return rwop8(op, bset8, d7);
		case 0777: // BSET D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, bset8, d7);
		default:
			return exception(4);
		}
	}

	void execute_1(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // MOVE.B Dn,D0
		case 0002: // MOVE.B (An),D0
		case 0003: // MOVE.B (An)+,D0
		case 0004: // MOVE.B -(An),D0
		case 0005: // MOVE.B d(An),D0
		case 0006: // MOVE.B d(An,Xi),D0
			return void(d0 = d0 & ~0xff | rop8(op, true));
		case 0007: // MOVE.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xff | rop8(op, true));
		case 0020: // MOVE.B Dn,(A0)
		case 0022: // MOVE.B (An),(A0)
		case 0023: // MOVE.B (An)+,(A0)
		case 0024: // MOVE.B -(An),(A0)
		case 0025: // MOVE.B d(An),(A0)
		case 0026: // MOVE.B d(An,Xi),(A0)
			return src = rop8(op, true), write8(src, a0);
		case 0027: // MOVE.B Abs...,(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a0);
		case 0030: // MOVE.B Dn,(A0)+
		case 0032: // MOVE.B (An),(A0)+
		case 0033: // MOVE.B (An)+,(A0)+
		case 0034: // MOVE.B -(An),(A0)+
		case 0035: // MOVE.B d(An),(A0)+
		case 0036: // MOVE.B d(An,Xi),(A0)+
			return src = rop8(op, true), write8(src, a0), void(a0 = a0 + 1);
		case 0037: // MOVE.B Abs...,(A0)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a0), void(a1 = a1 + 1);
		case 0040: // MOVE.B Dn,-(A0)
		case 0042: // MOVE.B (An),-(A0)
		case 0043: // MOVE.B (An)+,-(A0)
		case 0044: // MOVE.B -(An),-(A0)
		case 0045: // MOVE.B d(An),-(A0)
		case 0046: // MOVE.B d(An,Xi),-(A0)
			return src = rop8(op, true), a0 = a0 - 1, write8(src, a0);
		case 0047: // MOVE.B Abs...,-(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a0 = a0 - 1, write8(src, a0);
		case 0050: // MOVE.B Dn,d(A0)
		case 0052: // MOVE.B (An),d(A0)
		case 0053: // MOVE.B (An)+,d(A0)
		case 0054: // MOVE.B -(An),d(A0)
		case 0055: // MOVE.B d(An),d(A0)
		case 0056: // MOVE.B d(An,Xi),d(A0)
			return src = rop8(op, true), write8(src, disp(a0));
		case 0057: // MOVE.B Abs...,d(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a0));
		case 0060: // MOVE.B Dn,d(A0,Xi)
		case 0062: // MOVE.B (An),d(A0,Xi)
		case 0063: // MOVE.B (An)+,d(A0,Xi)
		case 0064: // MOVE.B -(An),d(A0,Xi)
		case 0065: // MOVE.B d(An),d(A0,Xi)
		case 0066: // MOVE.B d(An,Xi),d(A0,Xi)
			return src = rop8(op, true), write8(src, index(a0));
		case 0067: // MOVE.B Abs...,d(A0,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a0));
		case 0070: // MOVE.B Dn,Abs.W
		case 0072: // MOVE.B (An),Abs.W
		case 0073: // MOVE.B (An)+,Abs.W
		case 0074: // MOVE.B -(An),Abs.W
		case 0075: // MOVE.B d(An),Abs.W
		case 0076: // MOVE.B d(An,Xi),Abs.W
			return src = rop8(op, true), write8(src, fetch16s());
		case 0077: // MOVE.B Abs...,Abs.W
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, fetch16s());
		case 0100: // MOVE.B Dn,D1
		case 0102: // MOVE.B (An),D1
		case 0103: // MOVE.B (An)+,D1
		case 0104: // MOVE.B -(An),D1
		case 0105: // MOVE.B d(An),D1
		case 0106: // MOVE.B d(An,Xi),D1
			return void(d1 = d1 & ~0xff | rop8(op, true));
		case 0107: // MOVE.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xff | rop8(op, true));
		case 0120: // MOVE.B Dn,(A1)
		case 0122: // MOVE.B (An),(A1)
		case 0123: // MOVE.B (An)+,(A1)
		case 0124: // MOVE.B -(An),(A1)
		case 0125: // MOVE.B d(An),(A1)
		case 0126: // MOVE.B d(An,Xi),(A1)
			return src = rop8(op, true), write8(src, a1);
		case 0127: // MOVE.B Abs...,(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a1);
		case 0130: // MOVE.B Dn,(A1)+
		case 0132: // MOVE.B (An),(A1)+
		case 0133: // MOVE.B (An)+,(A1)+
		case 0134: // MOVE.B -(An),(A1)+
		case 0135: // MOVE.B d(An),(A1)+
		case 0136: // MOVE.B d(An,Xi),(A1)+
			return src = rop8(op, true), write8(src, a1), void(a1 = a1 + 1);
		case 0137: // MOVE.B Abs...,(A1)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a1), void(a1 = a1 + 1);
		case 0140: // MOVE.B Dn,-(A1)
		case 0142: // MOVE.B (An),-(A1)
		case 0143: // MOVE.B (An)+,-(A1)
		case 0144: // MOVE.B -(An),-(A1)
		case 0145: // MOVE.B d(An),-(A1)
		case 0146: // MOVE.B d(An,Xi),-(A1)
			return src = rop8(op, true), a1 = a1 - 1, write8(src, a1);
		case 0147: // MOVE.B Abs...,-(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a1 = a1 - 1, write8(src, a1);
		case 0150: // MOVE.B Dn,d(A1)
		case 0152: // MOVE.B (An),d(A1)
		case 0153: // MOVE.B (An)+,d(A1)
		case 0154: // MOVE.B -(An),d(A1)
		case 0155: // MOVE.B d(An),d(A1)
		case 0156: // MOVE.B d(An,Xi),d(A1)
			return src = rop8(op, true), write8(src, disp(a1));
		case 0157: // MOVE.B Abs...,d(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a1));
		case 0160: // MOVE.B Dn,d(A1,Xi)
		case 0162: // MOVE.B (An),d(A1,Xi)
		case 0163: // MOVE.B (An)+,d(A1,Xi)
		case 0164: // MOVE.B -(An),d(A1,Xi)
		case 0165: // MOVE.B d(An),d(A1,Xi)
		case 0166: // MOVE.B d(An,Xi),d(A1,Xi)
			return src = rop8(op, true), write8(src, index(a1));
		case 0167: // MOVE.B Abs...,d(A1,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a1));
		case 0170: // MOVE.B Dn,Abs.L
		case 0172: // MOVE.B (An),Abs.L
		case 0173: // MOVE.B (An)+,Abs.L
		case 0174: // MOVE.B -(An),Abs.L
		case 0175: // MOVE.B d(An),Abs.L
		case 0176: // MOVE.B d(An,Xi),Abs.L
			return src = rop8(op, true), write8(src, fetch32());
		case 0177: // MOVE.B Abs...,Abs.L
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, fetch32());
		case 0200: // MOVE.B Dn,D2
		case 0202: // MOVE.B (An),D2
		case 0203: // MOVE.B (An)+,D2
		case 0204: // MOVE.B -(An),D2
		case 0205: // MOVE.B d(An),D2
		case 0206: // MOVE.B d(An,Xi),D2
			return void(d2 = d2 & ~0xff | rop8(op, true));
		case 0207: // MOVE.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xff | rop8(op, true));
		case 0220: // MOVE.B Dn,(A2)
		case 0222: // MOVE.B (An),(A2)
		case 0223: // MOVE.B (An)+,(A2)
		case 0224: // MOVE.B -(An),(A2)
		case 0225: // MOVE.B d(An),(A2)
		case 0226: // MOVE.B d(An,Xi),(A2)
			return src = rop8(op, true), write8(src, a2);
		case 0227: // MOVE.B Abs...,(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a2);
		case 0230: // MOVE.B Dn,(A2)+
		case 0232: // MOVE.B (An),(A2)+
		case 0233: // MOVE.B (An)+,(A2)+
		case 0234: // MOVE.B -(An),(A2)+
		case 0235: // MOVE.B d(An),(A2)+
		case 0236: // MOVE.B d(An,Xi),(A2)+
			return src = rop8(op, true), write8(src, a2), void(a2 = a2 + 1);
		case 0237: // MOVE.B Abs...,(A2)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a2), void(a2 = a2 + 1);
		case 0240: // MOVE.B Dn,-(A2)
		case 0242: // MOVE.B (An),-(A2)
		case 0243: // MOVE.B (An)+,-(A2)
		case 0244: // MOVE.B -(An),-(A2)
		case 0245: // MOVE.B d(An),-(A2)
		case 0246: // MOVE.B d(An,Xi),-(A2)
			return src = rop8(op, true), a2 = a2 - 1, write8(src, a2);
		case 0247: // MOVE.B Abs...,-(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a2 = a2 - 1, write8(src, a2);
		case 0250: // MOVE.B Dn,d(A2)
		case 0252: // MOVE.B (An),d(A2)
		case 0253: // MOVE.B (An)+,d(A2)
		case 0254: // MOVE.B -(An),d(A2)
		case 0255: // MOVE.B d(An),d(A2)
		case 0256: // MOVE.B d(An,Xi),d(A2)
			return src = rop8(op, true), write8(src, disp(a2));
		case 0257: // MOVE.B Abs...,d(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a2));
		case 0260: // MOVE.B Dn,d(A2,Xi)
		case 0262: // MOVE.B (An),d(A2,Xi)
		case 0263: // MOVE.B (An)+,d(A2,Xi)
		case 0264: // MOVE.B -(An),d(A2,Xi)
		case 0265: // MOVE.B d(An),d(A2,Xi)
		case 0266: // MOVE.B d(An,Xi),d(A2,Xi)
			return src = rop8(op, true), write8(src, index(a2));
		case 0267: // MOVE.B Abs...,d(A2,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a2));
		case 0300: // MOVE.B Dn,D3
		case 0302: // MOVE.B (An),D3
		case 0303: // MOVE.B (An)+,D3
		case 0304: // MOVE.B -(An),D3
		case 0305: // MOVE.B d(An),D3
		case 0306: // MOVE.B d(An,Xi),D3
			return void(d3 = d3 & ~0xff | rop8(op, true));
		case 0307: // MOVE.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xff | rop8(op, true));
		case 0320: // MOVE.B Dn,(A3)
		case 0322: // MOVE.B (An),(A3)
		case 0323: // MOVE.B (An)+,(A3)
		case 0324: // MOVE.B -(An),(A3)
		case 0325: // MOVE.B d(An),(A3)
		case 0326: // MOVE.B d(An,Xi),(A3)
			return src = rop8(op, true), write8(src, a3);
		case 0327: // MOVE.B Abs...,(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a3);
		case 0330: // MOVE.B Dn,(A3)+
		case 0332: // MOVE.B (An),(A3)+
		case 0333: // MOVE.B (An)+,(A3)+
		case 0334: // MOVE.B -(An),(A3)+
		case 0335: // MOVE.B d(An),(A3)+
		case 0336: // MOVE.B d(An,Xi),(A3)+
			return src = rop8(op, true), write8(src, a3), void(a3 = a3 + 1);
		case 0337: // MOVE.B Abs...,(A3)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a3), void(a3 = a3 + 1);
		case 0340: // MOVE.B Dn,-(A3)
		case 0342: // MOVE.B (An),-(A3)
		case 0343: // MOVE.B (An)+,-(A3)
		case 0344: // MOVE.B -(An),-(A3)
		case 0345: // MOVE.B d(An),-(A3)
		case 0346: // MOVE.B d(An,Xi),-(A3)
			return src = rop8(op, true), a3 = a3 - 1, write8(src, a3);
		case 0347: // MOVE.B Abs...,-(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a3 = a3 - 1, write8(src, a3);
		case 0350: // MOVE.B Dn,d(A3)
		case 0352: // MOVE.B (An),d(A3)
		case 0353: // MOVE.B (An)+,d(A3)
		case 0354: // MOVE.B -(An),d(A3)
		case 0355: // MOVE.B d(An),d(A3)
		case 0356: // MOVE.B d(An,Xi),d(A3)
			return src = rop8(op, true), write8(src, disp(a3));
		case 0357: // MOVE.B Abs...,d(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a3));
		case 0360: // MOVE.B Dn,d(A3,Xi)
		case 0362: // MOVE.B (An),d(A3,Xi)
		case 0363: // MOVE.B (An)+,d(A3,Xi)
		case 0364: // MOVE.B -(An),d(A3,Xi)
		case 0365: // MOVE.B d(An),d(A3,Xi)
		case 0366: // MOVE.B d(An,Xi),d(A3,Xi)
			return src = rop8(op, true), write8(src, index(a3));
		case 0367: // MOVE.B Abs...,d(A3,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a3));
		case 0400: // MOVE.B Dn,D4
		case 0402: // MOVE.B (An),D4
		case 0403: // MOVE.B (An)+,D4
		case 0404: // MOVE.B -(An),D4
		case 0405: // MOVE.B d(An),D4
		case 0406: // MOVE.B d(An,Xi),D4
			return void(d4 = d4 & ~0xff | rop8(op, true));
		case 0407: // MOVE.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xff | rop8(op, true));
		case 0420: // MOVE.B Dn,(A4)
		case 0422: // MOVE.B (An),(A4)
		case 0423: // MOVE.B (An)+,(A4)
		case 0424: // MOVE.B -(An),(A4)
		case 0425: // MOVE.B d(An),(A4)
		case 0426: // MOVE.B d(An,Xi),(A4)
			return src = rop8(op, true), write8(src, a4);
		case 0427: // MOVE.B Abs...,(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a4);
		case 0430: // MOVE.B Dn,(A4)+
		case 0432: // MOVE.B (An),(A4)+
		case 0433: // MOVE.B (An)+,(A4)+
		case 0434: // MOVE.B -(An),(A4)+
		case 0435: // MOVE.B d(An),(A4)+
		case 0436: // MOVE.B d(An,Xi),(A4)+
			return src = rop8(op, true), write8(src, a4), void(a4 = a4 + 1);
		case 0437: // MOVE.B Abs...,(A4)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a4), void(a4 = a4 + 1);
		case 0440: // MOVE.B Dn,-(A4)
		case 0442: // MOVE.B (An),-(A4)
		case 0443: // MOVE.B (An)+,-(A4)
		case 0444: // MOVE.B -(An),-(A4)
		case 0445: // MOVE.B d(An),-(A4)
		case 0446: // MOVE.B d(An,Xi),-(A4)
			return src = rop8(op, true), a4 = a4 - 1, write8(src, a4);
		case 0447: // MOVE.B Abs...,-(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a4 = a4 - 1, write8(src, a4);
		case 0450: // MOVE.B Dn,d(A4)
		case 0452: // MOVE.B (An),d(A4)
		case 0453: // MOVE.B (An)+,d(A4)
		case 0454: // MOVE.B -(An),d(A4)
		case 0455: // MOVE.B d(An),d(A4)
		case 0456: // MOVE.B d(An,Xi),d(A4)
			return src = rop8(op, true), write8(src, disp(a4));
		case 0457: // MOVE.B Abs...,d(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a4));
		case 0460: // MOVE.B Dn,d(A4,Xi)
		case 0462: // MOVE.B (An),d(A4,Xi)
		case 0463: // MOVE.B (An)+,d(A4,Xi)
		case 0464: // MOVE.B -(An),d(A4,Xi)
		case 0465: // MOVE.B d(An),d(A4,Xi)
		case 0466: // MOVE.B d(An,Xi),d(A4,Xi)
			return src = rop8(op, true), write8(src, index(a4));
		case 0467: // MOVE.B Abs...,d(A4,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a4));
		case 0500: // MOVE.B Dn,D5
		case 0502: // MOVE.B (An),D5
		case 0503: // MOVE.B (An)+,D5
		case 0504: // MOVE.B -(An),D5
		case 0505: // MOVE.B d(An),D5
		case 0506: // MOVE.B d(An,Xi),D5
			return void(d5 = d5 & ~0xff | rop8(op, true));
		case 0507: // MOVE.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xff | rop8(op, true));
		case 0520: // MOVE.B Dn,(A5)
		case 0522: // MOVE.B (An),(A5)
		case 0523: // MOVE.B (An)+,(A5)
		case 0524: // MOVE.B -(An),(A5)
		case 0525: // MOVE.B d(An),(A5)
		case 0526: // MOVE.B d(An,Xi),(A5)
			return src = rop8(op, true), write8(src, a5);
		case 0527: // MOVE.B Abs...,(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a5);
		case 0530: // MOVE.B Dn,(A5)+
		case 0532: // MOVE.B (An),(A5)+
		case 0533: // MOVE.B (An)+,(A5)+
		case 0534: // MOVE.B -(An),(A5)+
		case 0535: // MOVE.B d(An),(A5)+
		case 0536: // MOVE.B d(An,Xi),(A5)+
			return src = rop8(op, true), write8(src, a5), void(a5 = a5 + 1);
		case 0537: // MOVE.B Abs...,(A5)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a5), void(a5 = a5 + 1);
		case 0540: // MOVE.B Dn,-(A5)
		case 0542: // MOVE.B (An),-(A5)
		case 0543: // MOVE.B (An)+,-(A5)
		case 0544: // MOVE.B -(An),-(A5)
		case 0545: // MOVE.B d(An),-(A5)
		case 0546: // MOVE.B d(An,Xi),-(A5)
			return src = rop8(op, true), a5 = a5 - 1, write8(src, a5);
		case 0547: // MOVE.B Abs...,-(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a5 = a5 - 1, write8(src, a5);
		case 0550: // MOVE.B Dn,d(A5)
		case 0552: // MOVE.B (An),d(A5)
		case 0553: // MOVE.B (An)+,d(A5)
		case 0554: // MOVE.B -(An),d(A5)
		case 0555: // MOVE.B d(An),d(A5)
		case 0556: // MOVE.B d(An,Xi),d(A5)
			return src = rop8(op, true), write8(src, disp(a5));
		case 0557: // MOVE.B Abs...,d(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a5));
		case 0560: // MOVE.B Dn,d(A5,Xi)
		case 0562: // MOVE.B (An),d(A5,Xi)
		case 0563: // MOVE.B (An)+,d(A5,Xi)
		case 0564: // MOVE.B -(An),d(A5,Xi)
		case 0565: // MOVE.B d(An),d(A5,Xi)
		case 0566: // MOVE.B d(An,Xi),d(A5,Xi)
			return src = rop8(op, true), write8(src, index(a5));
		case 0567: // MOVE.B Abs...,d(A5,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a5));
		case 0600: // MOVE.B Dn,D6
		case 0602: // MOVE.B (An),D6
		case 0603: // MOVE.B (An)+,D6
		case 0604: // MOVE.B -(An),D6
		case 0605: // MOVE.B d(An),D6
		case 0606: // MOVE.B d(An,Xi),D6
			return void(d6 = d6 & ~0xff | rop8(op, true));
		case 0607: // MOVE.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xff | rop8(op, true));
		case 0620: // MOVE.B Dn,(A6)
		case 0622: // MOVE.B (An),(A6)
		case 0623: // MOVE.B (An)+,(A6)
		case 0624: // MOVE.B -(An),(A6)
		case 0625: // MOVE.B d(An),(A6)
		case 0626: // MOVE.B d(An,Xi),(A6)
			return src = rop8(op, true), write8(src, a6);
		case 0627: // MOVE.B Abs...,(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a6);
		case 0630: // MOVE.B Dn,(A6)+
		case 0632: // MOVE.B (An),(A6)+
		case 0633: // MOVE.B (An)+,(A6)+
		case 0634: // MOVE.B -(An),(A6)+
		case 0635: // MOVE.B d(An),(A6)+
		case 0636: // MOVE.B d(An,Xi),(A6)+
			return src = rop8(op, true), write8(src, a6), void(a6 = a6 + 1);
		case 0637: // MOVE.B Abs...,(A6)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a6), void(a6 = a6 + 1);
		case 0640: // MOVE.B Dn,-(A6)
		case 0642: // MOVE.B (An),-(A6)
		case 0643: // MOVE.B (An)+,-(A6)
		case 0644: // MOVE.B -(An),-(A6)
		case 0645: // MOVE.B d(An),-(A6)
		case 0646: // MOVE.B d(An,Xi),-(A6)
			return src = rop8(op, true), a6 = a6 - 1, write8(src, a6);
		case 0647: // MOVE.B Abs...,-(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a6 = a6 - 1, write8(src, a6);
		case 0650: // MOVE.B Dn,d(A6)
		case 0652: // MOVE.B (An),d(A6)
		case 0653: // MOVE.B (An)+,d(A6)
		case 0654: // MOVE.B -(An),d(A6)
		case 0655: // MOVE.B d(An),d(A6)
		case 0656: // MOVE.B d(An,Xi),d(A6)
			return src = rop8(op, true), write8(src, disp(a6));
		case 0657: // MOVE.B Abs...,d(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a6));
		case 0660: // MOVE.B Dn,d(A6,Xi)
		case 0662: // MOVE.B (An),d(A6,Xi)
		case 0663: // MOVE.B (An)+,d(A6,Xi)
		case 0664: // MOVE.B -(An),d(A6,Xi)
		case 0665: // MOVE.B d(An),d(A6,Xi)
		case 0666: // MOVE.B d(An,Xi),d(A6,Xi)
			return src = rop8(op, true), write8(src, index(a6));
		case 0667: // MOVE.B Abs...,d(A6,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a6));
		case 0700: // MOVE.B Dn,D7
		case 0702: // MOVE.B (An),D7
		case 0703: // MOVE.B (An)+,D7
		case 0704: // MOVE.B -(An),D7
		case 0705: // MOVE.B d(An),D7
		case 0706: // MOVE.B d(An,Xi),D7
			return void(d7 = d7 & ~0xff | rop8(op, true));
		case 0707: // MOVE.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xff | rop8(op, true));
		case 0720: // MOVE.B Dn,(A7)
		case 0722: // MOVE.B (An),(A7)
		case 0723: // MOVE.B (An)+,(A7)
		case 0724: // MOVE.B -(An),(A7)
		case 0725: // MOVE.B d(An),(A7)
		case 0726: // MOVE.B d(An,Xi),(A7)
			return src = rop8(op, true), write8(src, a7);
		case 0727: // MOVE.B Abs...,(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a7);
		case 0730: // MOVE.B Dn,(A7)+
		case 0732: // MOVE.B (An),(A7)+
		case 0733: // MOVE.B (An)+,(A7)+
		case 0734: // MOVE.B -(An),(A7)+
		case 0735: // MOVE.B d(An),(A7)+
		case 0736: // MOVE.B d(An,Xi),(A7)+
			return src = rop8(op, true), write8(src, a7), void(a7 = a7 + 1);
		case 0737: // MOVE.B Abs...,(A7)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, a7), void(a7 = a7 + 1);
		case 0740: // MOVE.B Dn,-(A7)
		case 0742: // MOVE.B (An),-(A7)
		case 0743: // MOVE.B (An)+,-(A7)
		case 0744: // MOVE.B -(An),-(A7)
		case 0745: // MOVE.B d(An),-(A7)
		case 0746: // MOVE.B d(An,Xi),-(A7)
			return src = rop8(op, true), a7 = a7 - 1, write8(src, a7);
		case 0747: // MOVE.B Abs...,-(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), a7 = a7 - 1, write8(src, a7);
		case 0750: // MOVE.B Dn,d(A7)
		case 0752: // MOVE.B (An),d(A7)
		case 0753: // MOVE.B (An)+,d(A7)
		case 0754: // MOVE.B -(An),d(A7)
		case 0755: // MOVE.B d(An),d(A7)
		case 0756: // MOVE.B d(An,Xi),d(A7)
			return src = rop8(op, true), write8(src, disp(a7));
		case 0757: // MOVE.B Abs...,d(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, disp(a7));
		case 0760: // MOVE.B Dn,d(A7,Xi)
		case 0762: // MOVE.B (An),d(A7,Xi)
		case 0763: // MOVE.B (An)+,d(A7,Xi)
		case 0764: // MOVE.B -(An),d(A7,Xi)
		case 0765: // MOVE.B d(An),d(A7,Xi)
		case 0766: // MOVE.B d(An,Xi),d(A7,Xi)
			return src = rop8(op, true), write8(src, index(a7));
		case 0767: // MOVE.B Abs...,d(A7,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop8(op, true), write8(src, index(a7));
		default:
			return exception(4);
		}
	}

	void execute_2(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // MOVE.L Dn,D0
		case 0001: // MOVE.L An,D0
		case 0002: // MOVE.L (An),D0
		case 0003: // MOVE.L (An)+,D0
		case 0004: // MOVE.L -(An),D0
		case 0005: // MOVE.L d(An),D0
		case 0006: // MOVE.L d(An,Xi),D0
			return void(d0 = rop32(op, true));
		case 0007: // MOVE.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = rop32(op, true));
		case 0010: // MOVEA.L Dn,A0
		case 0011: // MOVEA.L An,A0
		case 0012: // MOVEA.L (An),A0
		case 0013: // MOVEA.L (An)+,A0
		case 0014: // MOVEA.L -(An),A0
		case 0015: // MOVEA.L d(An),A0
		case 0016: // MOVEA.L d(An,Xi),A0
			return void(a0 = rop32(op));
		case 0017: // MOVEA.L Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = rop32(op));
		case 0020: // MOVE.L Dn,(A0)
		case 0021: // MOVE.L An,(A0)
		case 0022: // MOVE.L (An),(A0)
		case 0023: // MOVE.L (An)+,(A0)
		case 0024: // MOVE.L -(An),(A0)
		case 0025: // MOVE.L d(An),(A0)
		case 0026: // MOVE.L d(An,Xi),(A0)
			return src = rop32(op, true), write32(src, a0);
		case 0027: // MOVE.L Abs...,(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a0);
		case 0030: // MOVE.L Dn,(A0)+
		case 0031: // MOVE.L An,(A0)+
		case 0032: // MOVE.L (An),(A0)+
		case 0033: // MOVE.L (An)+,(A0)+
		case 0034: // MOVE.L -(An),(A0)+
		case 0035: // MOVE.L d(An),(A0)+
		case 0036: // MOVE.L d(An,Xi),(A0)+
			return src = rop32(op, true), write32(src, a0), void(a0 = a0 + 4);
		case 0037: // MOVE.L Abs...,(A0)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a0), void(a0 = a0 + 4);
		case 0040: // MOVE.L Dn,-(A0)
		case 0041: // MOVE.L An,-(A0)
		case 0042: // MOVE.L (An),-(A0)
		case 0043: // MOVE.L (An)+,-(A0)
		case 0044: // MOVE.L -(An),-(A0)
		case 0045: // MOVE.L d(An),-(A0)
		case 0046: // MOVE.L d(An,Xi),-(A0)
			return src = rop32(op, true), a0 = a0 - 4, write32(src, a0);
		case 0047: // MOVE.L Abs...,-(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a0 = a0 - 4, write32(src, a0);
		case 0050: // MOVE.L Dn,d(A0)
		case 0051: // MOVE.L An,d(A0)
		case 0052: // MOVE.L (An),d(A0)
		case 0053: // MOVE.L (An)+,d(A0)
		case 0054: // MOVE.L -(An),d(A0)
		case 0055: // MOVE.L d(An),d(A0)
		case 0056: // MOVE.L d(An,Xi),d(A0)
			return src = rop32(op, true), write32(src, disp(a0));
		case 0057: // MOVE.L Abs...,d(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a0));
		case 0060: // MOVE.L Dn,d(A0,Xi)
		case 0061: // MOVE.L An,d(A0,Xi)
		case 0062: // MOVE.L (An),d(A0,Xi)
		case 0063: // MOVE.L (An)+,d(A0,Xi)
		case 0064: // MOVE.L -(An),d(A0,Xi)
		case 0065: // MOVE.L d(An),d(A0,Xi)
		case 0066: // MOVE.L d(An,Xi),d(A0,Xi)
			return src = rop32(op, true), write32(src, index(a0));
		case 0067: // MOVE.L Abs...,d(A0,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a0));
		case 0070: // MOVE.L Dn,Abs.W
		case 0071: // MOVE.L An,Abs.W
		case 0072: // MOVE.L (An),Abs.W
		case 0073: // MOVE.L (An)+,Abs.W
		case 0074: // MOVE.L -(An),Abs.W
		case 0075: // MOVE.L d(An),Abs.W
		case 0076: // MOVE.L d(An,Xi),Abs.W
			return src = rop32(op, true), write32(src, fetch16s());
		case 0077: // MOVE.L Abs...,Abs.W
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, fetch16s());
		case 0100: // MOVE.L Dn,D1
		case 0101: // MOVE.L An,D1
		case 0102: // MOVE.L (An),D1
		case 0103: // MOVE.L (An)+,D1
		case 0104: // MOVE.L -(An),D1
		case 0105: // MOVE.L d(An),D1
		case 0106: // MOVE.L d(An,Xi),D1
			return void(d1 = rop32(op, true));
		case 0107: // MOVE.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = rop32(op, true));
		case 0110: // MOVEA.L Dn,A1
		case 0111: // MOVEA.L An,A1
		case 0112: // MOVEA.L (An),A1
		case 0113: // MOVEA.L (An)+,A1
		case 0114: // MOVEA.L -(An),A1
		case 0115: // MOVEA.L d(An),A1
		case 0116: // MOVEA.L d(An,Xi),A1
			return void(a1 = rop32(op));
		case 0117: // MOVEA.L Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = rop32(op));
		case 0120: // MOVE.L Dn,(A1)
		case 0121: // MOVE.L An,(A1)
		case 0122: // MOVE.L (An),(A1)
		case 0123: // MOVE.L (An)+,(A1)
		case 0124: // MOVE.L -(An),(A1)
		case 0125: // MOVE.L d(An),(A1)
		case 0126: // MOVE.L d(An,Xi),(A1)
			return src = rop32(op, true), write32(src, a1);
		case 0127: // MOVE.L Abs...,(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a1);
		case 0130: // MOVE.L Dn,(A1)+
		case 0131: // MOVE.L An,(A1)+
		case 0132: // MOVE.L (An),(A1)+
		case 0133: // MOVE.L (An)+,(A1)+
		case 0134: // MOVE.L -(An),(A1)+
		case 0135: // MOVE.L d(An),(A1)+
		case 0136: // MOVE.L d(An,Xi),(A1)+
			return src = rop32(op, true), write32(src, a1), void(a1 = a1 + 4);
		case 0137: // MOVE.L Abs...,(A1)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a1), void(a1 = a1 + 4);
		case 0140: // MOVE.L Dn,-(A1)
		case 0141: // MOVE.L An,-(A1)
		case 0142: // MOVE.L (An),-(A1)
		case 0143: // MOVE.L (An)+,-(A1)
		case 0144: // MOVE.L -(An),-(A1)
		case 0145: // MOVE.L d(An),-(A1)
		case 0146: // MOVE.L d(An,Xi),-(A1)
			return src = rop32(op, true), a1 = a1 - 4, write32(src, a1);
		case 0147: // MOVE.L Abs...,-(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a1 = a1 - 4, write32(src, a1);
		case 0150: // MOVE.L Dn,d(A1)
		case 0151: // MOVE.L An,d(A1)
		case 0152: // MOVE.L (An),d(A1)
		case 0153: // MOVE.L (An)+,d(A1)
		case 0154: // MOVE.L -(An),d(A1)
		case 0155: // MOVE.L d(An),d(A1)
		case 0156: // MOVE.L d(An,Xi),d(A1)
			return src = rop32(op, true), write32(src, disp(a1));
		case 0157: // MOVE.L Abs...,d(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a1));
		case 0160: // MOVE.L Dn,d(A1,Xi)
		case 0161: // MOVE.L An,d(A1,Xi)
		case 0162: // MOVE.L (An),d(A1,Xi)
		case 0163: // MOVE.L (An)+,d(A1,Xi)
		case 0164: // MOVE.L -(An),d(A1,Xi)
		case 0165: // MOVE.L d(An),d(A1,Xi)
		case 0166: // MOVE.L d(An,Xi),d(A1,Xi)
			return src = rop32(op, true), write32(src, index(a1));
		case 0167: // MOVE.L Abs...,d(A1,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a1));
		case 0170: // MOVE.L Dn,Abs.L
		case 0171: // MOVE.L An,Abs.L
		case 0172: // MOVE.L (An),Abs.L
		case 0173: // MOVE.L (An)+,Abs.L
		case 0174: // MOVE.L -(An),Abs.L
		case 0175: // MOVE.L d(An),Abs.L
		case 0176: // MOVE.L d(An,Xi),Abs.L
			return src = rop32(op, true), write32(src, fetch32());
		case 0177: // MOVE.L Abs...,Abs.L
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, fetch32());
		case 0200: // MOVE.L Dn,D2
		case 0201: // MOVE.L An,D2
		case 0202: // MOVE.L (An),D2
		case 0203: // MOVE.L (An)+,D2
		case 0204: // MOVE.L -(An),D2
		case 0205: // MOVE.L d(An),D2
		case 0206: // MOVE.L d(An,Xi),D2
			return void(d2 = rop32(op, true));
		case 0207: // MOVE.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = rop32(op, true));
		case 0210: // MOVEA.L Dn,A2
		case 0211: // MOVEA.L An,A2
		case 0212: // MOVEA.L (An),A2
		case 0213: // MOVEA.L (An)+,A2
		case 0214: // MOVEA.L -(An),A2
		case 0215: // MOVEA.L d(An),A2
		case 0216: // MOVEA.L d(An,Xi),A2
			return void(a2 = rop32(op));
		case 0217: // MOVEA.L Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = rop32(op));
		case 0220: // MOVE.L Dn,(A2)
		case 0221: // MOVE.L An,(A2)
		case 0222: // MOVE.L (An),(A2)
		case 0223: // MOVE.L (An)+,(A2)
		case 0224: // MOVE.L -(An),(A2)
		case 0225: // MOVE.L d(An),(A2)
		case 0226: // MOVE.L d(An,Xi),(A2)
			return src = rop32(op, true), write32(src, a2);
		case 0227: // MOVE.L Abs...,(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a2);
		case 0230: // MOVE.L Dn,(A2)+
		case 0231: // MOVE.L An,(A2)+
		case 0232: // MOVE.L (An),(A2)+
		case 0233: // MOVE.L (An)+,(A2)+
		case 0234: // MOVE.L -(An),(A2)+
		case 0235: // MOVE.L d(An),(A2)+
		case 0236: // MOVE.L d(An,Xi),(A2)+
			return src = rop32(op, true), write32(src, a2), void(a2 = a2 + 4);
		case 0237: // MOVE.L Abs...,(A2)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a2), void(a2 = a2 + 4);
		case 0240: // MOVE.L Dn,-(A2)
		case 0241: // MOVE.L An,-(A2)
		case 0242: // MOVE.L (An),-(A2)
		case 0243: // MOVE.L (An)+,-(A2)
		case 0244: // MOVE.L -(An),-(A2)
		case 0245: // MOVE.L d(An),-(A2)
		case 0246: // MOVE.L d(An,Xi),-(A2)
			return src = rop32(op, true), a2 = a2 - 4, write32(src, a2);
		case 0247: // MOVE.L Abs...,-(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a2 = a2 - 4, write32(src, a2);
		case 0250: // MOVE.L Dn,d(A2)
		case 0251: // MOVE.L An,d(A2)
		case 0252: // MOVE.L (An),d(A2)
		case 0253: // MOVE.L (An)+,d(A2)
		case 0254: // MOVE.L -(An),d(A2)
		case 0255: // MOVE.L d(An),d(A2)
		case 0256: // MOVE.L d(An,Xi),d(A2)
			return src = rop32(op, true), write32(src, disp(a2));
		case 0257: // MOVE.L Abs...,d(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a2));
		case 0260: // MOVE.L Dn,d(A2,Xi)
		case 0261: // MOVE.L An,d(A2,Xi)
		case 0262: // MOVE.L (An),d(A2,Xi)
		case 0263: // MOVE.L (An)+,d(A2,Xi)
		case 0264: // MOVE.L -(An),d(A2,Xi)
		case 0265: // MOVE.L d(An),d(A2,Xi)
		case 0266: // MOVE.L d(An,Xi),d(A2,Xi)
			return src = rop32(op, true), write32(src, index(a2));
		case 0267: // MOVE.L Abs...,d(A2,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a2));
		case 0300: // MOVE.L Dn,D3
		case 0301: // MOVE.L An,D3
		case 0302: // MOVE.L (An),D3
		case 0303: // MOVE.L (An)+,D3
		case 0304: // MOVE.L -(An),D3
		case 0305: // MOVE.L d(An),D3
		case 0306: // MOVE.L d(An,Xi),D3
			return void(d3 = rop32(op, true));
		case 0307: // MOVE.L Abs..,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = rop32(op, true));
		case 0310: // MOVEA.L Dn,A3
		case 0311: // MOVEA.L An,A3
		case 0312: // MOVEA.L (An),A3
		case 0313: // MOVEA.L (An)+,A3
		case 0314: // MOVEA.L -(An),A3
		case 0315: // MOVEA.L d(An),A3
		case 0316: // MOVEA.L d(An,Xi),A3
			return void(a3 = rop32(op));
		case 0317: // MOVEA.L Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = rop32(op));
		case 0320: // MOVE.L Dn,(A3)
		case 0321: // MOVE.L An,(A3)
		case 0322: // MOVE.L (An),(A3)
		case 0323: // MOVE.L (An)+,(A3)
		case 0324: // MOVE.L -(An),(A3)
		case 0325: // MOVE.L d(An),(A3)
		case 0326: // MOVE.L d(An,Xi),(A3)
			return src = rop32(op, true), write32(src, a3);
		case 0327: // MOVE.L Abs...,(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a3);
		case 0330: // MOVE.L Dn,(A3)+
		case 0331: // MOVE.L An,(A3)+
		case 0332: // MOVE.L (An),(A3)+
		case 0333: // MOVE.L (An)+,(A3)+
		case 0334: // MOVE.L -(An),(A3)+
		case 0335: // MOVE.L d(An),(A3)+
		case 0336: // MOVE.L d(An,Xi),(A3)+
			return src = rop32(op, true), write32(src, a3), void(a3 = a3 + 4);
		case 0337: // MOVE.L Abs...,(A3)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a3), void(a3 = a3 + 4);
		case 0340: // MOVE.L Dn,-(A3)
		case 0341: // MOVE.L An,-(A3)
		case 0342: // MOVE.L (An),-(A3)
		case 0343: // MOVE.L (An)+,-(A3)
		case 0344: // MOVE.L -(An),-(A3)
		case 0345: // MOVE.L d(An),-(A3)
		case 0346: // MOVE.L d(An,Xi),-(A3)
			return src = rop32(op, true), a3 = a3 - 4, write32(src, a3);
		case 0347: // MOVE.L Abs...,-(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a3 = a3 - 4, write32(src, a3);
		case 0350: // MOVE.L Dn,d(A3)
		case 0351: // MOVE.L An,d(A3)
		case 0352: // MOVE.L (An),d(A3)
		case 0353: // MOVE.L (An)+,d(A3)
		case 0354: // MOVE.L -(An),d(A3)
		case 0355: // MOVE.L d(An),d(A3)
		case 0356: // MOVE.L d(An,Xi),d(A3)
			return src = rop32(op, true), write32(src, disp(a3));
		case 0357: // MOVE.L Abs...,d(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a3));
		case 0360: // MOVE.L Dn,d(A3,Xi)
		case 0361: // MOVE.L An,d(A3,Xi)
		case 0362: // MOVE.L (An),d(A3,Xi)
		case 0363: // MOVE.L (An)+,d(A3,Xi)
		case 0364: // MOVE.L -(An),d(A3,Xi)
		case 0365: // MOVE.L d(An),d(A3,Xi)
		case 0366: // MOVE.L d(An,Xi),d(A3,Xi)
			return src = rop32(op, true), write32(src, index(a3));
		case 0367: // MOVE.L Abs...,d(A3,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a3));
		case 0400: // MOVE.L Dn,D4
		case 0401: // MOVE.L An,D4
		case 0402: // MOVE.L (An),D4
		case 0403: // MOVE.L (An)+,D4
		case 0404: // MOVE.L -(An),D4
		case 0405: // MOVE.L d(An),D4
		case 0406: // MOVE.L d(An,Xi),D4
			return void(d4 = rop32(op, true));
		case 0407: // MOVE.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = rop32(op, true));
		case 0410: // MOVEA.L Dn,A4
		case 0411: // MOVEA.L An,A4
		case 0412: // MOVEA.L (An),A4
		case 0413: // MOVEA.L (An)+,A4
		case 0414: // MOVEA.L -(An),A4
		case 0415: // MOVEA.L d(An),A4
		case 0416: // MOVEA.L d(An,Xi),A4
			return void(a4 = rop32(op));
		case 0417: // MOVEA.L Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = rop32(op));
		case 0420: // MOVE.L Dn,(A4)
		case 0421: // MOVE.L An,(A4)
		case 0422: // MOVE.L (An),(A4)
		case 0423: // MOVE.L (An)+,(A4)
		case 0424: // MOVE.L -(An),(A4)
		case 0425: // MOVE.L d(An),(A4)
		case 0426: // MOVE.L d(An,Xi),(A4)
			return src = rop32(op, true), write32(src, a4);
		case 0427: // MOVE.L Abs...,(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a4);
		case 0430: // MOVE.L Dn,(A4)+
		case 0431: // MOVE.L An,(A4)+
		case 0432: // MOVE.L (An),(A4)+
		case 0433: // MOVE.L (An)+,(A4)+
		case 0434: // MOVE.L -(An),(A4)+
		case 0435: // MOVE.L d(An),(A4)+
		case 0436: // MOVE.L d(An,Xi),(A4)+
			return src = rop32(op, true), write32(src, a4), void(a4 = a4 + 4);
		case 0437: // MOVE.L Abs...,(A4)+
			if ((op & 7) >= 5)
				exception(4);
			return src = rop32(op, true), write32(src, a4), void(a4 = a4 + 4);
		case 0440: // MOVE.L Dn,-(A4)
		case 0441: // MOVE.L An,-(A4)
		case 0442: // MOVE.L (An),-(A4)
		case 0443: // MOVE.L (An)+,-(A4)
		case 0444: // MOVE.L -(An),-(A4)
		case 0445: // MOVE.L d(An),-(A4)
		case 0446: // MOVE.L d(An,Xi),-(A4)
			return src = rop32(op, true), a4 = a4 - 4, write32(src, a4);
		case 0447: // MOVE.L Abs...,-(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a4 = a4 - 4, write32(src, a4);
		case 0450: // MOVE.L Dn,d(A4)
		case 0451: // MOVE.L An,d(A4)
		case 0452: // MOVE.L (An),d(A4)
		case 0453: // MOVE.L (An)+,d(A4)
		case 0454: // MOVE.L -(An),d(A4)
		case 0455: // MOVE.L d(An),d(A4)
		case 0456: // MOVE.L d(An,Xi),d(A4)
			return src = rop32(op, true), write32(src, disp(a4));
		case 0457: // MOVE.L Abs...,d(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a4));
		case 0460: // MOVE.L Dn,d(A4,Xi)
		case 0461: // MOVE.L An,d(A4,Xi)
		case 0462: // MOVE.L (An),d(A4,Xi)
		case 0463: // MOVE.L (An)+,d(A4,Xi)
		case 0464: // MOVE.L -(An),d(A4,Xi)
		case 0465: // MOVE.L d(An),d(A4,Xi)
		case 0466: // MOVE.L d(An,Xi),d(A4,Xi)
			return src = rop32(op, true), write32(src, index(a4));
		case 0467: // MOVE.L Abs..W,d(A4,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a4));
		case 0500: // MOVE.L Dn,D5
		case 0501: // MOVE.L An,D5
		case 0502: // MOVE.L (An),D5
		case 0503: // MOVE.L (An)+,D5
		case 0504: // MOVE.L -(An),D5
		case 0505: // MOVE.L d(An),D5
		case 0506: // MOVE.L d(An,Xi),D5
			return void(d5 = rop32(op, true));
		case 0507: // MOVE.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = rop32(op, true));
		case 0510: // MOVEA.L Dn,A5
		case 0511: // MOVEA.L An,A5
		case 0512: // MOVEA.L (An),A5
		case 0513: // MOVEA.L (An)+,A5
		case 0514: // MOVEA.L -(An),A5
		case 0515: // MOVEA.L d(An),A5
		case 0516: // MOVEA.L d(An,Xi),A5
			return void(a5 = rop32(op));
		case 0517: // MOVEA.L Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = rop32(op));
		case 0520: // MOVE.L Dn,(A5)
		case 0521: // MOVE.L An,(A5)
		case 0522: // MOVE.L (An),(A5)
		case 0523: // MOVE.L (An)+,(A5)
		case 0524: // MOVE.L -(An),(A5)
		case 0525: // MOVE.L d(An),(A5)
		case 0526: // MOVE.L d(An,Xi),(A5)
			return src = rop32(op, true), write32(src, a5);
		case 0527: // MOVE.L Abs...,(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a5);
		case 0530: // MOVE.L Dn,(A5)+
		case 0531: // MOVE.L An,(A5)+
		case 0532: // MOVE.L (An),(A5)+
		case 0533: // MOVE.L (An)+,(A5)+
		case 0534: // MOVE.L -(An),(A5)+
		case 0535: // MOVE.L d(An),(A5)+
		case 0536: // MOVE.L d(An,Xi),(A5)+
			return src = rop32(op, true), write32(src, a5), void(a5 = a5 + 4);
		case 0537: // MOVE.L Abs...,(A5)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a5), void(a5 = a5 + 4);
		case 0540: // MOVE.L Dn,-(A5)
		case 0541: // MOVE.L An,-(A5)
		case 0542: // MOVE.L (An),-(A5)
		case 0543: // MOVE.L (An)+,-(A5)
		case 0544: // MOVE.L -(An),-(A5)
		case 0545: // MOVE.L d(An),-(A5)
		case 0546: // MOVE.L d(An,Xi),-(A5)
			return src = rop32(op, true), a5 = a5 - 4, write32(src, a5);
		case 0547: // MOVE.L Abs...,-(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a5 = a5 - 4, write32(src, a5);
		case 0550: // MOVE.L Dn,d(A5)
		case 0551: // MOVE.L An,d(A5)
		case 0552: // MOVE.L (An),d(A5)
		case 0553: // MOVE.L (An)+,d(A5)
		case 0554: // MOVE.L -(An),d(A5)
		case 0555: // MOVE.L d(An),d(A5)
		case 0556: // MOVE.L d(An,Xi),d(A5)
			return src = rop32(op, true), write32(src, disp(a5));
		case 0557: // MOVE.L Abs...,d(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a5));
		case 0560: // MOVE.L Dn,d(A5,Xi)
		case 0561: // MOVE.L An,d(A5,Xi)
		case 0562: // MOVE.L (An),d(A5,Xi)
		case 0563: // MOVE.L (An)+,d(A5,Xi)
		case 0564: // MOVE.L -(An),d(A5,Xi)
		case 0565: // MOVE.L d(An),d(A5,Xi)
		case 0566: // MOVE.L d(An,Xi),d(A5,Xi)
			return src = rop32(op, true), write32(src, index(a5));
		case 0567: // MOVE.L Abs...,d(A5,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a5));
		case 0600: // MOVE.L Dn,D6
		case 0601: // MOVE.L An,D6
		case 0602: // MOVE.L (An),D6
		case 0603: // MOVE.L (An)+,D6
		case 0604: // MOVE.L -(An),D6
		case 0605: // MOVE.L d(An),D6
		case 0606: // MOVE.L d(An,Xi),D6
			return void(d6 = rop32(op, true));
		case 0607: // MOVE.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = rop32(op, true));
		case 0610: // MOVEA.L Dn,A6
		case 0611: // MOVEA.L An,A6
		case 0612: // MOVEA.L (An),A6
		case 0613: // MOVEA.L (An)+,A6
		case 0614: // MOVEA.L -(An),A6
		case 0615: // MOVEA.L d(An),A6
		case 0616: // MOVEA.L d(An,Xi),A6
			return void(a6 = rop32(op));
		case 0617: // MOVEA.L Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = rop32(op));
		case 0620: // MOVE.L Dn,(A6)
		case 0621: // MOVE.L An,(A6)
		case 0622: // MOVE.L (An),(A6)
		case 0623: // MOVE.L (An)+,(A6)
		case 0624: // MOVE.L -(An),(A6)
		case 0625: // MOVE.L d(An),(A6)
		case 0626: // MOVE.L d(An,Xi),(A6)
			return src = rop32(op, true), write32(src, a6);
		case 0627: // MOVE.L Abs...,(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a6);
		case 0630: // MOVE.L Dn,(A6)+
		case 0631: // MOVE.L An,(A6)+
		case 0632: // MOVE.L (An),(A6)+
		case 0633: // MOVE.L (An)+,(A6)+
		case 0634: // MOVE.L -(An),(A6)+
		case 0635: // MOVE.L d(An),(A6)+
		case 0636: // MOVE.L d(An,Xi),(A6)+
			return src = rop32(op, true), write32(src, a6), void(a6 = a6 + 4);
		case 0637: // MOVE.L Abs...,(A6)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a6), void(a6 = a6 + 4);
		case 0640: // MOVE.L Dn,-(A6)
		case 0641: // MOVE.L An,-(A6)
		case 0642: // MOVE.L (An),-(A6)
		case 0643: // MOVE.L (An)+,-(A6)
		case 0644: // MOVE.L -(An),-(A6)
		case 0645: // MOVE.L d(An),-(A6)
		case 0646: // MOVE.L d(An,Xi),-(A6)
			return src = rop32(op, true), a6 = a6 - 4, write32(src, a6);
		case 0647: // MOVE.L Abs...,-(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a6 = a6 - 4, write32(src, a6);
		case 0650: // MOVE.L Dn,d(A6)
		case 0651: // MOVE.L An,d(A6)
		case 0652: // MOVE.L (An),d(A6)
		case 0653: // MOVE.L (An)+,d(A6)
		case 0654: // MOVE.L -(An),d(A6)
		case 0655: // MOVE.L d(An),d(A6)
		case 0656: // MOVE.L d(An,Xi),d(A6)
			return src = rop32(op, true), write32(src, disp(a6));
		case 0657: // MOVE.L Abs...,d(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a6));
		case 0660: // MOVE.L Dn,d(A6,Xi)
		case 0661: // MOVE.L An,d(A6,Xi)
		case 0662: // MOVE.L (An),d(A6,Xi)
		case 0663: // MOVE.L (An)+,d(A6,Xi)
		case 0664: // MOVE.L -(An),d(A6,Xi)
		case 0665: // MOVE.L d(An),d(A6,Xi)
		case 0666: // MOVE.L d(An,Xi),d(A6,Xi)
			return src = rop32(op, true), write32(src, index(a6));
		case 0667: // MOVE.L Abs...,d(A6,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a6));
		case 0700: // MOVE.L Dn,D7
		case 0701: // MOVE.L An,D7
		case 0702: // MOVE.L (An),D7
		case 0703: // MOVE.L (An)+,D7
		case 0704: // MOVE.L -(An),D7
		case 0705: // MOVE.L d(An),D7
		case 0706: // MOVE.L d(An,Xi),D7
			return void(d7 = rop32(op, true));
		case 0707: // MOVE.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = rop32(op, true));
		case 0710: // MOVEA.L Dn,A7
		case 0711: // MOVEA.L An,A7
		case 0712: // MOVEA.L (An),A7
		case 0713: // MOVEA.L (An)+,A7
		case 0714: // MOVEA.L -(An),A7
		case 0715: // MOVEA.L d(An),A7
		case 0716: // MOVEA.L d(An,Xi),A7
			return void(a7 = rop32(op));
		case 0717: // MOVEA.L Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = rop32(op));
		case 0720: // MOVE.L Dn,(A7)
		case 0721: // MOVE.L An,(A7)
		case 0722: // MOVE.L (An),(A7)
		case 0723: // MOVE.L (An)+,(A7)
		case 0724: // MOVE.L -(An),(A7)
		case 0725: // MOVE.L d(An),(A7)
		case 0726: // MOVE.L d(An,Xi),(A7)
			return src = rop32(op, true), write32(src, a7);
		case 0727: // MOVE.L Abs...,(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a7);
		case 0730: // MOVE.L Dn,(A7)+
		case 0731: // MOVE.L An,(A7)+
		case 0732: // MOVE.L (An),(A7)+
		case 0733: // MOVE.L (An)+,(A7)+
		case 0734: // MOVE.L -(An),(A7)+
		case 0735: // MOVE.L d(An),(A7)+
		case 0736: // MOVE.L d(An,Xi),(A7)+
			return src = rop32(op, true), write32(src, a7), void(a7 = a7 + 4);
		case 0737: // MOVE.L Abs...,(A7)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, a7), void(a7 = a7 + 4);
		case 0740: // MOVE.L Dn,-(A7)
		case 0741: // MOVE.L An,-(A7)
		case 0742: // MOVE.L (An),-(A7)
		case 0743: // MOVE.L (An)+,-(A7)
		case 0744: // MOVE.L -(An),-(A7)
		case 0745: // MOVE.L d(An),-(A7)
		case 0746: // MOVE.L d(An,Xi),-(A7)
			return src = rop32(op, true), a7 = a7 - 4, write32(src, a7);
		case 0747: // MOVE.L Abs...,-(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), a7 = a7 - 4, write32(src, a7);
		case 0750: // MOVE.L Dn,d(A7)
		case 0751: // MOVE.L An,d(A7)
		case 0752: // MOVE.L (An),d(A7)
		case 0753: // MOVE.L (An)+,d(A7)
		case 0754: // MOVE.L -(An),d(A7)
		case 0755: // MOVE.L d(An),d(A7)
		case 0756: // MOVE.L d(An,Xi),d(A7)
			return src = rop32(op, true), write32(src, disp(a7));
		case 0757: // MOVE.L Abs...,d(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, disp(a7));
		case 0760: // MOVE.L Dn,d(A7,Xi)
		case 0761: // MOVE.L An,d(A7,Xi)
		case 0762: // MOVE.L (An),d(A7,Xi)
		case 0763: // MOVE.L (An)+,d(A7,Xi)
		case 0764: // MOVE.L -(An),d(A7,Xi)
		case 0765: // MOVE.L d(An),d(A7,Xi)
		case 0766: // MOVE.L d(An,Xi),d(A7,Xi)
			return src = rop32(op, true), write32(src, index(a7));
		case 0767: // MOVE.L Abs...,d(A7,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop32(op, true), write32(src, index(a7));
		default:
			return exception(4);
		}
	}

	void execute_3(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // MOVE.W Dn,D0
		case 0001: // MOVE.W An,D0
		case 0002: // MOVE.W (An),D0
		case 0003: // MOVE.W (An)+,D0
		case 0004: // MOVE.W -(An),D0
		case 0005: // MOVE.W d(An),D0
		case 0006: // MOVE.W d(An,Xi),D0
			return void(d0 = d0 & ~0xffff | rop16(op, true));
		case 0007: // MOVE.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xffff | rop16(op, true));
		case 0010: // MOVEA.W Dn,A0
		case 0011: // MOVEA.W An,A0
		case 0012: // MOVEA.W (An),A0
		case 0013: // MOVEA.W (An)+,A0
		case 0014: // MOVEA.W -(An),A0
		case 0015: // MOVEA.W d(An),A0
		case 0016: // MOVEA.W d(An,Xi),A0
			return void(a0 = rop16(op) << 16 >> 16);
		case 0017: // MOVEA.W Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = rop16(op) << 16 >> 16);
		case 0020: // MOVE.W Dn,(A0)
		case 0021: // MOVE.W An,(A0)
		case 0022: // MOVE.W (An),(A0)
		case 0023: // MOVE.W (An)+,(A0)
		case 0024: // MOVE.W -(An),(A0)
		case 0025: // MOVE.W d(An),(A0)
		case 0026: // MOVE.W d(An,Xi),(A0)
			return src = rop16(op, true), write16(src, a0);
		case 0027: // MOVE.W Abs...,(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a0);
		case 0030: // MOVE.W Dn,(A0)+
		case 0031: // MOVE.W An,(A0)+
		case 0032: // MOVE.W (An),(A0)+
		case 0033: // MOVE.W (An)+,(A0)+
		case 0034: // MOVE.W -(An),(A0)+
		case 0035: // MOVE.W d(An),(A0)+
		case 0036: // MOVE.W d(An,Xi),(A0)+
			return src = rop16(op, true), write16(src, a0), void(a0 = a0 + 2);
		case 0037: // MOVE.W Abs...,(A0)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a0), void(a0 = a0 + 2);
		case 0040: // MOVE.W Dn,-(A0)
		case 0041: // MOVE.W An,-(A0)
		case 0042: // MOVE.W (An),-(A0)
		case 0043: // MOVE.W (An)+,-(A0)
		case 0044: // MOVE.W -(An),-(A0)
		case 0045: // MOVE.W d(An),-(A0)
		case 0046: // MOVE.W d(An,Xi),-(A0)
			return src = rop16(op, true), a0 = a0 - 2, write16(src, a0);
		case 0047: // MOVE.W Abs...,-(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a0 = a0 - 2, write16(src, a0);
		case 0050: // MOVE.W Dn,d(A0)
		case 0051: // MOVE.W An,d(A0)
		case 0052: // MOVE.W (An),d(A0)
		case 0053: // MOVE.W (An)+,d(A0)
		case 0054: // MOVE.W -(An),d(A0)
		case 0055: // MOVE.W d(An),d(A0)
		case 0056: // MOVE.W d(An,Xi),d(A0)
			return src = rop16(op, true), write16(src, disp(a0));
		case 0057: // MOVE.W Abs...,d(A0)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a0));
		case 0060: // MOVE.W Dn,d(A0,Xi)
		case 0061: // MOVE.W An,d(A0,Xi)
		case 0062: // MOVE.W (An),d(A0,Xi)
		case 0063: // MOVE.W (An)+,d(A0,Xi)
		case 0064: // MOVE.W -(An),d(A0,Xi)
		case 0065: // MOVE.W d(An),d(A0,Xi)
		case 0066: // MOVE.W d(An,Xi),d(A0,Xi)
			return src = rop16(op, true), write16(src, index(a0));
		case 0067: // MOVE.W Abs...,d(A0,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a0));
		case 0070: // MOVE.W Dn,Abs.W
		case 0071: // MOVE.W An,Abs.W
		case 0072: // MOVE.W (An),Abs.W
		case 0073: // MOVE.W (An)+,Abs.W
		case 0074: // MOVE.W -(An),Abs.W
		case 0075: // MOVE.W d(An),Abs.W
		case 0076: // MOVE.W d(An,Xi),Abs.W
			return src = rop16(op, true), write16(src, fetch16s());
		case 0077: // MOVE.W Abs...,Abs.W
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, fetch16s());
		case 0100: // MOVE.W Dn,D1
		case 0101: // MOVE.W An,D1
		case 0102: // MOVE.W (An),D1
		case 0103: // MOVE.W (An)+,D1
		case 0104: // MOVE.W -(An),D1
		case 0105: // MOVE.W d(An),D1
		case 0106: // MOVE.W d(An,Xi),D1
			return void(d1 = d1 & ~0xffff | rop16(op, true));
		case 0107: // MOVE.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xffff | rop16(op, true));
		case 0110: // MOVEA.W Dn,A1
		case 0111: // MOVEA.W An,A1
		case 0112: // MOVEA.W (An),A1
		case 0113: // MOVEA.W (An)+,A1
		case 0114: // MOVEA.W -(An),A1
		case 0115: // MOVEA.W d(An),A1
		case 0116: // MOVEA.W d(An,Xi),A1
			return void(a1 = rop16(op) << 16 >> 16);
		case 0117: // MOVEA.W Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = rop16(op) << 16 >> 16);
		case 0120: // MOVE.W Dn,(A1)
		case 0121: // MOVE.W An,(A1)
		case 0122: // MOVE.W (An),(A1)
		case 0123: // MOVE.W (An)+,(A1)
		case 0124: // MOVE.W -(An),(A1)
		case 0125: // MOVE.W d(An),(A1)
		case 0126: // MOVE.W d(An,Xi),(A1)
			return src = rop16(op, true), write16(src, a1);
		case 0127: // MOVE.W Abs...,(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a1);
		case 0130: // MOVE.W Dn,(A1)+
		case 0131: // MOVE.W An,(A1)+
		case 0132: // MOVE.W (An),(A1)+
		case 0133: // MOVE.W (An)+,(A1)+
		case 0134: // MOVE.W -(An),(A1)+
		case 0135: // MOVE.W d(An),(A1)+
		case 0136: // MOVE.W d(An,Xi),(A1)+
			return src = rop16(op, true), write16(src, a1), void(a1 = a1 + 2);
		case 0137: // MOVE.W Abs...,(A1)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a1), void(a1 = a1 + 2);
		case 0140: // MOVE.W Dn,-(A1)
		case 0141: // MOVE.W An,-(A1)
		case 0142: // MOVE.W (An),-(A1)
		case 0143: // MOVE.W (An)+,-(A1)
		case 0144: // MOVE.W -(An),-(A1)
		case 0145: // MOVE.W d(An),-(A1)
		case 0146: // MOVE.W d(An,Xi),-(A1)
			return src = rop16(op, true), a1 = a1 - 2, write16(src, a1);
		case 0147: // MOVE.W Abs...,-(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a1 = a1 - 2, write16(src, a1);
		case 0150: // MOVE.W Dn,d(A1)
		case 0151: // MOVE.W An,d(A1)
		case 0152: // MOVE.W (An),d(A1)
		case 0153: // MOVE.W (An)+,d(A1)
		case 0154: // MOVE.W -(An),d(A1)
		case 0155: // MOVE.W d(An),d(A1)
		case 0156: // MOVE.W d(An,Xi),d(A1)
			return src = rop16(op, true), write16(src, disp(a1));
		case 0157: // MOVE.W Abs...,d(A1)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a1));
		case 0160: // MOVE.W Dn,d(A1,Xi)
		case 0161: // MOVE.W An,d(A1,Xi)
		case 0162: // MOVE.W (An),d(A1,Xi)
		case 0163: // MOVE.W (An)+,d(A1,Xi)
		case 0164: // MOVE.W -(An),d(A1,Xi)
		case 0165: // MOVE.W d(An),d(A1,Xi)
		case 0166: // MOVE.W d(An,Xi),d(A1,Xi)
			return src = rop16(op, true), write16(src, index(a1));
		case 0167: // MOVE.W Abs...,d(A1,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a1));
		case 0170: // MOVE.W Dn,Abs.L
		case 0171: // MOVE.W An,Abs.L
		case 0172: // MOVE.W (An),Abs.L
		case 0173: // MOVE.W (An)+,Abs.L
		case 0174: // MOVE.W -(An),Abs.L
		case 0175: // MOVE.W d(An),Abs.L
		case 0176: // MOVE.W d(An,Xi),Abs.L
			return src = rop16(op, true), write16(src, fetch32());
		case 0177: // MOVE.W Abs...,Abs.L
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, fetch32());
		case 0200: // MOVE.W Dn,D2
		case 0201: // MOVE.W An,D2
		case 0202: // MOVE.W (An),D2
		case 0203: // MOVE.W (An)+,D2
		case 0204: // MOVE.W -(An),D2
		case 0205: // MOVE.W d(An),D2
		case 0206: // MOVE.W d(An,Xi),D2
			return void(d2 = d2 & ~0xffff | rop16(op, true));
		case 0207: // MOVE.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xffff | rop16(op, true));
		case 0210: // MOVEA.W Dn,A2
		case 0211: // MOVEA.W An,A2
		case 0212: // MOVEA.W (An),A2
		case 0213: // MOVEA.W (An)+,A2
		case 0214: // MOVEA.W -(An),A2
		case 0215: // MOVEA.W d(An),A2
		case 0216: // MOVEA.W d(An,Xi),A2
			return void(a2 = rop16(op) << 16 >> 16);
		case 0217: // MOVEA.W Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = rop16(op) << 16 >> 16);
		case 0220: // MOVE.W Dn,(A2)
		case 0221: // MOVE.W An,(A2)
		case 0222: // MOVE.W (An),(A2)
		case 0223: // MOVE.W (An)+,(A2)
		case 0224: // MOVE.W -(An),(A2)
		case 0225: // MOVE.W d(An),(A2)
		case 0226: // MOVE.W d(An,Xi),(A2)
			return src = rop16(op, true), write16(src, a2);
		case 0227: // MOVE.W Abs...,(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a2);
		case 0230: // MOVE.W Dn,(A2)+
		case 0231: // MOVE.W An,(A2)+
		case 0232: // MOVE.W (An),(A2)+
		case 0233: // MOVE.W (An)+,(A2)+
		case 0234: // MOVE.W -(An),(A2)+
		case 0235: // MOVE.W d(An),(A2)+
		case 0236: // MOVE.W d(An,Xi),(A2)+
			return src = rop16(op, true), write16(src, a2), void(a2 = a2 + 2);
		case 0237: // MOVE.W Abs...,(A2)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a2), void(a2 = a2 + 2);
		case 0240: // MOVE.W Dn,-(A2)
		case 0241: // MOVE.W An,-(A2)
		case 0242: // MOVE.W (An),-(A2)
		case 0243: // MOVE.W (An)+,-(A2)
		case 0244: // MOVE.W -(An),-(A2)
		case 0245: // MOVE.W d(An),-(A2)
		case 0246: // MOVE.W d(An,Xi),-(A2)
			return src = rop16(op, true), a2 = a2 - 2, write16(src, a2);
		case 0247: // MOVE.W Abs...,-(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a2 = a2 - 2, write16(src, a2);
		case 0250: // MOVE.W Dn,d(A2)
		case 0251: // MOVE.W An,d(A2)
		case 0252: // MOVE.W (An),d(A2)
		case 0253: // MOVE.W (An)+,d(A2)
		case 0254: // MOVE.W -(An),d(A2)
		case 0255: // MOVE.W d(An),d(A2)
		case 0256: // MOVE.W d(An,Xi),d(A2)
			return src = rop16(op, true), write16(src, disp(a2));
		case 0257: // MOVE.W Abs...,d(A2)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a2));
		case 0260: // MOVE.W Dn,d(A2,Xi)
		case 0261: // MOVE.W An,d(A2,Xi)
		case 0262: // MOVE.W (An),d(A2,Xi)
		case 0263: // MOVE.W (An)+,d(A2,Xi)
		case 0264: // MOVE.W -(An),d(A2,Xi)
		case 0265: // MOVE.W d(An),d(A2,Xi)
		case 0266: // MOVE.W d(An,Xi),d(A2,Xi)
			return src = rop16(op, true), write16(src, index(a2));
		case 0267: // MOVE.W Abs...,d(A2,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a2));
		case 0300: // MOVE.W Dn,D3
		case 0301: // MOVE.W An,D3
		case 0302: // MOVE.W (An),D3
		case 0303: // MOVE.W (An)+,D3
		case 0304: // MOVE.W -(An),D3
		case 0305: // MOVE.W d(An),D3
		case 0306: // MOVE.W d(An,Xi),D3
			return void(d3 = d3 & ~0xffff | rop16(op, true));
		case 0307: // MOVE.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xffff | rop16(op, true));
		case 0310: // MOVEA.W Dn,A3
		case 0311: // MOVEA.W An,A3
		case 0312: // MOVEA.W (An),A3
		case 0313: // MOVEA.W (An)+,A3
		case 0314: // MOVEA.W -(An),A3
		case 0315: // MOVEA.W d(An),A3
		case 0316: // MOVEA.W d(An,Xi),A3
			return void(a3 = rop16(op) << 16 >> 16);
		case 0317: // MOVEA.W Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = rop16(op) << 16 >> 16);
		case 0320: // MOVE.W Dn,(A3)
		case 0321: // MOVE.W An,(A3)
		case 0322: // MOVE.W (An),(A3)
		case 0323: // MOVE.W (An)+,(A3)
		case 0324: // MOVE.W -(An),(A3)
		case 0325: // MOVE.W d(An),(A3)
		case 0326: // MOVE.W d(An,Xi),(A3)
			return src = rop16(op, true), write16(src, a3);
		case 0327: // MOVE.W Abs...,(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a3);
		case 0330: // MOVE.W Dn,(A3)+
		case 0331: // MOVE.W An,(A3)+
		case 0332: // MOVE.W (An),(A3)+
		case 0333: // MOVE.W (An)+,(A3)+
		case 0334: // MOVE.W -(An),(A3)+
		case 0335: // MOVE.W d(An),(A3)+
		case 0336: // MOVE.W d(An,Xi),(A3)+
			return src = rop16(op, true), write16(src, a3), void(a3 = a3 + 2);
		case 0337: // MOVE.W Abs...,(A3)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a3), void(a3 = a3 + 2);
		case 0340: // MOVE.W Dn,-(A3)
		case 0341: // MOVE.W An,-(A3)
		case 0342: // MOVE.W (An),-(A3)
		case 0343: // MOVE.W (An)+,-(A3)
		case 0344: // MOVE.W -(An),-(A3)
		case 0345: // MOVE.W d(An),-(A3)
		case 0346: // MOVE.W d(An,Xi),-(A3)
			return src = rop16(op, true), a3 = a3 - 2, write16(src, a3);
		case 0347: // MOVE.W Abs...,-(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a3 = a3 - 2, write16(src, a3);
		case 0350: // MOVE.W Dn,d(A3)
		case 0351: // MOVE.W An,d(A3)
		case 0352: // MOVE.W (An),d(A3)
		case 0353: // MOVE.W (An)+,d(A3)
		case 0354: // MOVE.W -(An),d(A3)
		case 0355: // MOVE.W d(An),d(A3)
		case 0356: // MOVE.W d(An,Xi),d(A3)
			return src = rop16(op, true), write16(src, disp(a3));
		case 0357: // MOVE.W Abs...,d(A3)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a3));
		case 0360: // MOVE.W Dn,d(A3,Xi)
		case 0361: // MOVE.W An,d(A3,Xi)
		case 0362: // MOVE.W (An),d(A3,Xi)
		case 0363: // MOVE.W (An)+,d(A3,Xi)
		case 0364: // MOVE.W -(An),d(A3,Xi)
		case 0365: // MOVE.W d(An),d(A3,Xi)
		case 0366: // MOVE.W d(An,Xi),d(A3,Xi)
			return src = rop16(op, true), write16(src, index(a3));
		case 0367: // MOVE.W Abs...,d(A3,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a3));
		case 0400: // MOVE.W Dn,D4
		case 0401: // MOVE.W An,D4
		case 0402: // MOVE.W (An),D4
		case 0403: // MOVE.W (An)+,D4
		case 0404: // MOVE.W -(An),D4
		case 0405: // MOVE.W d(An),D4
		case 0406: // MOVE.W d(An,Xi),D4
			return void(d4 = d4 & ~0xffff | rop16(op, true));
		case 0407: // MOVE.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xffff | rop16(op, true));
		case 0410: // MOVEA.W Dn,A4
		case 0411: // MOVEA.W An,A4
		case 0412: // MOVEA.W (An),A4
		case 0413: // MOVEA.W (An)+,A4
		case 0414: // MOVEA.W -(An),A4
		case 0415: // MOVEA.W d(An),A4
		case 0416: // MOVEA.W d(An,Xi),A4
			return void(a4 = rop16(op) << 16 >> 16);
		case 0417: // MOVEA.W Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = rop16(op) << 16 >> 16);
		case 0420: // MOVE.W Dn,(A4)
		case 0421: // MOVE.W An,(A4)
		case 0422: // MOVE.W (An),(A4)
		case 0423: // MOVE.W (An)+,(A4)
		case 0424: // MOVE.W -(An),(A4)
		case 0425: // MOVE.W d(An),(A4)
		case 0426: // MOVE.W d(An,Xi),(A4)
			return src = rop16(op, true), write16(src, a4);
		case 0427: // MOVE.W Abs...,(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a4);
		case 0430: // MOVE.W Dn,(A4)+
		case 0431: // MOVE.W An,(A4)+
		case 0432: // MOVE.W (An),(A4)+
		case 0433: // MOVE.W (An)+,(A4)+
		case 0434: // MOVE.W -(An),(A4)+
		case 0435: // MOVE.W d(An),(A4)+
		case 0436: // MOVE.W d(An,Xi),(A4)+
			return src = rop16(op, true), write16(src, a4), void(a4 = a4 + 2);
		case 0437: // MOVE.W Abs...,(A4)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a4), void(a4 = a4 + 2);
		case 0440: // MOVE.W Dn,-(A4)
		case 0441: // MOVE.W An,-(A4)
		case 0442: // MOVE.W (An),-(A4)
		case 0443: // MOVE.W (An)+,-(A4)
		case 0444: // MOVE.W -(An),-(A4)
		case 0445: // MOVE.W d(An),-(A4)
		case 0446: // MOVE.W d(An,Xi),-(A4)
			return src = rop16(op, true), a4 = a4 - 2, write16(src, a4);
		case 0447: // MOVE.W Abs...,-(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a4 = a4 - 2, write16(src, a4);
		case 0450: // MOVE.W Dn,d(A4)
		case 0451: // MOVE.W An,d(A4)
		case 0452: // MOVE.W (An),d(A4)
		case 0453: // MOVE.W (An)+,d(A4)
		case 0454: // MOVE.W -(An),d(A4)
		case 0455: // MOVE.W d(An),d(A4)
		case 0456: // MOVE.W d(An,Xi),d(A4)
			return src = rop16(op, true), write16(src, disp(a4));
		case 0457: // MOVE.W Abs...,d(A4)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a4));
		case 0460: // MOVE.W Dn,d(A4,Xi)
		case 0461: // MOVE.W An,d(A4,Xi)
		case 0462: // MOVE.W (An),d(A4,Xi)
		case 0463: // MOVE.W (An)+,d(A4,Xi)
		case 0464: // MOVE.W -(An),d(A4,Xi)
		case 0465: // MOVE.W d(An),d(A4,Xi)
		case 0466: // MOVE.W d(An,Xi),d(A4,Xi)
			return src = rop16(op, true), write16(src, index(a4));
		case 0467: // MOVE.W Abs...,d(A4,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a4));
		case 0500: // MOVE.W Dn,D5
		case 0501: // MOVE.W An,D5
		case 0502: // MOVE.W (An),D5
		case 0503: // MOVE.W (An)+,D5
		case 0504: // MOVE.W -(An),D5
		case 0505: // MOVE.W d(An),D5
		case 0506: // MOVE.W d(An,Xi),D5
			return void(d5 = d5 & ~0xffff | rop16(op, true));
		case 0507: // MOVE.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xffff | rop16(op, true));
		case 0510: // MOVEA.W Dn,A5
		case 0511: // MOVEA.W An,A5
		case 0512: // MOVEA.W (An),A5
		case 0513: // MOVEA.W (An)+,A5
		case 0514: // MOVEA.W -(An),A5
		case 0515: // MOVEA.W d(An),A5
		case 0516: // MOVEA.W d(An,Xi),A5
			return void(a5 = rop16(op) << 16 >> 16);
		case 0517: // MOVEA.W Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = rop16(op) << 16 >> 16);
		case 0520: // MOVE.W Dn,(A5)
		case 0521: // MOVE.W An,(A5)
		case 0522: // MOVE.W (An),(A5)
		case 0523: // MOVE.W (An)+,(A5)
		case 0524: // MOVE.W -(An),(A5)
		case 0525: // MOVE.W d(An),(A5)
		case 0526: // MOVE.W d(An,Xi),(A5)
			return src = rop16(op, true), write16(src, a5);
		case 0527: // MOVE.W Abs...,(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a5);
		case 0530: // MOVE.W Dn,(A5)+
		case 0531: // MOVE.W An,(A5)+
		case 0532: // MOVE.W (An),(A5)+
		case 0533: // MOVE.W (An)+,(A5)+
		case 0534: // MOVE.W -(An),(A5)+
		case 0535: // MOVE.W d(An),(A5)+
		case 0536: // MOVE.W d(An,Xi),(A5)+
			return src = rop16(op, true), write16(src, a5), void(a5 = a5 + 2);
		case 0537: // MOVE.W Abs...,(A5)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a5), void(a5 = a5 + 2);
		case 0540: // MOVE.W Dn,-(A5)
		case 0541: // MOVE.W An,-(A5)
		case 0542: // MOVE.W (An),-(A5)
		case 0543: // MOVE.W (An)+,-(A5)
		case 0544: // MOVE.W -(An),-(A5)
		case 0545: // MOVE.W d(An),-(A5)
		case 0546: // MOVE.W d(An,Xi),-(A5)
			return src = rop16(op, true), a5 = a5 - 2, write16(src, a5);
		case 0547: // MOVE.W Abs...,-(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a5 = a5 - 2, write16(src, a5);
		case 0550: // MOVE.W Dn,d(A5)
		case 0551: // MOVE.W An,d(A5)
		case 0552: // MOVE.W (An),d(A5)
		case 0553: // MOVE.W (An)+,d(A5)
		case 0554: // MOVE.W -(An),d(A5)
		case 0555: // MOVE.W d(An),d(A5)
		case 0556: // MOVE.W d(An,Xi),d(A5)
			return src = rop16(op, true), write16(src, disp(a5));
		case 0557: // MOVE.W Abs...,d(A5)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a5));
		case 0560: // MOVE.W Dn,d(A5,Xi)
		case 0561: // MOVE.W An,d(A5,Xi)
		case 0562: // MOVE.W (An),d(A5,Xi)
		case 0563: // MOVE.W (An)+,d(A5,Xi)
		case 0564: // MOVE.W -(An),d(A5,Xi)
		case 0565: // MOVE.W d(An),d(A5,Xi)
		case 0566: // MOVE.W d(An,Xi),d(A5,Xi)
			return src = rop16(op, true), write16(src, index(a5));
		case 0567: // MOVE.W Abs...,d(A5,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a5));
		case 0600: // MOVE.W Dn,D6
		case 0601: // MOVE.W An,D6
		case 0602: // MOVE.W (An),D6
		case 0603: // MOVE.W (An)+,D6
		case 0604: // MOVE.W -(An),D6
		case 0605: // MOVE.W d(An),D6
		case 0606: // MOVE.W d(An,Xi),D6
			return void(d6 = d6 & ~0xffff | rop16(op, true));
		case 0607: // MOVE.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xffff | rop16(op, true));
		case 0610: // MOVEA.W Dn,A6
		case 0611: // MOVEA.W An,A6
		case 0612: // MOVEA.W (An),A6
		case 0613: // MOVEA.W (An)+,A6
		case 0614: // MOVEA.W -(An),A6
		case 0615: // MOVEA.W d(An),A6
		case 0616: // MOVEA.W d(An,Xi),A6
			return void(a6 = rop16(op) << 16 >> 16);
		case 0617: // MOVEA.W Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = rop16(op) << 16 >> 16);
		case 0620: // MOVE.W Dn,(A6)
		case 0621: // MOVE.W An,(A6)
		case 0622: // MOVE.W (An),(A6)
		case 0623: // MOVE.W (An)+,(A6)
		case 0624: // MOVE.W -(An),(A6)
		case 0625: // MOVE.W d(An),(A6)
		case 0626: // MOVE.W d(An,Xi),(A6)
			return src = rop16(op, true), write16(src, a6);
		case 0627: // MOVE.W Abs...,(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a6);
		case 0630: // MOVE.W Dn,(A6)+
		case 0631: // MOVE.W An,(A6)+
		case 0632: // MOVE.W (An),(A6)+
		case 0633: // MOVE.W (An)+,(A6)+
		case 0634: // MOVE.W -(An),(A6)+
		case 0635: // MOVE.W d(An),(A6)+
		case 0636: // MOVE.W d(An,Xi),(A6)+
			return src = rop16(op, true), write16(src, a6), void(a6 = a6 + 2);
		case 0637: // MOVE.W Abs...,(A6)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a6), void(a6 = a6 + 2);
		case 0640: // MOVE.W Dn,-(A6)
		case 0641: // MOVE.W An,-(A6)
		case 0642: // MOVE.W (An),-(A6)
		case 0643: // MOVE.W (An)+,-(A6)
		case 0644: // MOVE.W -(An),-(A6)
		case 0645: // MOVE.W d(An),-(A6)
		case 0646: // MOVE.W d(An,Xi),-(A6)
			return src = rop16(op, true), a6 = a6 - 2, write16(src, a6);
		case 0647: // MOVE.W Abs...,-(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a6 = a6 - 2, write16(src, a6);
		case 0650: // MOVE.W Dn,d(A6)
		case 0651: // MOVE.W An,d(A6)
		case 0652: // MOVE.W (An),d(A6)
		case 0653: // MOVE.W (An)+,d(A6)
		case 0654: // MOVE.W -(An),d(A6)
		case 0655: // MOVE.W d(An),d(A6)
		case 0656: // MOVE.W d(An,Xi),d(A6)
			return src = rop16(op, true), write16(src, disp(a6));
		case 0657: // MOVE.W Abs...,d(A6)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a6));
		case 0660: // MOVE.W Dn,d(A6,Xi)
		case 0661: // MOVE.W An,d(A6,Xi)
		case 0662: // MOVE.W (An),d(A6,Xi)
		case 0663: // MOVE.W (An)+,d(A6,Xi)
		case 0664: // MOVE.W -(An),d(A6,Xi)
		case 0665: // MOVE.W d(An),d(A6,Xi)
		case 0666: // MOVE.W d(An,Xi),d(A6,Xi)
			return src = rop16(op, true), write16(src, index(a6));
		case 0667: // MOVE.W Abs...,d(A6,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a6));
		case 0700: // MOVE.W Dn,D7
		case 0701: // MOVE.W An,D7
		case 0702: // MOVE.W (An),D7
		case 0703: // MOVE.W (An)+,D7
		case 0704: // MOVE.W -(An),D7
		case 0705: // MOVE.W d(An),D7
		case 0706: // MOVE.W d(An,Xi),D7
			return void(d7 = d7 & ~0xffff | rop16(op, true));
		case 0707: // MOVE.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xffff | rop16(op, true));
		case 0710: // MOVEA.W Dn,A7
		case 0711: // MOVEA.W An,A7
		case 0712: // MOVEA.W (An),A7
		case 0713: // MOVEA.W (An)+,A7
		case 0714: // MOVEA.W -(An),A7
		case 0715: // MOVEA.W d(An),A7
		case 0716: // MOVEA.W d(An,Xi),A7
			return void(a7 = rop16(op) << 16 >> 16);
		case 0717: // MOVEA.W Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = rop16(op) << 16 >> 16);
		case 0720: // MOVE.W Dn,(A7)
		case 0721: // MOVE.W An,(A7)
		case 0722: // MOVE.W (An),(A7)
		case 0723: // MOVE.W (An)+,(A7)
		case 0724: // MOVE.W -(An),(A7)
		case 0725: // MOVE.W d(An),(A7)
		case 0726: // MOVE.W d(An,Xi),(A7)
			return src = rop16(op, true), write16(src, a7);
		case 0727: // MOVE.W Abs...,(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a7);
		case 0730: // MOVE.W Dn,(A7)+
		case 0731: // MOVE.W An,(A7)+
		case 0732: // MOVE.W (An),(A7)+
		case 0733: // MOVE.W (An)+,(A7)+
		case 0734: // MOVE.W -(An),(A7)+
		case 0735: // MOVE.W d(An),(A7)+
		case 0736: // MOVE.W d(An,Xi),(A7)+
			return src = rop16(op, true), write16(src, a7), void(a7 = a7 + 2);
		case 0737: // MOVE.W Abs...,(A7)+
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, a7), void(a7 = a7 + 2);
		case 0740: // MOVE.W Dn,-(A7)
		case 0741: // MOVE.W An,-(A7)
		case 0742: // MOVE.W (An),-(A7)
		case 0743: // MOVE.W (An)+,-(A7)
		case 0744: // MOVE.W -(An),-(A7)
		case 0745: // MOVE.W d(An),-(A7)
		case 0746: // MOVE.W d(An,Xi),-(A7)
			return src = rop16(op, true), a7 = a7 - 2, write16(src, a7);
		case 0747: // MOVE.W Abs...,-(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), a7 = a7 - 2, write16(src, a7);
		case 0750: // MOVE.W Dn,d(A7)
		case 0751: // MOVE.W An,d(A7)
		case 0752: // MOVE.W (An),d(A7)
		case 0753: // MOVE.W (An)+,d(A7)
		case 0754: // MOVE.W -(An),d(A7)
		case 0755: // MOVE.W d(An),d(A7)
		case 0756: // MOVE.W d(An,Xi),d(A7)
			return src = rop16(op, true), write16(src, disp(a7));
		case 0757: // MOVE.W Abs...,d(A7)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, disp(a7));
		case 0760: // MOVE.W Dn,d(A7,Xi)
		case 0761: // MOVE.W An,d(A7,Xi)
		case 0762: // MOVE.W (An),d(A7,Xi)
		case 0763: // MOVE.W (An)+,d(A7,Xi)
		case 0764: // MOVE.W -(An),d(A7,Xi)
		case 0765: // MOVE.W d(An),d(A7,Xi)
		case 0766: // MOVE.W d(An,Xi),d(A7,Xi)
			return src = rop16(op, true), write16(src, index(a7));
		case 0767: // MOVE.W Abs...,d(A7,Xi)
			if ((op & 7) >= 5)
				return exception(4);
			return src = rop16(op, true), write16(src, index(a7));
		default:
			return exception(4);
		}
	}

	void execute_4(int op) {
		switch (op >> 3 & 0777) {
		case 0000: // NEGX.B Dn
		case 0002: // NEGX.B (An)
		case 0003: // NEGX.B (An)+
		case 0004: // NEGX.B -(An)
		case 0005: // NEGX.B d(An)
		case 0006: // NEGX.B d(An,Xi)
			return rwop8(op, negx8);
		case 0007: // NEGX.B Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, negx8);
		case 0010: // NEGX.W Dn
		case 0012: // NEGX.W (An)
		case 0013: // NEGX.W (An)+
		case 0014: // NEGX.W -(An)
		case 0015: // NEGX.W d(An)
		case 0016: // NEGX.W d(An,Xi)
			return rwop16(op, negx16);
		case 0017: // NEGX.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, negx16);
		case 0020: // NEGX.L Dn
		case 0022: // NEGX.L (An)
		case 0023: // NEGX.L (An)+
		case 0024: // NEGX.L -(An)
		case 0025: // NEGX.L d(An)
		case 0026: // NEGX.L d(An,Xi)
			return rwop32(op, negx32);
		case 0027: // NEGX.L Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, negx32);
		case 0030: // MOVE SR,Dn
		case 0032: // MOVE SR,(An)
		case 0033: // MOVE SR,(An)+
		case 0034: // MOVE SR,-(An)
		case 0035: // MOVE SR,d(An)
		case 0036: // MOVE SR,d(An,Xi)
			return rwop16(op, thru, sr);
		case 0037: // MOVE SR,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, thru, sr);
		case 0060: // CHK Dn,D0
		case 0062: // CHK (An),D0
		case 0063: // CHK (An)+,D0
		case 0064: // CHK -(An),D0
		case 0065: // CHK d(An),D0
		case 0066: // CHK d(An,Xi),D0
			return chk(rop16(op), d0);
		case 0067: // CHK Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d0);
		case 0072: // LEA (An),A0
		case 0075: // LEA d(An),A0
		case 0076: // LEA d(An,Xi),A0
			return void(a0 = lea(op));
		case 0077: // LEA Abs...,A0
			if ((op & 7) >= 4)
				return exception(4);
			return void(a0 = lea(op));
		case 0100: // CLR.B Dn
		case 0102: // CLR.B (An)
		case 0103: // CLR.B (An)+
		case 0104: // CLR.B -(An)
		case 0105: // CLR.B d(An)
		case 0106: // CLR.B d(An,Xi)
			return rwop8(op, clr);
		case 0107: // CLR.B Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, clr);
		case 0110: // CLR.W Dn
		case 0112: // CLR.W (An)
		case 0113: // CLR.W (An)+
		case 0114: // CLR.W -(An)
		case 0115: // CLR.W d(An)
		case 0116: // CLR.W d(An,Xi)
			return rwop16(op, clr);
		case 0117: // CLR.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, clr);
		case 0120: // CLR.L Dn
		case 0122: // CLR.L (An)
		case 0123: // CLR.L (An)+
		case 0124: // CLR.L -(An)
		case 0125: // CLR.L d(An)
		case 0126: // CLR.L d(An,Xi)
			return rwop32(op, clr);
		case 0127: // CLR.L Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, clr);
		case 0160: // CHK Dn,D1
		case 0162: // CHK (An),D1
		case 0163: // CHK (An)+,D1
		case 0164: // CHK -(An),D1
		case 0165: // CHK d(An),D1
		case 0166: // CHK d(An,Xi),D1
			return chk(rop16(op), d1);
		case 0167: // CHK Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d1);
		case 0172: // LEA (An),A1
		case 0175: // LEA d(An),A1
		case 0176: // LEA d(An,Xi),A1
			return void(a1 = lea(op));
		case 0177: // LEA Abs...,A1
			if ((op & 7) >= 4)
				return exception(4);
			return void(a1 = lea(op));
		case 0200: // NEG.B Dn
		case 0202: // NEG.B (An)
		case 0203: // NEG.B (An)+
		case 0204: // NEG.B -(An)
		case 0205: // NEG.B d(An)
		case 0206: // NEG.B d(An,Xi)
			return rwop8(op, neg8);
		case 0207: // NEG.B Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, neg8);
		case 0210: // NEG.W Dn
		case 0212: // NEG.W (An)
		case 0213: // NEG.W (An)+
		case 0214: // NEG.W -(An)
		case 0215: // NEG.W d(An)
		case 0216: // NEG.W d(An,Xi)
			return rwop16(op, neg16);
		case 0217: // NEG.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, neg16);
		case 0220: // NEG.L Dn
		case 0222: // NEG.L (An)
		case 0223: // NEG.L (An)+
		case 0224: // NEG.L -(An)
		case 0225: // NEG.L d(An)
		case 0226: // NEG.L d(An,Xi)
			return rwop32(op, neg32);
		case 0227: // NEG.L Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, neg32);
		case 0230: // MOVE Dn,CCR
		case 0232: // MOVE (An),CCR
		case 0233: // MOVE (An)+,CCR
		case 0234: // MOVE -(An),CCR
		case 0235: // MOVE d(An),CCR
		case 0236: // MOVE d(An,Xi),CCR
			return void(sr = sr & ~0xff | rop16(op) & 0xff);
		case 0237: // MOVE Abs...,CCR
			if ((op & 7) >= 5)
				return exception(4);
			return void(sr = sr & ~0xff | rop16(op) & 0xff);
		case 0260: // CHK Dn,D2
		case 0262: // CHK (An),D2
		case 0263: // CHK (An)+,D2
		case 0264: // CHK -(An),D2
		case 0265: // CHK d(An),D2
		case 0266: // CHK d(An,Xi),D2
			return chk(rop16(op), d2);
		case 0267: // CHK Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d2);
		case 0272: // LEA (An),A2
		case 0275: // LEA d(An),A2
		case 0276: // LEA d(An,Xi),A2
			return void(a2 = lea(op));
		case 0277: // LEA Abs...,A2
			if ((op & 7) >= 4)
				return exception(4);
			return void(a2 = lea(op));
		case 0300: // NOT.B Dn
		case 0302: // NOT.B (An)
		case 0303: // NOT.B (An)+
		case 0304: // NOT.B -(An)
		case 0305: // NOT.B d(An)
		case 0306: // NOT.B d(An,Xi)
			return rwop8(op, not8);
		case 0307: // NOT.B Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, not8);
		case 0310: // NOT.W Dn
		case 0312: // NOT.W (An)
		case 0313: // NOT.W (An)+
		case 0314: // NOT.W -(An)
		case 0315: // NOT.W d(An)
		case 0316: // NOT.W d(An,Xi)
			return rwop16(op, not16);
		case 0317: // NOT.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, not16);
		case 0320: // NOT.L Dn
		case 0322: // NOT.L (An)
		case 0323: // NOT.L (An)+
		case 0324: // NOT.L -(An)
		case 0325: // NOT.L d(An)
		case 0326: // NOT.L d(An,Xi)
			return rwop32(op, not32);
		case 0327: // NOT.L Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, not32);
		case 0330: // MOVE Dn,SR
		case 0332: // MOVE (An),SR
		case 0333: // MOVE (An)+,SR
		case 0334: // MOVE -(An),SR
		case 0335: // MOVE d(An),SR
		case 0336: // MOVE d(An,Xi),SR
			if ((sr & 0x2000) == 0)
				return exception(8);
			sr = rop16(op);
			if ((sr & 0x2000) == 0)
				ssp = a7, a7 = usp;
			return;
		case 0337: // MOVE Abs...,SR
			if ((op & 7) >= 5)
				return exception(4);
			if ((sr & 0x2000) == 0)
				return exception(8);
			sr = rop16(op);
			if ((sr & 0x2000) == 0)
				ssp = a7, a7 = usp;
			return;
		case 0360: // CHK Dn,D3
		case 0362: // CHK (An),D3
		case 0363: // CHK (An)+,D3
		case 0364: // CHK -(An),D3
		case 0365: // CHK d(An),D3
		case 0366: // CHK d(An,Xi),D3
			return chk(rop16(op), d3);
		case 0367: // CHK Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d3);
		case 0372: // LEA (An),A3
		case 0375: // LEA d(An),A3
		case 0376: // LEA d(An,Xi),A3
			return void(a3 = lea(op));
		case 0377: // LEA Abs...,A3
			if ((op & 7) >= 4)
				return exception(4);
			return void(a3 = lea(op));
		case 0400: // NBCD Dn
		case 0402: // NBCD (An)
		case 0403: // NBCD (An)+
		case 0404: // NBCD -(An)
		case 0405: // NBCD d(An)
		case 0406: // NBCD d(An,Xi)
			return rwop8(op, nbcd);
		case 0407: // NBCD Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, nbcd);
		case 0410: // SWAP Dn
			return rwop32(op, swap);
		case 0412: // PEA (An)
		case 0415: // PEA d(An)
		case 0416: // PEA d(An,Xi)
			return a7 = a7 - 4, write32(lea(op), a7);
		case 0417: // PEA Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return a7 = a7 - 4, write32(lea(op), a7);
		case 0420: // EXT.W Dn
			return rwop16(op, ext16);
		case 0422: // MOVEM.W <register list>,(An)
		case 0424: // MOVEM.W <register list>,-(An)
		case 0425: // MOVEM.W <register list>,d(An)
		case 0426: // MOVEM.W <register list>,d(An,Xi)
			return movem16rm(op);
		case 0427: // MOVEM.W <register list>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return movem16rm(op);
		case 0430: // EXT.L Dn
			return rwop32(op, ext32);
		case 0432: // MOVEM.L <register list>,(An)
		case 0434: // MOVEM.L <register list>,-(An)
		case 0435: // MOVEM.L <register list>,d(An)
		case 0436: // MOVEM.L <register list>,d(An,Xi)
			return movem32rm(op);
		case 0437: // MOVEM.L <register list>,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return movem32rm(op);
		case 0460: // CHK Dn,D4
		case 0462: // CHK (An),D4
		case 0463: // CHK (An)+,D4
		case 0464: // CHK -(An),D4
		case 0465: // CHK d(An),D4
		case 0466: // CHK d(An,Xi),D4
			return chk(rop16(op), d4);
		case 0467: // CHK Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d4);
		case 0472: // LEA (An),A4
		case 0475: // LEA d(An),A4
		case 0476: // LEA d(An,Xi),A4
			return void(a4 = lea(op));
		case 0477: // LEA Abs...,A4
			if ((op & 7) >= 4)
				return exception(4);
			return void(a4 = lea(op));
		case 0500: // TST.B Dn
		case 0502: // TST.B (An)
		case 0503: // TST.B (An)+
		case 0504: // TST.B -(An)
		case 0505: // TST.B d(An)
		case 0506: // TST.B d(An,Xi)
			return void(rop8(op, true));
		case 0507: // TST.B Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return void(rop8(op, true));
		case 0510: // TST.W Dn
		case 0512: // TST.W (An)
		case 0513: // TST.W (An)+
		case 0514: // TST.W -(An)
		case 0515: // TST.W d(An)
		case 0516: // TST.W d(An,Xi)
			return void(rop16(op, true));
		case 0517: // TST.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return void(rop16(op, true));
		case 0520: // TST.L Dn
		case 0522: // TST.L (An)
		case 0523: // TST.L (An)+
		case 0524: // TST.L -(An)
		case 0525: // TST.L d(An)
		case 0526: // TST.L d(An,Xi)
			return void(rop32(op, true));
		case 0527: // TST.L Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return void(rop32(op, true));
		case 0530: // TAS Dn
		case 0532: // TAS (An)
		case 0533: // TAS (An)+
		case 0534: // TAS -(An)
		case 0535: // TAS d(An)
		case 0536: // TAS d(An,Xi)
			return rwop8(op, tas);
		case 0537:
			switch (op & 7) {
			case 0: // TAS Abs.W
			case 1: // TAS Abs.L
				return rwop8(op, tas);
			case 4: // ILLEGAL
			default:
				return exception(4);
			}
		case 0560: // CHK Dn,D5
		case 0562: // CHK (An),D5
		case 0563: // CHK (An)+,D5
		case 0564: // CHK -(An),D5
		case 0565: // CHK d(An),D5
		case 0566: // CHK d(An,Xi),D5
			return chk(rop16(op), d5);
		case 0567: // CHK Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d5);
		case 0572: // LEA (An),A5
		case 0575: // LEA d(An),A5
		case 0576: // LEA d(An,Xi),A5
			return void(a5 = lea(op));
		case 0577: // LEA Abs...,A5
			if ((op & 7) >= 4)
				return exception(4);
			return void(a5 = lea(op));
		case 0622: // MOVEM.W (An),<register list>
		case 0623: // MOVEM.W (An)+,<register list>
		case 0625: // MOVEM.W d(An),<register list>
		case 0626: // MOVEM.W d(An,Xi),<register list>
			return movem16mr(op);
		case 0627: // MOVEM.W Abs...,<register list>
			if ((op & 7) >= 4)
				return exception(4);
			return movem16mr(op);
		case 0632: // MOVEM.L (An),<register list>
		case 0633: // MOVEM.L (An)+,<register list>
		case 0635: // MOVEM.L d(An),<register list>
		case 0636: // MOVEM.L d(An,Xi),<register list>
			return movem32mr(op);
		case 0637: // MOVEM.L Abs...,<register list>
			if ((op & 7) >= 4)
				return exception(4);
			return movem32mr(op);
		case 0660: // CHK Dn,D6
		case 0662: // CHK (An),D6
		case 0663: // CHK (An)+,D6
		case 0664: // CHK -(An),D6
		case 0665: // CHK d(An),D6
		case 0666: // CHK d(An,Xi),D6
			return chk(rop16(op), d6);
		case 0667: // CHK Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d6);
		case 0672: // LEA (An),A6
		case 0675: // LEA d(An),A6
		case 0676: // LEA d(An,Xi),A6
			return void(a6 = lea(op));
		case 0677: // LEA Abs...,A6
			if ((op & 7) >= 4)
				return exception(4);
			return void(a6 = lea(op));
		case 0710: // TRAP #<vector>
		case 0711:
			return exception(op & 0x0f | 0x20);
		case 0712: // LINK An,#<displacement>
			return link(op);
		case 0713: // UNLK An
			return unlk(op);
		case 0714: // MOVE An,USP
			if ((sr & 0x2000) == 0)
				return exception(8);
			return void(usp = rop32(op & 7 | 010));
		case 0715: // MOVE USP,An
			if ((sr & 0x2000) == 0)
				return exception(8);
			return rwop32(op & 7 | 010, thru, usp);
		case 0716:
			switch(op & 7) {
			case 0: // RESET
				if ((sr & 0x2000) == 0)
					return exception(8);
				return reset();
			case 1: // NOP
				return;
			case 2: // STOP
				if ((sr & 0x2000) == 0)
					return exception(8);
				return void(fSuspend = true);
			case 3: // RTE
				if ((sr & 0x2000) == 0)
					return exception(8);
				sr = read16(a7), a7 = a7 + 2, pc = read32(a7), a7 = a7 + 4;
				if ((sr & 0x2000) == 0)
					ssp = a7, a7 = usp;
				return;
			case 4: // RTD 68010
				return exception(4);
			case 5: // RTS
				return pc = read32(a7), void(a7 = a7 + 4);
			case 6: // TRAPV
				if ((sr & 2) != 0)
					return exception(7);
				return;
			case 7: // RTR
				return sr = sr & ~0xff | read16(a7) & 0xff, a7 = a7 + 2, pc = read32(a7), void(a7 = a7 + 4);
			default:
				return;
			}
		case 0722: // JSR (An)
		case 0725: // JSR d(An)
		case 0726: // JSR d(An,Xi)
			return jsr(op);
		case 0727: // JSR Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return jsr(op);
		case 0732: // JMP (An)
		case 0735: // JMP d(An)
		case 0736: // JMP d(An,Xi)
			return void(pc = lea(op));
		case 0737: // JMP Abs...
			if ((op & 7) >= 4)
				return exception(4);
			return void(pc = lea(op));
		case 0760: // CHK Dn,D7
		case 0762: // CHK (An),D7
		case 0763: // CHK (An)+,D7
		case 0764: // CHK -(An),D7
		case 0765: // CHK d(An),D7
		case 0766: // CHK d(An,Xi),D7
			return chk(rop16(op), d7);
		case 0767: // CHK Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return chk(rop16(op), d7);
		case 0772: // LEA (An),A7
		case 0775: // LEA d(An),A7
		case 0776: // LEA d(An,Xi),A7
			return void(a7 = lea(op));
		case 0777: // LEA Abs...,A7
			if ((op & 7) >= 4)
				return exception(4);
			return void(a7 = lea(op));
		default:
			return exception(4);
		}
	}

	void execute_5(int op) {
		switch (op >> 3 & 0777) {
		case 0000: // ADDQ.B #8,Dn
		case 0002: // ADDQ.B #8,(An)
		case 0003: // ADDQ.B #8,(An)+
		case 0004: // ADDQ.B #8,-(An)
		case 0005: // ADDQ.B #8,d(An)
		case 0006: // ADDQ.B #8,d(An,Xi)
			return rwop8(op, add8, 8);
		case 0007: // ADDQ.B #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 8);
		case 0010: // ADDQ.W #8,Dn
			return rwop16(op, add16, 8);
		case 0011: // ADDQ.W #8,An
			return rwop16(op, adda32, 8);
		case 0012: // ADDQ.W #8,(An)
		case 0013: // ADDQ.W #8,(An)+
		case 0014: // ADDQ.W #8,-(An)
		case 0015: // ADDQ.W #8,d(An)
		case 0016: // ADDQ.W #8,d(An,Xi)
			return rwop16(op, add16, 8);
		case 0017: // ADDQ.W #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 8);
		case 0020: // ADDQ.L #8,Dn
			return rwop32(op, add32, 8);
		case 0021: // ADDQ.L #8,An
			return rwop32(op, adda32, 8);
		case 0022: // ADDQ.L #8,(An)
		case 0023: // ADDQ.L #8,(An)+
		case 0024: // ADDQ.L #8,-(An)
		case 0025: // ADDQ.L #8,d(An)
		case 0026: // ADDQ.L #8,d(An,Xi)
			return rwop32(op, add32, 8);
		case 0027: // ADDQ.L #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 8);
		case 0030: // ST Dn
			return rwop8(op, thru, 0xff);
		case 0031: // DBT Dn,<label>
			return dbcc(op, true);
		case 0032: // ST (An)
		case 0033: // ST (An)+
		case 0034: // ST -(An)
		case 0035: // ST d(An)
		case 0036: // ST d(An,Xi)
			return rwop8(op, thru, 0xff);
		case 0037: // ST Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, 0xff);
		case 0040: // SUBQ.B #8,Dn
		case 0042: // SUBQ.B #8,(An)
		case 0043: // SUBQ.B #8,(An)+
		case 0044: // SUBQ.B #8,-(An)
		case 0045: // SUBQ.B #8,d(An)
		case 0046: // SUBQ.B #8,d(An,Xi)
			return rwop8(op, sub8, 8);
		case 0047: // SUBQ.B #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 8);
		case 0050: // SUBQ.W #8,Dn
			return rwop16(op, sub16, 8);
		case 0051: // SUBQ.W #8,An
			return rwop16(op, suba32, 8);
		case 0052: // SUBQ.W #8,(An)
		case 0053: // SUBQ.W #8,(An)+
		case 0054: // SUBQ.W #8,-(An)
		case 0055: // SUBQ.W #8,d(An)
		case 0056: // SUBQ.W #8,d(An,Xi)
			return rwop16(op, sub16, 8);
		case 0057: // SUBQ.W #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 8);
		case 0060: // SUBQ.L #8,Dn
			return rwop32(op, sub32, 8);
		case 0061: // SUBQ.L #8,An
			return rwop32(op, suba32, 8);
		case 0062: // SUBQ.L #8,(An)
		case 0063: // SUBQ.L #8,(An)+
		case 0064: // SUBQ.L #8,-(An)
		case 0065: // SUBQ.L #8,d(An)
		case 0066: // SUBQ.L #8,d(An,Xi)
			return rwop32(op, sub32, 8);
		case 0067: // SUBQ.L #8,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 8);
		case 0070: // SF Dn
			return rwop8(op, thru, 0);
		case 0071: // DBRA Dn,<label>
			return dbcc(op, false);
		case 0072: // SF (An)
		case 0073: // SF (An)+
		case 0074: // SF -(An)
		case 0075: // SF d(An)
		case 0076: // SF d(An,Xi)
			return rwop8(op, thru, 0);
		case 0077: // SF Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, 0);
		case 0100: // ADDQ.B #1,Dn
		case 0102: // ADDQ.B #1,(An)
		case 0103: // ADDQ.B #1,(An)+
		case 0104: // ADDQ.B #1,-(An)
		case 0105: // ADDQ.B #1,d(An)
		case 0106: // ADDQ.B #1,d(An,Xi)
			return rwop8(op, add8, 1);
		case 0107: // ADDQ.B #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 1);
		case 0110: // ADDQ.W #1,Dn
			return rwop16(op, add16, 1);
		case 0111: // ADDQ.W #1,An
			return rwop16(op, adda32, 1);
		case 0112: // ADDQ.W #1,(An)
		case 0113: // ADDQ.W #1,(An)+
		case 0114: // ADDQ.W #1,-(An)
		case 0115: // ADDQ.W #1,d(An)
		case 0116: // ADDQ.W #1,d(An,Xi)
			return rwop16(op, add16, 1);
		case 0117: // ADDQ.W #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 1);
		case 0120: // ADDQ.L #1,Dn
			return rwop32(op, add32, 1);
		case 0121: // ADDQ.L #1,An
			return rwop32(op, adda32, 1);
		case 0122: // ADDQ.L #1,(An)
		case 0123: // ADDQ.L #1,(An)+
		case 0124: // ADDQ.L #1,-(An)
		case 0125: // ADDQ.L #1,d(An)
		case 0126: // ADDQ.L #1,d(An,Xi)
			return rwop32(op, add32, 1);
		case 0127: // ADDQ.L #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 1);
		case 0130: // SHI Dn
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) == 0 ? 0xff : 0);
		case 0131: // DBHI Dn,<label>
			return dbcc(op, ((sr >> 2 | sr) & 1) == 0);
		case 0132: // SHI (An)
		case 0133: // SHI (An)+
		case 0134: // SHI -(An)
		case 0135: // SHI d(An)
		case 0136: // SHI d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) == 0 ? 0xff : 0);
		case 0137: // SHI Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) == 0 ? 0xff : 0);
		case 0140: // SUBQ.B #1,Dn
		case 0142: // SUBQ.B #1,(An)
		case 0143: // SUBQ.B #1,(An)+
		case 0144: // SUBQ.B #1,-(An)
		case 0145: // SUBQ.B #1,d(An)
		case 0146: // SUBQ.B #1,d(An,Xi)
			return rwop8(op, sub8, 1);
		case 0147: // SUBQ.B #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 1);
		case 0150: // SUBQ.W #1,Dn
			return rwop16(op, sub16, 1);
		case 0151: // SUBQ.W #1,An
			return rwop16(op, suba32, 1);
		case 0152: // SUBQ.W #1,(An)
		case 0153: // SUBQ.W #1,(An)+
		case 0154: // SUBQ.W #1,-(An)
		case 0155: // SUBQ.W #1,d(An)
		case 0156: // SUBQ.W #1,d(An,Xi)
			return rwop16(op, sub16, 1);
		case 0157: // SUBQ.W #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 1);
		case 0160: // SUBQ.L #1,Dn
			return rwop32(op, sub32, 1);
		case 0161: // SUBQ.L #1,An
			return rwop32(op, suba32, 1);
		case 0162: // SUBQ.L #1,(An)
		case 0163: // SUBQ.L #1,(An)+
		case 0164: // SUBQ.L #1,-(An)
		case 0165: // SUBQ.L #1,d(An)
		case 0166: // SUBQ.L #1,d(An,Xi)
			return rwop32(op, sub32, 1);
		case 0167: // SUBQ.L #1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 1);
		case 0170: // SLS Dn
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) != 0 ? 0xff : 0);
		case 0171: // DBLS Dn,<label>
			return dbcc(op, ((sr >> 2 | sr) & 1) != 0);
		case 0172: // SLS (An)
		case 0173: // SLS (An)+
		case 0174: // SLS -(An)
		case 0175: // SLS d(An)
		case 0176: // SLS d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) != 0 ? 0xff : 0);
		case 0177: // SLS Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 | sr) & 1) != 0 ? 0xff : 0);
		case 0200: // ADDQ.B #2,Dn
		case 0202: // ADDQ.B #2,(An)
		case 0203: // ADDQ.B #2,(An)+
		case 0204: // ADDQ.B #2,-(An)
		case 0205: // ADDQ.B #2,d(An)
		case 0206: // ADDQ.B #2,d(An,Xi)
			return rwop8(op, add8, 2);
		case 0207: // ADDQ.B #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 2);
		case 0210: // ADDQ.W #2,Dn
			return rwop16(op, add16, 2);
		case 0211: // ADDQ.W #2,An
			return rwop16(op, adda32, 2);
		case 0212: // ADDQ.W #2,(An)
		case 0213: // ADDQ.W #2,(An)+
		case 0214: // ADDQ.W #2,-(An)
		case 0215: // ADDQ.W #2,d(An)
		case 0216: // ADDQ.W #2,d(An,Xi)
			return rwop16(op, add16, 2);
		case 0217: // ADDQ.W #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 2);
		case 0220: // ADDQ.L #2,Dn
			return rwop32(op, add32, 2);
		case 0221: // ADDQ.L #2,An
			return rwop32(op, adda32, 2);
		case 0222: // ADDQ.L #2,(An)
		case 0223: // ADDQ.L #2,(An)+
		case 0224: // ADDQ.L #2,-(An)
		case 0225: // ADDQ.L #2,d(An)
		case 0226: // ADDQ.L #2,d(An,Xi)
			return rwop32(op, add32, 2);
		case 0227: // ADDQ.L #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 2);
		case 0230: // SCC Dn
			return rwop8(op, thru, (sr & 1) == 0 ? 0xff : 0);
		case 0231: // DBCC Dn,<label>
			return dbcc(op, (sr & 1) == 0);
		case 0232: // SCC (An)
		case 0233: // SCC (An)+
		case 0234: // SCC -(An)
		case 0235: // SCC d(An)
		case 0236: // SCC d(An,Xi)
			return rwop8(op, thru, (sr & 1) == 0 ? 0xff : 0);
		case 0237: // SCC Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 1) == 0 ? 0xff : 0);
		case 0240: // SUBQ.B #2,Dn
		case 0242: // SUBQ.B #2,(An)
		case 0243: // SUBQ.B #2,(An)+
		case 0244: // SUBQ.B #2,-(An)
		case 0245: // SUBQ.B #2,d(An)
		case 0246: // SUBQ.B #2,d(An,Xi)
			return rwop8(op, sub8, 2);
		case 0247: // SUBQ.B #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 2);
		case 0250: // SUBQ.W #2,Dn
			return rwop16(op, sub16, 2);
		case 0251: // SUBQ.W #2,An
			return rwop16(op, suba32, 2);
		case 0252: // SUBQ.W #2,(An)
		case 0253: // SUBQ.W #2,(An)+
		case 0254: // SUBQ.W #2,-(An)
		case 0255: // SUBQ.W #2,d(An)
		case 0256: // SUBQ.W #2,d(An,Xi)
			return rwop16(op, sub16, 2);
		case 0257: // SUBQ.W #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 2);
		case 0260: // SUBQ.L #2,Dn
			return rwop32(op, sub32, 2);
		case 0261: // SUBQ.L #2,An
			return rwop32(op, suba32, 2);
		case 0262: // SUBQ.L #2,(An)
		case 0263: // SUBQ.L #2,(An)+
		case 0264: // SUBQ.L #2,-(An)
		case 0265: // SUBQ.L #2,d(An)
		case 0266: // SUBQ.L #2,d(An,Xi)
			return rwop32(op, sub32, 2);
		case 0267: // SUBQ.L #2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 2);
		case 0270: // SCS Dn
			return rwop8(op, thru, (sr & 1) != 0 ? 0xff : 0);
		case 0271: // DBCS Dn,<label>
			return dbcc(op, (sr & 1) != 0);
		case 0272: // SCS (An)
		case 0273: // SCS (An)+
		case 0274: // SCS -(An)
		case 0275: // SCS d(An)
		case 0276: // SCS d(An,Xi)
			return rwop8(op, thru, (sr & 1) != 0 ? 0xff : 0);
		case 0277: // SCS Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 1) != 0 ? 0xff : 0);
		case 0300: // ADDQ.B #3,Dn
		case 0302: // ADDQ.B #3,(An)
		case 0303: // ADDQ.B #3,(An)+
		case 0304: // ADDQ.B #3,-(An)
		case 0305: // ADDQ.B #3,d(An)
		case 0306: // ADDQ.B #3,d(An,Xi)
			return rwop8(op, add8, 3);
		case 0307: // ADDQ.B #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 3);
		case 0310: // ADDQ.W #3,Dn
			return rwop16(op, add16, 3);
		case 0311: // ADDQ.W #3,An
			return rwop16(op, adda32, 3);
		case 0312: // ADDQ.W #3,(An)
		case 0313: // ADDQ.W #3,(An)+
		case 0314: // ADDQ.W #3,-(An)
		case 0315: // ADDQ.W #3,d(An)
		case 0316: // ADDQ.W #3,d(An,Xi)
			return rwop16(op, add16, 3);
		case 0317: // ADDQ.W #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 3);
		case 0320: // ADDQ.L #3,Dn
			return rwop32(op, add32, 3);
		case 0321: // ADDQ.L #3,An
			return rwop32(op, adda32, 3);
		case 0322: // ADDQ.L #3,(An)
		case 0323: // ADDQ.L #3,(An)+
		case 0324: // ADDQ.L #3,-(An)
		case 0325: // ADDQ.L #3,d(An)
		case 0326: // ADDQ.L #3,d(An,Xi)
			return rwop32(op, add32, 3);
		case 0327: // ADDQ.L #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 3);
		case 0330: // SNE Dn
			return rwop8(op, thru, (sr & 4) == 0 ? 0xff : 0);
		case 0331: // DBNE Dn,<label>
			return dbcc(op, (sr & 4) == 0);
		case 0332: // SNE (An)
		case 0333: // SNE (An)+
		case 0334: // SNE -(An)
		case 0335: // SNE d(An)
		case 0336: // SNE d(An,Xi)
			return rwop8(op, thru, (sr & 4) == 0 ? 0xff : 0);
		case 0337: // SNE Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 4) == 0 ? 0xff : 0);
		case 0340: // SUBQ.B #3,Dn
		case 0342: // SUBQ.B #3,(An)
		case 0343: // SUBQ.B #3,(An)+
		case 0344: // SUBQ.B #3,-(An)
		case 0345: // SUBQ.B #3,d(An)
		case 0346: // SUBQ.B #3,d(An,Xi)
			return rwop8(op, sub8, 3);
		case 0347: // SUBQ.B #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 3);
		case 0350: // SUBQ.W #3,Dn
			return rwop16(op, sub16, 3);
		case 0351: // SUBQ.W #3,An
			return rwop16(op, suba32, 3);
		case 0352: // SUBQ.W #3,(An)
		case 0353: // SUBQ.W #3,(An)+
		case 0354: // SUBQ.W #3,-(An)
		case 0355: // SUBQ.W #3,d(An)
		case 0356: // SUBQ.W #3,d(An,Xi)
			return rwop16(op, sub16, 3);
		case 0357: // SUBQ.W #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 3);
		case 0360: // SUBQ.L #3,Dn
			return rwop32(op, sub32, 3);
		case 0361: // SUBQ.L #3,An
			return rwop32(op, suba32, 3);
		case 0362: // SUBQ.L #3,(An)
		case 0363: // SUBQ.L #3,(An)+
		case 0364: // SUBQ.L #3,-(An)
		case 0365: // SUBQ.L #3,d(An)
		case 0366: // SUBQ.L #3,d(An,Xi)
			return rwop32(op, sub32, 3);
		case 0367: // SUBQ.L #3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 3);
		case 0370: // SEQ Dn
			return rwop8(op, thru, (sr & 4) != 0 ? 0xff : 0);
		case 0371: // DBEQ Dn,<label>
			return dbcc(op, (sr & 4) != 0);
		case 0372: // SEQ (An)
		case 0373: // SEQ (An)+
		case 0374: // SEQ -(An)
		case 0375: // SEQ d(An)
		case 0376: // SEQ d(An,Xi)
			return rwop8(op, thru, (sr & 4) != 0 ? 0xff : 0);
		case 0377: // SEQ Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 4) != 0 ? 0xff : 0);
		case 0400: // ADDQ.B #4,Dn
		case 0402: // ADDQ.B #4,(An)
		case 0403: // ADDQ.B #4,(An)+
		case 0404: // ADDQ.B #4,-(An)
		case 0405: // ADDQ.B #4,d(An)
		case 0406: // ADDQ.B #4,d(An,Xi)
			return rwop8(op, add8, 4);
		case 0407: // ADDQ.B #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 4);
		case 0410: // ADDQ.W #4,Dn
			return rwop16(op, add16, 4);
		case 0411: // ADDQ.W #4,An
			return rwop16(op, adda32, 4);
		case 0412: // ADDQ.W #4,(An)
		case 0413: // ADDQ.W #4,(An)+
		case 0414: // ADDQ.W #4,-(An)
		case 0415: // ADDQ.W #4,d(An)
		case 0416: // ADDQ.W #4,d(An,Xi)
			return rwop16(op, add16, 4);
		case 0417: // ADDQ.W #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 4);
		case 0420: // ADDQ.L #4,Dn
			return rwop32(op, add32, 4);
		case 0421: // ADDQ.L #4,An
			return rwop32(op, adda32, 4);
		case 0422: // ADDQ.L #4,(An)
		case 0423: // ADDQ.L #4,(An)+
		case 0424: // ADDQ.L #4,-(An)
		case 0425: // ADDQ.L #4,d(An)
		case 0426: // ADDQ.L #4,d(An,Xi)
			return rwop32(op, add32, 4);
		case 0427: // ADDQ.L #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 4);
		case 0430: // SVC Dn
			return rwop8(op, thru, (sr & 2) == 0 ? 0xff : 0);
		case 0431: // DBVC Dn,<label>
			return dbcc(op, (sr & 2) == 0);
		case 0432: // SVC (An)
		case 0433: // SVC (An)+
		case 0434: // SVC -(An)
		case 0435: // SVC d(An)
		case 0436: // SVC d(An,Xi)
			return rwop8(op, thru, (sr & 2) == 0 ? 0xff : 0);
		case 0437: // SVC Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 2) == 0 ? 0xff : 0);
		case 0440: // SUBQ.B #4,Dn
		case 0442: // SUBQ.B #4,(An)
		case 0443: // SUBQ.B #4,(An)+
		case 0444: // SUBQ.B #4,-(An)
		case 0445: // SUBQ.B #4,d(An)
		case 0446: // SUBQ.B #4,d(An,Xi)
			return rwop8(op, sub8, 4);
		case 0447: // SUBQ.B #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 4);
		case 0450: // SUBQ.W #4,Dn
			return rwop16(op, sub16, 4);
		case 0451: // SUBQ.W #4,An
			return rwop16(op, suba32, 4);
		case 0452: // SUBQ.W #4,(An)
		case 0453: // SUBQ.W #4,(An)+
		case 0454: // SUBQ.W #4,-(An)
		case 0455: // SUBQ.W #4,d(An)
		case 0456: // SUBQ.W #4,d(An,Xi)
			return rwop16(op, sub16, 4);
		case 0457: // SUBQ.W #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 4);
		case 0460: // SUBQ.L #4,Dn
			return rwop32(op, sub32, 4);
		case 0461: // SUBQ.L #4,An
			return rwop32(op, suba32, 4);
		case 0462: // SUBQ.L #4,(An)
		case 0463: // SUBQ.L #4,(An)+
		case 0464: // SUBQ.L #4,-(An)
		case 0465: // SUBQ.L #4,d(An)
		case 0466: // SUBQ.L #4,d(An,Xi)
			return rwop32(op, sub32, 4);
		case 0467: // SUBQ.L #4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 4);
		case 0470: // SVS Dn
			return rwop8(op, thru, (sr & 2) != 0 ? 0xff : 0);
		case 0471: // DBVS Dn,<label>
			return dbcc(op, (sr & 2) != 0);
		case 0472: // SVS (An)
		case 0473: // SVS (An)+
		case 0474: // SVS -(An)
		case 0475: // SVS d(An)
		case 0476: // SVS d(An,Xi)
			return rwop8(op, thru, (sr & 2) != 0 ? 0xff : 0);
		case 0477: // SVS Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 2) != 0 ? 0xff : 0);
		case 0500: // ADDQ.B #5,Dn
		case 0502: // ADDQ.B #5,(An)
		case 0503: // ADDQ.B #5,(An)+
		case 0504: // ADDQ.B #5,-(An)
		case 0505: // ADDQ.B #5,d(An)
		case 0506: // ADDQ.B #5,d(An,Xi)
			return rwop8(op, add8, 5);
		case 0507: // ADDQ.B #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 5);
		case 0510: // ADDQ.W #5,Dn
			return rwop16(op, add16, 5);
		case 0511: // ADDQ.W #5,An
			return rwop16(op, adda32, 5);
		case 0512: // ADDQ.W #5,(An)
		case 0513: // ADDQ.W #5,(An)+
		case 0514: // ADDQ.W #5,-(An)
		case 0515: // ADDQ.W #5,d(An)
		case 0516: // ADDQ.W #5,d(An,Xi)
			return rwop16(op, add16, 5);
		case 0517: // ADDQ.W #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 5);
		case 0520: // ADDQ.L #5,Dn
			return rwop32(op, add32, 5);
		case 0521: // ADDQ.L #5,An
			return rwop32(op, adda32, 5);
		case 0522: // ADDQ.L #5,(An)
		case 0523: // ADDQ.L #5,(An)+
		case 0524: // ADDQ.L #5,-(An)
		case 0525: // ADDQ.L #5,d(An)
		case 0526: // ADDQ.L #5,d(An,Xi)
			return rwop32(op, add32, 5);
		case 0527: // ADDQ.L #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 5);
		case 0530: // SPL Dn
			return rwop8(op, thru, (sr & 8) == 0 ? 0xff : 0);
		case 0531: // DBPL Dn,<label>
			return dbcc(op, (sr & 8) == 0);
		case 0532: // SPL (An)
		case 0533: // SPL (An)+
		case 0534: // SPL -(An)
		case 0535: // SPL d(An)
		case 0536: // SPL d(An,Xi)
			return rwop8(op, thru, (sr & 8) == 0 ? 0xff : 0);
		case 0537: // SPL Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 8) == 0 ? 0xff : 0);
		case 0540: // SUBQ.B #5,Dn
		case 0542: // SUBQ.B #5,(An)
		case 0543: // SUBQ.B #5,(An)+
		case 0544: // SUBQ.B #5,-(An)
		case 0545: // SUBQ.B #5,d(An)
		case 0546: // SUBQ.B #5,d(An,Xi)
			return rwop8(op, sub8, 5);
		case 0547: // SUBQ.B #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 5);
		case 0550: // SUBQ.W #5,Dn
			return rwop16(op, sub16, 5);
		case 0551: // SUBQ.W #5,An
			return rwop16(op, suba32, 5);
		case 0552: // SUBQ.W #5,(An)
		case 0553: // SUBQ.W #5,(An)+
		case 0554: // SUBQ.W #5,-(An)
		case 0555: // SUBQ.W #5,d(An)
		case 0556: // SUBQ.W #5,d(An,Xi)
			return rwop16(op, sub16, 5);
		case 0557: // SUBQ.W #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 5);
		case 0560: // SUBQ.L #5,Dn
			return rwop32(op, sub32, 5);
		case 0561: // SUBQ.L #5,An
			return rwop32(op, suba32, 5);
		case 0562: // SUBQ.L #5,(An)
		case 0563: // SUBQ.L #5,(An)+
		case 0564: // SUBQ.L #5,-(An)
		case 0565: // SUBQ.L #5,d(An)
		case 0566: // SUBQ.L #5,d(An,Xi)
			return rwop32(op, sub32, 5);
		case 0567: // SUBQ.L #5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 5);
		case 0570: // SMI Dn
			return rwop8(op, thru, (sr & 8) != 0 ? 0xff : 0);
		case 0571: // DBMI Dn,<label>
			return dbcc(op, (sr & 8) != 0);
		case 0572: // SMI (An)
		case 0573: // SMI (An)+
		case 0574: // SMI -(An)
		case 0575: // SMI d(An)
		case 0576: // SMI d(An,Xi)
			return rwop8(op, thru, (sr & 8) != 0 ? 0xff : 0);
		case 0577: // SMI Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, (sr & 8) != 0 ? 0xff : 0);
		case 0600: // ADDQ.B #6,Dn
		case 0602: // ADDQ.B #6,(An)
		case 0603: // ADDQ.B #6,(An)+
		case 0604: // ADDQ.B #6,-(An)
		case 0605: // ADDQ.B #6,d(An)
		case 0606: // ADDQ.B #6,d(An,Xi)
			return rwop8(op, add8, 6);
		case 0607: // ADDQ.B #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 6);
		case 0610: // ADDQ.W #6,Dn
			return rwop16(op, add16, 6);
		case 0611: // ADDQ.W #6,An
			return rwop16(op, adda32, 6);
		case 0612: // ADDQ.W #6,(An)
		case 0613: // ADDQ.W #6,(An)+
		case 0614: // ADDQ.W #6,-(An)
		case 0615: // ADDQ.W #6,d(An)
		case 0616: // ADDQ.W #6,d(An,Xi)
			return rwop16(op, add16, 6);
		case 0617: // ADDQ.W #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 6);
		case 0620: // ADDQ.L #6,Dn
			return rwop32(op, add32, 6);
		case 0621: // ADDQ.L #6,An
			return rwop32(op, adda32, 6);
		case 0622: // ADDQ.L #6,(An)
		case 0623: // ADDQ.L #6,(An)+
		case 0624: // ADDQ.L #6,-(An)
		case 0625: // ADDQ.L #6,d(An)
		case 0626: // ADDQ.L #6,d(An,Xi)
			return rwop32(op, add32, 6);
		case 0627: // ADDQ.L #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 6);
		case 0630: // SGE Dn
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) == 0 ? 0xff : 0);
		case 0631: // DBGE Dn,<label>
			return dbcc(op, ((sr >> 2 ^ sr) & 2) == 0);
		case 0632: // SGE (An)
		case 0633: // SGE (An)+
		case 0634: // SGE -(An)
		case 0635: // SGE d(An)
		case 0636: // SGE d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) == 0 ? 0xff : 0);
		case 0637: // SGE Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) == 0 ? 0xff : 0);
		case 0640: // SUBQ.B #6,Dn
		case 0642: // SUBQ.B #6,(An)
		case 0643: // SUBQ.B #6,(An)+
		case 0644: // SUBQ.B #6,-(An)
		case 0645: // SUBQ.B #6,d(An)
		case 0646: // SUBQ.B #6,d(An,Xi)
			return rwop8(op, sub8, 6);
		case 0647: // SUBQ.B #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 6);
		case 0650: // SUBQ.W #6,Dn
			return rwop16(op, sub16, 6);
		case 0651: // SUBQ.W #6,An
			return rwop16(op, suba32, 6);
		case 0652: // SUBQ.W #6,(An)
		case 0653: // SUBQ.W #6,(An)+
		case 0654: // SUBQ.W #6,-(An)
		case 0655: // SUBQ.W #6,d(An)
		case 0656: // SUBQ.W #6,d(An,Xi)
			return rwop16(op, sub16, 6);
		case 0657: // SUBQ.W #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 6);
		case 0660: // SUBQ.L #6,Dn
			return rwop32(op, sub32, 6);
		case 0661: // SUBQ.L #6,An
			return rwop32(op, suba32, 6);
		case 0662: // SUBQ.L #6,(An)
		case 0663: // SUBQ.L #6,(An)+
		case 0664: // SUBQ.L #6,-(An)
		case 0665: // SUBQ.L #6,d(An)
		case 0666: // SUBQ.L #6,d(An,Xi)
			return rwop32(op, sub32, 6);
		case 0667: // SUBQ.L #6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 6);
		case 0670: // SLT Dn
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) != 0 ? 0xff : 0);
		case 0671: // DBLT Dn,<label>
			return dbcc(op, ((sr >> 2 ^ sr) & 2) != 0);
		case 0672: // SLT (An)
		case 0673: // SLT (An)+
		case 0674: // SLT -(An)
		case 0675: // SLT d(An)
		case 0676: // SLT d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) != 0 ? 0xff : 0);
		case 0677: // SLT Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 ^ sr) & 2) != 0 ? 0xff : 0);
		case 0700: // ADDQ.B #7,Dn
		case 0702: // ADDQ.B #7,(An)
		case 0703: // ADDQ.B #7,(An)+
		case 0704: // ADDQ.B #7,-(An)
		case 0705: // ADDQ.B #7,d(An)
		case 0706: // ADDQ.B #7,d(An,Xi)
			return rwop8(op, add8, 7);
		case 0707: // ADDQ.B #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, 7);
		case 0710: // ADDQ.W #7,Dn
			return rwop16(op, add16, 7);
		case 0711: // ADDQ.W #7,An
			return rwop16(op, adda32, 7);
		case 0712: // ADDQ.W #7,(An)
		case 0713: // ADDQ.W #7,(An)+
		case 0714: // ADDQ.W #7,-(An)
		case 0715: // ADDQ.W #7,d(An)
		case 0716: // ADDQ.W #7,d(An,Xi)
			return rwop16(op, add16, 7);
		case 0717: // ADDQ.W #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, 7);
		case 0720: // ADDQ.L #7,Dn
			return rwop32(op, add32, 7);
		case 0721: // ADDQ.L #7,An
			return rwop32(op, adda32, 7);
		case 0722: // ADDQ.L #7,(An)
		case 0723: // ADDQ.L #7,(An)+
		case 0724: // ADDQ.L #7,-(An)
		case 0725: // ADDQ.L #7,d(An)
		case 0726: // ADDQ.L #7,d(An,Xi)
			return rwop32(op, add32, 7);
		case 0727: // ADDQ.L #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, 7);
		case 0730: // SGT Dn
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) == 0 ? 0xff : 0);
		case 0731: // DBGT Dn,<label>
			return dbcc(op, ((sr >> 2 ^ sr | sr >> 1) & 2) == 0);
		case 0732: // SGT (An)
		case 0733: // SGT (An)+
		case 0734: // SGT -(An)
		case 0735: // SGT d(An)
		case 0736: // SGT d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) == 0 ? 0xff : 0);
		case 0737: // SGT Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) == 0 ? 0xff : 0);
		case 0740: // SUBQ.B #7,Dn
		case 0742: // SUBQ.B #7,(An)
		case 0743: // SUBQ.B #7,(An)+
		case 0744: // SUBQ.B #7,-(An)
		case 0745: // SUBQ.B #7,d(An)
		case 0746: // SUBQ.B #7,d(An,Xi)
			return rwop8(op, sub8, 7);
		case 0747: // SUBQ.B #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, 7);
		case 0750: // SUBQ.W #7,Dn
			return rwop16(op, sub16, 7);
		case 0751: // SUBQ.W #7,An
			return rwop16(op, suba32, 7);
		case 0752: // SUBQ.W #7,(An)
		case 0753: // SUBQ.W #7,(An)+
		case 0754: // SUBQ.W #7,-(An)
		case 0755: // SUBQ.W #7,d(An)
		case 0756: // SUBQ.W #7,d(An,Xi)
			return rwop16(op, sub16, 7);
		case 0757: // SUBQ.W #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, 7);
		case 0760: // SUBQ.L #7,Dn
			return rwop32(op, sub32, 7);
		case 0761: // SUBQ.L #7,An
			return rwop32(op, suba32, 7);
		case 0762: // SUBQ.L #7,(An)
		case 0763: // SUBQ.L #7,(An)+
		case 0764: // SUBQ.L #7,-(An)
		case 0765: // SUBQ.L #7,d(An)
		case 0766: // SUBQ.L #7,d(An,Xi)
			return rwop32(op, sub32, 7);
		case 0767: // SUBQ.L #7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, 7);
		case 0770: // SLE Dn
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) != 0 ? 0xff : 0);
		case 0771: // DBLE Dn,<label>
			return dbcc(op, ((sr >> 2 ^ sr | sr >> 1) & 2) != 0);
		case 0772: // SLE (An)
		case 0773: // SLE (An)+
		case 0774: // SLE -(An)
		case 0775: // SLE d(An)
		case 0776: // SLE d(An,Xi)
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) != 0 ? 0xff : 0);
		case 0777: // SLE Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, thru, ((sr >> 2 ^ sr | sr >> 1) & 2) != 0 ? 0xff : 0);
		default:
			return exception(4);
		}
	}

	void execute_6(int op) {
		const int addr = (op & 0xff) == 0 ? disp(pc) : pc + (op << 24 >> 24);
		switch (op >> 8 & 0xf) {
		case 0x0: // BRA <label>
			return void(pc = addr);
		case 0x1: // BSR <label>
			return a7 = a7 - 4, write32(pc, a7), void(pc = addr);
		case 0x2: // BHI <label>
			return void(((sr >> 2 | sr) & 1) == 0 && (pc = addr));
		case 0x3: // BLS <label>
			return void(((sr >> 2 | sr) & 1) != 0 && (pc = addr));
		case 0x4: // BCC <label>
			return void((sr & 1) == 0 && (pc = addr));
		case 0x5: // BCS <label>
			return void((sr & 1) != 0 && (pc = addr));
		case 0x6: // BNE <label>
			return void((sr & 4) == 0 && (pc = addr));
		case 0x7: // BEQ <label>
			return void((sr & 4) != 0 && (pc = addr));
		case 0x8: // BVC <label>
			return void((sr & 2) == 0 && (pc = addr));
		case 0x9: // BVS <label>
			return void((sr & 2) != 0 && (pc = addr));
		case 0xa: // BPL <label>
			return void((sr & 8) == 0 && (pc = addr));
		case 0xb: // BMI <label>
			return void((sr & 8) != 0 && (pc = addr));
		case 0xc: // BGE <label>
			return void(((sr >> 2 ^ sr) & 2) == 0 && (pc = addr));
		case 0xd: // BLT <label>
			return void(((sr >> 2 ^ sr) & 2) != 0 && (pc = addr));
		case 0xe: // BGT <label>
			return void(((sr >> 2 ^ sr | sr >> 1) & 2) == 0 && (pc = addr));
		case 0xf: // BLE <label>
			return void(((sr >> 2 ^ sr | sr >> 1) & 2) != 0 && (pc = addr));
		default:
			return;
		}
	}

	void execute_7(int op) {
		const int data = op << 24 >> 24;
		sr = sr & ~0x0f | data >> 28 & 8 | !data << 2;
		switch (op >> 8 & 0xf) {
		case 0x0: // MOVEQ #<data>,D0
			return void(d0 = data);
		case 0x2: // MOVEQ #<data>,D1
			return void(d1 = data);
		case 0x4: // MOVEQ #<data>,D2
			return void(d2 = data);
		case 0x6: // MOVEQ #<data>,D3
			return void(d3 = data);
		case 0x8: // MOVEQ #<data>,D4
			return void(d4 = data);
		case 0xa: // MOVEQ #<data>,D5
			return void(d5 = data);
		case 0xc: // MOVEQ #<data>,D6
			return void(d6 = data);
		case 0xe: // MOVEQ #<data>,D7
			return void(d7 = data);
		default:
			return exception(4);
		}
	}

	void execute_8(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // OR.B Dn,D0
		case 0002: // OR.B (An),D0
		case 0003: // OR.B (An)+,D0
		case 0004: // OR.B -(An),D0
		case 0005: // OR.B d(An),D0
		case 0006: // OR.B d(An,Xi),D0
			return void(d0 = d0 & ~0xff | or8(rop8(op), d0, sr));
		case 0007: // OR.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xff | or8(rop8(op), d0, sr));
		case 0010: // OR.W Dn,D0
		case 0012: // OR.W (An),D0
		case 0013: // OR.W (An)+,D0
		case 0014: // OR.W -(An),D0
		case 0015: // OR.W d(An),D0
		case 0016: // OR.W d(An,Xi),D0
			return void(d0 = d0 & ~0xffff | or16(rop16(op), d0, sr));
		case 0017: // OR.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xffff | or16(rop16(op), d0, sr));
		case 0020: // OR.L Dn,D0
		case 0022: // OR.L (An),D0
		case 0023: // OR.L (An)+,D0
		case 0024: // OR.L -(An),D0
		case 0025: // OR.L d(An),D0
		case 0026: // OR.L d(An,Xi),D0
			return void(d0 = or32(rop32(op), d0, sr));
		case 0027: // OR.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = or32(rop32(op), d0, sr));
		case 0030: // DIVU Dn,D0
		case 0032: // DIVU (An),D0
		case 0033: // DIVU (An)+,D0
		case 0034: // DIVU -(An),D0
		case 0035: // DIVU d(An),D0
		case 0036: // DIVU d(An,Xi),D0
			return void(d0 = divu(rop16(op), d0));
		case 0037: // DIVU Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = divu(rop16(op), d0));
		case 0040: // SBCD Dy,D0
			return void(d0 = d0 & ~0xff | sbcd(rop8(op), d0, sr));
		case 0041: // SBCD -(Ay),-(A0)
			return src = rop8(op & 7 | 040), a0 = a0 - 1, write8(sbcd(src, read8(a0), sr), a0);
		case 0042: // OR.B D0,(An)
		case 0043: // OR.B D0,(An)+
		case 0044: // OR.B D0,-(An)
		case 0045: // OR.B D0,d(An)
		case 0046: // OR.B D0,d(An,Xi)
			return rwop8(op, or8, d0);
		case 0047: // OR.B D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d0);
		case 0052: // OR.W D0,(An)
		case 0053: // OR.W D0,(An)+
		case 0054: // OR.W D0,-(An)
		case 0055: // OR.W D0,d(An)
		case 0056: // OR.W D0,d(An,Xi)
			return rwop16(op, or16, d0);
		case 0057: // OR.W D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d0);
		case 0062: // OR.L D0,(An)
		case 0063: // OR.L D0,(An)+
		case 0064: // OR.L D0,-(An)
		case 0065: // OR.L D0,d(An)
		case 0066: // OR.L D0,d(An,Xi)
			return rwop32(op, or32, d0);
		case 0067: // OR.L D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d0);
		case 0070: // DIVS Dn,D0
		case 0072: // DIVS (An),D0
		case 0073: // DIVS (An)+,D0
		case 0074: // DIVS -(An),D0
		case 0075: // DIVS d(An),D0
		case 0076: // DIVS d(An,Xi),D0
			return void(d0 = divs(rop16(op), d0));
		case 0077: // DIVS Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = divs(rop16(op), d0));
		case 0100: // OR.B Dn,D1
		case 0102: // OR.B (An),D1
		case 0103: // OR.B (An)+,D1
		case 0104: // OR.B -(An),D1
		case 0105: // OR.B d(An),D1
		case 0106: // OR.B d(An,Xi),D1
			return void(d1 = d1 & ~0xff | or8(rop8(op), d1, sr));
		case 0107: // OR.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xff | or8(rop8(op), d1, sr));
		case 0110: // OR.W Dn,D1
		case 0112: // OR.W (An),D1
		case 0113: // OR.W (An)+,D1
		case 0114: // OR.W -(An),D1
		case 0115: // OR.W d(An),D1
		case 0116: // OR.W d(An,Xi),D1
			return void(d1 = d1 & ~0xffff | or16(rop16(op), d1, sr));
		case 0117: // OR.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xffff | or16(rop16(op), d1, sr));
		case 0120: // OR.L Dn,D1
		case 0122: // OR.L (An),D1
		case 0123: // OR.L (An)+,D1
		case 0124: // OR.L -(An),D1
		case 0125: // OR.L d(An),D1
		case 0126: // OR.L d(An,Xi),D1
			return void(d1 = or32(rop32(op), d1, sr));
		case 0127: // OR.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = or32(rop32(op), d1, sr));
		case 0130: // DIVU Dn,D1
		case 0132: // DIVU (An),D1
		case 0133: // DIVU (An)+,D1
		case 0134: // DIVU -(An),D1
		case 0135: // DIVU d(An),D1
		case 0136: // DIVU d(An,Xi),D1
			return void(d1 = divu(rop16(op), d1));
		case 0137: // DIVU Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = divu(rop16(op), d1));
		case 0140: // SBCD Dy,D1
			return void(d1 = d1 & ~0xff | sbcd(rop8(op), d1, sr));
		case 0141: // SBCD -(Ay),-(A1)
			return src = rop8(op & 7 | 040), a1 = a1 - 1, write8(sbcd(src, read8(a1), sr), a1);
		case 0142: // OR.B D1,(An)
		case 0143: // OR.B D1,(An)+
		case 0144: // OR.B D1,-(An)
		case 0145: // OR.B D1,d(An)
		case 0146: // OR.B D1,d(An,Xi)
			return rwop8(op, or8, d1);
		case 0147: // OR.B D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d1);
		case 0152: // OR.W D1,(An)
		case 0153: // OR.W D1,(An)+
		case 0154: // OR.W D1,-(An)
		case 0155: // OR.W D1,d(An)
		case 0156: // OR.W D1,d(An,Xi)
			return rwop16(op, or16, d1);
		case 0157: // OR.W D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d1);
		case 0162: // OR.L D1,(An)
		case 0163: // OR.L D1,(An)+
		case 0164: // OR.L D1,-(An)
		case 0165: // OR.L D1,d(An)
		case 0166: // OR.L D1,d(An,Xi)
			return rwop32(op, or32, d1);
		case 0167: // OR.L D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d1);
		case 0170: // DIVS Dn,D1
		case 0172: // DIVS (An),D1
		case 0173: // DIVS (An)+,D1
		case 0174: // DIVS -(An),D1
		case 0175: // DIVS d(An),D1
		case 0176: // DIVS d(An,Xi),D1
			return void(d1 = divs(rop16(op), d1));
		case 0177: // DIVS Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = divs(rop16(op), d1));
		case 0200: // OR.B Dn,D2
		case 0202: // OR.B (An),D2
		case 0203: // OR.B (An)+,D2
		case 0204: // OR.B -(An),D2
		case 0205: // OR.B d(An),D2
		case 0206: // OR.B d(An,Xi),D2
			return void(d2 = d2 & ~0xff | or8(rop8(op), d2, sr));
		case 0207: // OR.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xff | or8(rop8(op), d2, sr));
		case 0210: // OR.W Dn,D2
		case 0212: // OR.W (An),D2
		case 0213: // OR.W (An)+,D2
		case 0214: // OR.W -(An),D2
		case 0215: // OR.W d(An),D2
		case 0216: // OR.W d(An,Xi),D2
			return void(d2 = d2 & ~0xffff | or16(rop16(op), d2, sr));
		case 0217: // OR.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xffff | or16(rop16(op), d2, sr));
		case 0220: // OR.L Dn,D2
		case 0222: // OR.L (An),D2
		case 0223: // OR.L (An)+,D2
		case 0224: // OR.L -(An),D2
		case 0225: // OR.L d(An),D2
		case 0226: // OR.L d(An,Xi),D2
			return void(d2 = or32(rop32(op), d2, sr));
		case 0227: // OR.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = or32(rop32(op), d2, sr));
		case 0230: // DIVU Dn,D2
		case 0232: // DIVU (An),D2
		case 0233: // DIVU (An)+,D2
		case 0234: // DIVU -(An),D2
		case 0235: // DIVU d(An),D2
		case 0236: // DIVU d(An,Xi),D2
			return void(d2 = divu(rop16(op), d2));
		case 0237: // DIVU Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = divu(rop16(op), d2));
		case 0240: // SBCD Dy,D2
			return void(d2 = d2 & ~0xff | sbcd(rop8(op), d2, sr));
		case 0241: // SBCD -(Ay),-(A2)
			return src = rop8(op & 7 | 040), a2 = a2 - 1, write8(sbcd(src, read8(a2), sr), a2);
		case 0242: // OR.B D2,(An)
		case 0243: // OR.B D2,(An)+
		case 0244: // OR.B D2,-(An)
		case 0245: // OR.B D2,d(An)
		case 0246: // OR.B D2,d(An,Xi)
			return rwop8(op, or8, d2);
		case 0247: // OR.B D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d2);
		case 0252: // OR.W D2,(An)
		case 0253: // OR.W D2,(An)+
		case 0254: // OR.W D2,-(An)
		case 0255: // OR.W D2,d(An)
		case 0256: // OR.W D2,d(An,Xi)
			return rwop16(op, or16, d2);
		case 0257: // OR.W D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d2);
		case 0262: // OR.L D2,(An)
		case 0263: // OR.L D2,(An)+
		case 0264: // OR.L D2,-(An)
		case 0265: // OR.L D2,d(An)
		case 0266: // OR.L D2,d(An,Xi)
			return rwop32(op, or32, d2);
		case 0267: // OR.L D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d2);
		case 0270: // DIVS Dn,D2
		case 0272: // DIVS (An),D2
		case 0273: // DIVS (An)+,D2
		case 0274: // DIVS -(An),D2
		case 0275: // DIVS d(An),D2
		case 0276: // DIVS d(An,Xi),D2
			return void(d2 = divs(rop16(op), d2));
		case 0277: // DIVS Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = divs(rop16(op), d2));
		case 0300: // OR.B Dn,D3
		case 0302: // OR.B (An),D3
		case 0303: // OR.B (An)+,D3
		case 0304: // OR.B -(An),D3
		case 0305: // OR.B d(An),D3
		case 0306: // OR.B d(An,Xi),D3
			return void(d3 = d3 & ~0xff | or8(rop8(op), d3, sr));
		case 0307: // OR.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xff | or8(rop8(op), d3, sr));
		case 0310: // OR.W Dn,D3
		case 0312: // OR.W (An),D3
		case 0313: // OR.W (An)+,D3
		case 0314: // OR.W -(An),D3
		case 0315: // OR.W d(An),D3
		case 0316: // OR.W d(An,Xi),D3
			return void(d3 = d3 & ~0xffff | or16(rop16(op), d3, sr));
		case 0317: // OR.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xffff | or16(rop16(op), d3, sr));
		case 0320: // OR.L Dn,D3
		case 0322: // OR.L (An),D3
		case 0323: // OR.L (An)+,D3
		case 0324: // OR.L -(An),D3
		case 0325: // OR.L d(An),D3
		case 0326: // OR.L d(An,Xi),D3
			return void(d3 = or32(rop32(op), d3, sr));
		case 0327: // OR.L Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = or32(rop32(op), d3, sr));
		case 0330: // DIVU Dn,D3
		case 0332: // DIVU (An),D3
		case 0333: // DIVU (An)+,D3
		case 0334: // DIVU -(An),D3
		case 0335: // DIVU d(An),D3
		case 0336: // DIVU d(An,Xi),D3
			return void(d3 = divu(rop16(op), d3));
		case 0337: // DIVU Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = divu(rop16(op), d3));
		case 0340: // SBCD Dy,D3
			return void(d3 = d3 & ~0xff | sbcd(rop8(op), d3, sr));
		case 0341: // SBCD -(Ay),-(A3)
			return src = rop8(op & 7 | 040), a3 = a3 - 1, write8(sbcd(src, read8(a3), sr), a3);
		case 0342: // OR.B D3,(An)
		case 0343: // OR.B D3,(An)+
		case 0344: // OR.B D3,-(An)
		case 0345: // OR.B D3,d(An)
		case 0346: // OR.B D3,d(An,Xi)
			return rwop8(op, or8, d3);
		case 0347: // OR.B D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d3);
		case 0352: // OR.W D3,(An)
		case 0353: // OR.W D3,(An)+
		case 0354: // OR.W D3,-(An)
		case 0355: // OR.W D3,d(An)
		case 0356: // OR.W D3,d(An,Xi)
			return rwop16(op, or16, d3);
		case 0357: // OR.W D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d3);
		case 0362: // OR.L D3,(An)
		case 0363: // OR.L D3,(An)+
		case 0364: // OR.L D3,-(An)
		case 0365: // OR.L D3,d(An)
		case 0366: // OR.L D3,d(An,Xi)
			return rwop32(op, or32, d3);
		case 0367: // OR.L D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d3);
		case 0370: // DIVS Dn,D3
		case 0372: // DIVS (An),D3
		case 0373: // DIVS (An)+,D3
		case 0374: // DIVS -(An),D3
		case 0375: // DIVS d(An),D3
		case 0376: // DIVS d(An,Xi),D3
			return void(d3 = divs(rop16(op), d3));
		case 0377: // DIVS Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = divs(rop16(op), d3));
		case 0400: // OR.B Dn,D4
		case 0402: // OR.B (An),D4
		case 0403: // OR.B (An)+,D4
		case 0404: // OR.B -(An),D4
		case 0405: // OR.B d(An),D4
		case 0406: // OR.B d(An,Xi),D4
			return void(d4 = d4 & ~0xff | or8(rop8(op), d4, sr));
		case 0407: // OR.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xff | or8(rop8(op), d4, sr));
		case 0410: // OR.W Dn,D4
		case 0412: // OR.W (An),D4
		case 0413: // OR.W (An)+,D4
		case 0414: // OR.W -(An),D4
		case 0415: // OR.W d(An),D4
		case 0416: // OR.W d(An,Xi),D4
			return void(d4 = d4 & ~0xffff | or16(rop16(op), d4, sr));
		case 0417: // OR.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xffff | or16(rop16(op), d4, sr));
		case 0420: // OR.L Dn,D4
		case 0422: // OR.L (An),D4
		case 0423: // OR.L (An)+,D4
		case 0424: // OR.L -(An),D4
		case 0425: // OR.L d(An),D4
		case 0426: // OR.L d(An,Xi),D4
			return void(d4 = or32(rop32(op), d4, sr));
		case 0427: // OR.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = or32(rop32(op), d4, sr));
		case 0430: // DIVU Dn,D4
		case 0432: // DIVU (An),D4
		case 0433: // DIVU (An)+,D4
		case 0434: // DIVU -(An),D4
		case 0435: // DIVU d(An),D4
		case 0436: // DIVU d(An,Xi),D4
			return void(d4 = divu(rop16(op), d4));
		case 0437: // DIVU Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = divu(rop16(op), d4));
		case 0440: // SBCD Dy,D4
			return void(d4 = d4 & ~0xff | sbcd(rop8(op), d4, sr));
		case 0441: // SBCD -(Ay),-(A4)
			return src = rop8(op & 7 | 040), a4 = a4 - 1, write8(sbcd(src, read8(a4), sr), a4);
		case 0442: // OR.B D4,(An)
		case 0443: // OR.B D4,(An)+
		case 0444: // OR.B D4,-(An)
		case 0445: // OR.B D4,d(An)
		case 0446: // OR.B D4,d(An,Xi)
			return rwop8(op, or8, d4);
		case 0447: // OR.B D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d4);
		case 0452: // OR.W D4,(An)
		case 0453: // OR.W D4,(An)+
		case 0454: // OR.W D4,-(An)
		case 0455: // OR.W D4,d(An)
		case 0456: // OR.W D4,d(An,Xi)
			return rwop16(op, or16, d4);
		case 0457: // OR.W D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d4);
		case 0462: // OR.L D4,(An)
		case 0463: // OR.L D4,(An)+
		case 0464: // OR.L D4,-(An)
		case 0465: // OR.L D4,d(An)
		case 0466: // OR.L D4,d(An,Xi)
			return rwop32(op, or32, d4);
		case 0467: // OR.L D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d4);
		case 0470: // DIVS Dn,D4
		case 0472: // DIVS (An),D4
		case 0473: // DIVS (An)+,D4
		case 0474: // DIVS -(An),D4
		case 0475: // DIVS d(An),D4
		case 0476: // DIVS d(An,Xi),D4
			return void(d4 = divs(rop16(op), d4));
		case 0477: // DIVS Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = divs(rop16(op), d4));
		case 0500: // OR.B Dn,D5
		case 0502: // OR.B (An),D5
		case 0503: // OR.B (An)+,D5
		case 0504: // OR.B -(An),D5
		case 0505: // OR.B d(An),D5
		case 0506: // OR.B d(An,Xi),D5
			return void(d5 = d5 & ~0xff | or8(rop8(op), d5, sr));
		case 0507: // OR.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xff | or8(rop8(op), d5, sr));
		case 0510: // OR.W Dn,D5
		case 0512: // OR.W (An),D5
		case 0513: // OR.W (An)+,D5
		case 0514: // OR.W -(An),D5
		case 0515: // OR.W d(An),D5
		case 0516: // OR.W d(An,Xi),D5
			return void(d5 = d5 & ~0xffff | or16(rop16(op), d5, sr));
		case 0517: // OR.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xffff | or16(rop16(op), d5, sr));
		case 0520: // OR.L Dn,D5
		case 0522: // OR.L (An),D5
		case 0523: // OR.L (An)+,D5
		case 0524: // OR.L -(An),D5
		case 0525: // OR.L d(An),D5
		case 0526: // OR.L d(An,Xi),D5
			return void(d5 = or32(rop32(op), d5, sr));
		case 0527: // OR.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = or32(rop32(op), d5, sr));
		case 0530: // DIVU Dn,D5
		case 0532: // DIVU (An),D5
		case 0533: // DIVU (An)+,D5
		case 0534: // DIVU -(An),D5
		case 0535: // DIVU d(An),D5
		case 0536: // DIVU d(An,Xi),D5
			return void(d5 = divu(rop16(op), d5));
		case 0537: // DIVU Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = divu(rop16(op), d5));
		case 0540: // SBCD Dy,D5
			return void(d5 = d5 & ~0xff | sbcd(rop8(op), d5, sr));
		case 0541: // SBCD -(Ay),-(A5)
			return src = rop8(op & 7 | 040), a5 = a5 - 1, write8(sbcd(src, read8(a5), sr), a5);
		case 0542: // OR.B D5,(An)
		case 0543: // OR.B D5,(An)+
		case 0544: // OR.B D5,-(An)
		case 0545: // OR.B D5,d(An)
		case 0546: // OR.B D5,d(An,Xi)
			return rwop8(op, or8, d5);
		case 0547: // OR.B D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d5);
		case 0552: // OR.W D5,(An)
		case 0553: // OR.W D5,(An)+
		case 0554: // OR.W D5,-(An)
		case 0555: // OR.W D5,d(An)
		case 0556: // OR.W D5,d(An,Xi)
			return rwop16(op, or16, d5);
		case 0557: // OR.W D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d5);
		case 0562: // OR.L D5,(An)
		case 0563: // OR.L D5,(An)+
		case 0564: // OR.L D5,-(An)
		case 0565: // OR.L D5,d(An)
		case 0566: // OR.L D5,d(An,Xi)
			return rwop32(op, or32, d5);
		case 0567: // OR.L D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d5);
		case 0570: // DIVS Dn,D5
		case 0572: // DIVS (An),D5
		case 0573: // DIVS (An)+,D5
		case 0574: // DIVS -(An),D5
		case 0575: // DIVS d(An),D5
		case 0576: // DIVS d(An,Xi),D5
			return void(d5 = divs(rop16(op), d5));
		case 0577: // DIVS Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = divs(rop16(op), d5));
		case 0600: // OR.B Dn,D6
		case 0602: // OR.B (An),D6
		case 0603: // OR.B (An)+,D6
		case 0604: // OR.B -(An),D6
		case 0605: // OR.B d(An),D6
		case 0606: // OR.B d(An,Xi),D6
			return void(d6 = d6 & ~0xff | or8(rop8(op), d6, sr));
		case 0607: // OR.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xff | or8(rop8(op), d6, sr));
		case 0610: // OR.W Dn,D6
		case 0612: // OR.W (An),D6
		case 0613: // OR.W (An)+,D6
		case 0614: // OR.W -(An),D6
		case 0615: // OR.W d(An),D6
		case 0616: // OR.W d(An,Xi),D6
			return void(d6 = d6 & ~0xffff | or16(rop16(op), d6, sr));
		case 0617: // OR.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xffff | or16(rop16(op), d6, sr));
		case 0620: // OR.L Dn,D6
		case 0622: // OR.L (An),D6
		case 0623: // OR.L (An)+,D6
		case 0624: // OR.L -(An),D6
		case 0625: // OR.L d(An),D6
		case 0626: // OR.L d(An,Xi),D6
			return void(d6 = or32(rop32(op), d6, sr));
		case 0627: // OR.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = or32(rop32(op), d6, sr));
		case 0630: // DIVU Dn,D6
		case 0632: // DIVU (An),D6
		case 0633: // DIVU (An)+,D6
		case 0634: // DIVU -(An),D6
		case 0635: // DIVU d(An),D6
		case 0636: // DIVU d(An,Xi),D6
			return void(d6 = divu(rop16(op), d6));
		case 0637: // DIVU Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = divu(rop16(op), d6));
		case 0640: // SBCD Dy,D6
			return void(d6 = d6 & ~0xff | sbcd(rop8(op), d6, sr));
		case 0641: // SBCD -(Ay),-(A6)
			return src = rop8(op & 7 | 040), a6 = a6 - 1, write8(sbcd(src, read8(a6), sr), a6);
		case 0642: // OR.B D6,(An)
		case 0643: // OR.B D6,(An)+
		case 0644: // OR.B D6,-(An)
		case 0645: // OR.B D6,d(An)
		case 0646: // OR.B D6,d(An,Xi)
			return rwop8(op, or8, d6);
		case 0647: // OR.B D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d6);
		case 0652: // OR.W D6,(An)
		case 0653: // OR.W D6,(An)+
		case 0654: // OR.W D6,-(An)
		case 0655: // OR.W D6,d(An)
		case 0656: // OR.W D6,d(An,Xi)
			return rwop16(op, or16, d6);
		case 0657: // OR.W D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d6);
		case 0662: // OR.L D6,(An)
		case 0663: // OR.L D6,(An)+
		case 0664: // OR.L D6,-(An)
		case 0665: // OR.L D6,d(An)
		case 0666: // OR.L D6,d(An,Xi)
			return rwop32(op, or32, d6);
		case 0667: // OR.L D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d6);
		case 0670: // DIVS Dn,D6
		case 0672: // DIVS (An),D6
		case 0673: // DIVS (An)+,D6
		case 0674: // DIVS -(An),D6
		case 0675: // DIVS d(An),D6
		case 0676: // DIVS d(An,Xi),D6
			return void(d6 = divs(rop16(op), d6));
		case 0677: // DIVS Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = divs(rop16(op), d6));
		case 0700: // OR.B Dn,D7
		case 0702: // OR.B (An),D7
		case 0703: // OR.B (An)+,D7
		case 0704: // OR.B -(An),D7
		case 0705: // OR.B d(An),D7
		case 0706: // OR.B d(An,Xi),D7
			return void(d7 = d7 & ~0xff | or8(rop8(op), d7, sr));
		case 0707: // OR.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xff | or8(rop8(op), d7, sr));
		case 0710: // OR.W Dn,D7
		case 0712: // OR.W (An),D7
		case 0713: // OR.W (An)+,D7
		case 0714: // OR.W -(An),D7
		case 0715: // OR.W d(An),D7
		case 0716: // OR.W d(An,Xi),D7
			return void(d7 = d7 & ~0xffff | or16(rop16(op), d7, sr));
		case 0717: // OR.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xffff | or16(rop16(op), d7, sr));
		case 0720: // OR.L Dn,D7
		case 0722: // OR.L (An),D7
		case 0723: // OR.L (An)+,D7
		case 0724: // OR.L -(An),D7
		case 0725: // OR.L d(An),D7
		case 0726: // OR.L d(An,Xi),D7
			return void(d7 = or32(rop32(op), d7, sr));
		case 0727: // OR.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = or32(rop32(op), d7, sr));
		case 0730: // DIVU Dn,D7
		case 0732: // DIVU (An),D7
		case 0733: // DIVU (An)+,D7
		case 0734: // DIVU -(An),D7
		case 0735: // DIVU d(An),D7
		case 0736: // DIVU d(An,Xi),D7
			return void(d7 = divu(rop16(op), d7));
		case 0737: // DIVU Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = divu(rop16(op), d7));
		case 0740: // SBCD Dy,D7
			return void(d7 = d7 & ~0xff | sbcd(rop8(op), d7, sr));
		case 0741: // SBCD -(Ay),-(A7)
			return src = rop8(op & 7 | 040), a7 = a7 - 1, write8(sbcd(src, read8(a7), sr), a7);
		case 0742: // OR.B D7,(An)
		case 0743: // OR.B D7,(An)+
		case 0744: // OR.B D7,-(An)
		case 0745: // OR.B D7,d(An)
		case 0746: // OR.B D7,d(An,Xi)
			return rwop8(op, or8, d7);
		case 0747: // OR.B D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, or8, d7);
		case 0752: // OR.W D7,(An)
		case 0753: // OR.W D7,(An)+
		case 0754: // OR.W D7,-(An)
		case 0755: // OR.W D7,d(An)
		case 0756: // OR.W D7,d(An,Xi)
			return rwop16(op, or16, d7);
		case 0757: // OR.W D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, or16, d7);
		case 0762: // OR.L D7,(An)
		case 0763: // OR.L D7,(An)+
		case 0764: // OR.L D7,-(An)
		case 0765: // OR.L D7,d(An)
		case 0766: // OR.L D7,d(An,Xi)
			return rwop32(op, or32, d7);
		case 0767: // OR.L D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, or32, d7);
		case 0770: // DIVS Dn,D7
		case 0772: // DIVS (An),D7
		case 0773: // DIVS (An)+,D7
		case 0774: // DIVS -(An),D7
		case 0775: // DIVS d(An),D7
		case 0776: // DIVS d(An,Xi),D7
			return void(d7 = divs(rop16(op), d7));
		case 0777: // DIVS Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = divs(rop16(op), d7));
		default:
			return exception(4);
		}
	}

	void execute_9(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // SUB.B Dn,D0
		case 0002: // SUB.B (An),D0
		case 0003: // SUB.B (An)+,D0
		case 0004: // SUB.B -(An),D0
		case 0005: // SUB.B d(An),D0
		case 0006: // SUB.B d(An,Xi),D0
			return void(d0 = d0 & ~0xff | sub8(rop8(op), d0, sr));
		case 0007: // SUB.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xff | sub8(rop8(op), d0, sr));
		case 0010: // SUB.W Dn,D0
		case 0011: // SUB.W An,D0
		case 0012: // SUB.W (An),D0
		case 0013: // SUB.W (An)+,D0
		case 0014: // SUB.W -(An),D0
		case 0015: // SUB.W d(An),D0
		case 0016: // SUB.W d(An,Xi),D0
			return void(d0 = d0 & ~0xffff | sub16(rop16(op), d0, sr));
		case 0017: // SUB.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xffff | sub16(rop16(op), d0, sr));
		case 0020: // SUB.L Dn,D0
		case 0021: // SUB.L An,D0
		case 0022: // SUB.L (An),D0
		case 0023: // SUB.L (An)+,D0
		case 0024: // SUB.L -(An),D0
		case 0025: // SUB.L d(An),D0
		case 0026: // SUB.L d(An,Xi),D0
			return void(d0 = sub32(rop32(op), d0, sr));
		case 0027: // SUB.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = sub32(rop32(op), d0, sr));
		case 0030: // SUBA.W Dn,A0
		case 0031: // SUBA.W An,A0
		case 0032: // SUBA.W (An),A0
		case 0033: // SUBA.W (An)+,A0
		case 0034: // SUBA.W -(An),A0
		case 0035: // SUBA.W d(An),A0
		case 0036: // SUBA.W d(An,Xi),A0
			return void(a0 = a0 - (rop16(op) << 16 >> 16));
		case 0037: // SUBA.W Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = a0 - (rop16(op) << 16 >> 16));
		case 0040: // SUBX.B Dy,D0
			return void(d0 = d0 & ~0xff | subx8(rop8(op), d0, sr));
		case 0041: // SUBX.B -(Ay),-(A0)
			return src = rop8(op & 7 | 040), a0 = a0 - 1, write8(subx8(src, read8(a0), sr), a0);
		case 0042: // SUB.B D0,(An)
		case 0043: // SUB.B D0,(An)+
		case 0044: // SUB.B D0,-(An)
		case 0045: // SUB.B D0,d(An)
		case 0046: // SUB.B D0,d(An,Xi)
			return rwop8(op, sub8, d0);
		case 0047: // SUB.B D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d0);
		case 0050: // SUBX.W Dy,D0
			return void(d0 = d0 & ~0xffff | subx16(rop16(op), d0, sr));
		case 0051: // SUBX.W -(Ay),-(A0)
			return src = rop16(op & 7 | 040), a0 = a0 - 2, write16(subx16(src, read16(a0), sr), a0);
		case 0052: // SUB.W D0,(An)
		case 0053: // SUB.W D0,(An)+
		case 0054: // SUB.W D0,-(An)
		case 0055: // SUB.W D0,d(An)
		case 0056: // SUB.W D0,d(An,Xi)
			return rwop16(op, sub16, d0);
		case 0057: // SUB.W D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d0);
		case 0060: // SUBX.L Dy,D0
			return void(d0 = subx32(rop32(op), d0, sr));
		case 0061: // SUBX.L -(Ay),-(A0)
			return src = rop32(op & 7 | 040), a0 = a0 - 4, write32(subx32(src, read32(a0), sr), a0);
		case 0062: // SUB.L D0,(An)
		case 0063: // SUB.L D0,(An)+
		case 0064: // SUB.L D0,-(An)
		case 0065: // SUB.L D0,d(An)
		case 0066: // SUB.L D0,d(An,Xi)
			return rwop32(op, sub32, d0);
		case 0067: // SUB.L D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d0);
		case 0070: // SUBA.L Dn,A0
		case 0071: // SUBA.L An,A0
		case 0072: // SUBA.L (An),A0
		case 0073: // SUBA.L (An)+,A0
		case 0074: // SUBA.L -(An),A0
		case 0075: // SUBA.L d(An),A0
		case 0076: // SUBA.L d(An,Xi),A0
			return void(a0 = a0 - rop32(op));
		case 0077: // SUBA.L Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = a0 - rop32(op));
		case 0100: // SUB.B Dn,D1
		case 0102: // SUB.B (An),D1
		case 0103: // SUB.B (An)+,D1
		case 0104: // SUB.B -(An),D1
		case 0105: // SUB.B d(An),D1
		case 0106: // SUB.B d(An,Xi),D1
			return void(d1 = d1 & ~0xff | sub8(rop8(op), d1, sr));
		case 0107: // SUB.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xff | sub8(rop8(op), d1, sr));
		case 0110: // SUB.W Dn,D1
		case 0111: // SUB.W An,D1
		case 0112: // SUB.W (An),D1
		case 0113: // SUB.W (An)+,D1
		case 0114: // SUB.W -(An),D1
		case 0115: // SUB.W d(An),D1
		case 0116: // SUB.W d(An,Xi),D1
			return void(d1 = d1 & ~0xffff | sub16(rop16(op), d1, sr));
		case 0117: // SUB.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xffff | sub16(rop16(op), d1, sr));
		case 0120: // SUB.L Dn,D1
		case 0121: // SUB.L An,D1
		case 0122: // SUB.L (An),D1
		case 0123: // SUB.L (An)+,D1
		case 0124: // SUB.L -(An),D1
		case 0125: // SUB.L d(An),D1
		case 0126: // SUB.L d(An,Xi),D1
			return void(d1 = sub32(rop32(op), d1, sr));
		case 0127: // SUB.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = sub32(rop32(op), d1, sr));
		case 0130: // SUBA.W Dn,A1
		case 0131: // SUBA.W An,A1
		case 0132: // SUBA.W (An),A1
		case 0133: // SUBA.W (An)+,A1
		case 0134: // SUBA.W -(An),A1
		case 0135: // SUBA.W d(An),A1
		case 0136: // SUBA.W d(An,Xi),A1
			return void(a1 = a1 - (rop16(op) << 16 >> 16));
		case 0137: // SUBA.W Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = a1 - (rop16(op) << 16 >> 16));
		case 0140: // SUBX.B Dy,D1
			return void(d1 = d1 & ~0xff | subx8(rop8(op), d1, sr));
		case 0141: // SUBX.B -(Ay),-(A1)
			return src = rop8(op & 7 | 040), a1 = a1 - 1, write8(subx8(src, read8(a1), sr), a1);
		case 0142: // SUB.B D1,(An)
		case 0143: // SUB.B D1,(An)+
		case 0144: // SUB.B D1,-(An)
		case 0145: // SUB.B D1,d(An)
		case 0146: // SUB.B D1,d(An,Xi)
			return rwop8(op, sub8, d1);
		case 0147: // SUB.B D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d1);
		case 0150: // SUBX.W Dy,D1
			return void(d1 = d1 & ~0xffff | subx16(rop16(op), d1, sr));
		case 0151: // SUBX.W -(Ay),-(A1)
			return src = rop16(op & 7 | 040), a1 = a1 - 2, write16(subx16(src, read16(a1), sr), a1);
		case 0152: // SUB.W D1,(An)
		case 0153: // SUB.W D1,(An)+
		case 0154: // SUB.W D1,-(An)
		case 0155: // SUB.W D1,d(An)
		case 0156: // SUB.W D1,d(An,Xi)
			return rwop16(op, sub16, d1);
		case 0157: // SUB.W D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d1);
		case 0160: // SUBX.L Dy,D1
			return void(d1 = subx32(rop32(op), d1, sr));
		case 0161: // SUBX.L -(Ay),-(A1)
			return src = rop32(op & 7 | 040), a1 = a1 - 4, write32(subx32(src, read32(a1), sr), a1);
		case 0162: // SUB.L D1,(An)
		case 0163: // SUB.L D1,(An)+
		case 0164: // SUB.L D1,-(An)
		case 0165: // SUB.L D1,d(An)
		case 0166: // SUB.L D1,d(An,Xi)
			return rwop32(op, sub32, d1);
		case 0167: // SUB.L D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d1);
		case 0170: // SUBA.L Dn,A1
		case 0171: // SUBA.L An,A1
		case 0172: // SUBA.L (An),A1
		case 0173: // SUBA.L (An)+,A1
		case 0174: // SUBA.L -(An),A1
		case 0175: // SUBA.L d(An),A1
		case 0176: // SUBA.L d(An,Xi),A1
			return void(a1 = a1 - rop32(op));
		case 0177: // SUBA.L Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = a1 - rop32(op));
		case 0200: // SUB.B Dn,D2
		case 0202: // SUB.B (An),D2
		case 0203: // SUB.B (An)+,D2
		case 0204: // SUB.B -(An),D2
		case 0205: // SUB.B d(An),D2
		case 0206: // SUB.B d(An,Xi),D2
			return void(d2 = d2 & ~0xff | sub8(rop8(op), d2, sr));
		case 0207: // SUB.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xff | sub8(rop8(op), d2, sr));
		case 0210: // SUB.W Dn,D2
		case 0211: // SUB.W An,D2
		case 0212: // SUB.W (An),D2
		case 0213: // SUB.W (An)+,D2
		case 0214: // SUB.W -(An),D2
		case 0215: // SUB.W d(An),D2
		case 0216: // SUB.W d(An,Xi),D2
			return void(d2 = d2 & ~0xffff | sub16(rop16(op), d2, sr));
		case 0217: // SUB.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xffff | sub16(rop16(op), d2, sr));
		case 0220: // SUB.L Dn,D2
		case 0221: // SUB.L An,D2
		case 0222: // SUB.L (An),D2
		case 0223: // SUB.L (An)+,D2
		case 0224: // SUB.L -(An),D2
		case 0225: // SUB.L d(An),D2
		case 0226: // SUB.L d(An,Xi),D2
			return void(d2 = sub32(rop32(op), d2, sr));
		case 0227: // SUB.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = sub32(rop32(op), d2, sr));
		case 0230: // SUBA.W Dn,A2
		case 0231: // SUBA.W An,A2
		case 0232: // SUBA.W (An),A2
		case 0233: // SUBA.W (An)+,A2
		case 0234: // SUBA.W -(An),A2
		case 0235: // SUBA.W d(An),A2
		case 0236: // SUBA.W d(An,Xi),A2
			return void(a2 = a2 - (rop16(op) << 16 >> 16));
		case 0237: // SUBA.W Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = a2 - (rop16(op) << 16 >> 16));
		case 0240: // SUBX.B Dy,D2
			return void(d2 = d2 & ~0xff | subx8(rop8(op), d2, sr));
		case 0241: // SUBX.B -(Ay),-(A2)
			return src = rop8(op & 7 | 040), a2 = a2 - 1, write8(subx8(src, read8(a2), sr), a2);
		case 0242: // SUB.B D2,(An)
		case 0243: // SUB.B D2,(An)+
		case 0244: // SUB.B D2,-(An)
		case 0245: // SUB.B D2,d(An)
		case 0246: // SUB.B D2,d(An,Xi)
			return rwop8(op, sub8, d2);
		case 0247: // SUB.B D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d2);
		case 0250: // SUBX.W Dy,D2
			return void(d2 = d2 & ~0xffff | subx16(rop16(op), d2, sr));
		case 0251: // SUBX.W -(Ay),-(A2)
			return src = rop16(op & 7 | 040), a2 = a2 - 2, write16(subx16(src, read16(a2), sr), a2);
		case 0252: // SUB.W D2,(An)
		case 0253: // SUB.W D2,(An)+
		case 0254: // SUB.W D2,-(An)
		case 0255: // SUB.W D2,d(An)
		case 0256: // SUB.W D2,d(An,Xi)
			return rwop16(op, sub16, d2);
		case 0257: // SUB.W D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d2);
		case 0260: // SUBX.L Dy,D2
			return void(d2 = subx32(rop32(op), d2, sr));
		case 0261: // SUBX.L -(Ay),-(A2)
			return src = rop32(op & 7 | 040), a2 = a2 - 4, write32(subx32(src, read32(a2), sr), a2);
		case 0262: // SUB.L D2,(An)
		case 0263: // SUB.L D2,(An)+
		case 0264: // SUB.L D2,-(An)
		case 0265: // SUB.L D2,d(An)
		case 0266: // SUB.L D2,d(An,Xi)
			return rwop32(op, sub32, d2);
		case 0267: // SUB.L D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d2);
		case 0270: // SUBA.L Dn,A2
		case 0271: // SUBA.L An,A2
		case 0272: // SUBA.L (An),A2
		case 0273: // SUBA.L (An)+,A2
		case 0274: // SUBA.L -(An),A2
		case 0275: // SUBA.L d(An),A2
		case 0276: // SUBA.L d(An,Xi),A2
			return void(a2 = a2 - rop32(op));
		case 0277: // SUBA.L Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = a2 - rop32(op));
		case 0300: // SUB.B Dn,D3
		case 0302: // SUB.B (An),D3
		case 0303: // SUB.B (An)+,D3
		case 0304: // SUB.B -(An),D3
		case 0305: // SUB.B d(An),D3
		case 0306: // SUB.B d(An,Xi),D3
			return void(d3 = d3 & ~0xff | sub8(rop8(op), d3, sr));
		case 0307: // SUB.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xff | sub8(rop8(op), d3, sr));
		case 0310: // SUB.W Dn,D3
		case 0311: // SUB.W An,D3
		case 0312: // SUB.W (An),D3
		case 0313: // SUB.W (An)+,D3
		case 0314: // SUB.W -(An),D3
		case 0315: // SUB.W d(An),D3
		case 0316: // SUB.W d(An,Xi),D3
			return void(d3 = d3 & ~0xffff | sub16(rop16(op), d3, sr));
		case 0317: // SUB.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xffff | sub16(rop16(op), d3, sr));
		case 0320: // SUB.L Dn,D3
		case 0321: // SUB.L An,D3
		case 0322: // SUB.L (An),D3
		case 0323: // SUB.L (An)+,D3
		case 0324: // SUB.L -(An),D3
		case 0325: // SUB.L d(An),D3
		case 0326: // SUB.L d(An,Xi),D3
			return void(d3 = sub32(rop32(op), d3, sr));
		case 0327: // SUB.L Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = sub32(rop32(op), d3, sr));
		case 0330: // SUBA.W Dn,A3
		case 0331: // SUBA.W An,A3
		case 0332: // SUBA.W (An),A3
		case 0333: // SUBA.W (An)+,A3
		case 0334: // SUBA.W -(An),A3
		case 0335: // SUBA.W d(An),A3
		case 0336: // SUBA.W d(An,Xi),A3
			return void(a3 = a3 - (rop16(op) << 16 >> 16));
		case 0337: // SUBA.W Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = a3 - (rop16(op) << 16 >> 16));
		case 0340: // SUBX.B Dy,D3
			return void(d3 = d3 & ~0xff | subx8(rop8(op), d3, sr));
		case 0341: // SUBX.B -(Ay),-(A3)
			return src = rop8(op & 7 | 040), a3 = a3 - 1, write8(subx8(src, read8(a3), sr), a3);
		case 0342: // SUB.B D3,(An)
		case 0343: // SUB.B D3,(An)+
		case 0344: // SUB.B D3,-(An)
		case 0345: // SUB.B D3,d(An)
		case 0346: // SUB.B D3,d(An,Xi)
			return rwop8(op, sub8, d3);
		case 0347: // SUB.B D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d3);
		case 0350: // SUBX.W Dy,D3
			return void(d3 = d3 & ~0xffff | subx16(rop16(op), d3, sr));
		case 0351: // SUBX.W -(Ay),-(A3)
			return src = rop16(op & 7 | 040), a3 = a3 - 2, write16(subx16(src, read16(a3), sr), a3);
		case 0352: // SUB.W D3,(An)
		case 0353: // SUB.W D3,(An)+
		case 0354: // SUB.W D3,-(An)
		case 0355: // SUB.W D3,d(An)
		case 0356: // SUB.W D3,d(An,Xi)
			return rwop16(op, sub16, d3);
		case 0357: // SUB.W D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d3);
		case 0360: // SUBX.L Dy,D3
			return void(d3 = subx32(rop32(op), d3, sr));
		case 0361: // SUBX.L -(Ay),-(A3)
			return src = rop32(op & 7 | 040), a3 = a3 - 4, write32(subx32(src, read32(a3), sr), a3);
		case 0362: // SUB.L D3,(An)
		case 0363: // SUB.L D3,(An)+
		case 0364: // SUB.L D3,-(An)
		case 0365: // SUB.L D3,d(An)
		case 0366: // SUB.L D3,d(An,Xi)
			return rwop32(op, sub32, d3);
		case 0367: // SUB.L D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d3);
		case 0370: // SUBA.L Dn,A3
		case 0371: // SUBA.L An,A3
		case 0372: // SUBA.L (An),A3
		case 0373: // SUBA.L (An)+,A3
		case 0374: // SUBA.L -(An),A3
		case 0375: // SUBA.L d(An),A3
		case 0376: // SUBA.L d(An,Xi),A3
			return void(a3 = a3 - rop32(op));
		case 0377: // SUBA.L Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = a3 - rop32(op));
		case 0400: // SUB.B Dn,D4
		case 0402: // SUB.B (An),D4
		case 0403: // SUB.B (An)+,D4
		case 0404: // SUB.B -(An),D4
		case 0405: // SUB.B d(An),D4
		case 0406: // SUB.B d(An,Xi),D4
			return void(d4 = d4 & ~0xff | sub8(rop8(op), d4, sr));
		case 0407: // SUB.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xff | sub8(rop8(op), d4, sr));
		case 0410: // SUB.W Dn,D4
		case 0411: // SUB.W An,D4
		case 0412: // SUB.W (An),D4
		case 0413: // SUB.W (An)+,D4
		case 0414: // SUB.W -(An),D4
		case 0415: // SUB.W d(An),D4
		case 0416: // SUB.W d(An,Xi),D4
			return void(d4 = d4 & ~0xffff | sub16(rop16(op), d4, sr));
		case 0417: // SUB.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xffff | sub16(rop16(op), d4, sr));
		case 0420: // SUB.L Dn,D4
		case 0421: // SUB.L An,D4
		case 0422: // SUB.L (An),D4
		case 0423: // SUB.L (An)+,D4
		case 0424: // SUB.L -(An),D4
		case 0425: // SUB.L d(An),D4
		case 0426: // SUB.L d(An,Xi),D4
			return void(d4 = sub32(rop32(op), d4, sr));
		case 0427: // SUB.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = sub32(rop32(op), d4, sr));
		case 0430: // SUBA.W Dn,A4
		case 0431: // SUBA.W An,A4
		case 0432: // SUBA.W (An),A4
		case 0433: // SUBA.W (An)+,A4
		case 0434: // SUBA.W -(An),A4
		case 0435: // SUBA.W d(An),A4
		case 0436: // SUBA.W d(An,Xi),A4
			return void(a4 = a4 - (rop16(op) << 16 >> 16));
		case 0437: // SUBA.W Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = a4 - (rop16(op) << 16 >> 16));
		case 0440: // SUBX.B Dy,D4
			return void(d4 = d4 & ~0xff | subx8(rop8(op), d4, sr));
		case 0441: // SUBX.B -(Ay),-(A4)
			return src = rop8(op & 7 | 040), a4 = a4 - 1, write8(subx8(src, read8(a4), sr), a4);
		case 0442: // SUB.B D4,(An)
		case 0443: // SUB.B D4,(An)+
		case 0444: // SUB.B D4,-(An)
		case 0445: // SUB.B D4,d(An)
		case 0446: // SUB.B D4,d(An,Xi)
			return rwop8(op, sub8, d4);
		case 0447: // SUB.B D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d4);
		case 0450: // SUBX.W Dy,D4
			return void(d4 = d4 & ~0xffff | subx16(rop16(op), d4, sr));
		case 0451: // SUBX.W -(Ay),-(A4)
			return src = rop16(op & 7 | 040), a4 = a4 - 2, write16(subx16(src, read16(a4), sr), a4);
		case 0452: // SUB.W D4,(An)
		case 0453: // SUB.W D4,(An)+
		case 0454: // SUB.W D4,-(An)
		case 0455: // SUB.W D4,d(An)
		case 0456: // SUB.W D4,d(An,Xi)
			return rwop16(op, sub16, d4);
		case 0457: // SUB.W D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d4);
		case 0460: // SUBX.L Dy,D4
			return void(d4 = subx32(rop32(op), d4, sr));
		case 0461: // SUBX.L -(Ay),-(A4)
			return src = rop32(op & 7 | 040), a4 = a4 - 4, write32(subx32(src, read32(a4), sr), a4);
		case 0462: // SUB.L D4,(An)
		case 0463: // SUB.L D4,(An)+
		case 0464: // SUB.L D4,-(An)
		case 0465: // SUB.L D4,d(An)
		case 0466: // SUB.L D4,d(An,Xi)
			return rwop32(op, sub32, d4);
		case 0467: // SUB.L D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d4);
		case 0470: // SUBA.L Dn,A4
		case 0471: // SUBA.L An,A4
		case 0472: // SUBA.L (An),A4
		case 0473: // SUBA.L (An)+,A4
		case 0474: // SUBA.L -(An),A4
		case 0475: // SUBA.L d(An),A4
		case 0476: // SUBA.L d(An,Xi),A4
			return void(a4 = a4 - rop32(op));
		case 0477: // SUBA.L Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = a4 - rop32(op));
		case 0500: // SUB.B Dn,D5
		case 0502: // SUB.B (An),D5
		case 0503: // SUB.B (An)+,D5
		case 0504: // SUB.B -(An),D5
		case 0505: // SUB.B d(An),D5
		case 0506: // SUB.B d(An,Xi),D5
			return void(d5 = d5 & ~0xff | sub8(rop8(op), d5, sr));
		case 0507: // SUB.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xff | sub8(rop8(op), d5, sr));
		case 0510: // SUB.W Dn,D5
		case 0511: // SUB.W An,D5
		case 0512: // SUB.W (An),D5
		case 0513: // SUB.W (An)+,D5
		case 0514: // SUB.W -(An),D5
		case 0515: // SUB.W d(An),D5
		case 0516: // SUB.W d(An,Xi),D5
			return void(d5 = d5 & ~0xffff | sub16(rop16(op), d5, sr));
		case 0517: // SUB.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xffff | sub16(rop16(op), d5, sr));
		case 0520: // SUB.L Dn,D5
		case 0521: // SUB.L An,D5
		case 0522: // SUB.L (An),D5
		case 0523: // SUB.L (An)+,D5
		case 0524: // SUB.L -(An),D5
		case 0525: // SUB.L d(An),D5
		case 0526: // SUB.L d(An,Xi),D5
			return void(d5 = sub32(rop32(op), d5, sr));
		case 0527: // SUB.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = sub32(rop32(op), d5, sr));
		case 0530: // SUBA.W Dn,A5
		case 0531: // SUBA.W An,A5
		case 0532: // SUBA.W (An),A5
		case 0533: // SUBA.W (An)+,A5
		case 0534: // SUBA.W -(An),A5
		case 0535: // SUBA.W d(An),A5
		case 0536: // SUBA.W d(An,Xi),A5
			return void(a5 = a5 - (rop16(op) << 16 >> 16));
		case 0537: // SUBA.W Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = a5 - (rop16(op) << 16 >> 16));
		case 0540: // SUBX.B Dy,D5
			return void(d5 = d5 & ~0xff | subx8(rop8(op), d5, sr));
		case 0541: // SUBX.B -(Ay),-(A5)
			return src = rop8(op & 7 | 040), a5 = a5 - 1, write8(subx8(src, read8(a5), sr), a5);
		case 0542: // SUB.B D5,(An)
		case 0543: // SUB.B D5,(An)+
		case 0544: // SUB.B D5,-(An)
		case 0545: // SUB.B D5,d(An)
		case 0546: // SUB.B D5,d(An,Xi)
			return rwop8(op, sub8, d5);
		case 0547: // SUB.B D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d5);
		case 0550: // SUBX.W Dy,D5
			return void(d5 = d5 & ~0xffff | subx16(rop16(op), d5, sr));
		case 0551: // SUBX.W -(Ay),-(A5)
			return src = rop16(op & 7 | 040), a5 = a5 - 2, write16(subx16(src, read16(a5), sr), a5);
		case 0552: // SUB.W D5,(An)
		case 0553: // SUB.W D5,(An)+
		case 0554: // SUB.W D5,-(An)
		case 0555: // SUB.W D5,d(An)
		case 0556: // SUB.W D5,d(An,Xi)
			return rwop16(op, sub16, d5);
		case 0557: // SUB.W D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d5);
		case 0560: // SUBX.L Dy,D5
			return void(d5 = subx32(rop32(op), d5, sr));
		case 0561: // SUBX.L -(Ay),-(A5)
			return src = rop32(op & 7 | 040), a5 = a5 - 4, write32(subx32(src, read32(a5), sr), a5);
		case 0562: // SUB.L D5,(An)
		case 0563: // SUB.L D5,(An)+
		case 0564: // SUB.L D5,-(An)
		case 0565: // SUB.L D5,d(An)
		case 0566: // SUB.L D5,d(An,Xi)
			return rwop32(op, sub32, d5);
		case 0567: // SUB.L D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d5);
		case 0570: // SUBA.L Dn,A5
		case 0571: // SUBA.L An,A5
		case 0572: // SUBA.L (An),A5
		case 0573: // SUBA.L (An)+,A5
		case 0574: // SUBA.L -(An),A5
		case 0575: // SUBA.L d(An),A5
		case 0576: // SUBA.L d(An,Xi),A5
			return void(a5 = a5 - rop32(op));
		case 0577: // SUBA.L Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = a5 - rop32(op));
		case 0600: // SUB.B Dn,D6
		case 0602: // SUB.B (An),D6
		case 0603: // SUB.B (An)+,D6
		case 0604: // SUB.B -(An),D6
		case 0605: // SUB.B d(An),D6
		case 0606: // SUB.B d(An,Xi),D6
			return void(d6 = d6 & ~0xff | sub8(rop8(op), d6, sr));
		case 0607: // SUB.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xff | sub8(rop8(op), d6, sr));
		case 0610: // SUB.W Dn,D6
		case 0611: // SUB.W An,D6
		case 0612: // SUB.W (An),D6
		case 0613: // SUB.W (An)+,D6
		case 0614: // SUB.W -(An),D6
		case 0615: // SUB.W d(An),D6
		case 0616: // SUB.W d(An,Xi),D6
			return void(d6 = d6 & ~0xffff | sub16(rop16(op), d6, sr));
		case 0617: // SUB.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xffff | sub16(rop16(op), d6, sr));
		case 0620: // SUB.L Dn,D6
		case 0621: // SUB.L An,D6
		case 0622: // SUB.L (An),D6
		case 0623: // SUB.L (An)+,D6
		case 0624: // SUB.L -(An),D6
		case 0625: // SUB.L d(An),D6
		case 0626: // SUB.L d(An,Xi),D6
			return void(d6 = sub32(rop32(op), d6, sr));
		case 0627: // SUB.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = sub32(rop32(op), d6, sr));
		case 0630: // SUBA.W Dn,A6
		case 0631: // SUBA.W An,A6
		case 0632: // SUBA.W (An),A6
		case 0633: // SUBA.W (An)+,A6
		case 0634: // SUBA.W -(An),A6
		case 0635: // SUBA.W d(An),A6
		case 0636: // SUBA.W d(An,Xi),A6
			return void(a6 = a6 - (rop16(op) << 16 >> 16));
		case 0637: // SUBA.W Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = a6 - (rop16(op) << 16 >> 16));
		case 0640: // SUBX.B Dy,D6
			return void(d6 = d6 & ~0xff | subx8(rop8(op), d6, sr));
		case 0641: // SUBX.B -(Ay),-(A6)
			return src = rop8(op & 7 | 040), a6 = a6 - 1, write8(subx8(src, read8(a6), sr), a6);
		case 0642: // SUB.B D6,(An)
		case 0643: // SUB.B D6,(An)+
		case 0644: // SUB.B D6,-(An)
		case 0645: // SUB.B D6,d(An)
		case 0646: // SUB.B D6,d(An,Xi)
			return rwop8(op, sub8, d6);
		case 0647: // SUB.B D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d6);
		case 0650: // SUBX.W Dy,D6
			return void(d6 = d6 & ~0xffff | subx16(rop16(op), d6, sr));
		case 0651: // SUBX.W -(Ay),-(A6)
			return src = rop16(op & 7 | 040), a6 = a6 - 2, write16(subx16(src, read16(a6), sr), a6);
		case 0652: // SUB.W D6,(An)
		case 0653: // SUB.W D6,(An)+
		case 0654: // SUB.W D6,-(An)
		case 0655: // SUB.W D6,d(An)
		case 0656: // SUB.W D6,d(An,Xi)
			return rwop16(op, sub16, d6);
		case 0657: // SUB.W D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d6);
		case 0660: // SUBX.L Dy,D6
			return void(d6 = subx32(rop32(op), d6, sr));
		case 0661: // SUBX.L -(Ay),-(A6)
			return src = rop32(op & 7 | 040), a6 = a6 - 4, write32(subx32(src, read32(a6), sr), a6);
		case 0662: // SUB.L D6,(An)
		case 0663: // SUB.L D6,(An)+
		case 0664: // SUB.L D6,-(An)
		case 0665: // SUB.L D6,d(An)
		case 0666: // SUB.L D6,d(An,Xi)
			return rwop32(op, sub32, d6);
		case 0667: // SUB.L D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d6);
		case 0670: // SUBA.L Dn,A6
		case 0671: // SUBA.L An,A6
		case 0672: // SUBA.L (An),A6
		case 0673: // SUBA.L (An)+,A6
		case 0674: // SUBA.L -(An),A6
		case 0675: // SUBA.L d(An),A6
		case 0676: // SUBA.L d(An,Xi),A6
			return void(a6 = a6 - rop32(op));
		case 0677: // SUBA.L Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = a6 - rop32(op));
		case 0700: // SUB.B Dn,D7
		case 0702: // SUB.B (An),D7
		case 0703: // SUB.B (An)+,D7
		case 0704: // SUB.B -(An),D7
		case 0705: // SUB.B d(An),D7
		case 0706: // SUB.B d(An,Xi),D7
			return void(d7 = d7 & ~0xff | sub8(rop8(op), d7, sr));
		case 0707: // SUB.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xff | sub8(rop8(op), d7, sr));
		case 0710: // SUB.W Dn,D7
		case 0711: // SUB.W An,D7
		case 0712: // SUB.W (An),D7
		case 0713: // SUB.W (An)+,D7
		case 0714: // SUB.W -(An),D7
		case 0715: // SUB.W d(An),D7
		case 0716: // SUB.W d(An,Xi),D7
			return void(d7 = d7 & ~0xffff | sub16(rop16(op), d7, sr));
		case 0717: // SUB.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xffff | sub16(rop16(op), d7, sr));
		case 0720: // SUB.L Dn,D7
		case 0721: // SUB.L An,D7
		case 0722: // SUB.L (An),D7
		case 0723: // SUB.L (An)+,D7
		case 0724: // SUB.L -(An),D7
		case 0725: // SUB.L d(An),D7
		case 0726: // SUB.L d(An,Xi),D7
			return void(d7 = sub32(rop32(op), d7, sr));
		case 0727: // SUB.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = sub32(rop32(op), d7, sr));
		case 0730: // SUBA.W Dn,A7
		case 0731: // SUBA.W An,A7
		case 0732: // SUBA.W (An),A7
		case 0733: // SUBA.W (An)+,A7
		case 0734: // SUBA.W -(An),A7
		case 0735: // SUBA.W d(An),A7
		case 0736: // SUBA.W d(An,Xi),A7
			return void(a7 = a7 - (rop16(op) << 16 >> 16));
		case 0737: // SUBA.W Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = a7 - (rop16(op) << 16 >> 16));
		case 0740: // SUBX.B Dy,D7
			return void(d7 = d7 & ~0xff | subx8(rop8(op), d7, sr));
		case 0741: // SUBX.B -(Ay),-(A7)
			return src = rop8(op & 7 | 040), a7 = a7 - 1, write8(subx8(src, read8(a7), sr), a7);
		case 0742: // SUB.B D7,(An)
		case 0743: // SUB.B D7,(An)+
		case 0744: // SUB.B D7,-(An)
		case 0745: // SUB.B D7,d(An)
		case 0746: // SUB.B D7,d(An,Xi)
			return rwop8(op, sub8, d7);
		case 0747: // SUB.B D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, sub8, d7);
		case 0750: // SUBX.W Dy,D7
			return void(d7 = d7 & ~0xffff | subx16(rop16(op), d7, sr));
		case 0751: // SUBX.W -(Ay),-(A7)
			return src = rop16(op & 7 | 040), a7 = a7 - 2, write16(subx16(src, read16(a7), sr), a7);
		case 0752: // SUB.W D7,(An)
		case 0753: // SUB.W D7,(An)+
		case 0754: // SUB.W D7,-(An)
		case 0755: // SUB.W D7,d(An)
		case 0756: // SUB.W D7,d(An,Xi)
			return rwop16(op, sub16, d7);
		case 0757: // SUB.W D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, sub16, d7);
		case 0760: // SUBX.L Dy,D7
			return void(d7 = subx32(rop32(op), d7, sr));
		case 0761: // SUBX.L -(Ay),-(A7)
			return src = rop32(op & 7 | 040), a7 = a7 - 4, write32(subx32(src, read32(a7), sr), a7);
		case 0762: // SUB.L D7,(An)
		case 0763: // SUB.L D7,(An)+
		case 0764: // SUB.L D7,-(An)
		case 0765: // SUB.L D7,d(An)
		case 0766: // SUB.L D7,d(An,Xi)
			return rwop32(op, sub32, d7);
		case 0767: // SUB.L D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, sub32, d7);
		case 0770: // SUBA.L Dn,A7
		case 0771: // SUBA.L An,A7
		case 0772: // SUBA.L (An),A7
		case 0773: // SUBA.L (An)+,A7
		case 0774: // SUBA.L -(An),A7
		case 0775: // SUBA.L d(An),A7
		case 0776: // SUBA.L d(An,Xi),A7
			return void(a7 = a7 - rop32(op));
		case 0777: // SUBA.L Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = a7 - rop32(op));
		default:
			return exception(4);
		}
	}

	void execute_b(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // CMP.B Dn,D0
		case 0002: // CMP.B (An),D0
		case 0003: // CMP.B (An)+,D0
		case 0004: // CMP.B -(An),D0
		case 0005: // CMP.B d(An),D0
		case 0006: // CMP.B d(An,Xi),D0
			return cmp8(rop8(op), d0);
		case 0007: // CMP.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d0);
		case 0010: // CMP.W Dn,D0
		case 0011: // CMP.W An,D0
		case 0012: // CMP.W (An),D0
		case 0013: // CMP.W (An)+,D0
		case 0014: // CMP.W -(An),D0
		case 0015: // CMP.W d(An),D0
		case 0016: // CMP.W d(An,Xi),D0
			return cmp16(rop16(op), d0);
		case 0017: // CMP.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d0);
		case 0020: // CMP.L Dn,D0
		case 0021: // CMP.L An,D0
		case 0022: // CMP.L (An),D0
		case 0023: // CMP.L (An)+,D0
		case 0024: // CMP.L -(An),D0
		case 0025: // CMP.L d(An),D0
		case 0026: // CMP.L d(An,Xi),D0
			return cmp32(rop32(op), d0);
		case 0027: // CMP.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d0);
		case 0030: // CMPA.W Dn,A0
		case 0031: // CMPA.W An,A0
		case 0032: // CMPA.W (An),A0
		case 0033: // CMPA.W (An)+,A0
		case 0034: // CMPA.W -(An),A0
		case 0035: // CMPA.W d(An),A0
		case 0036: // CMPA.W d(An,Xi),A0
			return cmpa16(rop16(op), a0);
		case 0037: // CMPA.W Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a0);
		case 0040: // EOR.B D0,Dn
			return rwop8(op, eor8, d0);
		case 0041: // CMPM.B (Ay)+,(A0)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a0)), void(a0 = a0 + 1);
		case 0042: // EOR.B D0,(An)
		case 0043: // EOR.B D0,(An)+
		case 0044: // EOR.B D0,-(An)
		case 0045: // EOR.B D0,d(An)
		case 0046: // EOR.B D0,d(An,Xi)
			return rwop8(op, eor8, d0);
		case 0047: // EOR.B D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d0);
		case 0050: // EOR.W D0,Dn
			return rwop16(op, eor16, d0);
		case 0051: // CMPM.W (Ay)+,(A0)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a0)), void(a0 = a0 + 2);
		case 0052: // EOR.W D0,(An)
		case 0053: // EOR.W D0,(An)+
		case 0054: // EOR.W D0,-(An)
		case 0055: // EOR.W D0,d(An)
		case 0056: // EOR.W D0,d(An,Xi)
			return rwop16(op, eor16, d0);
		case 0057: // EOR.W D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d0);
		case 0060: // EOR.L D0,Dn
			return rwop32(op, eor32, d0);
		case 0061: // CMPM.L (Ay)+,(A0)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a0)), void(a0 = a0 + 4);
		case 0062: // EOR.L D0,(An)
		case 0063: // EOR.L D0,(An)+
		case 0064: // EOR.L D0,-(An)
		case 0065: // EOR.L D0,d(An)
		case 0066: // EOR.L D0,d(An,Xi)
			return rwop32(op, eor32, d0);
		case 0067: // EOR.L D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d0);
		case 0070: // CMPA.L Dn,A0
		case 0071: // CMPA.L An,A0
		case 0072: // CMPA.L (An),A0
		case 0073: // CMPA.L (An)+,A0
		case 0074: // CMPA.L -(An),A0
		case 0075: // CMPA.L d(An),A0
		case 0076: // CMPA.L d(An,Xi),A0
			return cmpa32(rop32(op), a0);
		case 0077: // CMPA.L Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a0);
		case 0100: // CMP.B Dn,D1
		case 0102: // CMP.B (An),D1
		case 0103: // CMP.B (An)+,D1
		case 0104: // CMP.B -(An),D1
		case 0105: // CMP.B d(An),D1
		case 0106: // CMP.B d(An,Xi),D1
			return cmp8(rop8(op), d1);
		case 0107: // CMP.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d1);
		case 0110: // CMP.W Dn,D1
		case 0111: // CMP.W An,D1
		case 0112: // CMP.W (An),D1
		case 0113: // CMP.W (An)+,D1
		case 0114: // CMP.W -(An),D1
		case 0115: // CMP.W d(An),D1
		case 0116: // CMP.W d(An,Xi),D1
			return cmp16(rop16(op), d1);
		case 0117: // CMP.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d1);
		case 0120: // CMP.L Dn,D1
		case 0121: // CMP.L An,D1
		case 0122: // CMP.L (An),D1
		case 0123: // CMP.L (An)+,D1
		case 0124: // CMP.L -(An),D1
		case 0125: // CMP.L d(An),D1
		case 0126: // CMP.L d(An,Xi),D1
			return cmp32(rop32(op), d1);
		case 0127: // CMP.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d1);
		case 0130: // CMPA.W Dn,A1
		case 0131: // CMPA.W An,A1
		case 0132: // CMPA.W (An),A1
		case 0133: // CMPA.W (An)+,A1
		case 0134: // CMPA.W -(An),A1
		case 0135: // CMPA.W d(An),A1
		case 0136: // CMPA.W d(An,Xi),A1
			return cmpa16(rop16(op), a1);
		case 0137: // CMPA.W Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a1);
		case 0140: // EOR.B D1,Dn
			return rwop8(op, eor8, d1);
		case 0141: // CMPM.B (Ay)+,(A1)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a1)), void(a1 = a1 + 1);
		case 0142: // EOR.B D1,(An)
		case 0143: // EOR.B D1,(An)+
		case 0144: // EOR.B D1,-(An)
		case 0145: // EOR.B D1,d(An)
		case 0146: // EOR.B D1,d(An,Xi)
			return rwop8(op, eor8, d1);
		case 0147: // EOR.B D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d1);
		case 0150: // EOR.W D1,Dn
			return rwop16(op, eor16, d1);
		case 0151: // CMPM.W (Ay)+,(A1)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a1)), void(a1 = a1 + 2);
		case 0152: // EOR.W D1,(An)
		case 0153: // EOR.W D1,(An)+
		case 0154: // EOR.W D1,-(An)
		case 0155: // EOR.W D1,d(An)
		case 0156: // EOR.W D1,d(An,Xi)
			return rwop16(op, eor16, d1);
		case 0157: // EOR.W D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d1);
		case 0160: // EOR.L D1,Dn
			return rwop32(op, eor32, d1);
		case 0161: // CMPM.L (Ay)+,(A1)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a1)), void(a1 = a1 + 4);
		case 0162: // EOR.L D1,(An)
		case 0163: // EOR.L D1,(An)+
		case 0164: // EOR.L D1,-(An)
		case 0165: // EOR.L D1,d(An)
		case 0166: // EOR.L D1,d(An,Xi)
			return rwop32(op, eor32, d1);
		case 0167: // EOR.L D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d1);
		case 0170: // CMPA.L Dn,A1
		case 0171: // CMPA.L An,A1
		case 0172: // CMPA.L (An),A1
		case 0173: // CMPA.L (An)+,A1
		case 0174: // CMPA.L -(An),A1
		case 0175: // CMPA.L d(An),A1
		case 0176: // CMPA.L d(An,Xi),A1
			return cmpa32(rop32(op), a1);
		case 0177: // CMPA.L Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a1);
		case 0200: // CMP.B Dn,D2
		case 0202: // CMP.B (An),D2
		case 0203: // CMP.B (An)+,D2
		case 0204: // CMP.B -(An),D2
		case 0205: // CMP.B d(An),D2
		case 0206: // CMP.B d(An,Xi),D2
			return cmp8(rop8(op), d2);
		case 0207: // CMP.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d2);
		case 0210: // CMP.W Dn,D2
		case 0211: // CMP.W An,D2
		case 0212: // CMP.W (An),D2
		case 0213: // CMP.W (An)+,D2
		case 0214: // CMP.W -(An),D2
		case 0215: // CMP.W d(An),D2
		case 0216: // CMP.W d(An,Xi),D2
			return cmp16(rop16(op), d2);
		case 0217: // CMP.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d2);
		case 0220: // CMP.L Dn,D2
		case 0221: // CMP.L An,D2
		case 0222: // CMP.L (An),D2
		case 0223: // CMP.L (An)+,D2
		case 0224: // CMP.L -(An),D2
		case 0225: // CMP.L d(An),D2
		case 0226: // CMP.L d(An,Xi),D2
			return cmp32(rop32(op), d2);
		case 0227: // CMP.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d2);
		case 0230: // CMPA.W Dn,A2
		case 0231: // CMPA.W An,A2
		case 0232: // CMPA.W (An),A2
		case 0233: // CMPA.W (An)+,A2
		case 0234: // CMPA.W -(An),A2
		case 0235: // CMPA.W d(An),A2
		case 0236: // CMPA.W d(An,Xi),A2
			return cmpa16(rop16(op), a2);
		case 0237: // CMPA.W Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a2);
		case 0240: // EOR.B D2,Dn
			return rwop8(op, eor8, d2);
		case 0241: // CMPM.B (Ay)+,(A2)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a2)), void(a2 = a2 + 1);
		case 0242: // EOR.B D2,(An)
		case 0243: // EOR.B D2,(An)+
		case 0244: // EOR.B D2,-(An)
		case 0245: // EOR.B D2,d(An)
		case 0246: // EOR.B D2,d(An,Xi)
			return rwop8(op, eor8, d2);
		case 0247: // EOR.B D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d2);
		case 0250: // EOR.W D2,Dn
			return rwop16(op, eor16, d2);
		case 0251: // CMPM.W (Ay)+,(A2)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a2)), void(a2 = a2 + 2);
		case 0252: // EOR.W D2,(An)
		case 0253: // EOR.W D2,(An)+
		case 0254: // EOR.W D2,-(An)
		case 0255: // EOR.W D2,d(An)
		case 0256: // EOR.W D2,d(An,Xi)
			return rwop16(op, eor16, d2);
		case 0257: // EOR.W D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d2);
		case 0260: // EOR.L D2,Dn
			return rwop32(op, eor32, d2);
		case 0261: // CMPM.L (Ay)+,(A2)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a2)), void(a2 = a2 + 4);
		case 0262: // EOR.L D2,(An)
		case 0263: // EOR.L D2,(An)+
		case 0264: // EOR.L D2,-(An)
		case 0265: // EOR.L D2,d(An)
		case 0266: // EOR.L D2,d(An,Xi)
			return rwop32(op, eor32, d2);
		case 0267: // EOR.L D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d2);
		case 0270: // CMPA.L Dn,A2
		case 0271: // CMPA.L An,A2
		case 0272: // CMPA.L (An),A2
		case 0273: // CMPA.L (An)+,A2
		case 0274: // CMPA.L -(An),A2
		case 0275: // CMPA.L d(An),A2
		case 0276: // CMPA.L d(An,Xi),A2
			return cmpa32(rop32(op), a2);
		case 0277: // CMPA.L Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a2);
		case 0300: // CMP.B Dn,D3
		case 0302: // CMP.B (An),D3
		case 0303: // CMP.B (An)+,D3
		case 0304: // CMP.B -(An),D3
		case 0305: // CMP.B d(An),D3
		case 0306: // CMP.B d(An,Xi),D3
			return cmp8(rop8(op), d3);
		case 0307: // CMP.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d3);
		case 0310: // CMP.W Dn,D3
		case 0311: // CMP.W An,D3
		case 0312: // CMP.W (An),D3
		case 0313: // CMP.W (An)+,D3
		case 0314: // CMP.W -(An),D3
		case 0315: // CMP.W d(An),D3
		case 0316: // CMP.W d(An,Xi),D3
			return cmp16(rop16(op), d3);
		case 0317: // CMP.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d3);
		case 0320: // CMP.L Dn,D3
		case 0321: // CMP.L An,D3
		case 0322: // CMP.L (An),D3
		case 0323: // CMP.L (An)+,D3
		case 0324: // CMP.L -(An),D3
		case 0325: // CMP.L d(An),D3
		case 0326: // CMP.L d(An,Xi),D3
			return cmp32(rop32(op), d3);
		case 0327: // CMP.L Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d3);
		case 0330: // CMPA.W Dn,A3
		case 0331: // CMPA.W An,A3
		case 0332: // CMPA.W (An),A3
		case 0333: // CMPA.W (An)+,A3
		case 0334: // CMPA.W -(An),A3
		case 0335: // CMPA.W d(An),A3
		case 0336: // CMPA.W d(An,Xi),A3
			return cmpa16(rop16(op), a3);
		case 0337: // CMPA.W Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a3);
		case 0340: // EOR.B D3,Dn
			return rwop8(op, eor8, d3);
		case 0341: // CMPM.B (Ay)+,(A3)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a3)), void(a3 = a3 + 1);
		case 0342: // EOR.B D3,(An)
		case 0343: // EOR.B D3,(An)+
		case 0344: // EOR.B D3,-(An)
		case 0345: // EOR.B D3,d(An)
		case 0346: // EOR.B D3,d(An,Xi)
			return rwop8(op, eor8, d3);
		case 0347: // EOR.B D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d3);
		case 0350: // EOR.W D3,Dn
			return rwop16(op, eor16, d3);
		case 0351: // CMPM.W (Ay)+,(A3)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a3)), void(a3 = a3 + 2);
		case 0352: // EOR.W D3,(An)
		case 0353: // EOR.W D3,(An)+
		case 0354: // EOR.W D3,-(An)
		case 0355: // EOR.W D3,d(An)
		case 0356: // EOR.W D3,d(An,Xi)
			return rwop16(op, eor16, d3);
		case 0357: // EOR.W D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d3);
		case 0360: // EOR.L D3,Dn
			return rwop32(op, eor32, d3);
		case 0361: // CMPM.L (Ay)+,(A3)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a3)), void(a3 = a3 + 4);
		case 0362: // EOR.L D3,(An)
		case 0363: // EOR.L D3,(An)+
		case 0364: // EOR.L D3,-(An)
		case 0365: // EOR.L D3,d(An)
		case 0366: // EOR.L D3,d(An,Xi)
			return rwop32(op, eor32, d3);
		case 0367: // EOR.L D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d3);
		case 0370: // CMPA.L Dn,A3
		case 0371: // CMPA.L An,A3
		case 0372: // CMPA.L (An),A3
		case 0373: // CMPA.L (An)+,A3
		case 0374: // CMPA.L -(An),A3
		case 0375: // CMPA.L d(An),A3
		case 0376: // CMPA.L d(An,Xi),A3
			return cmpa32(rop32(op), a3);
		case 0377: // CMPA.L Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a3);
		case 0400: // CMP.B Dn,D4
		case 0402: // CMP.B (An),D4
		case 0403: // CMP.B (An)+,D4
		case 0404: // CMP.B -(An),D4
		case 0405: // CMP.B d(An),D4
		case 0406: // CMP.B d(An,Xi),D4
			return cmp8(rop8(op), d4);
		case 0407: // CMP.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d4);
		case 0410: // CMP.W Dn,D4
		case 0411: // CMP.W An,D4
		case 0412: // CMP.W (An),D4
		case 0413: // CMP.W (An)+,D4
		case 0414: // CMP.W -(An),D4
		case 0415: // CMP.W d(An),D4
		case 0416: // CMP.W d(An,Xi),D4
			return cmp16(rop16(op), d4);
		case 0417: // CMP.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d4);
		case 0420: // CMP.L Dn,D4
		case 0421: // CMP.L An,D4
		case 0422: // CMP.L (An),D4
		case 0423: // CMP.L (An)+,D4
		case 0424: // CMP.L -(An),D4
		case 0425: // CMP.L d(An),D4
		case 0426: // CMP.L d(An,Xi),D4
			return cmp32(rop32(op), d4);
		case 0427: // CMP.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d4);
		case 0430: // CMPA.W Dn,A4
		case 0431: // CMPA.W An,A4
		case 0432: // CMPA.W (An),A4
		case 0433: // CMPA.W (An)+,A4
		case 0434: // CMPA.W -(An),A4
		case 0435: // CMPA.W d(An),A4
		case 0436: // CMPA.W d(An,Xi),A4
			return cmpa16(rop16(op), a4);
		case 0437: // CMPA.W Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a4);
		case 0440: // EOR.B D4,Dn
			return rwop8(op, eor8, d4);
		case 0441: // CMPM.B (Ay)+,(A4)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a4)), void(a4 = a4 + 1);
		case 0442: // EOR.B D4,(An)
		case 0443: // EOR.B D4,(An)+
		case 0444: // EOR.B D4,-(An)
		case 0445: // EOR.B D4,d(An)
		case 0446: // EOR.B D4,d(An,Xi)
			return rwop8(op, eor8, d4);
		case 0447: // EOR.B D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d4);
		case 0450: // EOR.W D4,Dn
			return rwop16(op, eor16, d4);
		case 0451: // CMPM.W (Ay)+,(A4)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a4)), void(a4 = a4 + 2);
		case 0452: // EOR.W D4,(An)
		case 0453: // EOR.W D4,(An)+
		case 0454: // EOR.W D4,-(An)
		case 0455: // EOR.W D4,d(An)
		case 0456: // EOR.W D4,d(An,Xi)
			return rwop16(op, eor16, d4);
		case 0457: // EOR.W D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d4);
		case 0460: // EOR.L D4,Dn
			return rwop32(op, eor32, d4);
		case 0461: // CMPM.L (Ay)+,(A4)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a4)), void(a4 = a4 + 4);
		case 0462: // EOR.L D4,(An)
		case 0463: // EOR.L D4,(An)+
		case 0464: // EOR.L D4,-(An)
		case 0465: // EOR.L D4,d(An)
		case 0466: // EOR.L D4,d(An,Xi)
			return rwop32(op, eor32, d4);
		case 0467: // EOR.L D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d4);
		case 0470: // CMPA.L Dn,A4
		case 0471: // CMPA.L An,A4
		case 0472: // CMPA.L (An),A4
		case 0473: // CMPA.L (An)+,A4
		case 0474: // CMPA.L -(An),A4
		case 0475: // CMPA.L d(An),A4
		case 0476: // CMPA.L d(An,Xi),A4
			return cmpa32(rop32(op), a4);
		case 0477: // CMPA.L Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a4);
		case 0500: // CMP.B Dn,D5
		case 0502: // CMP.B (An),D5
		case 0503: // CMP.B (An)+,D5
		case 0504: // CMP.B -(An),D5
		case 0505: // CMP.B d(An),D5
		case 0506: // CMP.B d(An,Xi),D5
			return cmp8(rop8(op), d5);
		case 0507: // CMP.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d5);
		case 0510: // CMP.W Dn,D5
		case 0511: // CMP.W An,D5
		case 0512: // CMP.W (An),D5
		case 0513: // CMP.W (An)+,D5
		case 0514: // CMP.W -(An),D5
		case 0515: // CMP.W d(An),D5
		case 0516: // CMP.W d(An,Xi),D5
			return cmp16(rop16(op), d5);
		case 0517: // CMP.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d5);
		case 0520: // CMP.L Dn,D5
		case 0521: // CMP.L An,D5
		case 0522: // CMP.L (An),D5
		case 0523: // CMP.L (An)+,D5
		case 0524: // CMP.L -(An),D5
		case 0525: // CMP.L d(An),D5
		case 0526: // CMP.L d(An,Xi),D5
			return cmp32(rop32(op), d5);
		case 0527: // CMP.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d5);
		case 0530: // CMPA.W Dn,A5
		case 0531: // CMPA.W An,A5
		case 0532: // CMPA.W (An),A5
		case 0533: // CMPA.W (An)+,A5
		case 0534: // CMPA.W -(An),A5
		case 0535: // CMPA.W d(An),A5
		case 0536: // CMPA.W d(An,Xi),A5
			return cmpa16(rop16(op), a5);
		case 0537: // CMPA.W Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a5);
		case 0540: // EOR.B D5,Dn
			return rwop8(op, eor8, d5);
		case 0541: // CMPM.B (Ay)+,(A5)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a5)), void(a5 = a5 + 1);
		case 0542: // EOR.B D5,(An)
		case 0543: // EOR.B D5,(An)+
		case 0544: // EOR.B D5,-(An)
		case 0545: // EOR.B D5,d(An)
		case 0546: // EOR.B D5,d(An,Xi)
			return rwop8(op, eor8, d5);
		case 0547: // EOR.B D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d5);
		case 0550: // EOR.W D5,Dn
			return rwop16(op, eor16, d5);
		case 0551: // CMPM.W (Ay)+,(A5)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a5)), void(a5 = a5 + 2);
		case 0552: // EOR.W D5,(An)
		case 0553: // EOR.W D5,(An)+
		case 0554: // EOR.W D5,-(An)
		case 0555: // EOR.W D5,d(An)
		case 0556: // EOR.W D5,d(An,Xi)
			return rwop16(op, eor16, d5);
		case 0557: // EOR.W D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d5);
		case 0560: // EOR.L D5,Dn
			return rwop32(op, eor32, d5);
		case 0561: // CMPM.L (Ay)+,(A5)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a5)), void(a5 = a5 + 4);
		case 0562: // EOR.L D5,(An)
		case 0563: // EOR.L D5,(An)+
		case 0564: // EOR.L D5,-(An)
		case 0565: // EOR.L D5,d(An)
		case 0566: // EOR.L D5,d(An,Xi)
			return rwop32(op, eor32, d5);
		case 0567: // EOR.L D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d5);
		case 0570: // CMPA.L Dn,A5
		case 0571: // CMPA.L An,A5
		case 0572: // CMPA.L (An),A5
		case 0573: // CMPA.L (An)+,A5
		case 0574: // CMPA.L -(An),A5
		case 0575: // CMPA.L d(An),A5
		case 0576: // CMPA.L d(An,Xi),A5
			return cmpa32(rop32(op), a5);
		case 0577: // CMPA.L Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a5);
		case 0600: // CMP.B Dn,D6
		case 0602: // CMP.B (An),D6
		case 0603: // CMP.B (An)+,D6
		case 0604: // CMP.B -(An),D6
		case 0605: // CMP.B d(An),D6
		case 0606: // CMP.B d(An,Xi),D6
			return cmp8(rop8(op), d6);
		case 0607: // CMP.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d6);
		case 0610: // CMP.W Dn,D6
		case 0611: // CMP.W An,D6
		case 0612: // CMP.W (An),D6
		case 0613: // CMP.W (An)+,D6
		case 0614: // CMP.W -(An),D6
		case 0615: // CMP.W d(An),D6
		case 0616: // CMP.W d(An,Xi),D6
			return cmp16(rop16(op), d6);
		case 0617: // CMP.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d6);
		case 0620: // CMP.L Dn,D6
		case 0621: // CMP.L An,D6
		case 0622: // CMP.L (An),D6
		case 0623: // CMP.L (An)+,D6
		case 0624: // CMP.L -(An),D6
		case 0625: // CMP.L d(An),D6
		case 0626: // CMP.L d(An,Xi),D6
			return cmp32(rop32(op), d6);
		case 0627: // CMP.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d6);
		case 0630: // CMPA.W Dn,A6
		case 0631: // CMPA.W An,A6
		case 0632: // CMPA.W (An),A6
		case 0633: // CMPA.W (An)+,A6
		case 0634: // CMPA.W -(An),A6
		case 0635: // CMPA.W d(An),A6
		case 0636: // CMPA.W d(An,Xi),A6
			return cmpa16(rop16(op), a6);
		case 0637: // CMPA.W Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a6);
		case 0640: // EOR.B D6,Dn
			return rwop8(op, eor8, d6);
		case 0641: // CMPM.B (Ay)+,(A6)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a6)), void(a6 = a6 + 1);
		case 0642: // EOR.B D6,(An)
		case 0643: // EOR.B D6,(An)+
		case 0644: // EOR.B D6,-(An)
		case 0645: // EOR.B D6,d(An)
		case 0646: // EOR.B D6,d(An,Xi)
			return rwop8(op, eor8, d6);
		case 0647: // EOR.B D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d6);
		case 0650: // EOR.W D6,Dn
			return rwop16(op, eor16, d6);
		case 0651: // CMPM.W (Ay)+,(A6)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a6)), void(a6 = a6 + 2);
		case 0652: // EOR.W D6,(An)
		case 0653: // EOR.W D6,(An)+
		case 0654: // EOR.W D6,-(An)
		case 0655: // EOR.W D6,d(An)
		case 0656: // EOR.W D6,d(An,Xi)
			return rwop16(op, eor16, d6);
		case 0657: // EOR.W D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d6);
		case 0660: // EOR.L D6,Dn
			return rwop32(op, eor32, d6);
		case 0661: // CMPM.L (Ay)+,(A6)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a6)), void(a6 = a6 + 4);
		case 0662: // EOR.L D6,(An)
		case 0663: // EOR.L D6,(An)+
		case 0664: // EOR.L D6,-(An)
		case 0665: // EOR.L D6,d(An)
		case 0666: // EOR.L D6,d(An,Xi)
			return rwop32(op, eor32, d6);
		case 0667: // EOR.L D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d6);
		case 0670: // CMPA.L Dn,A6
		case 0671: // CMPA.L An,A6
		case 0672: // CMPA.L (An),A6
		case 0673: // CMPA.L (An)+,A6
		case 0674: // CMPA.L -(An),A6
		case 0675: // CMPA.L d(An),A6
		case 0676: // CMPA.L d(An,Xi),A6
			return cmpa32(rop32(op), a6);
		case 0677: // CMPA.L Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a6);
		case 0700: // CMP.B Dn,D7
		case 0702: // CMP.B (An),D7
		case 0703: // CMP.B (An)+,D7
		case 0704: // CMP.B -(An),D7
		case 0705: // CMP.B d(An),D7
		case 0706: // CMP.B d(An,Xi),D7
			return cmp8(rop8(op), d7);
		case 0707: // CMP.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return cmp8(rop8(op), d7);
		case 0710: // CMP.W Dn,D7
		case 0711: // CMP.W An,D7
		case 0712: // CMP.W (An),D7
		case 0713: // CMP.W (An)+,D7
		case 0714: // CMP.W -(An),D7
		case 0715: // CMP.W d(An),D7
		case 0716: // CMP.W d(An,Xi),D7
			return cmp16(rop16(op), d7);
		case 0717: // CMP.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return cmp16(rop16(op), d7);
		case 0720: // CMP.L Dn,D7
		case 0721: // CMP.L An,D7
		case 0722: // CMP.L (An),D7
		case 0723: // CMP.L (An)+,D7
		case 0724: // CMP.L -(An),D7
		case 0725: // CMP.L d(An),D7
		case 0726: // CMP.L d(An,Xi),D7
			return cmp32(rop32(op), d7);
		case 0727: // CMP.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return cmp32(rop32(op), d7);
		case 0730: // CMPA.W Dn,A7
		case 0731: // CMPA.W An,A7
		case 0732: // CMPA.W (An),A7
		case 0733: // CMPA.W (An)+,A7
		case 0734: // CMPA.W -(An),A7
		case 0735: // CMPA.W d(An),A7
		case 0736: // CMPA.W d(An,Xi),A7
			return cmpa16(rop16(op), a7);
		case 0737: // CMPA.W Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa16(rop16(op), a7);
		case 0740: // EOR.B D7,Dn
			return rwop8(op, eor8, d7);
		case 0741: // CMPM.B (Ay)+,(A7)+
			return src = rop8(op & 7 | 030), cmp8(src, read8(a7)), void(a7 = a7 + 1);
		case 0742: // EOR.B D7,(An)
		case 0743: // EOR.B D7,(An)+
		case 0744: // EOR.B D7,-(An)
		case 0745: // EOR.B D7,d(An)
		case 0746: // EOR.B D7,d(An,Xi)
			return rwop8(op, eor8, d7);
		case 0747: // EOR.B D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, eor8, d7);
		case 0750: // EOR.W D7,Dn
			return rwop16(op, eor16, d7);
		case 0751: // CMPM.W (Ay)+,(A7)+
			return src = rop16(op & 7 | 030), cmp16(src, read16(a7)), void(a7 = a7 + 2);
		case 0752: // EOR.W D7,(An)
		case 0753: // EOR.W D7,(An)+
		case 0754: // EOR.W D7,-(An)
		case 0755: // EOR.W D7,d(An)
		case 0756: // EOR.W D7,d(An,Xi)
			return rwop16(op, eor16, d7);
		case 0757: // EOR.W D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, eor16, d7);
		case 0760: // EOR.L D7,Dn
			return rwop32(op, eor32, d7);
		case 0761: // CMPM.L (Ay)+,(A7)+
			return src = rop32(op & 7 | 030), cmp32(src, read32(a7)), void(a7 = a7 + 4);
		case 0762: // EOR.L D7,(An)
		case 0763: // EOR.L D7,(An)+
		case 0764: // EOR.L D7,-(An)
		case 0765: // EOR.L D7,d(An)
		case 0766: // EOR.L D7,d(An,Xi)
			return rwop32(op, eor32, d7);
		case 0767: // EOR.L D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, eor32, d7);
		case 0770: // CMPA.L Dn,A7
		case 0771: // CMPA.L An,A7
		case 0772: // CMPA.L (An),A7
		case 0773: // CMPA.L (An)+,A7
		case 0774: // CMPA.L -(An),A7
		case 0775: // CMPA.L d(An),A7
		case 0776: // CMPA.L d(An,Xi),A7
			return cmpa32(rop32(op), a7);
		case 0777: // CMPA.L Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return cmpa32(rop32(op), a7);
		default:
			return exception(4);
		}
	}

	void execute_c(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // AND.B Dn,D0
		case 0002: // AND.B (An),D0
		case 0003: // AND.B (An)+,D0
		case 0004: // AND.B -(An),D0
		case 0005: // AND.B d(An),D0
		case 0006: // AND.B d(An,Xi),D0
			return void(d0 = d0 & ~0xff | and8(rop8(op), d0, sr));
		case 0007: // AND.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xff | and8(rop8(op), d0, sr));
		case 0010: // AND.W Dn,D0
		case 0012: // AND.W (An),D0
		case 0013: // AND.W (An)+,D0
		case 0014: // AND.W -(An),D0
		case 0015: // AND.W d(An),D0
		case 0016: // AND.W d(An,Xi),D0
			return void(d0 = d0 & ~0xffff | and16(rop16(op), d0, sr));
		case 0017: // AND.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xffff | and16(rop16(op), d0, sr));
		case 0020: // AND.L Dn,D0
		case 0022: // AND.L (An),D0
		case 0023: // AND.L (An)+,D0
		case 0024: // AND.L -(An),D0
		case 0025: // AND.L d(An),D0
		case 0026: // AND.L d(An,Xi),D0
			return void(d0 = and32(rop32(op), d0, sr));
		case 0027: // AND.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = and32(rop32(op), d0, sr));
		case 0030: // MULU Dn,D0
		case 0032: // MULU (An),D0
		case 0033: // MULU (An)+,D0
		case 0034: // MULU -(An),D0
		case 0035: // MULU d(An),D0
		case 0036: // MULU d(An,Xi),D0
			return void(d0 = mulu(rop16(op), d0, sr));
		case 0037: // MULU Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = mulu(rop16(op), d0, sr));
		case 0040: // ABCD Dy,D0
			return void(d0 = d0 & ~0xff | abcd(rop8(op), d0, sr));
		case 0041: // ABCD -(Ay),-(A0)
			return src = rop8(op & 7 | 040), a0 = a0 - 1, write8(abcd(src, read8(a0), sr), a0);
		case 0042: // AND.B D0,(An)
		case 0043: // AND.B D0,(An)+
		case 0044: // AND.B D0,-(An)
		case 0045: // AND.B D0,d(An)
		case 0046: // AND.B D0,d(An,Xi)
			return rwop8(op, and8, d0);
		case 0047: // AND.B D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d0);
		case 0050: // EXG D0,Dy
			return void(d0 = exg(op, d0));
		case 0051: // EXG A0,Ay
			return void(a0 = exg(op, a0));
		case 0052: // AND.W D0,(An)
		case 0053: // AND.W D0,(An)+
		case 0054: // AND.W D0,-(An)
		case 0055: // AND.W D0,d(An)
		case 0056: // AND.W D0,d(An,Xi)
			return rwop16(op, and16, d0);
		case 0057: // AND.W D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d0);
		case 0061: // EXG D0,Ay
			return void(d0 = exg(op, d0));
		case 0062: // AND.L D0,(An)
		case 0063: // AND.L D0,(An)+
		case 0064: // AND.L D0,-(An)
		case 0065: // AND.L D0,d(An)
		case 0066: // AND.L D0,d(An,Xi)
			return rwop32(op, and32, d0);
		case 0067: // AND.L D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d0);
		case 0070: // MULS Dn,D0
		case 0072: // MULS (An),D0
		case 0073: // MULS (An)+,D0
		case 0074: // MULS -(An),D0
		case 0075: // MULS d(An),D0
		case 0076: // MULS d(An,Xi),D0
			return void(d0 = muls(rop16(op), d0, sr));
		case 0077: // MULS Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = muls(rop16(op), d0, sr));
		case 0100: // AND.B Dn,D1
		case 0102: // AND.B (An),D1
		case 0103: // AND.B (An)+,D1
		case 0104: // AND.B -(An),D1
		case 0105: // AND.B d(An),D1
		case 0106: // AND.B d(An,Xi),D1
			return void(d1 = d1 & ~0xff | and8(rop8(op), d1, sr));
		case 0107: // AND.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xff | and8(rop8(op), d1, sr));
		case 0110: // AND.W Dn,D1
		case 0112: // AND.W (An),D1
		case 0113: // AND.W (An)+,D1
		case 0114: // AND.W -(An),D1
		case 0115: // AND.W d(An),D1
		case 0116: // AND.W d(An,Xi),D1
			return void(d1 = d1 & ~0xffff | and16(rop16(op), d1, sr));
		case 0117: // AND.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xffff | and16(rop16(op), d1, sr));
		case 0120: // AND.L Dn,D1
		case 0122: // AND.L (An),D1
		case 0123: // AND.L (An)+,D1
		case 0124: // AND.L -(An),D1
		case 0125: // AND.L d(An),D1
		case 0126: // AND.L d(An,Xi),D1
			return void(d1 = and32(rop32(op), d1, sr));
		case 0127: // AND.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = and32(rop32(op), d1, sr));
		case 0130: // MULU Dn,D1
		case 0132: // MULU (An),D1
		case 0133: // MULU (An)+,D1
		case 0134: // MULU -(An),D1
		case 0135: // MULU d(An),D1
		case 0136: // MULU d(An,Xi),D1
			return void(d1 = mulu(rop16(op), d1, sr));
		case 0137: // MULU Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = mulu(rop16(op), d1, sr));
		case 0140: // ABCD Dy,D1
			return void(d1 = d1 & ~0xff | abcd(rop8(op), d1, sr));
		case 0141: // ABCD -(Ay),-(A1)
			return src = rop8(op & 7 | 040), a1 = a1 - 1, write8(abcd(src, read8(a1), sr), a1);
		case 0142: // AND.B D1,(An)
		case 0143: // AND.B D1,(An)+
		case 0144: // AND.B D1,-(An)
		case 0145: // AND.B D1,d(An)
		case 0146: // AND.B D1,d(An,Xi)
			return rwop8(op, and8, d1);
		case 0147: // AND.B D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d1);
		case 0150: // EXG D1,Dy
			return void(d1 = exg(op, d1));
		case 0151: // EXG A1,Ay
			return void(a1 = exg(op, a1));
		case 0152: // AND.W D1,(An)
		case 0153: // AND.W D1,(An)+
		case 0154: // AND.W D1,-(An)
		case 0155: // AND.W D1,d(An)
		case 0156: // AND.W D1,d(An,Xi)
			return rwop16(op, and16, d1);
		case 0157: // AND.W D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d1);
		case 0161: // EXG D1,Ay
			return void(d1 = exg(op, d1));
		case 0162: // AND.L D1,(An)
		case 0163: // AND.L D1,(An)+
		case 0164: // AND.L D1,-(An)
		case 0165: // AND.L D1,d(An)
		case 0166: // AND.L D1,d(An,Xi)
			return rwop32(op, and32, d1);
		case 0167: // AND.L D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d1);
		case 0170: // MULS Dn,D1
		case 0172: // MULS (An),D1
		case 0173: // MULS (An)+,D1
		case 0174: // MULS -(An),D1
		case 0175: // MULS d(An),D1
		case 0176: // MULS d(An,Xi),D1
			return void(d1 = muls(rop16(op), d1, sr));
		case 0177: // MULS Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = muls(rop16(op), d1, sr));
		case 0200: // AND.B Dn,D2
		case 0202: // AND.B (An),D2
		case 0203: // AND.B (An)+,D2
		case 0204: // AND.B -(An),D2
		case 0205: // AND.B d(An),D2
		case 0206: // AND.B d(An,Xi),D2
			return void(d2 = d2 & ~0xff | and8(rop8(op), d2, sr));
		case 0207: // AND.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xff | and8(rop8(op), d2, sr));
		case 0210: // AND.W Dn,D2
		case 0212: // AND.W (An),D2
		case 0213: // AND.W (An)+,D2
		case 0214: // AND.W -(An),D2
		case 0215: // AND.W d(An),D2
		case 0216: // AND.W d(An,Xi),D2
			return void(d2 = d2 & ~0xffff | and16(rop16(op), d2, sr));
		case 0217: // AND.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xffff | and16(rop16(op), d2, sr));
		case 0220: // AND.L Dn,D2
		case 0222: // AND.L (An),D2
		case 0223: // AND.L (An)+,D2
		case 0224: // AND.L -(An),D2
		case 0225: // AND.L d(An),D2
		case 0226: // AND.L d(An,Xi),D2
			return void(d2 = and32(rop32(op), d2, sr));
		case 0227: // AND.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = and32(rop32(op), d2, sr));
		case 0230: // MULU Dn,D2
		case 0232: // MULU (An),D2
		case 0233: // MULU (An)+,D2
		case 0234: // MULU -(An),D2
		case 0235: // MULU d(An),D2
		case 0236: // MULU d(An,Xi),D2
			return void(d2 = mulu(rop16(op), d2, sr));
		case 0237: // MULU Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = mulu(rop16(op), d2, sr));
		case 0240: // ABCD Dy,D2
			return void(d2 = d2 & ~0xff | abcd(rop8(op), d2, sr));
		case 0241: // ABCD -(Ay),-(A2)
			return src = rop8(op & 7 | 040), a2 = a2 - 1, write8(abcd(src, read8(a2), sr), a2);
		case 0242: // AND.B D2,(An)
		case 0243: // AND.B D2,(An)+
		case 0244: // AND.B D2,-(An)
		case 0245: // AND.B D2,d(An)
		case 0246: // AND.B D2,d(An,Xi)
			return rwop8(op, and8, d2);
		case 0247: // AND.B D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d2);
		case 0250: // EXG D2,Dy
			return void(d2 = exg(op, d2));
		case 0251: // EXG A2,Ay
			return void(a2 = exg(op, a2));
		case 0252: // AND.W D2,(An)
		case 0253: // AND.W D2,(An)+
		case 0254: // AND.W D2,-(An)
		case 0255: // AND.W D2,d(An)
		case 0256: // AND.W D2,d(An,Xi)
			return rwop16(op, and16, d2);
		case 0257: // AND.W D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d2);
		case 0261: // EXG D2,Ay
			return void(d2 = exg(op, d2));
		case 0262: // AND.L D2,(An)
		case 0263: // AND.L D2,(An)+
		case 0264: // AND.L D2,-(An)
		case 0265: // AND.L D2,d(An)
		case 0266: // AND.L D2,d(An,Xi)
			return rwop32(op, and32, d2);
		case 0267: // AND.L D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d2);
		case 0270: // MULS Dn,D2
		case 0272: // MULS (An),D2
		case 0273: // MULS (An)+,D2
		case 0274: // MULS -(An),D2
		case 0275: // MULS d(An),D2
		case 0276: // MULS d(An,Xi),D2
			return void(d2 = muls(rop16(op), d2, sr));
		case 0277: // MULS Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = muls(rop16(op), d2, sr));
		case 0300: // AND.B Dn,D3
		case 0302: // AND.B (An),D3
		case 0303: // AND.B (An)+,D3
		case 0304: // AND.B -(An),D3
		case 0305: // AND.B d(An),D3
		case 0306: // AND.B d(An,Xi),D3
			return void(d3 = d3 & ~0xff | and8(rop8(op), d3, sr));
		case 0307: // AND.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xff | and8(rop8(op), d3, sr));
		case 0310: // AND.W Dn,D3
		case 0312: // AND.W (An),D3
		case 0313: // AND.W (An)+,D3
		case 0314: // AND.W -(An),D3
		case 0315: // AND.W d(An),D3
		case 0316: // AND.W d(An,Xi),D3
			return void(d3 = d3 & ~0xffff | and16(rop16(op), d3, sr));
		case 0317: // AND.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xffff | and16(rop16(op), d3, sr));
		case 0320: // AND.L Dn,D3
		case 0322: // AND.L (An),D3
		case 0323: // AND.L (An)+,D3
		case 0324: // AND.L -(An),D3
		case 0325: // AND.L d(An),D3
		case 0326: // AND.L d(An,Xi),D3
			return void(d3 = and32(rop32(op), d3, sr));
		case 0327: // AND.L Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = and32(rop32(op), d3, sr));
		case 0330: // MULU Dn,D3
		case 0332: // MULU (An),D3
		case 0333: // MULU (An)+,D3
		case 0334: // MULU -(An),D3
		case 0335: // MULU d(An),D3
		case 0336: // MULU d(An,Xi),D3
			return void(d3 = mulu(rop16(op), d3, sr));
		case 0337: // MULU Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = mulu(rop16(op), d3, sr));
		case 0340: // ABCD Dy,D3
			return void(d3 = d3 & ~0xff | abcd(rop8(op), d3, sr));
		case 0341: // ABCD -(Ay),-(A3)
			return src = rop8(op & 7 | 040), a3 = a3 - 1, write8(abcd(src, read8(a3), sr), a3);
		case 0342: // AND.B D3,(An)
		case 0343: // AND.B D3,(An)+
		case 0344: // AND.B D3,-(An)
		case 0345: // AND.B D3,d(An)
		case 0346: // AND.B D3,d(An,Xi)
			return rwop8(op, and8, d3);
		case 0347: // AND.B D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d3);
		case 0350: // EXG D3,Dy
			return void(d3 = exg(op, d3));
		case 0351: // EXG A3,Ay
			return void(a3 = exg(op, a3));
		case 0352: // AND.W D3,(An)
		case 0353: // AND.W D3,(An)+
		case 0354: // AND.W D3,-(An)
		case 0355: // AND.W D3,d(An)
		case 0356: // AND.W D3,d(An,Xi)
			return rwop16(op, and16, d3);
		case 0357: // AND.W D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d3);
		case 0361: // EXG D3,Ay
			return void(d3 = exg(op, d3));
		case 0362: // AND.L D3,(An)
		case 0363: // AND.L D3,(An)+
		case 0364: // AND.L D3,-(An)
		case 0365: // AND.L D3,d(An)
		case 0366: // AND.L D3,d(An,Xi)
			return rwop32(op, and32, d3);
		case 0367: // AND.L D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d3);
		case 0370: // MULS Dn,D3
		case 0372: // MULS (An),D3
		case 0373: // MULS (An)+,D3
		case 0374: // MULS -(An),D3
		case 0375: // MULS d(An),D3
		case 0376: // MULS d(An,Xi),D3
			return void(d3 = muls(rop16(op), d3, sr));
		case 0377: // MULS Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = muls(rop16(op), d3, sr));
		case 0400: // AND.B Dn,D4
		case 0402: // AND.B (An),D4
		case 0403: // AND.B (An)+,D4
		case 0404: // AND.B -(An),D4
		case 0405: // AND.B d(An),D4
		case 0406: // AND.B d(An,Xi),D4
			return void(d4 = d4 & ~0xff | and8(rop8(op), d4, sr));
		case 0407: // AND.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xff | and8(rop8(op), d4, sr));
		case 0410: // AND.W Dn,D4
		case 0412: // AND.W (An),D4
		case 0413: // AND.W (An)+,D4
		case 0414: // AND.W -(An),D4
		case 0415: // AND.W d(An),D4
		case 0416: // AND.W d(An,Xi),D4
			return void(d4 = d4 & ~0xffff | and16(rop16(op), d4, sr));
		case 0417: // AND.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xffff | and16(rop16(op), d4, sr));
		case 0420: // AND.L Dn,D4
		case 0422: // AND.L (An),D4
		case 0423: // AND.L (An)+,D4
		case 0424: // AND.L -(An),D4
		case 0425: // AND.L d(An),D4
		case 0426: // AND.L d(An,Xi),D4
			return void(d4 = and32(rop32(op), d4, sr));
		case 0427: // AND.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = and32(rop32(op), d4, sr));
		case 0430: // MULU Dn,D4
		case 0432: // MULU (An),D4
		case 0433: // MULU (An)+,D4
		case 0434: // MULU -(An),D4
		case 0435: // MULU d(An),D4
		case 0436: // MULU d(An,Xi),D4
			return void(d4 = mulu(rop16(op), d4, sr));
		case 0437: // MULU Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = mulu(rop16(op), d4, sr));
		case 0440: // ABCD Dy,D4
			return void(d4 = d4 & ~0xff | abcd(rop8(op), d4, sr));
		case 0441: // ABCD -(Ay),-(A4)
			return src = rop8(op & 7 | 040), a4 = a4 - 1, write8(abcd(src, read8(a4), sr), a4);
		case 0442: // AND.B D4,(An)
		case 0443: // AND.B D4,(An)+
		case 0444: // AND.B D4,-(An)
		case 0445: // AND.B D4,d(An)
		case 0446: // AND.B D4,d(An,Xi)
			return rwop8(op, and8, d4);
		case 0447: // AND.B D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d4);
		case 0450: // EXG D4,Dy
			return void(d4 = exg(op, d4));
		case 0451: // EXG A4,Ay
			return void(a4 = exg(op, a4));
		case 0452: // AND.W D4,(An)
		case 0453: // AND.W D4,(An)+
		case 0454: // AND.W D4,-(An)
		case 0455: // AND.W D4,d(An)
		case 0456: // AND.W D4,d(An,Xi)
			return rwop16(op, and16, d4);
		case 0457: // AND.W D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d4);
		case 0461: // EXG D4,Ay
			return void(d4 = exg(op, d4));
		case 0462: // AND.L D4,(An)
		case 0463: // AND.L D4,(An)+
		case 0464: // AND.L D4,-(An)
		case 0465: // AND.L D4,d(An)
		case 0466: // AND.L D4,d(An,Xi)
			return rwop32(op, and32, d4);
		case 0467: // AND.L D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d4);
		case 0470: // MULS Dn,D4
		case 0472: // MULS (An),D4
		case 0473: // MULS (An)+,D4
		case 0474: // MULS -(An),D4
		case 0475: // MULS d(An),D4
		case 0476: // MULS d(An,Xi),D4
			return void(d4 = muls(rop16(op), d4, sr));
		case 0477: // MULS Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = muls(rop16(op), d4, sr));
		case 0500: // AND.B Dn,D5
		case 0502: // AND.B (An),D5
		case 0503: // AND.B (An)+,D5
		case 0504: // AND.B -(An),D5
		case 0505: // AND.B d(An),D5
		case 0506: // AND.B d(An,Xi),D5
			return void(d5 = d5 & ~0xff | and8(rop8(op), d5, sr));
		case 0507: // AND.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xff | and8(rop8(op), d5, sr));
		case 0510: // AND.W Dn,D5
		case 0512: // AND.W (An),D5
		case 0513: // AND.W (An)+,D5
		case 0514: // AND.W -(An),D5
		case 0515: // AND.W d(An),D5
		case 0516: // AND.W d(An,Xi),D5
			return void(d5 = d5 & ~0xffff | and16(rop16(op), d5, sr));
		case 0517: // AND.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xffff | and16(rop16(op), d5, sr));
		case 0520: // AND.L Dn,D5
		case 0522: // AND.L (An),D5
		case 0523: // AND.L (An)+,D5
		case 0524: // AND.L -(An),D5
		case 0525: // AND.L d(An),D5
		case 0526: // AND.L d(An,Xi),D5
			return void(d5 = and32(rop32(op), d5, sr));
		case 0527: // AND.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = and32(rop32(op), d5, sr));
		case 0530: // MULU Dn,D5
		case 0532: // MULU (An),D5
		case 0533: // MULU (An)+,D5
		case 0534: // MULU -(An),D5
		case 0535: // MULU d(An),D5
		case 0536: // MULU d(An,Xi),D5
			return void(d5 = mulu(rop16(op), d5, sr));
		case 0537: // MULU Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = mulu(rop16(op), d5, sr));
		case 0540: // ABCD Dy,D5
			return void(d5 = d5 & ~0xff | abcd(rop8(op), d5, sr));
		case 0541: // ABCD -(Ay),-(A5)
			return src = rop8(op & 7 | 040), a5 = a5 - 1, write8(abcd(src, read8(a5), sr), a5);
		case 0542: // AND.B D5,(An)
		case 0543: // AND.B D5,(An)+
		case 0544: // AND.B D5,-(An)
		case 0545: // AND.B D5,d(An)
		case 0546: // AND.B D5,d(An,Xi)
			return rwop8(op, and8, d5);
		case 0547: // AND.B D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d5);
		case 0550: // EXG D5,Dy
			return void(d5 = exg(op, d5));
		case 0551: // EXG A5,Ay
			return void(a5 = exg(op, a5));
		case 0552: // AND.W D5,(An)
		case 0553: // AND.W D5,(An)+
		case 0554: // AND.W D5,-(An)
		case 0555: // AND.W D5,d(An)
		case 0556: // AND.W D5,d(An,Xi)
			return rwop16(op, and16, d5);
		case 0557: // AND.W D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d5);
		case 0561: // EXG D5,Ay
			return void(d5 = exg(op, d5));
		case 0562: // AND.L D5,(An)
		case 0563: // AND.L D5,(An)+
		case 0564: // AND.L D5,-(An)
		case 0565: // AND.L D5,d(An)
		case 0566: // AND.L D5,d(An,Xi)
			return rwop32(op, and32, d5);
		case 0567: // AND.L D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d5);
		case 0570: // MULS Dn,D5
		case 0572: // MULS (An),D5
		case 0573: // MULS (An)+,D5
		case 0574: // MULS -(An),D5
		case 0575: // MULS d(An),D5
		case 0576: // MULS d(An,Xi),D5
			return void(d5 = muls(rop16(op), d5, sr));
		case 0577: // MULS Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = muls(rop16(op), d5, sr));
		case 0600: // AND.B Dn,D6
		case 0602: // AND.B (An),D6
		case 0603: // AND.B (An)+,D6
		case 0604: // AND.B -(An),D6
		case 0605: // AND.B d(An),D6
		case 0606: // AND.B d(An,Xi),D6
			return void(d6 = d6 & ~0xff | and8(rop8(op), d6, sr));
		case 0607: // AND.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xff | and8(rop8(op), d6, sr));
		case 0610: // AND.W Dn,D6
		case 0612: // AND.W (An),D6
		case 0613: // AND.W (An)+,D6
		case 0614: // AND.W -(An),D6
		case 0615: // AND.W d(An),D6
		case 0616: // AND.W d(An,Xi),D6
			return void(d6 = d6 & ~0xffff | and16(rop16(op), d6, sr));
		case 0617: // AND.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xffff | and16(rop16(op), d6, sr));
		case 0620: // AND.L Dn,D6
		case 0622: // AND.L (An),D6
		case 0623: // AND.L (An)+,D6
		case 0624: // AND.L -(An),D6
		case 0625: // AND.L d(An),D6
		case 0626: // AND.L d(An,Xi),D6
			return void(d6 = and32(rop32(op), d6, sr));
		case 0627: // AND.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = and32(rop32(op), d6, sr));
		case 0630: // MULU Dn,D6
		case 0632: // MULU (An),D6
		case 0633: // MULU (An)+,D6
		case 0634: // MULU -(An),D6
		case 0635: // MULU d(An),D6
		case 0636: // MULU d(An,Xi),D6
			return void(d6 = mulu(rop16(op), d6, sr));
		case 0637: // MULU Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = mulu(rop16(op), d6, sr));
		case 0640: // ABCD Dy,D6
			return void(d6 = d6 & ~0xff | abcd(rop8(op), d6, sr));
		case 0641: // ABCD -(Ay),-(A6)
			return src = rop8(op & 7 | 040), a6 = a6 - 1, write8(abcd(src, read8(a6), sr), a6);
		case 0642: // AND.B D6,(An)
		case 0643: // AND.B D6,(An)+
		case 0644: // AND.B D6,-(An)
		case 0645: // AND.B D6,d(An)
		case 0646: // AND.B D6,d(An,Xi)
			return rwop8(op, and8, d6);
		case 0647: // AND.B D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d6);
		case 0650: // EXG D6,Dy
			return void(d6 = exg(op, d6));
		case 0651: // EXG A6,Ay
			return void(a6 = exg(op, a6));
		case 0652: // AND.W D6,(An)
		case 0653: // AND.W D6,(An)+
		case 0654: // AND.W D6,-(An)
		case 0655: // AND.W D6,d(An)
		case 0656: // AND.W D6,d(An,Xi)
			return rwop16(op, and16, d6);
		case 0657: // AND.W D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d6);
		case 0661: // EXG D6,Ay
			return void(d6 = exg(op, d6));
		case 0662: // AND.L D6,(An)
		case 0663: // AND.L D6,(An)+
		case 0664: // AND.L D6,-(An)
		case 0665: // AND.L D6,d(An)
		case 0666: // AND.L D6,d(An,Xi)
			return rwop32(op, and32, d6);
		case 0667: // AND.L D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d6);
		case 0670: // MULS Dn,D6
		case 0672: // MULS (An),D6
		case 0673: // MULS (An)+,D6
		case 0674: // MULS -(An),D6
		case 0675: // MULS d(An),D6
		case 0676: // MULS d(An,Xi),D6
			return void(d6 = muls(rop16(op), d6, sr));
		case 0677: // MULS Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = muls(rop16(op), d6, sr));
		case 0700: // AND.B Dn,D7
		case 0702: // AND.B (An),D7
		case 0703: // AND.B (An)+,D7
		case 0704: // AND.B -(An),D7
		case 0705: // AND.B d(An),D7
		case 0706: // AND.B d(An,Xi),D7
			return void(d7 = d7 & ~0xff | and8(rop8(op), d7, sr));
		case 0707: // AND.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xff | and8(rop8(op), d7, sr));
		case 0710: // AND.W Dn,D7
		case 0712: // AND.W (An),D7
		case 0713: // AND.W (An)+,D7
		case 0714: // AND.W -(An),D7
		case 0715: // AND.W d(An),D7
		case 0716: // AND.W d(An,Xi),D7
			return void(d7 = d7 & ~0xffff | and16(rop16(op), d7, sr));
		case 0717: // AND.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xffff | and16(rop16(op), d7, sr));
		case 0720: // AND.L Dn,D7
		case 0722: // AND.L (An),D7
		case 0723: // AND.L (An)+,D7
		case 0724: // AND.L -(An),D7
		case 0725: // AND.L d(An),D7
		case 0726: // AND.L d(An,Xi),D7
			return void(d7 = and32(rop32(op), d7, sr));
		case 0727: // AND.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = and32(rop32(op), d7, sr));
		case 0730: // MULU Dn,D7
		case 0732: // MULU (An),D7
		case 0733: // MULU (An)+,D7
		case 0734: // MULU -(An),D7
		case 0735: // MULU d(An),D7
		case 0736: // MULU d(An,Xi),D7
			return void(d7 = mulu(rop16(op), d7, sr));
		case 0737: // MULU Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = mulu(rop16(op), d7, sr));
		case 0740: // ABCD Dy,D7
			return void(d7 = d7 & ~0xff | abcd(rop8(op), d7, sr));
		case 0741: // ABCD -(Ay),-(A7)
			return src = rop8(op & 7 | 040), a7 = a7 - 1, write8(abcd(src, read8(a7), sr), a7);
		case 0742: // AND.B D7,(An)
		case 0743: // AND.B D7,(An)+
		case 0744: // AND.B D7,-(An)
		case 0745: // AND.B D7,d(An)
		case 0746: // AND.B D7,d(An,Xi)
			return rwop8(op, and8, d7);
		case 0747: // AND.B D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, and8, d7);
		case 0750: // EXG D7,Dy
			return void(d7 = exg(op, d7));
		case 0751: // EXG A7,Ay
			return void(a7 = exg(op, a7));
		case 0752: // AND.W D7,(An)
		case 0753: // AND.W D7,(An)+
		case 0754: // AND.W D7,-(An)
		case 0755: // AND.W D7,d(An)
		case 0756: // AND.W D7,d(An,Xi)
			return rwop16(op, and16, d7);
		case 0757: // AND.W D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, and16, d7);
		case 0761: // EXG D7,Ay
			return void(d7 = exg(op, d7));
		case 0762: // AND.L D7,(An)
		case 0763: // AND.L D7,(An)+
		case 0764: // AND.L D7,-(An)
		case 0765: // AND.L D7,d(An)
		case 0766: // AND.L D7,d(An,Xi)
			return rwop32(op, and32, d7);
		case 0767: // AND.L D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, and32, d7);
		case 0770: // MULS Dn,D7
		case 0772: // MULS (An),D7
		case 0773: // MULS (An)+,D7
		case 0774: // MULS -(An),D7
		case 0775: // MULS d(An),D7
		case 0776: // MULS d(An,Xi),D7
			return void(d7 = muls(rop16(op), d7, sr));
		case 0777: // MULS Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = muls(rop16(op), d7, sr));
		default:
			return exception(4);
		}
	}

	void execute_d(int op) {
		int src;
		switch (op >> 3 & 0777) {
		case 0000: // ADD.B Dn,D0
		case 0002: // ADD.B (An),D0
		case 0003: // ADD.B (An)+,D0
		case 0004: // ADD.B -(An),D0
		case 0005: // ADD.B d(An),D0
		case 0006: // ADD.B d(An,Xi),D0
			return void(d0 = d0 & ~0xff | add8(rop8(op), d0, sr));
		case 0007: // ADD.B Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xff | add8(rop8(op), d0, sr));
		case 0010: // ADD.W Dn,D0
		case 0011: // ADD.W An,D0
		case 0012: // ADD.W (An),D0
		case 0013: // ADD.W (An)+,D0
		case 0014: // ADD.W -(An),D0
		case 0015: // ADD.W d(An),D0
		case 0016: // ADD.W d(An,Xi),D0
			return void(d0 = d0 & ~0xffff | add16(rop16(op), d0, sr));
		case 0017: // ADD.W Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = d0 & ~0xffff | add16(rop16(op), d0, sr));
		case 0020: // ADD.L Dn,D0
		case 0021: // ADD.L An,D0
		case 0022: // ADD.L (An),D0
		case 0023: // ADD.L (An)+,D0
		case 0024: // ADD.L -(An),D0
		case 0025: // ADD.L d(An),D0
		case 0026: // ADD.L d(An,Xi),D0
			return void(d0 = add32(rop32(op), d0, sr));
		case 0027: // ADD.L Abs...,D0
			if ((op & 7) >= 5)
				return exception(4);
			return void(d0 = add32(rop32(op), d0, sr));
		case 0030: // ADDA.W Dn,A0
		case 0031: // ADDA.W An,A0
		case 0032: // ADDA.W (An),A0
		case 0033: // ADDA.W (An)+,A0
		case 0034: // ADDA.W -(An),A0
		case 0035: // ADDA.W d(An),A0
		case 0036: // ADDA.W d(An,Xi),A0
			return void(a0 = a0 + (rop16(op) << 16 >> 16));
		case 0037: // ADDA.W Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = a0 + (rop16(op) << 16 >> 16));
		case 0040: // ADDX.B Dy,D0
			return void(d0 = d0 & ~0xff | addx8(rop8(op), d0, sr));
		case 0041: // ADDX.B -(Ay),-(A0)
			return src = rop8(op & 7 | 040), a0 = a0 - 1, write8(addx8(src, read8(a0), sr), a0);
		case 0042: // ADD.B D0,(An)
		case 0043: // ADD.B D0,(An)+
		case 0044: // ADD.B D0,-(An)
		case 0045: // ADD.B D0,d(An)
		case 0046: // ADD.B D0,d(An,Xi)
			return rwop8(op, add8, d0);
		case 0047: // ADD.B D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d0);
		case 0050: // ADDX.W Dy,D0
			return void(d0 = d0 & ~0xffff | addx16(rop16(op), d0, sr));
		case 0051: // ADDX.W -(Ay),-(A0)
			return src = rop16(op & 7 | 040), a0 = a0 - 2, write16(addx16(src, read16(a0), sr), a0);
		case 0052: // ADD.W D0,(An)
		case 0053: // ADD.W D0,(An)+
		case 0054: // ADD.W D0,-(An)
		case 0055: // ADD.W D0,d(An)
		case 0056: // ADD.W D0,d(An,Xi)
			return rwop16(op, add16, d0);
		case 0057: // ADD.W D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d0);
		case 0060: // ADDX.L Dy,D0
			return void(d0 = addx32(rop32(op), d0, sr));
		case 0061: // ADDX.L -(Ay),-(A0)
			return src = rop32(op & 7 | 040), a0 = a0 - 4, write32(addx32(src, read32(a0), sr), a0);
		case 0062: // ADD.L D0,(An)
		case 0063: // ADD.L D0,(An)+
		case 0064: // ADD.L D0,-(An)
		case 0065: // ADD.L D0,d(An)
		case 0066: // ADD.L D0,d(An,Xi)
			return rwop32(op, add32, d0);
		case 0067: // ADD.L D0,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d0);
		case 0070: // ADDA.L Dn,A0
		case 0071: // ADDA.L An,A0
		case 0072: // ADDA.L (An),A0
		case 0073: // ADDA.L (An)+,A0
		case 0074: // ADDA.L -(An),A0
		case 0075: // ADDA.L d(An),A0
		case 0076: // ADDA.L d(An,Xi),A0
			return void(a0 = a0 + rop32(op));
		case 0077: // ADDA.L Abs...,A0
			if ((op & 7) >= 5)
				return exception(4);
			return void(a0 = a0 + rop32(op));
		case 0100: // ADD.B Dn,D1
		case 0102: // ADD.B (An),D1
		case 0103: // ADD.B (An)+,D1
		case 0104: // ADD.B -(An),D1
		case 0105: // ADD.B d(An),D1
		case 0106: // ADD.B d(An,Xi),D1
			return void(d1 = d1 & ~0xff | add8(rop8(op), d1, sr));
		case 0107: // ADD.B Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xff | add8(rop8(op), d1, sr));
		case 0110: // ADD.W Dn,D1
		case 0111: // ADD.W An,D1
		case 0112: // ADD.W (An),D1
		case 0113: // ADD.W (An)+,D1
		case 0114: // ADD.W -(An),D1
		case 0115: // ADD.W d(An),D1
		case 0116: // ADD.W d(An,Xi),D1
			return void(d1 = d1 & ~0xffff | add16(rop16(op), d1, sr));
		case 0117: // ADD.W Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = d1 & ~0xffff | add16(rop16(op), d1, sr));
		case 0120: // ADD.L Dn,D1
		case 0121: // ADD.L An,D1
		case 0122: // ADD.L (An),D1
		case 0123: // ADD.L (An)+,D1
		case 0124: // ADD.L -(An),D1
		case 0125: // ADD.L d(An),D1
		case 0126: // ADD.L d(An,Xi),D1
			return void(d1 = add32(rop32(op), d1, sr));
		case 0127: // ADD.L Abs...,D1
			if ((op & 7) >= 5)
				return exception(4);
			return void(d1 = add32(rop32(op), d1, sr));
		case 0130: // ADDA.W Dn,A1
		case 0131: // ADDA.W An,A1
		case 0132: // ADDA.W (An),A1
		case 0133: // ADDA.W (An)+,A1
		case 0134: // ADDA.W -(An),A1
		case 0135: // ADDA.W d(An),A1
		case 0136: // ADDA.W d(An,Xi),A1
			return void(a1 = a1 + (rop16(op) << 16 >> 16));
		case 0137: // ADDA.W Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = a1 + (rop16(op) << 16 >> 16));
		case 0140: // ADDX.B Dy,D1
			return void(d1 = d1 & ~0xff | addx8(rop8(op), d1, sr));
		case 0141: // ADDX.B -(Ay),-(A1)
			return src = rop8(op & 7 | 040), a1 = a1 - 1, write8(addx8(src, read8(a1), sr), a1);
		case 0142: // ADD.B D1,(An)
		case 0143: // ADD.B D1,(An)+
		case 0144: // ADD.B D1,-(An)
		case 0145: // ADD.B D1,d(An)
		case 0146: // ADD.B D1,d(An,Xi)
			return rwop8(op, add8, d1);
		case 0147: // ADD.B D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d1);
		case 0150: // ADDX.W Dy,D1
			return void(d1 = d1 & ~0xffff | addx16(rop16(op), d1, sr));
		case 0151: // ADDX.W -(Ay),-(A1)
			return src = rop16(op & 7 | 040), a1 = a1 - 2, write16(addx16(src, read16(a1), sr), a1);
		case 0152: // ADD.W D1,(An)
		case 0153: // ADD.W D1,(An)+
		case 0154: // ADD.W D1,-(An)
		case 0155: // ADD.W D1,d(An)
		case 0156: // ADD.W D1,d(An,Xi)
			return rwop16(op, add16, d1);
		case 0157: // ADD.W D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d1);
		case 0160: // ADDX.L Dy,D1
			return void(d1 = addx32(rop32(op), d1, sr));
		case 0161: // ADDX.L -(Ay),-(A1)
			return src = rop32(op & 7 | 040), a1 = a1 - 4, write32(addx32(src, read32(a1), sr), a1);
		case 0162: // ADD.L D1,(An)
		case 0163: // ADD.L D1,(An)+
		case 0164: // ADD.L D1,-(An)
		case 0165: // ADD.L D1,d(An)
		case 0166: // ADD.L D1,d(An,Xi)
			return rwop32(op, add32, d1);
		case 0167: // ADD.L D1,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d1);
		case 0170: // ADDA.L Dn,A1
		case 0171: // ADDA.L An,A1
		case 0172: // ADDA.L (An),A1
		case 0173: // ADDA.L (An)+,A1
		case 0174: // ADDA.L -(An),A1
		case 0175: // ADDA.L d(An),A1
		case 0176: // ADDA.L d(An,Xi),A1
			return void(a1 = a1 + rop32(op));
		case 0177: // ADDA.L Abs...,A1
			if ((op & 7) >= 5)
				return exception(4);
			return void(a1 = a1 + rop32(op));
		case 0200: // ADD.B Dn,D2
		case 0202: // ADD.B (An),D2
		case 0203: // ADD.B (An)+,D2
		case 0204: // ADD.B -(An),D2
		case 0205: // ADD.B d(An),D2
		case 0206: // ADD.B d(An,Xi),D2
			return void(d2 = d2 & ~0xff | add8(rop8(op), d2, sr));
		case 0207: // ADD.B Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xff | add8(rop8(op), d2, sr));
		case 0210: // ADD.W Dn,D2
		case 0211: // ADD.W An,D2
		case 0212: // ADD.W (An),D2
		case 0213: // ADD.W (An)+,D2
		case 0214: // ADD.W -(An),D2
		case 0215: // ADD.W d(An),D2
		case 0216: // ADD.W d(An,Xi),D2
			return void(d2 = d2 & ~0xffff | add16(rop16(op), d2, sr));
		case 0217: // ADD.W Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = d2 & ~0xffff | add16(rop16(op), d2, sr));
		case 0220: // ADD.L Dn,D2
		case 0221: // ADD.L An,D2
		case 0222: // ADD.L (An),D2
		case 0223: // ADD.L (An)+,D2
		case 0224: // ADD.L -(An),D2
		case 0225: // ADD.L d(An),D2
		case 0226: // ADD.L d(An,Xi),D2
			return void(d2 = add32(rop32(op), d2, sr));
		case 0227: // ADD.L Abs...,D2
			if ((op & 7) >= 5)
				return exception(4);
			return void(d2 = add32(rop32(op), d2, sr));
		case 0230: // ADDA.W Dn,A2
		case 0231: // ADDA.W An,A2
		case 0232: // ADDA.W (An),A2
		case 0233: // ADDA.W (An)+,A2
		case 0234: // ADDA.W -(An),A2
		case 0235: // ADDA.W d(An),A2
		case 0236: // ADDA.W d(An,Xi),A2
			return void(a2 = a2 + (rop16(op) << 16 >> 16));
		case 0237: // ADDA.W Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = a2 + (rop16(op) << 16 >> 16));
		case 0240: // ADDX.B Dy,D2
			return void(d2 = d2 & ~0xff | addx8(rop8(op), d2, sr));
		case 0241: // ADDX.B -(Ay),-(A2)
			return src = rop8(op & 7 | 040), a2 = a2 - 1, write8(addx8(src, read8(a2), sr), a2);
		case 0242: // ADD.B D2,(An)
		case 0243: // ADD.B D2,(An)+
		case 0244: // ADD.B D2,-(An)
		case 0245: // ADD.B D2,d(An)
		case 0246: // ADD.B D2,d(An,Xi)
			return rwop8(op, add8, d2);
		case 0247: // ADD.B D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d2);
		case 0250: // ADDX.W Dy,D2
			return void(d2 = d2 & ~0xffff | addx16(rop16(op), d2, sr));
		case 0251: // ADDX.W -(Ay),-(A2)
			return src = rop16(op & 7 | 040), a2 = a2 - 2, write16(addx16(src, read16(a2), sr), a2);
		case 0252: // ADD.W D2,(An)
		case 0253: // ADD.W D2,(An)+
		case 0254: // ADD.W D2,-(An)
		case 0255: // ADD.W D2,d(An)
		case 0256: // ADD.W D2,d(An,Xi)
			return rwop16(op, add16, d2);
		case 0257: // ADD.W D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d2);
		case 0260: // ADDX.L Dy,D2
			return void(d2 = addx32(rop32(op), d2, sr));
		case 0261: // ADDX.L -(Ay),-(A2)
			return src = rop32(op & 7 | 040), a2 = a2 - 4, write32(addx32(src, read32(a2), sr), a2);
		case 0262: // ADD.L D2,(An)
		case 0263: // ADD.L D2,(An)+
		case 0264: // ADD.L D2,-(An)
		case 0265: // ADD.L D2,d(An)
		case 0266: // ADD.L D2,d(An,Xi)
			return rwop32(op, add32, d2);
		case 0267: // ADD.L D2,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d2);
		case 0270: // ADDA.L Dn,A2
		case 0271: // ADDA.L An,A2
		case 0272: // ADDA.L (An),A2
		case 0273: // ADDA.L (An)+,A2
		case 0274: // ADDA.L -(An),A2
		case 0275: // ADDA.L d(An),A2
		case 0276: // ADDA.L d(An,Xi),A2
			return void(a2 = a2 + rop32(op));
		case 0277: // ADDA.L Abs...,A2
			if ((op & 7) >= 5)
				return exception(4);
			return void(a2 = a2 + rop32(op));
		case 0300: // ADD.B Dn,D3
		case 0302: // ADD.B (An),D3
		case 0303: // ADD.B (An)+,D3
		case 0304: // ADD.B -(An),D3
		case 0305: // ADD.B d(An),D3
		case 0306: // ADD.B d(An,Xi),D3
			return void(d3 = d3 & ~0xff | add8(rop8(op), d3, sr));
		case 0307: // ADD.B Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xff | add8(rop8(op), d3, sr));
		case 0310: // ADD.W Dn,D3
		case 0311: // ADD.W An,D3
		case 0312: // ADD.W (An),D3
		case 0313: // ADD.W (An)+,D3
		case 0314: // ADD.W -(An),D3
		case 0315: // ADD.W d(An),D3
		case 0316: // ADD.W d(An,Xi),D3
			return void(d3 = d3 & ~0xffff | add16(rop16(op), d3, sr));
		case 0317: // ADD.W Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = d3 & ~0xffff | add16(rop16(op), d3, sr));
		case 0320: // ADD.L Dn,D3
		case 0321: // ADD.L An,D3
		case 0322: // ADD.L (An),D3
		case 0323: // ADD.L (An)+,D3
		case 0324: // ADD.L -(An),D3
		case 0325: // ADD.L d(An),D3
		case 0326: // ADD.L d(An,Xi),D3
			return void(d3 = add32(rop32(op), d3, sr));
		case 0327: // ADD.L Abs...,D3
			if ((op & 7) >= 5)
				return exception(4);
			return void(d3 = add32(rop32(op), d3, sr));
		case 0330: // ADDA.W Dn,A3
		case 0331: // ADDA.W An,A3
		case 0332: // ADDA.W (An),A3
		case 0333: // ADDA.W (An)+,A3
		case 0334: // ADDA.W -(An),A3
		case 0335: // ADDA.W d(An),A3
		case 0336: // ADDA.W d(An,Xi),A3
			return void(a3 = a3 + (rop16(op) << 16 >> 16));
		case 0337: // ADDA.W Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = a3 + (rop16(op) << 16 >> 16));
		case 0340: // ADDX.B Dy,D3
			return void(d3 = d3 & ~0xff | addx8(rop8(op), d3, sr));
		case 0341: // ADDX.B -(Ay),-(A3)
			return src = rop8(op & 7 | 040), a3 = a3 - 1, write8(addx8(src, read8(a3), sr), a3);
		case 0342: // ADD.B D3,(An)
		case 0343: // ADD.B D3,(An)+
		case 0344: // ADD.B D3,-(An)
		case 0345: // ADD.B D3,d(An)
		case 0346: // ADD.B D3,d(An,Xi)
			return rwop8(op, add8, d3);
		case 0347: // ADD.B D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d3);
		case 0350: // ADDX.W Dy,D3
			return void(d3 = d3 & ~0xffff | addx16(rop16(op), d3, sr));
		case 0351: // ADDX.W -(Ay),-(A3)
			return src = rop16(op & 7 | 040), a3 = a3 - 2, write16(addx16(src, read16(a3), sr), a3);
		case 0352: // ADD.W D3,(An)
		case 0353: // ADD.W D3,(An)+
		case 0354: // ADD.W D3,-(An)
		case 0355: // ADD.W D3,d(An)
		case 0356: // ADD.W D3,d(An,Xi)
			return rwop16(op, add16, d3);
		case 0357: // ADD.W D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d3);
		case 0360: // ADDX.L Dy,D3
			return void(d3 = addx32(rop32(op), d3, sr));
		case 0361: // ADDX.L -(Ay),-(A3)
			return src = rop32(op & 7 | 040), a3 = a3 - 4, write32(addx32(src, read32(a3), sr), a3);
		case 0362: // ADD.L D3,(An)
		case 0363: // ADD.L D3,(An)+
		case 0364: // ADD.L D3,-(An)
		case 0365: // ADD.L D3,d(An)
		case 0366: // ADD.L D3,d(An,Xi)
			return rwop32(op, add32, d3);
		case 0367: // ADD.L D3,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d3);
		case 0370: // ADDA.L Dn,A3
		case 0371: // ADDA.L An,A3
		case 0372: // ADDA.L (An),A3
		case 0373: // ADDA.L (An)+,A3
		case 0374: // ADDA.L -(An),A3
		case 0375: // ADDA.L d(An),A3
		case 0376: // ADDA.L d(An,Xi),A3
			return void(a3 = a3 + rop32(op));
		case 0377: // ADDA.L Abs...,A3
			if ((op & 7) >= 5)
				return exception(4);
			return void(a3 = a3 + rop32(op));
		case 0400: // ADD.B Dn,D4
		case 0402: // ADD.B (An),D4
		case 0403: // ADD.B (An)+,D4
		case 0404: // ADD.B -(An),D4
		case 0405: // ADD.B d(An),D4
		case 0406: // ADD.B d(An,Xi),D4
			return void(d4 = d4 & ~0xff | add8(rop8(op), d4, sr));
		case 0407: // ADD.B Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xff | add8(rop8(op), d4, sr));
		case 0410: // ADD.W Dn,D4
		case 0411: // ADD.W An,D4
		case 0412: // ADD.W (An),D4
		case 0413: // ADD.W (An)+,D4
		case 0414: // ADD.W -(An),D4
		case 0415: // ADD.W d(An),D4
		case 0416: // ADD.W d(An,Xi),D4
			return void(d4 = d4 & ~0xffff | add16(rop16(op), d4, sr));
		case 0417: // ADD.W Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = d4 & ~0xffff | add16(rop16(op), d4, sr));
		case 0420: // ADD.L Dn,D4
		case 0421: // ADD.L An,D4
		case 0422: // ADD.L (An),D4
		case 0423: // ADD.L (An)+,D4
		case 0424: // ADD.L -(An),D4
		case 0425: // ADD.L d(An),D4
		case 0426: // ADD.L d(An,Xi),D4
			return void(d4 = add32(rop32(op), d4, sr));
		case 0427: // ADD.L Abs...,D4
			if ((op & 7) >= 5)
				return exception(4);
			return void(d4 = add32(rop32(op), d4, sr));
		case 0430: // ADDA.W Dn,A4
		case 0431: // ADDA.W An,A4
		case 0432: // ADDA.W (An),A4
		case 0433: // ADDA.W (An)+,A4
		case 0434: // ADDA.W -(An),A4
		case 0435: // ADDA.W d(An),A4
		case 0436: // ADDA.W d(An,Xi),A4
			return void(a4 = a4 + (rop16(op) << 16 >> 16));
		case 0437: // ADDA.W Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = a4 + (rop16(op) << 16 >> 16));
		case 0440: // ADDX.B Dy,D4
			return void(d4 = d4 & ~0xff | addx8(rop8(op), d4, sr));
		case 0441: // ADDX.B -(Ay),-(A4)
			return src = rop8(op & 7 | 040), a4 = a4 - 1, write8(addx8(src, read8(a4), sr), a4);
		case 0442: // ADD.B D4,(An)
		case 0443: // ADD.B D4,(An)+
		case 0444: // ADD.B D4,-(An)
		case 0445: // ADD.B D4,d(An)
		case 0446: // ADD.B D4,d(An,Xi)
			return rwop8(op, add8, d4);
		case 0447: // ADD.B D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d4);
		case 0450: // ADDX.W Dy,D4
			return void(d4 = d4 & ~0xffff | addx16(rop16(op), d4, sr));
		case 0451: // ADDX.W -(Ay),-(A4)
			return src = rop16(op & 7 | 040), a4 = a4 - 2, write16(addx16(src, read16(a4), sr), a4);
		case 0452: // ADD.W D4,(An)
		case 0453: // ADD.W D4,(An)+
		case 0454: // ADD.W D4,-(An)
		case 0455: // ADD.W D4,d(An)
		case 0456: // ADD.W D4,d(An,Xi)
			return rwop16(op, add16, d4);
		case 0457: // ADD.W D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d4);
		case 0460: // ADDX.L Dy,D4
			return void(d4 = addx32(rop32(op), d4, sr));
		case 0461: // ADDX.L -(Ay),-(A4)
			return src = rop32(op & 7 | 040), a4 = a4 - 4, write32(addx32(src, read32(a4), sr), a4);
		case 0462: // ADD.L D4,(An)
		case 0463: // ADD.L D4,(An)+
		case 0464: // ADD.L D4,-(An)
		case 0465: // ADD.L D4,d(An)
		case 0466: // ADD.L D4,d(An,Xi)
			return rwop32(op, add32, d4);
		case 0467: // ADD.L D4,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d4);
		case 0470: // ADDA.L Dn,A4
		case 0471: // ADDA.L An,A4
		case 0472: // ADDA.L (An),A4
		case 0473: // ADDA.L (An)+,A4
		case 0474: // ADDA.L -(An),A4
		case 0475: // ADDA.L d(An),A4
		case 0476: // ADDA.L d(An,Xi),A4
			return void(a4 = a4 + rop32(op));
		case 0477: // ADDA.L Abs...,A4
			if ((op & 7) >= 5)
				return exception(4);
			return void(a4 = a4 + rop32(op));
		case 0500: // ADD.B Dn,D5
		case 0502: // ADD.B (An),D5
		case 0503: // ADD.B (An)+,D5
		case 0504: // ADD.B -(An),D5
		case 0505: // ADD.B d(An),D5
		case 0506: // ADD.B d(An,Xi),D5
			return void(d5 = d5 & ~0xff | add8(rop8(op), d5, sr));
		case 0507: // ADD.B Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xff | add8(rop8(op), d5, sr));
		case 0510: // ADD.W Dn,D5
		case 0511: // ADD.W An,D5
		case 0512: // ADD.W (An),D5
		case 0513: // ADD.W (An)+,D5
		case 0514: // ADD.W -(An),D5
		case 0515: // ADD.W d(An),D5
		case 0516: // ADD.W d(An,Xi),D5
			return void(d5 = d5 & ~0xffff | add16(rop16(op), d5, sr));
		case 0517: // ADD.W Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = d5 & ~0xffff | add16(rop16(op), d5, sr));
		case 0520: // ADD.L Dn,D5
		case 0521: // ADD.L An,D5
		case 0522: // ADD.L (An),D5
		case 0523: // ADD.L (An)+,D5
		case 0524: // ADD.L -(An),D5
		case 0525: // ADD.L d(An),D5
		case 0526: // ADD.L d(An,Xi),D5
			return void(d5 = add32(rop32(op), d5, sr));
		case 0527: // ADD.L Abs...,D5
			if ((op & 7) >= 5)
				return exception(4);
			return void(d5 = add32(rop32(op), d5, sr));
		case 0530: // ADDA.W Dn,A5
		case 0531: // ADDA.W An,A5
		case 0532: // ADDA.W (An),A5
		case 0533: // ADDA.W (An)+,A5
		case 0534: // ADDA.W -(An),A5
		case 0535: // ADDA.W d(An),A5
		case 0536: // ADDA.W d(An,Xi),A5
			return void(a5 = a5 + (rop16(op) << 16 >> 16));
		case 0537: // ADDA.W Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = a5 + (rop16(op) << 16 >> 16));
		case 0540: // ADDX.B Dy,D5
			return void(d5 = d5 & ~0xff | addx8(rop8(op), d5, sr));
		case 0541: // ADDX.B -(Ay),-(A5)
			return src = rop8(op & 7 | 040), a5 = a5 - 1, write8(addx8(src, read8(a5), sr), a5);
		case 0542: // ADD.B D5,(An)
		case 0543: // ADD.B D5,(An)+
		case 0544: // ADD.B D5,-(An)
		case 0545: // ADD.B D5,d(An)
		case 0546: // ADD.B D5,d(An,Xi)
			return rwop8(op, add8, d5);
		case 0547: // ADD.B D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d5);
		case 0550: // ADDX.W Dy,D5
			return void(d5 = d5 & ~0xffff | addx16(rop16(op), d5, sr));
		case 0551: // ADDX.W -(Ay),-(A5)
			return src = rop16(op & 7 | 040), a5 = a5 - 2, write16(addx16(src, read16(a5), sr), a5);
		case 0552: // ADD.W D5,(An)
		case 0553: // ADD.W D5,(An)+
		case 0554: // ADD.W D5,-(An)
		case 0555: // ADD.W D5,d(An)
		case 0556: // ADD.W D5,d(An,Xi)
			return rwop16(op, add16, d5);
		case 0557: // ADD.W D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d5);
		case 0560: // ADDX.L Dy,D5
			return void(d5 = addx32(rop32(op), d5, sr));
		case 0561: // ADDX.L -(Ay),-(A5)
			return src = rop32(op & 7 | 040), a5 = a5 - 4, write32(addx32(src, read32(a5), sr), a5);
		case 0562: // ADD.L D5,(An)
		case 0563: // ADD.L D5,(An)+
		case 0564: // ADD.L D5,-(An)
		case 0565: // ADD.L D5,d(An)
		case 0566: // ADD.L D5,d(An,Xi)
			return rwop32(op, add32, d5);
		case 0567: // ADD.L D5,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d5);
		case 0570: // ADDA.L Dn,A5
		case 0571: // ADDA.L An,A5
		case 0572: // ADDA.L (An),A5
		case 0573: // ADDA.L (An)+,A5
		case 0574: // ADDA.L -(An),A5
		case 0575: // ADDA.L d(An),A5
		case 0576: // ADDA.L d(An,Xi),A5
			return void(a5 = a5 + rop32(op));
		case 0577: // ADDA.L Abs...,A5
			if ((op & 7) >= 5)
				return exception(4);
			return void(a5 = a5 + rop32(op));
		case 0600: // ADD.B Dn,D6
		case 0602: // ADD.B (An),D6
		case 0603: // ADD.B (An)+,D6
		case 0604: // ADD.B -(An),D6
		case 0605: // ADD.B d(An),D6
		case 0606: // ADD.B d(An,Xi),D6
			return void(d6 = d6 & ~0xff | add8(rop8(op), d6, sr));
		case 0607: // ADD.B Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xff | add8(rop8(op), d6, sr));
		case 0610: // ADD.W Dn,D6
		case 0611: // ADD.W An,D6
		case 0612: // ADD.W (An),D6
		case 0613: // ADD.W (An)+,D6
		case 0614: // ADD.W -(An),D6
		case 0615: // ADD.W d(An),D6
		case 0616: // ADD.W d(An,Xi),D6
			return void(d6 = d6 & ~0xffff | add16(rop16(op), d6, sr));
		case 0617: // ADD.W Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = d6 & ~0xffff | add16(rop16(op), d6, sr));
		case 0620: // ADD.L Dn,D6
		case 0621: // ADD.L An,D6
		case 0622: // ADD.L (An),D6
		case 0623: // ADD.L (An)+,D6
		case 0624: // ADD.L -(An),D6
		case 0625: // ADD.L d(An),D6
		case 0626: // ADD.L d(An,Xi),D6
			return void(d6 = add32(rop32(op), d6, sr));
		case 0627: // ADD.L Abs...,D6
			if ((op & 7) >= 5)
				return exception(4);
			return void(d6 = add32(rop32(op), d6, sr));
		case 0630: // ADDA.W Dn,A6
		case 0631: // ADDA.W An,A6
		case 0632: // ADDA.W (An),A6
		case 0633: // ADDA.W (An)+,A6
		case 0634: // ADDA.W -(An),A6
		case 0635: // ADDA.W d(An),A6
		case 0636: // ADDA.W d(An,Xi),A6
			return void(a6 = a6 + (rop16(op) << 16 >> 16));
		case 0637: // ADDA.W Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = a6 + (rop16(op) << 16 >> 16));
		case 0640: // ADDX.B Dy,D6
			return void(d6 = d6 & ~0xff | addx8(rop8(op), d6, sr));
		case 0641: // ADDX.B -(Ay),-(A6)
			return src = rop8(op & 7 | 040), a6 = a6 - 1, write8(addx8(src, read8(a6), sr), a6);
		case 0642: // ADD.B D6,(An)
		case 0643: // ADD.B D6,(An)+
		case 0644: // ADD.B D6,-(An)
		case 0645: // ADD.B D6,d(An)
		case 0646: // ADD.B D6,d(An,Xi)
			return rwop8(op, add8, d6);
		case 0647: // ADD.B D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d6);
		case 0650: // ADDX.W Dy,D6
			return void(d6 = d6 & ~0xffff | addx16(rop16(op), d6, sr));
		case 0651: // ADDX.W -(Ay),-(A6)
			return src = rop16(op & 7 | 040), a6 = a6 - 2, write16(addx16(src, read16(a6), sr), a6);
		case 0652: // ADD.W D6,(An)
		case 0653: // ADD.W D6,(An)+
		case 0654: // ADD.W D6,-(An)
		case 0655: // ADD.W D6,d(An)
		case 0656: // ADD.W D6,d(An,Xi)
			return rwop16(op, add16, d6);
		case 0657: // ADD.W D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d6);
		case 0660: // ADDX.L Dy,D6
			return void(d6 = addx32(rop32(op), d6, sr));
		case 0661: // ADDX.L -(Ay),-(A6)
			return src = rop32(op & 7 | 040), a6 = a6 - 4, write32(addx32(src, read32(a6), sr), a6);
		case 0662: // ADD.L D6,(An)
		case 0663: // ADD.L D6,(An)+
		case 0664: // ADD.L D6,-(An)
		case 0665: // ADD.L D6,d(An)
		case 0666: // ADD.L D6,d(An,Xi)
			return rwop32(op, add32, d6);
		case 0667: // ADD.L D6,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d6);
		case 0670: // ADDA.L Dn,A6
		case 0671: // ADDA.L An,A6
		case 0672: // ADDA.L (An),A6
		case 0673: // ADDA.L (An)+,A6
		case 0674: // ADDA.L -(An),A6
		case 0675: // ADDA.L d(An),A6
		case 0676: // ADDA.L d(An,Xi),A6
			return void(a6 = a6 + rop32(op));
		case 0677: // ADDA.L Abs...,A6
			if ((op & 7) >= 5)
				return exception(4);
			return void(a6 = a6 + rop32(op));
		case 0700: // ADD.B Dn,D7
		case 0702: // ADD.B (An),D7
		case 0703: // ADD.B (An)+,D7
		case 0704: // ADD.B -(An),D7
		case 0705: // ADD.B d(An),D7
		case 0706: // ADD.B d(An,Xi),D7
			return void(d7 = d7 & ~0xff | add8(rop8(op), d7, sr));
		case 0707: // ADD.B Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xff | add8(rop8(op), d7, sr));
		case 0710: // ADD.W Dn,D7
		case 0711: // ADD.W An,D7
		case 0712: // ADD.W (An),D7
		case 0713: // ADD.W (An)+,D7
		case 0714: // ADD.W -(An),D7
		case 0715: // ADD.W d(An),D7
		case 0716: // ADD.W d(An,Xi),D7
			return void(d7 = d7 & ~0xffff | add16(rop16(op), d7, sr));
		case 0717: // ADD.W Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = d7 & ~0xffff | add16(rop16(op), d7, sr));
		case 0720: // ADD.L Dn,D7
		case 0721: // ADD.L An,D7
		case 0722: // ADD.L (An),D7
		case 0723: // ADD.L (An)+,D7
		case 0724: // ADD.L -(An),D7
		case 0725: // ADD.L d(An),D7
		case 0726: // ADD.L d(An,Xi),D7
			return void(d7 = add32(rop32(op), d7, sr));
		case 0727: // ADD.L Abs...,D7
			if ((op & 7) >= 5)
				return exception(4);
			return void(d7 = add32(rop32(op), d7, sr));
		case 0730: // ADDA.W Dn,A7
		case 0731: // ADDA.W An,A7
		case 0732: // ADDA.W (An),A7
		case 0733: // ADDA.W (An)+,A7
		case 0734: // ADDA.W -(An),A7
		case 0735: // ADDA.W d(An),A7
		case 0736: // ADDA.W d(An,Xi),A7
			return void(a7 = a7 + (rop16(op) << 16 >> 16));
		case 0737: // ADDA.W Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = a7 + (rop16(op) << 16 >> 16));
		case 0740: // ADDX.B Dy,D7
			return void(d7 = d7 & ~0xff | addx8(rop8(op), d7, sr));
		case 0741: // ADDX.B -(Ay),-(A7)
			return src = rop8(op & 7 | 040), a7 = a7 - 1, write8(addx8(src, read8(a7), sr), a7);
		case 0742: // ADD.B D7,(An)
		case 0743: // ADD.B D7,(An)+
		case 0744: // ADD.B D7,-(An)
		case 0745: // ADD.B D7,d(An)
		case 0746: // ADD.B D7,d(An,Xi)
			return rwop8(op, add8, d7);
		case 0747: // ADD.B D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop8(op, add8, d7);
		case 0750: // ADDX.W Dy,D7
			return void(d7 = d7 & ~0xffff | addx16(rop16(op), d7, sr));
		case 0751: // ADDX.W -(Ay),-(A7)
			return src = rop16(op & 7 | 040), a7 = a7 - 2, write16(addx16(src, read16(a7), sr), a7);
		case 0752: // ADD.W D7,(An)
		case 0753: // ADD.W D7,(An)+
		case 0754: // ADD.W D7,-(An)
		case 0755: // ADD.W D7,d(An)
		case 0756: // ADD.W D7,d(An,Xi)
			return rwop16(op, add16, d7);
		case 0757: // ADD.W D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, add16, d7);
		case 0760: // ADDX.L Dy,D7
			return void(d7 = addx32(rop32(op), d7, sr));
		case 0761: // ADDX.L -(Ay),-(A7)
			return src = rop32(op & 7 | 040), a7 = a7 - 4, write32(addx32(src, read32(a7), sr), a7);
		case 0762: // ADD.L D7,(An)
		case 0763: // ADD.L D7,(An)+
		case 0764: // ADD.L D7,-(An)
		case 0765: // ADD.L D7,d(An)
		case 0766: // ADD.L D7,d(An,Xi)
			return rwop32(op, add32, d7);
		case 0767: // ADD.L D7,Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop32(op, add32, d7);
		case 0770: // ADDA.L Dn,A7
		case 0771: // ADDA.L An,A7
		case 0772: // ADDA.L (An),A7
		case 0773: // ADDA.L (An)+,A7
		case 0774: // ADDA.L -(An),A7
		case 0775: // ADDA.L d(An),A7
		case 0776: // ADDA.L d(An,Xi),A7
			return void(a7 = a7 + rop32(op));
		case 0777: // ADDA.L Abs...,A7
			if ((op & 7) >= 5)
				return exception(4);
			return void(a7 = a7 + rop32(op));
		default:
			return exception(4);
		}
	}

	void execute_e(int op) {
		switch (op >> 3 & 0777) {
		case 0000: // ASR.B #8,Dy
			return rwop8(op & 7, asr8, 8);
		case 0001: // LSR.B #8,Dy
			return rwop8(op & 7, lsr8, 8);
		case 0002: // ROXR.B #8,Dy
			return rwop8(op & 7, roxr8, 8);
		case 0003: // ROR.B #8,Dy
			return rwop8(op & 7, ror8, 8);
		case 0004: // ASR.B D0,Dy
			return rwop8(op & 7, asr8, d0);
		case 0005: // LSR.B D0,Dy
			return rwop8(op & 7, lsr8, d0);
		case 0006: // ROXR.B D0,Dy
			return rwop8(op & 7, roxr8, d0);
		case 0007: // ROR.B D0,Dy
			return rwop8(op & 7, ror8, d0);
		case 0010: // ASR.W #8,Dy
			return rwop16(op & 7, asr16, 8);
		case 0011: // LSR.W #8,Dy
			return rwop16(op & 7, lsr16, 8);
		case 0012: // ROXR.W #8,Dy
			return rwop16(op & 7, roxr16, 8);
		case 0013: // ROR.W #8,Dy
			return rwop16(op & 7, ror16, 8);
		case 0014: // ASR.W D0,Dy
			return rwop16(op & 7, asr16, d0);
		case 0015: // LSR.W D0,Dy
			return rwop16(op & 7, lsr16, d0);
		case 0016: // ROXR.W D0,Dy
			return rwop16(op & 7, roxr16, d0);
		case 0017: // ROR.W D0,Dy
			return rwop16(op & 7, ror16, d0);
		case 0020: // ASR.L #8,Dy
			return rwop32(op & 7, asr32, 8);
		case 0021: // LSR.L #8,Dy
			return rwop32(op & 7, lsr32, 8);
		case 0022: // ROXR.L #8,Dy
			return rwop32(op & 7, roxr32, 8);
		case 0023: // ROR.L #8,Dy
			return rwop32(op & 7, ror32, 8);
		case 0024: // ASR.L D0,Dy
			return rwop32(op & 7, asr32, d0);
		case 0025: // LSR.L D0,Dy
			return rwop32(op & 7, lsr32, d0);
		case 0026: // ROXR.L D0,Dy
			return rwop32(op & 7, roxr32, d0);
		case 0027: // ROR.L D0,Dy
			return rwop32(op & 7, ror32, d0);
		case 0032: // ASR.W (An)
		case 0033: // ASR.W (An)+
		case 0034: // ASR.W -(An)
		case 0035: // ASR.W d(An)
		case 0036: // ASR.W d(An,Xi)
			return rwop16(op, asr16, 1);
		case 0037: // ASR.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, asr16, 1);
		case 0040: // ASL.B #8,Dy
			return rwop8(op & 7, asl8, 8);
		case 0041: // LSL.B #8,Dy
			return rwop8(op & 7, lsl8, 8);
		case 0042: // ROXL.B #8,Dy
			return rwop8(op & 7, roxl8, 8);
		case 0043: // ROL.B #8,Dy
			return rwop8(op & 7, rol8, 8);
		case 0044: // ASL.B D0,Dy
			return rwop8(op & 7, asl8, d0);
		case 0045: // LSL.B D0,Dy
			return rwop8(op & 7, lsl8, d0);
		case 0046: // ROXL.B D0,Dy
			return rwop8(op & 7, roxl8, d0);
		case 0047: // ROL.B D0,Dy
			return rwop8(op & 7, rol8, d0);
		case 0050: // ASL.W #8,Dy
			return rwop16(op & 7, asl16, 8);
		case 0051: // LSL.W #8,Dy
			return rwop16(op & 7, lsl16, 8);
		case 0052: // ROXL.W #8,Dy
			return rwop16(op & 7, roxl16, 8);
		case 0053: // ROL.W #8,Dy
			return rwop16(op & 7, rol16, 8);
		case 0054: // ASL.W D0,Dy
			return rwop16(op & 7, asl16, d0);
		case 0055: // LSL.W D0,Dy
			return rwop16(op & 7, lsl16, d0);
		case 0056: // ROXL.W D0,Dy
			return rwop16(op & 7, roxl16, d0);
		case 0057: // ROL.W D0,Dy
			return rwop16(op & 7, rol16, d0);
		case 0060: // ASL.L #8,Dy
			return rwop32(op & 7, asl32, 8);
		case 0061: // LSL.L #8,Dy
			return rwop32(op & 7, lsl32, 8);
		case 0062: // ROXL.L #8,Dy
			return rwop32(op & 7, roxl32, 8);
		case 0063: // ROL.L #8,Dy
			return rwop32(op & 7, rol32, 8);
		case 0064: // ASL.L D0,Dy
			return rwop32(op & 7, asl32, d0);
		case 0065: // LSL.L D0,Dy
			return rwop32(op & 7, lsl32, d0);
		case 0066: // ROXL.L D0,Dy
			return rwop32(op & 7, roxl32, d0);
		case 0067: // ROL.L D0,Dy
			return rwop32(op & 7, rol32, d0);
		case 0072: // ASL.W (An)
		case 0073: // ASL.W (An)+
		case 0074: // ASL.W -(An)
		case 0075: // ASL.W d(An)
		case 0076: // ASL.W d(An,Xi)
			return rwop16(op, asl16, 1);
		case 0077: // ASL.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, asl16, 1);
		case 0100: // ASR.B #1,Dy
			return rwop8(op & 7, asr8, 1);
		case 0101: // LSR.B #1,Dy
			return rwop8(op & 7, lsr8, 1);
		case 0102: // ROXR.B #1,Dy
			return rwop8(op & 7, roxr8, 1);
		case 0103: // ROR.B #1,Dy
			return rwop8(op & 7, ror8, 1);
		case 0104: // ASR.B D1,Dy
			return rwop8(op & 7, asr8, d1);
		case 0105: // LSR.B D1,Dy
			return rwop8(op & 7, lsr8, d1);
		case 0106: // ROXR.B D1,Dy
			return rwop8(op & 7, roxr8, d1);
		case 0107: // ROR.B D1,Dy
			return rwop8(op & 7, ror8, d1);
		case 0110: // ASR.W #1,Dy
			return rwop16(op & 7, asr16, 1);
		case 0111: // LSR.W #1,Dy
			return rwop16(op & 7, lsr16, 1);
		case 0112: // ROXR.W #1,Dy
			return rwop16(op & 7, roxr16, 1);
		case 0113: // ROR.W #1,Dy
			return rwop16(op & 7, ror16, 1);
		case 0114: // ASR.W D1,Dy
			return rwop16(op & 7, asr16, d1);
		case 0115: // LSR.W D1,Dy
			return rwop16(op & 7, lsr16, d1);
		case 0116: // ROXR.W D1,Dy
			return rwop16(op & 7, roxr16, d1);
		case 0117: // ROR.W D1,Dy
			return rwop16(op & 7, ror16, d1);
		case 0120: // ASR.L #1,Dy
			return rwop32(op & 7, asr32, 1);
		case 0121: // LSR.L #1,Dy
			return rwop32(op & 7, lsr32, 1);
		case 0122: // ROXR.L #1,Dy
			return rwop32(op & 7, roxr32, 1);
		case 0123: // ROR.L #1,Dy
			return rwop32(op & 7, ror32, 1);
		case 0124: // ASR.L D1,Dy
			return rwop32(op & 7, asr32, d1);
		case 0125: // LSR.L D1,Dy
			return rwop32(op & 7, lsr32, d1);
		case 0126: // ROXR.L D1,Dy
			return rwop32(op & 7, roxr32, d1);
		case 0127: // ROR.L D1,Dy
			return rwop32(op & 7, ror32, d1);
		case 0132: // LSR.W (An)
		case 0133: // LSR.W (An)+
		case 0134: // LSR.W -(An)
		case 0135: // LSR.W d(An)
		case 0136: // LSR.W d(An,Xi)
			return rwop16(op, lsr16, 1);
		case 0137: // LSR.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, lsr16, 1);
		case 0140: // ASL.B #1,Dy
			return rwop8(op & 7, asl8, 1);
		case 0141: // LSL.B #1,Dy
			return rwop8(op & 7, lsl8, 1);
		case 0142: // ROXL.B #1,Dy
			return rwop8(op & 7, roxl8, 1);
		case 0143: // ROL.B #1,Dy
			return rwop8(op & 7, rol8, 1);
		case 0144: // ASL.B D1,Dy
			return rwop8(op & 7, asl8, d1);
		case 0145: // LSL.B D1,Dy
			return rwop8(op & 7, lsl8, d1);
		case 0146: // ROXL.B D1,Dy
			return rwop8(op & 7, roxl8, d1);
		case 0147: // ROL.B D1,Dy
			return rwop8(op & 7, rol8, d1);
		case 0150: // ASL.W #1,Dy
			return rwop16(op & 7, asl16, 1);
		case 0151: // LSL.W #1,Dy
			return rwop16(op & 7, lsl16, 1);
		case 0152: // ROXL.W #1,Dy
			return rwop16(op & 7, roxl16, 1);
		case 0153: // ROL.W #1,Dy
			return rwop16(op & 7, rol16, 1);
		case 0154: // ASL.W D1,Dy
			return rwop16(op & 7, asl16, d1);
		case 0155: // LSL.W D1,Dy
			return rwop16(op & 7, lsl16, d1);
		case 0156: // ROXL.W D1,Dy
			return rwop16(op & 7, roxl16, d1);
		case 0157: // ROL.W D1,Dy
			return rwop16(op & 7, rol16, d1);
		case 0160: // ASL.L #1,Dy
			return rwop32(op & 7, asl32, 1);
		case 0161: // LSL.L #1,Dy
			return rwop32(op & 7, lsl32, 1);
		case 0162: // ROXL.L #1,Dy
			return rwop32(op & 7, roxl32, 1);
		case 0163: // ROL.L #1,Dy
			return rwop32(op & 7, rol32, 1);
		case 0164: // ASL.L D1,Dy
			return rwop32(op & 7, asl32, d1);
		case 0165: // LSL.L D1,Dy
			return rwop32(op & 7, lsl32, d1);
		case 0166: // ROXL.L D1,Dy
			return rwop32(op & 7, roxl32, d1);
		case 0167: // ROL.L D1,Dy
			return rwop32(op & 7, rol32, d1);
		case 0172: // LSL.W (An)
		case 0173: // LSL.W (An)+
		case 0174: // LSL.W -(An)
		case 0175: // LSL.W d(An)
		case 0176: // LSL.W d(An,Xi)
			return rwop16(op, lsl16, 1);
		case 0177: // LSL.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, lsl16, 1);
		case 0200: // ASR.B #2,Dy
			return rwop8(op & 7, asr8, 2);
		case 0201: // LSR.B #2,Dy
			return rwop8(op & 7, lsr8, 2);
		case 0202: // ROXR.B #2,Dy
			return rwop8(op & 7, roxr8, 2);
		case 0203: // ROR.B #2,Dy
			return rwop8(op & 7, ror8, 2);
		case 0204: // ASR.B D2,Dy
			return rwop8(op & 7, asr8, d2);
		case 0205: // LSR.B D2,Dy
			return rwop8(op & 7, lsr8, d2);
		case 0206: // ROXR.B D2,Dy
			return rwop8(op & 7, roxr8, d2);
		case 0207: // ROR.B D2,Dy
			return rwop8(op & 7, ror8, d2);
		case 0210: // ASR.W #2,Dy
			return rwop16(op & 7, asr16, 2);
		case 0211: // LSR.W #2,Dy
			return rwop16(op & 7, lsr16, 2);
		case 0212: // ROXR.W #2,Dy
			return rwop16(op & 7, roxr16, 2);
		case 0213: // ROR.W #2,Dy
			return rwop16(op & 7, ror16, 2);
		case 0214: // ASR.W D2,Dy
			return rwop16(op & 7, asr16, d2);
		case 0215: // LSR.W D2,Dy
			return rwop16(op & 7, lsr16, d2);
		case 0216: // ROXR.W D2,Dy
			return rwop16(op & 7, roxr16, d2);
		case 0217: // ROR.W D2,Dy
			return rwop16(op & 7, ror16, d2);
		case 0220: // ASR.L #2,Dy
			return rwop32(op & 7, asr32, 2);
		case 0221: // LSR.L #2,Dy
			return rwop32(op & 7, lsr32, 2);
		case 0222: // ROXR.L #2,Dy
			return rwop32(op & 7, roxr32, 2);
		case 0223: // ROR.L #2,Dy
			return rwop32(op & 7, ror32, 2);
		case 0224: // ASR.L D2,Dy
			return rwop32(op & 7, asr32, d2);
		case 0225: // LSR.L D2,Dy
			return rwop32(op & 7, lsr32, d2);
		case 0226: // ROXR.L D2,Dy
			return rwop32(op & 7, roxr32, d2);
		case 0227: // ROR.L D2,Dy
			return rwop32(op & 7, ror32, d2);
		case 0232: // ROXR.W (An)
		case 0233: // ROXR.W (An)+
		case 0234: // ROXR.W -(An)
		case 0235: // ROXR.W d(An)
		case 0236: // ROXR.W d(An,Xi)
			return rwop16(op, roxr16, 1);
		case 0237: // ROXR.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, roxr16, 1);
		case 0240: // ASL.B #2,Dy
			return rwop8(op & 7, asl8, 2);
		case 0241: // LSL.B #2,Dy
			return rwop8(op & 7, lsl8, 2);
		case 0242: // ROXL.B #2,Dy
			return rwop8(op & 7, roxl8, 2);
		case 0243: // ROL.B #2,Dy
			return rwop8(op & 7, rol8, 2);
		case 0244: // ASL.B D2,Dy
			return rwop8(op & 7, asl8, d2);
		case 0245: // LSL.B D2,Dy
			return rwop8(op & 7, lsl8, d2);
		case 0246: // ROXL.B D2,Dy
			return rwop8(op & 7, roxl8, d2);
		case 0247: // ROL.B D2,Dy
			return rwop8(op & 7, rol8, d2);
		case 0250: // ASL.W #2,Dy
			return rwop16(op & 7, asl16, 2);
		case 0251: // LSL.W #2,Dy
			return rwop16(op & 7, lsl16, 2);
		case 0252: // ROXL.W #2,Dy
			return rwop16(op & 7, roxl16, 2);
		case 0253: // ROL.W #2,Dy
			return rwop16(op & 7, rol16, 2);
		case 0254: // ASL.W D2,Dy
			return rwop16(op & 7, asl16, d2);
		case 0255: // LSL.W D2,Dy
			return rwop16(op & 7, lsl16, d2);
		case 0256: // ROXL.W D2,Dy
			return rwop16(op & 7, roxl16, d2);
		case 0257: // ROL.W D2,Dy
			return rwop16(op & 7, rol16, d2);
		case 0260: // ASL.L #2,Dy
			return rwop32(op & 7, asl32, 2);
		case 0261: // LSL.L #2,Dy
			return rwop32(op & 7, lsl32, 2);
		case 0262: // ROXL.L #2,Dy
			return rwop32(op & 7, roxl32, 2);
		case 0263: // ROL.L #2,Dy
			return rwop32(op & 7, rol32, 2);
		case 0264: // ASL.L D2,Dy
			return rwop32(op & 7, asl32, d2);
		case 0265: // LSL.L D2,Dy
			return rwop32(op & 7, lsl32, d2);
		case 0266: // ROXL.L D2,Dy
			return rwop32(op & 7, roxl32, d2);
		case 0267: // ROL.L D2,Dy
			return rwop32(op & 7, rol32, d2);
		case 0272: // ROXL.W (An)
		case 0273: // ROXL.W (An)+
		case 0274: // ROXL.W -(An)
		case 0275: // ROXL.W d(An)
		case 0276: // ROXL.W d(An,Xi)
			return rwop16(op, roxl16, 1);
		case 0277: // ROXL.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, roxl16, 1);
		case 0300: // ASR.B #3,Dy
			return rwop8(op & 7, asr8, 3);
		case 0301: // LSR.B #3,Dy
			return rwop8(op & 7, lsr8, 3);
		case 0302: // ROXR.B #3,Dy
			return rwop8(op & 7, roxr8, 3);
		case 0303: // ROR.B #3,Dy
			return rwop8(op & 7, ror8, 3);
		case 0304: // ASR.B D3,Dy
			return rwop8(op & 7, asr8, d3);
		case 0305: // LSR.B D3,Dy
			return rwop8(op & 7, lsr8, d3);
		case 0306: // ROXR.B D3,Dy
			return rwop8(op & 7, roxr8, d3);
		case 0307: // ROR.B D3,Dy
			return rwop8(op & 7, ror8, d3);
		case 0310: // ASR.W #3,Dy
			return rwop16(op & 7, asr16, 3);
		case 0311: // LSR.W #3,Dy
			return rwop16(op & 7, lsr16, 3);
		case 0312: // ROXR.W #3,Dy
			return rwop16(op & 7, roxr16, 3);
		case 0313: // ROR.W #3,Dy
			return rwop16(op & 7, ror16, 3);
		case 0314: // ASR.W D3,Dy
			return rwop16(op & 7, asr16, d3);
		case 0315: // LSR.W D3,Dy
			return rwop16(op & 7, lsr16, d3);
		case 0316: // ROXR.W D3,Dy
			return rwop16(op & 7, roxr16, d3);
		case 0317: // ROR.W D3,Dy
			return rwop16(op & 7, ror16, d3);
		case 0320: // ASR.L #3,Dy
			return rwop32(op & 7, asr32, 3);
		case 0321: // LSR.L #3,Dy
			return rwop32(op & 7, lsr32, 3);
		case 0322: // ROXR.L #3,Dy
			return rwop32(op & 7, roxr32, 3);
		case 0323: // ROR.L #3,Dy
			return rwop32(op & 7, ror32, 3);
		case 0324: // ASR.L D3,Dy
			return rwop32(op & 7, asr32, d3);
		case 0325: // LSR.L D3,Dy
			return rwop32(op & 7, lsr32, d3);
		case 0326: // ROXR.L D3,Dy
			return rwop32(op & 7, roxr32, d3);
		case 0327: // ROR.L D3,Dy
			return rwop32(op & 7, ror32, d3);
		case 0332: // ROR.W (An)
		case 0333: // ROR.W (An)+
		case 0334: // ROR.W -(An)
		case 0335: // ROR.W d(An)
		case 0336: // ROR.W d(An,Xi)
			return rwop16(op, ror16, 1);
		case 0337: // ROR.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, ror16, 1);
		case 0340: // ASL.B #3,Dy
			return rwop8(op & 7, asl8, 3);
		case 0341: // LSL.B #3,Dy
			return rwop8(op & 7, lsl8, 3);
		case 0342: // ROXL.B #3,Dy
			return rwop8(op & 7, roxl8, 3);
		case 0343: // ROL.B #3,Dy
			return rwop8(op & 7, rol8, 3);
		case 0344: // ASL.B D3,Dy
			return rwop8(op & 7, asl8, d3);
		case 0345: // LSL.B D3,Dy
			return rwop8(op & 7, lsl8, d3);
		case 0346: // ROXL.B D3,Dy
			return rwop8(op & 7, roxl8, d3);
		case 0347: // ROL.B D3,Dy
			return rwop8(op & 7, rol8, d3);
		case 0350: // ASL.W #3,Dy
			return rwop16(op & 7, asl16, 3);
		case 0351: // LSL.W #3,Dy
			return rwop16(op & 7, lsl16, 3);
		case 0352: // ROXL.W #3,Dy
			return rwop16(op & 7, roxl16, 3);
		case 0353: // ROL.W #3,Dy
			return rwop16(op & 7, rol16, 3);
		case 0354: // ASL.W D3,Dy
			return rwop16(op & 7, asl16, d3);
		case 0355: // LSL.W D3,Dy
			return rwop16(op & 7, lsl16, d3);
		case 0356: // ROXL.W D3,Dy
			return rwop16(op & 7, roxl16, d3);
		case 0357: // ROL.W D3,Dy
			return rwop16(op & 7, rol16, d3);
		case 0360: // ASL.L #3,Dy
			return rwop32(op & 7, asl32, 3);
		case 0361: // LSL.L #3,Dy
			return rwop32(op & 7, lsl32, 3);
		case 0362: // ROXL.L #3,Dy
			return rwop32(op & 7, roxl32, 3);
		case 0363: // ROL.L #3,Dy
			return rwop32(op & 7, rol32, 3);
		case 0364: // ASL.L D3,Dy
			return rwop32(op & 7, asl32, d3);
		case 0365: // LSL.L D3,Dy
			return rwop32(op & 7, lsl32, d3);
		case 0366: // ROXL.L D3,Dy
			return rwop32(op & 7, roxl32, d3);
		case 0367: // ROL.L D3,Dy
			return rwop32(op & 7, rol32, d3);
		case 0372: // ROL.W (An)
		case 0373: // ROL.W (An)+
		case 0374: // ROL.W -(An)
		case 0375: // ROL.W d(An)
		case 0376: // ROL.W d(An,Xi)
			return rwop16(op, rol16, 1);
		case 0377: // ROL.W Abs...
			if ((op & 7) >= 2)
				return exception(4);
			return rwop16(op, rol16, 1);
		case 0400: // ASR.B #4,Dy
			return rwop8(op & 7, asr8, 4);
		case 0401: // LSR.B #4,Dy
			return rwop8(op & 7, lsr8, 4);
		case 0402: // ROXR.B #4,Dy
			return rwop8(op & 7, roxr8, 4);
		case 0403: // ROR.B #4,Dy
			return rwop8(op & 7, ror8, 4);
		case 0404: // ASR.B D4,Dy
			return rwop8(op & 7, asr8, d4);
		case 0405: // LSR.B D4,Dy
			return rwop8(op & 7, lsr8, d4);
		case 0406: // ROXR.B D4,Dy
			return rwop8(op & 7, roxr8, d4);
		case 0407: // ROR.B D4,Dy
			return rwop8(op & 7, ror8, d4);
		case 0410: // ASR.W #4,Dy
			return rwop16(op & 7, asr16, 4);
		case 0411: // LSR.W #4,Dy
			return rwop16(op & 7, lsr16, 4);
		case 0412: // ROXR.W #4,Dy
			return rwop16(op & 7, roxr16, 4);
		case 0413: // ROR.W #4,Dy
			return rwop16(op & 7, ror16, 4);
		case 0414: // ASR.W D4,Dy
			return rwop16(op & 7, asr16, d4);
		case 0415: // LSR.W D4,Dy
			return rwop16(op & 7, lsr16, d4);
		case 0416: // ROXR.W D4,Dy
			return rwop16(op & 7, roxr16, d4);
		case 0417: // ROR.W D4,Dy
			return rwop16(op & 7, ror16, d4);
		case 0420: // ASR.L #4,Dy
			return rwop32(op & 7, asr32, 4);
		case 0421: // LSR.L #4,Dy
			return rwop32(op & 7, lsr32, 4);
		case 0422: // ROXR.L #4,Dy
			return rwop32(op & 7, roxr32, 4);
		case 0423: // ROR.L #4,Dy
			return rwop32(op & 7, ror32, 4);
		case 0424: // ASR.L D4,Dy
			return rwop32(op & 7, asr32, d4);
		case 0425: // LSR.L D4,Dy
			return rwop32(op & 7, lsr32, d4);
		case 0426: // ROXR.L D4,Dy
			return rwop32(op & 7, roxr32, d4);
		case 0427: // ROR.L D4,Dy
			return rwop32(op & 7, ror32, d4);
		case 0440: // ASL.B #4,Dy
			return rwop8(op & 7, asl8, 4);
		case 0441: // LSL.B #4,Dy
			return rwop8(op & 7, lsl8, 4);
		case 0442: // ROXL.B #4,Dy
			return rwop8(op & 7, roxl8, 4);
		case 0443: // ROL.B #4,Dy
			return rwop8(op & 7, rol8, 4);
		case 0444: // ASL.B D4,Dy
			return rwop8(op & 7, asl8, d4);
		case 0445: // LSL.B D4,Dy
			return rwop8(op & 7, lsl8, d4);
		case 0446: // ROXL.B D4,Dy
			return rwop8(op & 7, roxl8, d4);
		case 0447: // ROL.B D4,Dy
			return rwop8(op & 7, rol8, d4);
		case 0450: // ASL.W #4,Dy
			return rwop16(op & 7, asl16, 4);
		case 0451: // LSL.W #4,Dy
			return rwop16(op & 7, lsl16, 4);
		case 0452: // ROXL.W #4,Dy
			return rwop16(op & 7, roxl16, 4);
		case 0453: // ROL.W #4,Dy
			return rwop16(op & 7, rol16, 4);
		case 0454: // ASL.W D4,Dy
			return rwop16(op & 7, asl16, d4);
		case 0455: // LSL.W D4,Dy
			return rwop16(op & 7, lsl16, d4);
		case 0456: // ROXL.W D4,Dy
			return rwop16(op & 7, roxl16, d4);
		case 0457: // ROL.W D4,Dy
			return rwop16(op & 7, rol16, d4);
		case 0460: // ASL.L #4,Dy
			return rwop32(op & 7, asl32, 4);
		case 0461: // LSL.L #4,Dy
			return rwop32(op & 7, lsl32, 4);
		case 0462: // ROXL.L #4,Dy
			return rwop32(op & 7, roxl32, 4);
		case 0463: // ROL.L #4,Dy
			return rwop32(op & 7, rol32, 4);
		case 0464: // ASL.L D4,Dy
			return rwop32(op & 7, asl32, d4);
		case 0465: // LSL.L D4,Dy
			return rwop32(op & 7, lsl32, d4);
		case 0466: // ROXL.L D4,Dy
			return rwop32(op & 7, roxl32, d4);
		case 0467: // ROL.L D4,Dy
			return rwop32(op & 7, rol32, d4);
		case 0500: // ASR.B #5,Dy
			return rwop8(op & 7, asr8, 5);
		case 0501: // LSR.B #5,Dy
			return rwop8(op & 7, lsr8, 5);
		case 0502: // ROXR.B #5,Dy
			return rwop8(op & 7, roxr8, 5);
		case 0503: // ROR.B #5,Dy
			return rwop8(op & 7, ror8, 5);
		case 0504: // ASR.B D5,Dy
			return rwop8(op & 7, asr8, d5);
		case 0505: // LSR.B D5,Dy
			return rwop8(op & 7, lsr8, d5);
		case 0506: // ROXR.B D5,Dy
			return rwop8(op & 7, roxr8, d5);
		case 0507: // ROR.B D5,Dy
			return rwop8(op & 7, ror8, d5);
		case 0510: // ASR.W #5,Dy
			return rwop16(op & 7, asr16, 5);
		case 0511: // LSR.W #5,Dy
			return rwop16(op & 7, lsr16, 5);
		case 0512: // ROXR.W #5,Dy
			return rwop16(op & 7, roxr16, 5);
		case 0513: // ROR.W #5,Dy
			return rwop16(op & 7, ror16, 5);
		case 0514: // ASR.W D5,Dy
			return rwop16(op & 7, asr16, d5);
		case 0515: // LSR.W D5,Dy
			return rwop16(op & 7, lsr16, d5);
		case 0516: // ROXR.W D5,Dy
			return rwop16(op & 7, roxr16, d5);
		case 0517: // ROR.W D5,Dy
			return rwop16(op & 7, ror16, d5);
		case 0520: // ASR.L #5,Dy
			return rwop32(op & 7, asr32, 5);
		case 0521: // LSR.L #5,Dy
			return rwop32(op & 7, lsr32, 5);
		case 0522: // ROXR.L #5,Dy
			return rwop32(op & 7, roxr32, 5);
		case 0523: // ROR.L #5,Dy
			return rwop32(op & 7, ror32, 5);
		case 0524: // ASR.L D5,Dy
			return rwop32(op & 7, asr32, d5);
		case 0525: // LSR.L D5,Dy
			return rwop32(op & 7, lsr32, d5);
		case 0526: // ROXR.L D5,Dy
			return rwop32(op & 7, roxr32, d5);
		case 0527: // ROR.L D5,Dy
			return rwop32(op & 7, ror32, d5);
		case 0540: // ASL.B #5,Dy
			return rwop8(op & 7, asl8, 5);
		case 0541: // LSL.B #5,Dy
			return rwop8(op & 7, lsl8, 5);
		case 0542: // ROXL.B #5,Dy
			return rwop8(op & 7, roxl8, 5);
		case 0543: // ROL.B #5,Dy
			return rwop8(op & 7, rol8, 5);
		case 0544: // ASL.B D5,Dy
			return rwop8(op & 7, asl8, d5);
		case 0545: // LSL.B D5,Dy
			return rwop8(op & 7, lsl8, d5);
		case 0546: // ROXL.B D5,Dy
			return rwop8(op & 7, roxl8, d5);
		case 0547: // ROL.B D5,Dy
			return rwop8(op & 7, rol8, d5);
		case 0550: // ASL.W #5,Dy
			return rwop16(op & 7, asl16, 5);
		case 0551: // LSL.W #5,Dy
			return rwop16(op & 7, lsl16, 5);
		case 0552: // ROXL.W #5,Dy
			return rwop16(op & 7, roxl16, 5);
		case 0553: // ROL.W #5,Dy
			return rwop16(op & 7, rol16, 5);
		case 0554: // ASL.W D5,Dy
			return rwop16(op & 7, asl16, d5);
		case 0555: // LSL.W D5,Dy
			return rwop16(op & 7, lsl16, d5);
		case 0556: // ROXL.W D5,Dy
			return rwop16(op & 7, roxl16, d5);
		case 0557: // ROL.W D5,Dy
			return rwop16(op & 7, rol16, d5);
		case 0560: // ASL.L #5,Dy
			return rwop32(op & 7, asl32, 5);
		case 0561: // LSL.L #5,Dy
			return rwop32(op & 7, lsl32, 5);
		case 0562: // ROXL.L #5,Dy
			return rwop32(op & 7, roxl32, 5);
		case 0563: // ROL.L #5,Dy
			return rwop32(op & 7, rol32, 5);
		case 0564: // ASL.L D5,Dy
			return rwop32(op & 7, asl32, d5);
		case 0565: // LSL.L D5,Dy
			return rwop32(op & 7, lsl32, d5);
		case 0566: // ROXL.L D5,Dy
			return rwop32(op & 7, roxl32, d5);
		case 0567: // ROL.L D5,Dy
			return rwop32(op & 7, rol32, d5);
		case 0600: // ASR.B #6,Dy
			return rwop8(op & 7, asr8, 6);
		case 0601: // LSR.B #6,Dy
			return rwop8(op & 7, lsr8, 6);
		case 0602: // ROXR.B #6,Dy
			return rwop8(op & 7, roxr8, 6);
		case 0603: // ROR.B #6,Dy
			return rwop8(op & 7, ror8, 6);
		case 0604: // ASR.B D6,Dy
			return rwop8(op & 7, asr8, d6);
		case 0605: // LSR.B D6,Dy
			return rwop8(op & 7, lsr8, d6);
		case 0606: // ROXR.B D6,Dy
			return rwop8(op & 7, roxr8, d6);
		case 0607: // ROR.B D6,Dy
			return rwop8(op & 7, ror8, d6);
		case 0610: // ASR.W #6,Dy
			return rwop16(op & 7, asr16, 6);
		case 0611: // LSR.W #6,Dy
			return rwop16(op & 7, lsr16, 6);
		case 0612: // ROXR.W #6,Dy
			return rwop16(op & 7, roxr16, 6);
		case 0613: // ROR.W #6,Dy
			return rwop16(op & 7, ror16, 6);
		case 0614: // ASR.W D6,Dy
			return rwop16(op & 7, asr16, d6);
		case 0615: // LSR.W D6,Dy
			return rwop16(op & 7, lsr16, d6);
		case 0616: // ROXR.W D6,Dy
			return rwop16(op & 7, roxr16, d6);
		case 0617: // ROR.W D6,Dy
			return rwop16(op & 7, ror16, d6);
		case 0620: // ASR.L #6,Dy
			return rwop32(op & 7, asr32, 6);
		case 0621: // LSR.L #6,Dy
			return rwop32(op & 7, lsr32, 6);
		case 0622: // ROXR.L #6,Dy
			return rwop32(op & 7, roxr32, 6);
		case 0623: // ROR.L #6,Dy
			return rwop32(op & 7, ror32, 6);
		case 0624: // ASR.L D6,Dy
			return rwop32(op & 7, asr32, d6);
		case 0625: // LSR.L D6,Dy
			return rwop32(op & 7, lsr32, d6);
		case 0626: // ROXR.L D6,Dy
			return rwop32(op & 7, roxr32, d6);
		case 0627: // ROR.L D6,Dy
			return rwop32(op & 7, ror32, d6);
		case 0640: // ASL.B #6,Dy
			return rwop8(op & 7, asl8, 6);
		case 0641: // LSL.B #6,Dy
			return rwop8(op & 7, lsl8, 6);
		case 0642: // ROXL.B #6,Dy
			return rwop8(op & 7, roxl8, 6);
		case 0643: // ROL.B #6,Dy
			return rwop8(op & 7, rol8, 6);
		case 0644: // ASL.B D6,Dy
			return rwop8(op & 7, asl8, d6);
		case 0645: // LSL.B D6,Dy
			return rwop8(op & 7, lsl8, d6);
		case 0646: // ROXL.B D6,Dy
			return rwop8(op & 7, roxl8, d6);
		case 0647: // ROL.B D6,Dy
			return rwop8(op & 7, rol8, d6);
		case 0650: // ASL.W #6,Dy
			return rwop16(op & 7, asl16, 6);
		case 0651: // LSL.W #6,Dy
			return rwop16(op & 7, lsl16, 6);
		case 0652: // ROXL.W #6,Dy
			return rwop16(op & 7, roxl16, 6);
		case 0653: // ROL.W #6,Dy
			return rwop16(op & 7, rol16, 6);
		case 0654: // ASL.W D6,Dy
			return rwop16(op & 7, asl16, d6);
		case 0655: // LSL.W D6,Dy
			return rwop16(op & 7, lsl16, d6);
		case 0656: // ROXL.W D6,Dy
			return rwop16(op & 7, roxl16, d6);
		case 0657: // ROL.W D6,Dy
			return rwop16(op & 7, rol16, d6);
		case 0660: // ASL.L #6,Dy
			return rwop32(op & 7, asl32, 6);
		case 0661: // LSL.L #6,Dy
			return rwop32(op & 7, lsl32, 6);
		case 0662: // ROXL.L #6,Dy
			return rwop32(op & 7, roxl32, 6);
		case 0663: // ROL.L #6,Dy
			return rwop32(op & 7, rol32, 6);
		case 0664: // ASL.L D6,Dy
			return rwop32(op & 7, asl32, d6);
		case 0665: // LSL.L D6,Dy
			return rwop32(op & 7, lsl32, d6);
		case 0666: // ROXL.L D6,Dy
			return rwop32(op & 7, roxl32, d6);
		case 0667: // ROL.L D6,Dy
			return rwop32(op & 7, rol32, d6);
		case 0700: // ASR.B #7,Dy
			return rwop8(op & 7, asr8, 7);
		case 0701: // LSR.B #7,Dy
			return rwop8(op & 7, lsr8, 7);
		case 0702: // ROXR.B #7,Dy
			return rwop8(op & 7, roxr8, 7);
		case 0703: // ROR.B #7,Dy
			return rwop8(op & 7, ror8, 7);
		case 0704: // ASR.B D7,Dy
			return rwop8(op & 7, asr8, d7);
		case 0705: // LSR.B D7,Dy
			return rwop8(op & 7, lsr8, d7);
		case 0706: // ROXR.B D7,Dy
			return rwop8(op & 7, roxr8, d7);
		case 0707: // ROR.B D7,Dy
			return rwop8(op & 7, ror8, d7);
		case 0710: // ASR.W #7,Dy
			return rwop16(op & 7, asr16, 7);
		case 0711: // LSR.W #7,Dy
			return rwop16(op & 7, lsr16, 7);
		case 0712: // ROXR.W #7,Dy
			return rwop16(op & 7, roxr16, 7);
		case 0713: // ROR.W #7,Dy
			return rwop16(op & 7, ror16, 7);
		case 0714: // ASR.W D7,Dy
			return rwop16(op & 7, asr16, d7);
		case 0715: // LSR.W D7,Dy
			return rwop16(op & 7, lsr16, d7);
		case 0716: // ROXR.W D7,Dy
			return rwop16(op & 7, roxr16, d7);
		case 0717: // ROR.W D7,Dy
			return rwop16(op & 7, ror16, d7);
		case 0720: // ASR.L #7,Dy
			return rwop32(op & 7, asr32, 7);
		case 0721: // LSR.L #7,Dy
			return rwop32(op & 7, lsr32, 7);
		case 0722: // ROXR.L #7,Dy
			return rwop32(op & 7, roxr32, 7);
		case 0723: // ROR.L #7,Dy
			return rwop32(op & 7, ror32, 7);
		case 0724: // ASR.L D7,Dy
			return rwop32(op & 7, asr32, d7);
		case 0725: // LSR.L D7,Dy
			return rwop32(op & 7, lsr32, d7);
		case 0726: // ROXR.L D7,Dy
			return rwop32(op & 7, roxr32, d7);
		case 0727: // ROR.L D7,Dy
			return rwop32(op & 7, ror32, d7);
		case 0740: // ASL.B #7,Dy
			return rwop8(op & 7, asl8, 7);
		case 0741: // LSL.B #7,Dy
			return rwop8(op & 7, lsl8, 7);
		case 0742: // ROXL.B #7,Dy
			return rwop8(op & 7, roxl8, 7);
		case 0743: // ROL.B #7,Dy
			return rwop8(op & 7, rol8, 7);
		case 0744: // ASL.B D7,Dy
			return rwop8(op & 7, asl8, d7);
		case 0745: // LSL.B D7,Dy
			return rwop8(op & 7, lsl8, d7);
		case 0746: // ROXL.B D7,Dy
			return rwop8(op & 7, roxl8, d7);
		case 0747: // ROL.B D7,Dy
			return rwop8(op & 7, rol8, d7);
		case 0750: // ASL.W #7,Dy
			return rwop16(op & 7, asl16, 7);
		case 0751: // LSL.W #7,Dy
			return rwop16(op & 7, lsl16, 7);
		case 0752: // ROXL.W #7,Dy
			return rwop16(op & 7, roxl16, 7);
		case 0753: // ROL.W #7,Dy
			return rwop16(op & 7, rol16, 7);
		case 0754: // ASL.W D7,Dy
			return rwop16(op & 7, asl16, d7);
		case 0755: // LSL.W D7,Dy
			return rwop16(op & 7, lsl16, d7);
		case 0756: // ROXL.W D7,Dy
			return rwop16(op & 7, roxl16, d7);
		case 0757: // ROL.W D7,Dy
			return rwop16(op & 7, rol16, d7);
		case 0760: // ASL.L #7,Dy
			return rwop32(op & 7, asl32, 7);
		case 0761: // LSL.L #7,Dy
			return rwop32(op & 7, lsl32, 7);
		case 0762: // ROXL.L #7,Dy
			return rwop32(op & 7, roxl32, 7);
		case 0763: // ROL.L #7,Dy
			return rwop32(op & 7, rol32, 7);
		case 0764: // ASL.L D7,Dy
			return rwop32(op & 7, asl32, d7);
		case 0765: // LSL.L D7,Dy
			return rwop32(op & 7, lsl32, d7);
		case 0766: // ROXL.L D7,Dy
			return rwop32(op & 7, roxl32, d7);
		case 0767: // ROL.L D7,Dy
			return rwop32(op & 7, rol32, d7);
		default:
			return exception(4);
		}
	}

	void rwop8(int op, function<int(int src, int dst, int& sr)> fn, int src = 0) {
		int ea;
		switch(op & 077) {
		case 000: // B D0
			return void(d0 = d0 & ~0xff | fn(src, d0, sr));
		case 001: // B D1
			return void(d1 = d1 & ~0xff | fn(src, d1, sr));
		case 002: // B D2
			return void(d2 = d2 & ~0xff | fn(src, d2, sr));
		case 003: // B D3
			return void(d3 = d3 & ~0xff | fn(src, d3, sr));
		case 004: // B D4
			return void(d4 = d4 & ~0xff | fn(src, d4, sr));
		case 005: // B D5
			return void(d5 = d5 & ~0xff | fn(src, d5, sr));
		case 006: // B D6
			return void(d6 = d6 & ~0xff | fn(src, d6, sr));
		case 007: // B D7
			return void(d7 = d7 & ~0xff | fn(src, d7, sr));
		case 020: // B (A0)
			return write8(fn(src, read8(a0), sr), a0);
		case 021: // B (A1)
			return write8(fn(src, read8(a1), sr), a1);
		case 022: // B (A2)
			return write8(fn(src, read8(a2), sr), a2);
		case 023: // B (A3)
			return write8(fn(src, read8(a3), sr), a3);
		case 024: // B (A4)
			return write8(fn(src, read8(a4), sr), a4);
		case 025: // B (A5)
			return write8(fn(src, read8(a5), sr), a5);
		case 026: // B (A6)
			return write8(fn(src, read8(a6), sr), a6);
		case 027: // B (A7)
			return write8(fn(src, read8(a7), sr), a7);
		case 030: // B (A0)+
			return write8(fn(src, read8(a0), sr), a0), void(a0 = a0 + 1);
		case 031: // B (A1)+
			return write8(fn(src, read8(a1), sr), a1), void(a1 = a1 + 1);
		case 032: // B (A2)+
			return write8(fn(src, read8(a2), sr), a2), void(a2 = a2 + 1);
		case 033: // B (A3)+
			return write8(fn(src, read8(a3), sr), a3), void(a3 = a3 + 1);
		case 034: // B (A4)+
			return write8(fn(src, read8(a4), sr), a4), void(a4 = a4 + 1);
		case 035: // B (A5)+
			return write8(fn(src, read8(a5), sr), a5), void(a5 = a5 + 1);
		case 036: // B (A6)+
			return write8(fn(src, read8(a6), sr), a6), void(a6 = a6 + 1);
		case 037: // B (A7)+
			return write8(fn(src, read8(a7), sr), a7), void(a7 = a7 + 1);
		case 040: // B -(A0)
			return a0 = a0 - 1, write8(fn(src, read8(a0), sr), a0);
		case 041: // B -(A1)
			return a1 = a1 - 1, write8(fn(src, read8(a1), sr), a1);
		case 042: // B -(A2)
			return a2 = a2 - 1, write8(fn(src, read8(a2), sr), a2);
		case 043: // B -(A3)
			return a3 = a3 - 1, write8(fn(src, read8(a3), sr), a3);
		case 044: // B -(A4)
			return a4 = a4 - 1, write8(fn(src, read8(a4), sr), a4);
		case 045: // B -(A5)
			return a5 = a5 - 1, write8(fn(src, read8(a5), sr), a5);
		case 046: // B -(A6)
			return a6 = a6 - 1, write8(fn(src, read8(a6), sr), a6);
		case 047: // B -(A7)
			return a7 = a7 - 1, write8(fn(src, read8(a7), sr), a7);
		case 050: // B d(A0)
			return ea = disp(a0), write8(fn(src, read8(ea), sr), ea);
		case 051: // B d(A1)
			return ea = disp(a1), write8(fn(src, read8(ea), sr), ea);
		case 052: // B d(A2)
			return ea = disp(a2), write8(fn(src, read8(ea), sr), ea);
		case 053: // B d(A3)
			return ea = disp(a3), write8(fn(src, read8(ea), sr), ea);
		case 054: // B d(A4)
			return ea = disp(a4), write8(fn(src, read8(ea), sr), ea);
		case 055: // B d(A5)
			return ea = disp(a5), write8(fn(src, read8(ea), sr), ea);
		case 056: // B d(A6)
			return ea = disp(a6), write8(fn(src, read8(ea), sr), ea);
		case 057: // B d(A7)
			return ea = disp(a7), write8(fn(src, read8(ea), sr), ea);
		case 060: // B d(A0,Xi)
			return ea = index(a0), write8(fn(src, read8(ea), sr), ea);
		case 061: // B d(A1,Xi)
			return ea = index(a1), write8(fn(src, read8(ea), sr), ea);
		case 062: // B d(A2,Xi)
			return ea = index(a2), write8(fn(src, read8(ea), sr), ea);
		case 063: // B d(A3,Xi)
			return ea = index(a3), write8(fn(src, read8(ea), sr), ea);
		case 064: // B d(A4,Xi)
			return ea = index(a4), write8(fn(src, read8(ea), sr), ea);
		case 065: // B d(A5,Xi)
			return ea = index(a5), write8(fn(src, read8(ea), sr), ea);
		case 066: // B d(A6,Xi)
			return ea = index(a6), write8(fn(src, read8(ea), sr), ea);
		case 067: // B d(A7,Xi)
			return ea = index(a7), write8(fn(src, read8(ea), sr), ea);
		case 070: // B Abs.W
			return ea = fetch16s(), write8(fn(src, read8(ea), sr), ea);
		case 071: // B Abs.L
			return ea = fetch32(), write8(fn(src, read8(ea), sr), ea);
		default:
			return;
		}
	}

	void rwop16(int op, function<int(int src, int dst, int& sr)> fn, int src = 0) {
		int ea;
		switch(op & 077) {
		case 000: // W D0
			return void(d0 = d0 & ~0xffff | fn(src, d0, sr));
		case 001: // W D1
			return void(d1 = d1 & ~0xffff | fn(src, d1, sr));
		case 002: // W D2
			return void(d2 = d2 & ~0xffff | fn(src, d2, sr));
		case 003: // W D3
			return void(d3 = d3 & ~0xffff | fn(src, d3, sr));
		case 004: // W D4
			return void(d4 = d4 & ~0xffff | fn(src, d4, sr));
		case 005: // W D5
			return void(d5 = d5 & ~0xffff | fn(src, d5, sr));
		case 006: // W D6
			return void(d6 = d6 & ~0xffff | fn(src, d6, sr));
		case 007: // W D7
			return void(d7 = d7 & ~0xffff | fn(src, d7, sr));
		case 010: // W A0
			return void(a0 = fn(src, a0, sr));
		case 011: // W A1
			return void(a1 = fn(src, a1, sr));
		case 012: // W A2
			return void(a2 = fn(src, a2, sr));
		case 013: // W A3
			return void(a3 = fn(src, a3, sr));
		case 014: // W A4
			return void(a4 = fn(src, a4, sr));
		case 015: // W A5
			return void(a5 = fn(src, a5, sr));
		case 016: // W A6
			return void(a6 = fn(src, a6, sr));
		case 017: // W A7
			return void(a7 = fn(src, a7, sr));
		case 020: // W (A0)
			return write16(fn(src, read16(a0), sr), a0);
		case 021: // W (A1)
			return write16(fn(src, read16(a1), sr), a1);
		case 022: // W (A2)
			return write16(fn(src, read16(a2), sr), a2);
		case 023: // W (A3)
			return write16(fn(src, read16(a3), sr), a3);
		case 024: // W (A4)
			return write16(fn(src, read16(a4), sr), a4);
		case 025: // W (A5)
			return write16(fn(src, read16(a5), sr), a5);
		case 026: // W (A6)
			return write16(fn(src, read16(a6), sr), a6);
		case 027: // W (A7)
			return write16(fn(src, read16(a7), sr), a7);
		case 030: // W (A0)+
			return write16(fn(src, read16(a0), sr), a0), void(a0 = a0 + 2);
		case 031: // W (A1)+
			return write16(fn(src, read16(a1), sr), a1), void(a1 = a1 + 2);
		case 032: // W (A2)+
			return write16(fn(src, read16(a2), sr), a2), void(a2 = a2 + 2);
		case 033: // W (A3)+
			return write16(fn(src, read16(a3), sr), a3), void(a3 = a3 + 2);
		case 034: // W (A4)+
			return write16(fn(src, read16(a4), sr), a4), void(a4 = a4 + 2);
		case 035: // W (A5)+
			return write16(fn(src, read16(a5), sr), a5), void(a5 = a5 + 2);
		case 036: // W (A6)+
			return write16(fn(src, read16(a6), sr), a6), void(a6 = a6 + 2);
		case 037: // W (A7)+
			return write16(fn(src, read16(a7), sr), a7), void(a7 = a7 + 2);
		case 040: // W -(A0)
			return a0 = a0 - 2, write16(fn(src, read16(a0), sr), a0);
		case 041: // W -(A1)
			return a1 = a1 - 2, write16(fn(src, read16(a1), sr), a1);
		case 042: // W -(A2)
			return a2 = a2 - 2, write16(fn(src, read16(a2), sr), a2);
		case 043: // W -(A3)
			return a3 = a3 - 2, write16(fn(src, read16(a3), sr), a3);
		case 044: // W -(A4)
			return a4 = a4 - 2, write16(fn(src, read16(a4), sr), a4);
		case 045: // W -(A5)
			return a5 = a5 - 2, write16(fn(src, read16(a5), sr), a5);
		case 046: // W -(A6)
			return a6 = a6 - 2, write16(fn(src, read16(a6), sr), a6);
		case 047: // W -(A7)
			return a7 = a7 - 2, write16(fn(src, read16(a7), sr), a7);
		case 050: // W d(A0)
			return ea = disp(a0), write16(fn(src, read16(ea), sr), ea);
		case 051: // W d(A1)
			return ea = disp(a1), write16(fn(src, read16(ea), sr), ea);
		case 052: // W d(A2)
			return ea = disp(a2), write16(fn(src, read16(ea), sr), ea);
		case 053: // W d(A3)
			return ea = disp(a3), write16(fn(src, read16(ea), sr), ea);
		case 054: // W d(A4)
			return ea = disp(a4), write16(fn(src, read16(ea), sr), ea);
		case 055: // W d(A5)
			return ea = disp(a5), write16(fn(src, read16(ea), sr), ea);
		case 056: // W d(A6)
			return ea = disp(a6), write16(fn(src, read16(ea), sr), ea);
		case 057: // W d(A7)
			return ea = disp(a7), write16(fn(src, read16(ea), sr), ea);
		case 060: // W d(A0,Xi)
			return ea = index(a0), write16(fn(src, read16(ea), sr), ea);
		case 061: // W d(A1,Xi)
			return ea = index(a1), write16(fn(src, read16(ea), sr), ea);
		case 062: // W d(A2,Xi)
			return ea = index(a2), write16(fn(src, read16(ea), sr), ea);
		case 063: // W d(A3,Xi)
			return ea = index(a3), write16(fn(src, read16(ea), sr), ea);
		case 064: // W d(A4,Xi)
			return ea = index(a4), write16(fn(src, read16(ea), sr), ea);
		case 065: // W d(A5,Xi)
			return ea = index(a5), write16(fn(src, read16(ea), sr), ea);
		case 066: // W d(A6,Xi)
			return ea = index(a6), write16(fn(src, read16(ea), sr), ea);
		case 067: // W d(A7,Xi)
			return ea = index(a7), write16(fn(src, read16(ea), sr), ea);
		case 070: // W Abs.W
			return ea = fetch16s(), write16(fn(src, read16(ea), sr), ea);
		case 071: // W Abs.L
			return ea = fetch32(), write16(fn(src, read16(ea), sr), ea);
		default:
			return;
		}
	}

	void rwop32(int op, function<int(int src, int dst, int& sr)> fn, int src = 0) {
		int ea;
		switch(op & 077) {
		case 000: // L D0
			return void(d0 = fn(src, d0, sr));
		case 001: // L D1
			return void(d1 = fn(src, d1, sr));
		case 002: // L D2
			return void(d2 = fn(src, d2, sr));
		case 003: // L D3
			return void(d3 = fn(src, d3, sr));
		case 004: // L D4
			return void(d4 = fn(src, d4, sr));
		case 005: // L D5
			return void(d5 = fn(src, d5, sr));
		case 006: // L D6
			return void(d6 = fn(src, d6, sr));
		case 007: // L D7
			return void(d7 = fn(src, d7, sr));
		case 010: // L A0
			return void(a0 = fn(src, a0, sr));
		case 011: // L A1
			return void(a1 = fn(src, a1, sr));
		case 012: // L A2
			return void(a2 = fn(src, a2, sr));
		case 013: // L A3
			return void(a3 = fn(src, a3, sr));
		case 014: // L A4
			return void(a4 = fn(src, a4, sr));
		case 015: // L A5
			return void(a5 = fn(src, a5, sr));
		case 016: // L A6
			return void(a6 = fn(src, a6, sr));
		case 017: // L A7
			return void(a7 = fn(src, a7, sr));
		case 020: // L (A0)
			return write32(fn(src, read32(a0), sr), a0);
		case 021: // L (A1)
			return write32(fn(src, read32(a1), sr), a1);
		case 022: // L (A2)
			return write32(fn(src, read32(a2), sr), a2);
		case 023: // L (A3)
			return write32(fn(src, read32(a3), sr), a3);
		case 024: // L (A4)
			return write32(fn(src, read32(a4), sr), a4);
		case 025: // L (A5)
			return write32(fn(src, read32(a5), sr), a5);
		case 026: // L (A6)
			return write32(fn(src, read32(a6), sr), a6);
		case 027: // L (A7)
			return write32(fn(src, read32(a7), sr), a7);
		case 030: // L (A0)+
			return write32(fn(src, read32(a0), sr), a0), void(a0 = a0 + 4);
		case 031: // L (A1)+
			return write32(fn(src, read32(a1), sr), a1), void(a1 = a1 + 4);
		case 032: // L (A2)+
			return write32(fn(src, read32(a2), sr), a2), void(a2 = a2 + 4);
		case 033: // L (A3)+
			return write32(fn(src, read32(a3), sr), a3), void(a3 = a3 + 4);
		case 034: // L (A4)+
			return write32(fn(src, read32(a4), sr), a4), void(a4 = a4 + 4);
		case 035: // L (A5)+
			return write32(fn(src, read32(a5), sr), a5), void(a5 = a5 + 4);
		case 036: // L (A6)+
			return write32(fn(src, read32(a6), sr), a6), void(a6 = a6 + 4);
		case 037: // L (A7)+
			return write32(fn(src, read32(a7), sr), a7), void(a7 = a7 + 4);
		case 040: // L -(A0)
			return a0 = a0 - 4, write32(fn(src, read32(a0), sr), a0);
		case 041: // L -(A1)
			return a1 = a1 - 4, write32(fn(src, read32(a1), sr), a1);
		case 042: // L -(A2)
			return a2 = a2 - 4, write32(fn(src, read32(a2), sr), a2);
		case 043: // L -(A3)
			return a3 = a3 - 4, write32(fn(src, read32(a3), sr), a3);
		case 044: // L -(A4)
			return a4 = a4 - 4, write32(fn(src, read32(a4), sr), a4);
		case 045: // L -(A5)
			return a5 = a5 - 4, write32(fn(src, read32(a5), sr), a5);
		case 046: // L -(A6)
			return a6 = a6 - 4, write32(fn(src, read32(a6), sr), a6);
		case 047: // L -(A7)
			return a7 = a7 - 4, write32(fn(src, read32(a7), sr), a7);
		case 050: // L d(A0)
			return ea = disp(a0), write32(fn(src, read32(ea), sr), ea);
		case 051: // L d(A1)
			return ea = disp(a1), write32(fn(src, read32(ea), sr), ea);
		case 052: // L d(A2)
			return ea = disp(a2), write32(fn(src, read32(ea), sr), ea);
		case 053: // L d(A3)
			return ea = disp(a3), write32(fn(src, read32(ea), sr), ea);
		case 054: // L d(A4)
			return ea = disp(a4), write32(fn(src, read32(ea), sr), ea);
		case 055: // L d(A5)
			return ea = disp(a5), write32(fn(src, read32(ea), sr), ea);
		case 056: // L d(A6)
			return ea = disp(a6), write32(fn(src, read32(ea), sr), ea);
		case 057: // L d(A7)
			return ea = disp(a7), write32(fn(src, read32(ea), sr), ea);
		case 060: // L d(A0,Xi)
			return ea = index(a0), write32(fn(src, read32(ea), sr), ea);
		case 061: // L d(A1,Xi)
			return ea = index(a1), write32(fn(src, read32(ea), sr), ea);
		case 062: // L d(A2,Xi)
			return ea = index(a2), write32(fn(src, read32(ea), sr), ea);
		case 063: // L d(A3,Xi)
			return ea = index(a3), write32(fn(src, read32(ea), sr), ea);
		case 064: // L d(A4,Xi)
			return ea = index(a4), write32(fn(src, read32(ea), sr), ea);
		case 065: // L d(A5,Xi)
			return ea = index(a5), write32(fn(src, read32(ea), sr), ea);
		case 066: // L d(A6,Xi)
			return ea = index(a6), write32(fn(src, read32(ea), sr), ea);
		case 067: // L d(A7,Xi)
			return ea = index(a7), write32(fn(src, read32(ea), sr), ea);
		case 070: // L Abs.W
			return ea = fetch16s(), write32(fn(src, read32(ea), sr), ea);
		case 071: // L Abs.L
			return ea = fetch32(), write32(fn(src, read32(ea), sr), ea);
		default:
			return;
		}
	}

	int rop8(int op, int flag = false) {
		int data = 0xff;
		switch(op & 077) {
		case 000: // B D0
			data = d0 & 0xff;
			break;
		case 001: // B D1
			data = d1 & 0xff;
			break;
		case 002: // B D2
			data = d2 & 0xff;
			break;
		case 003: // B D3
			data = d3 & 0xff;
			break;
		case 004: // B D4
			data = d4 & 0xff;
			break;
		case 005: // B D5
			data = d5 & 0xff;
			break;
		case 006: // B D6
			data = d6 & 0xff;
			break;
		case 007: // B D7
			data = d7 & 0xff;
			break;
		case 020: // B (A0)
			data = read8(a0);
			break;
		case 021: // B (A1)
			data = read8(a1);
			break;
		case 022: // B (A2)
			data = read8(a2);
			break;
		case 023: // B (A3)
			data = read8(a3);
			break;
		case 024: // B (A4)
			data = read8(a4);
			break;
		case 025: // B (A5)
			data = read8(a5);
			break;
		case 026: // B (A6)
			data = read8(a6);
			break;
		case 027: // B (A7)
			data = read8(a7);
			break;
		case 030: // B (A0)+
			data = read8(a0), a0 = a0 + 1;
			break;
		case 031: // B (A1)+
			data = read8(a1), a1 = a1 + 1;
			break;
		case 032: // B (A2)+
			data = read8(a2), a2 = a2 + 1;
			break;
		case 033: // B (A3)+
			data = read8(a3), a3 = a3 + 1;
			break;
		case 034: // B (A4)+
			data = read8(a4), a4 = a4 + 1;
			break;
		case 035: // B (A5)+
			data = read8(a5), a5 = a5 + 1;
			break;
		case 036: // B (A6)+
			data = read8(a6), a6 = a6 + 1;
			break;
		case 037: // B (A7)+
			data = read8(a7), a7 = a7 + 1;
			break;
		case 040: // B -(A0)
			a0 = a0 - 1, data = read8(a0);
			break;
		case 041: // B -(A1)
			a1 = a1 - 1, data = read8(a1);
			break;
		case 042: // B -(A2)
			a2 = a2 - 1, data = read8(a2);
			break;
		case 043: // B -(A3)
			a3 = a3 - 1, data = read8(a3);
			break;
		case 044: // B -(A4)
			a4 = a4 - 1, data = read8(a4);
			break;
		case 045: // B -(A5)
			a5 = a5 - 1, data = read8(a5);
			break;
		case 046: // B -(A6)
			a6 = a6 - 1, data = read8(a6);
			break;
		case 047: // B -(A7)
			a7 = a7 - 1, data = read8(a7);
			break;
		case 050: // B d(A0)
			data = read8(disp(a0));
			break;
		case 051: // B d(A1)
			data = read8(disp(a1));
			break;
		case 052: // B d(A2)
			data = read8(disp(a2));
			break;
		case 053: // B d(A3)
			data = read8(disp(a3));
			break;
		case 054: // B d(A4)
			data = read8(disp(a4));
			break;
		case 055: // B d(A5)
			data = read8(disp(a5));
			break;
		case 056: // B d(A6)
			data = read8(disp(a6));
			break;
		case 057: // B d(A7)
			data = read8(disp(a7));
			break;
		case 060: // B d(A0,Xi)
			data = read8(index(a0));
			break;
		case 061: // B d(A1,Xi)
			data = read8(index(a1));
			break;
		case 062: // B d(A2,Xi)
			data = read8(index(a2));
			break;
		case 063: // B d(A3,Xi)
			data = read8(index(a3));
			break;
		case 064: // B d(A4,Xi)
			data = read8(index(a4));
			break;
		case 065: // B d(A5,Xi)
			data = read8(index(a5));
			break;
		case 066: // B d(A6,Xi)
			data = read8(index(a6));
			break;
		case 067: // B d(A7,Xi)
			data = read8(index(a7));
			break;
		case 070: // B Abs.W
			data = read8(fetch16s());
			break;
		case 071: // B Abs.L
			data = read8(fetch32());
			break;
		case 072: // B d(PC)
			data = read8(disp(pc));
			break;
		case 073: // B d(PC,Xi)
			data = read8(index(pc));
			break;
		case 074: // B #<data>
			data = fetch16() & 0xff;
			break;
		default:
			break;
		}
		if (flag)
			sr = sr & ~0x0f | data >> 4 & 8 | !data << 2;
		return data;
	}

	int rop16(int op, int flag = false) {
		int data = 0xffff;
		switch(op & 077) {
		case 000: // W D0
			data = d0 & 0xffff;
			break;
		case 001: // W D1
			data = d1 & 0xffff;
			break;
		case 002: // W D2
			data = d2 & 0xffff;
			break;
		case 003: // W D3
			data = d3 & 0xffff;
			break;
		case 004: // W D4
			data = d4 & 0xffff;
			break;
		case 005: // W D5
			data = d5 & 0xffff;
			break;
		case 006: // W D6
			data = d6 & 0xffff;
			break;
		case 007: // W D7
			data = d7 & 0xffff;
			break;
		case 010: // W A0
			data = a0 & 0xffff;
			break;
		case 011: // W A1
			data = a1 & 0xffff;
			break;
		case 012: // W A2
			data = a2 & 0xffff;
			break;
		case 013: // W A3
			data = a3 & 0xffff;
			break;
		case 014: // W A4
			data = a4 & 0xffff;
			break;
		case 015: // W A5
			data = a5 & 0xffff;
			break;
		case 016: // W A6
			data = a6 & 0xffff;
			break;
		case 017: // W A7
			data = a7 & 0xffff;
			break;
		case 020: // W (A0)
			data = read16(a0);
			break;
		case 021: // W (A1)
			data = read16(a1);
			break;
		case 022: // W (A2)
			data = read16(a2);
			break;
		case 023: // W (A3)
			data = read16(a3);
			break;
		case 024: // W (A4)
			data = read16(a4);
			break;
		case 025: // W (A5)
			data = read16(a5);
			break;
		case 026: // W (A6)
			data = read16(a6);
			break;
		case 027: // W (A7)
			data = read16(a7);
			break;
		case 030: // W (A0)+
			data = read16(a0), a0 = a0 + 2;
			break;
		case 031: // W (A1)+
			data = read16(a1), a1 = a1 + 2;
			break;
		case 032: // W (A2)+
			data = read16(a2), a2 = a2 + 2;
			break;
		case 033: // W (A3)+
			data = read16(a3), a3 = a3 + 2;
			break;
		case 034: // W (A4)+
			data = read16(a4), a4 = a4 + 2;
			break;
		case 035: // W (A5)+
			data = read16(a5), a5 = a5 + 2;
			break;
		case 036: // W (A6)+
			data = read16(a6), a6 = a6 + 2;
			break;
		case 037: // W (A7)+
			data = read16(a7), a7 = a7 + 2;
			break;
		case 040: // W -(A0)
			a0 = a0 - 2, data = read16(a0);
			break;
		case 041: // W -(A1)
			a1 = a1 - 2, data = read16(a1);
			break;
		case 042: // W -(A2)
			a2 = a2 - 2, data = read16(a2);
			break;
		case 043: // W -(A3)
			a3 = a3 - 2, data = read16(a3);
			break;
		case 044: // W -(A4)
			a4 = a4 - 2, data = read16(a4);
			break;
		case 045: // W -(A5)
			a5 = a5 - 2, data = read16(a5);
			break;
		case 046: // W -(A6)
			a6 = a6 - 2, data = read16(a6);
			break;
		case 047: // W -(A7)
			a7 = a7 - 2, data = read16(a7);
			break;
		case 050: // W d(A0)
			data = read16(disp(a0));
			break;
		case 051: // W d(A1)
			data = read16(disp(a1));
			break;
		case 052: // W d(A2)
			data = read16(disp(a2));
			break;
		case 053: // W d(A3)
			data = read16(disp(a3));
			break;
		case 054: // W d(A4)
			data = read16(disp(a4));
			break;
		case 055: // W d(A5)
			data = read16(disp(a5));
			break;
		case 056: // W d(A6)
			data = read16(disp(a6));
			break;
		case 057: // W d(A7)
			data = read16(disp(a7));
			break;
		case 060: // W d(A0,Xi)
			data = read16(index(a0));
			break;
		case 061: // W d(A1,Xi)
			data = read16(index(a1));
			break;
		case 062: // W d(A2,Xi)
			data = read16(index(a2));
			break;
		case 063: // W d(A3,Xi)
			data = read16(index(a3));
			break;
		case 064: // W d(A4,Xi)
			data = read16(index(a4));
			break;
		case 065: // W d(A5,Xi)
			data = read16(index(a5));
			break;
		case 066: // W d(A6,Xi)
			data = read16(index(a6));
			break;
		case 067: // W d(A7,Xi)
			data = read16(index(a7));
			break;
		case 070: // W Abs.W
			data = read16(fetch16s());
			break;
		case 071: // W Abs.L
			data = read16(fetch32());
			break;
		case 072: // W d(PC)
			data = read16(disp(pc));
			break;
		case 073: // W d(PC,Xi)
			data = read16(index(pc));
			break;
		case 074: // W #<data>
			data = fetch16();
			break;
		default:
			break;
		}
		if (flag)
			sr = sr & ~0x0f | data >> 12 & 8 | !data << 2;
		return data;
	}

	int rop32(int op, int flag = false) {
		int data = 0xffffffff;
		switch(op & 077) {
		case 000: // L D0
			data = d0;
			break;
		case 001: // L D1
			data = d1;
			break;
		case 002: // L D2
			data = d2;
			break;
		case 003: // L D3
			data = d3;
			break;
		case 004: // L D4
			data = d4;
			break;
		case 005: // L D5
			data = d5;
			break;
		case 006: // L D6
			data = d6;
			break;
		case 007: // L D7
			data = d7;
			break;
		case 010: // L A0
			data = a0;
			break;
		case 011: // L A1
			data = a1;
			break;
		case 012: // L A2
			data = a2;
			break;
		case 013: // L A3
			data = a3;
			break;
		case 014: // L A4
			data = a4;
			break;
		case 015: // L A5
			data = a5;
			break;
		case 016: // L A6
			data = a6;
			break;
		case 017: // L A7
			data = a7;
			break;
		case 020: // L (A0)
			data = read32(a0);
			break;
		case 021: // L (A1)
			data = read32(a1);
			break;
		case 022: // L (A2)
			data = read32(a2);
			break;
		case 023: // L (A3)
			data = read32(a3);
			break;
		case 024: // L (A4)
			data = read32(a4);
			break;
		case 025: // L (A5)
			data = read32(a5);
			break;
		case 026: // L (A6)
			data = read32(a6);
			break;
		case 027: // L (A7)
			data = read32(a7);
			break;
		case 030: // L (A0)+
			data = read32(a0), a0 = a0 + 4;
			break;
		case 031: // L (A1)+
			data = read32(a1), a1 = a1 + 4;
			break;
		case 032: // L (A2)+
			data = read32(a2), a2 = a2 + 4;
			break;
		case 033: // L (A3)+
			data = read32(a3), a3 = a3 + 4;
			break;
		case 034: // L (A4)+
			data = read32(a4), a4 = a4 + 4;
			break;
		case 035: // L (A5)+
			data = read32(a5), a5 = a5 + 4;
			break;
		case 036: // L (A6)+
			data = read32(a6), a6 = a6 + 4;
			break;
		case 037: // L (A7)+
			data = read32(a7), a7 = a7 + 4;
			break;
		case 040: // L -(A0)
			a0 = a0 - 4, data = read32(a0);
			break;
		case 041: // L -(A1)
			a1 = a1 - 4, data = read32(a1);
			break;
		case 042: // L -(A2)
			a2 = a2 - 4, data = read32(a2);
			break;
		case 043: // L -(A3)
			a3 = a3 - 4, data = read32(a3);
			break;
		case 044: // L -(A4)
			a4 = a4 - 4, data = read32(a4);
			break;
		case 045: // L -(A5)
			a5 = a5 - 4, data = read32(a5);
			break;
		case 046: // L -(A6)
			a6 = a6 - 4, data = read32(a6);
			break;
		case 047: // L -(A7)
			a7 = a7 - 4, data = read32(a7);
			break;
		case 050: // L d(A0)
			data = read32(disp(a0));
			break;
		case 051: // L d(A1)
			data = read32(disp(a1));
			break;
		case 052: // L d(A2)
			data = read32(disp(a2));
			break;
		case 053: // L d(A3)
			data = read32(disp(a3));
			break;
		case 054: // L d(A4)
			data = read32(disp(a4));
			break;
		case 055: // L d(A5)
			data = read32(disp(a5));
			break;
		case 056: // L d(A6)
			data = read32(disp(a6));
			break;
		case 057: // L d(A7)
			data = read32(disp(a7));
			break;
		case 060: // L d(A0,Xi)
			data = read32(index(a0));
			break;
		case 061: // L d(A1,Xi)
			data = read32(index(a1));
			break;
		case 062: // L d(A2,Xi)
			data = read32(index(a2));
			break;
		case 063: // L d(A3,Xi)
			data = read32(index(a3));
			break;
		case 064: // L d(A4,Xi)
			data = read32(index(a4));
			break;
		case 065: // L d(A5,Xi)
			data = read32(index(a5));
			break;
		case 066: // L d(A6,Xi)
			data = read32(index(a6));
			break;
		case 067: // L d(A7,Xi)
			data = read32(index(a7));
			break;
		case 070: // L Abs.W
			data = read32(fetch16s());
			break;
		case 071: // L Abs.L
			data = read32(fetch32());
			break;
		case 072: // L d(PC)
			data = read32(disp(pc));
			break;
		case 073: // L d(PC,Xi)
			data = read32(index(pc));
			break;
		case 074: // L #<data>
			data = fetch32();
			break;
		default:
			break;
		}
		if (flag)
			sr = sr & ~0x0f | data >> 28 & 8 | !data << 2;
		return data;
	}

	int movep(int op, int src = 0) {
		const int addr = lea(op & 7 | 050);
		switch (op >> 6 & 3){
		case 0: // MOVEP.W d(Ay),Dx
			return read8(addr) << 8 | read8(addr + 2);
		case 1: // MOVEP.L d(Ay),Dx
			return read8(addr) << 24 | read8(addr + 2) << 16 | read8(addr + 4) << 8 | read8(addr + 6);
		case 2: // MOVEP.W Dx,d(Ay)
			return write8(src >> 8, addr), write8(src, addr + 2), -1;
		case 3: // MOVEP.L Dx,d(Ay)
			return write8(src >> 24, addr), write8(src >> 16, addr + 2), write8(src >> 8, addr + 4), write8(src, addr + 6), -1;
		}
		return -1;
	}

	void movem16rm(int op) {
		const int list = fetch16();
		int ea = lea(op);
		if ((op & 070) == 040) {
			if ((list & 1) != 0)
				ea = ea - 2, write16(a7, ea);
			if ((list & 2) != 0)
				ea = ea - 2, write16(a6, ea);
			if ((list & 4) != 0)
				ea = ea - 2, write16(a5, ea);
			if ((list & 8) != 0)
				ea = ea - 2, write16(a4, ea);
			if ((list & 0x10) != 0)
				ea = ea - 2, write16(a3, ea);
			if ((list & 0x20) != 0)
				ea = ea - 2, write16(a2, ea);
			if ((list & 0x40) != 0)
				ea = ea - 2, write16(a1, ea);
			if ((list & 0x80) != 0)
				ea = ea - 2, write16(a0, ea);
			if ((list & 0x100) != 0)
				ea = ea - 2, write16(d7, ea);
			if ((list & 0x200) != 0)
				ea = ea - 2, write16(d6, ea);
			if ((list & 0x400) != 0)
				ea = ea - 2, write16(d5, ea);
			if ((list & 0x800) != 0)
				ea = ea - 2, write16(d4, ea);
			if ((list & 0x1000) != 0)
				ea = ea - 2, write16(d3, ea);
			if ((list & 0x2000) != 0)
				ea = ea - 2, write16(d2, ea);
			if ((list & 0x4000) != 0)
				ea = ea - 2, write16(d1, ea);
			if ((list & 0x8000) != 0)
				ea = ea - 2, write16(d0, ea);
			rwop32(op & 7 | 010, thru, ea);
		}
		else {
			if ((list & 1) != 0)
				write16(d0, ea), ea = ea + 2;
			if ((list & 2) != 0)
				write16(d1, ea), ea = ea + 2;
			if ((list & 4) != 0)
				write16(d2, ea), ea = ea + 2;
			if ((list & 8) != 0)
				write16(d3, ea), ea = ea + 2;
			if ((list & 0x10) != 0)
				write16(d4, ea), ea = ea + 2;
			if ((list & 0x20) != 0)
				write16(d5, ea), ea = ea + 2;
			if ((list & 0x40) != 0)
				write16(d6, ea), ea = ea + 2;
			if ((list & 0x80) != 0)
				write16(d7, ea), ea = ea + 2;
			if ((list & 0x100) != 0)
				write16(a0, ea), ea = ea + 2;
			if ((list & 0x200) != 0)
				write16(a1, ea), ea = ea + 2;
			if ((list & 0x400) != 0)
				write16(a2, ea), ea = ea + 2;
			if ((list & 0x800) != 0)
				write16(a3, ea), ea = ea + 2;
			if ((list & 0x1000) != 0)
				write16(a4, ea), ea = ea + 2;
			if ((list & 0x2000) != 0)
				write16(a5, ea), ea = ea + 2;
			if ((list & 0x4000) != 0)
				write16(a6, ea), ea = ea + 2;
			if ((list & 0x8000) != 0)
				write16(a7, ea);
		}
	}

	void movem32rm(int op) {
		const int list = fetch16();
		int ea = lea(op);
		if ((op & 070) == 040) {
			if ((list & 1) != 0)
				ea = ea - 4, write32(a7, ea);
			if ((list & 2) != 0)
				ea = ea - 4, write32(a6, ea);
			if ((list & 4) != 0)
				ea = ea - 4, write32(a5, ea);
			if ((list & 8) != 0)
				ea = ea - 4, write32(a4, ea);
			if ((list & 0x10) != 0)
				ea = ea - 4, write32(a3, ea);
			if ((list & 0x20) != 0)
				ea = ea - 4, write32(a2, ea);
			if ((list & 0x40) != 0)
				ea = ea - 4, write32(a1, ea);
			if ((list & 0x80) != 0)
				ea = ea - 4, write32(a0, ea);
			if ((list & 0x100) != 0)
				ea = ea - 4, write32(d7, ea);
			if ((list & 0x200) != 0)
				ea = ea - 4, write32(d6, ea);
			if ((list & 0x400) != 0)
				ea = ea - 4, write32(d5, ea);
			if ((list & 0x800) != 0)
				ea = ea - 4, write32(d4, ea);
			if ((list & 0x1000) != 0)
				ea = ea - 4, write32(d3, ea);
			if ((list & 0x2000) != 0)
				ea = ea - 4, write32(d2, ea);
			if ((list & 0x4000) != 0)
				ea = ea - 4, write32(d1, ea);
			if ((list & 0x8000) != 0)
				ea = ea - 4, write32(d0, ea);
			rwop32(op & 7 | 010, thru, ea);
		}
		else {
			if ((list & 1) != 0)
				write32(d0, ea), ea = ea + 4;
			if ((list & 2) != 0)
				write32(d1, ea), ea = ea + 4;
			if ((list & 4) != 0)
				write32(d2, ea), ea = ea + 4;
			if ((list & 8) != 0)
				write32(d3, ea), ea = ea + 4;
			if ((list & 0x10) != 0)
				write32(d4, ea), ea = ea + 4;
			if ((list & 0x20) != 0)
				write32(d5, ea), ea = ea + 4;
			if ((list & 0x40) != 0)
				write32(d6, ea), ea = ea + 4;
			if ((list & 0x80) != 0)
				write32(d7, ea), ea = ea + 4;
			if ((list & 0x100) != 0)
				write32(a0, ea), ea = ea + 4;
			if ((list & 0x200) != 0)
				write32(a1, ea), ea = ea + 4;
			if ((list & 0x400) != 0)
				write32(a2, ea), ea = ea + 4;
			if ((list & 0x800) != 0)
				write32(a3, ea), ea = ea + 4;
			if ((list & 0x1000) != 0)
				write32(a4, ea), ea = ea + 4;
			if ((list & 0x2000) != 0)
				write32(a5, ea), ea = ea + 4;
			if ((list & 0x4000) != 0)
				write32(a6, ea), ea = ea + 4;
			if ((list & 0x8000) != 0)
				write32(a7, ea);
		}
	}

	void movem16mr(int op) {
		const int list = fetch16();
		int ea = lea(op);
		if ((list & 1) != 0)
			d0 = read16s(ea), ea = ea + 2;
		if ((list & 2) != 0)
			d1 = read16s(ea), ea = ea + 2;
		if ((list & 4) != 0)
			d2 = read16s(ea), ea = ea + 2;
		if ((list & 8) != 0)
			d3 = read16s(ea), ea = ea + 2;
		if ((list & 0x10) != 0)
			d4 = read16s(ea), ea = ea + 2;
		if ((list & 0x20) != 0)
			d5 = read16s(ea), ea = ea + 2;
		if ((list & 0x40) != 0)
			d6 = read16s(ea), ea = ea + 2;
		if ((list & 0x80) != 0)
			d7 = read16s(ea), ea = ea + 2;
		if ((list & 0x100) != 0)
			a0 = read16s(ea), ea = ea + 2;
		if ((list & 0x200) != 0)
			a1 = read16s(ea), ea = ea + 2;
		if ((list & 0x400) != 0)
			a2 = read16s(ea), ea = ea + 2;
		if ((list & 0x800) != 0)
			a3 = read16s(ea), ea = ea + 2;
		if ((list & 0x1000) != 0)
			a4 = read16s(ea), ea = ea + 2;
		if ((list & 0x2000) != 0)
			a5 = read16s(ea), ea = ea + 2;
		if ((list & 0x4000) != 0)
			a6 = read16s(ea), ea = ea + 2;
		if ((list & 0x8000) != 0)
			a7 = read16s(ea), ea = ea + 2;
		if ((op & 070) == 030)
			rwop32(op & 7 | 010, thru, ea);
	}

	void movem32mr(int op) {
		const int list = fetch16();
		int ea = lea(op);
		if ((list & 1) != 0)
			d0 = read32(ea), ea = ea + 4;
		if ((list & 2) != 0)
			d1 = read32(ea), ea = ea + 4;
		if ((list & 4) != 0)
			d2 = read32(ea), ea = ea + 4;
		if ((list & 8) != 0)
			d3 = read32(ea), ea = ea + 4;
		if ((list & 0x10) != 0)
			d4 = read32(ea), ea = ea + 4;
		if ((list & 0x20) != 0)
			d5 = read32(ea), ea = ea + 4;
		if ((list & 0x40) != 0)
			d6 = read32(ea), ea = ea + 4;
		if ((list & 0x80) != 0)
			d7 = read32(ea), ea = ea + 4;
		if ((list & 0x100) != 0)
			a0 = read32(ea), ea = ea + 4;
		if ((list & 0x200) != 0)
			a1 = read32(ea), ea = ea + 4;
		if ((list & 0x400) != 0)
			a2 = read32(ea), ea = ea + 4;
		if ((list & 0x800) != 0)
			a3 = read32(ea), ea = ea + 4;
		if ((list & 0x1000) != 0)
			a4 = read32(ea), ea = ea + 4;
		if ((list & 0x2000) != 0)
			a5 = read32(ea), ea = ea + 4;
		if ((list & 0x4000) != 0)
			a6 = read32(ea), ea = ea + 4;
		if ((list & 0x8000) != 0)
			a7 = read32(ea), ea = ea + 4;
		if ((op & 070) == 030)
			rwop32(op & 7 | 010, thru, ea);
	}

	int exg(int op, int src) {
		int v = src;
		switch(op & 017) {
		case 000: // EXG Rx,D0
			return src = d0, d0 = v, src;
		case 001: // EXG Rx,D1
			return src = d1, d1 = v, src;
		case 002: // EXG Rx,D2
			return src = d2, d2 = v, src;
		case 003: // EXG Rx,D3
			return src = d3, d3 = v, src;
		case 004: // EXG Rx,D4
			return src = d4, d4 = v, src;
		case 005: // EXG Rx,D5
			return src = d5, d5 = v, src;
		case 006: // EXG Rx,D6
			return src = d6, d6 = v, src;
		case 007: // EXG Rx,D7
			return src = d7, d7 = v, src;
		case 010: // EXG Rx,A0
			return src = a0, a0 = v, src;
		case 011: // EXG Rx,A1
			return src = a1, a1 = v, src;
		case 012: // EXG Rx,A2
			return src = a2, a2 = v, src;
		case 013: // EXG Rx,A3
			return src = a3, a3 = v, src;
		case 014: // EXG Rx,A4
			return src = a4, a4 = v, src;
		case 015: // EXG Rx,A5
			return src = a5, a5 = v, src;
		case 016: // EXG Rx,A6
			return src = a6, a6 = v, src;
		case 017: // EXG Rx,A7
			return src = a7, a7 = v, src;
		}
		return -1;
	}

	void jsr(int op) {
		const int addr = lea(op);
		a7 = a7 - 4, write32(pc, a7), pc = addr;
	}

	void chk(int src, int dst) {
		dst = dst << 16 >> 16, src = src << 16 >> 16;
		if (dst >= 0 && dst <= src)
			return;
		sr = sr & ~8 | dst >> 12 & 8, exception(6);
	}

	int lea(int op) {
		switch(op & 077) {
		case 020: // (A0)
			return a0;
		case 021: // (A1)
			return a1;
		case 022: // (A2)
			return a2;
		case 023: // (A3)
			return a3;
		case 024: // (A4)
			return a4;
		case 025: // (A5)
			return a5;
		case 026: // (A6)
			return a6;
		case 027: // (A7)
			return a7;
		case 030: // (A0)+
			return a0;
		case 031: // (A1)+
			return a1;
		case 032: // (A2)+
			return a2;
		case 033: // (A3)+
			return a3;
		case 034: // (A4)+
			return a4;
		case 035: // (A5)+
			return a5;
		case 036: // (A6)+
			return a6;
		case 037: // (A7)+
			return a7;
		case 040: // -(A0)
			return a0;
		case 041: // -(A1)
			return a1;
		case 042: // -(A2)
			return a2;
		case 043: // -(A3)
			return a3;
		case 044: // -(A4)
			return a4;
		case 045: // -(A5)
			return a5;
		case 046: // -(A6)
			return a6;
		case 047: // -(A7)
			return a7;
		case 050: // d(A0)
			return disp(a0);
		case 051: // d(A1)
			return disp(a1);
		case 052: // d(A2)
			return disp(a2);
		case 053: // d(A3)
			return disp(a3);
		case 054: // d(A4)
			return disp(a4);
		case 055: // d(A5)
			return disp(a5);
		case 056: // d(A6)
			return disp(a6);
		case 057: // d(A7)
			return disp(a7);
		case 060: // d(A0,Xi)
			return index(a0);
		case 061: // d(A1,Xi)
			return index(a1);
		case 062: // d(A2,Xi)
			return index(a2);
		case 063: // d(A3,Xi)
			return index(a3);
		case 064: // d(A4,Xi)
			return index(a4);
		case 065: // d(A5,Xi)
			return index(a5);
		case 066: // d(A6,Xi)
			return index(a6);
		case 067: // d(A7,Xi)
			return index(a7);
		case 070: // Abs.W
			return fetch16s();
		case 071: // Abs.L
			return fetch32();
		case 072: // d(PC)
			return disp(pc);
		case 073: // d(PC,Xi)
			return index(pc);
		default:
			return -1;
		}
	}

	static int thru(int src, int dst, int& sr) {
		return src;
	}

	static int or8(int src, int dst, int& sr) {
		const int r = (dst | src) & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	static int or16(int src, int dst, int& sr) {
		const int r = (dst | src) & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	static int or32(int src, int dst, int& sr) {
		const int r = dst | src;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int and8(int src, int dst, int& sr) {
		const int r = dst & src & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	static int and16(int src, int dst, int& sr) {
		const int r = dst & src & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	static int and32(int src, int dst, int& sr) {
		const int r = dst & src;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int sub8(int src, int dst, int& sr) {
		const int r = dst - src & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int sub16(int src, int dst, int& sr) {
		const int r = dst - src & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int sub32(int src, int dst, int& sr) {
		const int r = dst - src, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	static int suba32(int src, int dst, int& sr) {
		return dst - src;
	}

	static int add8(int src, int dst, int& sr) {
		const int r = dst + src & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int add16(int src, int dst, int& sr) {
		const int r = dst + src & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int add32(int src, int dst, int& sr) {
		const int r = dst + src, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	static int adda32(int src, int dst, int& sr) {
		return dst + src;
	}

	void btst8(int src, int dst) {
		sr = sr & ~4 | ~dst >> (src & 7) << 2 & 4;
	}

	void btst32(int src, int dst) {
		sr = sr & ~4 | ~dst >> (src & 31) << 2 & 4;
	}

	static int bchg8(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, (dst ^ 1 << src) & 0xff;
	}

	static int bchg32(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst ^ 1 << src;
	}

	static int bclr8(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, dst & ~(1 << src) & 0xff;
	}

	static int bclr32(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst & ~(1 << src);
	}

	static int bset8(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 7) << 2 & 4, (dst | 1 << src) & 0xff;
	}

	static int bset32(int src, int dst, int& sr) {
		return sr = sr & ~4 | ~dst >> (src &= 31) << 2 & 4, dst | 1 << src;
	}

	static int eor8(int src, int dst, int& sr) {
		const int r = (dst ^ src) & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	static int eor16(int src, int dst, int& sr) {
		const int r = (dst ^ src) & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	static int eor32(int src, int dst, int& sr) {
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

	void cmp32(int src, int dst) {
		const int r = dst - src, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1;
	}

	int divu(int src, int dst) {
		if (!(src &= 0xffff))
			return exception(5), dst;
		const int r = (unsigned int)dst / src;
		if (r > 0xffff || r < 0)
			return sr |= 2, dst;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r | (unsigned int)dst % src << 16;
	}

	static int sbcd(int src, int dst, int& sr) {
		int r = dst - src - (sr >> 4 & 1) & 0xff, c = ~dst & src | src & r | r & ~dst;
		if ((c & 8) != 0 && (r & 0x0f) > 5 || (r & 0x0f) > 9)
			r -= 6;
		if ((c & 0x80) != 0 && (r & 0xf0) > 0x50 || (r & 0xf0) > 0x90)
			r -= 0x60, c |= 0x80;
		return r &= 0xff, sr = sr & ~0x15 | c >> 3 & 0x10 | sr & !r << 2 | c >> 7 & 1, r;
	}

	int divs(int src, int dst) {
		if (!(src = src << 16 >> 16))
			return exception(5), dst;
		const int r = dst / src;
		if (r > 0x7fff || r < -0x8000)
			return sr |= 2, dst;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r & 0xffff | dst % src << 16;
	}

	static int subx8(int src, int dst, int& sr) {
		const int r = dst - src - (sr >> 4 & 1) & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int subx16(int src, int dst, int& sr) {
		const int r = dst - src - (sr >> 4 & 1) & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int subx32(int src, int dst, int& sr) {
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

	static int mulu(int src, int dst, int& sr) {
		const int r = (src & 0xffff) * (dst & 0xffff);
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int abcd(int src, int dst, int& sr) {
		int r = dst + src + (sr >> 4 & 1) & 0xff, c = dst & src | src & ~r | ~r & dst;
		if ((c & 8) != 0 && (r & 0x0f) < 4 || (r & 0x0f) > 9)
			if ((r += 6) >= 0x100)
				c |= 0x80;
		if ((c & 0x80) != 0 && (r & 0xf0) < 0x40 || (r & 0xf0) > 0x90)
			r += 0x60, c |= 0x80;
		return r &= 0xff, sr = sr & ~0x15 | c >> 3 & 0x10 | sr & !r << 2 | c >> 7 & 1, r;
	}

	static int muls(int src, int dst, int& sr) {
		const int r = (dst << 16 >> 16) * (src << 16 >> 16);
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int addx8(int src, int dst, int& sr) {
		const int r = dst + src + (sr >> 4 & 1) & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int addx16(int src, int dst, int& sr) {
		const int r = dst + src + (sr >> 4 & 1) & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int addx32(int src, int dst, int& sr) {
		const int r = dst + src + (sr >> 4 & 1), v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | sr & !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	static int asr8(int src, int dst, int& sr) {
		src &= 63, dst = dst << 24 >> 24;
		const int r = dst >> min(src, 7) & 0xff, c = (src > 0) & dst >> min(src - 1, 7), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	static int asr16(int src, int dst, int& sr) {
		src &= 63, dst = dst << 16 >> 16;
		const int r = dst >> min(src, 15) & 0xffff, c = (src > 0) & dst >> min(src - 1, 15), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	static int asr32(int src, int dst, int& sr) {
		src &= 63;
		const int r = dst >> min(src, 31), c = (src > 0) & dst >> min(src - 1, 31), x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	static int lsr8(int src, int dst, int& sr) {
		src &= 63, dst &= 0xff;
		const int r = -(src < 8) & dst >> src, c = (src > 0 && src < 9) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	static int lsr16(int src, int dst, int& sr) {
		src &= 63, dst &= 0xffff;
		const int r = -(src < 16) & dst >> src, c = (src > 0 && src < 17) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	static int lsr32(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 32) & (unsigned int)dst >> src, c = (src > 0 && src < 33) & dst >> src - 1, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	static int roxr8(int src, int dst, int& sr) {
		src = (src & 63) % 9, dst &= 0xff;
		int x = sr >> 4 & 1, r = (dst | x << 8 | dst << 9) >> src & 0xff;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | x, r;
	}

	static int roxr16(int src, int dst, int& sr) {
		src = (src & 63) % 17, dst &= 0xffff;
		int x = sr >> 4 & 1, r = (dst | x << 16 | dst << 17) >> src & 0xffff;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | x, r;
	}

	static int roxr32(int src, int dst, int& sr) {
		src = (src & 63) % 33;
		int x = sr >> 4 & 1, r = -(src > 1) & dst << 33 - src | -(src > 0) & x << 32 - src | -(src < 32) & (unsigned int)dst >> src;
		x = src > 0 ? dst >> src - 1 & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | x, r;
	}

	static int ror8(int src, int dst, int& sr) {
		dst &= 0xff;
		const int r = (dst | dst << 8) >> (src & 7) & 0xff, c = (src > 0) & r >> 7;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2 | c, r;
	}

	static int ror16(int src, int dst, int& sr) {
		dst &= 0xffff;
		const int r = (dst | dst << 16) >> (src & 15) & 0xffff, c = (src > 0) & r >> 15;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2 | c, r;
	}

	static int ror32(int src, int dst, int& sr) {
		const int r = dst << (-src & 31) | (unsigned int)dst >> (src & 31), c = (src > 0) & r >> 31;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | c, r;
	}

	static int asl8(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 8) & dst << src & 0xff, c = (src > 0 && src < 9) & dst << src - 1 >> 7, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7f >> min(src - 1, 7) & 0xff, v = src > 0 ? dst >> 7 & ((~(dst << 1) & m) != 0) | ~dst >> 7 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	static int asl16(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 16) & dst << src & 0xffff, c = (src > 0 && src < 17) & dst << src - 1 >> 15, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7fff >> min(src - 1, 15) & 0xffff, v = src > 0 ? dst >> 15 & ((~(dst << 1) & m) != 0) | ~dst >> 15 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | v << 1 | c, r;
	}

	static int asl32(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 32) & dst << src, c = (src > 0 && src < 33) & dst << src - 1 >> 31, x = src > 0 ? c : sr >> 4 & 1;
		const int m = ~0x7fffffff >> min(src - 1, 31), v = src > 0 ? dst >> 31 & ((~(dst << 1) & m) != 0) | ~dst >> 31 & ((dst << 1 & m) != 0) : 0;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | v << 1 | c, r;
	}

	static int lsl8(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 8) & dst << src & 0xff, c = (src > 0 && src < 9) & dst << src - 1 >> 7, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | c, r;
	}

	static int lsl16(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 16) & dst << src & 0xffff, c = (src > 0 && src < 17) & dst << src - 1 >> 15, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | c, r;
	}

	static int lsl32(int src, int dst, int& sr) {
		src &= 63;
		const int r = -(src < 32) & dst << src, c = (src > 0 && src < 33) & dst << src - 1 >> 31, x = src > 0 ? c : sr >> 4 & 1;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | c, r;
	}

	static int roxl8(int src, int dst, int& sr) {
		src = (src & 63) % 9, dst &= 0xff;
		int x = sr >> 4 & 1, r = (dst >> 1 | x << 7 | dst << 8) >> 8 - src & 0xff;
		x = src > 0 ? dst >> 8 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 4 & 8 | !r << 2 | x, r;
	}

	static int roxl16(int src, int dst, int& sr) {
		src = (src & 63) % 17, dst &= 0xffff;
		int x = sr >> 4 & 1, r = (dst >> 1 | x << 15 | dst << 16) >> 16 - src & 0xffff;
		x = src > 0 ? dst >> 16 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 12 & 8 | !r << 2 | x, r;
	}

	static int roxl32(int src, int dst, int& sr) {
		src = (src & 63) % 33;
		int x = sr >> 4 & 1, r = -(src < 32) & dst << src | -(src > 0) & x << src - 1 | -(src > 1) & (unsigned int)dst >> 33 - src;
		x = src > 0 ? dst >> 32 - src & 1 : x;
		return sr = sr & ~0x1f | x << 4 | r >> 28 & 8 | !r << 2 | x, r;
	}

	static int rol8(int src, int dst, int& sr) {
		dst &= 0xff;
		const int r = (dst | dst << 8) >> (-src & 7) & 0xff, c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2 | c, r;
	}

	static int rol16(int src, int dst, int& sr) {
		dst &= 0xffff;
		const int r = (dst | dst << 16) >> (-src & 15) & 0xffff, c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2 | c, r;
	}

	static int rol32(int src, int dst, int& sr) {
		const int r = dst << (src & 31) | (unsigned int)dst >> (-src & 31), c = (src > 0) & r;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2 | c, r;
	}

	static int negx8(int src, int dst, int& sr) {
		const int r = -dst - (sr >> 4 & 1) & 0xff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | sr & !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int negx16(int src, int dst, int& sr) {
		const int r = -dst - (sr >> 4 & 1) & 0xffff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | sr & !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int negx32(int src, int dst, int& sr) {
		const int r = -dst - (sr >> 4 & 1), v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | sr & !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	static int clr(int src, int dst, int& sr) {
		return sr = sr & ~0x0f | 4, 0;
	}

	static int neg8(int src, int dst, int& sr) {
		const int r = -dst & 0xff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 3 & 0x10 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	static int neg16(int src, int dst, int& sr) {
		const int r = -dst & 0xffff, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 11 & 0x10 | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	static int neg32(int src, int dst, int& sr) {
		const int r = -dst, v = dst & r, c = dst | r;
		return sr = sr & ~0x1f | c >> 27 & 0x10 | r >> 28 & 8 | !r << 2 | v >> 30 & 2 | c >> 31 & 1, r;
	}

	static int not8(int src, int dst, int& sr) {
		const int r = ~dst & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r;
	}

	static int not16(int src, int dst, int& sr) {
		const int r = ~dst & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	static int not32(int src, int dst, int& sr) {
		const int r = ~dst;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int nbcd(int src, int dst, int& sr) {
		int r = -dst - (sr >> 4 & 1) & 0xff, c = dst | r;
		if ((c & 8) != 0 && (r & 0x0f) > 5 || (r & 0x0f) > 9)
			r -= 6;
		if ((c & 0x80) != 0 && (r & 0xf0) > 0x50 || (r & 0xf0) > 0x90)
			r -= 0x60, c |= 0x80;
		return r &= 0xff, sr = sr & ~0x15 | c >> 3 & 0x10 | sr & !r << 2 | c >> 7 & 1, r;
	}

	static int swap(int src, int dst, int& sr) {
		const int r = dst << 16 | (unsigned int)dst >> 16;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int ext16(int src, int dst, int& sr) {
		const int r = dst << 24 >> 24 & 0xffff;
		return sr = sr & ~0x0f | r >> 12 & 8 | !r << 2, r;
	}

	static int ext32(int src, int dst, int& sr) {
		const int r = dst << 16 >> 16;
		return sr = sr & ~0x0f | r >> 28 & 8 | !r << 2, r;
	}

	static int tas(int src, int dst, int& sr) {
		const int r = dst & 0xff;
		return sr = sr & ~0x0f | r >> 4 & 8 | !r << 2, r | 0x80;
	}

	void link(int op) {
		switch(op & 7) {
		case 0:
			return a7 = a7 - 4, write32(a0, a7), a0 = a7, void(a7 = disp(a7));
		case 1:
			return a7 = a7 - 4, write32(a1, a7), a1 = a7, void(a7 = disp(a7));
		case 2:
			return a7 = a7 - 4, write32(a2, a7), a2 = a7, void(a7 = disp(a7));
		case 3:
			return a7 = a7 - 4, write32(a3, a7), a3 = a7, void(a7 = disp(a7));
		case 4:
			return a7 = a7 - 4, write32(a4, a7), a4 = a7, void(a7 = disp(a7));
		case 5:
			return a7 = a7 - 4, write32(a5, a7), a5 = a7, void(a7 = disp(a7));
		case 6:
			return a7 = a7 - 4, write32(a6, a7), a6 = a7, void(a7 = disp(a7));
		case 7:
			return a7 = a7 - 4, write32(a7, a7), void(a7 = disp(a7));
		}
	}

	void unlk(int op) {
		switch(op & 7) {
		case 0:
			return a7 = a0, a0 = read32(a7), void(a7 = a7 + 4);
		case 1:
			return a7 = a1, a1 = read32(a7), void(a7 = a7 + 4);
		case 2:
			return a7 = a2, a2 = read32(a7), void(a7 = a7 + 4);
		case 3:
			return a7 = a3, a3 = read32(a7), void(a7 = a7 + 4);
		case 4:
			return a7 = a4, a4 = read32(a7), void(a7 = a7 + 4);
		case 5:
			return a7 = a5, a5 = read32(a7), void(a7 = a7 + 4);
		case 6:
			return a7 = a6, a6 = read32(a7), void(a7 = a7 + 4);
		case 7:
			return a7 = read32(a7), void(a7 = a7 + 4);
		}
	}

	void dbcc(int op, bool cond) {
		const int addr = disp(pc);
		switch(op & 7) {
		case 0:
			return void(!cond && ((d0 = d0 & ~0xffff | d0 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 1:
			return void(!cond && ((d1 = d1 & ~0xffff | d1 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 2:
			return void(!cond && ((d2 = d2 & ~0xffff | d2 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 3:
			return void(!cond && ((d3 = d3 & ~0xffff | d3 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 4:
			return void(!cond && ((d4 = d4 & ~0xffff | d4 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 5:
			return void(!cond && ((d5 = d5 & ~0xffff | d5 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 6:
			return void(!cond && ((d6 = d6 & ~0xffff | d6 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		case 7:
			return void(!cond && ((d7 = d7 & ~0xffff | d7 - 1 & 0xffff) & 0xffff) != 0xffff && (pc = addr));
		}
	}

	int disp(int base) {
		return base + fetch16s();
	}

	int index(int base) {
		const int word = fetch16(), disp = word << 24 >> 24;
		switch (word >> 11) {
		case 0x00: // D0.W
			return base + (d0 << 16 >> 16) + disp;
		case 0x01: // D0.L
			return base + d0 + disp;
		case 0x02: // D1.W
			return base + (d1 << 16 >> 16) + disp;
		case 0x03: // D1.L
			return base + d1 + disp;
		case 0x04: // D2.W
			return base + (d2 << 16 >> 16) + disp;
		case 0x05: // D2.L
			return base + d2 + disp;
		case 0x06: // D3.W
			return base + (d3 << 16 >> 16) + disp;
		case 0x07: // D3.L
			return base + d3 + disp;
		case 0x08: // D4.W
			return base + (d4 << 16 >> 16) + disp;
		case 0x09: // D4.L
			return base + d4 + disp;
		case 0x0a: // D5.W
			return base + (d5 << 16 >> 16) + disp;
		case 0x0b: // D5.L
			return base + d5 + disp;
		case 0x0c: // D6.W
			return base + (d6 << 16 >> 16) + disp;
		case 0x0d: // D6.L
			return base + d6 + disp;
		case 0x0e: // D7.W
			return base + (d7 << 16 >> 16) + disp;
		case 0x0f: // D7.L
			return base + d7 + disp;
		case 0x10: // A0.W
			return base + (a0 << 16 >> 16) + disp;
		case 0x11: // A0.L
			return base + a0 + disp;
		case 0x12: // A1.W
			return base + (a1 << 16 >> 16) + disp;
		case 0x13: // A1.L
			return base + a1 + disp;
		case 0x14: // A2.W
			return base + (a2 << 16 >> 16) + disp;
		case 0x15: // A2.L
			return base + a2 + disp;
		case 0x16: // A3.W
			return base + (a3 << 16 >> 16) + disp;
		case 0x17: // A3.L
			return base + a3 + disp;
		case 0x18: // A4.W
			return base + (a4 << 16 >> 16) + disp;
		case 0x19: // A4.L
			return base + a4 + disp;
		case 0x1a: // A5.W
			return base + (a5 << 16 >> 16) + disp;
		case 0x1b: // A5.L
			return base + a5 + disp;
		case 0x1c: // A6.W
			return base + (a6 << 16 >> 16) + disp;
		case 0x1d: // A6.L
			return base + a6 + disp;
		case 0x1e: // A7.W
			return base + (a7 << 16 >> 16) + disp;
		case 0x1f: // A7.L
			return base + a7 + disp;
		}
		return -1;
	}

	int fetch16() {
		Page16& page = memorymap[pc >> 8 & 0xffff];
		const int data = page.base[pc & 0xff] << 8 | page.base[pc + 1 & 0xff];
		return pc = pc + 2, data;
	}

	int fetch16s() {
		Page16& page = memorymap[pc >> 8 & 0xffff];
		const int data = page.base[pc & 0xff] << 8 | page.base[pc + 1 & 0xff];
		return pc = pc + 2, data << 16 >> 16;
	}

	int fetch32() {
		const int data = fetch16() << 16;
		return data | fetch16();
	}

	int read8(int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		return page.read ? page.read(addr) : page.base[addr & 0xff];
	}

	int read16(int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		return page.read16 ? page.read16(addr) : page.read ? page.read(addr) << 8 | page.read(addr + 1) : page.base[addr & 0xff] << 8 | page.base[addr + 1 & 0xff];
	}

	int read16s(int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		const int data = page.read16 ? page.read16(addr) : page.read ? page.read(addr) << 8 | page.read(addr + 1) : page.base[addr & 0xff] << 8 | page.base[addr + 1 & 0xff];
		return data << 16 >> 16;
	}

	int read32(int addr) {
		const int data = read16(addr) << 16;
		return data | read16(addr + 2);
	}

	void write8(int data, int addr) {
		Page16& page = memorymap[addr >> 8 & 0xffff];
		if (page.write)
			page.write(addr, data & 0xff);
		else
			page.base[addr & 0xff] = data;
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
