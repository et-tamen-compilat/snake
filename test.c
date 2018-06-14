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
int get_rand_int(int min, int max); 

typedef struct {
  point_t parent;
  int dist;
  direction d;
  bool visited;
} info_t;

#define LINKED_LIST_PUSH(name, type, sub_type, dtype) \
  void name##_enqueue(type *queue, dtype nova) { \
    sub_type *node = calloc(1, sizeof(sub_type)); \
    node->point = nova; \
    if (queue->tail != NULL) { \
      queue->tail->next = node; \
    } else { \
      queue->head = node; \
    } \
    queue->tail = node; \
    queue->length++; \
  }

LINKED_LIST_PUSH(queue, queue_t, point_node_t, point_t);

void queue_prepend(queue_t *queue, point_t nova) {
  point_node_t *node = calloc(1, sizeof(point_node_t));
  node->point = nova;
  node->next = queue->head;
  queue->head = node;
  if (queue->tail == NULL) {
    queue->tail = node;
  } 
  queue->length++;
}

bool queue_empty(queue_t *queue) {
  return queue->length == 0;
}

point_t queue_dequeue(queue_t *queue) {
  assert(!queue_empty(queue));
  point_node_t *node = queue->head;
  queue->head = queue->head->next; 
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
  point_t point = node->point;
  queue->length--;
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
  direction d;
} result_t;

result_t build_shortest_path(info_t **info, point_t dest, point_t start) {
  result_t result = { queue_init(), info[dest.x][dest.y].dist, info[dest.x][dest.y].d };
  point_t last = info[dest.x][dest.y].parent;
//  printf("Here\n");
//  point_print(last);
  while (!point_equal(last, start)) {
//    point_print(last);
    queue_prepend(result.queue, last);
    last = info[last.x][last.y].parent;
    result.dist++;
  }
  queue_prepend(result.queue, start);
  queue_enqueue(result.queue, dest);
  return result;
}

result_t get_shortest_path(snake_t *snake, point_t start, point_t dest, direction d) {
  queue_t *queue = queue_init();
  info_t **info = calloc(MAX_WIDTH, sizeof(info_t *)); 
  for (int i = 0; i < MAX_WIDTH; i++) {
    info[i] = calloc(MAX_HEIGHT, sizeof(info_t));
  }
  queue_enqueue(queue, start);
  info[start.x][start.y].d = d;
  info[start.x][start.y].dist = 0;
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
      if (!out_bounds(p, ds[i]) && ((!point_equal(p, start) && point_equal(l, dest)) || !intersects(*snake, l))) {
        if (!info[l.x][l.y].visited) {
          info[l.x][l.y].dist = info[p.x][p.y].dist + 1;
          info[l.x][l.y].d = ds[i];
          info[l.x][l.y].parent = p;
          info[l.x][l.y].visited = true;
          queue_enqueue(queue, l);
        }
      }
    }
  }
  for (int i = 0; i < MAX_WIDTH; i++) {
    free(info[i]);
  }
  free(info);
  return (result_t) { NULL, -1, LEFT };
}

result_t get_longest_path(snake_t *snake, point_t start, point_t point, direction d) {
  result_t shortest = get_shortest_path(snake, start, point, d);
  point_node_t *curr = shortest.queue->head;
  while (curr->next != NULL) {
    printf("K:\n");
    //queue_print(shortest.queue);
    result_t mini = get_shortest_path(shortest.queue, curr->point, curr->next->point, shortest.d);
    printf("Y: %i\n", mini.dist);
    if (mini.dist != -1) {
      //queue_print(mini.queue);
    }
    if (mini.dist == -1) {
      curr = curr->next;
    } else {
      mini.queue->tail->next = curr->next->next;
      curr->next = mini.queue->head->next;
    }
  }
  return shortest;
}

void queue_free(queue_t *queue) {
  point_node_t *curr = queue->head;
  while (curr != NULL) {
    point_node_t *temp = curr;
    curr = curr->next;
    free(temp);
  }
  free(queue);
}

direction get_direction(snake_t *snake, point_t dest, direction d) {
  direction ds[] = { UP, DOWN, LEFT, RIGHT };
  point_t first = snake->tail->point;
  result_t result = get_shortest_path(snake, snake->tail->point, dest, d);
  if (result.dist == -1) {
    queue_free(result.queue);
    return get_rand_int(0, 3);
  }
  point_t second = result.queue->head->next->point;
//  point_print(second);
  for (int i = 0; i < 4; i++) {
    if (!out_bounds(first, ds[i])) {
 //     point_print(direct_point(first, ds[i]));
      if (point_equal(direct_point(first, ds[i]), second)) {
        return ds[i];
      }
    }
  }
  queue_free(result.queue);
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

/*int main(int argc, char* argv[]) {
  snake_t *snake = create_snake();
  point_t p = {10, 10};
  //result_t res = get_shortest_path(snake, snake->tail->point, p, RIGHT);
  direction d = get_direction(snake, p, RIGHT);
  printf("%i\n", d);
  printf("Ended\n");
//  queue_print(res.queue);
  return EXIT_SUCCESS;
}*/
