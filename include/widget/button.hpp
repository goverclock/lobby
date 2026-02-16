#pragma once
#include <functional>

#include "widget/widget.hpp"

namespace widget {

class Button : public Widget {
   public:
    Button(float width = 100, float height = 100,
           std::string text = "unnamed button");

    void on_click(const std::function<void(void)>& callback_func);
    void set_size(sf::Vector2f size);
    sf::Vector2f get_size() const;
    void set_fill_color(sf::Color color);
    void set_text(std::string text);

    // Widget
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;
    // Drawable
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   private:
    std::function<void(void)> mClickCallbackFunc = nullptr;
    sf::RectangleShape mRectangle;
    sf::Text mText;
};

};  // namespace widget
