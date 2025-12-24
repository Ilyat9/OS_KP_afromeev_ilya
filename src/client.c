#include "defs.h"

char my_fifo[64];

void cleanup() {
    unlink(my_fifo);
}

Response wait_response() {
    Response res;
    int fd = open(my_fifo, O_RDONLY);
    if (fd == -1) {
        perror("open client fifo");
        exit(1);
    }
    read(fd, &res, sizeof(Response));
    close(fd);
    return res;
}

int main() {
    pid_t pid = getpid();
    sprintf(my_fifo, CLIENT_FIFO_TEMPLATE, pid);
    
    if (mkfifo(my_fifo, 0777) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }
    atexit(cleanup);

    printf("Client started (PID: %d)\n", pid);

    while (1) {
        printf("\n1. Create Game\n2. Join Game by Name\n3. Find Random Game\n4. Exit\n> ");
        int choice;
        if (scanf("%d", &choice) != 1) break;

        Request req;
        req.client_pid = pid;

        if (choice == 1) {
            req.cmd = CMD_CREATE;
            printf("Enter Game Name: ");
            scanf("%s", req.game_name);
            printf("Enter Number of Players (e.g. 2): ");
            scanf("%d", &req.max_players);
        } else if (choice == 2) {
            req.cmd = CMD_JOIN_BY_NAME;
            printf("Enter Game Name: ");
            scanf("%s", req.game_name);
        } else if (choice == 3) {
            req.cmd = CMD_JOIN_RANDOM;
            strcpy(req.game_name, "ANY");
        } else {
            break;
        }

        int server_fd = open(SERVER_FIFO, O_WRONLY);
        if (server_fd == -1) {
            perror("Server not running");
            exit(1);
        }
        write(server_fd, &req, sizeof(Request));
        close(server_fd);


        printf("Request sent. Waiting for server...\n");
        
        while(1) {
            Response res = wait_response();
            printf("\n[SERVER]: %s\n", res.message);

            if (res.type == RES_ERROR) break;
            
            if (res.type == RES_GAME_STARTED) {
                int game_active = 1;
                while(game_active) {
                    printf("\nEnter your guess (4 digits): ");
                    scanf("%s", req.guess);
                    req.cmd = CMD_GUESS;
                    
                    server_fd = open(SERVER_FIFO, O_WRONLY);
                    write(server_fd, &req, sizeof(Request));
                    close(server_fd);

                    Response game_res = wait_response();
                    printf("[RESULT]: %s\n", game_res.message);
                    
                    if (game_res.type == RES_WIN) {
                        printf("Game Over.\n");
                        game_active = 0;
                        break; 
                    }
                }
                break; 
            }
        }
    }

    return 0;
}
