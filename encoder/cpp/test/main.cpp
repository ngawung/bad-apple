#include <bits/stdint-uintn.h>
#include <iostream>
#include <fstream>
#include "BMPlib.h"
// #include "lzss.hpp"

#define RGB8(r,g,b)  ((r)>>3) | (((g)>>3)<<5) | (((b)>>3)<<10)
int main() {

	BMPlib::BMP bmp;
	bmp.Read("../../assets/out/out_240.bmp");

	unsigned char* pixel = bmp.GetPixelBuffer();
	uint16_t totalSize = bmp.GetWidth()*bmp.GetHeight();

	// uint16_t output[totalSize];
	// uint32_t counter = 0;
	std::ofstream out("output", std::ios::binary);
	
	for (int i=0; i<totalSize*3; i+=3) {
		uint16_t color = RGB8(pixel[i], pixel[i+1], pixel[i+2]);

		// output[counter] = color;
		// counter++;
		out.write((char*)&color, sizeof(color));
	}

	// unsigned char* output2;

	// LZS_Encode((unsigned char*)&output, totalSize);

	// std::cout << sizeof(*output2) << "\n";

	out.close();

	return 0;
}