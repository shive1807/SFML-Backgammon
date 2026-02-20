#include "BackgammonGame.hpp"
#include "BackgammonGameInternal.hpp"

#include <algorithm>
#include <string>

using namespace backgammon_internal;

void BackgammonGame::render(sf::RenderWindow& window) const {
	drawBoard(window);
	drawOffArea(window);
	drawCheckers(window);
	drawAnimatedChecker(window);
	drawDice(window);
	drawSelection(window);
	drawHighlights(window);
	drawHud(window);
}

void BackgammonGame::drawBoard(sf::RenderWindow& window) const {
	sf::RectangleShape board(m_boardRect.size);
	board.setPosition(m_boardRect.position);
	board.setFillColor(sf::Color(110, 82, 56));
	board.setOutlineThickness(4.f);
	board.setOutlineColor(sf::Color(60, 40, 20));
	window.draw(board);

	sf::RectangleShape bar(m_barRect.size);
	bar.setPosition(m_barRect.position);
	bar.setFillColor(sf::Color(80, 60, 40));
	window.draw(bar);

	for (int i = 0; i < 24; ++i) {
		const bool isTop = m_pointSlots[i].isTop;
		const int pointInRow = isTop ? i - 12 : 11 - i;
		const bool isLeftHalf = pointInRow < 6;
		const int column = isLeftHalf ? pointInRow : pointInRow - 6;
		const float x = (isLeftHalf ? m_boardRect.position.x : m_barRect.position.x + m_barRect.size.x) +
			column * m_pointWidth;
		const float boardTop = m_boardRect.position.y;
		const float boardBottom = m_boardRect.position.y + m_boardRect.size.y;
		const float topBaseY = boardTop + m_pointHeight;
		const float bottomBaseY = boardBottom - m_pointHeight;

		sf::ConvexShape triangle(3);
		triangle.setPoint(0, sf::Vector2f(x, isTop ? boardTop : boardBottom));
		triangle.setPoint(1, sf::Vector2f(x + m_pointWidth, isTop ? boardTop : boardBottom));
		triangle.setPoint(2, sf::Vector2f(x + m_pointWidth / 2.f, isTop ? topBaseY : bottomBaseY));
		const bool dark = (i % 2 == 0);
		triangle.setFillColor(dark ? sf::Color(170, 110, 70) : sf::Color(220, 170, 120));
		window.draw(triangle);
	}
}

