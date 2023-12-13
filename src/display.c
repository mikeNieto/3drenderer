#include "display.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *color_buffer_texture = NULL;
uint32_t *color_buffer = NULL;
int window_width = 0;
int window_height = 0;

bool initialize_window(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Get from SDL the max width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    window_width = display_mode.w;
    window_height = display_mode.h;

    printf("w = %d, h = %d\n", window_width, window_height);

    // Create SDL Window
    window = SDL_CreateWindow(
        NULL, // Screen will be in full screen for that reason we don't have window title
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_BORDERLESS);

    if (!window)
    {
        fprintf(stderr, "Error creating SDL Window.\n");
        return false;
    }

    // Create SDL renderer
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        fprintf(stderr, "Error creating SDL renderer\n");
        return false;
    }

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    return true;
}

void draw_grid(int size)
{
    for (int y = 0; y < window_height; y += size)
    {
        for (int x = 0; x < window_width; x += size)
        {
            color_buffer[window_width * y + x] = 0xFF666666;
        }
    }
}

void draw_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || x > window_width || y < 0 || y > window_height)
        return;

    color_buffer[window_width * y + x] = color;
}

void draw_line_DDA_algorithm(int x0, int y0, int x1, int y1, uint32_t color)
{
    int delta_x = x1 - x0;
    int delta_y = y1 - y0;

    int longest_side_lenght = abs(delta_x) >= abs(delta_y) ? abs(delta_x) : abs(delta_y);

    // Find how much we should increment in both x and y each step
    float x_inc = delta_x / (float)longest_side_lenght;
    float y_inc = delta_y / (float)longest_side_lenght;

    float current_x = x0;
    float current_y = y0;

    for (int i = 0; i < longest_side_lenght; i++)
    {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    draw_line_DDA_algorithm(x0, y0, x1, y1, color);
}

void draw_rect(int x, int y, int width, int height, uint32_t color)
{
    for (int i = y; i < y + height; i++)
    {
        for (int j = x; j < x + width; j++)
        {
            int current_x = j;
            int current_y = i;
            draw_pixel(current_x, current_y, color);
        }
    }
}

void render_color_buffer(void)
{
    SDL_UpdateTexture(
        color_buffer_texture,
        NULL,
        color_buffer,
        (int)(window_width * sizeof(uint32_t)));

    SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}

void clear_color_buffer(uint32_t color)
{
    for (int i = 0; i < window_height * window_width; i++)
    {
        color_buffer[i] = color;
    }
}

void destroy_window(void)
{
    // Destroy all SDL objects
    SDL_DestroyTexture(color_buffer_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}