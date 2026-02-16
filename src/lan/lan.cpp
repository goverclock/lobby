#include <cassert>
#include <cstring>
#include <print>
#include <thread>
#include <tuple>

#include "ergonomics.hpp"
#include "lan/lan.hpp"
#include "lan/packet.hpp"

namespace lan {

std::optional<LanMessageUpdated> LanPeer::poll_updates() {
    std::lock_guard guard(mLock);
    if (mUpdatedSet.empty()) return {};
    LanMessageUpdated ret = *mUpdatedSet.begin();
    mUpdatedSet.erase(*mUpdatedSet.begin());
    return ret;
}

void LanPeer::enque_updateL(LanMessageUpdated message) {
    assert(!mLock.try_lock());
    mUpdatedSet.insert(message);
}

void LanPeer::start_periodically_broadcast() {
    mIsBroadcasting.store(true);
    std::thread broadcasting_thread([this] {
        sf::UdpSocket socket;
        socket.setBlocking(false);
        while (mIsBroadcasting.load()) {
            sf::Packet packet;
            lan::packet::RoomBroadcastPacket rbp;
            rbp.room_name = "someroomname";
            packet << rbp;

            sf::Socket::Status status = sf::Socket::Status::Partial;
            while (status == sf::Socket::Status::Partial)
                status = socket.send(packet, sf::IpAddress::Broadcast, PORT);
            assert(status == sf::Socket::Status::Done);
            std::println("broadcasting rbp: {}", rbp.room_name);
            sf::sleep(sf::seconds(1.f));
        }
    });

    broadcasting_thread.detach();
}

void LanPeer::stop_periodically_broadcast() { mIsBroadcasting.store(false); }

void LanPeer::start_periodically_discover() {
    mIsDiscovering.store(true);
    std::thread discovering_thread([this] {
        sf::UdpSocket socket;
        socket.setBlocking(false);

        // try to bind until success or no longer discovering
        while (socket.bind(PORT) != sf::Socket::Status::Done &&
               mIsDiscovering.load()) {
            std::println("fail to UDP bind port {}, try again in 2s", PORT);
            update_room_info();
            sf::sleep(sf::seconds(2.f));
        }

        sf::Socket::Status status;
        sf::Packet packet;
        std::optional<sf::IpAddress> sender_ip = sf::IpAddress::Broadcast;
        unsigned short sender_port;
        while (mIsDiscovering.load()) {
            status = socket.receive(packet, sender_ip, sender_port);
            switch (status) {
                case sf::Socket::Status::Done: {
                    lan::packet::RoomBroadcastPacket rbp;
                    packet >> rbp;
                    std::println("received rbp from {}:{}, room_name: '{}' ",
                                 sender_ip->toString(), sender_port,
                                 rbp.room_name);
                    mLock.lock();
                    mRoomLastHeard[sender_ip->toString()] = std::time(nullptr);
                    mRoomInfoList[sender_ip->toString()].name = rbp.room_name;
                    mLock.unlock();
                    break;
                }
                case sf::Socket::Status::NotReady:
                    sf::sleep(sf::seconds(1.f));
                    break;
                default:
                    std::println("discovery status: {}",
                                 static_cast<int>(status));
            }
            update_room_info();
        }
    });

    discovering_thread.detach();
}

void LanPeer::stop_periodically_discover() { mIsDiscovering.store(false); }

void LanPeer::start_listen_guest() {
    mIsListeningGuest.store(true);
    std::thread listen_guest_thread([this] {
        sf::TcpListener listener;
        if (listener.listen(PORT) != sf::Socket::Status::Done) {
            std::println("fail to TCP listen on port {}", PORT);
            this->mIsListeningGuest.store(false);
        }
        sf::SocketSelector selector;
        selector.add(listener);

        while (mIsListeningGuest.load()) {
            // new guest connection ?
            sf::TcpSocket guest;
            if (selector.wait(sf::seconds(1.0f))) {
                // no need to call isReady since we added only the listener
                const sf::Socket::Status status = listener.accept(guest);
                switch (status) {
                    case sf::Socket::Status::Done: {
                        std::println("new guest connected!: {} ",
                                     guest.getRemoteAddress()->toString());
                        mGuestConnectionManager.add_connection(
                            std::move(guest));
                        mGuestConnectionManager.send_guest_list_to_all();
                        break;
                    }
                    case sf::Socket::Status::NotReady:
                        break;
                    default:
                        std::println("listen guest failed with status: {} ",
                                     static_cast<int>(status));
                }
            } else
                // only update guest info when no new guest connection is
                // coming, i.e. when selector.wait() returns false
                update_connected_guest_info();
        }
    });
    listen_guest_thread.detach();
}

void LanPeer::stop_listen_guest() { mIsListeningGuest.store(false); }

void LanPeer::disconnect_all_guests() {
    mGuestConnectionManager.disconnect_all();
}

void LanPeer::update_connected_guest_info() {
    std::lock_guard guard(mLock);
    if (mGuestConnectionManager.check_guest_info_update())
        enque_updateL(LanMessageUpdated::NewGuestInRoom);
}

std::tuple<std::vector<std::string>, std::vector<SignalStrength>>
LanPeer::get_connected_guest_info_list() {
    std::lock_guard guard(mLock);
    const auto& guest_conn_list =
        mGuestConnectionManager.guest_connection_list();
    std::vector<std::string> nicknames;
    std::vector<SignalStrength> signal_strengths;
    for (const auto& gc : guest_conn_list) {
        auto& conn = gc.second;
        nicknames.push_back(conn.guest_nickname());
        signal_strengths.push_back(conn.signal_strength());
    }

    return std::make_tuple(nicknames, signal_strengths);
}

void LanPeer::update_room_info() {
    std::lock_guard guard(mLock);
    std::time_t now = std::time(nullptr);
    bool updated = false;
    static constexpr int room_lost_timeout = 6 * BROADCAST_INTERVAL;

    // remove lost rooms
    for (auto it = mRoomLastHeard.begin(); it != mRoomLastHeard.end();) {
        if (std::difftime(now, it->second) > room_lost_timeout) {
            std::println("enque update for RoomInfo from {} to lost",
                         it->first);
            updated = true;
            mRoomInfoList.erase(it->first);
            it = mRoomLastHeard.erase(it);
        } else
            it++;
    }

    // update signal strength for rooms that isn't lost
    for (const auto& lh : mRoomLastHeard) {
        RoomInfo expected{
            .name = mRoomInfoList[lh.first].name,
            .ip = lh.first,
        };
        int time_elapsed = std::difftime(now, lh.second);
        if (time_elapsed <= 2 * LanPeer::BROADCAST_INTERVAL)
            expected.signal_strength = SignalStrength::Strong;
        else if (time_elapsed <= 4 * LanPeer::BROADCAST_INTERVAL)
            expected.signal_strength = SignalStrength::Medium;
        else if (time_elapsed <= room_lost_timeout)
            expected.signal_strength = SignalStrength::Weak;

        if (mRoomInfoList[lh.first] != expected) {
            std::println(
                "enque update for RoomInfo from {} to {}, room_name: {}",
                mRoomInfoList[lh.first].to_string(), expected.to_string(),
                mRoomInfoList[lh.first].name);
            mRoomInfoList[lh.first] = expected;
            updated = true;
        }
    }
    if (updated) enque_updateL(LanMessageUpdated::RoomInfoList);
}

std::vector<RoomInfo> LanPeer::get_room_info_list() {
    std::lock_guard guard(mLock);
    std::vector<RoomInfo> ret;
    for (const auto& p : mRoomInfoList) ret.push_back(p.second);
    return ret;
}

bool LanPeer::connect_to_host(std::string host_ip) {
    // TODO: for now we just block on the connect call, which means main
    // thread is freezed
    mToHostTcpSocket.setBlocking(true);
    sf::Socket::Status status =
        mToHostTcpSocket.connect(*sf::IpAddress::resolve(host_ip), PORT);
    mToHostTcpSocket.setBlocking(false);
    std::println("trying to connect to {}:{}, got status {}", host_ip, PORT,
                 static_cast<int>(status));
    if (status == sf::Socket::Status::Done) {
        std::println("successfully connected to host!");
        return true;
    }
    std::println("fail to connected to host!");
    return false;
}

void LanPeer::disconnect_from_host() {
    std::println("disconnecting from host");
    mToHostTcpSocket.disconnect();
}

void LanPeer::start_heartbeat_to_host() {
    mIsHeartbeating.store(true);
    std::thread heartbeat_thread([this] {
        sf::Packet heartbeat_packet;
        lan::packet::HeartbeatPacket heartbeat{
            *sf::IpAddress::getLocalAddress(),
            *mToHostTcpSocket.getRemoteAddress()};
        heartbeat_packet << heartbeat;
        while (mIsHeartbeating.load()) {
            sf::sleep(sf::seconds(1.f));
            std::println("sending heartbeat packet to host: from {} to {}",
                         heartbeat.from.toString(), heartbeat.to.toString());

            std::lock_guard guard(mLock);
            sf::Socket::Status status = mToHostTcpSocket.send(heartbeat_packet);
            while (status == sf::Socket::Status::Partial)
                status = mToHostTcpSocket.send(heartbeat_packet);
            if (status != sf::Socket::Status::Done) {
                std::println(
                    "(sending heartbeat)host lost, exit room passively as "
                    "guest");
                enque_updateL(LanMessageUpdated::HostDismissRoom);
                break;
            }
        }
    });
    heartbeat_thread.detach();
}

void LanPeer::stop_heartbeat_to_host() { mIsHeartbeating.store(false); }

void LanPeer::start_listen_host_packet() {
    mIsReceivingHostPacket.store(true);
    std::thread listen_host_packet_thread([this] {
        while (mIsReceivingHostPacket.load()) {
            sf::Packet host_packet;
            packet::HostToGuestPacket htgp;
            std::lock_guard guard(mLock);
            sf::Socket::Status status = mToHostTcpSocket.receive(host_packet);
            switch (status) {
                case sf::Socket::Status::Done:
                    host_packet >> htgp;
                    if (std::holds_alternative<packet::GuestsInRoomPacket>(
                            htgp.packet)) {
                        std::println("receiving GuestsInRoomPacket");
                        mPendingGIRP =
                            std::get<packet::GuestsInRoomPacket>(htgp.packet);
                        enque_updateL(LanMessageUpdated::NewGuestInRoom);
                    }
                    break;
                case sf::Socket::Status::NotReady:
                    // just next loop
                    break;
                default:  // Disconnected or Error
                    std::println("host lost, exit room passively as guest");
                    enque_updateL(LanMessageUpdated::HostDismissRoom);
                    mIsReceivingHostPacket.store(false);
                    break;
            }
        }
    });
    listen_host_packet_thread.detach();
}

packet::GuestsInRoomPacket LanPeer::get_pending_girp() {
    std::lock_guard guard(mLock);
    return mPendingGIRP;
}

void LanPeer::stop_listen_host_packet() { mIsReceivingHostPacket.store(false); }

};  // namespace lan
