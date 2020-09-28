/*
 *
 *	Z80 Emulator
 *
 */

#ifndef Z80_H
#define Z80_H

#include "cpu.h"

struct Z80 : Cpu {
	static int fLogic[0x100];
	int b = 0;
	int c = 0;
	int d = 0;
	int e = 0;
	int h = 0;
	int l = 0;
	int a = 0;
	int f = 0; // f:sz-h-pnc
	int ixh = 0;
	int ixl = 0;
	int iyh = 0;
	int iyl = 0;
	int iff = 0;
	int im = 0;
	int i = 0;
	int r = 0;
	int b_prime = 0;
	int c_prime = 0;
	int d_prime = 0;
	int e_prime = 0;
	int h_prime = 0;
	int l_prime = 0;
	int a_prime = 0;
	int f_prime = 0;
	int sp = 0;
	Page iomap[0x100];

	Z80() {}

	void reset() {
		Cpu::reset();
		iff = 0;
		im = 0;
		sp = 0;
		pc = 0;
	}

	bool interrupt() {
		if (!Cpu::interrupt() || iff != 3)
			return false;
		iff = 0;
		switch (im) {
		case 0:
		case 1:
			return rst(0x38), true;
		case 2:
			return rst(read16(0xff | i << 8)), true;
		}
		return true;
	}

	bool interrupt(int vector) {
		if (!Cpu::interrupt() || iff != 3)
			return false;
		iff = 0;
		switch (im) {
		case 0:
			switch (vector) {
			case 0xc7: // RST 00h
				return rst(0x00), true;
			case 0xcf: // RST 08h
				return rst(0x08), true;
			case 0xd7: // RST 10h
				return rst(0x10), true;
			case 0xdf: // RST 18h
				return rst(0x18), true;
			case 0xe7: // RST 20h
				return rst(0x20), true;
			case 0xef: // RST 28h
				return rst(0x28), true;
			case 0xf7: // RST 30h
				return rst(0x30), true;
			case 0xff: // RST 38h
				return rst(0x38), true;
			}
			break;
		case 1:
			return rst(0x38), true;
		case 2:
			return rst(read16(vector & 0xff | i << 8)), true;
		}
		return true;
	}

	bool non_maskable_interrupt() {
		if (!Cpu::interrupt())
			return false;
		return iff &= 2, rst(0x66), true;
	}

