#include <cassert>
#include <ctime>
#include <queue>

#include "ergonomics.hpp"
#include "mscore/local_status.hpp"

LocalStatus::LocalStatus() : mGameStatus(GameStatus::Lobby) {}

const GameStatus& LocalStatus::game_status() { return mGameStatus; }

void LocalStatus::update() {
    while (const std::optional lan_update = mLanPeer.poll_updates()) {
        switch (*lan_update) {
            case lan::LanMessageUpdated::RoomInfoList: {
                mRoomEntryList.clear();
                std::vector<lan::RoomInfo> room_info_list =
                    mLanPeer.get_room_info_list();
                for (const auto& ri : room_info_list) {
                    mRoomEntryList.push_back(RoomEntry{
                        .name = ri.name,
                        .ip = ri.ip,
                        .signal_strength =
                            static_cast<int>(ri.signal_strength) + 1});
                }
                break;
            }
            case lan::LanMessageUpdated::NewGuestInRoom: {
                mGuestInfoList.clear();
                if (mGameStatus == GameStatus::RoomAsHost) {
                    auto [nicknames, signal_strengths] =
                        mLanPeer.get_connected_guest_info_list();
                    size_t guest_count = nicknames.size();
                    for (size_t i = 0; i < guest_count; i++)
                        mGuestInfoList.push_back(
                            GuestInfo{.nickname = nicknames[i],
                                      .ip = "0.0.0.0",
                                      .signal_strength = signal_strengths[i]});
                } else if (mGameStatus == GameStatus::RoomAsGuest) {
                    lan::packet::GuestsInRoomPacket girp =
                        mLanPeer.get_pending_girp();
                    for (size_t i = 0; i < girp.guest_count; i++)
                        mGuestInfoList.push_back(GuestInfo{
                            .nickname = girp.nicknames[i],
                            .ip = "0.0.0.0",
                            .signal_strength = static_cast<lan::SignalStrength>(
                                girp.signal_strengths[i])});
                } else {
                    UNREACHABLE();
                }
                break;
            }
            case lan::LanMessageUpdated::GameStarting: {
                assert(mGameStatus ==
                       GameStatus::RoomAsGuest);  // because this message is
                                                  // send from host to guest
                mIsGameRunning = true;
            } break;
            case lan::LanMessageUpdated::HostDismissRoom: {
                std::println("host dismissed room, returning to lobby");
                guest_exit_room();
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }
}

const LocalStatus::RoomEntryList& LocalStatus::get_room_entry_list() {
    return mRoomEntryList;
}

void LocalStatus::host_exit_room() {
    mGuestInfoList.clear();
    mLanPeer.stop_periodically_broadcast();
    mLanPeer.stop_listen_guest();
    mLanPeer.disconnect_all_guests();
    mGameStatus = GameStatus::Lobby;
}

void LocalStatus::create_room() {
    mLanPeer.start_periodically_broadcast();
    mLanPeer.start_listen_guest();
    mGameStatus = GameStatus::RoomAsHost;
}

void LocalStatus::join_room(const RoomEntry& room_entry) {
    std::println("trying to join {}", room_entry.name);
    if (!mLanPeer.connect_to_host(room_entry.ip)) {
        std::println("fail to connect to {}", room_entry.ip);
        return;
    }
    mLanPeer.start_heartbeat_to_host();
    mLanPeer.start_listen_host_packet();
    mGameStatus = GameStatus::RoomAsGuest;
}

void LocalStatus::guest_exit_room() {
    mLanPeer.stop_heartbeat_to_host();
    mLanPeer.stop_listen_host_packet();
    mLanPeer.disconnect_from_host();
    mGameStatus = GameStatus::Lobby;
}

void LocalStatus::start_discover_room() {
    mLanPeer.start_periodically_discover();
    mGameStatus = GameStatus::Lobby;
}

void LocalStatus::stop_discover_room() {
    mLanPeer.stop_periodically_discover();
}

const LocalStatus::GuestInfoList& LocalStatus::get_guest_info_list() {
    return mGuestInfoList;
}

// only host uses this method
void LocalStatus::start_game() {
    mLanPeer.send_game_starting_packet();
	mIsGameRunning = true;
    //    TODO();
}

bool LocalStatus::is_game_running() { return mIsGameRunning; }
