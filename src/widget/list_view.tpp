#include "ergonomics.hpp"
#include "widget/list_view.hpp"

namespace widget {

template <HasToString T>
ListView<T>::ListView(const std::vector<T>& item_list)
    : mItemList(item_list), mEntryText(defaultWidgetFont) {
    mEntryRect.setSize({400.f, 80.f});
    mEntryRect.setFillColor(sf::Color::White);
    mEntryText.setFillColor(sf::Color::Black);
    mEntryText.setCharacterSize(50);
}

template <HasToString T>
void ListView<T>::on_item_click(std::function<void(const T&)> callback_func) {
    mClickItemCallbackFunc = callback_func;
}

template <HasToString T>
bool ListView<T>::handle_event(sf::RenderWindow& w, sf::Event e) {
    const sf::Event::MouseButtonPressed* mouse_button_pressed =
        e.getIf<sf::Event::MouseButtonPressed>();
    assert(mouse_button_pressed);
    const sf::Vector2i global_mouse_pos = sf::Mouse::getPosition(w);
    // when we say local we mean relative to this ListView class's transform
    const sf::Vector2f local_mouse_pos = getInverseTransform().transformPoint(
        {(float)global_mouse_pos.x, (float)global_mouse_pos.y});

    // TODO: could do better by simply computing coords rather than creating
    // rects
    sf::RectangleShape temp_entry_rect = mEntryRect;
    float y = 0.f;
    size_t index = 0;
    for (const T& e : mItemList) {
        temp_entry_rect.setPosition({0, y});
        if (temp_entry_rect.getLocalBounds().contains(
                {(float)local_mouse_pos.x, (float)local_mouse_pos.y})) {
            if (mClickItemCallbackFunc) mClickItemCallbackFunc(e);
            return true;
        }

        y += temp_entry_rect.getSize().y;
        index++;
    }
    return false;
}

template <HasToString T>
void ListView<T>::draw(sf::RenderTarget& target,
                       sf::RenderStates states) const {
    states.transform *= getTransform();

    sf::RectangleShape temp_entry_rect = mEntryRect;
    sf::Text temp_entry_text = mEntryText;
    float y = 0.f;
    for (const T& e : mItemList) {
        temp_entry_rect.setPosition({0, y});
        temp_entry_text.setPosition({0, y});
        temp_entry_text.setString(e.to_string());
        target.draw(temp_entry_rect, states);
        target.draw(temp_entry_text, states);

        y += temp_entry_rect.getSize().y;
    }
}

};  // namespace widget
