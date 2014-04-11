#ifndef _TEST_ANIMATION_HPP
#define _TEST_ANIMATION_HPP

#include "LedMatrix.hpp"

template<class FbType>
class LedMatrixTestAnimation : public LedMatrixAnimation<FbType> {
public:
	LedMatrixTestAnimation(LedMatrix<FbType> &matrix, LedMatrixAnimation<FbType> &anim1)
		: matrix(matrix), animation1(anim1),
	 	  mode(CLEAR),
		  yellow(15, 32,0) {
			  matrix.setAnimationInterval(1);
	}

	bool update(FbType &fb) {
		bool done = false;
		switch(mode) {
			case CLEAR:
				counter++;

				{
					LedMatrixColor color(3, counter, 0x00);
					fb.clear(color);
				}

				if( counter > 32 ) {
					matrix.setAnimationInterval(5);
					mode = ANIM1;
					counter = 0;
				}
			break;
			case ANIM1:
				if( animation1.update(fb) ) {
					matrix.setAnimationInterval(2);
					mode = FILL;
					fb.clear();
					counter = 0;
				}
			break;
			case FILL:
				fb.putPixel(fillX,fillY, yellow);
				fillX++;
				if( fillX >= fb.getColCount() ) {
					fillX = 0;
					fillY++;
				}
				if( fillY >= fb.getRowCount() ) {
					fillY = 0;
					mode = SCAN;
					matrix.setAnimationInterval(2);
				}
			break;
			case SCAN:
				counter++;
				fb.clear();
				fb.fillRow(fillY, yellow);
				
				if( fillX == 0) {
					fillY++;
					if( fillY >= fb.getRowCount() ) {
						fillX = 1;
						fillY -= 2;
					}
				} else {
					if( fillY == 0 ) {
						fillY++;
						fillX = 0;
					} else {
						fillY--;
					}
				}
				if( counter > 5 * fb.getRowCount() ) {
					counter = 0;
					fillX = fillY = 0;
					mode = CLEAR;
					matrix.setAnimationInterval(1);
					done = true;
				}
			break;
		}

		return done;
	}

	void reset() {
		counter = 0;
		fillX = fillY = 0;
		mode = CLEAR;
	}


private:
	enum Mode {CLEAR, ANIM1, FILL, SCAN};

	LedMatrix<FbType>		&matrix;
	LedMatrixAnimation<FbType>	&animation1;
	Mode		   	mode;
	int		   	counter;
	int			fillX, fillY;
	LedMatrixColor		yellow;
};

#endif
