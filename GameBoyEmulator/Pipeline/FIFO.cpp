#include "FIFO.h"

void FIFO::push(uint8_t pixel) {
	pixels.push(pixel);
}

uint8_t FIFO::pop() {
	if(pixels.empty())
		return 0;
	
	uint8_t& pixel = pixels.front();
	pixels.pop();
	
	return pixel;
}

// https://stackoverflow.com/questions/709146/how-do-i-clear-the-stdqueue-efficiently
void FIFO::clear() {
	std::queue<uint8_t> empty;
	std::swap(pixels, empty);
}
