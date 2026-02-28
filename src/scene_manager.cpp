#include "scene_manager.hpp"

void SceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= mSelf->getTransform();
    for (const auto& child : mChildren) target.draw(*child.mSelf, states);
    target.draw(*mSelf, states);
}

bool SceneNode::handle_event(sf::RenderWindow& w, sf::Event e) {
    if (mSelf->handle_event(w, e)) return true;
    for (const auto& child : mChildren)
        if (child.mSelf->handle_event(w, e)) return true;
    return false;
}

void SceneNode::add_child(SceneNode&& child) {
    mChildren.push_back(std::move(child));
}

Scene::~Scene() {}

void Scene::register_widget(widget::Widget& wgt) {
    mSceneWidgets.push_back(wgt);
};

bool Scene::handle_event(sf::RenderWindow& w, sf::Event e) {
    bool ret = false;
    for (auto wgt : mSceneWidgets) ret |= wgt.get().handle_event(w, e);
    return ret;
}

void Scene::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const auto wgt : mSceneWidgets) target.draw(wgt, states);
}

SceneLobby::SceneLobby(LocalStatus& local_status)
    : Scene(local_status), mRoomListView(local_status.get_room_entry_list()) {
    mCreateRoomBtn.set_text("Create room");
    mCreateRoomBtn.on_click([&] {
        std::println("Lobby: clicked on Create room");
        mLocalStatus.create_room();
    });
    mRoomListView.on_item_click([&](const LocalStatus::RoomEntry& room_entry) {
        std::println("Lobby: clicked on a room");
        mLocalStatus.join_room(room_entry);
    });
    Scene::register_widget(mCreateRoomBtn);
    Scene::register_widget(mRoomListView);

    mLocalStatus.start_discover_room();
};

SceneLobby::~SceneLobby() { mLocalStatus.stop_discover_room(); }

bool SceneLobby::handle_event(sf::RenderWindow& w, sf::Event e) {
    sf::Vector2u window_size = w.getSize();
    float x = 0;
    float y = window_size.y - 200.f;
    mCreateRoomBtn.setPosition({x, y});
    mRoomListView.setPosition({100.f, 100.f});
    if (e.is<sf::Event::MouseButtonPressed>()) {
        mCreateRoomBtn.handle_event(w, e);
        mRoomListView.handle_event(w, e);
    }
    return true;
}

SceneRoomAsHost::SceneRoomAsHost(LocalStatus& local_status)
    : Scene(local_status),
      mPlayerListView(local_status.get_guest_info_list()),
      mGameRunningSplash("Game running...") {
    mExitRoomBtn.set_text("Exit room");
    mExitRoomBtn.on_click([&] {
        std::println("RoomAsHost: clicked on Exit room");
        mLocalStatus.host_exit_room();
    });
    mStartGameBtn.set_text("Start game");
    mStartGameBtn.on_click([&] {
        mGameRunningSplash.enable(true);
        mLocalStatus.start_game();
    });
    mGameRunningSplash.enable(false);

    Scene::register_widget(mExitRoomBtn);
    Scene::register_widget(mStartGameBtn);
    Scene::register_widget(mPlayerListView);
    Scene::register_widget(mGameRunningSplash);
}

SceneRoomAsHost::~SceneRoomAsHost() {}

bool SceneRoomAsHost::handle_event(sf::RenderWindow& w, sf::Event e) {
    sf::Vector2u window_size = w.getSize();
    float x = window_size.x - mExitRoomBtn.get_size().x;
    float y = window_size.y - 200.f;
    mExitRoomBtn.setPosition({x, y});
    mStartGameBtn.setPosition({0.f, y});
    mPlayerListView.setPosition({100.f, 100.f});

    if (e.is<sf::Event::FocusGained>() || e.is<sf::Event::Resized>() ||
        e.is<sf::Event::MouseButtonPressed>()) {
        if (mGameRunningSplash.handle_event(w, e)) return true;
    }

    if (e.is<sf::Event::MouseButtonPressed>()) {
        mExitRoomBtn.handle_event(w, e);
        mStartGameBtn.handle_event(w, e);
        mPlayerListView.handle_event(w, e);
    }
    return true;
}

