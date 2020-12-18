/*
 *
 *	MB8840 Emulator
 *
 */

#ifndef MB8840_H
#define MB8840_H

enum {
	MB8840_EXTERNAL = 0, MB8840_TIMER, MB8840_SERIAL,
};

struct MB8840 {
	int pc = 0;
	int a = 0;
	int x = 0;
	int y = 0;
	bool zf = false;
	bool cf = false;
	bool st = false;
	unsigned char *m[8] = {};
	int o = 0;
	int p = 0;
	int r = 0;
	int k = 0;
	int t = 0;
	int sb = 0;
	unsigned char rom[0x800] = {};
	unsigned char ram[0x80] = {};
	unsigned short stack[4] = {};
	int sp = 0;
	int cause = 0; // cause:Ext(IF),Timer(VF),SB(SF)
	int mask = 0;
	int cycles = 0;

	MB8840() {
		for (int i = 0; i < 8; i++)
			m[i] = &ram[i * 16];
	}

	void reset() {
		pc = 0;
		st = true;
		sp = 3;
		cause = 0;
		mask = 0;
		cycles = 0;
	}

	bool interrupt(int _cause = MB8840_EXTERNAL) {
		int intvec;
		switch (_cause) {
		default:
		case MB8840_EXTERNAL:
			if (~mask & 4)
				return false;
			mask &= ~4, intvec = 2;
			break;
		case MB8840_TIMER:
			if (~mask & 2)
				return false;
			mask &= ~2, intvec = 4;
			break;
		case MB8840_SERIAL:
			if (~mask & 1)
				return false;
			mask &= ~1, intvec = 6;
		}
		return push(pc | zf << 13 | cf << 14 | st << 15), pc = intvec, st = true, cycles -= 3, true;
	}