void BackgammonGame::drawCheckers(sf::RenderWindow& window) const {
	auto pointCountAdjusted = [&](int index) {
		int count = m_points[index];
		if (!m_animating) {
			return count;
		}

		if (!m_animBearOff && index == m_animToIndex) {
			count -= (m_animPlayer == Player::White) ? 1 : -1;
		}
		return count;
	};

	for (int i = 0; i < 24; ++i) {
		const int count = std::abs(pointCountAdjusted(i));
		if (count == 0) {
			continue;
		}
		const Player owner = (pointCountAdjusted(i) > 0) ? Player::White : Player::Black;
		const bool isTop = m_pointSlots[i].isTop;
		sf::Vector2f base = m_pointSlots[i].center;
		const float maxStackHeight = std::max(0.f, m_pointHeight - m_checkerRadius * 2.f);
		float spacing = m_checkerRadius * 2.f - 2.f;
		if (count > 1 && maxStackHeight > 0.f) {
			spacing = std::min(spacing, maxStackHeight / static_cast<float>(count - 1));
		}
		for (int n = 0; n < count; ++n) {
			sf::CircleShape checker(m_checkerRadius);
			checker.setOrigin({m_checkerRadius, m_checkerRadius});
			checker.setFillColor(colorForPlayer(owner));
			checker.setOutlineThickness(2.f);
			checker.setOutlineColor(outlineForPlayer(owner));
			const float offset = static_cast<float>(n) * spacing;
			checker.setPosition({base.x, base.y + (isTop ? offset : -offset)});
			window.draw(checker);
		}
	}

	if (m_barWhite > 0) {
		int barCount = m_barWhite;
		const float maxStackHeight = std::max(0.f, m_barRect.size.y - m_checkerRadius * 2.f);
		float spacing = m_checkerRadius * 2.f - 2.f;
		if (barCount > 1 && maxStackHeight > 0.f) {
			spacing = std::min(spacing, maxStackHeight / static_cast<float>(barCount - 1));
		}
		for (int i = 0; i < barCount; ++i) {
			sf::CircleShape checker(m_checkerRadius);
			checker.setOrigin({m_checkerRadius, m_checkerRadius});
			checker.setFillColor(colorForPlayer(Player::White));
			checker.setOutlineThickness(2.f);
			checker.setOutlineColor(outlineForPlayer(Player::White));
			checker.setPosition({m_barRect.position.x + m_barRect.size.x / 2.f,
				m_barRect.position.y + m_barRect.size.y - (m_checkerRadius + i * spacing)});
			window.draw(checker);
		}
	}

	if (m_barBlack > 0) {
		int barCount = m_barBlack;
		const float maxStackHeight = std::max(0.f, m_barRect.size.y - m_checkerRadius * 2.f);
		float spacing = m_checkerRadius * 2.f - 2.f;
		if (barCount > 1 && maxStackHeight > 0.f) {
			spacing = std::min(spacing, maxStackHeight / static_cast<float>(barCount - 1));
		}
		for (int i = 0; i < barCount; ++i) {
			sf::CircleShape checker(m_checkerRadius);
			checker.setOrigin({m_checkerRadius, m_checkerRadius});
			checker.setFillColor(colorForPlayer(Player::Black));
			checker.setOutlineThickness(2.f);
			checker.setOutlineColor(outlineForPlayer(Player::Black));
			checker.setPosition({m_barRect.position.x + m_barRect.size.x / 2.f,
				m_barRect.position.y + (m_checkerRadius + i * spacing)});
			window.draw(checker);
		}
	}

	int offWhite = m_offWhite;
	if (m_animating && m_animPlayer == Player::White && m_animBearOff) {
		offWhite = std::max(0, offWhite - 1);
	}
	for (int i = 0; i < offWhite; ++i) {
		sf::CircleShape checker(m_checkerRadius * 0.85f);
		checker.setOrigin({m_checkerRadius * 0.85f, m_checkerRadius * 0.85f});
		checker.setFillColor(colorForPlayer(Player::White));
		checker.setOutlineThickness(1.5f);
		checker.setOutlineColor(outlineForPlayer(Player::White));
		checker.setPosition({m_offRectWhite.position.x + m_offRectWhite.size.x / 2.f,
			m_offRectWhite.position.y + m_offRectWhite.size.y - (m_checkerRadius + i * (m_checkerRadius * 1.6f))});
		window.draw(checker);
	}

	int offBlack = m_offBlack;
	if (m_animating && m_animPlayer == Player::Black && m_animBearOff) {
		offBlack = std::max(0, offBlack - 1);
	}
	for (int i = 0; i < offBlack; ++i) {
		sf::CircleShape checker(m_checkerRadius * 0.85f);
		checker.setOrigin({m_checkerRadius * 0.85f, m_checkerRadius * 0.85f});
		checker.setFillColor(colorForPlayer(Player::Black));
		checker.setOutlineThickness(1.5f);
		checker.setOutlineColor(outlineForPlayer(Player::Black));
		checker.setPosition({m_offRectBlack.position.x + m_offRectBlack.size.x / 2.f,
			m_offRectBlack.position.y + (m_checkerRadius + i * (m_checkerRadius * 1.6f))});
		window.draw(checker);
	}
}

void BackgammonGame::drawAnimatedChecker(sf::RenderWindow& window) const {
	if (!m_animating) {
		return;
	}

	const float t = std::min(1.f, m_animTime / m_animDuration);
	const sf::Vector2f position(m_animStart.x + (m_animEnd.x - m_animStart.x) * t,
		m_animStart.y + (m_animEnd.y - m_animStart.y) * t);

	sf::CircleShape checker(m_checkerRadius);
	checker.setOrigin({m_checkerRadius, m_checkerRadius});
	checker.setFillColor(colorForPlayer(m_animPlayer));
	checker.setOutlineThickness(2.f);
	checker.setOutlineColor(outlineForPlayer(m_animPlayer));
	checker.setPosition(position);
	window.draw(checker);
}

