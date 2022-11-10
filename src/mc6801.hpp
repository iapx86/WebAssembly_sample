/*
 *
 *	MC6801 Emulator
 *
 */

#ifndef MC6801_H
#define MC6801_H

#include "cpu.hpp"

enum {
	MC6801_ICF = 0, MC6801_OCF, MC6801_TOF, MC6801_SCI,
};

struct MC6801 : Cpu {
	static const unsigned char cc[0x100];
	int a = 0;
	int b = 0;
	int ccr = 0; // ccr:11hinzvc
	int x = 0;
	int s = 0;

	MC6801(int clock = 0) : Cpu(clock) {}

	void reset() override {
		Cpu::reset();
		ccr = 0xd0;
		pc = read16(0xfffe);
	}

	bool interrupt() override {
		if (!Cpu::interrupt() || ccr & 0x10)
			return false;
		return cycle -= cc[0x3f], psh16(pc), psh16(x), psh(a), psh(b), psh(ccr), ccr |= 0x10, pc = read16(0xfff8), true;
	}

	bool interrupt(int cause) {
		if (!Cpu::interrupt() || ccr & 0x10)
			return false;
		cycle -= cc[0x3f], psh16(pc), psh16(x), psh(a), psh(b), psh(ccr), ccr |= 0x10;
		switch (cause) {
		default:
			return pc = read16(0xfff8), true;
		case MC6801_ICF:
			return pc = read16(0xfff6), true;
		case MC6801_OCF:
			return pc = read16(0xfff4), true;
		case MC6801_TOF:
			return pc = read16(0xfff2), true;
		case MC6801_SCI:
			return pc = read16(0xfff0), true;
		}
	}

	int non_maskable_interrupt() {
		if (!Cpu::interrupt())
			return false;
		return cycle -= cc[0x3f], psh16(pc), psh16(x), psh(a), psh(b), psh(ccr), ccr |= 0x10, pc = read16(0xfffc), true;
	}

