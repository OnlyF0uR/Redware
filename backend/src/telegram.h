#ifndef TELEGRAM_DOT_H
#define TELEGRAM_DOT_H

void init_telegram(char* token);
void *handle_cmds();

void send_message(const char* json_string);
void send_picture(const char* json_string);

#endif