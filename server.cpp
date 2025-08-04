// tcp_server.cpp
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    const char* response = "메시지 수신 완료";

    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        return 1;
    }

    // 주소 정보 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 모든 IP로부터 수신
    address.sin_port = htons(PORT);       // 포트 설정

    // 바인딩
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    // 클라이언트 연결 대기
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "서버 대기 중..." << std::endl;

    // 클라이언트 연결 수락
    client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (client_socket < 0) {
        perror("accept");
        return 1;
    }

    read(client_socket, buffer, 1024);
    std::cout << "클라이언트 메시지: " << buffer << std::endl;

    send(client_socket, response, strlen(response), 0);
    std::cout << "응답 전송 완료" << std::endl;

    close(client_socket);
    close(server_fd);
    return 0;
}
