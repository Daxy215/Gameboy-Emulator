#pragma once
#include <queue>

class FIFO {
public:
	void push(uint8_t pixel);
	
	uint8_t pop();
	size_t size() const { return pixels.size(); }
	void clear();
	
private:
	std::queue<uint8_t> pixels;
};
