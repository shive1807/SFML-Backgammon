#include "Player.hpp"

namespace {
constexpr float kMoveSpeed = 220.f;
constexpr float kJumpSpeed = 420.f;
constexpr float kGravity = 1200.f;
} // namespace

Player::Player(const sf::Vector2f& position, const sf::Vector2f& size)
	: m_position(position)
	, m_size(size)
	, m_velocity(0.f, 0.f)
	, m_onGround(false)
	, m_body(size) {
	m_body.setPosition(position);
	m_body.setFillColor(sf::Color(100, 220, 180));
	m_body.setOutlineThickness(2.f);
	m_body.setOutlineColor(sf::Color::White);
}

void Player::update(float dtSeconds, const std::vector<sf::FloatRect>& colliders) {
	handleInput();
	applyPhysics(dtSeconds);
	moveAndCollide(dtSeconds, colliders);
	m_body.setPosition(m_position);
}

void Player::draw(sf::RenderWindow& window) const {
	window.draw(m_body);
}

void Player::handleInput() {
	float direction = 0.f;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
		direction -= 1.f;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
		direction += 1.f;
	}
	m_velocity.x = direction * kMoveSpeed;

	if (m_onGround &&
		(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
		 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))) {
		m_velocity.y = -kJumpSpeed;
		m_onGround = false;
	}
}

void Player::applyPhysics(float dtSeconds) {
	m_velocity.y += kGravity * dtSeconds;
}

void Player::moveAndCollide(float dtSeconds, const std::vector<sf::FloatRect>& colliders) {
	m_position.x += m_velocity.x * dtSeconds;
	for (const auto& collider : colliders) {
		if (!isColliding(collider)) {
			continue;
		}
		if (m_velocity.x > 0.f) {
			m_position.x = collider.position.x - m_size.x;
		} else if (m_velocity.x < 0.f) {
			m_position.x = collider.position.x + collider.size.x;
		}
		m_velocity.x = 0.f;
	}

	m_position.y += m_velocity.y * dtSeconds;
	m_onGround = false;
	for (const auto& collider : colliders) {
		if (!isColliding(collider)) {
			continue;
		}
		if (m_velocity.y > 0.f) {
			m_position.y = collider.position.y - m_size.y;
			m_onGround = true;
		} else if (m_velocity.y < 0.f) {
			m_position.y = collider.position.y + collider.size.y;
		}
		m_velocity.y = 0.f;
	}
}

bool Player::isColliding(const sf::FloatRect& other) const {
	return getBounds().findIntersection(other).has_value();
}

sf::FloatRect Player::getBounds() const {
	return sf::FloatRect(m_position, m_size);
}