	int execute() {
		int v;
		switch (fetch()) {
		case 0x00: // NOP
			return st = true, 0x00;
		case 0x01: // OUTO
			return o = o & ~(15 << (cf << 2)) | a << (cf << 2), st = true, 0x01;
		case 0x02: // OUTP
			return p = a, st = true, 0x02;
		case 0x03: // OUT
			return r = r & ~(15 << (y << 2 & 12)) | a << (y << 2 & 12), st = true, 0x03;
		case 0x04: // TAY
			return y = a, st = true, 0x04;
		case 0x05: // TATH
			return t = t & ~(15 << 4) | a << 4, st = true, 0x05;
		case 0x06: // TATL
			return t = t & ~15 | a, st = true, 0x06;
		case 0x07: // TAS
			return sb = a, st = true, 0x07;
		case 0x08: // ICY
			return y = y + 1 & 15, zf = !y, st = y != 0, 0x08;
		case 0x09: // ICM
			return v = m[x][y] = m[x][y] + 1 & 15, zf = !v, st = v != 0, 0x09;
		case 0x0a: // STIC
			return m[x][y] = a, y = y + 1 & 15, zf = !y, st = y != 0, 0x0a;
		case 0x0b: // X
			return v = m[x][y], m[x][y] = a, a = v, zf = !a, st = true, 0x0b;
		case 0x0c: // ROL
			return v = a << 1 | cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x0c;
		case 0x0d: // L
			return a = m[x][y], zf = !a, st = true, 0x0d;
		case 0x0e: // ADC
			return v = a + m[x][y] + cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x0e;
		case 0x0f: // AND
			return a &= m[x][y], zf = !a, st = !zf, 0x0f;
		case 0x10: // DAA
			return a > 9 && (cf = true), cf && (a = a + 6 & 15), st = !cf, 0x10;
		case 0x11: // DAS
			return a > 9 && (cf = true), cf && (a = a + 10 & 15), st = !cf, 0x11;
		case 0x12: // INK
			return a = k, zf = !a, st = true, 0x12;
		case 0x13: // IN
			return a = r >> (y << 2 & 12) & 15, st = true, 0x13;
		case 0x14: // TYA
			return a = y, zf = !a, st = true, 0x14;
		case 0x15: // TTHA
			return a = t >> 4, zf = !a, st = true, 0x15;
		case 0x16: // TTLA
			return a = t & 15, zf = !a, st = true, 0x16;
		case 0x17: // TSA
			return a = sb, zf = !a, st = true, 0x17;
		case 0x18: // DCY
			return y = y - 1 & 15, st = y != 15, 0x18;
		case 0x19: // DCM
			return v = m[x][y] = m[x][y] - 1 & 15, zf = !v, st = v != 15, 0x19;
		case 0x1a: // STDC
			return m[x][y] = a, y = y - 1 & 15, zf = !y, st = y != 15, 0x1a;
		case 0x1b: // XX
			return v = x, x = a, a = v, zf = !a, st = true, 0x1b;
		case 0x1c: // ROR
			return v = a >> 1 | cf << 3, zf = !v, cf = (a & 1) != 0, a = v, st = !cf, 0x1c;
		case 0x1d: // ST
			return m[x][y] = a, st = true, 0x1d;
		case 0x1e: // SBC
			return v = m[x][y] - a - cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x1e;
		case 0x1f: // OR
			return a |= m[x][y], zf = !a, st = !zf, 0x1f;
		case 0x20: // SETR
			return r |= 1 << y, st = true, 0x20;
		case 0x21: // SETC
			return cf = st = true, 0x21;
		case 0x22: // RSTR
			return r &= ~(1 << y), st = true, 0x22;
		case 0x23: // RSTC
			return cf = false, st = true, 0x23;
		case 0x24: // TSTR
			return st = !(r & 1 << y), 0x24;
		case 0x25: // TSTI
			return st = !(cause & 4), 0x25;
		case 0x26: // TSTV
			return st = !(cause & 2), cause &= ~2, 0x26;
		case 0x27: // TSTS
			return st = !(cause & 1), cause &= ~1, 0x27;
		case 0x28: // TSTC
			return st = !cf, 0x28;
		case 0x29: // TSTZ
			return st = !zf, 0x29;
		case 0x2a: // STS
			return m[x][y] = sb, zf = !sb, st = true, 0x2a;
		case 0x2b: // LS
			return sb = m[x][y], zf = !sb, st = true, 0x2b;
		case 0x2c: // RTS
			return pc = pop() & 0x7ff, st = true, 0x2c;
		case 0x2d: // NEG
			return a = -a & 15, st = a != 0, 0x2d;
		case 0x2e: // C
			return v = m[x][y] - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0x2e;
		case 0x2f: // EOR
			return a ^= m[x][y], zf = !a, st = !zf, 0x2f;
		case 0x30: // SBIT 0
			return m[x][y] |= 1, st = true, 0x30;
		case 0x31: // SBIT 1
			return m[x][y] |= 2, st = true, 0x31;
		case 0x32: // SBIT 2
			return m[x][y] |= 4, st = true, 0x32;
		case 0x33: // SBIT 3
			return m[x][y] |= 8, st = true, 0x33;
		case 0x34: // RBIT 0
			return m[x][y] &= ~1, st = true, 0x34;
		case 0x35: // RBIT 1
			return m[x][y] &= ~2, st = true, 0x35;
		case 0x36: // RBIT 2
			return m[x][y] &= ~4, st = true, 0x36;
		case 0x37: // RBIT 3
			return m[x][y] &= ~8, st = true, 0x37;
		case 0x38: // TBIT 0
			return st = !(m[x][y] & 1), 0x38;
		case 0x39: // TBIT 1
			return st = !(m[x][y] & 2), 0x39;
		case 0x3a: // TBIT 2
			return st = !(m[x][y] & 4), 0x3a;
		case 0x3b: // TBIT 3
			return st = !(m[x][y] & 8), 0x3b;
		case 0x3c: // RTI
			return v = pop(), zf = (v & 1 << 13) != 0, cf = (v & 1 << 14) != 0, st = (v & 1 << 15) != 0, pc = v & 0x7ff, 0x3c;
		case 0x3d: // JPA addr
			return pc = a << 2 | fetch() << 6 & 0x7c0, st = true, 0x3d;
		case 0x3e: // EN imm
			return mask |= fetch(), st = true, 0x3e;
		case 0x3f: // DIS imm
			return mask &= ~fetch(), st = true, 0x3f;
		case 0x40: // SETD 0
			return r |= 1, st = true, 0x40;
		case 0x41: // SETD 1
			return r |= 2, st = true, 0x41;
		case 0x42: // SETD 2
			return r |= 4, st = true, 0x42;
		case 0x43: // SETD 3
			return r |= 8, st = true, 0x43;
		case 0x44: // RSTD 0
			return r &= ~1, st = true, 0x44;
		case 0x45: // RSTD 1
			return r &= ~2, st = true, 0x45;
		case 0x46: // RSTD 2
			return r &= ~4, st = true, 0x46;
		case 0x47: // RSTD 3
			return r &= ~8, st = true, 0x47;
		case 0x48: // TSTD 8
			return st = !(r & 256), 0x48;
		case 0x49: // TSTD 9
			return st = !(r & 512), 0x49;
		case 0x4a: // TSTD 10
			return st = !(r & 1024), 0x4a;
		case 0x4b: // TSTD 11
			return st = !(r & 2048), 0x4b;
		case 0x4c: // TBA 0
			return st = !(a & 1), 0x4c;
		case 0x4d: // TBA 1
			return st = !(a & 2), 0x4d;
		case 0x4e: // TBA 2
			return st = !(a & 4), 0x4e;
		case 0x4f: // TBA 3
			return st = !(a & 8), 0x4f;
		case 0x50: // XD 0
			return v = m[0][0], m[0][0] = a, a = v, zf = !a, st = true, 0x50;
		case 0x51: // XD 1
			return v = m[0][1], m[0][1] = a, a = v, zf = !a, st = true, 0x51;
		case 0x52: // XD 2
			return v = m[0][2], m[0][2] = a, a = v, zf = !a, st = true, 0x52;
		case 0x53: // XD 3
			return v = m[0][3], m[0][3] = a, a = v, zf = !a, st = true, 0x53;
		case 0x54: // XYD 4
			return v = m[0][4], m[0][4] = y, y = v, zf = !y, st = true, 0x54;
		case 0x55: // XYD 5
			return v = m[0][5], m[0][5] = y, y = v, zf = !y, st = true, 0x55;
		case 0x56: // XYD 6
			return v = m[0][6], m[0][6] = y, y = v, zf = !y, st = true, 0x56;
		case 0x57: // XYD 7
			return v = m[0][7], m[0][7] = y, y = v, zf = !y, st = true, 0x57;
		case 0x58: // LXI 0
			return x = 0, zf = true, st = true, 0x58;
		case 0x59: // LXI 1
			return x = 1, zf = false, st = true, 0x59;
		case 0x5a: // LXI 2
			return x = 2, zf = false, st = true, 0x5a;
		case 0x5b: // LXI 3
			return x = 3, zf = false, st = true, 0x5b;
		case 0x5c: // LXI 4
			return x = 4, zf = false, st = true, 0x5c;
		case 0x5d: // LXI 5
			return x = 5, zf = false, st = true, 0x5d;
		case 0x5e: // LXI 6
			return x = 6, zf = false, st = true, 0x5e;
		case 0x5f: // LXI 7
			return x = 7, zf = false, st = true, 0x5f;
		case 0x60: // CALL addr
			return v = fetch(), st && (push(pc), pc = v), st = true, 0x60;
		case 0x61: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x100), st = true, 0x61;
		case 0x62: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x200), st = true, 0x62;
		case 0x63: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x300), st = true, 0x63;
		case 0x64: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x400), st = true, 0x64;
		case 0x65: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x500), st = true, 0x65;
		case 0x66: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x600), st = true, 0x66;
		case 0x67: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | 0x700), st = true, 0x67;
		case 0x68: // JPL addr
			return v = fetch(), st && (pc = v), st = true, 0x68;
		case 0x69: // JPL addr
			return v = fetch(), st && (pc = v | 0x100), st = true, 0x69;
		case 0x6a: // JPL addr
			return v = fetch(), st && (pc = v | 0x200), st = true, 0x6a;
		case 0x6b: // JPL addr
			return v = fetch(), st && (pc = v | 0x300), st = true, 0x6b;
		case 0x6c: // JPL addr
			return v = fetch(), st && (pc = v | 0x400), st = true, 0x6c;
		case 0x6d: // JPL addr
			return v = fetch(), st && (pc = v | 0x500), st = true, 0x6d;
		case 0x6e: // JPL addr
			return v = fetch(), st && (pc = v | 0x600), st = true, 0x6e;
		case 0x6f: // JPL addr
			return v = fetch(), st && (pc = v | 0x700), st = true, 0x6f;
		case 0x70: // AI 0
			return zf = !a, cf = false, st = true, 0x70;
		case 0x71: // AI 1/ICA
			return v = a + 1, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x71;
		case 0x72: // AI 2
			return v = a + 2, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x72;
		case 0x73: // AI 3
			return v = a + 3, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x73;
		case 0x74: // AI 4
			return v = a + 4, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x74;
		case 0x75: // AI 5
			return v = a + 5, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x75;
		case 0x76: // AI 6
			return v = a + 6, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x76;
		case 0x77: // AI 7
			return v = a + 7, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x77;
		case 0x78: // AI 8
			return v = a + 8, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x78;
		case 0x79: // AI 9
			return v = a + 9, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x79;
		case 0x7a: // AI 10
			return v = a + 10, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7a;
		case 0x7b: // AI 11
			return v = a + 11, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7b;
		case 0x7c: // AI 12
			return v = a + 12, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7c;
		case 0x7d: // AI 13
			return v = a + 13, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7d;
		case 0x7e: // AI 14
			return v = a + 14, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7e;
		case 0x7f: // AI 15/DCA
			return v = a + 15, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, 0x7f;
		case 0x80: // LYI 0
			return y = 0, zf = true, st = true, 0x80;
		case 0x81: // LYI 1
			return y = 1, zf = false, st = true, 0x81;
		case 0x82: // LYI 2
			return y = 2, zf = false, st = true, 0x82;
		case 0x83: // LYI 3
			return y = 3, zf = false, st = true, 0x83;
		case 0x84: // LYI 4
			return y = 4, zf = false, st = true, 0x84;
		case 0x85: // LYI 5
			return y = 5, zf = false, st = true, 0x85;
		case 0x86: // LYI 6
			return y = 6, zf = false, st = true, 0x86;
		case 0x87: // LYI 7
			return y = 7, zf = false, st = true, 0x87;
		case 0x88: // LYI 8
			return y = 8, zf = false, st = true, 0x88;
		case 0x89: // LYI 9
			return y = 9, zf = false, st = true, 0x89;
		case 0x8a: // LYI 10
			return y = 10, zf = false, st = true, 0x8a;
		case 0x8b: // LYI 11
			return y = 11, zf = false, st = true, 0x8b;
		case 0x8c: // LYI 12
			return y = 12, zf = false, st = true, 0x8c;
		case 0x8d: // LYI 13
			return y = 13, zf = false, st = true, 0x8d;
		case 0x8e: // LYI 14
			return y = 14, zf = false, st = true, 0x8e;
		case 0x8f: // LYI 15
			return y = 15, zf = false, st = true, 0x8f;
		case 0x90: // LI 0/CLA
			return a = 0, zf = true, st = true, 0x90;
		case 0x91: // LI 1
			return a = 1, zf = false, st = true, 0x91;
		case 0x92: // LI 2
			return a = 2, zf = false, st = true, 0x92;
		case 0x93: // LI 3
			return a = 3, zf = false, st = true, 0x93;
		case 0x94: // LI 4
			return a = 4, zf = false, st = true, 0x94;
		case 0x95: // LI 5
			return a = 5, zf = false, st = true, 0x95;
		case 0x96: // LI 6
			return a = 6, zf = false, st = true, 0x96;
		case 0x97: // LI 7
			return a = 7, zf = false, st = true, 0x97;
		case 0x98: // LI 8
			return a = 8, zf = false, st = true, 0x98;
		case 0x99: // LI 9
			return a = 9, zf = false, st = true, 0x99;
		case 0x9a: // LI 10
			return a = 10, zf = false, st = true, 0x9a;
		case 0x9b: // LI 11
			return a = 11, zf = false, st = true, 0x9b;
		case 0x9c: // LI 12
			return a = 12, zf = false, st = true, 0x9c;
		case 0x9d: // LI 13
			return a = 13, zf = false, st = true, 0x9d;
		case 0x9e: // LI 14
			return a = 14, zf = false, st = true, 0x9e;
		case 0x9f: // LI 15
			return a = 15, zf = false, st = true, 0x9f;
		case 0xa0: // CYI 0
			return v = -y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa0;
		case 0xa1: // CYI 1
			return v = 1 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa1;
		case 0xa2: // CYI 2
			return v = 2 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa2;
		case 0xa3: // CYI 3
			return v = 3 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa3;
		case 0xa4: // CYI 4
			return v = 4 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa4;
		case 0xa5: // CYI 5
			return v = 5 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa5;
		case 0xa6: // CYI 6
			return v = 6 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa6;
		case 0xa7: // CYI 7
			return v = 7 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa7;
		case 0xa8: // CYI 8
			return v = 8 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa8;
		case 0xa9: // CYI 9
			return v = 9 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xa9;
		case 0xaa: // CYI 10
			return v = 10 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xaa;
		case 0xab: // CYI 11
			return v = 11 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xab;
		case 0xac: // CYI 12
			return v = 12 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xac;
		case 0xad: // CYI 13
			return v = 13 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xad;
		case 0xae: // CYI 14
			return v = 14 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xae;
		case 0xaf: // CYI 15
			return v = 15 - y, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xaf;
		case 0xb0: // CI 0
			return v = -a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb0;
		case 0xb1: // CI 1
			return v = 1 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb1;
		case 0xb2: // CI 2
			return v = 2 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb2;
		case 0xb3: // CI 3
			return v = 3 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb3;
		case 0xb4: // CI 4
			return v = 4 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb4;
		case 0xb5: // CI 5
			return v = 5 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb5;
		case 0xb6: // CI 6
			return v = 6 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb6;
		case 0xb7: // CI 7
			return v = 7 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb7;
		case 0xb8: // CI 8
			return v = 8 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb8;
		case 0xb9: // CI 9
			return v = 9 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xb9;
		case 0xba: // CI 10
			return v = 10 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xba;
		case 0xbb: // CI 11
			return v = 11 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xbb;
		case 0xbc: // CI 12
			return v = 12 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xbc;
		case 0xbd: // CI 13
			return v = 13 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xbd;
		case 0xbe: // CI 14
			return v = 14 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xbe;
		case 0xbf: // CI 15
			return v = 15 - a, zf = !(v & 15), cf = v >> 4 != 0, st = !zf, 0xbf;
		case 0xc0: // JMP 0
			return st && (pc &= ~63), st = true, 0xc0;
		case 0xc1: // JMP 1
			return st && (pc = pc & ~63 | 1), st = true, 0xc1;
		case 0xc2: // JMP 2
			return st && (pc = pc & ~63 | 2), st = true, 0xc2;
		case 0xc3: // JMP 3
			return st && (pc = pc & ~63 | 3), st = true, 0xc3;
		case 0xc4: // JMP 4
			return st && (pc = pc & ~63 | 4), st = true, 0xc4;
		case 0xc5: // JMP 5
			return st && (pc = pc & ~63 | 5), st = true, 0xc5;
		case 0xc6: // JMP 6
			return st && (pc = pc & ~63 | 6), st = true, 0xc6;
		case 0xc7: // JMP 7
			return st && (pc = pc & ~63 | 7), st = true, 0xc7;
		case 0xc8: // JMP 8
			return st && (pc = pc & ~63 | 8), st = true, 0xc8;
		case 0xc9: // JMP 9
			return st && (pc = pc & ~63 | 9), st = true, 0xc9;
		case 0xca: // JMP 10
			return st && (pc = pc & ~63 | 10), st = true, 0xca;
		case 0xcb: // JMP 11
			return st && (pc = pc & ~63 | 11), st = true, 0xcb;
		case 0xcc: // JMP 12
			return st && (pc = pc & ~63 | 12), st = true, 0xcc;
		case 0xcd: // JMP 13
			return st && (pc = pc & ~63 | 13), st = true, 0xcd;
		case 0xce: // JMP 14
			return st && (pc = pc & ~63 | 14), st = true, 0xce;
		case 0xcf: // JMP 15
			return st && (pc = pc & ~63 | 15), st = true, 0xcf;
		case 0xd0: // JMP 16
			return st && (pc = pc & ~63 | 16), st = true, 0xd0;
		case 0xd1: // JMP 17
			return st && (pc = pc & ~63 | 17), st = true, 0xd1;
		case 0xd2: // JMP 18
			return st && (pc = pc & ~63 | 18), st = true, 0xd2;
		case 0xd3: // JMP 19
			return st && (pc = pc & ~63 | 19), st = true, 0xd3;
		case 0xd4: // JMP 20
			return st && (pc = pc & ~63 | 20), st = true, 0xd4;
		case 0xd5: // JMP 21
			return st && (pc = pc & ~63 | 21), st = true, 0xd5;
		case 0xd6: // JMP 22
			return st && (pc = pc & ~63 | 22), st = true, 0xd6;
		case 0xd7: // JMP 23
			return st && (pc = pc & ~63 | 23), st = true, 0xd7;
		case 0xd8: // JMP 24
			return st && (pc = pc & ~63 | 24), st = true, 0xd8;
		case 0xd9: // JMP 25
			return st && (pc = pc & ~63 | 25), st = true, 0xd9;
		case 0xda: // JMP 26
			return st && (pc = pc & ~63 | 26), st = true, 0xda;
		case 0xdb: // JMP 27
			return st && (pc = pc & ~63 | 27), st = true, 0xdb;
		case 0xdc: // JMP 28
			return st && (pc = pc & ~63 | 28), st = true, 0xdc;
		case 0xdd: // JMP 29
			return st && (pc = pc & ~63 | 29), st = true, 0xdd;
		case 0xde: // JMP 30
			return st && (pc = pc & ~63 | 30), st = true, 0xde;
		case 0xdf: // JMP 31
			return st && (pc = pc & ~63 | 31), st = true, 0xdf;
		case 0xe0: // JMP 32
			return st && (pc = pc & ~63 | 32), st = true, 0xe0;
		case 0xe1: // JMP 33
			return st && (pc = pc & ~63 | 33), st = true, 0xe1;
		case 0xe2: // JMP 34
			return st && (pc = pc & ~63 | 34), st = true, 0xe2;
		case 0xe3: // JMP 35
			return st && (pc = pc & ~63 | 35), st = true, 0xe3;
		case 0xe4: // JMP 36
			return st && (pc = pc & ~63 | 36), st = true, 0xe4;
		case 0xe5: // JMP 37
			return st && (pc = pc & ~63 | 37), st = true, 0xe5;
		case 0xe6: // JMP 38
			return st && (pc = pc & ~63 | 38), st = true, 0xe6;
		case 0xe7: // JMP 39
			return st && (pc = pc & ~63 | 39), st = true, 0xe7;
		case 0xe8: // JMP 40
			return st && (pc = pc & ~63 | 40), st = true, 0xe8;
		case 0xe9: // JMP 41
			return st && (pc = pc & ~63 | 41), st = true, 0xe9;
		case 0xea: // JMP 42
			return st && (pc = pc & ~63 | 42), st = true, 0xea;
		case 0xeb: // JMP 43
			return st && (pc = pc & ~63 | 43), st = true, 0xeb;
		case 0xec: // JMP 44
			return st && (pc = pc & ~63 | 44), st = true, 0xec;
		case 0xed: // JMP 45
			return st && (pc = pc & ~63 | 45), st = true, 0xed;
		case 0xee: // JMP 46
			return st && (pc = pc & ~63 | 46), st = true, 0xee;
		case 0xef: // JMP 47
			return st && (pc = pc & ~63 | 47), st = true, 0xef;
		case 0xf0: // JMP 48
			return st && (pc = pc & ~63 | 48), st = true, 0xf0;
		case 0xf1: // JMP 49
			return st && (pc = pc & ~63 | 49), st = true, 0xf1;
		case 0xf2: // JMP 50
			return st && (pc = pc & ~63 | 50), st = true, 0xf2;
		case 0xf3: // JMP 51
			return st && (pc = pc & ~63 | 51), st = true, 0xf3;
		case 0xf4: // JMP 52
			return st && (pc = pc & ~63 | 52), st = true, 0xf4;
		case 0xf5: // JMP 53
			return st && (pc = pc & ~63 | 53), st = true, 0xf5;
		case 0xf6: // JMP 54
			return st && (pc = pc & ~63 | 54), st = true, 0xf6;
		case 0xf7: // JMP 55
			return st && (pc = pc & ~63 | 55), st = true, 0xf7;
		case 0xf8: // JMP 56
			return st && (pc = pc & ~63 | 56), st = true, 0xf8;
		case 0xf9: // JMP 57
			return st && (pc = pc & ~63 | 57), st = true, 0xf9;
		case 0xfa: // JMP 58
			return st && (pc = pc & ~63 | 58), st = true, 0xfa;
		case 0xfb: // JMP 59
			return st && (pc = pc & ~63 | 59), st = true, 0xfb;
		case 0xfc: // JMP 60
			return st && (pc = pc & ~63 | 60), st = true, 0xfc;
		case 0xfd: // JMP 61
			return st && (pc = pc & ~63 | 61), st = true, 0xfd;
		case 0xfe: // JMP 62
			return st && (pc = pc & ~63 | 62), st = true, 0xfe;
		case 0xff: // JMP 63
			return st && (pc |= 63), st = true, 0xff;
		}
		return -1;
	}

	void push(int addr) {
		stack[sp] = addr, sp = sp - 1 & 3;
	}

	int pop() {
		return sp = sp + 1 & 3, stack[sp];
	}

	int fetch() {
		const int data = rom[pc];
		return pc = pc + 1 & 0x7ff, cycles -= 1, data;
	}
};

#endif //MB8840_H
