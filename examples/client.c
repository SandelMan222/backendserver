#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock;
    struct sockaddr_in server;
    char message[] = "Hello from C client!";
    char reply[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(sock, (struct sockaddr*)&server, sizeof(server));
    send(sock, message, strlen(message), 0);
    recv(sock, reply, sizeof(reply), 0);
    printf("Received: %s\n", reply);

    close(sock);
    return 0;
}

