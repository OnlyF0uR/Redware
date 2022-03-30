#ifndef TELEGRAM_DOT_H
#define TELEGRAM_DOT_H

void init_telegram(char *token);
void *handle_cmds();

void send_telegram_post(const char *ep, const char *data);
void write_commands(json_object *buf_arr);

#endif