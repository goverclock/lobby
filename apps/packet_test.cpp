#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <cassert>
#include <memory>
#include <print>

#include "ergonomics.hpp"
#include "lan/packet.hpp"

using namespace lan::packet;

int main() {
    auto prt_htgp = [&](const HostToGuestPacket& htgp) {
        if (std::holds_alternative<GuestsInRoomPacket>(htgp.packet)) {
            std::println("GuestsInRoomPacket");
            auto girp = std::get<GuestsInRoomPacket>(htgp.packet);
            std::println("girp: guest_count = {}", girp.guest_count);
            for (size_t i = 0; i < girp.guest_count; i++)
                std::println("girp: {}, {}", girp.nicknames[i],
                             girp.signal_strengths[i]);

        } else if (std::holds_alternative<GameStartingPacket>(htgp.packet)) {
            std::println("GameStartingPacket");
        }
    };

    sf::Packet packet;

    // sending
    HostToGuestPacket htgp;
    GuestsInRoomPacket girp;
    girp.guest_count = 10;
    for (size_t i = 0; i < girp.guest_count; i++) {
        girp.nicknames.push_back("nick " + std::to_string(i));
        girp.signal_strengths.push_back(i % 4);
    }
    htgp.packet = girp;
    packet << htgp;

    // receiving
    HostToGuestPacket htgp_received;
    GuestsInRoomPacket girp_received;
    packet >> htgp_received;
    prt_htgp(htgp_received);

    return 0;
}
