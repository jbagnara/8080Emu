#include "display.h"

SDL_Window* win;
SDL_Renderer* renderer;
SDL_Event events;

uint8_t i_d;		//display interrupt
uint8_t d_bus;		//display interrupt bus

int pollEvent(){
	while(SDL_PollEvent(&events)){
		if(events.type == SDL_QUIT){
			SDL_Quit();
			return -1;
		}
	}
	return 0;
}

void createWindow(){
	SDL_CreateWindowAndRenderer(1000, 1000, 0, &win, &renderer);
}

void closeWindow(){
	SDL_Quit();
}

void* drawScreen(void* p){
	uint8_t* fb = (uint8_t*)p;
	
	while(1){
		struct timespec ts;
		timespec_get(&ts, TIME_UTC);
		long start = ts.tv_nsec;

		d_bus = 1;
		i_d = 1;

		SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 224);
		SDL_SetRenderTarget(renderer, texture);
      	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      	SDL_RenderClear(renderer);
      	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
	
		for(int x=0; x<32; x++){
			for(int bit=0; bit<8; bit++){
				for(int y=0; y<224; y++){
					if(fb[x+32*y] & (1 << bit)){
						SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
						SDL_RenderDrawPoint(renderer, x*8+bit, y);
					} 
				}
			}
		}
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopyEx(renderer, texture, NULL, NULL, 270, NULL, SDL_FLIP_NONE);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);

		d_bus = 2;
		i_d = 1;

		long current, timedif;;
		do {
			timespec_get(&ts, TIME_UTC);
			current = ts.tv_nsec;
			timedif = current - start;
			if(timedif < 0)
				timedif += start;
		} while(timedif < DISPLAY_SPD_NSEC);
	}
}
