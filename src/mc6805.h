/*
 *
 *	MC6805 Emulator
 *
 */

#ifndef MC6805_H
#define MC6805_H

#include "cpu.h"

enum {
	MC6805_TIMER = 0, MC6805_EXTERNAL,
};

struct MC6805 : Cpu {
	static const unsigned char cc[0x100];
	int a = 0;
	int ccr = 0; // ccr:111hinzc
	int x = 0;
	int s = 0;
	bool irq = false;

	MC6805(int clock = 0) : Cpu(clock) {}

	void reset() override {
		Cpu::reset();
		a = 0;
		ccr = 0xe8;
		x = 0;
		s = 0x7f;
		pc = read16(0x7fe) & 0x7ff;
		irq = false;
	}

	bool interrupt() override {
		if (!Cpu::interrupt() || ccr & 8)
			return false;
		return cycle -= cc[0x83], psh16(pc), psh(x), psh(a), psh(ccr), ccr |= 8, pc = read16(0x7fa) & 0x7ff, true;
	}

	bool interrupt(int intvec) {
		if (!Cpu::interrupt() || ccr & 8)
			return false;
		cycle -= cc[0x83], psh16(pc), psh(x), psh(a), psh(ccr), ccr |= 8;
		switch (intvec) {
		case MC6805_TIMER:
			return pc = read16(0x7f8) & 0x7ff, true;
		default:
		case MC6805_EXTERNAL:
			return pc = read16(0x7fa) & 0x7ff, true;
		}
	}

