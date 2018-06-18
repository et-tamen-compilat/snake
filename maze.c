#include <assert.h>
#include <stdbool.h>
#include "types.h"

int NUM_WALLS = 0;

int **create_collision_map(wall_t *wall_arr);
int get_rand_int(int, int);

void create_maze(int **collision_map, int x1, int y1, int x2, int y2) {
//  printf("%i %i %i %i\n", x1, x2, y1, y2);
  assert(x1 <= x2);
  assert(y1 <= y2);
  int width = (x2 - x1 + 1); 
  int height = (y2 - y1 + 1); 
  bool doX = width >= 3;
  bool doY = height >= 3;
  int chooseX = -1, chooseY = -1;
  if (doX) {
    if (y1 == 0 && x1 + 1 < 6) {
      if (x2 - 1 < 6) {
        doX = false;
      } else {
        chooseX = get_rand_int(6, x2 - 1);
//        printf("Here %i\n", chooseX);
      }
    } else {
//      printf("Here2\n");
      chooseX = get_rand_int(x1 + 1, x2 - 1);
    }
  }
  if (doY) {
//    printf("Here3\n");
    chooseY = get_rand_int(y1 + 1, y2 - 1);
  }
  if (!doX && !doY) {
    return;  
  } if (doX && !doY) {
//    printf("Here4\n");
    int hole = get_rand_int(y1, y2);
    for (int i = y1; i <= y2; i++) {
      if (i != hole) {
        collision_map[chooseX][i] = 1;
      }
    } 
    create_maze(collision_map, x1, y1, chooseX - 1, y2);
    create_maze(collision_map, chooseX + 1, y1, x2, y2);
  } else if (!doX && doY) {
//    printf("Here5\n"); 
    int hole = get_rand_int(x1, x2);
    for (int i = x1; i <= x2; i++) {
      if (i != hole) {
        collision_map[i][chooseY] = 1;
      }
    } 
    create_maze(collision_map, x1, y1, x2, chooseY - 1);
    create_maze(collision_map, x1, chooseY + 1, x2, y2);
  } else {
//    printf("Here6 %i %i\n", x1, chooseX - 1); 
    int hole1 = get_rand_int(x1, chooseX - 1);
//    printf("Here7\n"); 
    int hole2 = get_rand_int(chooseX + 1, x2);
//    printf("Here8\n"); 
    int hole3 = get_rand_int(y1, y2);
    for (int i = y1; i <= y2; i++) {
      if (i != hole3) {
        collision_map[chooseX][i] = 1;
      }
    } 
    for (int i = x1; i <= x2; i++) {
      if (i != hole1 && i != hole2) {
        collision_map[i][chooseY] = 1;
      }
    } 
    create_maze(collision_map, x1, y1, chooseX - 1, chooseY - 1);
    create_maze(collision_map, chooseX + 1, y1, x2, chooseY - 1);
    create_maze(collision_map, x1, chooseY + 1, chooseX - 1, y2);
    create_maze(collision_map, chooseX + 1, chooseY + 1, x2, y2);
  }
}

int main() {
  int **map = create_collision_map(NULL);
  create_maze(map, 0, 0, 31, 31);
}
