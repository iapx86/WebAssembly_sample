/*
 * Sound Effect
 */

#ifndef SOUND_EFFECT_H
#define SOUND_EFFECT_H

#include <vector>
using namespace std;

struct SE {
	int freq = 48000;
	vector<short> buf;
	bool loop = false;
	bool play = false;
	bool start = false;
	bool stop = false;
	int p = 0;
	int frac = 0;
};

struct SoundEffect {
	vector<SE> *se = nullptr;
	int sampleRate;
	double gain;
	double output = 0;

	SoundEffect(vector<SE>& se, int sampleRate = 48000, double gain = 1) {
		this->se = &se;
		this->sampleRate = sampleRate;
		this->gain = gain;
	}

	void update() {
		for (auto& ch: *se) {
			if (ch.stop)
				ch.play = false;
			if (ch.start && !ch.play)
				ch.play = true, ch.p = ch.frac = 0;
			ch.start = ch.stop = false;
			if (!ch.play)
				continue;
			for (ch.frac += ch.freq; ch.frac >= sampleRate; ch.frac -= sampleRate) {
				if (++ch.p < ch.buf.size())
					continue;
				if (!(ch.play = ch.loop))
					break;
				ch.p = 0;
			}
		}
		output = 0;
		for (auto& ch: *se)
			ch.play && (output += ch.buf[ch.p] / 32767.0 * gain);
	}
};

#endif //SOUND_EFFECT_H
