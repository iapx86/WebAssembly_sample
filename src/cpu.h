/*
 *
 *	CPU Emulator
 *
 */

#ifndef CPU_H
#define CPU_H

#include <functional>
using namespace std;

typedef function<int(int addr)> FetchHandler;
typedef function<int(int addr)> ReadHandler;
typedef function<void(int addr, int data)> WriteHandler;
typedef function<bool()> InterruptHandler;
typedef function<void(int addr)> BreakpointHandler;
typedef function<void(int addr)> UndefHandler;

struct Page {
	static unsigned char dummypage[0x100];

	unsigned char *base = dummypage;
	FetchHandler fetch = nullptr;
	ReadHandler read = nullptr;
	WriteHandler write = dummywrite;

	static void dummywrite(int addr, int data) {}
};

struct Page16 : Page {
	ReadHandler read16 = nullptr;
	WriteHandler write16 = nullptr;
};

struct Cpu {
	bool fActive = false;
	bool fSuspend = false;
	int pc = 0;
	Page memorymap[0x100];
	InterruptHandler check_interrupt = nullptr;
	int breakpointmap[0x800] = {};
	BreakpointHandler breakpoint = nullptr;
	UndefHandler undef = nullptr;
	int undefsize = 0;
	int clock = 0;
	int frac = 0;
	int cycle = 0;

	Cpu(int clock = 0) {
		this->clock = clock;
	}

	void set_breakpoint(int addr) {
		breakpointmap[addr >> 5] |= 1 << (addr & 0x1f);
	}

	void clear_breakpoint(int addr) {
		breakpointmap[addr >> 5] &= ~(1 << (addr & 0x1f));
	}

	void clear_all_breakpoint() {
		for	(auto& e : breakpointmap)
			e =	0;
	}

	virtual void reset() {
		fActive = true;
		fSuspend = false;
		frac = 0;
		cycle = 0;
	}

	void enable() {
		if (fActive)
			return;
		reset();
	}

	void disable() {
		fActive = false;
	}

	void suspend() {
		if (fActive && !fSuspend)
			fSuspend = true;
	}

	void resume() {
		if (fActive && fSuspend)
			fSuspend = false;
	}

	virtual bool interrupt() {
		if (!fActive)
			return false;
		resume();
		return true;
	}

	void execute(int rate) {
		if (!fActive)
			return;
		for (cycle += (frac += clock) / rate, frac %= rate; cycle > 0;) {
			if (check_interrupt && check_interrupt())
				continue;
			if (fSuspend)
				return void(cycle = 0);
			if (breakpoint && breakpointmap[pc >> 5] >> (pc & 31) & 1)
				breakpoint(pc);
			_execute();
		}
	}

	void execute1() {
		if (!fActive || check_interrupt && check_interrupt() || fSuspend)
			return;
		if (breakpoint && breakpointmap[pc >> 5] >> (pc & 31) & 1)
			breakpoint(pc);
		_execute();
	}

	virtual void _execute() = 0;

	virtual int fetch() {
//		Page& page = memorymap[pc >> 8];
//		const int data = !page.fetch ? page.base[pc & 0xff] : page.fetch(pc, arg);
		const int data = memorymap[pc >> 8].base[pc & 0xff];
		return pc = pc + 1 & 0xffff, data;
	}

	virtual int read(int addr) {
		Page& page = memorymap[addr >> 8];
		return !page.read ? page.base[addr & 0xff] : page.read(addr);
	}

	void write(int addr, int data) {
		Page& page = memorymap[addr >> 8];
		!page.write ? void(page.base[addr & 0xff] = data) : page.write(addr, data);
	}
};

#endif //CPU_H
