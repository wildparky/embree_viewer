#include <iostream>
#include <stdexcept>

#include <SDL/SDL.h>

#include "scene.h"
#include "maths.h"

#define SCREEN_SIZE	1024
#define SCREEN_BPP	32

namespace {
void set_pixel(SDL_Surface *surface, int x, int y, const Vec3& pixel) {
	Uint32 *target_pixel = (Uint32*)((Uint8 *)surface->pixels + y * surface->pitch +
	                       x * sizeof(* target_pixel));

	Uint32 value = ((Uint32)(pixel.x * 255) << 16) + ((Uint32)(pixel.y * 255) << 8) +
		((Uint32)(pixel.z * 255) << 0) + (255 << 24);

	*target_pixel = value;
}
}

int main(int /*argc*/, char* /*argv*/[]) {
	// SDL initialisation
	if(SDL_Init(SDL_INIT_EVERYTHING))
		throw std::runtime_error(SDL_GetError());

	// make the window
	SDL_Surface* screen = SDL_SetVideoMode(SCREEN_SIZE, SCREEN_SIZE, SCREEN_BPP, SDL_SWSURFACE);

	{
		// make the scene
		Scene scene;

		// add a bunch of spheres
		scene.addSphere(Vec3{ -2, 0, 0}, 0.3);
		scene.addSphere(Vec3{0, 0, 0}, 0.3);
		scene.addSphere(Vec3{2, 0, 0}, 0.3);

		scene.addSphere(Vec3{0, -2, 0}, 0.3);
		scene.addSphere(Vec3{0, 2, 0}, 0.3);

		scene.commit();

		///////////////////////////

		// the main loop
		unsigned ctr = 0;
		while(1) {
			// render pixels
			for(int y=0;y<screen->h;++y)
				for(int x=0;x<screen->w;++x) {
					const float xf = ((float)x / (float)screen->w - 0.5f) * 2.0f;
					const float yf = ((float)y / (float)screen->h - 0.5f) * 2.0f;

					const Vec3 color = scene.renderPixel(xf, yf);

					set_pixel(screen, x, y, color);
				}

			// show the result by flipping the double buffer
			SDL_Flip(screen);

			// exit condition
			{
				SDL_Event event;
				if(SDL_PollEvent(&event) && event.type == SDL_QUIT)
					break;
			}

			++ctr;
		}
	}

	// clean up
	SDL_FreeSurface(screen);
	SDL_Quit();

	return 0;
}
