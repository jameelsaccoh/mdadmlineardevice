#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
  if (cache != NULL){
    return -1;
  }
  if (num_entries < 2){
    return -1;
  }
  if (num_entries > 4096){
    return -1;
  }

  cache = calloc(num_entries, sizeof(cache_entry_t));
  if (cache == NULL){
    return -1;
  }

  memset(cache, 0, num_entries * sizeof(cache_entry_t));

  cache_size = num_entries;
  clock = 0;
  num_queries = 0;
  num_hits = 0;

  return 1;
}

int cache_destroy(void) {
  if (cache == NULL){
    return -1;
  }

  cache = NULL;
  cache_size = 0;
  clock = 0;
  num_queries = 0;
  num_hits = 0;

  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {

  clock++;
  num_queries++;

  if (buf == NULL){
    return -1;
  }
  if (cache == NULL){
    return -1;
  }
  if (cache_size == 0){
    return -1;
  }
  if (clock == 0){
    return -1;
  }


  for (int i = 0; i < cache_size; i++){
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      for (int j = 0; j < 256; j++){
        buf[j] = cache[i].block[j];
      }

      cache[i].access_time = clock;
      num_hits++;
      return 1;
    }
  }
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {

  for (int i = 0; i < cache_size; i++){
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      memcpy(cache[i].block, buf, 256);
      cache[i].access_time = clock++;
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  if (cache == NULL){
    return -1;
  }
  if (buf == NULL){
    return -1;
  }
  if (disk_num < 0 || disk_num > 16){
    return -1;
  }
  if (block_num < 0 || block_num > 256){
    return -1;
  }
  
  int lru = 0;
  clock++;
  num_queries++;

  for (int i = 0; i <cache_size; i++){
    if (cache[i].block_num == block_num && cache[i].disk_num == disk_num){
      if (cache[i].valid == true){
        return -1;
      }
    }
  }

  
  for (int j = 0; j < cache_size; j++){
    if (cache[lru].access_time > cache[j].access_time){
      lru = j;
    }
  }

  cache[lru].disk_num = disk_num;
  cache[lru].block_num = block_num;
  cache[lru].valid = true;

  for (int x = 0; x < 256; x++){
    cache[lru].block[x] = buf[x];
  }
  
  cache[lru].access_time = clock;
  return 1;
}

bool cache_enabled(void) {
  if (cache_size <= 2){
    return false;
  }
  return true;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}

