#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

#include "../backgammon/BackgammonGame.hpp"

class Engine {
public:
	Engine();
	void run();

private:
	void processEvents();
	void update(float dtSeconds);
	void render();

	sf::RenderWindow m_window;
	std::unique_ptr<BackgammonGame> m_game;
};