	void _execute() {
		int v;

		r = r + 1 & 0x7f;
		switch (fetch()) {
		case 0x00: // NOP
			return;
		case 0x01: // LD BC,nn
			return split(c, b, fetch16());
		case 0x02: // LD (BC),A
			return write(c | b << 8, a);
		case 0x03: // INC BC
			return split(c, b, (c | b << 8) + 1 & 0xffff);
		case 0x04: // INC B
			return void(b = inc8(b));
		case 0x05: // DEC B
			return void(b = dec8(b));
		case 0x06: // LD B,n
			return void(b = fetch());
		case 0x07: // RLCA
			return f = f & ~0x13 | a >> 7, void(a = a << 1 & 0xff | a >> 7);
		case 0x08: // EX AF,AF'
			return ex(f, a, f_prime, a_prime);
		case 0x09: // ADD HL,BC
			return split(l, h, add16(l | h << 8, c | b << 8));
		case 0x0a: // LD A,(BC)
			return void(a = read(c | b << 8));
		case 0x0b: // DEC BC
			return split(c, b, (c | b << 8) - 1 & 0xffff);
		case 0x0c: // INC C
			return void(c = inc8(c));
		case 0x0d: // DEC C
			return void(c = dec8(c));
		case 0x0e: // LD C,n
			return void(c = fetch());
		case 0x0f: // RRCA
			return f = f & ~0x13 | a & 1, void(a = a >> 1 | a << 7 & 0x80);
		case 0x10: // DJNZ e
			return jr((b = b - 1 & 0xff) != 0);
		case 0x11: // LD DE,nn
			return split(e, d, fetch16());
		case 0x12: // LD (DE),A
			return write(e | d << 8, a);
		case 0x13: // INC DE
			return split(e, d, (e | d << 8) + 1 & 0xffff);
		case 0x14: // INC D
			return void(d = inc8(d));
		case 0x15: // DEC D
			return void(d = dec8(d));
		case 0x16: // LD D,n
			return void(d = fetch());
		case 0x17: // RLA
			return v = f, f = f & ~0x13 | a >> 7, void(a = a << 1 & 0xff | v & 1);
		case 0x18: // JR e
			return jr(true);
		case 0x19: // ADD HL,DE
			return split(l, h, add16(l | h << 8, e | d << 8));
		case 0x1a: // LD A,(DE)
			return void(a = read(e | d << 8));
		case 0x1b: // DEC DE
			return split(e, d, (e | d << 8) - 1 & 0xffff);
		case 0x1c: // INC E
			return void(e = inc8(e));
		case 0x1d: // DEC E
			return void(e = dec8(e));
		case 0x1e: // LD E,n
			return void(e = fetch());
		case 0x1f: // RRA
			return v = f, f = f & ~0x13 | a & 1, void(a = a >> 1 | v << 7 & 0x80);
		case 0x20: // JR NZ,e
			return jr((f & 0x40) == 0);
		case 0x21: // LD HL,nn
			return split(l, h, fetch16());
		case 0x22: // LD (nn),HL
			return write16(fetch16(), l | h << 8);
		case 0x23: // INC HL
			return split(l, h, (l | h << 8) + 1 & 0xffff);
		case 0x24: // INC H
			return void(h = inc8(h));
		case 0x25: // DEC H
			return void(h = dec8(h));
		case 0x26: // LD H,n
			return void(h = fetch());
		case 0x27: // DAA
			return daa();
		case 0x28: // JR Z,e
			return jr((f & 0x40) != 0);
		case 0x29: // ADD HL,HL
			return split(l, h, add16(l | h << 8, l | h << 8));
		case 0x2a: // LD HL,(nn)
			return split(l, h, read16(fetch16()));
		case 0x2b: // DEC HL
			return split(l, h, (l | h << 8) - 1 & 0xffff);
		case 0x2c: // INC L
			return void(l = inc8(l));
		case 0x2d: // DEC L
			return void(l = dec8(l));
		case 0x2e: // LD L,n
			return void(l = fetch());
		case 0x2f: // CPL
			return f |= 0x12, void(a = ~a & 0xff);
		case 0x30: // JR NC,e
			return jr((f & 1) == 0);
		case 0x31: // LD SP,nn
			return void(sp = fetch16());
		case 0x32: // LD (nn),A
			return write(fetch16(), a);
		case 0x33: // INC SP
			return void(sp = sp + 1 & 0xffff);
		case 0x34: // INC (HL)
			return v = l | h << 8, write(v, inc8(read(v)));
		case 0x35: // DEC (HL)
			return v = l | h << 8, write(v, dec8(read(v)));
		case 0x36: // LD (HL),n
			return write(l | h << 8, fetch());
		case 0x37: // SCF
			return void(f = f & ~0x12 | 1);
		case 0x38: // JR C,e
			return jr((f & 1) != 0);
		case 0x39: // ADD HL,SP
			return split(l, h, add16(l | h << 8, sp));
		case 0x3a: // LD A,(nn)
			return void(a = read(fetch16()));
		case 0x3b: // DEC SP
			return void(sp = sp - 1 & 0xffff);
		case 0x3c: // INC A
			return void(a = inc8(a));
		case 0x3d: // DEC A
			return void(a = dec8(a));
		case 0x3e: // LD A,n
			return void(a = fetch());
		case 0x3f: // CCF
			return void(f = f & ~0x12 ^ 1 | f << 4 & 0x10);
		case 0x40: // LD B,B
			return;
		case 0x41: // LD B,C
			return void(b = c);
		case 0x42: // LD B,D
			return void(b = d);
		case 0x43: // LD B,E
			return void(b = e);
		case 0x44: // LD B,H
			return void(b = h);
		case 0x45: // LD B,L
			return void(b = l);
		case 0x46: // LD B,(HL)
			return void(b = read(l | h << 8));
		case 0x47: // LD B,A
			return void(b = a);
		case 0x48: // LD C,B
			return void(c = b);
		case 0x49: // LD C,C
			return;
		case 0x4a: // LD C,D
			return void(c = d);
		case 0x4b: // LD C,E
			return void(c = e);
		case 0x4c: // LD C,H
			return void(c = h);
		case 0x4d: // LD C,L
			return void(c = l);
		case 0x4e: // LD C,(HL)
			return void(c = read(l | h << 8));
		case 0x4f: // LD C,A
			return void(c = a);
		case 0x50: // LD D,B
			return void(d = b);
		case 0x51: // LD D,C
			return void(d = c);
		case 0x52: // LD D,D
			return;
		case 0x53: // LD D,E
			return void(d = e);
		case 0x54: // LD D,H
			return void(d = h);
		case 0x55: // LD D,L
			return void(d = l);
		case 0x56: // LD D,(HL)
			return void(d = read(l | h << 8));
		case 0x57: // LD D,A
			return void(d = a);
		case 0x58: // LD E,B
			return void(e = b);
		case 0x59: // LD E,C
			return void(e = c);
		case 0x5a: // LD E,D
			return void(e = d);
		case 0x5b: // LD E,E
			return;
		case 0x5c: // LD E,H
			return void(e = h);
		case 0x5d: // LD E,L
			return void(e = l);
		case 0x5e: // LD E,(HL)
			return void(e = read(l | h << 8));
		case 0x5f: // LD E,A
			return void(e = a);
		case 0x60: // LD H,B
			return void(h = b);
		case 0x61: // LD H,C
			return void(h = c);
		case 0x62: // LD H,D
			return void(h = d);
		case 0x63: // LD H,E
			return void(h = e);
		case 0x64: // LD H,H
			return;
		case 0x65: // LD H,L
			return void(h = l);
		case 0x66: // LD H,(HL)
			return void(h = read(l | h << 8));
		case 0x67: // LD H,A
			return void(h = a);
		case 0x68: // LD L,B
			return void(l = b);
		case 0x69: // LD L,C
			return void(l = c);
		case 0x6a: // LD L,D
			return void(l = d);
		case 0x6b: // LD L,E
			return void(l = e);
		case 0x6c: // LD L,H
			return void(l = h);
		case 0x6d: // LD L,L
			return;
		case 0x6e: // LD L,(HL)
			return void(l = read(l | h << 8));
		case 0x6f: // LD L,A
			return void(l = a);
		case 0x70: // LD (HL),B
			return write(l | h << 8, b);
		case 0x71: // LD (HL),C
			return write(l | h << 8, c);
		case 0x72: // LD (HL),D
			return write(l | h << 8, d);
		case 0x73: // LD (HL),E
			return write(l | h << 8, e);
		case 0x74: // LD (HL),H
			return write(l | h << 8, h);
		case 0x75: // LD (HL),L
			return write(l | h << 8, l);
		case 0x76: // HALT
			return suspend();
		case 0x77: // LD (HL),A
			return write(l | h << 8, a);
		case 0x78: // LD A,B
			return void(a = b);
		case 0x79: // LD A,C
			return void(a = c);
		case 0x7a: // LD A,D
			return void(a = d);
		case 0x7b: // LD A,E
			return void(a = e);
		case 0x7c: // LD A,H
			return void(a = h);
		case 0x7d: // LD A,L
			return void(a = l);
		case 0x7e: // LD A,(HL)
			return void(a = read(l | h << 8));
		case 0x7f: // LD A,A
			return;
		case 0x80: // ADD A,B
			return void(a = add8(a, b));
		case 0x81: // ADD A,C
			return void(a = add8(a, c));
		case 0x82: // ADD A,D
			return void(a = add8(a, d));
		case 0x83: // ADD A,E
			return void(a = add8(a, e));
		case 0x84: // ADD A,H
			return void(a = add8(a, h));
		case 0x85: // ADD A,L
			return void(a = add8(a, l));
		case 0x86: // ADD A,(HL)
			return void(a = add8(a, read(l | h << 8)));
		case 0x87: // ADD A,A
			return void(a = add8(a, a));
		case 0x88: // ADC A,B
			return void(a = adc8(a, b));
		case 0x89: // ADC A,C
			return void(a = adc8(a, c));
		case 0x8a: // ADC A,D
			return void(a = adc8(a, d));
		case 0x8b: // ADC A,E
			return void(a = adc8(a, e));
		case 0x8c: // ADC A,H
			return void(a = adc8(a, h));
		case 0x8d: // ADC A,L
			return void(a = adc8(a, l));
		case 0x8e: // ADC A,(HL)
			return void(a = adc8(a, read(l | h << 8)));
		case 0x8f: // ADC A,A
			return void(a = adc8(a, a));
		case 0x90: // SUB B
			return void(a = sub8(a, b));
		case 0x91: // SUB C
			return void(a = sub8(a, c));
		case 0x92: // SUB D
			return void(a = sub8(a, d));
		case 0x93: // SUB E
			return void(a = sub8(a, e));
		case 0x94: // SUB H
			return void(a = sub8(a, h));
		case 0x95: // SUB L
			return void(a = sub8(a, l));
		case 0x96: // SUB (HL)
			return void(a = sub8(a, read(l | h << 8)));
		case 0x97: // SUB A
			return void(a = sub8(a, a));
		case 0x98: // SBC A,B
			return void(a = sbc8(a, b));
		case 0x99: // SBC A,C
			return void(a = sbc8(a, c));
		case 0x9a: // SBC A,D
			return void(a = sbc8(a, d));
		case 0x9b: // SBC A,E
			return void(a = sbc8(a, e));
		case 0x9c: // SBC A,H
			return void(a = sbc8(a, h));
		case 0x9d: // SBC A,L
			return void(a = sbc8(a, l));
		case 0x9e: // SBC A,(HL)
			return void(a = sbc8(a, read(l | h << 8)));
		case 0x9f: // SBC A,A
			return void(a = sbc8(a, a));
		case 0xa0: // AND B
			return void(a = and8(a, b));
		case 0xa1: // AND C
			return void(a = and8(a, c));
		case 0xa2: // AND D
			return void(a = and8(a, d));
		case 0xa3: // AND E
			return void(a = and8(a, e));
		case 0xa4: // AND H
			return void(a = and8(a, h));
		case 0xa5: // AND L
			return void(a = and8(a, l));
		case 0xa6: // AND (HL)
			return void(a = and8(a, read(l | h << 8)));
		case 0xa7: // AND A
			return void(a = and8(a, a));
		case 0xa8: // XOR B
			return void(a = xor8(a, b));
		case 0xa9: // XOR C
			return void(a = xor8(a, c));
		case 0xaa: // XOR D
			return void(a = xor8(a, d));
		case 0xab: // XOR E
			return void(a = xor8(a, e));
		case 0xac: // XOR H
			return void(a = xor8(a, h));
		case 0xad: // XOR L
			return void(a = xor8(a, l));
		case 0xae: // XOR (HL)
			return void(a = xor8(a, read(l | h << 8)));
		case 0xaf: // XOR A
			return void(a = xor8(a, a));
		case 0xb0: // OR B
			return void(a = or8(a, b));
		case 0xb1: // OR C
			return void(a = or8(a, c));
		case 0xb2: // OR D
			return void(a = or8(a, d));
		case 0xb3: // OR E
			return void(a = or8(a, e));
		case 0xb4: // OR H
			return void(a = or8(a, h));
		case 0xb5: // OR L
			return void(a = or8(a, l));
		case 0xb6: // OR (HL)
			return void(a = or8(a, read(l | h << 8)));
		case 0xb7: // OR A
			return void(a = or8(a, a));
		case 0xb8: // CP B
			return void(sub8(a, b));
		case 0xb9: // CP C
			return void(sub8(a, c));
		case 0xba: // CP D
			return void(sub8(a, d));
		case 0xbb: // CP E
			return void(sub8(a, e));
		case 0xbc: // CP H
			return void(sub8(a, h));
		case 0xbd: // CP L
			return void(sub8(a, l));
		case 0xbe: // CP (HL)
			return void(sub8(a, read(l | h << 8)));
		case 0xbf: // CP A
			return void(sub8(a, a));
		case 0xc0: // RET NZ
			return ret((f & 0x40) == 0);
		case 0xc1: // POP BC
			return split(c, b, pop16());
		case 0xc2: // JP NZ,nn
			return jp((f & 0x40) == 0);
		case 0xc3: // JP nn
			return jp(true);
		case 0xc4: // CALL NZ,nn
			return call((f & 0x40) == 0);
		case 0xc5: // PUSH BC
			return push16(c | b << 8);
		case 0xc6: // ADD A,n
			return void(a = add8(a, fetch()));
		case 0xc7: // RST 00h
			return rst(0x00);
		case 0xc8: // RET Z
			return ret((f & 0x40) != 0);
		case 0xc9: // RET
			return ret(true);
		case 0xca: // JP Z,nn
			return jp((f & 0x40) != 0);
		case 0xcb:
			return execute_cb();
		case 0xcc: // CALL Z,nn
			return call((f & 0x40) != 0);
		case 0xcd: // CALL nn
			return call(true);
		case 0xce: // ADC A,n
			return void(a = adc8(a, fetch()));
		case 0xcf: // RST 08h
			return rst(0x08);
		case 0xd0: // RET NC
			return ret((f & 1) == 0);
		case 0xd1: // POP DE
			return split(e, d, pop16());
		case 0xd2: // JP NC,nn
			return jp((f & 1) == 0);
		case 0xd3: // OUT n,A
			return iowrite(a, fetch(), a);
		case 0xd4: // CALL NC,nn
			return call((f & 1) == 0);
		case 0xd5: // PUSH DE
			return push16(e | d << 8);
		case 0xd6: // SUB n
			return void(a = sub8(a, fetch()));
		case 0xd7: // RST 10h
			return rst(0x10);
		case 0xd8: // RET C
			return ret((f & 1) != 0);
		case 0xd9: // EXX
			return ex(c, b, c_prime, b_prime), ex(e, d, e_prime, d_prime), ex(l, h, l_prime, h_prime);
		case 0xda: // JP C,nn
			return jp((f & 1) != 0);
		case 0xdb: // IN A,n
			return void(a = ioread(a, fetch()));
		case 0xdc: // CALL C,nn
			return call((f & 1) != 0);
		case 0xdd:
			return execute_dd();
		case 0xde: // SBC A,n
			return void(a = sbc8(a, fetch()));
		case 0xdf: // RST 18h
			return rst(0x18);
		case 0xe0: // RET PO
			return ret((f & 4) == 0);
		case 0xe1: // POP HL
			return split(l, h, pop16());
		case 0xe2: // JP PO,nn
			return jp((f & 4) == 0);
		case 0xe3: // EX (SP),HL
			return v = l | h << 8, split(l, h, pop16()), push16(v);
		case 0xe4: // CALL PO,nn
			return call((f & 4) == 0);
		case 0xe5: // PUSH HL
			return push16(l | h << 8);
		case 0xe6: // AND n
			return void(a = and8(a, fetch()));
		case 0xe7: // RST 20h
			return rst(0x20);
		case 0xe8: // RET PE
			return ret((f & 4) != 0);
		case 0xe9: // JP (HL)
			return void(pc = l | h << 8);
		case 0xea: // JP PE,nn
			return jp((f & 4) != 0);
		case 0xeb: // EX DE,HL
			return ex(e, d, l, h);
		case 0xec: // CALL PE,nn
			return call((f & 4) != 0);
		case 0xed:
			return execute_ed();
		case 0xee: // XOR n
			return void(a = xor8(a, fetch()));
		case 0xef: // RST 28h
			return rst(0x28);
		case 0xf0: // RET P
			return ret((f & 0x80) == 0);
		case 0xf1: // POP AF
			return split(f, a, pop16());
		case 0xf2: // JP P,nn
			return jp((f & 0x80) == 0);
		case 0xf3: // DI
			return void(iff = 0);
		case 0xf4: // CALL P,nn
			return call((f & 0x80) == 0);
		case 0xf5: // PUSH AF
			return push16(f | a << 8);
		case 0xf6: // OR n
			return void(a = or8(a, fetch()));
		case 0xf7: // RST 30h
			return rst(0x30);
		case 0xf8: // RET M
			return ret((f & 0x80) != 0);
		case 0xf9: // LD SP,HL
			return void(sp = l | h << 8);
		case 0xfa: // JP M,nn
			return jp((f & 0x80) != 0);
		case 0xfb: // EI
			return void(iff = 3);
		case 0xfc: // CALL M,nn
			return call((f & 0x80) != 0);
		case 0xfd:
			return execute_fd();
		case 0xfe: // CP A,n
			return void(sub8(a, fetch()));
		case 0xff: // RST 38h
			return rst(0x38);
		}
	}

