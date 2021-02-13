/*
 *
 *	MC6809 Emulator
 *
 */

#ifndef MC6809_H
#define MC6809_H

#include "cpu.h"

struct MC6809 : Cpu {
	static const unsigned char cc[0x100], cc_i[0x100];
	int a = 0;
	int b = 0;
	int dp = 0;
	int ccr = 0; // ccr:efhinzvc
	int x = 0;
	int y = 0;
	int u = 0;
	int s = 0;

	MC6809(int clock = 0) : Cpu(clock) {}

	void reset() override {
		Cpu::reset();
		ccr = 0x50;
		dp = 0;
		pc = read16(0xfffe);
	}

	bool fast_interrupt() {
		if (!Cpu::interrupt() || ccr & 0x40)
			return false;
		return cycle -= 10, pshs16(pc), pshs(ccr &= ~0x80), ccr |= 0x50, pc = read16(0xfff6), true;
	}

	bool interrupt() override {
		if (!Cpu::interrupt() || ccr & 0x10)
			return false;
		return cycle -= cc[0x3f], pshs16(pc), pshs16(u), pshs16(y), pshs16(x), pshs(dp), pshs(b), pshs(a), pshs(ccr |= 0x80), ccr |= 0x10, pc = read16(0xfff8), true;
	}

	bool non_maskable_interrupt() {
		if (!Cpu::interrupt())
			return false;
		return cycle -= cc[0x3f], pshs16(pc), pshs16(u), pshs16(y), pshs16(x), pshs(dp), pshs(b), pshs(a), pshs(ccr |= 0x80), ccr |= 0x50, pc = read16(0xfffc), true;
	}

