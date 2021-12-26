#include <nds.h>
#include <filesystem.h>
#include <iostream>
#include <fstream>
#include <maxmod9.h>

#define TIMER_SPEED (BUS_CLOCK/1024)

FILE* file = 0;
std::ifstream in;

uint16_t blockSize;
char buffer[256*192*2];
char buffer2[256*256*2];

bool shouldDraw = false;
bool queueAvaible = false;

void VBlankProc() {
	if (shouldDraw) {
		shouldDraw = false;

		if (queueAvaible) {
			queueAvaible = false;

			dmaCopyWordsAsynch(3, buffer2, VRAM_A, 256*192*2);
		}
	}
}

void TimerTick() {
	shouldDraw = true;
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
	consoleDemoInit();

	videoSetMode(MODE_FB0);
	vramSetBankA(VRAM_A_LCD);

	std::cerr << "<=== init ===>" << std::endl;
	

	// nitrofiles initialization
	if (!nitroFSInit(NULL)) {
		chdir("nitro:/");
		std::cerr << "nitrofs init success" << std::endl;
	} else {
		consoleDemoInit();
		std::cout << "failed to init nitrofs" << std::endl;
		std::cout << "Please launch the game\nfrom nds-bootstrap from twilightmenu" << std::endl;
		while(1) {
			// freezee
		}
	}

	mmInitDefault((char*)"soundbank.bin");
	file = fopen("music.raw","rb");

	mm_stream mystream;
	mystream.sampling_rate	= 22050;					// sampling rate = 25khz
	mystream.buffer_length	= 2400;						// buffer length = 1200 samples
	mystream.callback		= on_stream_request;		// set callback function
	mystream.format			= MM_STREAM_16BIT_MONO;		// format = stereo 16-bit
	mystream.timer			= MM_TIMER0;				// use hardware timer 0
	mystream.manual			= true;						// use manual filling
	mmStreamOpen( &mystream );

	// quick hack... sometime it need to update once?
	mmStreamUpdate(); mmStreamUpdate();

	irqSet(IRQ_VBLANK, VBlankProc);
	timerStart(2, ClockDivider_1024, TIMER_FREQ_1024(30), TimerTick);

	in.open("data_compress", std::ios::in | std::ios::binary);

	while(true) {
		// timerElapsed(2);
		mmStreamUpdate();

		if (!in.eof() && !queueAvaible) {
			in.read((char*)&blockSize, sizeof(blockSize));
			in.read(buffer, blockSize);
			swiDecompressLZSSWram(buffer, buffer2);
			queueAvaible = true;
		}
		
		swiWaitForVBlank();
	}

	// crashh
	return 0;
}