	void execute_cb() {
		int v;

		switch (fetch()) {
		case 0x00: // RLC B
			return void(b = rlc8(b));
		case 0x01: // RLC C
			return void(c = rlc8(c));
		case 0x02: // RLC D
			return void(d = rlc8(d));
		case 0x03: // RLC E
			return void(e = rlc8(e));
		case 0x04: // RLC H
			return void(h = rlc8(h));
		case 0x05: // RLC L
			return void(l = rlc8(l));
		case 0x06: // RLC (HL)
			return v = l | h << 8, write(v, rlc8(read(v)));
		case 0x07: // RLC A
			return void(a = rlc8(a));
		case 0x08: // RRC B
			return void(b = rrc8(b));
		case 0x09: // RRC C
			return void(c = rrc8(c));
		case 0x0a: // RRC D
			return void(d = rrc8(d));
		case 0x0b: // RRC E
			return void(e = rrc8(e));
		case 0x0c: // RRC H
			return void(h = rrc8(h));
		case 0x0d: // RRC L
			return void(l = rrc8(l));
		case 0x0e: // RRC (HL)
			return v = l | h << 8, write(v, rrc8(read(v)));
		case 0x0f: // RRC A
			return void(a = rrc8(a));
		case 0x10: // RL B
			return void(b = rl8(b));
		case 0x11: // RL C
			return void(c = rl8(c));
		case 0x12: // RL D
			return void(d = rl8(d));
		case 0x13: // RL E
			return void(e = rl8(e));
		case 0x14: // RL H
			return void(h = rl8(h));
		case 0x15: // RL L
			return void(l = rl8(l));
		case 0x16: // RL (HL)
			return v = l | h << 8, write(v, rl8(read(v)));
		case 0x17: // RL A
			return void(a = rl8(a));
		case 0x18: // RR B
			return void(b = rr8(b));
		case 0x19: // RR C
			return void(c = rr8(c));
		case 0x1a: // RR D
			return void(d = rr8(d));
		case 0x1b: // RR E
			return void(e = rr8(e));
		case 0x1c: // RR H
			return void(h = rr8(h));
		case 0x1d: // RR L
			return void(l = rr8(l));
		case 0x1e: // RR (HL)
			return v = l | h << 8, write(v, rr8(read(v)));
		case 0x1f: // RR A
			return void(a = rr8(a));
		case 0x20: // SLA B
			return void(b = sla8(b));
		case 0x21: // SLA C
			return void(c = sla8(c));
		case 0x22: // SLA D
			return void(d = sla8(d));
		case 0x23: // SLA E
			return void(e = sla8(e));
		case 0x24: // SLA H
			return void(h = sla8(h));
		case 0x25: // SLA L
			return void(l = sla8(l));
		case 0x26: // SLA (HL)
			return v = l | h << 8, write(v, sla8(read(v)));
		case 0x27: // SLA A
			return void(a = sla8(a));
		case 0x28: // SRA B
			return void(b = sra8(b));
		case 0x29: // SRA C
			return void(c = sra8(c));
		case 0x2a: // SRA D
			return void(d = sra8(d));
		case 0x2b: // SRA E
			return void(e = sra8(e));
		case 0x2c: // SRA H
			return void(h = sra8(h));
		case 0x2d: // SRA L
			return void(l = sra8(l));
		case 0x2e: // SRA (HL)
			return v = l | h << 8, write(v, sra8(read(v)));
		case 0x2f: // SRA A
			return void(a = sra8(a));
		case 0x38: // SRL B
			return void(b = srl8(b));
		case 0x39: // SRL C
			return void(c = srl8(c));
		case 0x3a: // SRL D
			return void(d = srl8(d));
		case 0x3b: // SRL E
			return void(e = srl8(e));
		case 0x3c: // SRL H
			return void(h = srl8(h));
		case 0x3d: // SRL L
			return void(l = srl8(l));
		case 0x3e: // SRL (HL)
			return v = l | h << 8, write(v, srl8(read(v)));
		case 0x3f: // SRL A
			return void(a = srl8(a));
		case 0x40: // BIT 0,B
			return bit8(0, b);
		case 0x41: // BIT 0,C
			return bit8(0, c);
		case 0x42: // BIT 0,D
			return bit8(0, d);
		case 0x43: // BIT 0,E
			return bit8(0, e);
		case 0x44: // BIT 0,H
			return bit8(0, h);
		case 0x45: // BIT 0,L
			return bit8(0, l);
		case 0x46: // BIT 0,(HL)
			return bit8(0, read(l | h << 8));
		case 0x47: // BIT 0,A
			return bit8(0, a);
		case 0x48: // BIT 1,B
			return bit8(1, b);
		case 0x49: // BIT 1,C
			return bit8(1, c);
		case 0x4a: // BIT 1,D
			return bit8(1, d);
		case 0x4b: // BIT 1,E
			return bit8(1, e);
		case 0x4c: // BIT 1,H
			return bit8(1, h);
		case 0x4d: // BIT 1,L
			return bit8(1, l);
		case 0x4e: // BIT 1,(HL)
			return bit8(1, read(l | h << 8));
		case 0x4f: // BIT 1,A
			return bit8(1, a);
		case 0x50: // BIT 2,B
			return bit8(2, b);
		case 0x51: // BIT 2,C
			return bit8(2, c);
		case 0x52: // BIT 2,D
			return bit8(2, d);
		case 0x53: // BIT 2,E
			return bit8(2, e);
		case 0x54: // BIT 2,H
			return bit8(2, h);
		case 0x55: // BIT 2,L
			return bit8(2, l);
		case 0x56: // BIT 2,(HL)
			return bit8(2, read(l | h << 8));
		case 0x57: // BIT 2,A
			return bit8(2, a);
		case 0x58: // BIT 3,B
			return bit8(3, b);
		case 0x59: // BIT 3,C
			return bit8(3, c);
		case 0x5a: // BIT 3,D
			return bit8(3, d);
		case 0x5b: // BIT 3,E
			return bit8(3, e);
		case 0x5c: // BIT 3,H
			return bit8(3, h);
		case 0x5d: // BIT 3,L
			return bit8(3, l);
		case 0x5e: // BIT 3,(HL)
			return bit8(3, read(l | h << 8));
		case 0x5f: // BIT 3,A
			return bit8(3, a);
		case 0x60: // BIT 4,B
			return bit8(4, b);
		case 0x61: // BIT 4,C
			return bit8(4, c);
		case 0x62: // BIT 4,D
			return bit8(4, d);
		case 0x63: // BIT 4,E
			return bit8(4, e);
		case 0x64: // BIT 4,H
			return bit8(4, h);
		case 0x65: // BIT 4,L
			return bit8(4, l);
		case 0x66: // BIT 4,(HL)
			return bit8(4, read(l | h << 8));
		case 0x67: // BIT 4,A
			return bit8(4, a);
		case 0x68: // BIT 5,B
			return bit8(5, b);
		case 0x69: // BIT 5,C
			return bit8(5, c);
		case 0x6a: // BIT 5,D
			return bit8(5, d);
		case 0x6b: // BIT 5,E
			return bit8(5, e);
		case 0x6c: // BIT 5,H
			return bit8(5, h);
		case 0x6d: // BIT 5,L
			return bit8(5, l);
		case 0x6e: // BIT 5,(HL)
			return bit8(5, read(l | h << 8));
		case 0x6f: // BIT 5,A
			return bit8(5, a);
		case 0x70: // BIT 6,B
			return bit8(6, b);
		case 0x71: // BIT 6,C
			return bit8(6, c);
		case 0x72: // BIT 6,D
			return bit8(6, d);
		case 0x73: // BIT 6,E
			return bit8(6, e);
		case 0x74: // BIT 6,H
			return bit8(6, h);
		case 0x75: // BIT 6,L
			return bit8(6, l);
		case 0x76: // BIT 6,(HL)
			return bit8(6, read(l | h << 8));
		case 0x77: // BIT 6,A
			return bit8(6, a);
		case 0x78: // BIT 7,B
			return bit8(7, b);
		case 0x79: // BIT 7,C
			return bit8(7, c);
		case 0x7a: // BIT 7,D
			return bit8(7, d);
		case 0x7b: // BIT 7,E
			return bit8(7, e);
		case 0x7c: // BIT 7,H
			return bit8(7, h);
		case 0x7d: // BIT 7,L
			return bit8(7, l);
		case 0x7e: // BIT 7,(HL)
			return bit8(7, read(l | h << 8));
		case 0x7f: // BIT 7,A
			return bit8(7, a);
		case 0x80: // RES 0,B
			return void(b = res8(0, b));
		case 0x81: // RES 0,C
			return void(c = res8(0, c));
		case 0x82: // RES 0,D
			return void(d = res8(0, d));
		case 0x83: // RES 0,E
			return void(e = res8(0, e));
		case 0x84: // RES 0,H
			return void(h = res8(0, h));
		case 0x85: // RES 0,L
			return void(l = res8(0, l));
		case 0x86: // RES 0,(HL)
			return v = l | h << 8, write(v, res8(0, read(v)));
		case 0x87: // RES 0,A
			return void(a = res8(0, a));
		case 0x88: // RES 1,B
			return void(b = res8(1, b));
		case 0x89: // RES 1,C
			return void(c = res8(1, c));
		case 0x8a: // RES 1,D
			return void(d = res8(1, d));
		case 0x8b: // RES 1,E
			return void(e = res8(1, e));
		case 0x8c: // RES 1,H
			return void(h = res8(1, h));
		case 0x8d: // RES 1,L
			return void(l = res8(1, l));
		case 0x8e: // RES 1,(HL)
			return v = l | h << 8, write(v, res8(1, read(v)));
		case 0x8f: // RES 1,A
			return void(a = res8(1, a));
		case 0x90: // RES 2,B
			return void(b = res8(2, b));
		case 0x91: // RES 2,C
			return void(c = res8(2, c));
		case 0x92: // RES 2,D
			return void(d = res8(2, d));
		case 0x93: // RES 2,E
			return void(e = res8(2, e));
		case 0x94: // RES 2,H
			return void(h = res8(2, h));
		case 0x95: // RES 2,L
			return void(l = res8(2, l));
		case 0x96: // RES 2,(HL)
			return v = l | h << 8, write(v, res8(2, read(v)));
		case 0x97: // RES 2,A
			return void(a = res8(2, a));
		case 0x98: // RES 3,B
			return void(b = res8(3, b));
		case 0x99: // RES 3,C
			return void(c = res8(3, c));
		case 0x9a: // RES 3,D
			return void(d = res8(3, d));
		case 0x9b: // RES 3,E
			return void(e = res8(3, e));
		case 0x9c: // RES 3,H
			return void(h = res8(3, h));
		case 0x9d: // RES 3,L
			return void(l = res8(3, l));
		case 0x9e: // RES 3,(HL)
			return v = l | h << 8, write(v, res8(3, read(v)));
		case 0x9f: // RES 3,A
			return void(a = res8(3, a));
		case 0xa0: // RES 4,B
			return void(b = res8(4, b));
		case 0xa1: // RES 4,C
			return void(c = res8(4, c));
		case 0xa2: // RES 4,D
			return void(d = res8(4, d));
		case 0xa3: // RES 4,E
			return void(e = res8(4, e));
		case 0xa4: // RES 4,H
			return void(h = res8(4, h));
		case 0xa5: // RES 4,L
			return void(l = res8(4, l));
		case 0xa6: // RES 4,(HL)
			return v = l | h << 8, write(v, res8(4, read(v)));
		case 0xa7: // RES 4,A
			return void(a = res8(4, a));
		case 0xa8: // RES 5,B
			return void(b = res8(5, b));
		case 0xa9: // RES 5,C
			return void(c = res8(5, c));
		case 0xaa: // RES 5,D
			return void(d = res8(5, d));
		case 0xab: // RES 5,E
			return void(e = res8(5, e));
		case 0xac: // RES 5,H
			return void(h = res8(5, h));
		case 0xad: // RES 5,L
			return void(l = res8(5, l));
		case 0xae: // RES 5,(HL)
			return v = l | h << 8, write(v, res8(5, read(v)));
		case 0xaf: // RES 5,A
			return void(a = res8(5, a));
		case 0xb0: // RES 6,B
			return void(b = res8(6, b));
		case 0xb1: // RES 6,C
			return void(c = res8(6, c));
		case 0xb2: // RES 6,D
			return void(d = res8(6, d));
		case 0xb3: // RES 6,E
			return void(e = res8(6, e));
		case 0xb4: // RES 6,H
			return void(h = res8(6, h));
		case 0xb5: // RES 6,L
			return void(l = res8(6, l));
		case 0xb6: // RES 6,(HL)
			return v = l | h << 8, write(v, res8(6, read(v)));
		case 0xb7: // RES 6,A
			return void(a = res8(6, a));
		case 0xb8: // RES 7,B
			return void(b = res8(7, b));
		case 0xb9: // RES 7,C
			return void(c = res8(7, c));
		case 0xba: // RES 7,D
			return void(d = res8(7, d));
		case 0xbb: // RES 7,E
			return void(e = res8(7, e));
		case 0xbc: // RES 7,H
			return void(h = res8(7, h));
		case 0xbd: // RES 7,L
			return void(l = res8(7, l));
		case 0xbe: // RES 7,(HL)
			return v = l | h << 8, write(v, res8(7, read(v)));
		case 0xbf: // RES 7,A
			return void(a = res8(7, a));
		case 0xc0: // SET 0,B
			return void(b = set8(0, b));
		case 0xc1: // SET 0,C
			return void(c = set8(0, c));
		case 0xc2: // SET 0,D
			return void(d = set8(0, d));
		case 0xc3: // SET 0,E
			return void(e = set8(0, e));
		case 0xc4: // SET 0,H
			return void(h = set8(0, h));
		case 0xc5: // SET 0,L
			return void(l = set8(0, l));
		case 0xc6: // SET 0,(HL)
			return v = l | h << 8, write(v, set8(0, read(v)));
		case 0xc7: // SET 0,A
			return void(a = set8(0, a));
		case 0xc8: // SET 1,B
			return void(b = set8(1, b));
		case 0xc9: // SET 1,C
			return void(c = set8(1, c));
		case 0xca: // SET 1,D
			return void(d = set8(1, d));
		case 0xcb: // SET 1,E
			return void(e = set8(1, e));
		case 0xcc: // SET 1,H
			return void(h = set8(1, h));
		case 0xcd: // SET 1,L
			return void(l = set8(1, l));
		case 0xce: // SET 1,(HL)
			return v = l | h << 8, write(v, set8(1, read(v)));
		case 0xcf: // SET 1,A
			return void(a = set8(1, a));
		case 0xd0: // SET 2,B
			return void(b = set8(2, b));
		case 0xd1: // SET 2,C
			return void(c = set8(2, c));
		case 0xd2: // SET 2,D
			return void(d = set8(2, d));
		case 0xd3: // SET 2,E
			return void(e = set8(2, e));
		case 0xd4: // SET 2,H
			return void(h = set8(2, h));
		case 0xd5: // SET 2,L
			return void(l = set8(2, l));
		case 0xd6: // SET 2,(HL)
			return v = l | h << 8, write(v, set8(2, read(v)));
		case 0xd7: // SET 2,A
			return void(a = set8(2, a));
		case 0xd8: // SET 3,B
			return void(b = set8(3, b));
		case 0xd9: // SET 3,C
			return void(c = set8(3, c));
		case 0xda: // SET 3,D
			return void(d = set8(3, d));
		case 0xdb: // SET 3,E
			return void(e = set8(3, e));
		case 0xdc: // SET 3,H
			return void(h = set8(3, h));
		case 0xdd: // SET 3,L
			return void(l = set8(3, l));
		case 0xde: // SET 3,(HL)
			return v = l | h << 8, write(v, set8(3, read(v)));
		case 0xdf: // SET 3,A
			return void(a = set8(3, a));
		case 0xe0: // SET 4,B
			return void(b = set8(4, b));
		case 0xe1: // SET 4,C
			return void(c = set8(4, c));
		case 0xe2: // SET 4,D
			return void(d = set8(4, d));
		case 0xe3: // SET 4,E
			return void(e = set8(4, e));
		case 0xe4: // SET 4,H
			return void(h = set8(4, h));
		case 0xe5: // SET 4,L
			return void(l = set8(4, l));
		case 0xe6: // SET 4,(HL)
			return v = l | h << 8, write(v, set8(4, read(v)));
		case 0xe7: // SET 4,A
			return void(a = set8(4, a));
		case 0xe8: // SET 5,B
			return void(b = set8(5, b));
		case 0xe9: // SET 5,C
			return void(c = set8(5, c));
		case 0xea: // SET 5,D
			return void(d = set8(5, d));
		case 0xeb: // SET 5,E
			return void(e = set8(5, e));
		case 0xec: // SET 5,H
			return void(h = set8(5, h));
		case 0xed: // SET 5,L
			return void(l = set8(5, l));
		case 0xee: // SET 5,(HL)
			return v = l | h << 8, write(v, set8(5, read(v)));
		case 0xef: // SET 5,A
			return void(a = set8(5, a));
		case 0xf0: // SET 6,B
			return void(b = set8(6, b));
		case 0xf1: // SET 6,C
			return void(c = set8(6, c));
		case 0xf2: // SET 6,D
			return void(d = set8(6, d));
		case 0xf3: // SET 6,E
			return void(e = set8(6, e));
		case 0xf4: // SET 6,H
			return void(h = set8(6, h));
		case 0xf5: // SET 6,L
			return void(l = set8(6, l));
		case 0xf6: // SET 6,(HL)
			return v = l | h << 8, write(v, set8(6, read(v)));
		case 0xf7: // SET 6,A
			return void(a = set8(6, a));
		case 0xf8: // SET 7,B
			return void(b = set8(7, b));
		case 0xf9: // SET 7,C
			return void(c = set8(7, c));
		case 0xfa: // SET 7,D
			return void(d = set8(7, d));
		case 0xfb: // SET 7,E
			return void(e = set8(7, e));
		case 0xfc: // SET 7,H
			return void(h = set8(7, h));
		case 0xfd: // SET 7,L
			return void(l = set8(7, l));
		case 0xfe: // SET 7,(HL)
			return v = l | h << 8, write(v, set8(7, read(v)));
		case 0xff: // SET 7,A
			return void(a = set8(7, a));
		default:
			undefsize = 2;
			if (undef)
				undef(pc);
			return;
		}
	}

