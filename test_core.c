#include "core.c"

#define ASSERT(expr) \
  if (!(expr)) \
  report(__LINE__)

void report(int line) {
  printf("Assertion failed at line %i\n", line);
}

void point_equal_test() {
  point_t p1 = {1, 1};
  point_t p2 = {1, 2};
  point_t p3 = {1, 1};
  point_t p4 = {6, 2};

  ASSERT(point_equal(p1, p1));
  ASSERT(!(point_equal(p1, p2)));
  ASSERT(point_equal(p1, p3));
  ASSERT(!(point_equal(p1, p4)));
  ASSERT(!(point_equal(p2, p4)));
  ASSERT(point_equal(p4, p4));

  return;
}

void out_bounds_test() {
  point_t p1 = {0, 0};
  point_t p2 = {10, 10};
  point_t p3 = {31, 31};
  point_t p4 = {0, 31};

  ASSERT(out_bounds(p1, UP));
  ASSERT(out_bounds(p1, LEFT));
  ASSERT(!(out_bounds(p2, UP)));
  ASSERT(out_bounds(p3, DOWN));
  ASSERT(out_bounds(p3, RIGHT));
  ASSERT(out_bounds(p4, DOWN));
  ASSERT(!(out_bounds(p4, RIGHT)));

  return;
}

void intersects_test() {
  snake_t *s = create_snake(); 
  ASSERT(intersects(*s, (point_t) {0,0}));
  ASSERT(intersects(*s, (point_t) {1,0}));
  ASSERT(intersects(*s, (point_t) {2,0}));
  ASSERT(intersects(*s, (point_t) {3,0}));
  ASSERT(intersects(*s, (point_t) {4,0}));
  ASSERT(!(intersects(*s, (point_t) {5,0})));
  ASSERT(!(intersects(*s, (point_t) {0,1})));
  ASSERT(!(intersects(*s, (point_t) {2,1})));

  return;
}

void direct_point_test() {
  point_t p1 = {5, 5};

  ASSERT(point_equal(direct_point(p1, UP), (point_t) {5, 4}));
  ASSERT(point_equal(direct_point(p1, DOWN), (point_t) {5, 6}));
  ASSERT(point_equal(direct_point(p1, RIGHT), (point_t) {6, 5}));
  ASSERT(point_equal(direct_point(p1, LEFT), (point_t) {4, 5}));
  ASSERT(!(point_equal(direct_point(p1, LEFT), (point_t) {4, 4})));
  ASSERT(!(point_equal(direct_point(p1, RIGHT), (point_t) {5, 6})));
  ASSERT(!(point_equal(direct_point(p1, DOWN), (point_t) {4, 6})));

  return;
}

void illegal_direction_test() {
  ASSERT(illegal_direction(UP, DOWN));
  ASSERT(illegal_direction(DOWN, UP));
  ASSERT(illegal_direction(LEFT, RIGHT));
  ASSERT(illegal_direction(RIGHT, LEFT));
  ASSERT(!(illegal_direction(UP, RIGHT)));
  ASSERT(!(illegal_direction(DOWN, LEFT)));
  ASSERT(illegal_direction(RIGHT, RIGHT));
  ASSERT(!(illegal_direction(LEFT, UP)));

  return;
}

void node_equal_test() {
  node_t n1 = (node_t) {((point_t) {3, 4}), NULL};
  node_t n2 = (node_t) {((point_t) {4, 5}), &n1};
  node_t n3 = (node_t) {((point_t) {4, 5}), &n1};

  ASSERT(node_equal(n1, n1));
  ASSERT(node_equal(n2, n2));
  ASSERT(node_equal(n2, n3));
  ASSERT(!(node_equal(n1, n2)));

  return;
}

void food_wall_test() {
  int **map = malloc(sizeof(int *) * MAX_WIDTH);
  for (int i = 0; i < MAX_WIDTH; i++) {
    map[i] = calloc(MAX_HEIGHT, sizeof(int));
  }

  map[0][0] = 1;
  map[1][0] = 1;
  map[0][2] = 1;
  map[1][1] = 1;
  map[2][1] = 1;
  point_t p1 = (point_t) {0, 0};
  point_t p2 = (point_t) {2, 2};
  point_t p3 = (point_t) {0, 2};

  ASSERT(food_wall(p1, map)); 
  ASSERT(!(food_wall(p2, map)));
  ASSERT(food_wall(p3,  map));

  return;
}

void get_rand_int_test() {
  int min = 2;
  int max = 10;
  int rand = get_rand_int(min, max);
  ASSERT(rand >= min && rand <= max);
  rand = get_rand_int(min, max);
  ASSERT(rand >= min && rand <= max);
  rand = get_rand_int(min, max);
  ASSERT(rand >= min && rand <= max);
  rand = get_rand_int(min, max);
  ASSERT(rand >= min && rand <= max);

  return;
}

int main(int argc, char **argv) {

  point_equal_test();
  out_bounds_test();
  intersects_test();
  direct_point_test();
  illegal_direction_test();
  node_equal_test();
  food_wall_test();
  get_rand_int_test();

  printf("ALL TESTS COMPLETE\n");
  return EXIT_SUCCESS;
}