	void _execute() override {
		int v, ea, op = fetch();
		cycle -= cc[op];
		switch (op) {
		case 0x01: // NOP
			return;
		case 0x04: // LSRD
			return split(a, b, lsr16(a << 8 | b));
		case 0x05: // LSLD
			return split(a, b, lsl16(a << 8 | b));
		case 0x06: // TAP
			return void(ccr = a | 0xc0);
		case 0x07: // TPA
			return void(a = ccr);
		case 0x08: // INX
			return void(x = inc16(x));
		case 0x09: // DEX
			return void(x = dec16(x));
		case 0x0a: // CLV
			return void(ccr &= ~2);
		case 0x0b: // SEV
			return void(ccr |= 2);
		case 0x0c: // CLC
			return void(ccr &= ~1);
		case 0x0d: // SEC
			return void(ccr |= 1);
		case 0x0e: // CLI
			return void(ccr &= ~0x10);
		case 0x0f: // SEI
			return void(ccr |= 0x10);
		case 0x10: // SBA
			return void(a = sub8(b, a));
		case 0x11: // CBA
			return void(sub8(b, a));
		case 0x12: // undocumented instruction
		case 0x13: // undocumented instruction
			return void(x = x + read(s + 1 & 0xffff) & 0xffff);
		case 0x16: // TAB
			return void(b = mov8(a));
		case 0x17: // TBA
			return void(a = mov8(b));
		case 0x18: // XGDX
			return v = x, x = a << 8 | b, split(a, b, v);
		case 0x19: // DAA
			return daa();
		case 0x1a: // SLP
			return suspend();
		case 0x1b: // ABA
			return void(a = add8(b, a));
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
		case 0x30: // TSX
			return void(x = s + 1 & 0xffff);
		case 0x31: // INS
			return void(s = s + 1 & 0xffff);
		case 0x32: // PULA
			return void(a = pul());
		case 0x33: // PULB
			return void(b = pul());
		case 0x34: // DES
			return void(s = s - 1 & 0xffff);
		case 0x35: // TXS
			return void(s = x - 1 & 0xffff);
		case 0x36: // PSHA
			return psh(a);
		case 0x37: // PSHB
			return psh(b);
		case 0x38: // PULX
			return void(x = pul16());
		case 0x39: // RTS
			return void(pc = pul16());
		case 0x3a: // ABX
			return void(x = x + b & 0xffff);
		case 0x3b: // RTI
			return ccr = pul() | 0xc0, b = pul(), a = pul(), x = pul16(), void(pc = pul16());
		case 0x3c: // PSHX
			return psh16(x);
		case 0x3d: // MUL
			return split(a, b, a * b), void(ccr = ccr & ~1 | b >> 7);
		case 0x3e: // WAI
			return suspend();
		case 0x3f: // SWI
			return psh16(pc), psh16(x), psh(a), psh(b), psh(ccr), ccr |= 0x10, void(pc = read16(0xfffa));
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
			return tst8(a);
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
			return tst8(b);
		case 0x5f: // CLRB
			return void(b = clr8());
		case 0x60: // NEG ,X
			return ea = x + fetch() & 0xffff, write8(neg8(read(ea)), ea);
		case 0x61: // AIM ,X
			return v = fetch(), ea = x + fetch() & 0xffff, write8(mov8(v & read(ea)), ea);
		case 0x62: // OIM ,X
			return v = fetch(), ea = x + fetch() & 0xffff, write8(mov8(v | read(ea)), ea);
		case 0x63: // COM ,X
			return ea = x + fetch() & 0xffff, write8(com8(read(ea)), ea);
		case 0x64: // LSR ,X
			return ea = x + fetch() & 0xffff, write8(lsr8(read(ea)), ea);
		case 0x65: // EIM ,X
			return v = fetch(), ea = x + fetch() & 0xffff, write8(mov8(v ^ read(ea)), ea);
		case 0x66: // ROR ,X
			return ea = x + fetch() & 0xffff, write8(ror8(read(ea)), ea);
		case 0x67: // ASR ,X
			return ea = x + fetch() & 0xffff, write8(asr8(read(ea)), ea);
		case 0x68: // LSL ,X
			return ea = x + fetch() & 0xffff, write8(lsl8(read(ea)), ea);
		case 0x69: // ROL ,X
			return ea = x + fetch() & 0xffff, write8(rol8(read(ea)), ea);
		case 0x6a: // DEC ,X
			return ea = x + fetch() & 0xffff, write8(dec8(read(ea)), ea);
		case 0x6b: // TIM ,X
			return v = fetch(), void(mov8(v & read(x + fetch() & 0xffff)));
		case 0x6c: // INC ,X
			return ea = x + fetch() & 0xffff, write8(inc8(read(ea)), ea);
		case 0x6d: // TST ,X
			return tst8(read(x + fetch() & 0xffff));
		case 0x6e: // JMP ,X
			return void(pc = x + fetch() & 0xffff);
		case 0x6f: // CLR ,X
			return write8(clr8(), x + fetch() & 0xffff);
		case 0x70: // NEG >nn
			return ea = fetch16(), write8(neg8(read(ea)), ea);
		case 0x71: // AIM <n
			return v = fetch(), ea = fetch(), write8(mov8(v & read(ea)), ea);
		case 0x72: // OIM <n
			return v = fetch(), ea = fetch(), write8(mov8(v | read(ea)), ea);
		case 0x73: // COM >nn
			return ea = fetch16(), write8(com8(read(ea)), ea);
		case 0x74: // LSR >nn
			return ea = fetch16(), write8(lsr8(read(ea)), ea);
		case 0x75: // EIM <n
			return v = fetch(), ea = fetch(), write8(mov8(v ^ read(ea)), ea);
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
		case 0x7b: // TIM <n
			return v = fetch(), void(mov8(v & read(fetch())));
		case 0x7c: // INC >nn
			return ea = fetch16(), write8(inc8(read(ea)), ea);
		case 0x7d: // TST >nn
			return tst8(read(fetch16()));
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
		case 0x86: // LDAA #n
			return void(a = mov8(fetch()));
		case 0x88: // EORA #n
			return void(a = mov8(a ^ fetch()));
		case 0x89: // ADCA #n
			return void(a = adc8(fetch(), a));
		case 0x8a: // ORAA #n
			return void(a = mov8(a | fetch()));
		case 0x8b: // ADDA #n
			return void(a = add8(fetch(), a));
		case 0x8c: // CPX #nn
			return void(sub16(fetch16(), x));
		case 0x8d: // BSR
			return bsr();
		case 0x8e: // LDS #nn
			return void(s = mov16(fetch16()));
		case 0x90: // SUBA <n
			return void(a = sub8(read(fetch()), a));
		case 0x91: // CMPA <n
			return void(sub8(read(fetch()), a));
		case 0x92: // SBCA <n
			return void(a = sbc8(read(fetch()), a));
		case 0x93: // SUBD <n
			return split(a, b, sub16(read16(fetch()), a << 8 | b));
		case 0x94: // ANDA <n
			return void(a = mov8(a & read(fetch())));
		case 0x95: // BITA <n
			return void(mov8(a & read(fetch())));
		case 0x96: // LDAA <n
			return void(a = mov8(read(fetch())));
		case 0x97: // STAA <n
			return write8(mov8(a), fetch());
		case 0x98: // EORA <n
			return void(a = mov8(a ^ read(fetch())));
		case 0x99: // ADCA <n
			return void(a = adc8(read(fetch()), a));
		case 0x9a: // ORAA <n
			return void(a = mov8(a | read(fetch())));
		case 0x9b: // ADDA <n
			return void(a = add8(read(fetch()), a));
		case 0x9c: // CPX <n
			return void(sub16(read16(fetch()), x));
		case 0x9d: // JSR <n
			return jsr(fetch());
		case 0x9e: // LDS <n
			return void(s = mov16(read16(fetch())));
		case 0x9f: // STS <n
			return write16(mov16(s), fetch());
		case 0xa0: // SUBA ,X
			return void(a = sub8(read(x + fetch() & 0xffff), a));
		case 0xa1: // CMPA ,X
			return void(sub8(read(x + fetch() & 0xffff), a));
		case 0xa2: // SBCA ,X
			return void(a = sbc8(read(x + fetch() & 0xffff), a));
		case 0xa3: // SUBD ,X
			return split(a, b, sub16(read16(x + fetch() & 0xffff), a << 8 | b));
		case 0xa4: // ANDA ,X
			return void(a = mov8(a & read(x + fetch() & 0xffff)));
		case 0xa5: // BITA ,X
			return void(mov8(a & read(x + fetch() & 0xffff)));
		case 0xa6: // LDAA ,X
			return void(a = mov8(read(x + fetch() & 0xffff)));
		case 0xa7: // STAA ,X
			return write8(mov8(a), x + fetch() & 0xffff);
		case 0xa8: // EORA ,X
			return void(a = mov8(a ^ read(x + fetch() & 0xffff)));
		case 0xa9: // ADCA ,X
			return void(a = adc8(read(x + fetch() & 0xffff), a));
		case 0xaa: // ORAA ,X
			return void(a = mov8(a | read(x + fetch() & 0xffff)));
		case 0xab: // ADDA ,X
			return void(a = add8(read(x + fetch() & 0xffff), a));
		case 0xac: // CPX ,X
			return void(sub16(read16(x + fetch() & 0xffff), x));
		case 0xad: // JSR ,X
			return jsr(x + fetch() & 0xffff);
		case 0xae: // LDS ,X
			return void(s = mov16(read16(x + fetch() & 0xffff)));
		case 0xaf: // STS ,X
			return write16(mov16(s), x + fetch() & 0xffff);
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
		case 0xb6: // LDAA >nn
			return void(a = mov8(read(fetch16())));
		case 0xb7: // STAA >nn
			return write8(mov8(a), fetch16());
		case 0xb8: // EORA >nn
			return void(a = mov8(a ^ read(fetch16())));
		case 0xb9: // ADCA >nn
			return void(a = adc8(read(fetch16()), a));
		case 0xba: // ORAA >nn
			return void(a = mov8(a | read(fetch16())));
		case 0xbb: // ADDA >nn
			return void(a = add8(read(fetch16()), a));
		case 0xbc: // CPX >nn
			return void(sub16(read16(fetch16()), x));
		case 0xbd: // JSR >nn
			return jsr(fetch16());
		case 0xbe: // LDS >nn
			return void(s = mov16(read16(fetch16())));
		case 0xbf: // STS >nn
			return write16(mov16(s), fetch16());
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
		case 0xc6: // LDAB #n
			return void(b = mov8(fetch()));
		case 0xc8: // EORB #n
			return void(b = mov8(b ^ fetch()));
		case 0xc9: // ADCB #n
			return void(b = adc8(fetch(), b));
		case 0xca: // ORAB #n
			return void(b = mov8(b | fetch()));
		case 0xcb: // ADDB #n
			return void(b = add8(fetch(), b));
		case 0xcc: // LDD #nn
			return split(a, b, mov16(fetch16()));
		case 0xce: // LDX #nn
			return void(x = mov16(fetch16()));
		case 0xd0: // SUBB <n
			return void(b = sub8(read(fetch()), b));
		case 0xd1: // CMPB <n
			return void(sub8(read(fetch()), b));
		case 0xd2: // SBCB <n
			return void(b = sbc8(read(fetch()), b));
		case 0xd3: // ADDD <n
			return split(a, b, add16(read16(fetch()), a << 8 | b));
		case 0xd4: // ANDB <n
			return void(b = mov8(b & read(fetch())));
		case 0xd5: // BITB <n
			return void(mov8(b & read(fetch())));
		case 0xd6: // LDAB <n
			return void(b = mov8(read(fetch())));
		case 0xd7: // STAB <n
			return write8(mov8(b), fetch());
		case 0xd8: // EORB <n
			return void(b = mov8(b ^ read(fetch())));
		case 0xd9: // ADCB <n
			return void(b = adc8(read(fetch()), b));
		case 0xda: // ORAB <n
			return void(b = mov8(b | read(fetch())));
		case 0xdb: // ADDB <n
			return void(b = add8(read(fetch()), b));
		case 0xdc: // LDD <n
			return split(a, b, mov16(read16(fetch())));
		case 0xdd: // STD <n
			return write16(mov16(a << 8 | b), fetch());
		case 0xde: // LDX <n
			return void(x = mov16(read16(fetch())));
		case 0xdf: // STX <n
			return write16(mov16(x), fetch());
		case 0xe0: // SUBB ,X
			return void(b = sub8(read(x + fetch() & 0xffff), b));
		case 0xe1: // CMPB ,X
			return void(sub8(read(x + fetch() & 0xffff), b));
		case 0xe2: // SBCB ,X
			return void(b = sbc8(read(x + fetch() & 0xffff), b));
		case 0xe3: // ADDD ,X
			return split(a, b, add16(read16(x + fetch() & 0xffff), a << 8 | b));
		case 0xe4: // ANDB ,X
			return void(b = mov8(b & read(x + fetch() & 0xffff)));
		case 0xe5: // BITB ,X
			return void(mov8(b & read(x + fetch() & 0xffff)));
		case 0xe6: // LDAB ,X
			return void(b = mov8(read(x + fetch() & 0xffff)));
		case 0xe7: // STAB ,X
			return write8(mov8(b), x + fetch() & 0xffff);
		case 0xe8: // EORB ,X
			return void(b = mov8(b ^ read(x + fetch() & 0xffff)));
		case 0xe9: // ADCB ,X
			return void(b = adc8(read(x + fetch() & 0xffff), b));
		case 0xea: // ORAB ,X
			return void(b = mov8(b | read(x + fetch() & 0xffff)));
		case 0xeb: // ADDB ,X
			return void(b = add8(read(x + fetch() & 0xffff), b));
		case 0xec: // LDD ,X
			return split(a, b, mov16(read16(x + fetch() & 0xffff)));
		case 0xed: // STD ,X
			return write16(mov16(a << 8 | b), x + fetch() & 0xffff);
		case 0xee: // LDX ,X
			return void(x = mov16(read16(x + fetch() & 0xffff)));
		case 0xef: // STX ,X
			return write16(mov16(x), x + fetch() & 0xffff);
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
		case 0xf6: // LDAB >nn
			return void(b = mov8(read(fetch16())));
		case 0xf7: // STAB >nn
			return write8(mov8(b), fetch16());
		case 0xf8: // EORB >nn
			return void(b = mov8(b ^ read(fetch16())));
		case 0xf9: // ADCB >nn
			return void(b = adc8(read(fetch16()), b));
		case 0xfa: // ORAB >nn
			return void(b = mov8(b | read(fetch16())));
		case 0xfb: // ADDB >nn
			return void(b = add8(read(fetch16()), b));
		case 0xfc: // LDD >nn
			return split(a, b, mov16(read16(fetch16())));
		case 0xfd: // STD >nn
			return write16(mov16(a << 8 | b), fetch16());
		case 0xfe: // LDX >nn
			return void(x = mov16(read16(fetch16())));
		case 0xff: // STX >nn
			return write16(mov16(x), fetch16());
		default:
			undefsize = 1;
			if (undef)
				undef(pc);
			return;
		}
	}

