#include "lan/guest_connection.hpp"
#include "lan/packet.hpp"

namespace lan {

bool GuestConnection::check_update() const {
    if (!getRemoteAddress()) return true;
    if (mLastSignalStrengthl == signal_strength()) return true;
    return false;
}

std::string GuestConnection::guest_ip() const {
    return getRemoteAddress()->toString();
}

std::string GuestConnection::guest_nickname() const {
    return "guestnick" + guest_ip();  // TEST:
}

SignalStrength GuestConnection::signal_strength() const {
    std::time_t now = std::time(nullptr);
    SignalStrength ret = SignalStrength::Lost;
    int time_elapsed = std::difftime(now, mLastHeartbeat);
    if (time_elapsed <= 2)
        ret = SignalStrength::Strong;
    else if (time_elapsed <= 4)
        ret = SignalStrength::Medium;
    else if (time_elapsed <= guest_lost_timeout)
        ret = SignalStrength::Weak;

    return ret;
}

// we don't return anything, we want GuestConnectionManager to check connection
// status ahead of sending/receiving any packet
void GuestConnection::send_packet(packet::HostToGuestPacket htgp) {
    bool done = false;
    sf::Packet packet;
    packet << htgp;

    sf::Socket::Status status = sf::Socket::Status::Partial;
    while (status == sf::Socket::Status::Partial) status = send(packet);
    if (status != sf::Socket::Status::Done)
        std::println("error sending packet, status: %d",
                     static_cast<int>(status));
}

GuestConnectionManager::~GuestConnectionManager() {
    mLock.lock();
    mIsDestructed = true;
    mLock.unlock();
    mBookkeepThread.join();
}

void GuestConnectionManager::add_connection(sf::TcpSocket&& socket) {
    std::lock_guard guard(mLock);
    std::string guest_ip = socket.getRemoteAddress()->toString();
    if (mConnections.find(guest_ip) != mConnections.end()) {
        std::println("connection already exist, this should not happen!");
        UNREACHABLE();
        return;
    }
    mConnections.emplace(guest_ip, std::move(socket));
    mConnectionSelector.add(mConnections[guest_ip]);
}

void GuestConnectionManager::disconnect_all() {
    std::lock_guard guard(mLock);
    mConnectionSelector.clear();
    for (auto& gc : mConnections) gc.second.disconnect();
    mConnections.clear();
}

int GuestConnectionManager::guest_count() {
    std::lock_guard guard(mLock);
    return mConnections.size();
}

// returns true if any guest is lost, or
// signal strength changes
bool GuestConnectionManager::check_guest_info_update() {
    std::lock_guard guard(mLock);
    if (mIsAnyConnErased) {
        mIsAnyConnErased = false;
        return true;
    }
    for (auto& gc : mConnections)
        if (gc.second.check_update()) return true;
    return false;
}

const std::unordered_map<std::string, GuestConnection>&
GuestConnectionManager::guest_connection_list() {
    std::lock_guard guard(mLock);
    return mConnections;
}

void GuestConnectionManager::send_guest_list_to_all() {
    std::lock_guard guard(mLock);

    packet::GuestsInRoomPacket girp;
    girp.guest_count = mConnections.size();
    for (const auto& gc : mConnections) {
        const auto& conn = gc.second;
        girp.nicknames.push_back(conn.guest_nickname());
        girp.signal_strengths.push_back(
            static_cast<int>(conn.signal_strength()));
    }

    packet::HostToGuestPacket htgp;
    htgp.packet = girp;
    // send packet to all connections
    for (auto& gc : mConnections) {
        auto& conn = gc.second;
        // yep, we don't fucking care about if this succeed
        conn.send_packet(htgp);
    }
}

void GuestConnectionManager::bookkeep() {
    std::println("GuestConnectionManager bookkeep thread running");
    while (true) {
        mLock.lock();
        if (mIsDestructed) {
            std::println("GuestConnectionManager bookkeep thread exiting");
            mLock.unlock();
            return;
        }
        // FUCK it we just want to let add_connection can interrupt
        // us but dont't know how to do it conveniently, we have to avoid
        // concurrent access to the selector
        std::time_t now = std::time(nullptr);
        while (mConnectionSelector.wait(sf::seconds(0.000001f))) {
            std::vector<std::string> pending_remove;
            for (auto& gc : mConnections) {
                auto& conn = gc.second;
                if (!mConnectionSelector.isReady(conn)) continue;

                sf::Packet packet;
                auto status = conn.receive(packet);
                // handle heartbeat packet
                switch (status) {
                    case sf::Socket::Status::Done: {
                        packet::HeartbeatPacket hb;
                        packet >> hb;
                        std::println("receiving heartbeatpacket: {}->{} ",
                                     hb.from.toString(), hb.to.toString());
                        conn.mLastHeartbeat = now;
                        break;
                    }
                    case sf::Socket::Status::Disconnected:
                        std::println("guest disconnected: {}, removing ",
                                     gc.first);
                        pending_remove.push_back(gc.first);
                        break;
                    default:
                        std::println("fail to receive packet: {} ",
                                     static_cast<int>(status));
                }
            }
            for (auto& pr : pending_remove) {
                auto& conn = mConnections[pr];
                conn.disconnect();
                mConnectionSelector.remove(conn);
                mConnections.erase(pr);
                mIsAnyConnErased = true;
            }
        }
        mLock.unlock();
        sf::sleep(sf::seconds(1.f));
    }
}

};  // namespace lan
