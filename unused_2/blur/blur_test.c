#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}


SDL_Surface* Filter(SDL_Surface* source)
{
    SDL_Surface *target;
    int x, y;

    // if( source->flags & SDL_SRCCOLORKEY )
    if (0) {
        target = SDL_CreateRGBSurface( SDL_SWSURFACE, source->w ,source->h, source->format->BitsPerPixel, source->format->Rmask, source->format->Gmask, source->format->Bmask, 0 );
    }
    else
        {target = SDL_CreateRGBSurface( SDL_SWSURFACE, source->w ,source->h, source->format->BitsPerPixel, source->format->Rmask, source->format->Gmask, source->format->Bmask, source->format->Amask );}

    for(y=0; y<source->h; ++y){
        for(x=0; x<source->w; ++x)
        {
           unsigned a = getpixel(source,x-1, y-1);
           unsigned b = getpixel(source,x  , y-1);
           unsigned c = getpixel(source,x+1, y-1);

           unsigned d = getpixel(source,x-1, y);
           unsigned z = getpixel(source,x  , y);
           unsigned e = getpixel(source,x+1, y);

           unsigned f = getpixel(source,x-1, y+1);
           unsigned g = getpixel(source,x  , y+1);
           unsigned h = getpixel(source,x+1, y+1);

           unsigned pixels[9] = {a, b, c, d, z, e, f, g, h};
           unsigned sums[3];

           for (int i = 0; i < 9; i++) {
                unsigned pixel = pixels[i];
                Uint8 rt, gt, bt;
                SDL_GetRGB(pixel, source->format, &rt, &gt, &bt);
                sums[0] += rt;
                sums[1] += gt;
                sums[2] += bt;
           }

           sums[0] /= 9;
           sums[1] /= 9;
           sums[2] /= 9;

           unsigned avg = SDL_MapRGB(target->format, sums[0], sums[1], sums[2]);

           // int avg = (a+b+c + d+z+e + f+g+h)/9;

           putpixel(target,x, y, avg);
        }
    }
return target;
}

int main(void) {
   SDL_Surface* const image = SDL_LoadBMP("../assets/walls/mesa.bmp");
   SDL_Surface* blurred = Filter(image);
   SDL_SaveBMP(blurred, "out.bmp");
}
