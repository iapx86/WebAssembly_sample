/*
 *	Utilities
 */

#ifndef UTILS_H
#define UTILS_H

#include <initializer_list>
#include <numeric>
using namespace std;

#define seq2(s, d) (s), (s) + (d)
#define seq4(s, d) seq2(s, d), seq2((s) + 2 * (d), d)
#define seq8(s, d) seq4(s, d), seq4((s) + 4 * (d), d)
#define seq16(s, d) seq8(s, d), seq8((s) + 8 * (d), d)
#define seq32(s, d) seq16(s, d), seq16((s) + 16 * (d), d)
#define rseq2(s, d) (s) + (d), (s)
#define rseq4(s, d) rseq2((s) + 2 * (d), d), rseq2(s, d)
#define rseq8(s, d) rseq4((s) + 4 * (d), d), rseq4(s, d)
#define rseq16(s, d) rseq8((s) + 8 * (d), d), rseq8(s, d)
#define rseq32(s, d) rseq16((s) + 16 * (d), d), rseq16(s, d)

inline int bitswap(int val, const initializer_list<int>& b) {
	return reduce(b.begin(), b.end(), 0, [&](int a, int i) { return a << 1 | val >> i & 1; });
}

inline void convertGFX(unsigned char *dst, const unsigned char *src, int n, const initializer_list<int>& x, const initializer_list<int>& y, const initializer_list<int>& z, int d) {
	for (int i = 0; i < n; src += d, i++)
		for (int j : y)
			for (int k : x)
				*dst++ ^= reduce(z.begin(), z.end(), 0, [&](int a, int l) { return a << 1 | (l < 0 ? 0 : ~src[k + j + l >> 3] >> (k + j + l & 7 ^ 7) & 1); });
}

template<typename T> struct Timer {
	T rate = 0;
	T frac = 0;
	function<void()> fn = []() {};
	Timer(T rate = 0) { this->rate = rate; }
	void execute(T rate) {
		for (frac += this->rate; frac >= rate; frac -= rate)
			fn();
	}
	void execute(T rate, function<void()> fn) {
		for (frac += this->rate; frac >= rate; frac -= rate)
			fn();
	}
};

#endif //UTILS_H
