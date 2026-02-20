#pragma once

#include <SFML/Graphics.hpp>

#include "Level.hpp"
#include "Player.hpp"

class Game {
public:
	explicit Game(sf::Vector2u windowSize);

	void update(float dtSeconds);
	void render(sf::RenderWindow& window) const;

private:
	Level m_level;
	Player m_player;
};

