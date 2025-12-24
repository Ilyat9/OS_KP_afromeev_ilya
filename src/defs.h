#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_FIFO "/tmp/bnc_server_fifo"
#define CLIENT_FIFO_TEMPLATE "/tmp/bnc_client_%d_fifo"
#define MAX_PLAYERS 4
#define STR_LEN 64

// Типы запросов от клиента
typedef enum {
    CMD_CREATE,
    CMD_JOIN_BY_NAME,
    CMD_JOIN_RANDOM,
    CMD_GUESS,
    CMD_EXIT
} CommandType;

// Структура запроса (Клиент -> Сервер)
typedef struct {
    pid_t client_pid;       // ID процесса клиента (для ответа)
    CommandType cmd;        // Тип команды
    char game_name[STR_LEN];// Имя игры (для создания/поиска)
    int max_players;        // Кол-во игроков (для создания)
    char guess[5];          // Попытка угадать число
} Request;

// Типы ответов от сервера
typedef enum {
    RES_OK,
    RES_ERROR,
    RES_GAME_STARTED,
    RES_TURN_RESULT,
    RES_WIN,
    RES_WAITING
} ResponseType;

// Структура ответа (Сервер -> Клиент)
typedef struct {
    ResponseType type;
    char message[256];
    int bulls;
    int cows;
} Response;

#endif
