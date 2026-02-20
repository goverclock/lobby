#pragma once
#include <memory>
#include <vector>

#include "mscore/local_status.hpp"
#include "widget/button.hpp"
#include "widget/list_view.hpp"
#include "widget/splash_screen.hpp"

enum class SceneType {
    Lobby,
    RoomAsGuest,
    RoomAsHost,
};

// essentially a warpper for widget, to make it a tree hierarchy
class SceneNode : public sf::Drawable, public widget::Responsive {
   public:
    SceneNode() = delete;
    // SceneNode(const SceneNode&) = delete;
    template <typename T>
    explicit SceneNode(T self) : mSelf(std::make_unique<T>(self)){};

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;

    void add_child(SceneNode&& child);

   private:
    std::unique_ptr<widget::Widget> mSelf;
    std::vector<SceneNode> mChildren;
};

class Scene : public sf::Drawable {
   public:
    virtual ~Scene();
    void register_widget(widget::Widget& wgt);
    virtual bool handle_event(sf::RenderWindow& w, sf::Event e);
    virtual void draw(sf::RenderTarget& target,
                      sf::RenderStates states) const override;

    Scene(LocalStatus& local_status) : mLocalStatus(local_status) {};
    LocalStatus& mLocalStatus;

   private:
    std::vector<std::reference_wrapper<widget::Widget>> mSceneWidgets;
};

class SceneLobby : public Scene {
   public:
    SceneLobby(LocalStatus& local_status);
    ~SceneLobby();
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;

   private:
    widget::Button mCreateRoomBtn;
    widget::ListView<LocalStatus::RoomEntry> mRoomListView;
};

class SceneRoomAsHost : public Scene {
   public:
    SceneRoomAsHost(LocalStatus& local_status);
    ~SceneRoomAsHost();
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;

   private:
    widget::Button mExitRoomBtn;
    widget::Button mStartGameBtn;
    widget::ListView<LocalStatus::GuestInfo> mPlayerListView;
	widget::SplashScreen mGameRunningSplash;
};

class SceneRoomAsGuest : public Scene {
   public:
    SceneRoomAsGuest(LocalStatus& local_status);
    ~SceneRoomAsGuest();
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;

   private:
    widget::Button mExitRoomBtn;
    widget::ListView<LocalStatus::GuestInfo> mPlayerListView;
};

class SceneManager {
   public:
    SceneManager(LocalStatus& local_status);

    bool is_window_open();
    void handle_window_event();
    void display();

   private:
    void update_scene_type();
    std::unique_ptr<Scene> mScene;
    sf::RenderWindow mWindow;
    LocalStatus& mLocalStatus;
};
