#include "Level.hpp"

namespace {
sf::RectangleShape makePlatform(const sf::Vector2f& position, const sf::Vector2f& size) {
	sf::RectangleShape shape(size);
	shape.setPosition(position);
	shape.setFillColor(sf::Color(90, 90, 110));
	shape.setOutlineThickness(2.f);
	shape.setOutlineColor(sf::Color(140, 140, 160));
	return shape;
}
} // namespace

Level::Level(sf::Vector2u windowSize) {
	const float width = static_cast<float>(windowSize.x);
	const float height = static_cast<float>(windowSize.y);

	m_platforms.reserve(5);
	m_platforms.push_back(makePlatform({0.f, height - 64.f}, {width, 64.f}));
	m_platforms.push_back(makePlatform({120.f, height - 160.f}, {180.f, 24.f}));
	m_platforms.push_back(makePlatform({380.f, height - 230.f}, {200.f, 24.f}));
	m_platforms.push_back(makePlatform({620.f, height - 140.f}, {140.f, 24.f}));
	m_platforms.push_back(makePlatform({40.f, height - 280.f}, {140.f, 24.f}));

	m_colliders.reserve(m_platforms.size());
	for (const auto& platform : m_platforms) {
		m_colliders.push_back(platform.getGlobalBounds());
	}
}

const std::vector<sf::FloatRect>& Level::getColliders() const {
	return m_colliders;
}

void Level::draw(sf::RenderWindow& window) const {
	for (const auto& platform : m_platforms) {
		window.draw(platform);
	}
}

