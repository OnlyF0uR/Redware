#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "http.h"
#include "fiobj.h"
#include "telegram.h"

void on_request(http_s *request);

int chatId;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <BotToken> <ChatId>\n", argv[0]);
    return 1;
  }

  // Construct the default uri
  init_telegram(argv[1]);
  chatId = atoi(argv[2]);

  // Run the "command listener" on a different thread
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, handle_cmds, NULL);

  // Start listening, (Port, Binding(NULL = 0.0.0.0))
  http_listen("8080", NULL, .on_request = on_request, .log = 1);
  fio_start(.threads = 1);

  // Prevent scope collaps (program exit)
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
    // https://core.telegram.org/bots/api#sendmessage
    if (strcmp(path, "/data/text") == 0) {
      char* json = fiobj_obj2cstr(req->body).data;

      FIOBJ obj = FIOBJ_INVALID;
      size_t consumed = fiobj_json2obj(&obj, json, strlen(json));
      if (!consumed || !obj) {
        fprintf(stderr, "Failed to parse body.\n");
      }

      if (FIOBJ_TYPE_IS(obj, FIOBJ_T_HASH)) {
        FIOBJ textKey = fiobj_str_new("text", 4);
        
        if (fiobj_hash_get(obj, textKey)) {
          // Get the text for the message
          char* text = fiobj_obj2cstr(fiobj_hash_get(obj, textKey)).data;

          // Handle the json part
          struct json_object *object, *tmp;
          
          object = json_object_new_object();

          tmp = json_object_new_int(chatId);
          json_object_object_add(object, "chat_id", tmp);
          tmp = json_object_new_string(text);
          json_object_object_add(object, "text", tmp);

          tmp = NULL; // No dangling my friend 

          // Send the post request
          send_message(json_object_to_json_string(object));
          // Cleanup
          json_object_object_del(object, "chat_id");
          json_object_object_del(object, "text");
          free(object);
          free(tmp);
        }

        fiobj_free(textKey);
      } else {
        fprintf(stderr, "Invalid JSON-body.\n");
      }

      fiobj_free(obj);
    }
    else if (strcmp(path, "/data/picture") == 0) {
      // sendPhoto
    }
  }
}
