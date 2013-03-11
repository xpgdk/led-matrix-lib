#ifndef _LED_MATRIX_HPP
#define _LED_MATRIX_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

extern "C" {
#include "led_matrix_config.h"
#include "uart.h"
}

#include <mcu++/gpio.hpp>

#define NEW_GPIO

//template <unsigned R, unsigned C, unsigned LEVELS> class LedMatrixFrameBuffer;
template <typename CONFIG> class LedMatrixFrameBuffer;
class AbstractLedMatrixFrameBuffer;

struct LedMatrixColor
{
public:
	LedMatrixColor() 
		: color(0) {
	}

	LedMatrixColor(LedMatrixColor &color) {
		this->color = color.color;
	}

	LedMatrixColor(uint8_t red, uint8_t green, uint8_t blue) {
		NOT_USED(blue);
		color = (red & 0xFF) | ((green & 0xFF)<<8);
	}

	inline uint16_t getValue() {
		return color;
	}
private:
	uint16_t color;
};

typedef	uint8_t FontChar[8];

class LedMatrixFont
{
public:
	virtual const FontChar*	getFontData() = 0;
	virtual uint8_t		getFontWidth() = 0;
	virtual uint8_t		getFontHeight() = 0;

	int 			getStart(char c);
	int			getEnd(char c);
};

class AbstractLedMatrixFrameBuffer
{
public:
	virtual uint16_t getRowCount() = 0;
	virtual uint16_t getColCount() = 0;
	virtual uint16_t getLevels() = 0;
	virtual void setChar(char c, LedMatrixColor &color, LedMatrixFont &font) = 0;
	virtual bool tick() = 0;
	virtual void clear(LedMatrixColor &color) = 0;
	virtual void clear() = 0;
	/*virtual uint16_t* operator [](int index) = 0;*/
	virtual void fillRow(uint16_t row, LedMatrixColor &color) = 0;
	virtual void init() = 0;
	virtual void putPixel(uint16_t x, uint16_t y, LedMatrixColor &color) = 0;
	virtual void putPixel(uint16_t x, uint16_t y, uint16_t color) = 0;
	virtual uint16_t getPixel(uint16_t x, uint16_t y) = 0;
	virtual void reset() = 0;
};

//template <unsigned int R, unsigned int C, unsigned int LEVELS>
template <typename CONFIG>
class LedMatrixFrameBuffer : public AbstractLedMatrixFrameBuffer
{
public:
	static const uint16_t R = CONFIG::Rows;
	static const uint16_t C = CONFIG::Cols;
	static const uint16_t LEVELS = CONFIG::Levels;
	LedMatrixFrameBuffer() {
		currentRow = 0;
		currentIntensity = 0;
	}

	void init() {
		CONFIG::GPIOColOutput::ConfigureDirection(MCU::GPIO::Output);
		CONFIG::GPIOColLatch::ConfigureDirection(MCU::GPIO::Output);
		CONFIG::GPIOColClock::ConfigureDirection(MCU::GPIO::Output);

		CONFIG::GPIORowLatch::ConfigureDirection(MCU::GPIO::Output);
		CONFIG::GPIORowClock::ConfigureDirection(MCU::GPIO::Output);
		CONFIG::GPIORowOutput::ConfigureDirection(MCU::GPIO::Output);
		CONFIG::GPIORowEnable::ConfigureDirection(MCU::GPIO::Output);

		rowReset();
	}

	void reset() {
		currentRow = 0;
		currentIntensity = 0;
	}

	uint16_t getRowCount() {
		return R;
	}

	uint16_t getColCount() {
		return C;
	}

	uint16_t getLevels() {
		return LEVELS;
	}

	void setChar(char c, LedMatrixColor &color, LedMatrixFont &font) {
		for(unsigned int i=0; i<R; i++) {
			for(unsigned int l=0; l<C; l++) {
				fb[i][l] = ((font.getFontData()[c-32][i] >> (C-1-l)) & 0x1) * color.getValue();
			}
		}
	}

