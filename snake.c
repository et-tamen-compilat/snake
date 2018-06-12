#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "led-matrix-c.h"

//Checks to see if two points are equal
bool point_equal(point_t p1, point_t p2){
  return (p1.x == p2.x && p1.y == p2.y);
}

//Checks to see if two nodes are equal
bool node_equal(node_t n1, node_t n2){
  return (point_equal(n1.point, n2.point) && n1.next == n2.next);
}

// Checks to see if point occurs in snake
bool intersects(snake_t s, point_t p){
  node_t *curr = s.head;
  while (!node_equal(*curr, *s.tail)){
    if (point_equal(curr->point, p)){
      return true;
    } 
    curr = curr->next;
  }
  return false;
}

void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_t *c) {
  node_t *current = s->head;
  while (current != NULL) {
    point_t *p = malloc(sizeof(point_t));
    *p = current->point;
    led_canvas_set_pixel(canvas, p->x, p->y, c->r, c->g, c->b);
    free(p);
    current = current->next;
  }
}

// applies movement of one space to point in direction 
point_t direct_point(point_t p, direction d){
  switch (d){
    case UP:
      p.y = p.y - 1;
      break;
    case DOWN:
      p.y = p.y + 1;
      break;
    case LEFT:
      p.x = p.x - 1;
      break;
    case RIGHT:
      p.x = p.x + 1;
      break;
  }
  return p;
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


// Returns true if will be out of bounds
bool out_bounds(point_t p, direction d){
  return (
      (p.y == 0 && d == UP) ||
      (p.y == MAXHEIGHT - 1 && d == DOWN) ||
      (p.x == 0 && d == LEFT) ||
      (p.x == MAXWIDTH - 1 && d == RIGHT)
      );
}

snake_t *create_snake(){
  snake_t *s = malloc(sizeof(snake_t));
  node_t *tail = malloc(sizeof(node_t));
  *tail = (node_t) {{4,0}, NULL};
  node_t *b3 = malloc(sizeof(node_t));
  *b3 = (node_t) {{3,0}, tail};
  node_t *b2 = malloc(sizeof(node_t));
  *b2 = (node_t) {{2,0}, b3};
  node_t *b1 = malloc(sizeof(node_t));
  *b1 = (node_t) {{1,0}, b2};
  node_t *head = malloc(sizeof(node_t));
  *head = (node_t) {{0,0}, b1};
  s->head= head;
  s->tail = tail;
  return s;
}



