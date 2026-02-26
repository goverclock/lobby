// GameStartingPacket test

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
    auto prt_gsp = [&](const GameStartingPacket& gsp) {
        std::println("GameStartingPacket:");
        std::println("game_name: {}", gsp.game_name);
        size_t player_count = gsp.player_nicknames.size();
        for (size_t i = 0; i < player_count; i++)
            std::println("nick: {}, ip: {}", gsp.player_nicknames[i],
                         gsp.player_ips[i]);
    };

    auto prt_htgp = [&](const HostToGuestPacket& htgp) {
        if (std::holds_alternative<GuestsInRoomPacket>(htgp.packet)) {
            std::println("GuestsInRoomPacket");
            auto girp = std::get<GuestsInRoomPacket>(htgp.packet);
            std::println("girp: guest_count = {}", girp.guest_count);
            for (size_t i = 0; i < girp.guest_count; i++)
                std::println("girp: {}, {}", girp.nicknames[i],
                             girp.signal_strengths[i]);
        } else if (std::holds_alternative<GameStartingPacket>(htgp.packet)) {
            prt_gsp(std::get<GameStartingPacket>(htgp.packet));
        }
    };

    sf::Packet packet;

    // sending
    HostToGuestPacket htgp;
    GameStartingPacket gsp;
    gsp.game_name = "the fucking game name";
    size_t player_count = 6;
    for (size_t i = 0; i < player_count; i++) {
        gsp.player_nicknames.push_back("nick " + std::to_string(i) + "lol");
        gsp.player_ips.push_back("(pure ip" + std::to_string(i) + ")");
    }
	htgp.packet = gsp;
    packet << htgp;

    // receiving
    HostToGuestPacket htgp_received;
    GameStartingPacket gsp_received;
    packet >> htgp_received;
    prt_htgp(htgp_received);

    return 0;
}
