#ifndef _PULSE_ANIMATION_H
#define _PULSE_ANIMATION_H

#include "LedMatrix.hpp"

class PulseAnimation : public LedMatrixAnimation {
public:
	PulseAnimation() 
		: red(0), green(32),
		  redInc(1), greenInc(-1) {
	}

	bool update(AbstractLedMatrixFrameBuffer &fb) {
		LedMatrixColor color(red, green, 0);
		fb.clear(color);
		red += redInc;
		green += greenInc;
		if( red > fb.getLevels() ) {
			redInc *= -1;
		}
		if( red > fb.getLevels()+1 ) {
			red = 0;
		}
		if( green > fb.getLevels() ) {
			greenInc *= -1;
		}
		if( green > fb.getLevels()+1 ) {
			green = 0;
		}
		return false;
	}

	void reset() {
	}

private:
	uint8_t red, green;
	int8_t redInc, greenInc;
};

#endif
