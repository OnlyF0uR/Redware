#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <curl/curl.h>
#include <json-c/json.h>

#include "utils.h"
 
size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    printf("Could not allocate memory to the uri variable.\n");
    fflush(stdout);
    exit(1);
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

const char *error_res(const char *reason) {
  json_object *obj = json_object_new_object();
  
  json_object *ok = json_object_new_boolean(0);
  json_object *rsn = json_object_new_string(reason);

  json_object_object_add(obj, "ok", ok);
  json_object_object_add(obj, "reason", rsn);

  return json_object_to_json_string(obj);
}