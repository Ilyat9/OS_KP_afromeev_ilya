# Настройки компилятора
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = 

# Имена исполняемых файлов
SERVER_BIN = server
CLIENT_BIN = client

# Исходные файлы
SERVER_SRC = server.c
CLIENT_SRC = client.c
DEFS = defs.h

.PHONY: all clean

# Сборка всего проекта
all: $(SERVER_BIN) $(CLIENT_BIN)

# Сборка сервера
$(SERVER_BIN): $(SERVER_SRC) $(DEFS)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN) $(LDFLAGS)

# Сборка клиента
$(CLIENT_BIN): $(CLIENT_SRC) $(DEFS)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

# Очистка сборочных файлов
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f /tmp/bnc_server_fifo
	rm -f /tmp/bnc_client_*_fifo