	bool tick() {
		CONFIG::GPIORowEnable::Write(1);

		if( currentIntensity > (LEVELS-1) ) {
			currentIntensity = 0;
			currentRow++;
			if( currentRow >= R ) {
				currentRow = 0;
				rowFirstTick();
			} else {
				rowTick();
			}
			rowLatch();
		} else {
			currentIntensity++;
		}

		const uint16_t *dots = fb[currentRow];

		for(uint8_t disp=0; disp<C/8; disp+=2) {
			shiftOut(dots+(disp+0)*8, 0, currentIntensity);
			shiftOut(dots+(disp+1)*8, 0, currentIntensity);
			shiftOut(dots+(disp+0)*8, 8, currentIntensity);
			shiftOut(dots+(disp+1)*8, 8, currentIntensity);
		}


		colLatch();

		CONFIG::GPIORowEnable::Write(0);
		if( currentRow == R-1 && currentIntensity == (LEVELS-1)) {
			return true;
		} else {
			return false;
		}
	}

	void clear(LedMatrixColor &color) {
		for(unsigned int i=0; i<R; i++) {
			for(unsigned int l=0; l<C; l++) {
				fb[i][l] = color.getValue();
			}
		}
	}

	void clear() {
		LedMatrixColor blank(0,0,0);
		clear(blank);
	}

	void fillRow(uint16_t row, LedMatrixColor &color) {
		for(uint16_t c = 0; c<getColCount(); c++) {
			fb[row][c] = color.getValue();
		}
	}

	inline void putPixel(uint16_t x, uint16_t y, LedMatrixColor &color) {
		fb[y][x] = color.getValue();
	}

	inline void putPixel(uint16_t x, uint16_t y, uint16_t color) {
		fb[y][x] = color;
	}

	inline uint16_t getPixel(uint16_t x, uint16_t y) {
		return fb[y][x];
	}


private:
	void shiftOut(const uint16_t b[8], uint8_t shift, uint8_t threshold) {
		uint16_t mask = 0xFF << shift;
		uint16_t mt = threshold << shift;
		for(unsigned int i=0;i<8; i++) {
#ifdef NEW_GPIO
			if( (b[8-1-i] & mask) > mt) {
				CONFIG::GPIOColOutput::Write(1);
			} else {
				CONFIG::GPIOColOutput::Write(0);
			}
			CONFIG::GPIOColClock::Write(1);
			CONFIG::GPIOColClock::Write(0);
#else
			if( (b[8-1-i] & mask) > mt) {
				FAST_GPIOPinWrite(SER_OUT_PORT, SER_OUT_PIN, SER_OUT_PIN);
			} else {
				FAST_GPIOPinWrite(SER_OUT_PORT, SER_OUT_PIN, 0);
			}
			FAST_GPIOPinWrite(CLK_OUT_PORT, CLK_OUT_PIN, CLK_OUT_PIN);
			FAST_GPIOPinWrite(CLK_OUT_PORT, CLK_OUT_PIN, 0);
#endif
		}
	}

	void rowReset(void) {
		for(unsigned int i=0; i<R; i++) {
#ifdef NEW_GPIO
			CONFIG::GPIORowOutput::Write(1);
			CONFIG::GPIORowClock::Write(1);
			CONFIG::GPIORowClock::Write(0);
#else
			FAST_GPIOPinWrite(ROW_SER_OUT_PORT, ROW_SER_OUT_PIN, ROW_SER_OUT_PIN);

			FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, ROW_CLK_OUT_PIN);
			FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, 0);
#endif
		}
	}

	void rowTick(void) {
#ifdef NEW_GPIO
		CONFIG::GPIORowOutput::Write(1);
		CONFIG::GPIORowClock::Write(1);
		CONFIG::GPIORowClock::Write(0);
#else
		FAST_GPIOPinWrite(ROW_SER_OUT_PORT, ROW_SER_OUT_PIN, ROW_SER_OUT_PIN);

		FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, ROW_CLK_OUT_PIN);
		FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, 0);
