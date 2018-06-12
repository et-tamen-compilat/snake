#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "led-matrix-c.h"

bool are_equal(point_t p1, point_t p2){
  return(p1.x == p2.x && p1.y == p2.y);
}

void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_t *c) {
  node_t *current = &(s->head);
  while (current != NULL) {
    point_t *p = malloc(sizeof(point_t));
    *p = current->point;
    led_canvas_set_pixel(canvas, p->x, p->y, c->r, c->g, c->b);
    free(p);
    current = current->next;
  }
}

int main(int argc, char **argv) {
  struct RGBLedMatrixOptions options;
  struct RGBLedMatrix *matrix;
  struct LedCanvas *offscreen_canvas;
  int width, height;
  int x, y, i;

  memset(&options, 0, sizeof(options));
  options.rows = 32;
  options.chain_length = 1;

  matrix = led_matrix_create_from_options(&options, &argc, &argv);
  if (matrix == NULL) {
    return 1;
  }
  /* Let's do an example with double-buffering. We create one extra
   * buffer onto which we draw, which is then swapped on each refresh.
   * This is typically a good aproach for animations and such.
   */
  offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);
  led_canvas_get_size(offscreen_canvas, &width, &height);
  fprintf(stderr, "Size: %dx%d. Hardware gpio mapping: %s\n",
      width, height, options.hardware_mapping);

    /* Now, we swap the canvas. We give swap_on_vsync the buffer we
     * just have drawn into, and wait until the next vsync happens.
     * we get back the unused buffer to which we'll draw in the next
     * iteration.
     */
    offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  /*
   * Make sure to always call led_matrix_delete() in the end to reset the
   * display. Installing signal handlers for defined exit is a good idea.
   */
  led_matrix_delete(matrix);
  return 0;
}


