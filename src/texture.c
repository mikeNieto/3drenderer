#include "texture.h"
#include <stdio.h>

int texture_width = 64;
int texture_height = 64;

upng_t *png_texture = NULL;
uint32_t *mesh_texture = NULL;

void load_png_texture_data(char *filename) {
  png_texture = upng_new_from_file(filename);

  if (png_texture != NULL) {
    upng_decode(png_texture);

    if (upng_get_error(png_texture) == UPNG_EOK) {
      mesh_texture = (uint32_t *)upng_get_buffer(png_texture);
      texture_width = upng_get_width(png_texture);
      texture_height = upng_get_height(png_texture);

      for (int i = 0; i < texture_width * texture_height; i++) {
        uint32_t color = mesh_texture[i];
        uint32_t a = (color & 0xFF000000);
        uint32_t r = (color & 0x00FF0000) >> 16;
        uint32_t g = (color & 0x0000FF00);
        uint32_t b = (color & 0x000000FF) << 16;

        mesh_texture[i] = (a | r | g | b);
      }
    }
  }
}
