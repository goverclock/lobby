#pragma once

#include <string>

#include "lan/lan.hpp"

enum class GameStatus {
    Lobby,
    RoomAsHost,
    RoomAsGuest,
    NotStarted,  // aka title screen
    Running,
};

enum class GameType {
    MineSweeper,
    Geowartry,
};

class LocalStatus {
   public:
    LocalStatus();

    const GameStatus& game_status();
    void update();

    using RoomEntry = struct RoomEntry {
        std::string name = "";
        std::string ip;
        int signal_strength;
        int player_count;
        GameType game_type;
        std::string to_string() const {
            return name + " " + std::string(signal_strength, '|');
        }
    };
    using RoomEntryList = std::vector<RoomEntry>;

    // network, in lobby
    const RoomEntryList& get_room_entry_list();
    void create_room();
    void join_room(const RoomEntry& room_entry);
    void start_discover_room();
    void stop_discover_room();

    // network, in room
    void host_exit_room();   // host clicking exit room button calls this
    void guest_exit_room();  // guest clicking exit room button calls this
    using GuestInfo = struct GuestInfo {
        std::string nickname;
        std::string ip;
        lan::SignalStrength signal_strength;
        std::string to_string() const {
            return nickname + " " + ip + " " +
                   std::string(static_cast<int>(signal_strength) + 1, '|');
        }
    };
    using GuestInfoList = std::vector<GuestInfo>;
    const GuestInfoList& get_guest_info_list();

    void start_game();

   private:
    // network
    lan::LanPeer mLanPeer;
    // different lan role handles same lan message updates differently
    RoomEntryList mRoomEntryList;
    GuestInfoList mGuestInfoList;
    GameStatus mGameStatus;
};
