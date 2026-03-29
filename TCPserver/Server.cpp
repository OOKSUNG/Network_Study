#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <utility>
#include "Packet.h"


using namespace std;

#define PACKET_SIZE 2048
SOCKET skt;		
mutex mtx;		
// 클라이언트 마다 할당한 소켓 관리 {소켓, 닉네임}
struct Client {
	SOCKET sock;
	string nickname;
};
vector<Client> clients;

bool ReceivePacket(SOCKET skt, PacketReader& outPacket) {
	// 헤더 수신 (Size 4 + Type 2 = 6바이트)
	int recvLen = recv(skt, (char*)outPacket.buffer, 6, 0);
	if (recvLen != 6) return false;

	// 헤더 해석 (내부 offset이 6으로 이동)
	uint32_t totalSize = outPacket.readSize();
	uint16_t type = outPacket.readType();
	uint32_t bodySize = totalSize - 6;

	// 바디 데이터 완성이 될 때까지 수신
	uint32_t totalReceivedBody = 0;
	while (totalReceivedBody < bodySize) {
		int ret = recv(skt, (char*)outPacket.buffer + outPacket.offset + totalReceivedBody,
			bodySize - totalReceivedBody, 0);
		if (ret <= 0) return false;
		totalReceivedBody += ret;
	}

	return true; 
}

void handle_client(SOCKET client_sock) {
	// 닉네임을 입력 받는다.
	string nickname;
	PacketReader pr;

	ReceivePacket(client_sock, pr);
	nickname = pr.readString();

	// clients는 공유 자원이므로 보호한다.
	mtx.lock();
	for (auto& c : clients) {
		if (c.sock == client_sock) {	// 닉네임을 입력받은 소켓이라면
			c.nickname = nickname;		// 해당 pair의 nickname을 저장
		}
	}
	mtx.unlock();

	// 다른 클라이언트 들에게 접속을 알려줄 문자열
	string msg = nickname + " joined";

	// 다른 클라이언트 들에게 접속 문자열 전송
	Packet p(CHAT);
	p.writeString(msg);
	p.finalize();

	mtx.lock();
	for (auto& c : clients) {
		if (c.sock != client_sock) send(c.sock, p.buffer, (int)p.offset, 0);
	}
	mtx.unlock();

	// 서버에 접속 출력
	cout << "[JOIN] " << nickname << endl;

	while (true) {
		//ZeroMemory(buffer, PACKET_SIZE);

		//int ret = recv(client_sock, buffer, PACKET_SIZE - 1, 0);

		PacketReader pr;
		bool isRet = ReceivePacket(client_sock, pr);
		
		if (!isRet) { 
			// 접속 종료시 left 알림
			string msg = nickname + " left";
			Packet p(CHAT);
			p.writeString(msg);
			p.finalize();

			mtx.lock();
			// 모든 클라이언트에게 알림
			for (auto& c : clients) {
				if (c.sock != client_sock) send(c.sock, p.buffer, (int)p.offset, 0);
			}
			
			// 배열에서 접속 종료한 클라이언트 정보 제거
			clients.erase(std::remove_if(clients.begin(), clients.end(),
				[client_sock](const Client& c) {
					return c.sock == client_sock;
				}), clients.end());
			mtx.unlock();
			closesocket(client_sock);

			break;
		}

		string full_msg = "[" + nickname + "]: " + pr.readString();
		cout << "[MSG] " << full_msg << endl;

		Packet p(CHAT);
		p.writeString(full_msg);
		p.finalize();

		// 메세지를 보낸 클라이언트를 제외하고 메세지를 보냄
		mtx.lock();
		for (auto& c : clients) {
			if (c.sock != client_sock) {
				send(c.sock, p.buffer, (int)p.offset, 0);
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
	for (auto& c : clients) {
		closesocket(c.sock);
	}
	clients.clear(); // vector 정리

	closesocket(skt);
	WSACleanup();
}