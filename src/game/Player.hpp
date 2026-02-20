#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

class Player {
public:
	Player(const sf::Vector2f& position, const sf::Vector2f& size);

	void update(float dtSeconds, const std::vector<sf::FloatRect>& colliders);
	void draw(sf::RenderWindow& window) const;

private:
	void handleInput();
	void applyPhysics(float dtSeconds);
	void moveAndCollide(float dtSeconds, const std::vector<sf::FloatRect>& colliders);
	bool isColliding(const sf::FloatRect& other) const;

	sf::FloatRect getBounds() const;

	sf::Vector2f m_position;
	sf::Vector2f m_size;
	sf::Vector2f m_velocity;
	bool m_onGround;

	sf::RectangleShape m_body;
};

