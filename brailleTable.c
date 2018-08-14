#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define brailleTable_charW 2
#define brailleTable_charH 3

typedef struct {
  int** table;
  size_t width;
  size_t realWidth;
  size_t height;
  size_t totalSize;
} brailleTable_t;

brailleTable_t* brailleTable_new(size_t width, size_t height) {
  brailleTable_t* bt = malloc(sizeof(brailleTable_t));
  bt->width = (width/brailleTable_charW) + 1;
  bt->height = (height/brailleTable_charH) + 1;
  bt->totalSize = (sizeof(char) + sizeof(int)*bt->width)*bt->height;
  bt->table = malloc(bt->height*sizeof(int*));
  *bt->table = malloc(bt->totalSize);
  size_t y = 0, x = 0;
  while (y < bt->height) {
    bt->table[y] = (void*)(*bt->table) + (y*(bt->width*sizeof(int)+sizeof(char)));
    x=0;
    while (x < bt->width) {
      ((char*)&bt->table[y][x])[0]=0b11100010;//do not change
      ((char*)&bt->table[y][x])[1]=0b10100000;//do not change
      ((char*)&bt->table[y][x])[2]=0b10000000;//braille code
      ((char*)&bt->table[y][x])[3]=0;//do not change
      if (x+1 >= bt->width) {
	((char*)&bt->table[y][x+1])[0]='\n';
      }
      ++x;
    }
    ++y;
  }
  return (bt);
}

static 

void brailleTable_setPixel(brailleTable_t* bt, size_t x, size_t y, bool value) {
  size_t x1 = x/brailleTable_charW, x2 = x%brailleTable_charW, y1 = y/brailleTable_charH, y2 = y%brailleTable_charH;
  char *bc = &(((char*)&bt->table[y1][x1])[2]);
  *bc ^= (-value ^ *bc) & (1UL << ((x2*3)+y2));
}

int test_main() {
  int w=25, x, h=25, y;
  brailleTable_t* bt = brailleTable_new(w, h);
  
  write(0, "\033[2J\033[0;0f\033[s", strlen("\033[2J\033[0;0f\033[s"));
  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      brailleTable_setPixel(bt, x, y, true);
      write(0, "\033[u", strlen("\033[u"));
      write(0, *bt->table, bt->totalSize);
      brailleTable_setPixel(bt, x, y, false);
      usleep(100000);
    }
  }
}

typedef struct s_map {
  int32_t sizeX, sizeY;
  char** board;
  int** neighbours;
  unsigned long death_and_desolation;
} map_t;

int usage() {
  printf("./gol sizeX sizeY sizePopulation fps\n\n");
  return -1;
}

void display(map_t* map, brailleTable_t *bt) {
  int x, mx=map->sizeX, y, my=map->sizeY;

  for (y=0; y<my; ++y) {
    for(x=0; x<mx; ++x){
      if (map->board[y][x]==1)
	brailleTable_setPixel(bt, x, y, true);
      else
	brailleTable_setPixel(bt, x, y, false);
    }
  }
  write(0, "\033[u", strlen("\033[u"));
  write(0, *bt->table, bt->totalSize);
}

#define CORRECT_MAX(var, max) (var < 0 ? max-1 : var >= max ? 0 : var)

int32_t countNeighbours(map_t* map, int32_t x, int32_t y) {
  int32_t count = 0;

  count+=map->board[CORRECT_MAX(y-1, map->sizeY)][CORRECT_MAX(x-1, map->sizeX)];
  count+=map->board[CORRECT_MAX(y, map->sizeY)][CORRECT_MAX(x-1, map->sizeX)];
  count+=map->board[CORRECT_MAX(y+1, map->sizeY)][CORRECT_MAX(x-1, map->sizeX)];
  count+=map->board[CORRECT_MAX(y-1, map->sizeY)][CORRECT_MAX(x, map->sizeX)];
  count+=map->board[CORRECT_MAX(y+1, map->sizeY)][CORRECT_MAX(x, map->sizeX)];
  count+=map->board[CORRECT_MAX(y-1, map->sizeY)][CORRECT_MAX(x+1, map->sizeX)];
  count+=map->board[CORRECT_MAX(y, map->sizeY)][CORRECT_MAX(x+1, map->sizeX)];
  count+=map->board[CORRECT_MAX(y+1, map->sizeY)][CORRECT_MAX(x+1, map->sizeX)];
  return count;
}

void process(map_t* map) {
  int32_t x, y;

  map->death_and_desolation = 0;
  y = -1;
  while (++y < map->sizeY) {
    x = -1;
    while (++x < map->sizeX) {
      map->neighbours[y][x] = countNeighbours(map, x, y);
      map->death_and_desolation += map->neighbours[y][x];
    }
  }
  y = -1;
  while (++y < map->sizeY) {
    x = -1;
    while (++x < map->sizeX) {
      if (map->board[y][x] == 1) {
	if (map->neighbours[y][x] <= 1)
	  map->board[y][x] = 0;
	else if (map->neighbours[y][x] >= 4)
	  map->board[y][x] = 0;
      }
      else {
	if (map->neighbours[y][x] == 3)
	  map->board[y][x] = 1;
      }
    }
  }
}

int main(int ac, char**av) {
  int x, y;

  if (ac != 5)
    return usage();

  map_t map;
  map.sizeX = atoi(av[1]);
  map.sizeY = atoi(av[2]);
  map.board = calloc(sizeof(char*), map.sizeY);
  map.neighbours = calloc(sizeof(int*), map.sizeY);
  y = -1;
  while (++y < map.sizeY) {
    map.board[y] = calloc(sizeof(char), map.sizeX);
    map.neighbours[y] = calloc(sizeof(int), map.sizeX);
  }
  brailleTable_t* bt = brailleTable_new(map.sizeX, map.sizeY);

  int32_t sizePopulation = atoi(av[3]);
  srand(time(0));
  while (--sizePopulation > 0) {
    x = rand() % map.sizeX;
    y = rand() % map.sizeY;
    map.board[y][x] = 1;
  }

  int32_t delay = 1000 / atoi(av[4]);
  clock_t timer, clocked = 0;
  map.death_and_desolation = 1;
  write(0, "\033[2J\033[0;0f\033[s", strlen("\033[2J\033[0;0f\033[s"));
  while (map.death_and_desolation > 0) {
    timer = clocked;
    display(&map, bt);
    process(&map);
    clocked = clock() / CLOCKS_PER_SEC;
    timer = delay - (clocked - timer);
    if (timer > 0)
      usleep(timer * 1000);
  }
  return 0;
} 
