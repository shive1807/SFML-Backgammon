#include "Game.hpp"

Game::Game(sf::Vector2u windowSize)
	: m_level(windowSize)
	, m_player({64.f, 400.f}, {40.f, 56.f}) {
}

void Game::update(float dtSeconds) {
	m_player.update(dtSeconds, m_level.getColliders());
}

void Game::render(sf::RenderWindow& window) const {
	m_level.draw(window);
	m_player.draw(window);
}

