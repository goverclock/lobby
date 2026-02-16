#include "lan/packet.hpp"

namespace lan {
namespace packet {

sf::Packet& operator<<(sf::Packet& packet, const RoomBroadcastPacket& rbp) {
    return packet << rbp.room_name;
}
sf::Packet& operator>>(sf::Packet& packet, RoomBroadcastPacket& rbp) {
    return packet >> rbp.room_name;
}

// HeartbeatPacketPacket
sf::Packet& operator<<(sf::Packet& packet, const HeartbeatPacket& heartbeat) {
    return packet << heartbeat.from.toString() << heartbeat.to.toString();
}
sf::Packet& operator>>(sf::Packet& packet, HeartbeatPacket& heartbeat) {
    std::string from, to;
    packet >> from >> to;
    heartbeat.from = *sf::IpAddress::resolve(from);
    heartbeat.to = *sf::IpAddress::resolve(to);
    return packet;
}

// GuestsInRoomPacket
sf::Packet& operator<<(sf::Packet& packet, const GuestsInRoomPacket& gir) {
    packet << gir.guest_count;
    for (size_t i = 0; i < gir.guest_count; i++) packet << gir.nicknames[i];
    for (size_t i = 0; i < gir.guest_count; i++)
        packet << gir.signal_strengths[i];
    return packet;
}
sf::Packet& operator>>(sf::Packet& packet, GuestsInRoomPacket& gir) {
    packet >> gir.guest_count;
    gir.nicknames.resize(gir.guest_count);
    for (size_t i = 0; i < gir.guest_count; i++) packet >> gir.nicknames[i];
    gir.signal_strengths.resize(gir.guest_count);
    for (size_t i = 0; i < gir.guest_count; i++)
        packet >> gir.signal_strengths[i];
    return packet;
}

// GameStartingPacket
sf::Packet& operator<<(sf::Packet& packet, const GameStartingPacket& gs) {
    return packet << gs.game_name;
}
sf::Packet& operator>>(sf::Packet& packet, GameStartingPacket& gs) {
    packet >> gs.game_name;
    return packet;
}

// HostToGuestPacket
sf::Packet& operator<<(sf::Packet& packet, const HostToGuestPacket& htgp) {
    HostToGuestPacketType type;
    if (std::holds_alternative<GuestsInRoomPacket>(htgp.packet)) {
        type = HostToGuestPacketType::GuestsInRoom;
        packet << static_cast<int>(type);
        packet << std::get<GuestsInRoomPacket>(htgp.packet);
    } else if (std::holds_alternative<GameStartingPacket>(htgp.packet)) {
        TODO();
    }
    return packet;
}
sf::Packet& operator>>(sf::Packet& packet, HostToGuestPacket& htgp) {
    int t;
    packet >> t;
    HostToGuestPacketType type{t};

    if (type == HostToGuestPacketType::GuestsInRoom) {
        packet >> std::get<GuestsInRoomPacket>(htgp.packet);
    } else if (type == HostToGuestPacketType::GameStarting) {
        TODO();
    }

    return packet;
}

};  // namespace packet
};  // namespace lan
