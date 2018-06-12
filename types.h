#include <stdlib.h>
#define MAXHEIGHT 32
#define MAXWIDTH 32

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

typedef enum {
  I_UP,
  I_DOWN,
  I_LEFT,
  I_RIGHT,
  I_A,
  I_B,
  I_SELECT
} input; 
