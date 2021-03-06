#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
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
direction get_direction(snake_t *snake, point_t dest, direction d, wall_t *walls); 
bool illegal_direction(direction curr_d, direction new_d); 
bool perform_move(snake_t *snake, direction d, point_t food, wall_t *walls, bool check);
struct LedFont *font;
void queue_free(snake_t *snake);

volatile sig_atomic_t done = 0;
volatile struct RGBLedMatrix *matrix;
int NUM_WALLS;

void term(int signum) {
  printf("Hello\n");
  //  led_matrix_delete(matrix);
  done = true;
}

int TIME_AFTER_DEATH = -1;

// Draws the given wall to the canvas 
/*
void draw_wall(struct LedCanvas *canvas, wall_t *wall) {
  int x0 = wall->start.x;
  int y0 = wall->start.y;
  switch (wall->direction) {
    case DOWN:
      draw_line(canvas, x0, y0, x0, y0 + wall->length - 1, WALL_COLOUR.r,
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
    case RIGHT:
      draw_line(canvas, x0, y0, x0 + wall->length - 1, y0, WALL_COLOUR.r, 
          WALL_COLOUR.g, WALL_COLOUR.b);
      break;
  }
}*/

void draw_walls(struct LedCanvas *canvas, int **walls) {
  for (int i = 0; i < MAX_WIDTH; i++) {
    for (int j = 0; j < MAX_HEIGHT; j++) {
      set_pixel(canvas, i, j, WALL_COLOUR.r, WALL_COLOUR.g, WALL_COLOUR.b);
    }
  }
}

// Creates an array of walls
int **create_map() {
  int num_walls = NUM_WALLS;
  int **collision_map = create_collision_map();
  for (int i = 0; i < num_walls; i++) {
    bool created = false;
    while (!created) {
      wall_t *wall = create_wall();
      if (wall->start.x > SNAKE_SAFETY.x || wall->start.y > SNAKE_SAFETY.y) {
        created = true;
        add_wall_to_map(collision_map, wall);
        free(wall);
      } else {
        free(wall);
      }
    }
  }
  return collision_map;
}

