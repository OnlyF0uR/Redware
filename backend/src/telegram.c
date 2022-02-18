#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

char *base_uri;

void init_telegram(char* token) {
    base_uri = malloc((28 + strlen(token) + 1) * sizeof(char));
    strcpy(base_uri, "https://api.telegram.org/bot");
    strcat(base_uri, token);
}

void send_message(const char* json_string) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    // Telegram URI
    char *uri = NULL;
    uri = malloc((strlen(base_uri) + 12 + 1) * sizeof(char));
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
  }

  curl_global_cleanup();
  fflush(stdout); // Flush for curl outpu
}

void send_picture(const char* json_string) {
  // TODO: This
  printf("%s", json_string);
  fflush(stdout);
}

void* handle_cmds() {
  // TODO: Implement logic

  // printf("Hi mate I am so cool did you know that?");
  // fflush(stdout);
  pthread_exit(NULL);
}
