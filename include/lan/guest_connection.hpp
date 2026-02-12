#pragma once
#include <SFML/Network.hpp>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "ergonomics.hpp"
#include "lan/guest_connection.hpp"
#include "lan/packet.hpp"

namespace lan {

enum class SignalStrength {
    Weak,
    Medium,
    Strong,
    Lost,
};

// no lock in this class, the GuestConnectionManager should take care of the
// concurrent access to it
class GuestConnection : public sf::TcpSocket {
   public:
    GuestConnection() { UNREACHABLE(); };
    GuestConnection(sf::TcpSocket&& socket)
        : sf::TcpSocket(std::move(socket)),
          mLastHeartbeat(std::time(nullptr)) {};

    bool check_update() const;
    std::string guest_ip() const;
    std::string guest_nickname() const;
    static constexpr int guest_lost_timeout = 5;
    SignalStrength signal_strength() const;
    void send_packet(packet::HostToGuestPacket packet);

    std::time_t mLastHeartbeat;

   private:
    // record return value of last signal_strength() call, to calculate updates
    SignalStrength mLastSignalStrengthl = SignalStrength::Strong;
};

class GuestConnectionManager {
   public:
    GuestConnectionManager() : mBookkeepThread([&]() { bookkeep(); }) {};
    ~GuestConnectionManager();

    void add_connection(sf::TcpSocket&& socket);
    // no need for remove connection method, beacause it's already handled
    // inside this class
    void disconnect_all();
    int guest_count();
    bool check_guest_info_update();
    const std::unordered_map<std::string, GuestConnection>&
    guest_connection_list();
    void send_guest_list_to_all();

   private:
    void bookkeep();

    std::mutex mLock;
    bool mIsDestructed = false;
    bool mIsAnyConnErased = false;
    std::unordered_map<std::string, GuestConnection> mConnections;
    sf::SocketSelector mConnectionSelector;
    std::thread mBookkeepThread;
};

};  // namespace lan
