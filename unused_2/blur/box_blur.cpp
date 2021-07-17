#include <iostream>
#include <string>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

//Input data
int window_width = 800;
int window_height = 600;
std::string input_image_name = "../assets/walls/mesa.bmp";
std::string output_image_name = "out.bmp";
int blur_extent = 1;

//SDL structures
SDL_Window* window = NULL;
SDL_Event input;
SDL_Surface* windowSurface = NULL;
SDL_Surface* imageSurface = NULL;
 
bool quit = false;

void getInput(std::string inputFileName)
{
    std::ifstream inputFile;
    inputFile.open(inputFileName);
 
    if (inputFile.fail())
    {
        std::cerr << "ERROR: Failed to open " << inputFileName << "." << std::endl;     }   else    {       while (inputFile.good())        {           std::string tag;            inputFile >> tag;
 
            if (tag == "[window_width]") inputFile >> window_width;
            else if (tag == "[window_height]") inputFile >> window_height;
            else if (tag == "[input_image_name]") inputFile >> input_image_name;
            else if (tag == "[output_image_name]") inputFile >> output_image_name;
            else if (tag == "[blur_extent]") inputFile >> blur_extent;
        }
    }
 
    inputFile.close();
}

void init()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("BoxBlurrer",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height, SDL_WINDOW_SHOWN);
    windowSurface = SDL_GetWindowSurface(window);
    imageSurface = IMG_Load(input_image_name.c_str());
}

void blur() //This manipulates with SDL_Surface and gives it box blur effect
{
    for (int y = 0; y < imageSurface->h; y++)
    {
        for (int x = 0; x < (imageSurface->pitch / 4); x++)
        {
            Uint32 color = ((Uint32*)imageSurface->pixels)[(y * (imageSurface->pitch / 4)) + x];
 
            //SDL_GetRGBA() is a method for getting color
            //components from a 32 bit color
            Uint8 r = 0, g = 0, b = 0, a = 0;
            SDL_GetRGBA(color, imageSurface->format, &r, &g, &b, &a);
 
            Uint32 rb = 0, gb = 0, bb = 0, ab = 0;
 
            //Within the two for-loops below, colors of adjacent pixels are added up
 
            for (int yo = -blur_extent; yo <= blur_extent; yo++)
            {
                for (int xo = -blur_extent; xo <= blur_extent; xo++)                 {                   if (y + yo >= 0 && x + xo >= 0
                        && y + yo < imageSurface->h && x + xo < (imageSurface->pitch / 4)
                        )
                    {
                        Uint32 colOth = ((Uint32*)imageSurface->pixels)[((y + yo)
                                                * (imageSurface->pitch / 4)) + (x + xo)];
 
                        Uint8 ro = 0, go = 0, bo = 0, ao = 0;
                        SDL_GetRGBA(colOth, imageSurface->format, &ro, &go, &bo, &ao);
 
                        rb += ro;
                        gb += go;
                        bb += bo;
                        ab += ao;
                    }
                }
            }
 
            //The sum is then, divided by the total number of
            //pixels present in a block of blur radius
 
            //For blur_extent 1, it will be 9
            //For blur_extent 2, it will be 25
            //and so on...
 
            //In this way, we are getting the average of
            //all the pixels in a block of blur radius
 
            //(((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)) calculates
            //the total number of pixels present in a block of blur radius
 
            r = (Uint8)(rb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            g = (Uint8)(gb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            b = (Uint8)(bb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            a = (Uint8)(ab / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
 
            //Bit shifting color bits to form a 32 bit proper colour
            color = (r) | (g << 8) | (b << 16) | (a << 24);           ((Uint32*)imageSurface->pixels)[(y * (imageSurface->pitch / 4)) + x] = color;
        }
    }
}

void update()
{
    //Putting surface onto the screen
    SDL_BlitSurface(imageSurface, NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
}

void setOutput() //For saving image as PNG
{
    if (output_image_name != "NULL")
        IMG_SavePNG(imageSurface, output_image_name.c_str());
}

void clear()
{
    SDL_FreeSurface(imageSurface);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

int main()
{
    getInput("../assets/walls/mesa.bmp");
    init();
    blur();
    while (!quit)
    {
        while (SDL_PollEvent(&input) > 0)
            if (input.type == SDL_QUIT) quit = true;
        update();
    }
    setOutput();
    clear();
    return 0;
}
