#ifndef UTILS_DOT_H
#define UTILS_DOT_H

#include <stddef.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
void proc_data_msg(int *chat_id, char *id);

#endif