	void _execute() override {
		int v, ea, op = fetch();
		cycle -= cc[op];
		switch (op) {
		case 0x00: // NEG <n
			return ea = dp << 8 | fetch(), write8(neg8(read(ea)), ea);
		case 0x03: // COM <n
			return ea = dp << 8 | fetch(), write8(com8(read(ea)), ea);
		case 0x04: // LSR <n
			return ea = dp << 8 | fetch(), write8(lsr8(read(ea)), ea);
		case 0x06: // ROR <n
			return ea = dp << 8 | fetch(), write8(ror8(read(ea)), ea);
		case 0x07: // ASR <n
			return ea = dp << 8 | fetch(), write8(asr8(read(ea)), ea);
		case 0x08: // LSL <n
			return ea = dp << 8 | fetch(), write8(lsl8(read(ea)), ea);
		case 0x09: // ROL <n
			return ea = dp << 8 | fetch(), write8(rol8(read(ea)), ea);
		case 0x0a: // DEC <n
			return ea = dp << 8 | fetch(), write8(dec8(read(ea)), ea);
		case 0x0c: // INC <n
			return ea = dp << 8 | fetch(), write8(inc8(read(ea)), ea);
		case 0x0d: // TST <n
			return void(mov8(read(dp << 8 | fetch())));
		case 0x0e: // JMP <n
			return void(pc = dp << 8 | fetch());
		case 0x0f: // CLR <n
			return write8(clr8(), dp << 8 | fetch());
		case 0x10:
			cycle -= cc[op = fetch()];
			switch (op) {
			case 0x21: // LBRN
				return lbcc(false);
			case 0x22: // LBHI
				return lbcc(!((ccr >> 2 | ccr) & 1));
			case 0x23: // LBLS
				return lbcc(((ccr >> 2 | ccr) & 1) != 0);
			case 0x24: // LBHS(LBCC)
				return lbcc(!(ccr & 1));
			case 0x25: // LBLO(LBCS)
				return lbcc((ccr & 1) != 0);
			case 0x26: // LBNE
				return lbcc(!(ccr & 4));
			case 0x27: // LBEQ
				return lbcc((ccr & 4) != 0);
			case 0x28: // LBVC
				return lbcc(!(ccr & 2));
			case 0x29: // LBVS
				return lbcc((ccr & 2) != 0);
			case 0x2a: // LBPL
				return lbcc(!(ccr & 8));
			case 0x2b: // LBMI
				return lbcc((ccr & 8) != 0);
			case 0x2c: // LBGE
				return lbcc(!((ccr >> 2 ^ ccr) & 2));
			case 0x2d: // LBLT
				return lbcc(((ccr >> 2 ^ ccr) & 2) != 0);
			case 0x2e: // LBGT
				return lbcc(!((ccr >> 2 ^ ccr | ccr >> 1) & 2));
			case 0x2f: // LBLE
				return lbcc(((ccr >> 2 ^ ccr | ccr >> 1) & 2) != 0);
			case 0x3f: // SWI2
				return pshs16(pc), pshs16(u), pshs16(y), pshs16(x), pshs(dp), pshs(b), pshs(a), pshs(ccr |= 0x80), void(pc = read16(0xfff4));
			case 0x83: // CMPD #nn
				return void(sub16(fetch16(), a << 8 | b));
			case 0x8c: // CMPY #nn
				return void(sub16(fetch16(), y));
			case 0x8e: // LDY #nn
				return void(y = mov16(fetch16()));
			case 0x93: // CMPD <n
				return void(sub16(read16(dp << 8 | fetch()), a << 8 | b));
			case 0x9c: // CMPY <n
				return void(sub16(read16(dp << 8 | fetch()), y));
			case 0x9e: // LDY <n
				return void(y = mov16(read16(dp << 8 | fetch())));
			case 0x9f: // STY <n
				return write16(mov16(y), dp << 8 | fetch());
			case 0xa3: // CMPD ,r
				return void(sub16(read16(index()), a << 8 | b));
			case 0xac: // CMPY ,r
				return void(sub16(read16(index()), y));
			case 0xae: // LDY ,r
				return void(y = mov16(read16(index())));
			case 0xaf: // STY ,r
				return write16(mov16(y), index());
			case 0xb3: // CMPD >nn
				return void(sub16(read16(fetch16()), a << 8 | b));
			case 0xbc: // CMPY >nn
				return void(sub16(read16(fetch16()), y));
			case 0xbe: // LDY >nn
				return void(y = mov16(read16(fetch16())));
			case 0xbf: // STY >nn
				return write16(mov16(y), fetch16());
			case 0xce: // LDS #nn
				return void(s = mov16(fetch16()));
			case 0xde: // LDS <n
				return void(s = mov16(read16(dp << 8 | fetch())));
			case 0xdf: // STS <n
				return write16(mov16(s), dp << 8 | fetch());
			case 0xee: // LDS ,r
				return void(s = mov16(read16(index())));
			case 0xef: // STS ,r
				return write16(mov16(s), index());
			case 0xfe: // LDS >nn
				return void(s = mov16(read16(fetch16())));
			case 0xff: // STS >nn
				return write16(mov16(s), fetch16());
			default:
				undefsize = 2;
				if (undef)
					undef(pc);
				return;
			}
		case 0x11:
			cycle -= cc[op = fetch()];
			switch (op) {
			case 0x3f: // SWI3
				return pshs16(pc), pshs16(u), pshs16(y), pshs16(x), pshs(dp), pshs(b), pshs(a), pshs(ccr |= 0x80), void(pc = read16(0xfff2));
			case 0x83: // CMPU #nn
				return void(sub16(fetch16(), u));
			case 0x8c: // CMPS #nn
				return void(sub16(fetch16(), s));
			case 0x93: // CMPU <n
				return void(sub16(read16(dp << 8 | fetch()), u));
			case 0x9c: // CMPS <n
				return void(sub16(read16(dp << 8 | fetch()), s));
			case 0xa3: // CMPU ,r
				return void(sub16(read16(index()), u));
			case 0xac: // CMPS ,r
				return void(sub16(read16(index()), s));
			case 0xb3: // CMPU >nn
				return void(sub16(read16(fetch16()), u));
			case 0xbc: // CMPS >nn
				return void(sub16(read16(fetch16()), s));
			default:
				undefsize = 2;
				if (undef)
					undef(pc);
				return;
			}
		case 0x12: // NOP
			return;
		case 0x13: // SYNC
			return suspend();
		case 0x16: // LBRA
			return lbcc(true);
		case 0x17: // LBSR
			return lbsr();
		case 0x19: // DAA
			return daa();
		case 0x1a: // ORCC
			return void(ccr |= fetch());
		case 0x1c: // ANDCC
			return void(ccr &= fetch());
		case 0x1d: // SEX
			return void(b & 0x80 ? (a = 0xff, ccr = ccr & ~4 | 8) : (a = 0, ccr = ccr & ~0xc | !b << 2));
		case 0x1e: // EXG
			switch (fetch()) {
			case 0x00: // EXG D,D
			case 0x11: // EXG X,X
			case 0x22: // EXG Y,Y
			case 0x33: // EXG U,U
			case 0x44: // EXG S,S
			case 0x55: // EXG PC,PC
			case 0x88: // EXG A,A
			case 0x99: // EXG B,B
			case 0xaa: // EXG CCR,CCR
			case 0xbb: // EXG DP,DP
				return;
			case 0x01: // EXG D,X
			case 0x10: // EXG X,D
				return v = a << 8 | b, exg(v, x), split(a, b, v);
			case 0x02: // EXG D,Y
			case 0x20: // EXG Y,D
				return v = a << 8 | b, exg(v, y), split(a, b, v);
			case 0x03: // EXG D,U
			case 0x30: // EXG U,D
				return v = a << 8 | b, exg(v, u), split(a, b, v);
			case 0x04: // EXG D,S
			case 0x40: // EXG S,D
				return v = a << 8 | b, exg(v, s), split(a, b, v);
			case 0x05: // EXG D,PC
			case 0x50: // EXG PC,D
				return v = a << 8 | b, exg(v, pc), split(a, b, v);
			case 0x12: // EXG X,Y
			case 0x21: // EXG Y,X
				return exg(x, y);
			case 0x13: // EXG X,U
			case 0x31: // EXG U,X
				return exg(x, u);
			case 0x14: // EXG X,S
			case 0x41: // EXG S,X
				return exg(x, s);
			case 0x15: // EXG X,PC
			case 0x51: // EXG PC,X
				return exg(x, pc);
			case 0x23: // EXG Y,U
			case 0x32: // EXG U,Y
				return exg(y, u);
			case 0x24: // EXG Y,S
			case 0x42: // EXG S,Y
				return exg(y, s);
			case 0x25: // EXG Y,PC
			case 0x52: // EXG PC,Y
				return exg(y, pc);
			case 0x34: // EXG U,S
			case 0x43: // EXG S,U
				return exg(u, s);
			case 0x35: // EXG U,PC
			case 0x53: // EXG PC,U
				return exg(u, pc);
			case 0x45: // EXG S,PC
			case 0x54: // EXG PC,S
				return exg(s, pc);
			case 0x89: // EXG A,B
			case 0x98: // EXG B,A
				return exg(a, b);
			case 0x8a: // EXG A,CCR
			case 0xa8: // EXG CCR,A
				return exg(a, ccr);
			case 0x8b: // EXG A,DP
			case 0xb8: // EXG DP,A
				return exg(a, dp);
			case 0x9a: // EXG B,CCR
			case 0xa9: // EXG CCR,B
				return exg(b, ccr);
			case 0x9b: // EXG B,DP
			case 0xb9: // EXG DP,B
				return exg(b, dp);
			case 0xab: // EXG CCR,DP
			case 0xba: // EXG DP,CCR
				return exg(ccr, dp);
			default:
				undefsize = 2;
				if (undef)
					undef(pc);
				return;
			}
		case 0x1f: // TFR
			switch (fetch()) {
			case 0x00: // TFR D,D
			case 0x11: // TFR X,X
			case 0x22: // TFR Y,Y
			case 0x33: // TFR U,U
			case 0x44: // TFR S,S
			case 0x55: // TFR PC,PC
			case 0x88: // TFR A,A
			case 0x99: // TFR B,B
			case 0xaa: // TFR CCR,CCR
			case 0xbb: // TFR DP,DP
				return;
			case 0x01: // TFR D,X
				return void(x = a << 8 | b);
			case 0x02: // TFR D,Y
				return void(y = a << 8 | b);
			case 0x03: // TFR D,U
				return void(u = a << 8 | b);
			case 0x04: // TFR D,S
				return void(s = a << 8 | b);
			case 0x05: // TFR D,PC
				return void(pc = a << 8 | b);
			case 0x10: // TFR X,D
				return split(a, b, x);
			case 0x12: // TFR X,Y
				return void(y = x);
			case 0x13: // TFR X,U
				return void(u = x);
			case 0x14: // TFR X,S
				return void(s = x);
			case 0x15: // TFR X,PC
				return void(pc = x);
			case 0x20: // TFR Y,D
				return split(a, b, y);
			case 0x21: // TFR Y,X
				return void(x = y);
			case 0x23: // TFR Y,U
				return void(u = y);
			case 0x24: // TFR Y,S
				return void(s = y);
			case 0x25: // TFR Y,PC
				return void(pc = y);
			case 0x30: // TFR U,D
				return split(a, b, u);
			case 0x31: // TFR U,X
				return void(x = u);
			case 0x32: // TFR U,Y
				return void(y = u);
			case 0x34: // TFR U,S
				return void(s = u);
			case 0x35: // TFR U,PC
				return void(pc = u);
			case 0x40: // TFR S,D
				return split(a, b, s);
			case 0x41: // TFR S,X
				return void(x = s);
			case 0x42: // TFR S,Y
				return void(y = s);
			case 0x43: // TFR S,U
				return void(u = s);
			case 0x45: // TFR S,PC
				return void(pc = s);
			case 0x50: // TFR PC,D
				return split(a, b, pc);
			case 0x51: // TFR PC,X
				return void(x = pc);
			case 0x52: // TFR PC,Y
				return void(y = pc);
			case 0x53: // TFR PC,U
				return void(u = pc);
			case 0x54: // TFR PC,S
				return void(s = pc);
			case 0x89: // TFR A,B
				return void(b = a);
			case 0x8a: // TFR A,CCR
				return void(ccr = a);
			case 0x8b: // TFR A,DP
				return void(dp = a);
			case 0x98: // TFR B,A
				return void(a = b);
			case 0x9a: // TFR B,CCR
				return void(ccr = b);
			case 0x9b: // TFR B,DP
				return void(dp = b);
			case 0xa8: // TFR CCR,A
				return void(a = ccr);
			case 0xa9: // TFR CCR,B
				return void(b = ccr);
			case 0xab: // TFR CCR,DP
				return void(dp = ccr);
			case 0xb8: // TFR DP,A
				return void(a = dp);
			case 0xb9: // TFR DP,B
				return void(b = dp);
			case 0xba: // TFR DP,CCR
				return void(ccr = dp);
			default:
				undefsize = 2;
				if (undef)
					undef(pc);
				return;
			}
		case 0x20: // BRA
			return bcc(true);
		case 0x21: // BRN
			return bcc(false);
		case 0x22: // BHI
			return bcc(!((ccr >> 2 | ccr) & 1));
		case 0x23: // BLS
			return bcc(((ccr >> 2 | ccr) & 1) != 0);
		case 0x24: // BHS(BCC)
			return bcc(!(ccr & 1));
		case 0x25: // BLO(BCS)
			return bcc((ccr & 1) != 0);
		case 0x26: // BNE
			return bcc(!(ccr & 4));
		case 0x27: // BEQ
			return bcc((ccr & 4) != 0);
		case 0x28: // BVC
			return bcc(!(ccr & 2));
		case 0x29: // BVS
			return bcc((ccr & 2) != 0);
		case 0x2a: // BPL
			return bcc(!(ccr & 8));
		case 0x2b: // BMI
			return bcc((ccr & 8) != 0);
		case 0x2c: // BGE
			return bcc(!((ccr >> 2 ^ ccr) & 2));
		case 0x2d: // BLT
			return bcc(((ccr >> 2 ^ ccr) & 2) != 0);
		case 0x2e: // BGT
			return bcc(!((ccr >> 2 ^ ccr | ccr >> 1) & 2));
		case 0x2f: // BLE
			return bcc(((ccr >> 2 ^ ccr | ccr >> 1) & 2) != 0);
		case 0x30: // LEAX
			return void(ccr = ccr & ~4 | !(x = index()) << 2);
		case 0x31: // LEAY
			return void(ccr = ccr & ~4 | !(y = index()) << 2);
		case 0x32: // LEAS
			return void(s = index());
		case 0x33: // LEAU
			return void(u = index());
		case 0x34: // PSHS
			(v = fetch()) & 0x80 ? (cycle -= 2, pshs16(pc)) : void(0), v & 0x40 ? (cycle -= 2, pshs16(u)) : void(0);
			v & 0x20 ? (cycle -= 2, pshs16(y)) : void(0), v & 0x10 ? (cycle -= 2, pshs16(x)) : void(0), v & 8 ? (cycle -= 1, pshs(dp)) : void(0);
			return v & 4 ? (cycle -= 1, pshs(b)) : void(0), v & 2 ? (cycle -= 1, pshs(a)) : void(0), v & 1 ? (cycle -= 1, pshs(ccr)) : void(0);
		case 0x35: // PULS
			(v = fetch()) & 1 && (cycle -= 1, ccr = puls()), v & 2 && (cycle -= 1, a = puls()), v & 4 && (cycle -= 1, b = puls());
			v & 8 && (cycle -= 1, dp = puls()), v & 0x10 && (cycle -= 2, x = puls16()), v & 0x20 && (cycle -= 2, y = puls16());
			return v & 0x40 && (cycle -= 2, u = puls16()), void(v & 0x80 && (cycle -= 2, pc = puls16()));
		case 0x36: // PSHU
			(v = fetch()) & 0x80 ? (cycle -= 2, pshu16(pc)) : void(0), v & 0x40 ? (cycle -= 2, pshu16(s)) : void(0);
			v & 0x20 ? (cycle -= 2, pshu16(y)) : void(0), v & 0x10 ? (cycle -= 2, pshu16(x)) : void(0), v & 8 ? (cycle -= 1, pshu(dp)) : void(0);
			return v & 4 ? (cycle -= 1, pshu(b)) : void(0), v & 2 ? (cycle -= 1, pshu(a)) : void(0), v & 1 ? (cycle -= 1, pshu(ccr)) : void(0);
		case 0x37: // PULU
			(v = fetch()) & 1 && (cycle -= 1, ccr = pulu()), v & 2 && (cycle -= 1, a = pulu()), v & 4 && (cycle -= 1, b = pulu());
			v & 8 && (cycle -= 1, dp = pulu()), v & 0x10 && (cycle -= 2, x = pulu16()), v & 0x20 && (cycle -= 2, y = pulu16());
			return v & 0x40 && (cycle -= 2, s = pulu16()), void(v & 0x80 && (cycle -= 2, pc = pulu16()));
		case 0x39: // RTS
			return void(pc = puls16());
		case 0x3a: // ABX
			return void(x = x + b & 0xffff);
		case 0x3b: // RTI
			return (ccr = puls()) & 0x80 && (cycle -= 9, a = puls(), b = puls(), dp = puls(), x = puls16(), y = puls16(), u = puls16()), void(pc = puls16());
		case 0x3c: // CWAI
			return ccr &= fetch(), suspend();
		case 0x3d: // MUL
			return split(a, b, a * b), void(ccr = ccr & ~5 | !(a | b) << 2 | b >> 7);
		case 0x3f: // SWI
			return pshs16(pc), pshs16(u), pshs16(y), pshs16(x), pshs(dp), pshs(b), pshs(a), pshs(ccr |= 0x80), ccr |= 0x50, void(pc = read16(0xfffa));
		case 0x40: // NEGA
			return void(a = neg8(a));
		case 0x43: // COMA
			return void(a = com8(a));
		case 0x44: // LSRA
			return void(a = lsr8(a));
		case 0x46: // RORA
			return void(a = ror8(a));
		case 0x47: // ASRA
			return void(a = asr8(a));
		case 0x48: // LSLA
			return void(a = lsl8(a));
		case 0x49: // ROLA
			return void(a = rol8(a));
		case 0x4a: // DECA
			return void(a = dec8(a));
		case 0x4c: // INCA
			return void(a = inc8(a));
		case 0x4d: // TSTA
			return void(mov8(a));
		case 0x4f: // CLRA
			return void(a = clr8());
		case 0x50: // NEGB
			return void(b = neg8(b));
		case 0x53: // COMB
			return void(b = com8(b));
		case 0x54: // LSRB
			return void(b = lsr8(b));
		case 0x56: // RORB
			return void(b = ror8(b));
		case 0x57: // ASRB
			return void(b = asr8(b));
		case 0x58: // LSLB
			return void(b = lsl8(b));
		case 0x59: // ROLB
			return void(b = rol8(b));
		case 0x5a: // DECB
			return void(b = dec8(b));
		case 0x5c: // INCB
			return void(b = inc8(b));
		case 0x5d: // TSTB
			return void(mov8(b));
		case 0x5f: // CLRB
			return void(b = clr8());
		case 0x60: // NEG ,r
			return ea = index(), write8(neg8(read(ea)), ea);
		case 0x63: // COM ,r
			return ea = index(), write8(com8(read(ea)), ea);
		case 0x64: // LSR ,r
			return ea = index(), write8(lsr8(read(ea)), ea);
		case 0x66: // ROR ,r
			return ea = index(), write8(ror8(read(ea)), ea);
		case 0x67: // ASR ,r
			return ea = index(), write8(asr8(read(ea)), ea);
		case 0x68: // LSL ,r
			return ea = index(), write8(lsl8(read(ea)), ea);
		case 0x69: // ROL ,r
			return ea = index(), write8(rol8(read(ea)), ea);
		case 0x6a: // DEC ,r
			return ea = index(), write8(dec8(read(ea)), ea);
		case 0x6c: // INC ,r
			return ea = index(), write8(inc8(read(ea)), ea);
		case 0x6d: // TST ,r
			return void(mov8(read(index())));
		case 0x6e: // JMP ,r
			return void(pc = index());
		case 0x6f: // CLR ,r
			return write8(clr8(), index());
		case 0x70: // NEG >nn
			return ea = fetch16(), write8(neg8(read(ea)), ea);
		case 0x73: // COM >nn
			return ea = fetch16(), write8(com8(read(ea)), ea);
		case 0x74: // LSR >nn
			return ea = fetch16(), write8(lsr8(read(ea)), ea);
		case 0x76: // ROR >nn
			return ea = fetch16(), write8(ror8(read(ea)), ea);
		case 0x77: // ASR >nn
			return ea = fetch16(), write8(asr8(read(ea)), ea);
		case 0x78: // LSL >nn
			return ea = fetch16(), write8(lsl8(read(ea)), ea);
		case 0x79: // ROL >nn
			return ea = fetch16(), write8(rol8(read(ea)), ea);
		case 0x7a: // DEC >nn
			return ea = fetch16(), write8(dec8(read(ea)), ea);
		case 0x7c: // INC >nn
			return ea = fetch16(), write8(inc8(read(ea)), ea);
		case 0x7d: // TST >nn
			return void(mov8(read(fetch16())));
		case 0x7e: // JMP >nn
			return void(pc = fetch16());
		case 0x7f: // CLR >nn
			return write8(clr8(), fetch16());
		case 0x80: // SUBA #n
			return void(a = sub8(fetch(), a));
		case 0x81: // CMPA #n
			return void(sub8(fetch(), a));
		case 0x82: // SBCA #n
			return void(a = sbc8(fetch(), a));
		case 0x83: // SUBD #nn
			return split(a, b, sub16(fetch16(), a << 8 | b));
		case 0x84: // ANDA #n
			return void(a = mov8(a & fetch()));
		case 0x85: // BITA #n
			return void(mov8(a & fetch()));
		case 0x86: // LDA #n
			return void(a = mov8(fetch()));
		case 0x88: // EORA #n
			return void(a = mov8(a ^ fetch()));
		case 0x89: // ADCA #n
			return void(a = adc8(fetch(), a));
		case 0x8a: // ORA #n
			return void(a = mov8(a | fetch()));
		case 0x8b: // ADDA #n
			return void(a = add8(fetch(), a));
		case 0x8c: // CMPX #nn
			return void(sub16(fetch16(), x));
		case 0x8d: // BSR
			return bsr();
		case 0x8e: // LDX #nn
			return void(x = mov16(fetch16()));
		case 0x90: // SUBA <n
			return void(a = sub8(read(dp << 8 | fetch()), a));
		case 0x91: // CMPA <n
			return void(sub8(read(dp << 8 | fetch()), a));
		case 0x92: // SBCA <n
			return void(a = sbc8(read(dp << 8 | fetch()), a));
		case 0x93: // SUBD <n
			return split(a, b, sub16(read16(dp << 8 | fetch()), a << 8 | b));
		case 0x94: // ANDA <n
			return void(a = mov8(a & read(dp << 8 | fetch())));
		case 0x95: // BITA <n
			return void(mov8(a & read(dp << 8 | fetch())));
		case 0x96: // LDA <n
			return void(a = mov8(read(dp << 8 | fetch())));
		case 0x97: // STA <n
			return write8(mov8(a), dp << 8 | fetch());
		case 0x98: // EORA <n
			return void(a = mov8(a ^ read(dp << 8 | fetch())));
		case 0x99: // ADCA <n
			return void(a = adc8(read(dp << 8 | fetch()), a));
		case 0x9a: // ORA <n
			return void(a = mov8(a | read(dp << 8 | fetch())));
		case 0x9b: // ADDA <n
			return void(a = add8(read(dp << 8 | fetch()), a));
		case 0x9c: // CMPX <n
			return void(sub16(read16(dp << 8 | fetch()), x));
		case 0x9d: // JSR <n
			return jsr(dp << 8 | fetch());
		case 0x9e: // LDX <n
			return void(x = mov16(read16(dp << 8 | fetch())));
		case 0x9f: // STX <n
			return write16(mov16(x), dp << 8 | fetch());
		case 0xa0: // SUBA ,r
			return void(a = sub8(read(index()), a));
		case 0xa1: // CMPA ,r
			return void(sub8(read(index()), a));
		case 0xa2: // SBCA ,r
			return void(a = sbc8(read(index()), a));
		case 0xa3: // SUBD ,r
			return split(a, b, sub16(read16(index()), a << 8 | b));
		case 0xa4: // ANDA ,r
			return void(a = mov8(a & read(index())));
		case 0xa5: // BITA ,r
			return void(mov8(a & read(index())));
		case 0xa6: // LDA ,r
			return void(a = mov8(read(index())));
		case 0xa7: // STA ,r
			return write8(mov8(a), index());
		case 0xa8: // EORA ,r
			return void(a = mov8(a ^ read(index())));
		case 0xa9: // ADCA ,r
			return void(a = adc8(read(index()), a));
		case 0xaa: // ORA ,r
			return void(a = mov8(a | read(index())));
		case 0xab: // ADDA ,r
			return void(a = add8(read(index()), a));
		case 0xac: // CMPX ,r
			return void(sub16(read16(index()), x));
		case 0xad: // JSR ,r
			return jsr(index());
		case 0xae: // LDX ,r
			return void(x = mov16(read16(index())));
		case 0xaf: // STX ,r
			return write16(mov16(x), index());
		case 0xb0: // SUBA >nn
			return void(a = sub8(read(fetch16()), a));
		case 0xb1: // CMPA >nn
			return void(sub8(read(fetch16()), a));
		case 0xb2: // SBCA >nn
			return void(a = sbc8(read(fetch16()), a));
		case 0xb3: // SUBD >nn
			return split(a, b, sub16(read16(fetch16()), a << 8 | b));
		case 0xb4: // ANDA >nn
			return void(a = mov8(a & read(fetch16())));
		case 0xb5: // BITA >nn
			return void(mov8(a & read(fetch16())));
		case 0xb6: // LDA >nn
			return void(a = mov8(read(fetch16())));
		case 0xb7: // STA >nn
			return write8(mov8(a), fetch16());
		case 0xb8: // EORA >nn
			return void(a = mov8(a ^ read(fetch16())));
		case 0xb9: // ADCA >nn
			return void(a = adc8(read(fetch16()), a));
		case 0xba: // ORA >nn
			return void(a = mov8(a | read(fetch16())));
		case 0xbb: // ADDA >nn
			return void(a = add8(read(fetch16()), a));
		case 0xbc: // CMPX >nn
			return void(sub16(read16(fetch16()), x));
		case 0xbd: // JSR >nn
			return jsr(fetch16());
		case 0xbe: // LDX >nn
			return void(x = mov16(read16(fetch16())));
		case 0xbf: // STX >nn
			return write16(mov16(x), fetch16());
		case 0xc0: // SUBB #n
			return void(b = sub8(fetch(), b));
		case 0xc1: // CMPB #n
			return void(sub8(fetch(), b));
		case 0xc2: // SBCB #n
			return void(b = sbc8(fetch(), b));
		case 0xc3: // ADDD #nn
			return split(a, b, add16(fetch16(), a << 8 | b));
		case 0xc4: // ANDB #n
			return void(b = mov8(b & fetch()));
		case 0xc5: // BITB #n
			return void(mov8(b & fetch()));
		case 0xc6: // LDB #n
			return void(b = mov8(fetch()));
		case 0xc8: // EORB #n
			return void(b = mov8(b ^ fetch()));
		case 0xc9: // ADCB #n
			return void(b = adc8(fetch(), b));
		case 0xca: // ORB #n
			return void(b = mov8(b | fetch()));
		case 0xcb: // ADDB #n
			return void(b = add8(fetch(), b));
		case 0xcc: // LDD #nn
			return split(a, b, mov16(fetch16()));
		case 0xce: // LDU #nn
			return void(u = mov16(fetch16()));
		case 0xd0: // SUBB <n
			return void(b = sub8(read(dp << 8 | fetch()), b));
		case 0xd1: // CMPB <n
			return void(sub8(read(dp << 8 | fetch()), b));
		case 0xd2: // SBCB <n
			return void(b = sbc8(read(dp << 8 | fetch()), b));
		case 0xd3: // ADDD <n
			return split(a, b, add16(read16(dp << 8 | fetch()), a << 8 | b));
		case 0xd4: // ANDB <n
			return void(b = mov8(b & read(dp << 8 | fetch())));
		case 0xd5: // BITB <n
			return void(mov8(b & read(dp << 8 | fetch())));
		case 0xd6: // LDB <n
			return void(b = mov8(read(dp << 8 | fetch())));
		case 0xd7: // STB <n
			return write8(mov8(b), dp << 8 | fetch());
		case 0xd8: // EORB <n
			return void(b = mov8(b ^ read(dp << 8 | fetch())));
		case 0xd9: // ADCB <n
			return void(b = adc8(read(dp << 8 | fetch()), b));
		case 0xda: // ORB <n
			return void(b = mov8(b | read(dp << 8 | fetch())));
		case 0xdb: // ADDB <n
			return void(b = add8(read(dp << 8 | fetch()), b));
		case 0xdc: // LDD <n
			return split(a, b, mov16(read16(dp << 8 | fetch())));
		case 0xdd: // STD <n
			return write16(mov16(a << 8 | b), dp << 8 | fetch());
		case 0xde: // LDU <n
			return void(u = mov16(read16(dp << 8 | fetch())));
		case 0xdf: // STU <n
			return write16(mov16(u), dp << 8 | fetch());
		case 0xe0: // SUBB ,r
			return void(b = sub8(read(index()), b));
		case 0xe1: // CMPB ,r
			return void(sub8(read(index()), b));
		case 0xe2: // SBCB ,r
			return void(b = sbc8(read(index()), b));
		case 0xe3: // ADDD ,r
			return split(a, b, add16(read16(index()), a << 8 | b));
		case 0xe4: // ANDB ,r
			return void(b = mov8(b & read(index())));
		case 0xe5: // BITB ,r
			return void(mov8(b & read(index())));
		case 0xe6: // LDB ,r
			return void(b = mov8(read(index())));
		case 0xe7: // STB ,r
			return write8(mov8(b), index());
		case 0xe8: // EORB ,r
			return void(b = mov8(b ^ read(index())));
		case 0xe9: // ADCB ,r
			return void(b = adc8(read(index()), b));
		case 0xea: // ORB ,r
			return void(b = mov8(b | read(index())));
		case 0xeb: // ADDB ,r
			return void(b = add8(read(index()), b));
		case 0xec: // LDD ,r
			return split(a, b, mov16(read16(index())));
		case 0xed: // STD ,r
			return write16(mov16(a << 8 | b), index());
		case 0xee: // LDU ,r
			return void(u = mov16(read16(index())));
		case 0xef: // STU ,r
			return write16(mov16(u), index());
		case 0xf0: // SUBB >nn
			return void(b = sub8(read(fetch16()), b));
		case 0xf1: // CMPB >nn
			return void(sub8(read(fetch16()), b));
		case 0xf2: // SBCB >nn
			return void(b = sbc8(read(fetch16()), b));
		case 0xf3: // ADDD >nn
			return split(a, b, add16(read16(fetch16()), a << 8 | b));
		case 0xf4: // ANDB >nn
			return void(b = mov8(b & read(fetch16())));
		case 0xf5: // BITB >nn
			return void(mov8(b & read(fetch16())));
		case 0xf6: // LDB >nn
			return void(b = mov8(read(fetch16())));
		case 0xf7: // STB >nn
			return write8(mov8(b), fetch16());
		case 0xf8: // EORB >nn
			return void(b = mov8(b ^ read(fetch16())));
		case 0xf9: // ADCB >nn
			return void(b = adc8(read(fetch16()), b));
		case 0xfa: // ORB >nn
			return void(b = mov8(b | read(fetch16())));
		case 0xfb: // ADDB >nn
			return void(b = add8(read(fetch16()), b));
		case 0xfc: // LDD >nn
			return split(a, b, mov16(read16(fetch16())));
		case 0xfd: // STD >nn
			return write16(mov16(a << 8 | b), fetch16());
		case 0xfe: // LDU >nn
			return void(u = mov16(read16(fetch16())));
		case 0xff: // STU >nn
			return write16(mov16(u), fetch16());
		default:
			undefsize = 1;
			if (undef)
				undef(pc);
			return;
		}
	}

