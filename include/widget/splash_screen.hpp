#pragma once

#include "widget/widget.hpp"

namespace widget {

class SplashScreen : public Widget {
   public:
    SplashScreen(std::string text = "splash screen text");
    SplashScreen& set_text(std::string text);

    // Responsive
    bool handle_event(sf::RenderWindow& w, sf::Event e) override;
    // Drawable
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   private:
    sf::Text mText;
    sf::RectangleShape mTextRect;
    sf::RectangleShape mDimLayer;
};

}  // namespace widget
