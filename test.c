#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "types.h"

bool out_bounds(point_t p, direction d);
bool intersects(snake_t s, point_t p);
bool point_equal(point_t p1, point_t p2);
point_t direct_point(point_t p, direction d);
snake_t *create_snake();

typedef struct {
  point_t parent;
  int dist;
  direction d;
  bool visited;
} info_t;

typedef struct point_node {
  point_t point;
  struct point_node *next;
} point_node_t;

typedef struct {
  point_node_t *head;
  point_node_t *tail;
} queue_t;

void queue_enqueue(queue_t *queue, point_t nova) {
  point_node_t *node = calloc(1, sizeof(point_node_t));
  node->point = nova;
  if (queue->tail != NULL) {
    queue->tail->next = node;
  } else {
    queue->head = node;
  }
  queue->tail = node;
}

void queue_prepend(queue_t *queue, point_t nova) {
  point_node_t *node = calloc(1, sizeof(point_node_t));
  node->point = nova;
  node->next = queue->head;
  queue->head = node;
  if (queue->tail == NULL) {
    queue->tail = node;
  } 
}

bool queue_empty(queue_t *queue) {
  return queue->head == NULL;
}

point_t queue_dequeue(queue_t *queue) {
  assert(!queue_empty(queue));
  point_node_t *node = queue->head;
  queue->head = queue->head->next; 
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
  point_t point = node->point;
  free(node);
  return point;
}

queue_t *queue_init() {
  return calloc(1, sizeof(queue_t));
}

void point_print(point_t point) {
  printf("(%i, %i)\n", point.x, point.y);
}

void queue_print(queue_t *queue) {
  printf("Printing Queue:\n");
  point_node_t *curr = queue->head;
  while (curr != NULL) {
    point_print(curr->point);
    curr = curr->next;
  }
}

typedef struct {
  queue_t *queue;
  int dist;
} result_t;

result_t build_shortest_path(info_t **info, point_t dest, point_t start) {
  result_t result = { queue_init(), 0 };
  point_t last = info[dest.x][dest.y].parent;
//  printf("Here\n");
//  point_print(last);
  while (!point_equal(last, start)) {
//    point_print(last);
    queue_prepend(result.queue, last);
    last = info[last.x][last.y].parent;
    result.dist++;
  }
  return result;
}

result_t get_shortest_path(snake_t *snake, point_t dest, direction d) {
  queue_t *queue = queue_init();
  info_t **info = calloc(MAX_WIDTH, sizeof(info_t *)); 
  for (int i = 0; i < MAX_WIDTH; i++) {
    info[i] = calloc(MAX_HEIGHT, sizeof(info_t));
  }
  point_t start = snake->tail->point;
  queue_enqueue(queue, start);
  info[start.x][start.y].d = d;
  info[start.x][start.y].visited = true;
  while (!queue_empty(queue)) {
//    printf("Hello\n");
   point_t p = queue_dequeue(queue);
//    point_print(p);
    if (point_equal(p, dest)) { 
      return build_shortest_path(info, dest, start);
    }
    direction q = info[p.x][p.y].d;
    direction ds[] = { q, DOWN, LEFT, RIGHT };
    ds[q] = UP;
    for (int i = 0; i < 4; i++) {
      point_t l = direct_point(p, ds[i]); 
      if (!out_bounds(p, ds[i]) && !intersects(*snake, l)) {
        if (!info[l.x][l.y].visited) {
          info[l.x][l.y].dist = info[p.x][p.y].dist + 1;
          info[l.x][l.y].parent = p;
          info[l.x][l.y].visited = true;
          queue_enqueue(queue, l);
        }
      }
    }
  }
}

void queue_test() {
  queue_t *queue = queue_init();
  queue_print(queue);
  point_t p[] = { {1, 1}, {2, 2}, {3, 3} };
  queue_enqueue(queue, p[0]);
  queue_enqueue(queue, p[1]);
  queue_enqueue(queue, p[2]);
  queue_print(queue);
  printf("Dequeueing\n");
  point_print(queue_dequeue(queue));
  queue_print(queue);
}

int main(int argc, char* argv[]) {
  snake_t *snake = create_snake();
  point_t p = {10, 10};
  result_t res = get_shortest_path(snake, p, RIGHT);
  printf("Ended\n");
  queue_print(res.queue);
  return EXIT_SUCCESS;
}