	void bcc(bool cond) {
		const int n = fetch();
		if (cond) pc = pc + (n << 24 >> 24) & 0xffff;
	}

	void bsr() {
		const int n = fetch();
		psh16(pc), pc = pc + (n << 24 >> 24) & 0xffff;
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
		return ccr = ccr & ~0x0f | !r << 2 | c << 1 | c, r;
	}

	int lsr16(int dst) {
		const int r = dst >> 1, c = dst & 1;
		return ccr = ccr & ~0x0f | !r << 2 | c << 1 | c, r;
	}

	int ror8(int dst) {
		const int r = dst >> 1 | ccr << 7 & 0x80, c = dst & 1, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int asr8(int dst) {
		const int r = dst >> 1 | dst & 0x80, c = dst & 1, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int lsl8(int dst) {
		const int r = dst << 1 & 0xff, c = dst >> 7, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int lsl16(int dst) {
		const int r = dst << 1 & 0xffff, c = dst >> 15, v = r >> 15 ^ c;
		return ccr = ccr & ~0x0f | r >> 12 & 8 | !r << 2 | v << 1 | c, r;
	}

	int rol8(int dst) {
		const int r = dst << 1 & 0xff | ccr & 1, c = dst >> 7, v = r >> 7 ^ c;
		return ccr = ccr & ~0x0f | r >> 4 & 8 | !r << 2 | v << 1 | c, r;
	}

	int dec8(int dst) {
		const int r = dst - 1 & 0xff, v = dst & ~1 & ~r | ~dst & 1 & r;
		return ccr = ccr & ~0x0e | r >> 4 & 8 | !r << 2 | v >> 6 & 2, r;
	}

	int dec16(int dst) {
		const int r = dst - 1 & 0xffff;
		return ccr = ccr & ~4 | !r << 2, r;
	}

	int inc8(int dst) {
		const int r = dst + 1 & 0xff, v = dst & 1 & ~r | ~dst & ~1 & r;
		return ccr = ccr & ~0x0e | r >> 4 & 8 | !r << 2 | v >> 6 & 2, r;
	}

	int inc16(int dst) {
		const int r = dst + 1 & 0xffff;
		return ccr = ccr & ~4 | !r << 2, r;
	}

	void tst8(int src) {
		ccr = ccr & ~0x0f | src >> 4 & 8 | !src << 2;
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
		if (ccr & 0x20 && (a & 0xf) < 4 || (a & 0xf) > 9)
			cf += 6;
		if (ccr & 1 && (a & 0xf0) < 0x40 || (a & 0xf0) > 0x90 || (a & 0xf0) > 0x80 && (a & 0xf) > 9)
			cf += 0x60, ccr |= 1;
		a = a + cf & 0xff, ccr = ccr & ~0x0c | a >> 4 & 8 | !a << 2;
	}

	void jsr(int ea) {
		psh16(pc), pc = ea;
	}

	void psh(int r) {
		write8(r, s), s = s - 1 & 0xffff;
	}

	int pul() {
		return s = s + 1 & 0xffff, read(s);
	}

	void psh16(int r) {
		psh(r & 0xff), psh(r >> 8);
	}

	int pul16() {
		const int r = pul() << 8;
		return r | pul();
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
};

#endif //MC6801_H
