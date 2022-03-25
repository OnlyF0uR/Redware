#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "utils.h"

char *base_uri;

void init_telegram(char* token) {
    base_uri = malloc((28 + strlen(token) + 1) * sizeof(char));
    if (!base_uri) {
      printf("Could not allocate memory to the base_uri variable.\n");
      fflush(stdout);
      exit(1);
    }
    strcpy(base_uri, "https://api.telegram.org/bot");
    strcat(base_uri, token);
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

void get_updates() {
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
      // TODO: Do something with the data
    }

    curl_easy_cleanup(curl);
    free(uri);
  }
  
  curl_global_cleanup();
  fflush(stdout); // Flush for curl outpu
}

void* handle_cmds() {
  while (1) {
    // get_updates() (https://core.telegram.org/bots/api#getupdates)

    sleep(1);
  }

  // printf("Hi mate I am so cool did you know that?");
  // fflush(stdout);
  pthread_exit(NULL);
}
