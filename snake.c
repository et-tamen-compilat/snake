#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <linux/joystick.h>
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
      (p.y == MAX_HEIGHT - 1 && d == DOWN) ||
      (p.x == 0 && d == LEFT) ||
      (p.x == MAX_WIDTH - 1 && d == RIGHT)
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

point_t get_food(snake_t *snake) {
  point_t point;
  do {
    point.x = rand() % MAX_WIDTH;
    point.y = rand() % MAX_HEIGHT;
  } while(intersects(*snake, point));
  return point;
}

//returns random int in range
int get_rand_int(int min, int max) {
  return rand() % (max + 1 - min) + min;
}

direction get_rand_dir() {
  int num = get_rand_int(0, 3);
  switch (num) {
    case 0: return UP; break;
    case 1: return DOWN; break;
    case 2: return LEFT; break;
    case 3: return RIGHT; break;
    default: break;
  }
}

int calc_length(point_t start, direction d) {
  switch (d) {
    case UP: 
      return start.y - 1 > WALL_MAX_LEN ? WALL_MAX_LEN : start.y - 1;
      break;
    case DOWN:
      return MAX_HEIGHT - start.y - 1 > WALL_MAX_LEN ? WALL_MAX_LEN :
        MAX_HEIGHT - start.y - 1;
      break;
    case LEFT:
      return start.x - 1 > WALL_MAX_LEN ? WALL_MAX_LEN : start.x - 1;
      break;
    case RIGHT:
      return MAX_WIDTH - start.x - 1 > WALL_MAX_LEN ? WALL_MAX_LEN :
        MAX_WIDTH - start.x - 1;
      break;
  }
}

wall_t *create_wall() {
  wall_t *wall = malloc(sizeof(wall_t));
  wall->start = (point_t) {get_rand_int(WALL_MIN_LEN + 1, MAX_WIDTH - 1),
    get_rand_int(WALL_MIN_LEN + 1, MAX_WIDTH - 1)};
  wall->direction = get_rand_dir();
  wall->length = get_rand_int(WALL_MIN_LEN, calc_length(wall->start, 
        wall->direction));
  wall->colour = WALL_COLOUR;
  return wall;
}

void draw_map(struct LedCanvas *canvas, colour_t *c) {

}

void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_t *c, point_t food) {
  led_canvas_clear(canvas);
  node_t *current = s->head;
  while (current != NULL) {
    point_t *p = &(current->point);
    printf("%i %i %i %i %i\n", p->x, p->y, c->r, c->g, c->b);
    led_canvas_set_pixel(canvas, p->x, p->y, c->r, c->g, c->b);
    current = current->next;
  }
  led_canvas_set_pixel(canvas, food.x, food.y, 0, 255, 0);
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

bool perform_move(snake_t *snake, direction d, point_t* food) {
  if (out_bounds(snake->tail->point, d)) {
    return false; 
  }
  //  printf("%p\n", snake->head);
  snake->head = snake->head->next;
  point_t p = direct_point(snake->tail->point, d);
  bool eq = point_equal(*food, p);
  if (!eq) {
    snake->head = snake->head->next;
    if (intersects(*snake, p)) {
      return false;
    } 
  } 
  node_t *nova = malloc(sizeof(node_t));
  *nova = (node_t) { p, NULL };
  snake->tail->next = nova;
  snake->tail = nova;
  if (eq) {
    *food = get_food(snake);
  }
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


input input_init(struct js_event e) {
  input i = 10;
  if (e.value == -32767 && e.type == 2 && e.number == 1) {
    i = I_UP;
  } else if (e.value == 32767 && e.type == 2 && e.number == 1) {
    i = I_DOWN;
  } else if (e.value == -32767 && e.type == 2 && e.number == 0) {
    i = I_LEFT;
  } else if (e.value == 32767 && e.type == 2 && e.number == 0) {
    i = I_RIGHT;
  } else if (e.value == 1 && e.type == 1 && e.number == 1) {
    i = I_A;
  } else if (e.value == 1 && e.type == 1 && e.number == 0) {
    i = I_B;
  } else if (e.value == 1 && e.type == 1 && e.number ==8) {
    i = I_SELECT;
//  } else if (e.value == 1 && e.type == 1 && e.number ==9) {
//    i = I_START;
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
  point_t food = get_food(snake);
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
  draw_snake(offscreen_canvas, snake, colour, food);
  offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  int fd = open("/dev/input/js0", O_RDONLY);
  struct pollfd p = (struct pollfd) { fd, POLLIN };
  struct input_event *garbage = malloc(sizeof(struct js_event)*5);
  long time = getMilliseconds() + INTERVAL;
  while (true) {
    int poll_res = poll(&p, 1, (int) MAX(time - getMilliseconds(), 0));
    if (poll_res == 0) {
      printf("Timed out %ld\n", getMilliseconds());
      if (perform_move(snake, d, &food)) {
        draw_snake(offscreen_canvas, snake, colour, food);
        offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
        time += INTERVAL * multiplier;
        continue;
      } else {
        break;
      }
    }  
    struct js_event e;
    read(fd, &e, sizeof(struct js_event));
    input i = input_init(e);
    // Saves input direction
    if (i != 10) {
      read(fd, garbage, sizeof(struct js_event));
      if (i >= 0 && i <= 3) {
        d = i;
        printf("D: %i\n", d);
      }
    }
    // A, B or Select command
    //Changes speed of snake
    if (i = I_B && multiplier > 1) {
      multiplier--;
    } else if (i = I_A && multiplier < 10) {
      multiplier++;
    }
    printf("%i %i %i\n", e.value, e.type, e.number);
  }
  free(garbage);
  led_matrix_delete(matrix);
  return EXIT_SUCCESS;
}