void BackgammonGame::drawDice(sf::RenderWindow& window) const {
	sf::RectangleShape diceArea(m_diceRect.size);
	diceArea.setPosition(m_diceRect.position);
	diceArea.setFillColor(sf::Color(90, 70, 50));
	diceArea.setOutlineThickness(2.f);
	diceArea.setOutlineColor(sf::Color(40, 30, 20));
	window.draw(diceArea);

	auto drawDie = [&](sf::Vector2f pos, int value) {
		sf::RectangleShape die({kDiceSize, kDiceSize});
		die.setPosition(pos);
		die.setFillColor(sf::Color(230, 230, 230));
		die.setOutlineThickness(2.f);
		die.setOutlineColor(sf::Color(50, 50, 50));
		window.draw(die);

		auto drawPip = [&](float x, float y) {
			sf::CircleShape pip(4.f);
			pip.setOrigin({4.f, 4.f});
			pip.setFillColor(sf::Color(40, 40, 40));
			pip.setPosition({pos.x + x, pos.y + y});
			window.draw(pip);
		};

		const float left = kDiceSize * 0.25f;
		const float center = kDiceSize * 0.5f;
		const float right = kDiceSize * 0.75f;
		const float top = kDiceSize * 0.25f;
		const float middle = kDiceSize * 0.5f;
		const float bottom = kDiceSize * 0.75f;

		if (value == 1 || value == 3 || value == 5) {
			drawPip(center, middle);
		}
		if (value >= 2) {
			drawPip(left, top);
			drawPip(right, bottom);
		}
		if (value >= 4) {
			drawPip(right, top);
			drawPip(left, bottom);
		}
		if (value == 6) {
			drawPip(left, middle);
			drawPip(right, middle);
		}
	};

	if (m_moves.empty()) {
		drawDie(m_diceRect.position, 0);
		drawDie({m_diceRect.position.x + kDiceSize + kDicePadding, m_diceRect.position.y}, 0);
		return;
	}

	const int dieA = (m_dieUsesA > 0) ? m_dieValueA : 0;
	const int dieB = (m_dieUsesB > 0) ? m_dieValueB : 0;
	drawDie(m_diceRect.position, dieA);
	drawDie({m_diceRect.position.x + kDiceSize + kDicePadding, m_diceRect.position.y}, dieB);
}

void BackgammonGame::drawOffArea(sf::RenderWindow& window) const {
	sf::RectangleShape offWhite(m_offRectWhite.size);
	offWhite.setPosition(m_offRectWhite.position);
	offWhite.setFillColor(sf::Color(160, 160, 160));
	offWhite.setOutlineThickness(2.f);
	offWhite.setOutlineColor(sf::Color(80, 80, 80));
	window.draw(offWhite);

	sf::RectangleShape offBlack(m_offRectBlack.size);
	offBlack.setPosition(m_offRectBlack.position);
	offBlack.setFillColor(sf::Color(70, 70, 70));
	offBlack.setOutlineThickness(2.f);
	offBlack.setOutlineColor(sf::Color(140, 140, 140));
	window.draw(offBlack);
}

void BackgammonGame::drawSelection(sf::RenderWindow& window) const {
	if (m_selectedPoint.has_value()) {
		const PointSlot& slot = m_pointSlots[*m_selectedPoint];
		sf::RectangleShape highlight(slot.area.size);
		highlight.setPosition(slot.area.position);
		highlight.setFillColor(sf::Color(255, 255, 255, 40));
		window.draw(highlight);
	}

	sf::RectangleShape turnBar({m_boardRect.size.x, 6.f});
	turnBar.setPosition({m_boardRect.position.x, m_currentPlayer == Player::White
		? m_boardRect.position.y + m_boardRect.size.y - 6.f
		: m_boardRect.position.y});
	turnBar.setFillColor(colorForPlayer(m_currentPlayer));
	window.draw(turnBar);
}

