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
#include <unistd.h>
#include "types.h"
#include "led-matrix-c.h"

bool out_bounds(point_t p, direction d);
bool intersects(snake_t s, point_t p);
bool point_equal(point_t p1, point_t p2);
point_t direct_point(point_t p, direction d);
snake_t *create_snake();
wall_t *create_wall();
colour_t multicolour(int pos);
colour_t get_default(int pos);
point_t get_food(snake_t *snake, wall_t* wall_arr);
direction get_direction(snake_t *snake, point_t dest, direction d); 

volatile sig_atomic_t done = 0;
volatile struct RGBLedMatrix *matrix;

void term(int signum) {
  printf("Hello\n");
//  led_matrix_delete(matrix);
  done = 1;
}

int TIME_AFTER_DEATH = -1;

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
  } else if (e.value == 1 && e.type == 1 && e.number ==9) {
    i = I_START;
} else {
  printf("Invalid input event value: %i\n", e.value);
}
return i;
}

void draw_rectangle(struct LedCanvas *canvas, colour_t c, int x0, int y0, int x1, int y1) {
  draw_line(canvas, x0, y0, x1, y0, c.r, c.g, c.b);
  draw_line(canvas, x0, y0, x0, y1, c.r, c.g, c.b);
  draw_line(canvas, x0, y1, x1, y1, c.r, c.g, c.b);
  draw_line(canvas, x1, y0, x1, y1, c.r, c.g, c.b);
}

void draw_pause_screen(struct LedCanvas *canvas, struct LedFont *font) {
  led_canvas_clear(canvas);
  colour_t blue = {0,0,255};
  colour_t red = {255,0,0};
  draw_text(canvas, font, 5, 7, 0,0,255, "PAUSED", 0);
  draw_rectangle(canvas, blue, 2, 9, 30, 19);
  draw_text(canvas, font, 5, 17, 0, 0, 255, "RESUME", 0);
  draw_rectangle(canvas, blue, 2, 21, 30, 30);
  draw_text(canvas, font, 9, 28, 0, 0, 255, "QUIT", 0);
}

void draw_menu_screen(struct LedCanvas *canvas, struct LedFont *font) {
  led_canvas_clear(canvas);
  colour_t blue = {0,0,255};
  colour_t red = {255,0,0};
  colour_t test = {68, 180, 244};
  draw_text(canvas, font, 3,8, 68,180,244, "CLASSIC", 0);
  //draw_rectangle(canvas, blue, 1, 1, 31, 9);
  draw_text(canvas, font, 6, 17, 68, 180, 244, "CRAZY", 0);
  //draw_rectangle(canvas, blue, 2, 10, 29, 18);
  draw_text(canvas, font, 3, 26, 68, 180, 244, "AI", 0);
  //draw_rectangle(canvas, blue, 1, 19, 11, 27);
  draw_text(canvas, font, 14, 26, 244, 173, 31, "MAZE", 0);
  draw_rectangle(canvas,test, 12, 19, 30, 27);
}


void draw_score_screen(struct LedCanvas *canvas, struct LedFont *font) {
  led_canvas_clear(canvas);
  const char *bdf_font_file = "./5x7.bdf";
  struct LedFont *font2 = load_font(bdf_font_file);
  char *score = "228";
  colour_t blue = {0,0,255};
  colour_t red = {255,0,0};
  colour_t test = {68, 180, 244};
  draw_text(canvas, font2, 4, 13, 68, 180, 244, "SCORE", 0);
  draw_text(canvas, font2, 9, 24, 68, 180, 244, score, 0);  
}

void draw_retry_screen(struct LedCanvas *canvas, struct LedFont *font) {
  led_canvas_clear(canvas);
  const char *bdf_font_file = "./5x8.bdf";
  struct LedFont *font2 = load_font(bdf_font_file);
  char *score = "228";
  colour_t blue = {0,0,255};
  colour_t red = {255,0,0};
  colour_t test = {68, 180, 244};
  draw_text(canvas, font, 5, 12, 68, 180, 244, "RETRY?", 0);
  draw_text(canvas, font2, 8, 25, 68, 180, 244, "Y", 0);
  draw_rectangle(canvas, test, 6, 17, 14, 26);
  draw_text(canvas, font2, 20, 25, 68, 180, 244, "N", 0);
  draw_rectangle(canvas, test, 18, 26, 25, 17);
}

static long get_milliseconds() {
  struct timeval t;
  gettimeofday(&t, NULL); 
  return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

//Takes in a number corresponding to the sound to play
void play_sound(int i){
  int pid;
  pid=fork();
  if(pid==0)
  {
    printf("I am the child\n");
    switch(i){
      case 1:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/classic.m4a", NULL);
        break;
      case 2:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/crazy.m4a", NULL);
        break;
      case 3:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/menu.m4a", NULL);
        break;
      case 4:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/eat.wav", NULL);
        break;
      case 5:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/die.wav", NULL);
        break;
      case 6:
        execpl("/usr/bin/omxplayer", " ", "arm11_24/snake/sounds/pause.wav", NULL);
        break;
    }
    _exit(0);
  }
  else
  {
    printf("I am the parent\n");
    wait();
  } 
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
  const char *bdf_font_file = "./4x6.bdf";
  struct LedFont *font = load_font(bdf_font_file);
  
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
  long curr_time = get_milliseconds() + (INTERVAL / 10);
  int k = 0;
  bool paused = false;
  while (true) {
    int poll_res = poll(&p, 1, (int) MAX(curr_time - get_milliseconds(), 0));
    if (poll_res == 0) {
      if (paused) {
        continue;
      }
      k++;
      printf("Timed out %ld\n", get_milliseconds());
      if (k % 10 == 9) {
//        d = get_direction(snake, food, d);
        if (!perform_move(snake, d, &food, walls)) {
          TIME_AFTER_DEATH = 0;
          break;
        }
      } 
      draw_snake(offscreen_canvas, snake, &get_default, food, k, walls);
      offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
      curr_time = get_milliseconds() + (INTERVAL / 10) * multiplier;
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
    if (i == I_A && multiplier > 1) {
      multiplier--;
    } else if (i == I_B && multiplier < 10) {
      multiplier++;
    }

    if (i == I_START) {
      paused = !paused;
      if (paused) {
        draw_retry_screen(offscreen_canvas, font);
        offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
      }
/*      while (paused) {
        int selection = 0;
        if (selection == 1) { //Resume game
          paused = false;
        } else if (selection == 2) { //Quit game
          return EXIT_SUCCESS;
        }
      }*/
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

