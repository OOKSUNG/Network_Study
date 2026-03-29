#include <cstdint>
#include <cstring>
#include <string>
#include "Packet.h"

using namespace std;

void Packet::writeInt(uint32_t value) {
    memcpy(buffer + offset, &value, sizeof(uint32_t));
    offset += sizeof(uint32_t);
}

void Packet::writeString(const string& str) {
    uint32_t len = static_cast<uint32_t>(str.length());
    writeInt(len);
    memcpy(buffer + offset, str.c_str(), len);
    offset += len;
}

void Packet::finalize() {
    uint32_t size = offset;
    memcpy(buffer, &size, sizeof(uint32_t));
}

uint32_t PacketReader::readSize() {
    memcpy(&size, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return size;
}

uint16_t PacketReader::readType() {
    uint16_t val;
    memcpy(&val, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return val;
}

string PacketReader::readString() {
    // 1. 기록된 문자열의 길이를 먼저 읽어옵니다 (offset이 4바이트 이동)
    uint32_t len;
    memcpy(&len, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 2. 딱 그 길이만큼만 string 객체로 만듭니다
    string s(buffer + offset, len);
    offset += len;
    return s;
}