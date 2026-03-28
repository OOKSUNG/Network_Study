#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <utility>

using namespace std;

#define PACKET_SIZE 1024
SOCKET skt;		
mutex mtx;		
// 클라이언트 마다 할당한 소켓 관리 {소켓, 닉네임}
vector<pair<SOCKET, string>> clients;

void handle_client(SOCKET client_sock) {
	// 입력받을 버퍼와 nickname
	char buffer[PACKET_SIZE];
	string nickname;

	// 닉네임을 입력 받는다.
	int ret = recv(client_sock, buffer, PACKET_SIZE - 1, 0);
	if (ret <= 0) return;
	buffer[ret] = '\0';
	nickname = buffer;

	// clients는 공유 자원이므로 보호한다.
	mtx.lock();
	for (auto& p : clients) {
		if (p.first == client_sock) {	// 닉네임을 입력받은 소켓이라면
			p.second = nickname;		// 해당 pair의 nickname을 저장
		}
	}
	mtx.unlock();

	// 다른 클라이언트 들에게 접속을 알려줄 문자열
	string msg = nickname + " joined";

	// 다른 클라이언트 들에게 접속 문자열 전송
	mtx.lock();
	for (auto& p : clients) {
		if (p.first != client_sock) send(p.first, msg.c_str(), (int)msg.length(), 0);
	}
	mtx.unlock();

	// 서버에 접속 출력
	cout << "[JOIN] " << nickname << endl;

	while (true) {
		ZeroMemory(buffer, PACKET_SIZE);

		int ret = recv(client_sock, buffer, PACKET_SIZE - 1, 0);

		if (ret <= 0) { 
			// 접속 종료시 left 알림
			string msg = nickname + " left";
			mtx.lock();
			// 모든 클라이언트에게 알림
			for (auto& p : clients) {
				if (p.first != client_sock) send(p.first, msg.c_str(), (int)msg.length(), 0);
			}
			
			// 배열에서 접속 종료한 클라이언트 정보 제거
			clients.erase(std::remove_if(clients.begin(), clients.end(),
				[client_sock](const pair<SOCKET, string>& p) {
					return p.first == client_sock;
				}), clients.end());
			mtx.unlock();
			closesocket(client_sock);

			break;
		}

		buffer[ret] = '\0';
		string full_msg = "[" + nickname + "]: " + buffer;
		cout << "[MSG] " << full_msg << endl;

		// 메세지를 보낸 클라이언트를 제외하고 메세지를 보냄
		mtx.lock();
		for (auto& p : clients) {
			if (p.first != client_sock) {
				send(p.first, full_msg.c_str(), (int)full_msg.length(), 0);
			}
		}
		mtx.unlock();
	}
}


int main() {
	WSADATA wsa;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (result != 0) return 1;

	skt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(4444);
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 로컬 pc의 모든 ip에서 요청 받음

	bind(skt, (SOCKADDR*)&addr, sizeof(addr)); // bind(skt, (SOCKADDR*)&addr, sizeof(addr));
	listen(skt, SOMAXCONN);					   // listen(skt, SOMAXCONN);

	SOCKADDR_IN client = {};
	int client_size = sizeof(client);
	ZeroMemory(&client, client_size);

	while (true) {
		// accept: 클라이언트 요청 수락, 새로운 소켓 반환
		SOCKET new_client = accept(skt, (SOCKADDR*)&client, &client_size);
		if (new_client != INVALID_SOCKET) {
			// clients 벡터는 공유자원이므로 뮤텍스로 보호
			mtx.lock();
			clients.push_back({ new_client, "" });	
			mtx.unlock();
			thread proc2(handle_client, new_client); // 클라이언트 접속 마다 새로운 스레드 생성
			proc2.detach();	// 스레드를 메인과 독립적으로 실행
		}
	}

	// 모든 클라이언트 소켓을 닫는다
	for (pair<SOCKET,string> target : clients) {
		closesocket(target.first);
	}
	clients.clear(); // vector 정리

	closesocket(skt);
	WSACleanup();
}