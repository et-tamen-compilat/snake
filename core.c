#include "types.h"
#include <stdbool.h>
#include <stdio.h>

bool point_equal(point_t p1, point_t p2) {
  return (p1.x == p2.x && p1.y == p2.y);
}

bool out_bounds(point_t p, direction d) {
  return (
      (p.y == 0 && d == UP) ||
      (p.y == MAX_HEIGHT - 1 && d == DOWN) ||
      (p.x == 0 && d == LEFT) ||
      (p.x == MAX_WIDTH - 1 && d == RIGHT)
      );
}

// Checks to see if point occurs in snake
bool intersects(snake_t s, point_t p) {
  node_t *curr = s.head;
  while (curr != NULL) {
    if (point_equal(curr->point, p)) {
      //      printf("(%i, %i) (%i, %i)\n", p.x, p.y, curr->point.x, curr->point.y);
      return true;
    } 
    curr = curr->next;
  }
  return false;
}

point_t direct_point(point_t p, direction d) {
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

snake_t *create_snake() {
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

// Returns true if will be out of bounds
bool illegal_direction(direction curr_d, direction new_d) {
  return (
      (curr_d == RIGHT && (new_d == LEFT || new_d == RIGHT)) ||
      (curr_d == LEFT && (new_d == RIGHT || new_d == LEFT)) ||
      (curr_d == UP && (new_d == DOWN || new_d == UP)) ||
      (curr_d == DOWN && (new_d == UP || new_d == DOWN))
      );
}

//Checks to see if two nodes are equal
bool node_equal(node_t n1, node_t n2) {
  return (point_equal(n1.point, n2.point) && n1.next == n2.next);
}


bool food_wall(point_t point, int **collision_map) {
  return (collision_map[point.x][point.y]);
}

void add_wall_to_map(int **map, wall_t *wall) {
  printf("x val: %i, y val: %i\n", wall->start.x, wall->start.y);
  for (int i = 0; i <= wall->length; i++) {
    printf("x val: %i, y val: %i\n", wall->start.x, wall->start.y);
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
  { 235, 66, 244},
  { 0, 10, 216 },
  { 2, 241, 249 },
  { 74, 206, 18 },
  { 255, 250, 7 },
  { 244, 168, 17 }
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

wall_t *create_wall() {
  wall_t *wall = malloc(sizeof(wall_t));
  wall->length = get_rand_int(WALL_MIN_LEN, WALL_MAX_LEN);
  wall->direction = get_rand_dir();

  int x = MAX_WIDTH - 1, y = MAX_HEIGHT - 1;
  switch (wall->direction) {
    case RIGHT: x -= wall->length;
    case DOWN: y -= wall->length;
  }

  wall->start = (point_t) {get_rand_int(0, x), get_rand_int(0, y)};
  wall->colour = WALL_COLOUR;
  return wall;
}

void play_sound(int i);

bool perform_move(snake_t *snake, direction d, point_t* food, wall_t *walls) {
  if (out_bounds(snake->tail->point, d)) {
    return false; 
  }
  //  printf("%p\n", snake->head);
  //  snake->head = snake->head->next;
  point_t p = direct_point(snake->tail->point, d);
  for (int i = 0; i < NUM_WALLS; i++) {
    point_t point = walls[i].start;
    for (int j = 0; j < walls[i].length; j++) {
      if (point_equal(point, p)) {
        return false;
      }
      switch (walls[i].direction) {
        case DOWN: point.y++; break;
        case RIGHT: point.x++; break;
      }
    }
  }
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
    play_sound(4);
    *food = get_food(snake, walls);
    snake->length++;
  }
  return true;
}


