#pragma once
#include <SFML/Network.hpp>
#include <variant>

#include "ergonomics.hpp"

namespace lan {
namespace packet {

struct RoomBroadcastPacket {
    std::string room_name;
};
sf::Packet& operator<<(sf::Packet& packet, const RoomBroadcastPacket& rbp);
sf::Packet& operator>>(sf::Packet& packet, RoomBroadcastPacket& rbp);

/********************* from guest to host packets start *********************/
struct HeartbeatPacket {
    sf::IpAddress from = sf::IpAddress::Broadcast;
    sf::IpAddress to = sf::IpAddress::Broadcast;
};
sf::Packet& operator<<(sf::Packet& packet, const HeartbeatPacket& heartbeat);
sf::Packet& operator>>(sf::Packet& packet, HeartbeatPacket& heartbeat);

/********************** from guest to host packets end **********************/

/********************* from host to guest packets start *********************/

struct GuestsInRoomPacket {
    size_t guest_count;
    std::vector<std::string> nicknames;
    std::vector<int> signal_strengths;
};
sf::Packet& operator<<(sf::Packet& packet, const GuestsInRoomPacket& gir);
sf::Packet& operator>>(sf::Packet& packet, GuestsInRoomPacket& gir);

struct GameStartingPacket {
    std::string game_name;
};
sf::Packet& operator<<(sf::Packet& packet, const GameStartingPacket& gs);
sf::Packet& operator>>(sf::Packet& packet, GameStartingPacket& gs);

enum class HostToGuestPacketType {
    GuestsInRoom,
    GameStarting,
};

struct HostToGuestPacket {
    std::variant<GuestsInRoomPacket, GameStartingPacket> packet;
};
sf::Packet& operator<<(sf::Packet& packet, const HostToGuestPacket& htgp);
sf::Packet& operator>>(sf::Packet& packet, HostToGuestPacket& htgp);

/********************** from host to guest packets end **********************/

};  // namespace packet
};  // namespace lan