	void execute_dd() {
		int v;

		switch (fetch()) {
		case 0x09: // ADD IX,BC
			return split(ixl, ixh, add16(ixl | ixh << 8, c | b << 8));
		case 0x19: // ADD IX,DE
			return split(ixl, ixh, add16(ixl | ixh << 8, e | d << 8));
		case 0x21: // LD IX,nn
			return split(ixl, ixh, fetch16());
		case 0x22: // LD (nn),IX
			return write16(fetch16(), ixl | ixh << 8);
		case 0x23: // INC IX
			return split(ixl, ixh, (ixl | ixh << 8) + 1 & 0xffff);
		case 0x24: // INC IXH (undefined operation)
			return void(ixh = inc8(ixh));
		case 0x25: // DEC IXH (undefined operation)
			return void(ixh = dec8(ixh));
		case 0x26: // LD IXH,n (undefined operation)
			return void(ixh = fetch());
		case 0x29: // ADD IX,IX
			return split(ixl, ixh, add16(ixl | ixh << 8, ixl | ixh << 8));
		case 0x2a: // LD IX,(nn)
			return split(ixl, ixh, read16(fetch16()));
		case 0x2b: // DEC IX
			return split(ixl, ixh, (ixl | ixh << 8) - 1 & 0xffff);
		case 0x2c: // INC IXL (undefined operation)
			return void(ixl = inc8(ixl));
		case 0x2d: // DEC IXL (undefined operation)
			return void(ixl = dec8(ixl));
		case 0x2e: // LD IXL,n (undefined operation)
			return void(ixl = fetch());
		case 0x34: // INC (IX+d)
			return v = disp(ixh, ixl), write(v, inc8(read(v)));
		case 0x35: // DEC (IX+d)
			return v = disp(ixh, ixl), write(v, dec8(read(v)));
		case 0x36: // LD (IX+d),n
			return write(disp(ixh, ixl), fetch());
		case 0x39: // ADD IX,SP
			return split(ixl, ixh, add16(ixl | ixh << 8, sp));
		case 0x44: // LD B,IXH (undefined operation)
			return void(b = ixh);
		case 0x45: // LD B,IXL (undefined operation)
			return void(b = ixl);
		case 0x46: // LD B,(IX+d)
			return void(b = read(disp(ixh, ixl)));
		case 0x4c: // LD C,IXH (undefined operation)
			return void(c = ixh);
		case 0x4d: // LD C,IXL (undefined operation)
			return void(c = ixl);
		case 0x4e: // LD C,(IX+d)
			return void(c = read(disp(ixh, ixl)));
		case 0x54: // LD D,IXH (undefined operation)
			return void(d = ixh);
		case 0x55: // LD D,IXL (undefined operation)
			return void(d = ixl);
		case 0x56: // LD D,(IX+d)
			return void(d = read(disp(ixh, ixl)));
		case 0x5c: // LD E,IXH (undefined operation)
			return void(e = ixh);
		case 0x5d: // LD E,IXL (undefined operation)
			return void(e = ixl);
		case 0x5e: // LD E,(IX+d)
			return void(e = read(disp(ixh, ixl)));
		case 0x60: // LD IXH,B (undefined operation)
			return void(ixh = b);
		case 0x61: // LD IXH,C (undefined operation)
			return void(ixh = c);
		case 0x62: // LD IXH,D (undefined operation)
			return void(ixh = d);
		case 0x63: // LD IXH,E (undefined operation)
			return void(ixh = e);
		case 0x66: // LD H,(IX+d)
			return void(h = read(disp(ixh, ixl)));
		case 0x67: // LD IXH,A (undefined operation)
			return void(ixh = a);
		case 0x68: // LD IXL,B (undefined operation)
			return void(ixl = b);
		case 0x69: // LD IXL,C (undefined operation)
			return void(ixl = c);
		case 0x6a: // LD IXL,D (undefined operation)
			return void(ixl = d);
		case 0x6b: // LD IXL,E (undefined operation)
			return void(ixl = e);
		case 0x6e: // LD L,(IX+d)
			return void(l = read(disp(ixh, ixl)));
		case 0x6f: // LD IXL,A (undefined operation)
			return void(ixl = a);
		case 0x70: // LD (IX+d),B
			return write(disp(ixh, ixl), b);
		case 0x71: // LD (IX+d),C
			return write(disp(ixh, ixl), c);
		case 0x72: // LD (IX+d),D
			return write(disp(ixh, ixl), d);
		case 0x73: // LD (IX+d),E
			return write(disp(ixh, ixl), e);
		case 0x74: // LD (IX+d),H
			return write(disp(ixh, ixl), h);
		case 0x75: // LD (IX+d),L
			return write(disp(ixh, ixl), l);
		case 0x77: // LD (IX+d),A
			return write(disp(ixh, ixl), a);
		case 0x7c: // LD A,IXH (undefined operation)
			return void(a = ixh);
		case 0x7d: // LD A,IXL (undefined operation)
			return void(a = ixl);
		case 0x7e: // LD A,(IX+d)
			return void(a = read(disp(ixh, ixl)));
		case 0x84: // ADD A,IXH (undefined operation)
			return void(a = add8(a, ixh));
		case 0x85: // ADD A,IXL (undefined operation)
			return void(a = add8(a, ixl));
		case 0x86: // ADD A,(IX+d)
			return void(a = add8(a, read(disp(ixh, ixl))));
		case 0x8c: // ADC A,IXH (undefined operation)
			return void(a = adc8(a, ixh));
		case 0x8d: // ADC A,IXL (undefined operation)
			return void(a = adc8(a, ixl));
		case 0x8e: // ADC A,(IX+d)
			return void(a = adc8(a, read(disp(ixh, ixl))));
		case 0x94: // SUB IXH (undefined operation)
			return void(a = sub8(a, ixh));
		case 0x95: // SUB IXL (undefined operation)
			return void(a = sub8(a, ixl));
		case 0x96: // SUB (IX+d)
			return void(a = sub8(a, read(disp(ixh, ixl))));
		case 0x9c: // SBC A,IXH (undefined operation)
			return void(a = sbc8(a, ixh));
		case 0x9d: // SBC A,IXL (undefined operation)
			return void(a = sbc8(a, ixl));
		case 0x9e: // SBC A,(IX+d)
			return void(a = sbc8(a, read(disp(ixh, ixl))));
		case 0xa4: // AND IXH (undefined operation)
			return void(a = and8(a, ixh));
		case 0xa5: // AND IXL (undefined operation)
			return void(a = and8(a, ixl));
		case 0xa6: // AND (IX+d)
			return void(a = and8(a, read(disp(ixh, ixl))));
		case 0xac: // XOR IXH (undefined operation)
			return void(a = xor8(a, ixh));
		case 0xad: // XOR IXL (undefined operation)
			return void(a = xor8(a, ixl));
		case 0xae: // XOR (IX+d)
			return void(a = xor8(a, read(disp(ixh, ixl))));
		case 0xb4: // OR IXH (undefined operation)
			return void(a = or8(a, ixh));
		case 0xb5: // OR IXL (undefined operation)
			return void(a = or8(a, ixl));
		case 0xb6: // OR (IX+d)
			return void(a = or8(a, read(disp(ixh, ixl))));
		case 0xbc: // CP IXH (undefined operation)
			return void(sub8(a, ixh));
		case 0xbd: // CP IXL (undefined operation)
			return void(sub8(a, ixl));
		case 0xbe: // CP (IX+d)
			return void(sub8(a, read(disp(ixh, ixl))));
		case 0xcb:
			v = disp(ixh, ixl);
			switch (fetch()) {
			case 0x06: // RLC (IX+d)
				return write(v, rlc8(read(v)));
			case 0x0e: // RRC (IX+d)
				return write(v, rrc8(read(v)));
			case 0x16: // RL (IX+d)
				return write(v, rl8(read(v)));
			case 0x1e: // RR (IX+d)
				return write(v, rr8(read(v)));
			case 0x26: // SLA (IX+d)
				return write(v, sla8(read(v)));
			case 0x2e: // SRA (IX+d)
				return write(v, sra8(read(v)));
			case 0x3e: // SRL (IX+d)
				return write(v, srl8(read(v)));
			case 0x46: // BIT 0,(IX+d)
				return bit8(0, read(v));
			case 0x4e: // BIT 1,(IX+d)
				return bit8(1, read(v));
			case 0x56: // BIT 2,(IX+d)
				return bit8(2, read(v));
			case 0x5e: // BIT 3,(IX+d)
				return bit8(3, read(v));
			case 0x66: // BIT 4,(IX+d)
				return bit8(4, read(v));
			case 0x6e: // BIT 5,(IX+d)
				return bit8(5, read(v));
			case 0x76: // BIT 6,(IX+d)
				return bit8(6, read(v));
			case 0x7e: // BIT 7,(IX+d)
				return bit8(7, read(v));
			case 0x86: // RES 0,(IX+d)
				return write(v, res8(0, read(v)));
			case 0x8e: // RES 1,(IX+d)
				return write(v, res8(1, read(v)));
			case 0x96: // RES 2,(IX+d)
				return write(v, res8(2, read(v)));
			case 0x9e: // RES 3,(IX+d)
				return write(v, res8(3, read(v)));
			case 0xa6: // RES 4,(IX+d)
				return write(v, res8(4, read(v)));
			case 0xae: // RES 5,(IX+d)
				return write(v, res8(5, read(v)));
			case 0xb6: // RES 6,(IX+d)
				return write(v, res8(6, read(v)));
			case 0xbe: // RES 7,(IX+d)
				return write(v, res8(7, read(v)));
			case 0xc6: // SET 0,(IX+d)
				return write(v, set8(0, read(v)));
			case 0xce: // SET 1,(IX+d)
				return write(v, set8(1, read(v)));
			case 0xd6: // SET 2,(IX+d)
				return write(v, set8(2, read(v)));
			case 0xde: // SET 3,(IX+d)
				return write(v, set8(3, read(v)));
			case 0xe6: // SET 4,(IX+d)
				return write(v, set8(4, read(v)));
			case 0xee: // SET 5,(IX+d)
				return write(v, set8(5, read(v)));
			case 0xf6: // SET 6,(IX+d)
				return write(v, set8(6, read(v)));
			case 0xfe: // SET 7,(IX+d)
				return write(v, set8(7, read(v)));
			default:
				undefsize = 4;
				if (undef)
					undef(pc);
				return;
			}
		case 0xe1: // POP IX
			return split(ixl, ixh, pop16());
		case 0xe3: // EX (SP),IX
			return v = ixl | ixh << 8, split(ixl, ixh, pop16()), push16(v);
		case 0xe5: // PUSH IX
			return push16(ixl | ixh << 8);
		case 0xe9: // JP (IX)
			return void(pc = ixl | ixh << 8);
		case 0xf9: // LD SP,IX
			return void(sp = ixl | ixh << 8);
		default:
			undefsize = 2;
			if (undef)
				undef(pc);
			return;
		}
	}

