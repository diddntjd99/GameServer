#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib") // 링커에 ws2_32 라이브러리 연결

using namespace std;

struct PER_IO_CONTEXT {
	OVERLAPPED overlapped{};
	WSABUF wsaBuf;
	char buffer[1024];
	int operationType; // 예: 0=recv, 1=send
};

void SocketThread(HANDLE hIOCP) {
	DWORD bytesTransferred;
	ULONG_PTR key;
	OVERLAPPED* pOverlapped;

	while (true) {
        //IOCP 이벤트 대기
		BOOL result = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &key, &pOverlapped, INFINITE);
		SOCKET clientSock = (SOCKET)key;
		auto* ctx = reinterpret_cast<PER_IO_CONTEXT*>(pOverlapped);

		if (!result || bytesTransferred == 0) {
			cout << "클라이언트 연결 종료" << endl;
			closesocket(clientSock);
			delete ctx;
			continue;
		}

        if (ctx->operationType == 0) { // recv 완료
            // 수신 처리
            if (bytesTransferred < sizeof(ctx->buffer))
                ctx->buffer[bytesTransferred] = '\0';

            cout << "클라이언트 메시지: " << ctx->buffer << endl;

            // Echo 처리
            send(clientSock, ctx->buffer, bytesTransferred, 0);

            // 다음 수신 예약
            ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
            DWORD flags = 0;
            WSARecv(clientSock, &ctx->wsaBuf, 1, nullptr, &flags, &ctx->overlapped, nullptr);
        }
        if (ctx->operationType == 1) { // send 완료
            cout << "송신 완료!" << endl;
            delete ctx;
        }
	}
}

int main() {
    WSADATA wsaData;
    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 실패" << endl;
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        cerr << "소켓 생성 실패" << endl;
        WSACleanup();
        return 1;
    }
    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(65432);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "바인딩 실패" << endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    listen(listenSock, SOMAXCONN);

    //IOCP 핸들 생성
    HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // 워커 스레드 4개 생성
    for (int i = 0; i < 4; ++i) {
        thread(SocketThread, hIOCP).detach();
    }

    cout << "서버 대기 중..." << endl;

    while (true) {
        // 클라이언트 연결 수락
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) {
            cerr << "클라이언트 수락 실패" << endl;
            break;
        }

        //IOCP에 client 소켓을 등록
        CreateIoCompletionPort((HANDLE)clientSock, hIOCP, (ULONG_PTR)clientSock, 0);
        cout << "클라이언트 연결 성공" << endl;

        //첫 비동기 수신 등록
        auto* ctx = new PER_IO_CONTEXT();
        ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
        ctx->wsaBuf.buf = ctx->buffer;
        ctx->wsaBuf.len = sizeof(ctx->buffer);
        ctx->operationType = 0;

        DWORD flags = 0;
        WSARecv(clientSock, &ctx->wsaBuf, 1, nullptr, &flags, &ctx->overlapped, nullptr);
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
