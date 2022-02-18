#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "http.h"
#include "fiobj.h"

void on_request(http_s *request);
void *handle_cmds(void *arg);

FIOBJ HTTP_HEADER_X_DATA;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <BotToken>\n", argv[0]);
    return 1;
  }

  // Run the bot on different thread
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, handle_cmds, argv[1]);

  // Allocate some freq. used values
  HTTP_HEADER_X_DATA = fiobj_str_new("X-Data", 6);
  // Start listening, (Port, Binding(NULL = 0.0.0.0))
  http_listen("8080", NULL, .on_request = on_request, .log = 1);
  // Start the actual server
  fio_start(.threads = 1);
  // Deallocate the freq. used values
  fiobj_free(HTTP_HEADER_X_DATA);

  pthread_join(thread_id, NULL);

  return 0;
}

void on_request(http_s *req) {
  char *method = fiobj_obj2cstr(req->method).data;
  char *path = fiobj_obj2cstr(req->path).data;

  // Check if the request is a get
  if (strcmp(method, "GET") == 0) {
    // TODO: Command query?
  }
  else if (strcmp(method, "POST") == 0) {
    if (strcmp(path, "/data/text") == 0) {
      char* json = fiobj_obj2cstr(req->body).data;

      FIOBJ obj = FIOBJ_INVALID;
      size_t consumed = fiobj_json2obj(&obj, json, strlen(json));
      if (!consumed || !obj) {
        fprintf(stderr, "Failed to parse body.\n");
      }

      if (FIOBJ_TYPE_IS(obj, FIOBJ_T_HASH)) {
        // Title
        FIOBJ titleKey = fiobj_str_new("title", 5);
        if (fiobj_hash_get(obj, titleKey)) {
          char* title = fiobj_obj2cstr(fiobj_hash_get(obj, titleKey)).data;
          printf("%s\n", title);

          // TODO: sendMessage for telegram
        }

        // Desc
        FIOBJ descKey = fiobj_str_new("description", 11);
        if (fiobj_hash_get(obj, descKey)) {
          char* desc = fiobj_obj2cstr(fiobj_hash_get(obj, descKey)).data;
          printf("%s\n", desc);

          // TODO: sendMessage for telegram
        }

        // Cleanup
        fiobj_free(descKey);
        fiobj_free(titleKey);
        fiobj_free(obj);
      } else {
        fprintf(stderr, "Invalid JSON-body.\n");
      }
    }
    else if (strcmp(path, "/data/picture") == 0) {
      // sendPhoto
    }
  }
}

void *handle_cmds(void *arg) {
  char *token = (char*) arg;
  char *dest = malloc((25 + strlen(token) + 1) * sizeof(char));

  strcpy(dest, "https://api.telegram.org/");
  strcat(dest, token);

  // TODO: Add function here

  // printf("%s\n", dest);
  // fflush(stdout);

  free(dest);
  pthread_exit(NULL);
}
