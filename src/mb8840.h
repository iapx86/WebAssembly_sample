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

	bool interrupt(int cause = MB8840_EXTERNAL) {
		int vector = 0;
		switch (cause) {
		default:
		case MB8840_EXTERNAL:
			if (~mask & 4)
				return false;
			mask &= ~4, vector = 2;
			break;
		case MB8840_TIMER:
			if (~mask & 2)
				return false;
			mask &= ~2, vector = 4;
			break;
		case MB8840_SERIAL:
			if (~mask & 1)
				return false;
			mask &= ~1, vector = 6;
		}
		return push(pc | zf << 13 | cf << 14 | st << 15), pc = vector, st = true, cycles -= 3, true;
	}

	int execute() {
		int op = fetch(), v;
		switch (op) {
		case 0x00: // NOP
			return st = true, op;
		case 0x01: // OUTO
			return o = o & ~(15 << (cf << 2)) | a << (cf << 2), st = true, op;
		case 0x02: // OUTP
			return p = a, st = true, op;
		case 0x03: // OUT
			return r = r & ~(15 << (y << 2 & 12)) | a << (y << 2 & 12), st = true, op;
		case 0x04: // TAY
			return y = a, st = true, op;
		case 0x05: // TATH
			return t = t & ~(15 << 4) | a << 4, st = true, op;
		case 0x06: // TATL
			return t = t & ~15 | a, st = true, op;
		case 0x07: // TAS
			return sb = a, st = true, op;
		case 0x08: // ICY
			return y = y + 1 & 15, zf = !y, st = y != 0, op;
		case 0x09: // ICM
			return v = m[x][y] = m[x][y] + 1 & 15, zf = !v, st = v != 0, op;
		case 0x0a: // STIC
			return m[x][y] = a, y = y + 1 & 15, zf = !y, st = y != 0, op;
		case 0x0b: // X
			return v = m[x][y], m[x][y] = a, a = v, zf = !a, st = true, op;
		case 0x0c: // ROL
			return v = a << 1 | cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, op;
		case 0x0d: // L
			return a = m[x][y], zf = !a, st = true, op;
		case 0x0e: // ADC
			return v = a + m[x][y] + cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, op;
		case 0x0f: // AND
			return a &= m[x][y], zf = !a, st = !zf, op;
		case 0x10: // DAA
			return a > 9 && (cf = true), cf && (a = a + 6 & 15), st = !cf, op;
		case 0x11: // DAS
			return a > 9 && (cf = true), cf && (a = a + 10 & 15), st = !cf, op;
		case 0x12: // INK
			return a = k, zf = !a, st = true, op;
		case 0x13: // IN
			return a = r >> (y << 2 & 12) & 15, st = true, op;
		case 0x14: // TYA
			return a = y, zf = !a, st = true, op;
		case 0x15: // TTHA
			return a = t >> 4, zf = !a, st = true, op;
		case 0x16: // TTLA
			return a = t & 15, zf = !a, st = true, op;
		case 0x17: // TSA
			return a = sb, zf = !a, st = true, op;
		case 0x18: // DCY
			return y = y - 1 & 15, st = y != 15, op;
		case 0x19: // DCM
			return v = m[x][y] = m[x][y] - 1 & 15, zf = !v, st = v != 15, op;
		case 0x1a: // STDC
			return m[x][y] = a, y = y - 1 & 15, zf = !y, st = y != 15, op;
		case 0x1b: // XX
			return v = x, x = a, a = v, zf = !a, st = true, op;
		case 0x1c: // ROR
			return v = a >> 1 | cf << 3, zf = !v, cf = (a & 1) != 0, a = v, st = !cf, op;
		case 0x1d: // ST
			return m[x][y] = a, st = true, op;
		case 0x1e: // SBC
			return v = m[x][y] - a - cf, a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, op;
		case 0x1f: // OR
			return a |= m[x][y], zf = !a, st = !zf, op;
		case 0x20: // SETR
			return r |= 1 << y, st = true, op;
		case 0x21: // SETC
			return cf = st = true, op;
		case 0x22: // RSTR
			return r &= ~(1 << y), st = true, op;
		case 0x23: // RSTC
			return cf = false, st = true, op;
		case 0x24: // TSTR
			return st = (~r >> y & 1) != 0, op;
		case 0x25: // TSTI
			return st = (~cause >> 2 & 1) != 0, op;
		case 0x26: // TSTV
			return st = (~cause >> 1 & 1) != 0, cause &= ~2, op;
		case 0x27: // TSTS
			return st = (~cause & 1) != 0, cause &= ~1, op;
		case 0x28: // TSTC
			return st = !cf, op;
		case 0x29: // TSTZ
			return st = !zf, op;
		case 0x2a: // STS
			return m[x][y] = sb, zf = !sb, st = true, op;
		case 0x2b: // LS
			return sb = m[x][y], zf = !sb, st = true, op;
		case 0x2c: // RTS
			return pc = pop() & 0x7ff, st = true, op;
		case 0x2d: // NEG
			return a = -a & 15, st = !!a, op;
		case 0x2e: // C
			return v = m[x][y] - a, zf = (v & 15) == 0, cf = v >> 4 != 0, st = !zf, op;
		case 0x2f: // EOR
			return a ^= m[x][y], zf = !a, st = !zf, op;
		case 0x30: case 0x31: case 0x32: case 0x33: // SBIT bp
			return m[x][y] |= 1 << (op & 3), st = true, op;
		case 0x34: case 0x35: case 0x36: case 0x37: // RBIT bp
			return m[x][y] &= ~(1 << (op & 3)), st = true, op;
		case 0x38: case 0x39: case 0x3a: case 0x3b: // TBIT bp
			return st = (~m[x][y] >> (op & 3) & 1) != 0, op;
		case 0x3c: // RTI
			return v = pop(), zf = (v >> 13 & 1) != 0, cf = (v >> 14 & 1) != 0, st = (v >> 15 & 1) != 0, pc = v & 0x7ff, op;
		case 0x3d: // JPA addr
			return pc = a << 2 | fetch() << 6 & 0x7c0, st = true, op;
		case 0x3e: // EN imm
			return mask |= fetch(), st = true, op;
		case 0x3f: // DIS imm
			return mask &= ~fetch(), st = true, op;
		case 0x40: case 0x41: case 0x42: case 0x43: // SETD d
			return r |= 1 << (op & 3), st = true, op;
		case 0x44: case 0x45: case 0x46: case 0x47: // RSTD d
			return r &= ~(1 << (op & 3)), st = true, op;
		case 0x48: case 0x49: case 0x4a: case 0x4b: // TSTD d
			return st = (~r >> (op & 15) & 1) != 0, op;
		case 0x4c: case 0x4d: case 0x4e: case 0x4f: // TBA bp
			return st = (~a >> (op & 3) & 1) != 0, op;
		case 0x50: case 0x51: case 0x52: case 0x53: // XD D
			return v = m[0][op & 3], m[0][op & 3] = a, a = v, zf = !a, st = true, op;
		case 0x54: case 0x55: case 0x56: case 0x57: // XYD D
			return v = m[0][op & 7], m[0][op & 7] = y, y = v, zf = !y, st = true, op;
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // LXI imm
			return x = op & 7, zf = !x, st = true, op;
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: // CALL addr
			return v = fetch(), st && (push(pc), pc = v | op << 8 & 0x700), st = true, op;
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // JPL addr
			return v = fetch(), st && (pc = v | op << 8 & 0x700), st = true, op;
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // AI imm
			return v = a + (op & 15), a = v & 15, zf = !a, cf = v >> 4 != 0, st = !cf, op;
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f: // LYI imm
			return y = op & 15, zf = !y, st = true, op;
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: // LI imm
			return a = op & 15, zf = !a, st = true, op;
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf: // CYI imm
			return v = (op & 15) - y, zf = (v & 15) == 0, cf = v >> 4 != 0, st = !zf, op;
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: // CI imm
			return v = (op & 15) - a, zf = (v & 15) == 0, cf = v >> 4 != 0, st = !zf, op;
		default: // JMP addr
			return st && (pc = pc & ~63 | op & 63), st = true, op;
		}
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