	void execute_ed() {
		int v;

		switch (fetch()) {
		case 0x40: // IN B,(C)
			return void(f = f & ~0xd6 | fLogic[b = ioread(b, c)]);
		case 0x41: // OUT (C),B
			return iowrite(b, c, b);
		case 0x42: // SBC HL,BC
			return split(l, h, sbc16(l | h << 8, c | b << 8));
		case 0x43: // LD (nn),BC
			return write16(fetch16(), c | b << 8);
		case 0x44: // NEG
			return void(a = neg8(a));
		case 0x45: // RETN
			return iff = (iff & 2) != 0 ? 3 : 0, ret(true);
		case 0x46: // IM 0
			return void(im = 0);
		case 0x47: // LD I,A
			return void(i = a);
		case 0x48: // IN C,(C)
			return void(f = f & ~0xd6 | fLogic[c = ioread(b, c)]);
		case 0x49: // OUT (C),C
			return iowrite(b, c, c);
		case 0x4a: // ADC HL,BC
			return split(l, h, adc16(l | h << 8, c | b << 8));
		case 0x4b: // LD BC,(nn)
			return split(c, b, read16(fetch16()));
		case 0x4d: // RETI
			return ret(true);
		case 0x4f: // LD R,A
			return void(r = a & 0x7f);
		case 0x50: // IN D,(C)
			return void(f = f & ~0xd6 | fLogic[d = ioread(b, c)]);
		case 0x51: // OUT (C),D
			return iowrite(b, c, d);
		case 0x52: // SBC HL,DE
			return split(l, h, sbc16(l | h << 8, e | d << 8));
		case 0x53: // LD (nn),DE
			return write16(fetch16(), e | d << 8);
		case 0x56: // IM 1
			return void(im = 1);
		case 0x57: // LD A,I
			return void(f = f & ~0xd6 | fLogic[a = i] & 0xc0 | iff << 1 & 4);
		case 0x58: // IN E,(C)
			return void(f = f & ~0xd6 | fLogic[e = ioread(b, c)]);
		case 0x59: // OUT (C),E
			return iowrite(b, c, e);
		case 0x5a: // ADC HL,DE
			return split(l, h, adc16(l | h << 8, e | d << 8));
		case 0x5b: // LD DE,(nn)
			return split(e, d, read16(fetch16()));
		case 0x5e: // IM 2
			return void(im = 2);
		case 0x5f: // LD A,R
			return void(f = f & ~0xd6 | fLogic[a = r] & 0xc0 | iff << 1 & 4);
		case 0x60: // IN H,(C)
			return void(f = f & ~0xd6 | fLogic[h = ioread(b, c)]);
		case 0x61: // OUT (C),H
			return iowrite(b, c, h);
		case 0x62: // SBC HL,HL
			return split(l, h, sbc16(l | h << 8, l | h << 8));
		case 0x67: // RRD
			return v = read(l | h << 8) | a << 8, write(l | h << 8, v >> 4 & 0xff), void(f = f & ~0xd6 | fLogic[a = a & 0xf0 | v & 0x0f]);
		case 0x68: // IN L,(C)
			return void(f = f & ~0xd6 | fLogic[l = ioread(b, c)]);
		case 0x69: // OUT (C),L
			return iowrite(b, c, l);
		case 0x6a: // ADC HL,HL
			return split(l, h, adc16(l | h << 8, l | h << 8));
		case 0x6f: // RLD
			return v = a & 0x0f | read(l | h << 8) << 4, write(l | h << 8, v & 0xff), void(f = f & ~0xd6 | fLogic[a = a & 0xf0 | v >> 8]);
		case 0x72: // SBC HL,SP
			return split(l, h, sbc16(l | h << 8, sp));
		case 0x73: // LD (nn),SP
			return write16(fetch16(), sp);
		case 0x78: // IN A,(C)
			return void(f = f & ~0xd6 | fLogic[a = ioread(b, c)]);
		case 0x79: // OUT (C),A
			return iowrite(b, c, a);
		case 0x7a: // ADC HL,SP
			return split(l, h, adc16(l | h << 8, sp));
		case 0x7b: // LD SP,(nn)
			return void(sp = read16(fetch16()));
		case 0xa0: // LDI
			return ldi();
		case 0xa1: // CPI
			return cpi();
		case 0xa2: // INI
			return ini();
		case 0xa3: // OUTI
			return outi();
		case 0xa8: // LDD
			return ldd();
		case 0xa9: // CPD
			return cpd();
		case 0xaa: // IND
			return ind();
		case 0xab: // OUTD
			return outd();
		case 0xb0: // LDIR
			return ldi(), void((f & 4) != 0 && (pc = pc - 2 & 0xffff));
		case 0xb1: // CPIR
			return cpi(), void((f & 0x44) == 4 && (pc = pc - 2 & 0xffff));
		case 0xb2: // INIR
			return ini(), void((f & 0x40) == 0 && (pc = pc - 2 & 0xffff));
		case 0xb3: // OTIR
			return outi(), void((f & 0x40) == 0 && (pc = pc - 2 & 0xffff));
		case 0xb8: // LDDR
			return ldd(), void((f & 4) != 0 && (pc = pc - 2 & 0xffff));
		case 0xb9: // CPDR
			return cpd(), void((f & 0x44) == 4 && (pc = pc - 2 & 0xffff));
		case 0xba: // INDR
			return ind(), void((f & 0x40) == 0 && (pc = pc - 2 & 0xffff));
		case 0xbb: // OTDR
			return outd(), void((f & 0x40) == 0 && (pc = pc - 2 & 0xffff));
		default:
			undefsize = 2;
			if (undef)
				undef(pc);
			return;
		}
	}

