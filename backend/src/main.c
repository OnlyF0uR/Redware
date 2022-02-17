#include "http.h"
#include "fiobj.h"
#include <string.h>

void on_request(http_s *request);

FIOBJ HTTP_HEADER_X_DATA;

int main() {
  // Allocate some freq. used values
  HTTP_HEADER_X_DATA = fiobj_str_new("X-Data", 6);
  // Start listening, (Port, Binding(NULL = 0.0.0.0))
  http_listen("8080", NULL, .on_request = on_request, .log = 1);
  // Start the actual server
  fio_start(.threads = 1);
  // Deallocate the freq. used values
  fiobj_free(HTTP_HEADER_X_DATA);

  return 0;
}

void on_request(http_s *req) {
  /* /data/activity
   * /data/pic
   * /data/text
   */
  if (strcmp(fiobj_obj2cstr(req->path).data, "/data") == 0) {
    http_send_body(req, "Sending juicy data huh?\r\n", 25);
  } else {
    http_send_body(req, "Nothing here mate!\r\n", 18);
  }
}