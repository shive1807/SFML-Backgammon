#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

#include "../backgammon/BackgammonGame.hpp"

class Engine {
public:
	Engine();
	void run();

private:
	enum class State { Menu, Playing };

	void processEvents();
	void update(float dtSeconds);
	void render();
	void renderMenu();
	void handleMenuEvent(const sf::Event& event);
	void startGame();
	void ensureFontLoaded();
	void buildMenuText(sf::Text& title, sf::Text& start, sf::Text& quit) const;

	sf::RenderWindow m_window;
	State m_state;
	std::unique_ptr<BackgammonGame> m_game;
	sf::Font m_font;
	bool m_hasFont;
};