#endif
	}

	void rowFirstTick(void) {
#ifdef NEW_GPIO
		CONFIG::GPIORowOutput::Write(0);
		CONFIG::GPIORowClock::Write(1);
		CONFIG::GPIORowClock::Write(0);
#else
		FAST_GPIOPinWrite(ROW_SER_OUT_PORT, ROW_SER_OUT_PIN, 0);
		FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, ROW_CLK_OUT_PIN);
		FAST_GPIOPinWrite(ROW_CLK_OUT_PORT, ROW_CLK_OUT_PIN, 0);
#endif
	}

	void rowLatch(void)
	{
#ifdef NEW_GPIO
		CONFIG::GPIORowLatch::Write(1);
		CONFIG::GPIORowLatch::Write(0);
#else
		FAST_GPIOPinWrite(ROW_LATCH_PORT, ROW_LATCH_PIN, ROW_LATCH_PIN);
		FAST_GPIOPinWrite(ROW_LATCH_PORT, ROW_LATCH_PIN, 0);
#endif
	}

	void colLatch(void) {
#ifdef NEW_GPIO
		CONFIG::GPIOColLatch::Write(1);
		CONFIG::GPIOColLatch::Write(0);
#else
		FAST_GPIOPinWrite(LATCH_PORT, LATCH_PIN, LATCH_PIN);
		FAST_GPIOPinWrite(LATCH_PORT, LATCH_PIN, 0);
#endif
	}

private:
	uint8_t		currentRow;
	uint8_t		currentIntensity;

public:
	uint16_t	fb[R][C]; // Holds actual framebuffer data
};


class LedMatrixAnimation
{
public:
	virtual void reset() = 0;
	virtual bool update(AbstractLedMatrixFrameBuffer &fb) = 0;
};

class LedMatrixScrollAnimation : public LedMatrixAnimation
{
public:
	LedMatrixScrollAnimation(LedMatrixFont &font) 
		: msgLen(0), font(font) {
	}

	void reset() {
		nextChar = 0;
		currentColor = LedMatrixColor(0x3F, 0, 0);
		parseColor();
		offset = font.getStart(msgBuffer[nextChar]);
		charEnd = font.getEnd(msgBuffer[nextChar]);
	}

	bool update(AbstractLedMatrixFrameBuffer &fb) {
		bool restarted = false;
		uint16_t start = fb.getRowCount()/2-4;
		for(uint16_t i=0; i<8; i++) {
			for(uint16_t l=0; l<fb.getColCount()-1; l++) {
				fb.putPixel(l, start+i, fb.getPixel(l+1, start+i));
			}
			if( nextChar < msgLen ) {
				fb.putPixel(fb.getColCount()-1, start+i, ((font.getFontData()[msgBuffer[nextChar]-32][i] >> (7-offset)) & 0x1) * currentColor.getValue());
				//fb[start+i][fb.getColCount()-1] = ((font.getFontData()[msgBuffer[nextChar]-32][i] >> (7-offset)) & 0x1) * currentColor.getValue();
			} else {
				fb.putPixel(fb.getColCount()-1, start+i, 0);
				//fb[start+i][fb.getColCount()-1] = 0;
			}
		}
		offset++;
		if( offset >= charEnd ) {
			nextChar++;
			parseColor();

			if( nextChar == msgLen ) {
				offset = 0;
				charEnd = fb.getColCount() / 2;
			} else {
				if( nextChar > msgLen ) {
					nextChar = 0;
					restarted = true;
					parseColor();
				}
				offset = font.getStart(msgBuffer[nextChar]);
				charEnd = font.getEnd(msgBuffer[nextChar]);
			}
		}
		return restarted;
	}

