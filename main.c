#include <stdio.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "types.h"

input input_init(struct input_event e) {
  input i;
  if (e.type == 3 && e.code == 1 && e.value == 0) {
    i = I_UP;
  } else if (e.type == 3 && e.code == 1 && e.value == 255) {
    i = I_DOWN;
  } else if (e.type == 3 && e.code == 0 && e.value == 0) {
    i = I_LEFT;
  } else if (e.type == 3 && e.code == 0 && e.value == 255) {
    i = I_RIGHT;
  } else if (e.value == 589826) {
    i = I_A;
  } else if (e.value == 589825) {
    i = I_B;
  } else if (e.value == 589833) {
    i = I_SELECT;
  } else {
    printf("Invalid input event value: %i\n", e.value);
  }
  return i;
}

static long getMilliseconds() {
  struct timeval t;
  gettimeofday(&t, NULL); 
  return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

#define MAX(a, b) (a < b ? b : a)


int main(int argc, char *argv[]) {
  int fd = open("/dev/input/by-id/usb-0810_usb_gamepad-event-joystick", O_RDONLY);
  struct pollfd p = (struct pollfd) { fd, POLLIN };
  struct input_event *garbage = malloc(sizeof(struct input_event)*5);
  long time = getMilliseconds() + INTERVAL;
  while (1) {
    int poll_res = poll(&p, 1, (int) MAX(time - getMilliseconds(), 0));
    if (poll_res == 0) {
      printf("Timed out %ld\n", getMilliseconds());
      time += INTERVAL;
      continue;
    }  
    struct input_event e;
    input i;
    read(fd, &e, sizeof(struct input_event));
    // Saves input direction
    i = input_init(e);
    printf("%i\n", i);
    // Skips next 3 or 5 input events depending on command
    // Direction command
    if (e.type == 3) {
      read(fd, garbage, sizeof(struct input_event));
      read(fd, garbage, sizeof(struct input_event));
      read(fd, garbage, sizeof(struct input_event));
    }
    // A, B or Select command
    else {
      read(fd, garbage, sizeof(struct input_event)*5);
    }
    printf("%i %i %i\n", e.type, e.code, e.value);
  }
  free(garbage);
  return EXIT_SUCCESS;
}

// enum 
// function converts to enum
// read next 5 --> pointer to then free