	int index() {
		int v, pb = fetch();
		cycle -= cc_i[pb];
		switch (pb) {
		case 0x00: // $0,X
			return x;
		case 0x01: // $1,X
			return x + 1 & 0xffff;
		case 0x02: // $2,X
			return x + 2 & 0xffff;
		case 0x03: // $3,X
			return x + 3 & 0xffff;
		case 0x04: // $4,X
			return x + 4 & 0xffff;
		case 0x05: // $5,X
			return x + 5 & 0xffff;
		case 0x06: // $6,X
			return x + 6 & 0xffff;
		case 0x07: // $7,X
			return x + 7 & 0xffff;
		case 0x08: // $8,X
			return x + 8 & 0xffff;
		case 0x09: // $9,X
			return x + 9 & 0xffff;
		case 0x0a: // $a,X
			return x + 0x0a & 0xffff;
		case 0x0b: // $b,X
			return x + 0x0b & 0xffff;
		case 0x0c: // $c,X
			return x + 0x0c & 0xffff;
		case 0x0d: // $d,X
			return x + 0x0d & 0xffff;
		case 0x0e: // $e,X
			return x + 0x0e & 0xffff;
		case 0x0f: // $f,X
			return x + 0x0f & 0xffff;
		case 0x10: // -$10,X
			return x - 0x10 & 0xffff;
		case 0x11: // -$f,X
			return x - 0x0f & 0xffff;
		case 0x12: // -$e,X
			return x - 0x0e & 0xffff;
		case 0x13: // -$d,X
			return x - 0x0d & 0xffff;
		case 0x14: // -$c,X
			return x - 0x0c & 0xffff;
		case 0x15: // -$b,X
			return x - 0x0b & 0xffff;
		case 0x16: // -$a,X
			return x - 0x0a & 0xffff;
		case 0x17: // -$9,X
			return x - 9 & 0xffff;
		case 0x18: // -$8,X
			return x - 8 & 0xffff;
		case 0x19: // -$7,X
			return x - 7 & 0xffff;
		case 0x1a: // -$6,X
			return x - 6 & 0xffff;
		case 0x1b: // -$5,X
			return x - 5 & 0xffff;
		case 0x1c: // -$4,X
			return x - 4 & 0xffff;
		case 0x1d: // -$3,X
			return x - 3 & 0xffff;
		case 0x1e: // -$2,X
			return x - 2 & 0xffff;
		case 0x1f: // -$1,X
			return x - 1 & 0xffff;
		case 0x20: // $0,Y
			return y;
		case 0x21: // $1,Y
			return y + 1 & 0xffff;
		case 0x22: // $2,Y
			return y + 2 & 0xffff;
		case 0x23: // $3,Y
			return y + 3 & 0xffff;
		case 0x24: // $4,Y
			return y + 4 & 0xffff;
		case 0x25: // $5,Y
			return y + 5 & 0xffff;
		case 0x26: // $6,Y
			return y + 6 & 0xffff;
		case 0x27: // $7,Y
			return y + 7 & 0xffff;
		case 0x28: // $8,Y
			return y + 8 & 0xffff;
		case 0x29: // $9,Y
			return y + 9 & 0xffff;
		case 0x2a: // $a,Y
			return y + 0x0a & 0xffff;
		case 0x2b: // $b,Y
			return y + 0x0b & 0xffff;
		case 0x2c: // $c,Y
			return y + 0x0c & 0xffff;
		case 0x2d: // $d,Y
			return y + 0x0d & 0xffff;
		case 0x2e: // $e,Y
			return y + 0x0e & 0xffff;
		case 0x2f: // $f,Y
			return y + 0x0f & 0xffff;
		case 0x30: // -$10,Y
			return y - 0x10 & 0xffff;
		case 0x31: // -$f,Y
			return y - 0x0f & 0xffff;
		case 0x32: // -$e,Y
			return y - 0x0e & 0xffff;
		case 0x33: // -$d,Y
			return y - 0x0d & 0xffff;
		case 0x34: // -$c,Y
			return y - 0x0c & 0xffff;
		case 0x35: // -$b,Y
			return y - 0x0b & 0xffff;
		case 0x36: // -$a,Y
			return y - 0x0a & 0xffff;
		case 0x37: // -$9,Y
			return y - 9 & 0xffff;
		case 0x38: // -$8,Y
			return y - 8 & 0xffff;
		case 0x39: // -$7,Y
			return y - 7 & 0xffff;
		case 0x3a: // -$6,Y
			return y - 6 & 0xffff;
		case 0x3b: // -$5,Y
			return y - 5 & 0xffff;
		case 0x3c: // -$4,Y
			return y - 4 & 0xffff;
		case 0x3d: // -$3,Y
			return y - 3 & 0xffff;
		case 0x3e: // -$2,Y
			return y - 2 & 0xffff;
		case 0x3f: // -$1,Y
			return y - 1 & 0xffff;
		case 0x40: // $0,U
			return u;
		case 0x41: // $1,U
			return u + 1 & 0xffff;
		case 0x42: // $2,U
			return u + 2 & 0xffff;
		case 0x43: // $3,U
			return u + 3 & 0xffff;
		case 0x44: // $4,U
			return u + 4 & 0xffff;
		case 0x45: // $5,U
			return u + 5 & 0xffff;
		case 0x46: // $6,U
			return u + 6 & 0xffff;
		case 0x47: // $7,U
			return u + 7 & 0xffff;
		case 0x48: // $8,U
			return u + 8 & 0xffff;
		case 0x49: // $9,U
			return u + 9 & 0xffff;
		case 0x4a: // $a,U
			return u + 0x0a & 0xffff;
		case 0x4b: // $b,U
			return u + 0x0b & 0xffff;
		case 0x4c: // $c,U
			return u + 0x0c & 0xffff;
		case 0x4d: // $d,U
			return u + 0x0d & 0xffff;
		case 0x4e: // $e,U
			return u + 0x0e & 0xffff;
		case 0x4f: // $f,U
			return u + 0x0f & 0xffff;
		case 0x50: // -$10,U
			return u - 0x10 & 0xffff;
		case 0x51: // -$f,U
			return u - 0x0f & 0xffff;
		case 0x52: // -$e,U
			return u - 0x0e & 0xffff;
		case 0x53: // -$d,U
			return u - 0x0d & 0xffff;
		case 0x54: // -$c,U
			return u - 0x0c & 0xffff;
		case 0x55: // -$b,U
			return u - 0x0b & 0xffff;
		case 0x56: // -$a,U
			return u - 0x0a & 0xffff;
		case 0x57: // -$9,U
			return u - 9 & 0xffff;
		case 0x58: // -$8,U
			return u - 8 & 0xffff;
		case 0x59: // -$7,U
			return u - 7 & 0xffff;
		case 0x5a: // -$6,U
			return u - 6 & 0xffff;
		case 0x5b: // -$5,U
			return u - 5 & 0xffff;
		case 0x5c: // -$4,U
			return u - 4 & 0xffff;
		case 0x5d: // -$3,U
			return u - 3 & 0xffff;
		case 0x5e: // -$2,U
			return u - 2 & 0xffff;
		case 0x5f: // -$1,U
			return u - 1 & 0xffff;
		case 0x60: // $0,S
			return s;
		case 0x61: // $1,S
			return s + 1 & 0xffff;
		case 0x62: // $2,S
			return s + 2 & 0xffff;
		case 0x63: // $3,S
			return s + 3 & 0xffff;
		case 0x64: // $4,S
			return s + 4 & 0xffff;
		case 0x65: // $5,S
			return s + 5 & 0xffff;
		case 0x66: // $6,S
			return s + 6 & 0xffff;
		case 0x67: // $7,S
			return s + 7 & 0xffff;
		case 0x68: // $8,S
			return s + 8 & 0xffff;
		case 0x69: // $9,S
			return s + 9 & 0xffff;
		case 0x6a: // $a,S
			return s + 0x0a & 0xffff;
		case 0x6b: // $b,S
			return s + 0x0b & 0xffff;
		case 0x6c: // $c,S
			return s + 0x0c & 0xffff;
		case 0x6d: // $d,S
			return s + 0x0d & 0xffff;
		case 0x6e: // $e,S
			return s + 0x0e & 0xffff;
		case 0x6f: // $f,S
			return s + 0x0f & 0xffff;
		case 0x70: // -$10,S
			return s - 0x10 & 0xffff;
		case 0x71: // -$f,S
			return s - 0x0f & 0xffff;
		case 0x72: // -$e,S
			return s - 0x0e & 0xffff;
		case 0x73: // -$d,S
			return s - 0x0d & 0xffff;
		case 0x74: // -$c,S
			return s - 0x0c & 0xffff;
		case 0x75: // -$b,S
			return s - 0x0b & 0xffff;
		case 0x76: // -$a,S
			return s - 0x0a & 0xffff;
		case 0x77: // -$9,S
			return s - 9 & 0xffff;
		case 0x78: // -$8,S
			return s - 8 & 0xffff;
		case 0x79: // -$7,S
			return s - 7 & 0xffff;
		case 0x7a: // -$6,S
			return s - 6 & 0xffff;
		case 0x7b: // -$5,S
			return s - 5 & 0xffff;
		case 0x7c: // -$4,S
			return s - 4 & 0xffff;
		case 0x7d: // -$3,S
			return s - 3 & 0xffff;
		case 0x7e: // -$2,S
			return s - 2 & 0xffff;
		case 0x7f: // -$1,S
			return s - 1 & 0xffff;
		case 0x80: // ,X+
			return v = x, x = x + 1 & 0xffff, v;
		case 0x81: // ,X++
			return v = x, x = x + 2 & 0xffff, v;
		case 0x82: // ,-X
			return x = x - 1 & 0xffff;
		case 0x83: // ,--X
			return x = x - 2 & 0xffff;
		case 0x84: // ,X
			return x;
		case 0x85: // B,X
			return x + (b << 24 >> 24) & 0xffff;
		case 0x86: // A,X
			return x + (a << 24 >> 24) & 0xffff;
		case 0x88: // n,X
			return x + (fetch() << 24 >> 24) & 0xffff;
		case 0x89: // nn,X
			return x + fetch16() & 0xffff;
		case 0x8b: // D,X
			return x + (a << 8 | b) & 0xffff;
		case 0x91: // [,X++]
			return v = read16(x), x = x + 2 & 0xffff, v;
		case 0x93: // [,--X]
			return x = x - 2 & 0xffff, read16(x);
		case 0x94: // [,X]
			return read16(x);
		case 0x95: // [B,X]
			return read16(x + (b << 24 >> 24) & 0xffff);
		case 0x96: // [A,X]
			return read16(x + (a << 24 >> 24) & 0xffff);
		case 0x98: // [n,X]
			return read16(x + (fetch() << 24 >> 24) & 0xffff);
		case 0x99: // [nn,X]
			return read16(x + fetch16() & 0xffff);
		case 0x9b: // [D,X]
			return read16(x + (a << 8 | b) & 0xffff);
		case 0xa0: // ,Y+
			return v = y, y = y + 1 & 0xffff, v;
		case 0xa1: // ,Y++
			return v = y, y = y + 2 & 0xffff, v;
		case 0xa2: // ,-Y
			return y = y - 1 & 0xffff;
		case 0xa3: // ,--Y
			return y = y - 2 & 0xffff;
		case 0xa4: // ,Y
			return y;
		case 0xa5: // B,Y
			return y + (b << 24 >> 24) & 0xffff;
		case 0xa6: // A,Y
			return y + (a << 24 >> 24) & 0xffff;
		case 0xa8: // n,Y
			return y + (fetch() << 24 >> 24) & 0xffff;
		case 0xa9: // nn,Y
			return y + fetch16() & 0xffff;
		case 0xab: // D,Y
			return y + (a << 8 | b) & 0xffff;
		case 0xb1: // [,Y++]
			return v = read16(y), y = y + 2 & 0xffff, v;
		case 0xb3: // [,--Y]
			return y = y - 2 & 0xffff, read16(y);
		case 0xb4: // [,Y]
			return read16(y);
		case 0xb5: // [B,Y]
			return read16(y + (b << 24 >> 24) & 0xffff);
		case 0xb6: // [A,Y]
			return read16(y + (a << 24 >> 24) & 0xffff);
		case 0xb8: // [n,Y]
			return read16(y + (fetch() << 24 >> 24) & 0xffff);
		case 0xb9: // [nn,Y]
			return read16(y + fetch16() & 0xffff);
		case 0xbb: // [D,Y]
			return read16(y + (a << 8 | b) & 0xffff);
		case 0xc0: // ,U+
			return v = u, u = u + 1 & 0xffff, v;
		case 0xc1: // ,U++
			return v = u, u = u + 2 & 0xffff, v;
		case 0xc2: // ,-U
			return u = u - 1 & 0xffff;
		case 0xc3: // ,--u
			return u = u - 2 & 0xffff;
		case 0xc4: // ,U
			return u;
		case 0xc5: // B,U
			return u + (b << 24 >> 24) & 0xffff;
		case 0xc6: // A,U
			return u + (a << 24 >> 24) & 0xffff;
		case 0xc8: // n,U
			return u + (fetch() << 24 >> 24) & 0xffff;
		case 0xc9: // nn,U
			return u + fetch16() & 0xffff;
		case 0xcb: // D,U
			return u + (a << 8 | b) & 0xffff;
		case 0xd1: // [,U++]
			return v = read16(u), u = u + 2 & 0xffff, v;
		case 0xd3: // [,--U]
			return u = u - 2 & 0xffff, read16(u);
		case 0xd4: // [,U]
			return read16(u);
		case 0xd5: // [B,U]
			return read16(u + (b << 24 >> 24) & 0xffff);
		case 0xd6: // [A,U]
			return read16(u + (a << 24 >> 24) & 0xffff);
		case 0xd8: // [n,U]
			return read16(u + (fetch() << 24 >> 24) & 0xffff);
		case 0xd9: // [nn,U]
			return read16(u + fetch16() & 0xffff);
		case 0xdb: // [D,U]
			return read16(u + (a << 8 | b) & 0xffff);
		case 0xe0: // ,S+
			return v = s, s = s + 1 & 0xffff, v;
		case 0xe1: // ,S++
			return v = s, s = s + 2 & 0xffff, v;
		case 0xe2: // ,-S
			return s = s - 1 & 0xffff;
		case 0xe3: // ,--S
			return s = s - 2 & 0xffff;
		case 0xe4: // ,S
			return s;
		case 0xe5: // B,S
			return s + (b << 24 >> 24) & 0xffff;
		case 0xe6: // A,S
			return s + (a << 24 >> 24) & 0xffff;
		case 0xe8: // n,S
			return s + (fetch() << 24 >> 24) & 0xffff;
		case 0xe9: // nn,S
			return s + fetch16() & 0xffff;
		case 0xeb: // D,S
			return s + (a << 8 | b) & 0xffff;
		case 0xf1: // [,S++]
			return v = read16(s), s = s + 2 & 0xffff, v;
		case 0xf3: // [,--S]
			return s = s - 2 & 0xffff, read16(s);
		case 0xf4: // [,S]
			return read16(s);
		case 0xf5: // [B,S]
			return read16(s + (b << 24 >> 24) & 0xffff);
		case 0xf6: // [A,S]
			return read16(s + (a << 24 >> 24) & 0xffff);
		case 0xf8: // [n,S]
			return read16(s + (fetch() << 24 >> 24) & 0xffff);
		case 0xf9: // [nn,S]
			return read16(s + fetch16() & 0xffff);
		case 0xfb: // [D,S]
			return read16(s + (a << 8 | b) & 0xffff);
		case 0x8c: case 0xac: case 0xcc: case 0xec: // n,PC
			return v = fetch(), pc + (v << 24 >> 24) & 0xffff;
		case 0x8d: case 0xad: case 0xcd: case 0xed: // nn,PC
			return v = fetch16(), pc + v & 0xffff;
		case 0x9c: case 0xbc: case 0xdc: case 0xfc: // [n,PC]
			return v = fetch(), read16(pc + (v << 24 >> 24) & 0xffff);
		case 0x9d: case 0xbd: case 0xdd: case 0xfd: // [nn,PC]
			return v = fetch16(), read16(pc + v & 0xffff);
		case 0x9f: case 0xbf: case 0xdf: case 0xff: // [nn]
			return read16(fetch16());
		default:
			return 0xffffffff;
		}
	}