	void execute_fd() {
		int v;

		switch (fetch()) {
		case 0x09: // ADD IY,BC
			return split(iyl, iyh, add16(iyl | iyh << 8, c | b << 8));
		case 0x19: // ADD IY,DE
			return split(iyl, iyh, add16(iyl | iyh << 8, e | d << 8));
		case 0x21: // LD IY,nn
			return split(iyl, iyh, fetch16());
		case 0x22: // LD (nn),IY
			return write16(fetch16(), iyl | iyh << 8);
		case 0x23: // INC IY
			return split(iyl, iyh, (iyl | iyh << 8) + 1 & 0xffff);
		case 0x24: // INC IYH (undefined operation)
			return void(iyh = inc8(iyh));
		case 0x25: // DEC IYH (undefined operation)
			return void(iyh = dec8(iyh));
		case 0x26: // LD IYH,n (undefined operation)
			return void(iyh = fetch());
		case 0x29: // ADD IY,IY
			return split(iyl, iyh, add16(iyl | iyh << 8, iyl | iyh << 8));
		case 0x2a: // LD IY,(nn)
			return split(iyl, iyh, read16(fetch16()));
		case 0x2b: // DEC IY
			return split(iyl, iyh, (iyl | iyh << 8) - 1 & 0xffff);
		case 0x2c: // INC IYL (undefined operation)
			return void(iyl = inc8(iyl));
		case 0x2d: // DEC IYL (undefined operation)
			return void(iyl = dec8(iyl));
		case 0x2e: // LD IYL,n (undefined operation)
			return void(iyl = fetch());
		case 0x34: // INC (IY+d)
			return v = disp(iyh, iyl), write(v, inc8(read(v)));
		case 0x35: // DEC (IY+d)
			return v = disp(iyh, iyl), write(v, dec8(read(v)));
		case 0x36: // LD (IY+d),n
			return write(disp(iyh, iyl), fetch());
		case 0x39: // ADD IY,SP
			return split(iyl, iyh, add16(iyl | iyh << 8, sp));
		case 0x44: // LD B,IYH (undefined operation)
			return void(b = iyh);
		case 0x45: // LD B,IYL (undefined operation)
			return void(b = iyl);
		case 0x46: // LD B,(IY+d)
			return void(b = read(disp(iyh, iyl)));
		case 0x4c: // LD C,IYH (undefined operation)
			return void(c = iyh);
		case 0x4d: // LD C,IYL (undefined operation)
			return void(c = iyl);
		case 0x4e: // LD C,(IY+d)
			return void(c = read(disp(iyh, iyl)));
		case 0x54: // LD D,IYH (undefined operation)
			return void(d = iyh);
		case 0x55: // LD D,IYL (undefined operation)
			return void(d = iyl);
		case 0x56: // LD D,(IY+d)
			return void(d = read(disp(iyh, iyl)));
		case 0x5c: // LD E,IYH (undefined operation)
			return void(e = iyh);
		case 0x5d: // LD E,IYL (undefined operation)
			return void(e = iyl);
		case 0x5e: // LD E,(IY+d)
			return void(e = read(disp(iyh, iyl)));
		case 0x60: // LD IYH,B (undefined operation)
			return void(iyh = b);
		case 0x61: // LD IYH,C (undefined operation)
			return void(iyh = c);
		case 0x62: // LD IYH,D (undefined operation)
			return void(iyh = d);
		case 0x63: // LD IYH,E (undefined operation)
			return void(iyh = e);
		case 0x66: // LD H,(IY+d)
			return void(h = read(disp(iyh, iyl)));
		case 0x67: // LD IYH,A (undefined operation)
			return void(iyh = a);
		case 0x68: // LD IYL,B (undefined operation)
			return void(iyl = b);
		case 0x69: // LD IYL,C (undefined operation)
			return void(iyl = c);
		case 0x6a: // LD IYL,D (undefined operation)
			return void(iyl = d);
		case 0x6b: // LD IYL,E (undefined operation)
			return void(iyl = e);
		case 0x6e: // LD L,(IY+d)
			return void(l = read(disp(iyh, iyl)));
		case 0x6f: // LD IYL,A (undefined operation)
			return void(iyl = a);
		case 0x70: // LD (IY+d),B
			return write(disp(iyh, iyl), b);
		case 0x71: // LD (IY+d),C
			return write(disp(iyh, iyl), c);
		case 0x72: // LD (IY+d),D
			return write(disp(iyh, iyl), d);
		case 0x73: // LD (IY+d),E
			return write(disp(iyh, iyl), e);
		case 0x74: // LD (IY+d),H
			return write(disp(iyh, iyl), h);
		case 0x75: // LD (IY+d),L
			return write(disp(iyh, iyl), l);
		case 0x77: // LD (IY+d),A
			return write(disp(iyh, iyl), a);
		case 0x7c: // LD A,IYH (undefined operation)
			return void(a = iyh);
		case 0x7d: // LD A,IYL (undefined operation)
			return void(a = iyl);
		case 0x7e: // LD A,(IY+d)
			return void(a = read(disp(iyh, iyl)));
		case 0x84: // ADD A,IYH (undefined operation)
			return void(a = add8(a, iyh));
		case 0x85: // ADD A,IYL (undefined operation)
			return void(a = add8(a, iyl));
		case 0x86: // ADD A,(IY+d)
			return void(a = add8(a, read(disp(iyh, iyl))));
		case 0x8c: // ADC A,IYH (undefined operation)
			return void(a = adc8(a, iyh));
		case 0x8d: // ADC A,IYL (undefined operation)
			return void(a = adc8(a, iyl));
		case 0x8e: // ADC A,(IY+d)
			return void(a = adc8(a, read(disp(iyh, iyl))));
		case 0x94: // SUB IYH (undefined operation)
			return void(a = sub8(a, iyh));
		case 0x95: // SUB IYL (undefined operation)
			return void(a = sub8(a, iyl));
		case 0x96: // SUB (IY+d)
			return void(a = sub8(a, read(disp(iyh, iyl))));
		case 0x9c: // SBC A,IYH (undefined operation)
			return void(a = sbc8(a, iyh));
		case 0x9d: // SBC A,IYL (undefined operation)
			return void(a = sbc8(a, iyl));
		case 0x9e: // SBC A,(IY+d)
			return void(a = sbc8(a, read(disp(iyh, iyl))));
		case 0xa4: // AND IYH (undefined operation)
			return void(a = and8(a, iyh));
		case 0xa5: // AND IYL (undefined operation)
			return void(a = and8(a, iyl));
		case 0xa6: // AND (IY+d)
			return void(a = and8(a, read(disp(iyh, iyl))));
		case 0xac: // XOR IYH (undefined operation)
			return void(a = xor8(a, iyh));
		case 0xad: // XOR IYL (undefined operation)
			return void(a = xor8(a, iyl));
		case 0xae: // XOR (IY+d)
			return void(a = xor8(a, read(disp(iyh, iyl))));
		case 0xb4: // OR IYH (undefined operation)
			return void(a = or8(a, iyh));
		case 0xb5: // OR IYL (undefined operation)
			return void(a = or8(a, iyl));
		case 0xb6: // OR (IY+d)
			return void(a = or8(a, read(disp(iyh, iyl))));
		case 0xbc: // CP IYH (undefined operation)
			return void(sub8(a, iyh));
		case 0xbd: // CP IYL (undefined operation)
			return void(sub8(a, iyl));
		case 0xbe: // CP (IY+d)
			return void(sub8(a, read(disp(iyh, iyl))));
		case 0xcb:
			v = disp(iyh, iyl);
			switch (fetch()) {
			case 0x06: // RLC (IY+d)
				return write(v, rlc8(read(v)));
			case 0x0e: // RRC (IY+d)
				return write(v, rrc8(read(v)));
			case 0x16: // RL (IY+d)
				return write(v, rl8(read(v)));
			case 0x1e: // RR (IY+d)
				return write(v, rr8(read(v)));
			case 0x26: // SLA (IY+d)
				return write(v, sla8(read(v)));
			case 0x2e: // SRA (IY+d)
				return write(v, sra8(read(v)));
			case 0x3e: // SRL (IY+d)
				return write(v, srl8(read(v)));
			case 0x46: // BIT 0,(IY+d)
				return bit8(0, read(v));
			case 0x4e: // BIT 1,(IY+d)
				return bit8(1, read(v));
			case 0x56: // BIT 2,(IY+d)
				return bit8(2, read(v));
			case 0x5e: // BIT 3,(IY+d)
				return bit8(3, read(v));
			case 0x66: // BIT 4,(IY+d)
				return bit8(4, read(v));
			case 0x6e: // BIT 5,(IY+d)
				return bit8(5, read(v));
			case 0x76: // BIT 6,(IY+d)
				return bit8(6, read(v));
			case 0x7e: // BIT 7,(IY+d)
				return bit8(7, read(v));
			case 0x86: // RES 0,(IY+d)
				return write(v, res8(0, read(v)));
			case 0x8e: // RES 1,(IY+d)
				return write(v, res8(1, read(v)));
			case 0x96: // RES 2,(IY+d)
				return write(v, res8(2, read(v)));
			case 0x9e: // RES 3,(IY+d)
				return write(v, res8(3, read(v)));
			case 0xa6: // RES 4,(IY+d)
				return write(v, res8(4, read(v)));
			case 0xae: // RES 5,(IY+d)
				return write(v, res8(5, read(v)));
			case 0xb6: // RES 6,(IY+d)
				return write(v, res8(6, read(v)));
			case 0xbe: // RES 7,(IY+d)
				return write(v, res8(7, read(v)));
			case 0xc6: // SET 0,(IY+d)
				return write(v, set8(0, read(v)));
			case 0xce: // SET 1,(IY+d)
				return write(v, set8(1, read(v)));
			case 0xd6: // SET 2,(IY+d)
				return write(v, set8(2, read(v)));
			case 0xde: // SET 3,(IY+d)
				return write(v, set8(3, read(v)));
			case 0xe6: // SET 4,(IY+d)
				return write(v, set8(4, read(v)));
			case 0xee: // SET 5,(IY+d)
				return write(v, set8(5, read(v)));
			case 0xf6: // SET 6,(IY+d)
				return write(v, set8(6, read(v)));
			case 0xfe: // SET 7,(IY+d)
				return write(v, set8(7, read(v)));
			default:
				undefsize = 4;
				if (undef)
					undef(pc);
				return;
			}
		case 0xe1: // POP IY
			return split(iyl, iyh, pop16());
		case 0xe3: // EX (SP),IY
			return v = iyl | iyh << 8, split(iyl, iyh, pop16()), push16(v);
		case 0xe5: // PUSH IY
			return push16(iyl | iyh << 8);
		case 0xe9: // JP (IY)
			return void(pc = iyl | iyh << 8);
		case 0xf9: // LD SP,IY
			return void(sp = iyl | iyh << 8);
		default:
			undefsize = 2;
			if (undef)
				undef(pc);
			return;
		}
	}

