#include "Engine.hpp"

#include "../backgammon/BackgammonGame.hpp"

namespace {
constexpr unsigned int kWindowWidth = 1100;
constexpr unsigned int kWindowHeight = 700;
} // namespace

Engine::Engine()
	: m_window(sf::VideoMode({kWindowWidth, kWindowHeight}), "SFML Backgammon")
	, m_state(State::Menu)
	, m_game(nullptr)
	, m_hasFont(false) {
	m_window.setFramerateLimit(60);
	ensureFontLoaded();
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
		if (m_state == State::Menu) {
			handleMenuEvent(*event);
		} else if (m_game) {
			m_game->handleEvent(*event);
		}
	}
}

void Engine::update(float dtSeconds) {
	if (m_state == State::Playing && m_game) {
		m_game->update(dtSeconds);
	}
}

void Engine::render() {
	m_window.clear(sf::Color(30, 30, 40));
	if (m_state == State::Menu) {
		renderMenu();
	} else if (m_game) {
		m_game->render(m_window);
	}
	m_window.display();
}

void Engine::renderMenu() {
	sf::RectangleShape backdrop({static_cast<float>(m_window.getSize().x),
		static_cast<float>(m_window.getSize().y)});
	backdrop.setFillColor(sf::Color(20, 20, 30));
	m_window.draw(backdrop);

	if (!m_hasFont) {
		return;
	}

	sf::Text title(m_font);
	sf::Text start(m_font);
	sf::Text quit(m_font);
	buildMenuText(title, start, quit);

	m_window.draw(title);
	m_window.draw(start);
	m_window.draw(quit);
}

void Engine::handleMenuEvent(const sf::Event& event) {
	if (auto key = event.getIf<sf::Event::KeyPressed>()) {
		if (key->code == sf::Keyboard::Key::Enter) {
			startGame();
		} else if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Q) {
			m_window.close();
		}
		return;
	}

	if (!m_hasFont) {
		return;
	}

	if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>()) {
		if (mouse->button != sf::Mouse::Button::Left) {
			return;
		}
		const sf::Vector2f position(static_cast<float>(mouse->position.x),
			static_cast<float>(mouse->position.y));

		sf::Text title(m_font);
		sf::Text start(m_font);
		sf::Text quit(m_font);
		buildMenuText(title, start, quit);

		if (start.getGlobalBounds().contains(position)) {
			startGame();
		} else if (quit.getGlobalBounds().contains(position)) {
			m_window.close();
		}
	}
}

void Engine::startGame() {
	m_game = std::make_unique<BackgammonGame>(m_window.getSize());
	m_state = State::Playing;
}

void Engine::ensureFontLoaded() {
	if (m_hasFont) {
		return;
	}
	const char* candidates[] = {
		"/System/Library/Fonts/SFNS.ttf",
		"/System/Library/Fonts/Supplemental/Arial.ttf",
		"/Library/Fonts/Arial.ttf"
	};
	for (const char* path : candidates) {
		if (m_font.openFromFile(path)) {
			m_hasFont = true;
			break;
		}
	}
}

void Engine::buildMenuText(sf::Text& title, sf::Text& start, sf::Text& quit) const {
	const float centerX = m_window.getSize().x * 0.5f;
	const float centerY = m_window.getSize().y * 0.5f;

	title.setString("Backgammon");
	title.setCharacterSize(48);
	title.setFillColor(sf::Color(240, 230, 200));

	start.setString("Start");
	start.setCharacterSize(28);
	start.setFillColor(sf::Color(220, 220, 220));

	quit.setString("Quit");
	quit.setCharacterSize(28);
	quit.setFillColor(sf::Color(220, 220, 220));

	auto centerText = [&](sf::Text& text, float y) {
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({bounds.position.x + bounds.size.x * 0.5f,
			bounds.position.y + bounds.size.y * 0.5f});
		text.setPosition({centerX, y});
	};

	centerText(title, centerY - 120.f);
	centerText(start, centerY - 10.f);
	centerText(quit, centerY + 50.f);
}
