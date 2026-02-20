#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

class Level {
public:
	explicit Level(sf::Vector2u windowSize);

	const std::vector<sf::FloatRect>& getColliders() const;
	void draw(sf::RenderWindow& window) const;

private:
	std::vector<sf::RectangleShape> m_platforms;
	std::vector<sf::FloatRect> m_colliders;
};

