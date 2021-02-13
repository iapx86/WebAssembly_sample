/*
 *	DAC
 */

#ifndef DAC_H
#define DAC_H

struct Dac1Ch {
	double gain = 0;
	double output = 0;
	double data = 0;
	double vol = 1;
	Dac1Ch(double gain = 1) { this->gain = gain; }
	void update() {
		output = data * vol * gain;
	}
};

struct Dac2Ch {
	double gain = 0;
	double output = 0;
	double d1 = 0;
	double d2 = 0;
	double v1 = 1;
	double v2 = 1;
	Dac2Ch(double gain = 1) { this->gain = gain; }
	void update() {
		output = (d1 * v1 + d2 * v2) * gain;
	}
};

#endif //DAC_H
