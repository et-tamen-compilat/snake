#include <stdint.h>
#include <stdlib.h>
#define MAX_HEIGHT 32
#define MAX_WIDTH 32
#define INTERVAL 100
#define WALL_MAX_LEN 7
#define WALL_MIN_LEN 3
#define WALL_COLOUR (colour_t){216, 150, 18}
#define SNAKE_SAFETY (point_t){10, 4}

typedef struct {
 uint8_t x;
 uint8_t y;
} point_t;

typedef struct{
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

typedef struct {
  node_t *head;
  node_t *tail;
} snake_t;

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction;

typedef struct {
  point_t start;
  uint8_t length;
  direction direction;
  colour_t colour;
} wall_t;

typedef struct {
  wall_t *head;
  wall_t *tail;
} wall_list_t;

typedef enum {
  I_UP,
  I_DOWN,
  I_LEFT,
  I_RIGHT,
  I_A,
  I_B,
  I_SELECT
} input; 

typedef colour_t colour_function_t(int);
