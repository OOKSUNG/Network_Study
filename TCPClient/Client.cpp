#pragma comment(lib, "ws2_32.lib") // 링커에 ws2_32.lib를 링크하도록 지시합니다.
#include <iostream>
#include <Ws2tcpip.h> // InetPton 사용
#include <winsock2.h> // win2_32.lib의 선언이 있는 헤더 파일입니다.
#include <thread>
#include "Packet.h"
using namespace std;

#define PACKET_SIZE 2048

SOCKET skt;

bool ReceivePacket(SOCKET skt, PacketReader& outPacket) {
	// 1. 헤더 수신 (Size 4 + Type 2 = 6바이트)
	int recvLen = recv(skt, (char*)outPacket.buffer, 6, 0);
	if (recvLen != 6) return false;

	// 2. 헤더 해석 (내부 offset이 6으로 이동)
	uint32_t totalSize = outPacket.readSize();
	uint16_t type = outPacket.readType();
	uint32_t bodySize = totalSize - 6;

	// 3. 바디 데이터 완성이 될 때까지 수신
	uint32_t totalReceivedBody = 0;
	while (totalReceivedBody < bodySize) {
		int ret = recv(skt, (char*)outPacket.buffer + outPacket.offset + totalReceivedBody,
			bodySize - totalReceivedBody, 0);
		if (ret <= 0) return false;
		totalReceivedBody += ret;
	}

	return true; // 패킷 하나가 완벽하게 조립됨
}

void proc_recv() {
	while (true) {
		PacketReader pr;
		
		ReceivePacket(skt, pr);

		cout << "\n" << pr.readString() << endl;
		cout.flush();
	}
}

int main() {
	// 소켓 구현에 대한 정보가 포함된 구조체
	WSADATA wsa;
	// WSAStartup 함수가 wsa 초기화
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (result != 0) return 1;

	// 소켓 생성
	skt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// PF_INET: IPv4 인터넷 프로토콜
	// SOCK_STREAM: TCP 소켓 타입
	// IPPROTO_TCP: TCP 프로토콜 사용

	// 서버 ip 연결을 위한 주소 구조체 생성
	SOCKADDR_IN addr = {};								// IPv4 주소 정보를 담는 구조체
	addr.sin_family = AF_INET;							// AF_INET: IPv4 인터넷 주소
	addr.sin_port = htons(4444);						// 포트 번호 지정 htons: 시스템의 호스트 바이트 순서에서 네트워크 바이트 순서로 변환(big endian)
	InetPton(AF_INET, L"127.0.0.1", &addr.sin_addr);	// 문자열 IP를 바이너리로 변환

	// 연결될 때까지 반복
	while (true) {
		// connect(소켓, SOCKADDR 구조체(주소 정보), SOCKADDR 구조체 크기)
		if (!connect(skt, (SOCKADDR*)&addr, sizeof(addr))) break;
	}

	// 닉네임 패킷
	Packet p(JOIN);


	// 서버와 연결시 닉네임 설정
	cout << "Please enter your nickname: ";
	char nickname[100];
	cin >> nickname;
	cin.ignore();
	p.writeString(nickname);
	p.finalize();
	// send(소켓, 버퍼에 대한 포인터, 버퍼의 데이터 길이, flag)
	send(skt, p.buffer, p.offset, 0);

	// 스레드 생성
	thread proc1(proc_recv);

	// 보낼 메세지를 저장할 배열
	char msg[PACKET_SIZE] = { 0 };

	while (true) {
		Packet p(CHAT);
		cin.getline(msg, PACKET_SIZE);		// 공백을 포함한 메시지 입력
		p.writeString(msg);
		p.finalize();
		int ret = send(skt, p.buffer, (int)p.offset, 0);
		if (ret < 0) break;					// send는 전송된 총 바이트 수를 반환 
	}

	proc1.join();		// 지정된 스레드가 완료될 때까지 현재 스레드 실행 차단
	closesocket(skt);	// 소켓 종료
	WSACleanup();		// Winsock 정리
}