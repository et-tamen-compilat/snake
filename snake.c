#include <stdio.h>
#include <stdbool.h>
#include "types.h"

//Checks to see if two points are equal
bool point_equal(point_t p1, point_t p2){
  return(p1.x == p2.x && p1.y == p2.y);
}

//Checks to see if two nodes are equal
bool node_equal(node_t n1, node_t n2){
  return(point_equal(n1.point, n2.point) && n1.next == n2.next);
}

// Checks to see if point occurs in snake
bool intersects(snake_t s, point_t p){
  node_t curr = s.head;
  while(!node_equal(curr, s.tail)){
    if(point_equal(curr.point, p)){
      return true;
    } 
    curr = *curr.next;
  }
  return false;
}

// applies movement of one space to point in direction 
point_t direct_point(point_t p, direction d){
  switch(d){
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

// Returns true if will be out of bounds
bool out_bounds(point_t p, direction d){
  return(
      (p.y == 0 && d == UP) ||
      (p.y == MAXHEIGHT - 1 && d == DOWN) ||
      (p.x == 0 && d == LEFT) ||
      (p.x == MAXWIDTH - 1 && d == RIGHT)
      );
}
