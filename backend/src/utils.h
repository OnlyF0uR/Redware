#ifndef UTILS_DOT_H
#define UTILS_DOT_H

struct MemoryStruct
{
  char *memory;
  size_t size;
};

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
const char *error_res(const char *reason);

#endif