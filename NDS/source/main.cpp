#include <nds.h>
#include <filesystem.h>
#include <iostream>
#include <fstream>
#include <maxmod9.h>
#include <vector>

#define TIMER_SPEED (BUS_CLOCK/1024)

FILE* file = 0;
std::ifstream in;

uint16_t blockSize;
char buffer[256*192*2];
char buffer2[256*256*2];

void VBlankProc() {
	in.read((char*)&blockSize, sizeof(blockSize));
	in.read(buffer, blockSize);
}

mm_word on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	if(file){
		size_t samplesize = 1;
		switch(format){
			case MM_STREAM_8BIT_MONO: samplesize = 1; break;
			case MM_STREAM_8BIT_STEREO: samplesize = 2; break;
			case MM_STREAM_16BIT_MONO: samplesize = 2; break;
			case MM_STREAM_16BIT_STEREO: samplesize = 4; break;
		}
	
		int res = fread(dest,samplesize,length,file);
		if(res){
			length = res;
		} else {
			mmStreamClose();
			fclose(file);
			length = 0;
		}
	}
	return length;
}

int main(void) {
	consoleDebugInit(DebugDevice_NOCASH);

	videoSetMode(MODE_FB0);
	vramSetBankA(VRAM_A_LCD);

	std::cerr << "<=== init ===>" << std::endl;
	

	// nitrofiles initialization
	if (nitroFSInit(NULL)) {
		chdir("nitro:/");
		std::cerr << "nitrofs init success" << std::endl;
	} else {
		consoleDemoInit();
		std::cout << "cannot init nitrofs" << std::endl;
		while(1) {
			// freezee
		}
	}

	in.open("output", std::ios::binary);

	uint16_t totalsize;
	char buf[256*192*2];
	char buf2[256*256*2];

	in.read((char*)&totalsize, sizeof(totalsize));
	in.read(buf, totalsize);

	swiDecompressLZSSWram(buf, buf2);
	dmaCopyWordsAsynch(3, buf2, VRAM_A, 256*192*2);

	// for (int i=0; i<192*256; i++) {
	// 	VRAM_A[i] = buf[i];
	// }

	while(1) {
		// loop
	}

	mmInitDefault((char*)"soundbank.bin");
	file = fopen("music.raw","rb");

	mm_stream mystream;
	mystream.sampling_rate	= 22050;					// sampling rate = 25khz
	mystream.buffer_length	= 1200;						// buffer length = 1200 samples
	mystream.callback		= on_stream_request;		// set callback function
	mystream.format			= MM_STREAM_16BIT_MONO;		// format = stereo 16-bit
	mystream.timer			= MM_TIMER0;				// use hardware timer 0
	mystream.manual			= false;						// use manual filling
	mmStreamOpen( &mystream );

	irqSet(IRQ_VBLANK, VBlankProc);

	while(1) {
		// loop
	}

	in.open("data_compress", std::ios::in | std::ios::binary);

	while(1) {
		swiDecompressLZSSWram(buffer, buffer2);	
		dmaCopyWordsAsynch(3, buffer2, VRAM_A, 256*192*2);
		
		swiWaitForVBlank();
	}

	// crashh
	return 0;
}
