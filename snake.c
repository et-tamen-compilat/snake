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
#include <signal.h>
#include <time.h>
#include "types.h"
#include "led-matrix-c.h"

#define NUM_WALLS 20
volatile sig_atomic_t done = 0;
volatile struct RGBLedMatrix *matrix;

void term(int signum) {
  printf("Hello\n");
//  led_matrix_delete(matrix);
  done = 1;
}

int TIME_AFTER_DEATH = -1;
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

bool illegal_direction(direction curr_d, direction new_d) {
  return (
      (curr_d == RIGHT && (new_d == LEFT || new_d == RIGHT)) ||
      (curr_d == LEFT && (new_d == RIGHT || new_d == LEFT)) ||
      (curr_d == UP && (new_d == DOWN || new_d == UP)) ||
      (curr_d == DOWN && (new_d == UP || new_d == DOWN))
      );
}

//Checks to see if two nodes are equal
bool node_equal(node_t n1, node_t n2){
  return (point_equal(n1.point, n2.point) && n1.next == n2.next);
}


bool food_wall(point_t point, int **collision_map){
  return (collision_map[point.x][point.y]);
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

void add_wall_to_map(int **map, wall_t *wall) {
  for (int i = 0; i < wall->length; i++) {
    switch (wall->direction) {
      case UP: 
        map[wall->start.x][wall->start.y - i] = 1;
        break;
      case DOWN:
        map[wall->start.x][wall->start.y + i] = 1;
        break;
      case LEFT:
        map[wall->start.x - i][wall->start.y] = 1;
        break;
      case RIGHT:
        map[wall->start.x + i][wall->start.y] = 1;
        break;
    }
  }
}


int **create_collision_map(wall_t *wall_arr) {
  int **map = malloc(sizeof(int *) * MAX_WIDTH);
  for (int i = 0; i < MAX_WIDTH; i++) {
    map[i] = calloc(MAX_HEIGHT, sizeof(int));
  }
//  memset(map, 0, sizeof(int) * MAX_WIDTH * MAX_HEIGHT);
  for (int i = 0; i < NUM_WALLS; i++) {
    add_wall_to_map(map, &(wall_arr[i]));
  }
  return map; 
}


point_t get_food(snake_t *snake, wall_t* wall_arr) {
  point_t point;
  do {
    point.x = rand() % MAX_WIDTH;
    point.y = rand() % MAX_HEIGHT;
  } while(intersects(*snake, point) || food_wall(point, create_collision_map(wall_arr)));
  return point;
}

colour_t get_default(int pos) {
  colour_t b = {0, 255, 0};
  return b;
}

colour_t colours[] = {
  { 255, 0, 0 },
  { 0, 0, 255 },
  { 175, 65, 244 },
  { 65, 244, 238 },
  { 232, 126, 99 },
  { 249, 154, 222 },
  { 192, 249, 154 }
};

colour_t multicolour(int pos) {
  return colours[pos % 7];
}

//returns random int in range
int get_rand_int(int min, int max) {
  return rand() % (max + 1 - min) + min;
}

direction get_rand_dir() {
  int num = get_rand_int(0, 1);
  switch (num) {
    case 0: return DOWN; break;
    case 1: return RIGHT; break;
    default: break;
  }
}

// Makes sure the wall doesn't go off of the map
int calc_length(point_t start, direction d) {
  switch (d) {
    case UP: 
      return start.y > WALL_MAX_LEN ? WALL_MAX_LEN : start.y;
      break;
    case DOWN:
      return MAX_HEIGHT - start.y > WALL_MAX_LEN ? WALL_MAX_LEN :
        MAX_HEIGHT - start.y;
      break;
    case LEFT:
      return start.x > WALL_MAX_LEN ? WALL_MAX_LEN : start.x;
      break;
    case RIGHT:
      return MAX_WIDTH - start.x > WALL_MAX_LEN ? WALL_MAX_LEN :
        MAX_WIDTH - start.x;
      break;
  }
}

wall_t *create_wall() {
  wall_t *wall = malloc(sizeof(wall_t));
  wall->length = get_rand_int(WALL_MIN_LEN, WALL_MAX_LEN);
  wall->direction = get_rand_dir();

  int x = MAX_WIDTH, y = MAX_HEIGHT;
  switch (wall->direction) {
    case RIGHT: x -= wall->length;
    case DOWN: y -= wall->length;
  }

  wall->start = (point_t) {get_rand_int(0, x), get_rand_int(0, y)};
  wall->colour = WALL_COLOUR;
  return wall;
}

void draw_wall(struct LedCanvas *canvas, wall_t *wall) {
  int x0 = wall->start.x;
  int y0 = wall->start.y;
  switch (wall->direction) {
    case UP: 
      draw_line(canvas, x0, y0, x0, y0 - wall->length, WALL_COLOUR.r,
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
    case DOWN:
      draw_line(canvas, x0, y0, x0, y0 + wall->length, WALL_COLOUR.r,
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
    case RIGHT:
      draw_line(canvas, x0, y0, x0 + wall->length, y0, WALL_COLOUR.r, 
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
    case LEFT:
      draw_line(canvas, x0, y0, x0 - wall->length, y0, WALL_COLOUR.r,
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
  }
}


wall_t *create_map() {
  int num_walls = NUM_WALLS;
  wall_t *wall_arr = malloc(sizeof(wall_t) * num_walls);
  for (int i = 0; i < num_walls; i++) {
    bool created = false;
    while (!created) {
      wall_t *wall;
      wall_arr[i] = *(create_wall());
      if (wall->start.x > SNAKE_SAFETY.x && wall->start.y > SNAKE_SAFETY.y) {
        created = true;
      } else {
        free(wall);
      }
    }
  }
  return wall_arr;
}
//Check map for food, check it's not on top of a wall basically
//In main, put this in a while look when generating food
void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_function_t *c, point_t food, int k, wall_t *walls) {
  led_canvas_clear(canvas);
  for (int i = 0; i < NUM_WALLS; i++) {
    draw_wall(canvas, &walls[i]);
  }
  node_t *current = s->head;
  int len = 0;
  while (current != NULL) {
    current = current->next;
    len++;
  }
  current = s->head;
  int pos = 0;
  while (current != NULL) {
    point_t *p = &(current->point);
    //    printf("%i %i %i %i %i\n", p->x, p->y, c->r, c->g, c->b);
    colour_t k = c(len - pos);
    pos++;
    printf("%i %i %i\n", TIME_AFTER_DEATH, pos, TIME_AFTER_DEATH >= pos);
    if (TIME_AFTER_DEATH >= (len - pos)) {
      led_canvas_set_pixel(canvas, p->x, p->y, 255, 0, 0);
    }
    else {
      led_canvas_set_pixel(canvas, p->x, p->y, k.r, k.g, k.b);
    }  
//    led_canvas_set_pixel(canvas, p->x, p->y, 255, 0, 0);
    current = current->next;
  }
  if (k % 10 == 0 && TIME_AFTER_DEATH == -1) {
    led_canvas_set_pixel(canvas, food.x, food.y, 255, 0, 0);
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

bool perform_move(snake_t *snake, direction d, point_t* food, wall_t *walls) {
  if (out_bounds(snake->tail->point, d)) {
    return false; 
  }
  //  printf("%p\n", snake->head);
  //  snake->head = snake->head->next;
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
    *food = get_food(snake, walls);
    snake->length++;
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
  s->length = 5;
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
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = &term;
  sigaction(SIGINT, &action, NULL);
  
  struct RGBLedMatrixOptions options;
  struct LedCanvas *offscreen_canvas;
  int width, height;
  int x, y, i;
  srand(time(NULL));
  
//  srand(45);

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
  snake_t *snake = create_snake();
  colour_t *colour = malloc(sizeof(colour));
  colour->r = 255;
  colour->g = colour->b = 0;
  direction d = RIGHT;
  /*
   * Make sure to always call led_matrix_delete() in the end to reset the
   * display. Installing signal handlers for defined exit is a good idea.
   */
  int multiplier = 5;
  wall_t *walls = create_map();
  point_t food = get_food(snake, walls);
  draw_snake(offscreen_canvas, snake, &multicolour, food, 0, walls);
  offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  int fd = open("/dev/input/js0", O_RDONLY);
  struct pollfd p = (struct pollfd) { fd, POLLIN };
  struct input_event *garbage = malloc(sizeof(struct js_event)*5);
  long curr_time = getMilliseconds() + (INTERVAL / 10);
  int k = 0;
  while (true) {
    int poll_res = poll(&p, 1, (int) MAX(curr_time - getMilliseconds(), 0));
    if (poll_res == 0) {
      k++;
      printf("Timed out %ld\n", getMilliseconds());
      if (k % 10 == 9) {
        if (!perform_move(snake, d, &food, walls)) {
          TIME_AFTER_DEATH = 0;
          break;
        }
      } 
      draw_snake(offscreen_canvas, snake, &get_default, food, k, walls);
      offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
      curr_time += (INTERVAL / 10) * multiplier;
      continue;
    }  
    struct js_event e;
    read(fd, &e, sizeof(struct js_event));
    input i = input_init(e);
    // Saves input direction
    if (i != 10) {
      read(fd, garbage, sizeof(struct js_event));
      if (i >= 0 && i <= 3) {
        if (!illegal_direction(d, i)) {
          d = i;
        }
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
    //printf("%i %i %i\n", e.type, e.code, e.value);
  }

  for (int i = 0; i < snake->length; i++) {
    draw_snake(offscreen_canvas, snake, &get_default, food, k, walls);
    offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
    TIME_AFTER_DEATH++;
    poll(NULL, 0, INTERVAL);
  }    

  while(!done);

  free(garbage);
  led_matrix_delete(matrix);
  return EXIT_SUCCESS;
}

