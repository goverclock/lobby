#include "ergonomics.hpp"
#include "widget/splash_screen.hpp"

namespace widget {

SplashScreen::SplashScreen(std::string text) : mText(defaultWidgetFont, text) {
    mTextRect.setFillColor(sf::Color::Black);
    mTextRect.setSize({380.f, 100.f});
    mText.setFillColor(sf::Color::White);
    mText.setCharacterSize(30);

    mDimLayer.setFillColor(sf::Color(55, 55, 55, 200));
}

// note: remember to emit a resize event after calling this function
SplashScreen& SplashScreen::set_text(std::string text) {
    mText.setString(text);
    return *this;
}

SplashScreen& SplashScreen::enable(bool en) {
    mIsEnable = en;
    return *this;
}

// note: if we want to fix the text in the center, use this method to
// handle window resize event
bool SplashScreen::handle_event(sf::RenderWindow& w, sf::Event e) {
    if (!mIsEnable) return false;

    sf::Vector2f text_size = mText.getLocalBounds().size;
    mTextRect.setSize({text_size.x + 30.f, text_size.y + 30.f});

    sf::Vector2u window_size = w.getSize();
    mDimLayer.setSize({(float)window_size.x, (float)window_size.y});

    sf::Vector2f rect_size = mTextRect.getSize();

    sf::Vector2f text_center_pos =
        sf::Vector2f(window_size.x / 2 - text_size.x / 2,
                     window_size.y / 2 - text_size.y / 2);
    sf::Vector2f rect_center_pos =
        sf::Vector2f(window_size.x / 2 - rect_size.x / 2,
                     window_size.y / 2 - rect_size.y / 2);

    mText.setPosition({text_center_pos.x, text_center_pos.y});
    mTextRect.setPosition({rect_center_pos.x, rect_center_pos.y});

    return true;
};

// Drawable
void SplashScreen::draw(sf::RenderTarget& target,
                        sf::RenderStates states) const {
    if (!mIsEnable) return;
    states.transform *= getTransform();
    target.draw(mDimLayer, states);
    target.draw(mTextRect, states);
    target.draw(mText, states);
}

};  // namespace widget
