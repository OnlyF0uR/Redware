#include "utils.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telegram.h"

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    printf("Could not allocate memory to the uri variable.\n");
    fflush(stdout);
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void proc_data_msg(int *chat_id, char *id) {
  struct json_object *object, *tmp;

  object = json_object_new_object();

  tmp = json_object_new_int(*chat_id);
  json_object_object_add(object, "chat_id", tmp);

  // Manufacture the text
  char text[29] = "Processing data from ";
  strcat(text, id);
  strcat(text, ":");

  tmp = json_object_new_string(text);
  json_object_object_add(object, "text", tmp);

  // Send the post request
  send_telegram_post("/sendMessage", json_object_to_json_string(object));

  json_object_put(object);
}