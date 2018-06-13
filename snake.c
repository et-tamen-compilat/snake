#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "types.h"
#include "led-matrix-c.h"

//Checks to see if two points are equal
bool point_equal(point_t p1, point_t p2){
  return (p1.x == p2.x && p1.y == p2.y);
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

bool illegal_direction(point_t p, direction curr_d, direction new_d) {
  return (
      (curr_d == RIGHT && new_d == LEFT) ||
      (curr_d == LEFT && new_d == RIGHT) ||
      (curr_d == UP && new_d == DOWN) ||
      (curr_d == DOWN && new_d == UP)
      );
}

//Checks to see if two nodes are equal
bool node_equal(node_t n1, node_t n2){
  return (point_equal(n1.point, n2.point) && n1.next == n2.next);
}

// Checks to see if point occurs in snake
bool intersects(snake_t s, point_t p){
  node_t *curr = s.head;
  while (curr != NULL) {
    if (point_equal(curr->point, p)) {
      printf("(%i, %i) (%i, %i)\n", p.x, p.y, curr->point.x, curr->point.y);
      return true;
    } 
    curr = curr->next;
  }
  return false;
}

void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_t *c) {
  led_canvas_clear(canvas);
  node_t *current = s->head;
  while (current != NULL) {
    point_t *p = &(current->point);
    printf("%i %i %i %i %i\n", p->x, p->y, c->r, c->g, c->b);
    led_canvas_set_pixel(canvas, p->x, p->y, c->r, c->g, c->b);
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

bool perform_move(snake_t *snake, direction d) {
  if (out_bounds(snake->tail->point, d)) {
    return false; 
  }
  //  printf("%p\n", snake->head);
  snake->head = snake->head->next;
  point_t p = direct_point(snake->tail->point, d);
  if (intersects(*snake, p)) {
    return false;
  }
  node_t *nova = malloc(sizeof(node_t));
  *nova = (node_t) { p, NULL };
  snake->tail->next = nova;
  snake->tail = nova;
  return true;
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



input input_init(struct input_event e) {
  input i;
  if (e.type == 3 && e.code == 1 && e.value == 0) {
    i = I_UP;
  } else if (e.type == 3 && e.code == 1 && e.value == 255) {
    i = I_DOWN;
  } else if (e.type == 3 && e.code == 0 && e.value == 0) {
    i = I_LEFT;
  } else if (e.type == 3 && e.code == 0 && e.value == 255) {
    i = I_RIGHT;
  } else if (e.value == 589826) {
    i = I_A;
  } else if (e.value == 589825) {
    i = I_B;
  } else if (e.value == 589833) {
    i = I_SELECT;
  } else {
    printf("Invalid input event value: %i\n", e.value);
  }
  return i;
}

static long getMilliseconds() {
  struct timeval t;
  gettimeofday(&t, NULL); 
  return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

#define MAX(a, b) (a < b ? b : a)

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
  /*Creates an extra buffer to draw to before swapping on refresh*/
  offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);
  led_canvas_get_size(offscreen_canvas, &width, &height);
  fprintf(stderr, "Size: %dx%d. Hardware gpio mapping: %s\n",
      width, height, options.hardware_mapping);
  /* Now, we swap the canvas. We give swap_on_vsync the buffer we
   * just have drawn into, and wait until the next vsync happens.
   * we get back the unused buffer to which we'll draw in the next
   * iteration.
   */
  //snake_t *snake = malloc(sizeof(snake));
  snake_t *snake = create_snake();
  colour_t *colour = malloc(sizeof(colour));
  colour->r = 255;
  colour->g = colour->b = 0;
  direction d = RIGHT;
  /*  for (int i = 0; i < 1000; i++) {
      draw_snake(offscreen_canvas, snake, colour);

      offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
      }*/

  /*
   * Make sure to always call led_matrix_delete() in the end to reset the
   * display. Installing signal handlers for defined exit is a good idea.
   */
  int multiplier = 5;
  draw_snake(offscreen_canvas, snake, colour);
  offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  int fd = open("/dev/input/by-id/usb-0810_usb_gamepad-event-joystick", O_RDONLY);
  struct pollfd p = (struct pollfd) { fd, POLLIN };
  struct input_event *garbage = malloc(sizeof(struct input_event)*5);
  long time = getMilliseconds() + INTERVAL;
  while (true) {
    int poll_res = poll(&p, 1, (int) MAX(time - getMilliseconds(), 0));
    if (poll_res == 0) {
      printf("Timed out %ld\n", getMilliseconds());
      if (perform_move(snake, d)) {
        draw_snake(offscreen_canvas, snake, colour);
        offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
        time += INTERVAL * multiplier;
        continue;
      } else {
        break;
      }
    }  
    struct input_event e;
    input i;
    read(fd, &e, sizeof(struct input_event));
    // Saves input direction
    i = input_init(e);
    printf("%i\n", i);
    // Skips next 3 or 5 input events depending on command
    // Direction command
    if (e.type == 3) {
      read(fd, garbage, sizeof(struct input_event));
      read(fd, garbage, sizeof(struct input_event));
      read(fd, garbage, sizeof(struct input_event));
      if (!(illegal_direction(snake->tail->point, d, i))) {
        d = i;
      }
    }
    // A, B or Select command
    else {
      read(fd, garbage, sizeof(struct input_event)*5);
    }
    //Changes speed of snake
    if (i = I_B && multiplier > 1) {
      multiplier--;
    } else if (i = I_A && multiplier < 10) {
      multiplier++;
    }
    printf("%i %i %i\n", e.type, e.code, e.value);
  }
  free(garbage);
  led_matrix_delete(matrix);
  return EXIT_SUCCESS;
}

