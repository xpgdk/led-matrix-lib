#ifndef _SPIFrameBuffer_HPP
#define _SPIFrameBuffer_HPP

namespace LedMatrixNS {

class SPIFrameBufferConfigConcept {
	static const uint16_t Rows = 16;
	static const uint16_t Cols = 16;
	static const uint16_t Levels = 32;
};

template<typename Config>
class SPIFrameBuffer
{
public:
	inline uint16_t getRowCount() {return Config::Rows;}
	inline uint16_t getColCount() {return Config::Cols;}
	inline uint16_t getLevels() {return Config::Levels;}

	void setChar(char c, LedMatrixColor &color, LedMatrixFont &font) {}

	bool tick() {

		Config::SpiSelect();

		uint16_t startX = 0;
		uint16_t startY = 0;
		uint16_t endX = getColCount();
		uint16_t endY = getRowCount();

		/* Send frame-buffer */
		Config::SpiSend(0x04);
		Config::SpiSend((startX >> 8) & 0xFF);
		Config::SpiSend(startX & 0xFF);
		Config::SpiSend((startY >> 8) & 0xFF);
		Config::SpiSend(startY & 0xFF);

		Config::SpiSend((endX >> 8) & 0xFF);
		Config::SpiSend(endX & 0xFF);
		Config::SpiSend((endY >> 8) & 0xFF);
		Config::SpiSend(endY & 0xFF);

		for(uint16_t y=startY; y<endY; y++) {
		  for(uint16_t x=startX; x<endX; x++) {
		    uint16_t pixel = getPixel(x, y);
		    Config::SpiSend(pixel & 0xFF);
		    Config::SpiSend((pixel >> 8) & 0xFF);
		  }
		}


		Config::SpiSend(0);
		Config::SpiSend(0);
                
		//Config::SpiDeSelect();


#if 1
		//Config::SpiSelect();
		/* Send flip-command */
		Config::SpiSend(0x5);
		Config::SpiSend(0);
		Config::SpiSend(0);

		Config::SpiDeSelect();
#endif
		
		// We always update an entire frame at a time
		return true;
	}

	void clear(LedMatrixColor &color) {
		clear(color.getValue());
	}
	void clear(uint16_t color) {
		for(uint16_t y=0; y<getRowCount(); y++) {
			for(uint16_t x=0; x<getColCount(); x++) {
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
		if( x < Config::Cols && y < Config::Rows ) {
			data[x][y] = color;
		}
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
