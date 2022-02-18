#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "http.h"
#include "fiobj.h"

void on_request(http_s *request);
void *handle_cmds();

FIOBJ HTTP_HEADER_X_DATA;

char *base_uri;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <BotToken>\n", argv[0]);
    return 1;
  }

  base_uri = malloc((28 + strlen(argv[1]) + 1) * sizeof(char));
  strcpy(base_uri, "https://api.telegram.org/bot");
  strcat(base_uri, argv[1]);

  // Run the bot on different thread
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, handle_cmds, NULL);

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
        // Text
        FIOBJ textKey = fiobj_str_new("text", 4);
        if (fiobj_hash_get(obj, textKey)) {
          char* text = fiobj_obj2cstr(fiobj_hash_get(obj, textKey)).data;

          // Send the curl request
          CURL *curl;
          CURLcode res;

          curl = curl_easy_init();

          if (curl) {
            // JSON Object (https://core.telegram.org/bots/api#sendmessage)
            struct json_object *object, *tmp;

            object = json_object_new_object();
            
            tmp = json_object_new_int(-642850803);
            json_object_object_add(object, "chat_id", tmp);
            tmp = json_object_new_string(text);
            json_object_object_add(object, "text", tmp);

            tmp = NULL;

            // Telegram URI
            char* uri = NULL;
            uri = malloc((strlen(base_uri) + 12 + 1) * sizeof(char));
            strcpy(uri, base_uri);
            strcat(uri, "/sendMessage");

            printf("%s", json_object_to_json_string(object));
            fflush(stdout);

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");

            curl_easy_setopt(curl, CURLOPT_URL, uri);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(object));
            // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"chat_id\":-642850803, \"text\": \"Awesome description mate\"}");
            // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
              fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            curl_easy_cleanup(curl);
            json_object_object_del(object, "chat_id");
            json_object_object_del(object, "text");
            free(object);
          }

          curl_global_cleanup();
        }

        // Cleanup
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

void *handle_cmds() {
  // TODO: Add function here

  // printf("%s\n", base_uri);
  // fflush(stdout);
  pthread_exit(NULL);
}