//Check map for food, check it's not on top of a wall basically
//In main, put this in a while look when generating food
void draw_snake(struct LedCanvas *canvas, snake_t *s, colour_function_t *c, point_t food, int k, int **walls, int power_up_time) {
  led_canvas_clear(canvas);
  if (power_up_time > 5) {
    c = &multicolour;
  }
  draw_walls(canvas, walls);
  /*for (int i = 0; i < NUM_WALLS; i++) {
    draw_wall(canvas, &walls[i]);
  }*/
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
    colour_t k = c(len - pos);
    pos++;
    if (TIME_AFTER_DEATH >= (len - pos)) {
      led_canvas_set_pixel(canvas, p->x, p->y, 255, 0, 0);
    }
    else {
      led_canvas_set_pixel(canvas, p->x, p->y, k.r, k.g, k.b);
    }  
    current = current->next;
  }
  if (k % 10 == 0 && TIME_AFTER_DEATH == -1) {
    if (power_up_time >= 0) {
      led_canvas_set_pixel(canvas, food.x, food.y, 255, 0, 0);
    } else {
      led_canvas_set_pixel(canvas, food.x, food.y, 174, 83, 239);
    }
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
  } else if (e.value == 1 && e.type == 1 && e.number == 8) {
    i = I_SELECT;
  } else if (e.value == 1 && e.type == 1 && e.number == 9) {
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

void draw_pause_screen(struct LedCanvas *canvas, struct LedFont *font, int selection) {
  led_canvas_clear(canvas);
  colour_t text = {68, 180, 244};
  colour_t box = {244, 173, 31};
  draw_text(canvas, font, 5, 7, text.r,text.g, text.b, "PAUSED", 0);
  draw_text(canvas, font, 5, 17, text.r, text.g, text.b, "RESUME", 0);
  draw_text(canvas, font, 9, 28, text.r, text.g, text.b, "QUIT", 0);
  if (selection == 0) {
    draw_rectangle(canvas, box, 2, 9, 30, 19);
  } else {
    draw_rectangle(canvas, box, 2, 21, 30, 30);
  }
}

void draw_menu_screen(struct LedCanvas *canvas, struct LedFont *font, int selection) {
  led_canvas_clear(canvas);
  colour_t text = {68, 180, 244};
  colour_t box = {244, 173, 31};
  draw_text(canvas, font, 3, 8, text.r, text.g, text.b, "CLASSIC", 0);
  draw_text(canvas, font, 6, 17, text.r, text.g, text.b, "CRAZY", 0);
  draw_text(canvas, font, 3, 26, text.r, text.g, text.b, "AI", 0);
  draw_text(canvas, font, 14, 26, text.r, text.g, text.b, "MAZE", 0);
  switch (selection) {
    case 0: draw_rectangle(canvas, box, 1, 1, 31, 9); break;
    case 1: draw_rectangle(canvas, box, 2, 10, 29, 18); break;
    case 2: draw_rectangle(canvas, box, 1, 19, 11, 27); break;
    case 3: draw_rectangle(canvas, box, 12, 19, 30, 27); break;
  }
}


void draw_score_screen(struct LedCanvas *canvas, struct LedFont *font, int score) {
  led_canvas_clear(canvas);
  const char *bdf_font_file = "./5x7.bdf";
  struct LedFont *font2 = load_font(bdf_font_file);
  char score_str[5];
  sprintf(score_str, "%i", score);
  int start_spacing = 0;
  //Fixes spacing on different number of digits.
  switch (strlen(score_str)) {
    case 1: start_spacing = 13; break;
    case 2: start_spacing = 11; break;
    case 3: start_spacing = 9; break;
    case 4: start_spacing = 7; break;
  }
  colour_t text = {68, 180, 244};
  draw_text(canvas, font2, 4, 13, text.r, text.g, text.b, "SCORE", 0);
  draw_text(canvas, font2, start_spacing, 24, text.r, text.g, text.b, score_str, 0);  
}

void draw_retry_screen(struct LedCanvas *canvas, struct LedFont *font, int selection) {
  led_canvas_clear(canvas);
  const char *bdf_font_file = "./5x8.bdf";
  struct LedFont *font2 = load_font(bdf_font_file);
  colour_t text = {68, 180, 244};
  colour_t box = {244, 173, 31};
  draw_text(canvas, font, 5, 12, text.r, text.g, text.b, "RETRY?", 0);
  draw_text(canvas, font2, 8, 25, text.r, text.g, text.b, "Y", 0);
  draw_text(canvas, font2, 20, 25, text.r, text.g, text.b, "N", 0);
  if (selection == 0) {
    draw_rectangle(canvas, box, 6, 17, 14, 26);
  } else {
    draw_rectangle(canvas, box, 18, 26, 25, 17);
  }
}

static long get_milliseconds() {
  struct timeval t;
  gettimeofday(&t, NULL); 
  return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

//Takes in a number corresponding to the sound to play
void play_sound(int i){
  int pid;
  pid = fork();
  if(pid == 0)
  {
    switch (i) {
      case 1:
        execlp("/usr/bin/omxplayer", " ", "-o", "local",  "--loop", "/home/pi/arm11_24/snake/sounds/classic.m4a", NULL);
        break;
      case 2:
        execlp("/usr/bin/omxplayer", " ", "-o", "local",  "--loop", "/home/pi/arm11_24/snake/sounds/crazy.m4a", NULL);
        break;
      case 3:
        execlp("/usr/bin/omxplayer", " ", "-o", "local",  "--loop", "/home/pi/arm11_24/snake/sounds/menu.m4a", NULL);
        break;
      case 4:
        system("aplay /home/pi/arm11_24/snake/sounds/eat.wav");
        break;
      case 5:
        system("aplay /home/pi/arm11_24/snake/sounds/die.wav");
        break;
      case 6:
        system("aplay /home/pi/arm11_24/snake/sounds/pause.wav");
        break;
    }
    _exit(0);
  }
  return;
}

//Stops sound
void stop_sound() {
  system("killall omxplayer.bin");
  return;
}

#define MAX(a, b) (a < b ? b : a)

int handle_main(event_t event, state_t *state) {
  switch (event.type) {
    case I_INIT:
      stop_sound();
      state->multiplier = 3;
      if (state->snake == NULL) {
        state->snake = create_snake();
        if (state->selection2 == 1 || state->selection2 == 2) {
          NUM_WALLS = 20;
        } else {
          NUM_WALLS = 0;
        }
        state->walls = create_map();
        state->d = RIGHT;
        state->nova_d = RIGHT;
        state->food = get_food(state->snake, state->walls);
        state->power_up_time = 0;
        TIME_AFTER_DEATH = -1;
      }
      if (state->selection2 == 1) {
        play_sound(2);
      }
      else {
        play_sound(1);
      }
      return EVENT_REMAIN;
    case I_TIMEOUT:
     if (event.type == I_INIT || event.k % (2 * state->multiplier) == 0) {
        if (state->selection2 == 2) {
          state->d = get_direction(state->snake, state->food, state->d, state->walls);
        } else {
          state->d = state->nova_d;
        }
        perform_move_result res = perform_move(state->snake, state->d, state->food, state->walls, state->power_up_time > 0);
        if (state->power_up_time > 0) {
          state->power_up_time--;
        }
        switch (res) {
          case EATS:
            if (state->power_up_time == -1) {
              state->power_up_time = 100;
            }
            play_sound(4);
            state->food = get_food(state->snake, state->walls);
            if (state->selection2 == 1 && state->power_up_time == 0) {
              if (use_power_up()) {
                state->power_up_time = -1; 
              }
            }
            break;
          case DIES:
            stop_sound();
            play_sound(5);
            TIME_AFTER_DEATH = 0;
            return 5;
        }
      }
      draw_snake(state->offscreen_canvas, state->snake, &get_default, state->food, event.k, state->walls, state->power_up_time);
      state->offscreen_canvas 
        = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
      return EVENT_REMAIN;
    case I_LEFT:
    case I_RIGHT:
    case I_UP:
    case I_DOWN:
      if (!illegal_direction(state->d, event.type)) {
        state->nova_d = event.type;
      }
      return EVENT_REMAIN;
    case I_A:
      if (state->multiplier > 1) {
        state->multiplier--;
      } 
      return EVENT_REMAIN;
    case I_B:
      if (state->multiplier < 10) {
        state->multiplier++;
      }
      return EVENT_REMAIN;
    case I_START:
      return 2;
    default:
      return EVENT_REMAIN;
  }
}

int handle_pause(event_t event, state_t *state) {
  switch (event.type) {
    case I_INIT:
      stop_sound();
      play_sound(6);
    case I_UP:
      state->selection = 0;
      break;
    case I_DOWN:
      state->selection = 1;
      break; 
  }

  switch (event.type) {
    case I_A:
    case I_SELECT:
      if (state->selection == 0) {
        return 1;
      } else if (state->selection == 1) {
        queue_free(state->snake);
        state->snake = NULL;
        return 0;
      }
      break;
    case I_UP:
    case I_DOWN:
    case I_INIT:
      draw_pause_screen(state->offscreen_canvas, font, state->selection);
      state->offscreen_canvas 
        = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
      return EVENT_REMAIN;
    default:
      return EVENT_REMAIN;
  }
}

int handle_death_throes(event_t event, state_t *state) {
  switch (event.type) {
    case I_TIMEOUT:
      if (event.k % 10 != 0) {
        return EVENT_REMAIN;
      }
      if (TIME_AFTER_DEATH == state->snake->length) {
        //printf("Flash\n");
        return 3;
      } else {
        draw_snake(state->offscreen_canvas, state->snake, &get_default, state->food, event.k, state->walls, 0);
        state->offscreen_canvas = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
        TIME_AFTER_DEATH++;
        return EVENT_REMAIN;
      }
      /*if (TIME_AFTER_DEATH != 1) {
        TIME_AFTER_DEATH = 1; 
        draw_snake(state->offscreen_canvas, state->snake, &get_default, state->food, event.k, state->walls);
        state->offscreen_canvas = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
        fflush(stdout);
      }
      return EVENT_REMAIN;*/
      break;
    default:
      return EVENT_REMAIN;
  }
}

int handle_score(event_t event, state_t *state) {
  switch (event.type) {
    case I_INIT:
      stop_sound();
      play_sound(3);
      draw_score_screen(state->offscreen_canvas, font, state->snake->length * 10);
      state->offscreen_canvas = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
      queue_free(state->snake);
      state->snake = NULL;
      return EVENT_REMAIN;
    case I_TIMEOUT:
      //printf("Kid Flash: %i\n", event.k);
      if (event.k == 20) {
        return 4;
      }
      return EVENT_REMAIN;
    default:
      return EVENT_REMAIN;
  }
}

int handle_menu(event_t event, state_t *state) {
  //printf("H: %i\n", event.type);
  switch (event.type) {
    case I_TIMEOUT:
      if (event.k != 0) {
        break;
      }
    case I_INIT:
      stop_sound();
      play_sound(3);
      state->selection2 = 0;
      break;
    case I_DOWN:
      if (state->selection2 < 3) {
        state->selection2++;
        //printf("LEMON: %i\n", state->selection);
      }
      break;
    case I_UP:
      if (state->selection2 > 0) {
        state->selection2--;
        //printf("CITRON: %i\n", state->selection);
      }
      break;
  }
  switch (event.type) {
    case I_TIMEOUT:
      if (event.k != 0) {
        return EVENT_REMAIN;
      }
    case I_INIT:
    case I_UP:
    case I_DOWN:
      //printf("J: %i\n", state->selection);
      draw_menu_screen(state->offscreen_canvas, font, state->selection2);
      state->offscreen_canvas 
        = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
      return EVENT_REMAIN;
    case I_A:
    case I_SELECT:
      return 1;
    default:
      return EVENT_REMAIN;
  }
}

int handle_retry(event_t event, state_t *state) {
  switch (event.type) {
    case I_INIT:
    case I_LEFT:
      state->selection = 0;
      break;
    case I_RIGHT:
      state->selection = 1;
      break; 
  }
  switch (event.type) {
    case I_INIT:
    case I_LEFT:
    case I_RIGHT:
      draw_retry_screen(state->offscreen_canvas, font, state->selection);
      state->offscreen_canvas 
        = led_matrix_swap_on_vsync(matrix, state->offscreen_canvas);
      return EVENT_REMAIN;
    case I_A:
    case I_SELECT:
      if (state->selection == 0) {
        return 1;
      } else {
        return 0;
      }
    default:
      return EVENT_REMAIN;
  }
}

void run_event_system(event_system_t system, void *initial) {
  int fd = open("/dev/input/js0", O_RDONLY);
  long curr_time = get_milliseconds() + (INTERVAL / 10);
  struct pollfd p = (struct pollfd) { fd, POLLIN };
  struct input_event *garbage = malloc(sizeof(struct js_event)*5);
  event_handler_t *curr = system.handlers[0];
  void *state = initial;
  int k = 0;
  int debug = 0;
  while (!done) {
    //printf("Here: %i\n", done);
    int poll_res = poll(&p, 1, (int) MAX(curr_time - get_milliseconds(), 0));
    //   printf("Here3: %i\n", debug);
    int index;
    if (poll_res == 0) {
      event_t ev = { I_TIMEOUT, k++ };
      //    printf("Here2: %i\n", debug);
      index = curr(ev, state);
      //   printf("Here5: %i\n", debug);
      curr_time = get_milliseconds() + (INTERVAL / 10) * 5 ;//* multiplier;
    } else {
      //  printf("Here4: %i\n", debug);
      struct js_event e;
      read(fd, &e, sizeof(struct js_event));
      input i = input_init(e);
      if (i != 10) {
        event_t ev = { i, k++ };
        index = curr(ev, state);
        read(fd, garbage, sizeof(struct js_event));
      } else {
        index = -2;
      }
    }
    if (index == EVENT_EXIT) {
      done = true;
    } else if (index != -2) {
      // printf("Here6: %i\n", debug);
      event_handler_t *nova = system.handlers[index];
      if (nova != curr) {
        printf("Here7: %i\n", debug);
        k = 0;
        event_t ev = { I_INIT, k++ };
        int l = nova(ev,state);
        printf("%d/n", l);
        assert(l == EVENT_REMAIN);
        curr = nova;
      }
    }
    debug++;
  }

  free(garbage);
  return;
} 

int main(int argc, char **argv) {
  atexit(stop_sound);
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = &term;
  sigaction(SIGINT, &action, NULL);

  struct RGBLedMatrixOptions options;
  int width, height;
  int x, y, i;
  int k = /*1529192801*/ time(NULL);
  printf("Random: %i", k);
  srand(k);
  const char *bdf_font_file = "./4x6.bdf";
  font = load_font(bdf_font_file);

  //  srand(45);

  memset(&options, 0, sizeof(options));
  options.rows = 32;
  options.chain_length = 1;

  matrix = led_matrix_create_from_options(&options, &argc, &argv);
  assert(matrix != NULL);
  state_t state
    = { NULL
      , RIGHT
        , RIGHT
        , led_matrix_create_offscreen_canvas(matrix)
        , 5
        , NULL
        , {0,0}
      , 0
        , 0
    };

  event_handler_t *handlers[] = 
  { handle_menu
    , handle_main
      , handle_pause
      , handle_score
      , handle_retry
      , handle_death_throes
  };

  event_system_t system = 
  { handlers
    , &state
  };

  run_event_system(system, &state);

  led_matrix_delete(matrix);

  return EXIT_SUCCESS;
}