	int disp(int h, int l) {
		const int d = fetch();
		return (l | h << 8) + (d << 24 >> 24) & 0xffff;
	}

	void jr(bool cond) {
		const int d = fetch();
		if (cond) pc = pc + (d << 24 >> 24) & 0xffff;
	}

	void ret(bool cond) {
		if (cond) pc = pop16();
	}

	void jp(bool cond) {
		const int addr = fetch16();
		if (cond) pc = addr;
	}

	void call(bool cond) {
		const int addr = fetch16();
		if (cond) rst(addr);
	}

	void rst(int addr) {
		push16(pc), pc = addr;
	}

	void push16(int r) {
		sp = sp - 1 & 0xffff, write(sp, r >> 8), sp = sp - 1 & 0xffff, write(sp, r & 0xff);
	}

	int pop16() {
		const int r = read16(sp);
		return sp = sp + 2 & 0xffff, r;
	}

	int add8(int dst, int src) {
		const int r = dst + src & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return f = f & ~0xd7 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | c >> 7 & 1, r;
	}

	int add16(int dst, int src) {
		const int r = dst + src & 0xffff, c = dst & src | src & ~r | ~r & dst;
		return f = f & ~13 | c >> 7 & 0x10 | c >> 15 & 1, r;
	}

	int adc8(int dst, int src) {
		const int r = dst + src + (f & 1) & 0xff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return f = f & ~0xd7 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | c >> 7 & 1, r;
	}

