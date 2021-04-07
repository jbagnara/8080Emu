#include "display.h"

SDL_Window* win;
SDL_Renderer* renderer;
SDL_Event events;

static int fps = 60;
static int cpuClock = 3125000;

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

void* drawScreen(void* p){
	uint8_t* fb = (uint8_t*)p;
	
	while(1){
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
        SDL_Delay(1/fps); //TODO change this
	}
}
