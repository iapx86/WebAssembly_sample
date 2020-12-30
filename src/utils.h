/*
 *	Utilities
 */

#ifndef UTILS_H
#define UTILS_H

#include <vector>

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

inline void convertGFX(unsigned char *dst, const unsigned char *src, int n, vector<int> x, vector<int> y, vector<int> z, int d) {
	for (int i = 0; i < n; src += d, i++)
		for (int j = 0; j < y.size(); j++)
			for (int k = 0; k < x.size(); dst++, k++)
				for (int l = 0; l < z.size(); l++)
					z[l] >= 0 && (*dst ^= (~src[x[k] + y[j] + z[l] >> 3] >> (x[k] + y[j] + z[l] & 7 ^ 7) & 1) << z.size() - l - 1);
}

#endif //UTILS_H