	void setMessage(char *msg) {
		setMessage(msg, strlen(msg));
	}

	void setMessage(char *msg, uint16_t len) {
		strncpy(msgBuffer, msg, len);
		msgLen = len;
		reset();
	}

	void appendMessage(char *msg, uint16_t len) {
		strncpy(msgBuffer+msgLen, msg, len);
		msgLen += len;
	}


private:
	void parseColor() {
		if( msgBuffer[nextChar] == '#' ) {
			uint8_t r = (hexCharToInt(msgBuffer[nextChar+1]) << 4) +
				    hexCharToInt(msgBuffer[nextChar+2]);
			uint8_t g = (hexCharToInt(msgBuffer[nextChar+3]) << 4) +
				    hexCharToInt(msgBuffer[nextChar+4]);
			currentColor = LedMatrixColor(r, g, 0);
			nextChar += 5;
		}
	}

	uint8_t hexCharToInt(char c) {
		if( c >= 0x30 && c <= 0x39 ) {
			return c-0x30;
		} else if( c >= 0x41 && c <= 0x46 ) {
			return 10 + (c-0x41);
		}
		return 0;
	}

private:
	char 			msgBuffer[60];
	uint16_t 		msgLen;
	uint16_t		nextChar;
	uint16_t		offset;
	uint16_t		charEnd;
	LedMatrixColor		currentColor;
	LedMatrixFont		&font;
};



class LedMatrix
{
public:
	LedMatrix(AbstractLedMatrixFrameBuffer &fb, LedMatrixFont &font) 
		: defaultFont(font), frameBuffer(&fb), animation((LedMatrixAnimation*)NULL)
	{
	}

	void init() {
		frameBuffer->init();
	}

	inline void setChar(char c, LedMatrixColor &color) {
		frameBuffer->setChar(c, color, defaultFont);
	}

	void clear() {
		LedMatrixColor blank(0,0,0);
		clear(blank);
	}

	void clear(LedMatrixColor &color) {
		frameBuffer->clear(color);
	}

	inline void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, LedMatrixColor &color) {
		for(uint16_t x = x1; x <= x2; x++) {
			for(uint16_t y = y1; y <= y2; y++) {
				frameBuffer->putPixel(x, y, color);
			}
		}
	}

	bool update() {
		bool frameDone = frameBuffer->tick();
		if( frameDone && nextFrameBuffer ) {
			frameBuffer = nextFrameBuffer;
			nextFrameBuffer = NULL;
		}
		if( frameDone && animation != NULL ) {
			animCountdown--;
			if( animCountdown == 0 ) {
				animTick();
				animCountdown = animInterval;
			}
		}
		return frameDone;
	}

	void animTick() {
		if( animation != NULL ) {
			animation->update(*frameBuffer);
		}
	}

	void setAnimation(LedMatrixAnimation *animation, uint8_t interval) {
		animInterval = interval;
		if( animInterval == 0 ) {
			animInterval = 1;
		}
		animCountdown = animInterval;
		this->animation = animation;
	}

	void setAnimationInterval(uint8_t interval) {
		animInterval = interval;
		if( animInterval == 0 ) {
			animInterval = 1;
		}
		animCountdown = animInterval;
	}

	uint8_t getAnimationInterval();

	AbstractLedMatrixFrameBuffer &getFrameBuffer() {
		return *frameBuffer;
	}

	void clearAnimation() {
		animation = NULL;
	}

	void changeFrameBuffer(AbstractLedMatrixFrameBuffer *fb) {
		nextFrameBuffer = fb;
	}

private:
	LedMatrixFont 			&defaultFont;
	AbstractLedMatrixFrameBuffer	*frameBuffer; 
	AbstractLedMatrixFrameBuffer	*nextFrameBuffer; 
	LedMatrixAnimation		*animation;
	uint8_t				animInterval;
	uint8_t				animCountdown;

};

#endif