	int adc16(int dst, int src) {
		const int r = dst + src + (f & 1) & 0xffff, v = dst & src & ~r | ~dst & ~src & r, c = dst & src | src & ~r | ~r & dst;
		return f = f & ~0xd7 | r >> 8 & 0x80 | !r << 6 | c >> 7 & 0x10 | v >> 13 & 4 | c >> 15 & 1, r;
	}

	int sub8(int dst, int src) {
		const int r = dst - src & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return f = f & ~0xd7 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | 2 | c >> 7 & 1, r;
	}

	int sbc8(int dst, int src) {
		const int r = dst - src - (f & 1) & 0xff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return f = f & ~0xd7 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | 2 | c >> 7 & 1, r;
	}

	int sbc16(int dst, int src) {
		const int r = dst - src - (f & 1) & 0xffff, v = dst & ~src & ~r | ~dst & src & r, c = ~dst & src | src & r | r & ~dst;
		return f = f & ~0xd7 | r >> 8 & 0x80 | !r << 6 | c >> 7 & 0x10 | v >> 13 & 4 | 2 | c >> 15 & 1, r;
	}

	int and8(int dst, int src) {
		const int r = dst & src;
		return f = f & ~0xd7 | fLogic[r] | 0x10, r;
	}

	int xor8(int dst, int src) {
		const int r = dst ^ src;
		return f = f & ~0xd7 | fLogic[r], r;
	}

	int or8(int dst, int src) {
		const int r = dst | src;
		return f = f & ~0xd7 | fLogic[r], r;
	}

	int inc8(int dst) {
		const int r = dst + 1 & 0xff, v = dst & 1 & ~r | ~dst & ~1 & r, c = dst & 1 | 1 & ~r | ~r & dst;
		return f = f & ~0xd6 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4, r;
	}

	int dec8(int dst) {
		const int r = dst - 1 & 0xff, v = dst & ~1 & ~r | ~dst & 1 & r, c = ~dst & 1 | 1 & r | r & ~dst;
		return f = f & ~0xd6 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | 2, r;
	}

	void daa() {
		int r = a;
		if ((f & 2) != 0) {
			if ((f & 0x10) != 0 && (r & 0x0f) > 5 || (r & 0x0f) > 9)
				r -= 6, f |= 0x10;
			if ((f & 1) != 0 && (r & 0xf0) > 0x50 || (r & 0xf0) > 0x90)
				r -= 0x60, f |= 1;
		}
		else {
			if ((f & 0x10) != 0 && (r & 0x0f) < 4 || (r & 0x0f) > 9) {
				if ((r += 6) >= 0x100)
					f |= 1;
				f |= 0x10;
			}
			if ((f & 1) != 0 && (r & 0xf0) < 0x40 || (r & 0xf0) > 0x90)
				r += 0x60, f |= 1;
		}
		a = r &= 0xff, f = f & ~0xc4 | fLogic[r];
	}

	int rlc8(int dst) {
		const int r = dst << 1 & 0xff | dst >> 7, c = dst >> 7;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int rrc8(int dst) {
		const int r = dst >> 1 | dst << 7 & 0x80, c = dst & 1;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int rl8(int dst) {
		const int r = dst << 1 & 0xff | f & 1, c = dst >> 7;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int rr8(int dst) {
		const int r = dst >> 1 | f << 7 & 0x80, c = dst & 1;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int sla8(int dst) {
		const int r = dst << 1 & 0xff, c = dst >> 7;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int sra8(int dst) {
		const int r = dst >> 1 | dst & 0x80, c = dst & 1;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	int srl8(int dst) {
		const int r = dst >> 1, c = dst & 1;
		return f = f & ~0xd7 | fLogic[r] | c, r;
	}

	void bit8(int src, int dst) {
		f = f & ~0x52 | ~dst >> src << 6 & 0x40 | 0x10;
	}

	static int set8(int src, int dst) {
		return dst | 1 << src;
	}

	static int res8(int src, int dst) {
		return dst & ~(1 << src);
	}

	int neg8(int dst) {
		const int r = -dst & 0xff, v = dst & r, c = dst | r;
		return f = f & ~0xd7 | r & 0x80 | !r << 6 | c << 1 & 0x10 | v >> 5 & 4 | 2 | c >> 7 & 1, r;
	}

	void ldi() {
		write(e | d << 8, read(l | h << 8));
		split(e, d, (e | d << 8) + 1 & 0xffff), split(l, h, (l | h << 8) + 1 & 0xffff), split(c, b, (c | b << 8) - 1 & 0xffff);
		f = f & ~0x16 | ((b | c) != 0) << 2;
	}

	void ldd() {
		write(e | d << 8, read(l | h << 8));
		split(e, d, (e | d << 8) - 1 & 0xffff), split(l, h, (l | h << 8) - 1 & 0xffff), split(c, b, (c | b << 8) - 1 & 0xffff);
		f = f & ~0x16 | ((b | c) != 0) << 2;
	}

	void cpi() {
		const int dst = a, src = read(l | h << 8), r = dst - src & 0xff, c = ~dst & src | src & r | r & ~dst;
		split(l, h, (l | h << 8) + 1 & 0xffff);
		split(this->c, b, (this->c | b << 8) - 1 & 0xffff);
		f = f & ~0xd6 | r & 0x80 | !r << 6 | c << 1 & 0x10 | ((b | this->c) != 0) << 2 | 2;
	}

	void cpd() {
		const int dst = a, src = read(l | h << 8), r = dst - src & 0xff, c = ~dst & src | src & r | r & ~dst;
		split(l, h, (l | h << 8) - 1 & 0xffff);
		split(this->c, b, (this->c | b << 8) - 1 & 0xffff);
		f = f & ~0xd6 | r & 0x80 | !r << 6 | c << 1 & 0x10 | ((b | this->c) != 0) << 2 | 2;
	}

	void ini() {
		write(l | h << 8, ioread(b, c)), split(l, h, (l | h << 8) + 1 & 0xffff), f = f & ~0x42 | !(b = b - 1 & 0xff) << 6 | 2;
	}

	void ind() {
		write(l | h << 8, ioread(b, c)), split(l, h, (l | h << 8) - 1 & 0xffff), f = f & ~0x42 | !(b = b - 1 & 0xff) << 6 | 2;
	}

	void outi() {
		iowrite(b, c, read(l | h << 8)), split(l, h, (l | h << 8) + 1 & 0xffff), f = f & ~0x42 | !(b = b - 1 & 0xff) << 6 | 2;
	}

	void outd() {
		iowrite(b, c, read(l | h << 8)), split(l, h, (l | h << 8) - 1 & 0xffff), f = f & ~0x42 | !(b = b - 1 & 0xff) << 6 | 2;
	}

	int ioread(int h, int l) {
		Page& page = iomap[h];
		return !page.read ? page.base[l] : page.read(l | h << 8);
	}

	void iowrite(int h, int l, int data) {
		Page& page = iomap[h];
		!page.write ? void(page.base[l] = data) : page.write(l | h << 8, data);
	}

	static void split(int& l, int& h, int v) {
		l = v & 0xff, h = v >> 8;
	}

	int fetch16() {
		const int data = fetch();
		return data | fetch() << 8;
	}

	int read16(int addr) {
		const int data = read(addr);
		return data | read(addr + 1 & 0xffff) << 8;
	}

	void write16(int addr, int data) {
		write(addr, data & 0xff), write(addr + 1 & 0xffff, data >> 8);
	}

	static void ex(int& l1, int& h1, int& l2, int& h2) {
		int v;
		v = l1, l1 = l2, l2 = v, v = h1, h1 = h2, h2 = v;
	}

	static void init() {
		static bool initialized;
		if (initialized)
			return;
		initialized	= true;
		for (int r = 0; r < 0x100; r++) {
			int p = r ^ r >> 4;
			p ^= p >> 2;
			p ^= p >> 1;
			fLogic[r] = r & 0x80 | !r << 6 | ~p << 2 & 4;
		}
	}
};

#endif //Z80_H
