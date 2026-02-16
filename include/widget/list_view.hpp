#pragma once
#include <concepts>
#include <functional>
#include <vector>

#include "widget/widget.hpp"

namespace widget {

template <typename T>
concept HasToString = requires(const T& obj) {
    { obj.to_string() } -> std::same_as<std::string>;
};
template <HasToString T>
class ListView : public Widget {
   public:
    ListView(const std::vector<T>& item_list);

    void on_item_click(std::function<void(const T&)>);

    // Widget
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;
    // Drawable
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   private:
    const std::vector<T>& mItemList;
    sf::RectangleShape mEntryRect;
    sf::Text mEntryText;
    std::function<void(const T&)> mClickItemCallbackFunc = nullptr;
};

};  // namespace widget

#include "../../src/widget/list_view.tpp"  // FUCK C++
