#ifndef CMDS_DOT_H
#define CMDS_DOT_H

typedef struct {
  char *key;
  char *value;
} HashItem;

typedef struct {
  HashItem **items;
  int size;
  int count;
} HashTable;

HashTable *create_table();
void ht_insert(HashTable *table, char *key, char *value);
char *ht_search(HashTable *table, char *key);
void free_item(HashItem *item);
void free_table(HashTable *table);

#endif