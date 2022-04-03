/**
 * Partially inspired by:
 * https://www.journaldev.com/35238/hash-table-in-c-plus-plus
 */
#include "cmds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACITY 3000

unsigned long hash_func(char *str) {
  unsigned long i = 0;
  for (int j = 0; str[j]; j++) i += str[j];
  return i % CAPACITY;
}

HashItem *create_item(char *key, char *value) {
  HashItem *item = (HashItem *)malloc(sizeof(HashItem));
  item->key = (char *)malloc(strlen(key) + 1);
  item->value = (char *)malloc(strlen(value) + 1);

  strcpy(item->key, key);
  strcpy(item->value, value);

  return item;
}

HashTable *create_table() {
  HashTable *table = (HashTable *)malloc(sizeof(HashTable));

  table->size = CAPACITY;
  table->count = 0;
  table->items = (HashItem **)calloc(table->size, sizeof(HashItem *));

  for (int i = 0; i < table->size; i++) {
    table->items[i] = NULL;
  }

  return table;
}

void free_item(HashItem *item) {
  free(item->key);
  free(item->value);
  free(item);
}

void consume(HashTable *table, char *key) {
  int index = hash_func(key);
  HashItem *item = table->items[index];
  if (item != NULL) {
    if (strcmp(item->key, key) == 0) {
      free_item(item);
    }
  }
}

void free_table(HashTable *table) {
  for (int i = 0; i < table->size; i++) {
    HashItem *item = table->items[i];
    if (item != NULL) {
      free_item(item);
    }
  }

  free(table->items);
  free(table);
}

void ht_insert(HashTable *table, char *key, char *value) {
  HashItem *item = create_item(key, value);

  unsigned long index = hash_func(key);
  HashItem *current_item = table->items[index];

  if (current_item == NULL) {
    if (table->count == table->size) {
      printf("Hash Table is full\n");
      free_item(item);
      return;
    }

    table->items[index] = item;
    table->count++;
  } else {
    if (strcmp(current_item->key, key) == 0) {
      strcpy(table->items[index]->value, value);
      return;
    } else {
      // Collision handling is not really worth it in this case
      return;
    }
  }
}

char *ht_search(HashTable *table, char *key) {
  int index = hash_func(key);
  HashItem *item = table->items[index];

  if (item != NULL) {
    if (strcmp(item->key, key) == 0) {
      return item->value;
    }
  }
  return NULL;
}