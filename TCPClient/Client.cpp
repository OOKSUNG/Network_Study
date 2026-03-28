#pragma comment(lib, "ws2_32.lib") // 링커에 ws2_32.lib를 링크하도록 지시합니다.
#include <iostream>
#include <Ws2tcpip.h> // InetPton 사용
#include <winsock2.h> // win2_32.lib의 선언이 있는 헤더 파일입니다.
#include <thread>
using namespace std;

#define PACKET_SIZE 1024

SOCKET skt;

void proc_recv() {
	char buffer[PACKET_SIZE] = {}; // 받은 메시지를 저장할 char 배열
	while (true) {
		// buffer 배열을 0으로 채워준다.
		ZeroMemory(&buffer, PACKET_SIZE); // ZeroMemory(PVOID Destination, SIZE_T Length)
		// 수신된 데이터의 실제 바이트 수를 ret 변수에 반환한다.
		int ret = recv(skt, buffer, PACKET_SIZE - 1, 0); // int recv(SOCKET s, char *buf, int len, int flags);
		if (ret > 0) {
			buffer[ret] = '\0';		// buffer의 마지막에 줄바꿈 추가
			cout << "\n" << buffer << endl;		// 받은 메시지 출력
			cout.flush();			// 출력 스티림 버퍼를 비워 버퍼에 남아 있는 데이터를 즉시 터미널에 출력
		}
		// recv는 상대의 소켓이 닫았다면 0을 반환, 오류시 SOCKET_ERROR 반환
		else {
			break;
		}
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

	// 서버와 연결시 닉네임 설정
	cout << "Please enter your nickname: ";
	char nickname[100];
	cin >> nickname;
	// send(소켓, 버퍼에 대한 포인터, 버퍼의 데이터 길이, flag)
	send(skt, nickname, (int)strlen(nickname), 0);

	// 스레드 생성
	thread proc1(proc_recv);

	// 보낼 메세지를 저장할 배열
	char msg[PACKET_SIZE] = { 0 };

	while (true) {
		cin.getline(msg, PACKET_SIZE);		// 공백을 포함한 메시지 입력
		int ret = send(skt, msg, (int)strlen(msg), 0);
		if (ret < 0) break;					// send는 전송된 총 바이트 수를 반환 
	}

	proc1.join();		// 지정된 스레드가 완료될 때까지 현재 스레드 실행 차단
	closesocket(skt);	// 소켓 종료
	WSACleanup();		// Winsock 정리
}