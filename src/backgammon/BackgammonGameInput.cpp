#include "BackgammonGame.hpp"

void BackgammonGame::handleEvent(const sf::Event& event) {
	if (m_gameOver || m_animating) {
		return;
	}

	if (auto key = event.getIf<sf::Event::KeyPressed>()) {
		if (key->code == sf::Keyboard::Key::R && m_moves.empty() && currentPlayer() == m_humanPlayer) {
			rollDice();
			checkAutoPass();
		}
		if (key->code == sf::Keyboard::Key::Escape) {
			m_selectedPoint.reset();
		}
		return;
	}

	if (currentPlayer() != m_humanPlayer) {
		return;
	}

	auto handleTap = [&](sf::Vector2f position) {
		if (m_moves.empty()) {
			if (isOnDice(position)) {
				rollDice();
				checkAutoPass();
			}
			return;
		}

		if (isOnOffArea(position, currentPlayer()) && m_selectedPoint.has_value()) {
			tryBearOff();
			return;
		}

		if (auto point = pointFromMouse(position)) {
			if (barCount(currentPlayer()) > 0) {
				tryMoveFromBar(*point);
				return;
			}

			if (!m_selectedPoint.has_value()) {
				trySelectPoint(*point);
				return;
			}

			if (*m_selectedPoint == *point) {
				m_selectedPoint.reset();
				return;
			}

			const bool moved = tryMoveToPoint(*point);
			if (!moved) {
				trySelectPoint(*point);
			}
		} else {
			m_selectedPoint.reset();
		}
	};

	if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>()) {
		if (mouse->button != sf::Mouse::Button::Left) {
			return;
		}
		const sf::Vector2f position(static_cast<float>(mouse->position.x),
			static_cast<float>(mouse->position.y));
		handleTap(position);
	}

	if (auto touch = event.getIf<sf::Event::TouchBegan>()) {
		if (touch->finger != 0) {
			return;
		}
		const sf::Vector2f position(static_cast<float>(touch->position.x),
			static_cast<float>(touch->position.y));
		handleTap(position);
	}
}

void BackgammonGame::trySelectPoint(int pointIndex) {
	const int count = m_points[pointIndex];
	if (count == 0) {
		return;
	}

	if ((count > 0 && currentPlayer() == Player::White) ||
		(count < 0 && currentPlayer() == Player::Black)) {
		m_selectedPoint = pointIndex;
	}
}

bool BackgammonGame::tryMoveToPoint(int pointIndex) {
	if (!m_selectedPoint.has_value()) {
		return false;
	}
	return tryMoveChecker(*m_selectedPoint, pointIndex);
}

void BackgammonGame::tryBearOff() {
	if (!m_selectedPoint.has_value()) {
		return;
	}

	if (!canBearOff(currentPlayer())) {
		return;
	}

	const int fromIndex = *m_selectedPoint;
	int distance = 0;
	if (currentPlayer() == Player::White) {
		distance = fromIndex + 1;
	} else {
		distance = 24 - fromIndex;
	}

	const bool allowLarger = true;
	const int dieIndex = findUsableDie(distance, allowLarger, fromIndex);
	if (dieIndex < 0) {
		return;
	}

	MoveOption move{fromIndex, -1, false, true, dieIndex, m_moves[dieIndex]};
	applyMove(move);
}

bool BackgammonGame::tryMoveFromBar(int pointIndex) {
	if (barCount(currentPlayer()) == 0) {
		return false;
	}

	const int distance = (currentPlayer() == Player::White) ? 24 - pointIndex : pointIndex + 1;
	const int dieIndex = findUsableDie(distance, false, -1);
	if (dieIndex < 0) {
		return false;
	}

	if (!isLegalDestination(currentPlayer(), pointIndex)) {
		return false;
	}

	MoveOption move{-1, pointIndex, true, false, dieIndex, m_moves[dieIndex]};
	return applyMove(move);
}

bool BackgammonGame::tryMoveChecker(int fromIndex, std::optional<int> toIndex) {
	if (!toIndex.has_value()) {
		return false;
	}

	const int distance = (currentPlayer() == Player::White)
		? fromIndex - *toIndex
		: *toIndex - fromIndex;

	if (distance <= 0) {
		return false;
	}

	const int dieIndex = findUsableDie(distance, false, fromIndex);
	if (dieIndex < 0) {
		return false;
	}

	if (!isLegalDestination(currentPlayer(), *toIndex)) {
		return false;
	}

	MoveOption move{fromIndex, *toIndex, false, false, dieIndex, m_moves[dieIndex]};
	return applyMove(move);
}
