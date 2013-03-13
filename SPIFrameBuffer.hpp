#ifndef _SPIFrameBuffer_HPP
#define _SPIFrameBuffer_HPP

namespace LedMatrixNS {

class SPIFrameBufferConfigConcept {
	static const uint16_t Rows = 16;
	static const uint16_t Cols = 16;
	static const uint16_t Levels = 32;
};

template<typename Config>
class SPIFrameBuffer: public AbstractLedMatrixFrameBuffer
{
public:
	inline uint16_t getRowCount() {return Config::Rows;}
	inline uint16_t getColCount() {return Config::Cols;}
	inline uint16_t getLevels() {return Config::Levels;}

	void setChar(char c, LedMatrixColor &color, LedMatrixFont &font) {}

	bool tick() {
		UARTprintf("Start\r\n");

		Config::SpiSelect();

		Config::SpiSend(0x5);
		Config::SpiSend(0);
		Config::SpiSend(0);

		Config::SpiDeSelect();
		
		UARTprintf("End\r\n");
		// We always update an entire frame at a time
		return true;
	}

	void clear(LedMatrixColor &color) {
		for(uint16_t y=0; y<getColCount(); y++) {
			for(uint16_t x=0; x<getRowCount(); x++) {
				putPixel(x, y, color);
			}
		}
	}

	void clear() {
		LedMatrixColor white(0,0,0);
		clear(white);
	}

	void fillRow(uint16_t row, LedMatrixColor &color) {
		for(uint16_t x=0; x<getColCount(); x++) {
			putPixel(x, row, color);
		}
	}

	void init() {
		Config::SPIInit();
	}

	void putPixel(uint16_t x, uint16_t y, LedMatrixColor &color) {
		putPixel(x, y, color.getValue());
	}

	void putPixel(uint16_t x, uint16_t y, uint16_t color) {
		data[x][y] = color;
	}

	uint16_t getPixel(uint16_t x, uint16_t y) {
		return data[x][y];
	}

	void reset() {
	}

private:
	uint16_t data[Config::Cols][Config::Rows];
};

}

#endif