void SceneRoomAsHost::update(sf::RenderWindow& w) {
    bool is_game_running = mLocalStatus.is_game_running();
    static bool last_is_game_running = false;
    if (last_is_game_running != is_game_running) {
        mGameRunningSplash.handle_event(w, sf::Event::FocusGained());
    }
    last_is_game_running = is_game_running;
}

SceneRoomAsGuest::SceneRoomAsGuest(LocalStatus& local_status)
    : Scene(local_status),
      mPlayerListView(local_status.get_guest_info_list()),
      mGameRunningSplash("Game running..") {
    mExitRoomBtn.set_text("Exit room");
    mExitRoomBtn.on_click([&] {
        std::println("RoomAsGuest: clicked on Exit room");
        mLocalStatus.guest_exit_room();
    });
    mGameRunningSplash.enable(false);

    Scene::register_widget(mExitRoomBtn);
    Scene::register_widget(mPlayerListView);
    Scene::register_widget(mGameRunningSplash);
}

bool SceneRoomAsGuest::handle_event(sf::RenderWindow& w, sf::Event e) {
    sf::Vector2u window_size = w.getSize();
    float x = window_size.x - mExitRoomBtn.get_size().x;
    float y = window_size.y - 200.f;
    mExitRoomBtn.setPosition({x, y});
    mPlayerListView.setPosition({100.f, 100.f});

    if (e.is<sf::Event::FocusGained>() || e.is<sf::Event::Resized>() ||
        e.is<sf::Event::MouseButtonPressed>())
        if (mGameRunningSplash.handle_event(w, e)) return true;

    if (e.is<sf::Event::MouseButtonPressed>()) mExitRoomBtn.handle_event(w, e);

    return true;
}

SceneRoomAsGuest::~SceneRoomAsGuest() {};

void SceneRoomAsGuest::update(sf::RenderWindow& w) {
    bool is_game_running = mLocalStatus.is_game_running();
    static bool last_is_game_running = false;
    if (last_is_game_running != is_game_running) {
        mGameRunningSplash.enable(is_game_running);
        mGameRunningSplash.handle_event(w, sf::Event::FocusGained());
    }
    last_is_game_running = is_game_running;
}

SceneManager::SceneManager(LocalStatus& local_status)
    : mScene(std::make_unique<SceneLobby>(local_status)),
      mLocalStatus(local_status),
      mWindow(sf::VideoMode({1920, 1080}), "LAN Client") {
    // to run handle event on startup once, so widgets get the chance to
    // initialize their position
    mScene->handle_event(mWindow, sf::Event::FocusGained());
}

bool SceneManager::is_window_open() { return mWindow.isOpen(); }

void SceneManager::handle_window_event() {
    static const auto on_mouse_button_pressed =
        [&](const sf::Event::MouseButtonPressed& mouse_button_pressed) {
            mScene->handle_event(mWindow, mouse_button_pressed);
        };
    static const auto on_key_pressed =
        [&](const sf::Event::KeyPressed& key_pressed) {
            if (key_pressed.scancode == sf::Keyboard::Scancode::Q) {
                mWindow.close();
            }
        };

    mWindow.handleEvents([&](const sf::Event::Closed) { mWindow.close(); },
                         on_key_pressed, on_mouse_button_pressed);
}

void SceneManager::display() {
    update_scene_type();
    mWindow.clear(sf::Color(55, 55, 55, 0));
    mScene->update(mWindow);
    mWindow.draw(*mScene);
    mWindow.display();
}

void SceneManager::update_scene_type() {
    static auto last_scene_type = mLocalStatus.game_status();
    auto current_scene_type = mLocalStatus.game_status();
    if (last_scene_type != current_scene_type) {
        switch (current_scene_type) {
            case GameStatus::Lobby:
                std::println("to Lobby");
                mScene = std::make_unique<SceneLobby>(mLocalStatus);
                break;
            case GameStatus::RoomAsHost:
                std::println("to RoomAsHost");
                mScene = std::make_unique<SceneRoomAsHost>(mLocalStatus);
                break;
            case GameStatus::RoomAsGuest:
                std::println("to RoomAsGuest");
                mScene = std::make_unique<SceneRoomAsGuest>(mLocalStatus);
                break;
            default:
                UNREACHABLE();
        }

        last_scene_type = current_scene_type;
        // to run handle event on startup once, so widgets get the chance to
        // initialize their position
        mScene->handle_event(mWindow, sf::Event::FocusGained());
    }
}
