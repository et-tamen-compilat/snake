#include <stdint.h>
#include <stdlib.h>
#define MAX_HEIGHT 32
#define MAX_WIDTH 32
#define INTERVAL 100
#define WALL_MAX_LEN 7
#define WALL_MIN_LEN 3
#define WALL_COLOUR (colour_t){216, 150, 18}
#define SNAKE_SAFETY (point_t){13, 5}

extern int NUM_WALLS;

typedef struct {
 uint8_t x;
 uint8_t y;
} point_t;

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} colour_t;

typedef struct {
  point_t point;
  colour_t colour;
} food_t;

typedef struct node {
  point_t point;
  struct node *next;
} node_t;

typedef struct direction_node {
  direction point;
  struct direction_node *next;
} direction_node_t;

typedef struct {
  node_t *head;
  node_t *tail;
  uint32_t length;
} snake_t;

typedef struct {
  node_t *head;
  node_t *tail;
  uint8_t length;
} direction_t;

typedef node_t point_node_t;

typedef snake_t queue_t;

typedef struct {
  point_t start;
  uint8_t length;
  direction direction;
  colour_t colour;
} wall_t;

typedef enum {
  I_UP,
  I_DOWN,
  I_LEFT,
  I_RIGHT,
  I_A,
  I_B,
  I_SELECT,
  I_START,
  I_TIMEOUT,
  I_INIT
} input; 

typedef struct {
  snake_t *snake;
  direction d;
  direction nova_d;
  struct LedCanvas *offscreen_canvas;
  int multiplier;
  int **walls;
  point_t food;
  int selection;
  int selection2;
  int power_up_time; 
} state_t;

typedef enum {
  DIES,
  STATIC,
  EATS
} perform_move_result;

typedef struct {
  input type;
  int k;
} event_t;

typedef int event_handler_t(event_t event, state_t *state);
// -1 means exit
// -2 means stay the same

#define EVENT_EXIT -1
#define EVENT_REMAIN -2

typedef struct {
  event_handler_t **handlers;
  void *state;
} event_system_t;

typedef colour_t colour_function_t(int);
