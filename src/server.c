#include "defs.h"
#include <time.h>

typedef struct {
    char name[STR_LEN];
    int max_players;
    int current_players;
    pid_t player_pids[MAX_PLAYERS];
    char secret_number[5];
    int is_active;
} Game;

#define MAX_GAMES 10
Game games[MAX_GAMES];

void generate_secret(char* buf) {
    int digits[10] = {0};
    int count = 0;
    while(count < 4) {
        int d = rand() % 10;
        if (!digits[d]) {
            buf[count++] = d + '0';
            digits[d] = 1;
        }
    }
    buf[4] = '\0';
}

void send_response(pid_t client_pid, Response* res) {
    char client_fifo[64];
    sprintf(client_fifo, CLIENT_FIFO_TEMPLATE, client_pid);
    
    int fd = open(client_fifo, O_WRONLY);
    if (fd != -1) {
        write(fd, res, sizeof(Response));
        close(fd);
    }
}

// Подсчет быков и коров
void calculate_bulls_cows(const char* secret, const char* guess, int* bulls, int* cows) {
    *bulls = 0;
    *cows = 0;
    for (int i = 0; i < 4; i++) {
        if (guess[i] == secret[i]) {
            (*bulls)++;
        } else {
            for (int j = 0; j < 4; j++) {
                if (guess[i] == secret[j]) (*cows)++;
            }
        }
    }
}

int main() {
    srand(time(NULL));
    
    // Создаем публичный канал сервера
    if (mkfifo(SERVER_FIFO, 0777) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    printf("Server started. Waiting for requests...\n");

    int server_fd = open(SERVER_FIFO, O_RDWR); // O_RDWR чтобы не получать EOF если нет писателей
    if (server_fd == -1) {
        perror("open server fifo");
        exit(1);
    }

    Request req;
    while (1) {
        ssize_t n = read(server_fd, &req, sizeof(Request));
        if (n > 0) {
            Response res;
            memset(&res, 0, sizeof(Response));
            
            int game_idx = -1;

            if (req.cmd == CMD_CREATE) {
                // Логика создания игры
                int free_slot = -1;
                for (int i = 0; i < MAX_GAMES; i++) {
                    if (games[i].current_players == 0) {
                        free_slot = i;
                        break;
                    }
                }

                if (free_slot != -1) {
                    strncpy(games[free_slot].name, req.game_name, STR_LEN);
                    games[free_slot].max_players = req.max_players;
                    games[free_slot].current_players = 1;
                    games[free_slot].player_pids[0] = req.client_pid;
                    games[free_slot].is_active = 1;
                    generate_secret(games[free_slot].secret_number);
                    
                    printf("Game '%s' created by PID %d. Secret: %s\n", req.game_name, req.client_pid, games[free_slot].secret_number);
                    
                    res.type = RES_WAITING;
                    sprintf(res.message, "Game created. Waiting for opponents (%d/%d)...", 1, req.max_players);
                } else {
                    res.type = RES_ERROR;
                    strcpy(res.message, "Server is full.");
                }
                send_response(req.client_pid, &res);
            } 
            else if (req.cmd == CMD_JOIN_BY_NAME || req.cmd == CMD_JOIN_RANDOM) {
                // Логика присоединения
                int found = -1;
                for (int i = 0; i < MAX_GAMES; i++) {
                    if (games[i].is_active && games[i].current_players < games[i].max_players) {
                        if (req.cmd == CMD_JOIN_RANDOM || strcmp(games[i].name, req.game_name) == 0) {
                            found = i;
                            break;
                        }
                    }
                }


                if (found != -1) {
                    games[found].player_pids[games[found].current_players] = req.client_pid;
                    games[found].current_players++;
                    
                    printf("Player %d joined game '%s'.\n", req.client_pid, games[found].name);

                    // Уведомляем текущего игрока

Илья, [24.12.2025 15:57]
res.type = RES_WAITING;
                    sprintf(res.message, "Joined game '%s'. Waiting (%d/%d)...", games[found].name, games[found].current_players, games[found].max_players);
                    send_response(req.client_pid, &res);

                    // Если лобби заполнено, уведомляем всех о начале
                    if (games[found].current_players == games[found].max_players) {
                        Response start_res;
                        start_res.type = RES_GAME_STARTED;
                        strcpy(start_res.message, "Game Started! Guess the 4-digit number.");
                        for(int p=0; p < games[found].max_players; p++) {
                            send_response(games[found].player_pids[p], &start_res);
                        }
                    }
                } else {
                    res.type = RES_ERROR;
                    strcpy(res.message, "Game not found or full.");
                    send_response(req.client_pid, &res);
                }
            }
            else if (req.cmd == CMD_GUESS) {
                // Логика угадывания
                // Находим игру, в которой этот игрок
                for(int i=0; i<MAX_GAMES; i++) {
                    if(!games[i].is_active) continue;
                    
                    int is_player_in_game = 0;
                    for(int p=0; p<games[i].current_players; p++) {
                        if (games[i].player_pids[p] == req.client_pid) {
                            is_player_in_game = 1;
                            break;
                        }
                    }

                    if(is_player_in_game) {
                        int bulls, cows;
                        calculate_bulls_cows(games[i].secret_number, req.guess, &bulls, &cows);
                        
                        if (bulls == 4) {
                            res.type = RES_WIN;
                            sprintf(res.message, "Player %d WON! The number was %s", req.client_pid, games[i].secret_number);
                            
                            // Рассылаем всем
                            for(int p=0; p<games[i].max_players; p++) {
                                send_response(games[i].player_pids[p], &res);
                            }
                            
                            // Удаляем игру
                            games[i].current_players = 0;
                            games[i].is_active = 0;
                        } else {
                            res.type = RES_TURN_RESULT;
                            res.bulls = bulls;
                            res.cows = cows;
                            sprintf(res.message, "Result: %dB %dC", bulls, cows);
                            send_response(req.client_pid, &res);
                        }
                        break;
                    }
                }
            }
        }
    }
    close(server_fd);
    unlink(SERVER_FIFO);
    return 0;
}