	void _execute() override {
		int ea, op = fetch();
		cycle -= cc[op];
		switch (op) {
		case 0x00: // BRSET0
			return bcc(((ccr = ccr & ~1 | read(fetch()) & 1) & 1) != 0);
		case 0x01: // BRCLR0
			return bcc(!((ccr = ccr & ~1 | read(fetch()) & 1) & 1));
		case 0x02: // BRSET1
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 1 & 1) & 1) != 0);
		case 0x03: // BRCLR1
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 1 & 1) & 1));
		case 0x04: // BRSET2
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 2 & 1) & 1) != 0);
		case 0x05: // BRCLR2
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 2 & 1) & 1));
		case 0x06: // BRSET3
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 3 & 1) & 1) != 0);
		case 0x07: // BRCLR3
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 3 & 1) & 1));
		case 0x08: // BRSET4
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 4 & 1) & 1) != 0);
		case 0x09: // BRCLR4
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 4 & 1) & 1));
		case 0x0a: // BRSET5
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 5 & 1) & 1) != 0);
		case 0x0b: // BRCLR5
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 5 & 1) & 1));
		case 0x0c: // BRSET6
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 6 & 1) & 1) != 0);
		case 0x0d: // BRCLR6
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 6 & 1) & 1));
		case 0x0e: // BRSET7
			return bcc(((ccr = ccr & ~1 | read(fetch()) >> 7 & 1) & 1) != 0);
		case 0x0f: // BRCLR7
			return bcc(!((ccr = ccr & ~1 | read(fetch()) >> 7 & 1) & 1));
		case 0x10: // BSET0
			return ea = fetch(), write8(read(ea) | 1, ea);
		case 0x11: // BCLR0
			return ea = fetch(), write8(read(ea) & ~1, ea);
		case 0x12: // BSET1
			return ea = fetch(), write8(read(ea) | 2, ea);
		case 0x13: // BCLR1
			return ea = fetch(), write8(read(ea) & ~2, ea);
		case 0x14: // BSET2
			return ea = fetch(), write8(read(ea) | 4, ea);
		case 0x15: // BCLR2
			return ea = fetch(), write8(read(ea) & ~4, ea);
		case 0x16: // BSET3
			return ea = fetch(), write8(read(ea) | 8, ea);
		case 0x17: // BCLR3
			return ea = fetch(), write8(read(ea) & ~8, ea);
		case 0x18: // BSET4
			return ea = fetch(), write8(read(ea) | 0x10, ea);
		case 0x19: // BCLR4
			return ea = fetch(), write8(read(ea) & ~0x10, ea);
		case 0x1a: // BSET5
			return ea = fetch(), write8(read(ea) | 0x20, ea);
		case 0x1b: // BCLR5
			return ea = fetch(), write8(read(ea) & ~0x20, ea);
		case 0x1c: // BSET6
			return ea = fetch(), write8(read(ea) | 0x40, ea);
		case 0x1d: // BCLR6
			return ea = fetch(), write8(read(ea) & ~0x40, ea);
		case 0x1e: // BSET7
			return ea = fetch(), write8(read(ea) | 0x80, ea);
		case 0x1f: // BCLR7
			return ea = fetch(), write8(read(ea) & ~0x80, ea);
		case 0x20: // BRA
			return bcc(true);
		case 0x21: // BRN
			return bcc(false);
		case 0x22: // BHI
			return bcc(!((ccr >> 1 | ccr) & 1));
		case 0x23: // BLS
			return bcc(((ccr >> 1 | ccr) & 1) != 0);
		case 0x24: // BCC
			return bcc(!(ccr & 1));
		case 0x25: // BLO(BCS)
			return bcc((ccr & 1) != 0);
		case 0x26: // BNE
			return bcc(!(ccr & 2));
		case 0x27: // BEQ
			return bcc((ccr & 2) != 0);
		case 0x28: // BHCC
			return bcc(!(ccr & 0x10));
		case 0x29: // BHCS
			return bcc((ccr & 0x10) != 0);
		case 0x2a: // BPL
			return bcc(!(ccr & 4));
		case 0x2b: // BMI
			return bcc((ccr & 4) != 0);
		case 0x2c: // BMC
			return bcc(!(ccr & 8));
		case 0x2d: // BMS
			return bcc((ccr & 8) != 0);
		case 0x2e: // BIL
			return bcc(irq);
		case 0x2f: // BIH
			return bcc(!irq);
		case 0x30: // NEG <n
			return ea = fetch(), write8(neg8(read(ea)), ea);
		case 0x33: // COM <n
			return ea = fetch(), write8(com8(read(ea)), ea);
		case 0x34: // LSR <n
			return ea = fetch(), write8(lsr8(read(ea)), ea);
		case 0x36: // ROR <n
			return ea = fetch(), write8(ror8(read(ea)), ea);
		case 0x37: // ASR <n
			return ea = fetch(), write8(asr8(read(ea)), ea);
		case 0x38: // ASL(LSL) <n
			return ea = fetch(), write8(lsl8(read(ea)), ea);
		case 0x39: // ROL <n
			return ea = fetch(), write8(rol8(read(ea)), ea);
		case 0x3a: // DEC <n
			return ea = fetch(), write8(mov8(read(ea) - 1 & 0xff), ea);
		case 0x3c: // INC <n
			return ea = fetch(), write8(mov8(read(ea) + 1 & 0xff), ea);
		case 0x3d: // TST <n
			return void(mov8(read(fetch())));
		case 0x3f: // CLR <n
			return write8(clr8(), fetch());
		case 0x40: // NEGA
			return void(a = neg8(a));
		case 0x42: // MUL
			return split(x, a, x * a), void(ccr &= ~0x11);
		case 0x43: // COMA
			return void(a = com8(a));
		case 0x44: // LSRA
			return void(a = lsr8(a));
		case 0x46: // RORA
			return void(a = ror8(a));
		case 0x47: // ASRA
			return void(a = asr8(a));
		case 0x48: // ASLA(LSLA)
			return void(a = lsl8(a));
		case 0x49: // ROLA
			return void(a = rol8(a));
		case 0x4a: // DECA
			return void(a = mov8(a - 1 & 0xff));
		case 0x4c: // INCA
			return void(a = mov8(a + 1 & 0xff));
		case 0x4d: // TSTA
			return void(mov8(a));
		case 0x4f: // CLRA
			return void(a = clr8());
		case 0x50: // NEGX
			return void(x = neg8(x));
		case 0x53: // COMX
			return void(x = com8(x));
		case 0x54: // LSRX
			return void(x = lsr8(x));
		case 0x56: // RORX
			return void(x = ror8(x));
		case 0x57: // ASRX
			return void(x = asr8(x));
		case 0x58: // ASLX(LSLX)
			return void(x = lsl8(x));
		case 0x59: // ROLX
			return void(x = rol8(x));
		case 0x5a: // DECX
			return void(x = mov8(x - 1 & 0xff));
		case 0x5c: // INCX
			return void(x = mov8(x + 1 & 0xff));
		case 0x5d: // TSTX
			return void(mov8(x));
		case 0x5f: // CLRX
			return void(x = clr8());
		case 0x60: // NEG n,X
			return ea = x + fetch(), write8(neg8(read(ea)), ea);
		case 0x63: // COM n,X
			return ea = x + fetch(), write8(com8(read(ea)), ea);
		case 0x64: // LSR n,X
			return ea = x + fetch(), write8(lsr8(read(ea)), ea);
		case 0x66: // ROR n,X
			return ea = x + fetch(), write8(ror8(read(ea)), ea);
		case 0x67: // ASR n,X
			return ea = x + fetch(), write8(asr8(read(ea)), ea);
		case 0x68: // ASL(LSL) n,X
			return ea = x + fetch(), write8(lsl8(read(ea)), ea);
		case 0x69: // ROL n,X
			return ea = x + fetch(), write8(rol8(read(ea)), ea);
		case 0x6a: // DEC n,X
			return ea = x + fetch(), write8(mov8(read(ea) - 1 & 0xff), ea);
		case 0x6c: // INC n,X
			return ea = x + fetch(), write8(mov8(read(ea) + 1 & 0xff), ea);
		case 0x6d: // TST n,X
			return void(mov8(read(x + fetch())));
		case 0x6f: // CLR n,X
			return write8(clr8(), x + fetch());
		case 0x70: // NEG ,X
			return write8(neg8(read(x)), x);
		case 0x73: // COM ,X
			return write8(com8(read(x)), x);
		case 0x74: // LSR ,X
			return write8(lsr8(read(x)), x);
		case 0x76: // ROR ,X
			return write8(ror8(read(x)), x);
		case 0x77: // ASR ,X
			return write8(asr8(read(x)), x);
		case 0x78: // ASL(LSL) ,X
			return write8(lsl8(read(x)), x);
		case 0x79: // ROL ,X
			return write8(rol8(read(x)), x);
		case 0x7a: // DEC ,X
			return write8(mov8(read(x) - 1 & 0xff), x);
		case 0x7c: // INC ,X
			return write8(mov8(read(x) + 1 & 0xff), x);
		case 0x7d: // TST ,X
			return void(mov8(read(x)));
		case 0x7f: // CLR ,X
			return write8(clr8(), x);
		case 0x80: // RTI
			return ccr = pul() | 0xe0, a = pul(), x = pul(), void(pc = pul16() & 0x7ff);
		case 0x81: // RTS
			return void(pc = pul16() & 0x7ff);
		case 0x83: // SWI
			return psh16(pc), psh(x), psh(a), psh(ccr), ccr |= 8, void(pc = read16(0x7fc) & 0x7ff);
		case 0x8e: // STOP
		case 0x8f: // WAIT
			return ccr &= ~8, suspend();
		case 0x97: // TAX
			return void(x = a);
		case 0x98: // CLC
			return void(ccr &= ~1);
		case 0x99: // SEC
			return void(ccr |= 1);
		case 0x9a: // CLI
			return void(ccr &= ~8);
		case 0x9b: // SEI
			return void(ccr |= 8);
		case 0x9c: // RSP
			return void(s = 0x7f);
		case 0x9d: // NOP
			return;
		case 0x9f: // TXA
			return void(a = x);
		case 0xa0: // SUB #n
			return void(a = sub8(fetch(), a));
		case 0xa1: // CMP #n
			return void(sub8(fetch(), a));
		case 0xa2: // SBC #n
			return void(a = sbc8(fetch(), a));
		case 0xa3: // CPX #n
			return void(sub8(fetch(), x));
		case 0xa4: // AND #n
			return void(a = mov8(a & fetch()));
		case 0xa5: // BIT #n
			return void(mov8(a & fetch()));
		case 0xa6: // LDA #n
			return void(a = mov8(fetch()));
		case 0xa8: // EOR #n
			return void(a = mov8(a ^ fetch()));
		case 0xa9: // ADC #n
			return void(a = adc8(fetch(), a));
		case 0xaa: // ORA #n
			return void(a = mov8(a | fetch()));
		case 0xab: // ADD #n
			return void(a = add8(fetch(), a));
		case 0xad: // BSR
			return bsr();
		case 0xae: // LDX #n
			return void(x = mov8(fetch()));
		case 0xb0: // SUB <n
			return void(a = sub8(read(fetch()), a));
		case 0xb1: // CMP <n
			return void(sub8(read(fetch()), a));
		case 0xb2: // SBC <n
			return void(a = sbc8(read(fetch()), a));
		case 0xb3: // CPX <n
			return void(sub8(read(fetch()), x));
		case 0xb4: // AND <n
			return void(a = mov8(a & read(fetch())));
		case 0xb5: // BIT <n
			return void(mov8(a & read(fetch())));
		case 0xb6: // LDA <n
			return void(a = mov8(read(fetch())));
		case 0xb7: // STA <n
			return write8(mov8(a), fetch());
		case 0xb8: // EOR <n
			return void(a = mov8(a ^ read(fetch())));
		case 0xb9: // ADC <n
			return void(a = adc8(read(fetch()), a));
		case 0xba: // ORA <n
			return void(a = mov8(a | read(fetch())));
		case 0xbb: // ADD <n
			return void(a = add8(read(fetch()), a));
		case 0xbc: // JMP <n
			return void(pc = fetch());
		case 0xbd: // JSR <n
			return jsr(fetch());
		case 0xbe: // LDX <n
			return void(x = mov8(read(fetch())));
		case 0xbf: // STX <n
			return write8(mov8(x), fetch());
		case 0xc0: // SUB >nn
			return void(a = sub8(read(fetch16() & 0x7ff), a));
		case 0xc1: // CMP >nn
			return void(sub8(read(fetch16() & 0x7ff), a));
		case 0xc2: // SBC >nn
			return void(a = sbc8(read(fetch16() & 0x7ff), a));
		case 0xc3: // CPX >nn
			return void(sub8(read(fetch16() & 0x7ff), x));
		case 0xc4: // AND >nn
			return void(a = mov8(a & read(fetch16() & 0x7ff)));
		case 0xc5: // BIT >nn
			return void(mov8(a & read(fetch16() & 0x7ff)));
		case 0xc6: // LDA >nn
			return void(a = mov8(read(fetch16() & 0x7ff)));
		case 0xc7: // STA >nn
			return write8(mov8(a), fetch16() & 0x7ff);
		case 0xc8: // EOR >nn
			return void(a = mov8(a ^ read(fetch16() & 0x7ff)));
		case 0xc9: // ADC >nn
			return void(a = adc8(read(fetch16() & 0x7ff), a));
		case 0xca: // ORA >nn
			return void(a = mov8(a | read(fetch16() & 0x7ff)));
		case 0xcb: // ADD >nn
			return void(a = add8(read(fetch16() & 0x7ff), a));
		case 0xcc: // JMP >nn
			return void(pc = fetch16() & 0x7ff);
		case 0xcd: // JSR >nn
			return jsr(fetch16() & 0x7ff);
		case 0xce: // LDX >nn
			return void(x = mov8(read(fetch16() & 0x7ff)));
		case 0xcf: // STX >nn
			return write8(mov8(x), fetch16() & 0x7ff);
		case 0xd0: // SUB nn,X
			return void(a = sub8(read(x + fetch16() & 0x7ff), a));
		case 0xd1: // CMP nn,X
			return void(sub8(read(x + fetch16() & 0x7ff), a));
		case 0xd2: // SBC nn,X
			return void(a = sbc8(read(x + fetch16() & 0x7ff), a));
		case 0xd3: // CPX nn,X
			return void(sub8(read(x + fetch16() & 0x7ff), x));
		case 0xd4: // AND nn,X
			return void(a = mov8(a & read(x + fetch16() & 0x7ff)));
		case 0xd5: // BIT nn,X
			return void(mov8(a & read(x + fetch16() & 0x7ff)));
		case 0xd6: // LDA nn,X
			return void(a = mov8(read(x + fetch16() & 0x7ff)));
		case 0xd7: // STA nn,X
			return write8(mov8(a), x + fetch16() & 0x7ff);
		case 0xd8: // EOR nn,X
			return void(a = mov8(a ^ read(x + fetch16() & 0x7ff)));
		case 0xd9: // ADC nn,X
			return void(a = adc8(read(x + fetch16() & 0x7ff), a));
		case 0xda: // ORA nn,X
			return void(a = mov8(a | read(x + fetch16() & 0x7ff)));
		case 0xdb: // ADD nn,X
			return void(a = add8(read(x + fetch16() & 0x7ff), a));
		case 0xdc: // JMP nn,X
			return void(pc = x + fetch16() & 0x7ff);
		case 0xdd: // JSR nn,X
			return jsr(x + fetch16() & 0x7ff);
		case 0xde: // LDX nn,X
			return void(x = mov8(read(x + fetch16() & 0x7ff)));
		case 0xdf: // STX nn,X
			return write8(mov8(x), x + fetch16() & 0x7ff);
		case 0xe0: // SUB n,X
			return void(a = sub8(read(x + fetch()), a));
		case 0xe1: // CMP n,X
			return void(sub8(read(x + fetch()), a));
		case 0xe2: // SBC n,X
			return void(a = sbc8(read(x + fetch()), a));
		case 0xe3: // CPX n,X
			return void(sub8(read(x + fetch()), x));
		case 0xe4: // AND n,X
			return void(a = mov8(a & read(x + fetch())));
		case 0xe5: // BIT n,X
			return void(mov8(a & read(x + fetch())));
		case 0xe6: // LDA n,X
			return void(a = mov8(read(x + fetch())));
		case 0xe7: // STA n,X
			return write8(mov8(a), x + fetch());
		case 0xe8: // EOR n,X
			return void(a = mov8(a ^ read(x + fetch())));
		case 0xe9: // ADC n,X
			return void(a = adc8(read(x + fetch()), a));
		case 0xea: // ORA n,X
			return void(a = mov8(a | read(x + fetch())));
		case 0xeb: // ADD n,X
			return void(a = add8(read(x + fetch()), a));
		case 0xec: // JMP n,X
			return void(pc = x + fetch());
		case 0xed: // JSR n,X
			return jsr(x + fetch());
		case 0xee: // LDX n,X
			return void(x = mov8(read(x + fetch())));
		case 0xef: // STX n,X
			return write8(mov8(x), x + fetch());
		case 0xf0: // SUB ,X
			return void(a = sub8(read(x), a));
		case 0xf1: // CMP ,X
			return void(sub8(read(x), a));
		case 0xf2: // SBC ,X
			return void(a = sbc8(read(x), a));
		case 0xf3: // CPX ,X
			return void(sub8(read(x), x));
		case 0xf4: // AND ,X
			return void(a = mov8(a & read(x)));
		case 0xf5: // BIT ,X
			return void(mov8(a & read(x)));
		case 0xf6: // LDA ,X
			return void(a = mov8(read(x)));
		case 0xf7: // STA ,X
			return write8(mov8(a), x);
		case 0xf8: // EOR ,X
			return void(a = mov8(a ^ read(x)));
		case 0xf9: // ADC ,X
			return void(a = adc8(read(x), a));
		case 0xfa: // ORA ,X
			return void(a = mov8(a | read(x)));
		case 0xfb: // ADD ,X
			return void(a = add8(read(x), a));
		case 0xfc: // JMP ,X
			return void(pc = x);
		case 0xfd: // JSR ,X
			return jsr(x);
		case 0xfe: // LDX ,X
			return void(x = mov8(read(x)));
		case 0xff: // STX ,X
			return write8(mov8(x), x);
		default:
			undefsize = 1;
			if (undef)
				undef(pc);
			return;
		}
	}

	void bcc(bool cond) {
		const int n = fetch();
		if (cond) pc = pc + (n << 24 >> 24) & 0x7ff;
	}

	void bsr() {
		const int n = fetch();
		psh16(pc), pc = pc + (n << 24 >> 24) & 0x7ff;
	}

	int neg8(int dst) {
		const int r = -dst & 0xff, c = dst | r;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c >> 7 & 1, r;
	}

	int com8(int dst) {
		const int r = ~dst & 0xff;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | 1, r;
	}

	int lsr8(int dst) {
		const int r = dst >> 1, c = dst & 1;
		return ccr = ccr & ~7 | !r << 1 | c, r;
	}

	int ror8(int dst) {
		const int r = dst >> 1 | ccr << 7 & 0x80, c = dst & 1;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c, r;
	}

	int asr8(int dst) {
		const int r = dst >> 1 | dst & 0x80, c = dst & 1;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c, r;
	}

	int lsl8(int dst) {
		const int r = dst << 1 & 0xff, c = dst >> 7;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c, r;
	}

	int rol8(int dst) {
		const int r = dst << 1 & 0xff | ccr & 1, c = dst >> 7;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c, r;
	}

	int clr8() {
		return ccr = ccr & ~6 | 2, 0;
	}

	int sub8(int src, int dst) {
		const int r = dst - src & 0xff, c = ~dst & src | src & r | r & ~dst;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c >> 7 & 1, r;
	}

	int sbc8(int src, int dst) {
		const int r = dst - src - (ccr & 1) & 0xff, c = ~dst & src | src & r | r & ~dst;
		return ccr = ccr & ~7 | r >> 5 & 4 | !r << 1 | c >> 7 & 1, r;
	}

	int mov8(int src) {
		return ccr = ccr & ~6 | src >> 5 & 4 | !src << 1, src;
	}

	int adc8(int src, int dst) {
		const int r = dst + src + (ccr & 1) & 0xff, c = dst & src | src & ~r | ~r & dst;
		return ccr = ccr & ~0x17 | c << 1 & 0x10 | r >> 5 & 4 | !r << 1 | c >> 7 & 1, r;
	}

	int add8(int src, int dst) {
		const int r = dst + src & 0xff, c = dst & src | src & ~r | ~r & dst;
		return ccr = ccr & ~0x17 | c << 1 & 0x10 | r >> 5 & 4 | !r << 1 | c >> 7 & 1, r;
	}

	void jsr(int ea) {
		psh16(pc), pc = ea;
	}

	void psh(int r) {
		write8(r, s), s = s - 1 & 0x1f | 0x60;
	}

	int pul() {
		return s = s + 1 & 0x1f | 0x60, read(s);
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

	int fetch() override {
		Page& page = memorymap[pc >> 8];
		const int data = !page.fetch ? page.base[pc & 0xff] : page.fetch(pc);
		return pc = pc + 1 & 0x7ff, data;
	}

	int fetch16() {
		const int data = fetch() << 8;
		return data | fetch();
	}

	int read16(int addr) {
		const int data = read(addr) << 8;
		return data | read(addr + 1);
	}

	void write8(int data, int addr) {
		Page& page = memorymap[addr >> 8];
		!page.write ? void(page.base[addr & 0xff] = data) : page.write(addr, data);
	}
};

#endif //MC6805_H
