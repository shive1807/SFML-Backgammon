#include "Engine.hpp"

#include "../backgammon/BackgammonGame.hpp"

namespace {
constexpr unsigned int kWindowWidth = 1100;
constexpr unsigned int kWindowHeight = 700;
} // namespace

Engine::Engine()
	: m_window(sf::VideoMode({kWindowWidth, kWindowHeight}), "SFML Backgammon")
	, m_game(nullptr) {
	m_window.setFramerateLimit(60);
	m_game = std::make_unique<BackgammonGame>(m_window.getSize());
}

void Engine::run() {
	sf::Clock clock;
	while (m_window.isOpen()) {
		processEvents();

		const float dtSeconds = clock.restart().asSeconds();
		update(dtSeconds);
		render();
	}
}

void Engine::processEvents() {
	while (auto event = m_window.pollEvent()) {
		if (event->is<sf::Event::Closed>()) {
			m_window.close();
		}
		m_game->handleEvent(*event);
	}
}

void Engine::update(float dtSeconds) {
	m_game->update(dtSeconds);
}

void Engine::render() {
	m_window.clear(sf::Color(30, 30, 40));
	m_game->render(m_window);
	m_window.display();
}