	void lbcc(bool cond) {
		const int nn = fetch16();
		cycle -= cond ? 2 : 1, cond && (pc = pc + nn & 0xffff);
	}

	void lbsr() {
		const int nn = fetch16();
		pshs16(pc), pc = pc + nn & 0xffff;
	}

	void bcc(bool cond) {
		const int n = fetch();
		if (cond) pc = pc + (n << 24 >> 24) & 0xffff;
	}

	void bsr() {
		const int n = fetch();
		pshs16(pc), pc = pc + (n << 24 >> 24) & 0xffff;
	}

	int neg8(int dst) {
		const int r = -dst & 0xff, v = dst & r, c = dst | r;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int com8(int dst) {
		const int r = ~dst & 0xff;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | 1, r;
	}

	int lsr8(int dst) {
		const int r = dst >> 1, c = dst & 1;
		return ccr = ccr & ~0x0d | !r << 2 | c, r;
	}

	int ror8(int dst) {
		const int r = dst >> 1 | ccr << 7 & 0x80, c = dst & 1;
		return ccr = ccr & ~0x0d | r >> 4 & 8 | !r << 2 | c, r;
	}

	int asr8(int dst) {
		const int r = dst >> 1 | dst & 0x80, c = dst & 1;
		return ccr = ccr & ~0x0d | r >> 4 & 8 | !r << 2 | c, r;
	}

	int lsl8(int dst) {
		const int r = dst << 1 & 0xff, c = dst >> 7, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int rol8(int dst) {
		const int r = dst << 1 & 0xff | ccr & 1, c = dst >> 7, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int dec8(int dst) {
		const int r = dst - 1 & 0xff, v = dst & ~1 & ~r | ~dst & 1 & r;
		return ccr = ccr & ~0x0e | r >> 4 & 8 | !r << 2 | v >> 6 & 2, r;
	}

	int inc8(int dst) {
		const int r = dst + 1 & 0xff, v = dst & 1 & ~r | ~dst & ~1 & r;
		return ccr = ccr & ~0x0e | r >> 4 & 8 | !r << 2 | v >> 6 & 2, r;
	}

	int clr8() {
		return ccr = ccr & ~0x0f | 4, 0;
	}

	int sub8(int src, int dst) {
		const int r = dst - src & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int sub16(int src, int dst) {
		const int r = dst - src & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return ccr = ccr & ~0x0f | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	int sbc8(int src, int dst) {
		const int r = dst - src - (ccr & 1) & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int mov8(int src) {
		return ccr = ccr & ~0x0e | src >> 4 & 8 | !src << 2, src;
	}

	int mov16(int src) {
		return ccr = ccr & ~0x0e | src >> 12 & 8 | !src << 2, src;
	}

	int adc8(int src, int dst) {
		const int r = dst + src + (ccr & 1) & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return ccr = ccr & ~0x2f | c << 2 & 0x20 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int add8(int src, int dst) {
		const int r = dst + src & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return ccr = ccr & ~0x2f | c << 2 & 0x20 | r >> 4 & 8 | !r << 2 | v >> 6 & 2 | c >> 7 & 1, r;
	}

	int add16(int src, int dst) {
		const int r = dst + src & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return ccr = ccr & ~0x0f | r >> 12 & 8 | !r << 2 | v >> 14 & 2 | c >> 15 & 1, r;
	}

	void daa() {
		int cf = 0;
		if (ccr & 0x20 || (a & 0xf) > 9)
			cf += 6;
		if (ccr & 1 || (a & 0xf0) > 0x90 || (a & 0xf0) > 0x80 && (a & 0xf) > 9)
			cf += 0x60, ccr |= 1;
		a = a + cf & 0xff, ccr = ccr & ~0x0c | a >> 4 & 8 | !a << 2;
	}

	void jsr(int ea) {
		pshs16(pc), pc = ea;
	}

	void pshs(int r) {
		s = s - 1 & 0xffff, write8(r, s);
	}

	void pshu(int r) {
		u = u - 1 & 0xffff, write8(r, u);
	}

	int puls() {
		const int r = read(s);
		return s = s + 1 & 0xffff, r;
	}

	int pulu() {
		const int r = read(u);
		return u = u + 1 & 0xffff, r;
	}

	void pshs16(int r) {
		pshs(r & 0xff), pshs(r >> 8);
	}

	void pshu16(int r) {
		pshu(r & 0xff), pshu(r >> 8);
	}

	int puls16() {
		const int r = puls() << 8;
		return r | puls();
	}

	int pulu16() {
		const int r = pulu() << 8;
		return r | pulu();
	}

	static void split(int& h, int& l, int v) {
		h = v >> 8, l = v & 0xff;
	}

	int fetch16() {
		const int data = fetch() << 8;
		return data | fetch();
	}

	int read16(int addr) {
		const int data = read(addr) << 8;
		return data | read(addr + 1 & 0xffff);
	}

	void write8(int data, int addr) {
		Page& page = memorymap[addr >> 8];
		!page.write ? void(page.base[addr & 0xff] = data) : page.write(addr, data);
	}

	void write16(int data, int addr) {
		write8(data >> 8, addr), write8(data & 0xff, addr + 1 & 0xffff);
	}

	static void exg(int& src, int& dst) {
		int v;
		v = dst, dst = src, src = v;
	}
};

#endif //MC6809_H