void BackgammonGame::drawHighlights(sf::RenderWindow& window) const {
	if (m_gameOver || currentPlayer() != m_humanPlayer || m_moves.empty()) {
		return;
	}

	const sf::Color highlightColor(240, 220, 80, 90);
	const sf::Color offColor(120, 220, 140, 80);

	if (barCount(currentPlayer()) > 0) {
		const auto moves = legalBarMoves(currentPlayer());
		for (const auto& move : moves) {
			const PointSlot& slot = m_pointSlots[move.toIndex];
			sf::RectangleShape area(slot.area.size);
			area.setPosition(slot.area.position);
			area.setFillColor(highlightColor);
			window.draw(area);
		}
		return;
	}

	if (!m_selectedPoint.has_value()) {
		return;
	}

	const auto moves = legalMovesFromPoint(*m_selectedPoint, currentPlayer());
	bool canBearOffMove = false;
	for (const auto& move : moves) {
		if (move.bearOff) {
			canBearOffMove = true;
			continue;
		}
		const PointSlot& slot = m_pointSlots[move.toIndex];
		sf::RectangleShape area(slot.area.size);
		area.setPosition(slot.area.position);
		area.setFillColor(highlightColor);
		window.draw(area);
	}

	if (canBearOffMove) {
		const sf::FloatRect& offRect = (currentPlayer() == Player::White) ? m_offRectWhite : m_offRectBlack;
		sf::RectangleShape offArea(offRect.size);
		offArea.setPosition(offRect.position);
		offArea.setFillColor(offColor);
		window.draw(offArea);
	}
}

void BackgammonGame::drawHud(sf::RenderWindow& window) const {
	if (!m_hasFont) {
		return;
	}

	auto drawText = [&](const std::string& text, sf::Vector2f position, unsigned int size, sf::Color color) {
		sf::Text label(m_font);
		label.setString(text);
		label.setCharacterSize(size);
		label.setFillColor(color);
		label.setPosition(position);
		window.draw(label);
	};

	const float hudX = 8.f;
	const float hudTop = 6.f;
	const float lineGap = 14.f;
	const unsigned int mainSize = 13;
	const unsigned int smallSize = 12;

	const std::string playerText = (m_currentPlayer == Player::White) ? "White" : "Black";
	const std::string scoreText = "Score W " + std::to_string(m_scoreWhite) + " - B " +
		std::to_string(m_scoreBlack) + " (to " + std::to_string(m_matchTarget) + ")";
	drawText(scoreText, {hudX, hudTop}, mainSize, sf::Color::White);

	const std::string pipText = "Pip W " + std::to_string(pipCount(Player::White)) +
		" / B " + std::to_string(pipCount(Player::Black));
	drawText(pipText, {hudX, hudTop + lineGap}, smallSize, sf::Color(220, 220, 220));

	const std::string barOffText = "Bar W " + std::to_string(m_barWhite) + " B " +
		std::to_string(m_barBlack) + " | Off W " + std::to_string(m_offWhite) +
		" B " + std::to_string(m_offBlack);
	drawText(barOffText, {hudX, hudTop + lineGap * 2.f}, smallSize, sf::Color(200, 200, 200));

	drawText("Turn: " + playerText, {hudX, m_windowSize.y - 22.f}, mainSize, sf::Color::White);

	if (!m_moves.empty()) {
		std::string diceText = "Dice:";
		for (int die : m_moves) {
			diceText += " " + std::to_string(die);
		}
		drawText(diceText, {m_diceRect.position.x, m_diceRect.position.y + m_diceRect.size.y + 6.f}, 12, sf::Color::White);
	} else if (m_currentPlayer == m_humanPlayer && !m_gameOver) {
		drawText("Press R or click dice to roll", {m_diceRect.position.x - 18.f, m_diceRect.position.y + m_diceRect.size.y + 6.f}, 12, sf::Color(230, 230, 160));
	}

	if (m_gameOver) {
		const std::string winner = (m_scoreWhite > m_scoreBlack) ? "White wins match" : "Black wins match";
		drawText(winner, {m_windowSize.x * 0.35f, m_windowSize.y * 0.45f}, 28, sf::Color(255, 220, 120));
	}
}
