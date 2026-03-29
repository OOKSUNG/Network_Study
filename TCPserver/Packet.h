#pragma once
#include <cstdint>
#include <cstring>
#include <string>

using namespace std;

enum PacketType {
    CHAT,
    JOIN,
    EXIT
};

class Packet {
public:
    char buffer[2048] = {};
    uint32_t offset = 6;

    Packet(PacketType type) {
        uint16_t t = static_cast<uint16_t>(type);
        memcpy(buffer + 4, &t, 2);
    }

    void writeInt(uint32_t value);

    void writeString(const string& str);

    void finalize();
};

class PacketReader {
public:
    char buffer[1024] = {};
    uint32_t offset = 0;
    uint32_t size;

    PacketType getType() const {
        uint16_t type;
        memcpy(&type, buffer + 4, 2);
        return static_cast<PacketType>(type);
    }

    uint32_t readSize();
    uint16_t readType();
    string readString();
};
