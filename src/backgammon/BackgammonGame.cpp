#include "BackgammonGame.hpp"
#include "BackgammonGameInternal.hpp"

#include <algorithm>

using namespace backgammon_internal;

BackgammonGame::BackgammonGame(sf::Vector2u windowSize)
	: m_windowSize(windowSize)
	, m_points{}
	, m_barWhite(0)
	, m_barBlack(0)
	, m_offWhite(0)
	, m_offBlack(0)
	, m_currentPlayer(Player::White)
	, m_humanPlayer(Player::White)
	, m_aiPlayer(Player::Black)
	, m_moves()
	, m_selectedPoint(std::nullopt) {
	m_aiMoveCooldown = 0.f;
	m_animating = false;
	m_animTime = 0.f;
	m_animDuration = 0.45f;
	m_animPlayer = Player::White;
	m_animFromBar = false;
	m_animBearOff = false;
	m_animFromIndex = -1;
	m_animToIndex = -1;
	m_animStart = {0.f, 0.f};
	m_animEnd = {0.f, 0.f};
	m_dieValueA = 0;
	m_dieValueB = 0;
	m_dieUsesA = 0;
	m_dieUsesB = 0;
	m_matchTarget = 3;
	m_scoreWhite = 0;
	m_scoreBlack = 0;
	m_gameOver = false;
	m_hasFont = false;
	setupBoard();
	rebuildLayout();
	ensureFontLoaded();
}

void BackgammonGame::setupBoard() {
	m_points.fill(0);

	m_points[23] = 2;
	m_points[12] = 5;
	m_points[7] = 3;
	m_points[5] = 5;

	m_points[0] = -2;
	m_points[11] = -5;
	m_points[16] = -3;
	m_points[18] = -5;
}

void BackgammonGame::rebuildLayout() {
	const float boardWidth = static_cast<float>(m_windowSize.x) - 2.f * kMargin - kOffAreaWidth;
	const float boardHeight = static_cast<float>(m_windowSize.y) - 2.f * kMargin;

	m_boardRect = {sf::Vector2f(kMargin, kMargin), sf::Vector2f(boardWidth, boardHeight)};
	const float halfWidth = (boardWidth - boardWidth * kBarWidthRatio) / 2.f;
	const float barWidth = boardWidth * kBarWidthRatio;
	m_barRect = {sf::Vector2f(m_boardRect.position.x + halfWidth, kMargin),
		sf::Vector2f(barWidth, boardHeight)};

	m_pointWidth = halfWidth / 6.f;
	m_midGap = kMidGap;
	m_pointHeight = (boardHeight - m_midGap) / 2.f;
	m_checkerRadius = std::min(m_pointWidth * 0.45f, 24.f);

	const float offLeft = m_boardRect.position.x + boardWidth + 10.f;
	m_offRectWhite = {sf::Vector2f(offLeft, m_boardRect.position.y + boardHeight / 2.f + 8.f),
		sf::Vector2f(kOffAreaWidth - 20.f, boardHeight / 2.f - 8.f)};
	m_offRectBlack = {sf::Vector2f(offLeft, m_boardRect.position.y),
		sf::Vector2f(kOffAreaWidth - 20.f, boardHeight / 2.f - 8.f)};

	const float diceX = m_barRect.position.x + (m_barRect.size.x - kDiceSize * 2.f - kDicePadding) / 2.f;
	const float diceY = m_boardRect.position.y + (m_boardRect.size.y - kDiceSize) / 2.f;
	m_diceRect = {sf::Vector2f(diceX, diceY), sf::Vector2f(kDiceSize * 2.f + kDicePadding, kDiceSize)};

	m_pointSlots.clear();
	m_pointSlots.resize(24);

	const float topBaseY = m_boardRect.position.y + m_checkerRadius + 6.f;
	const float bottomBaseY = m_boardRect.position.y + m_boardRect.size.y - m_checkerRadius - 6.f;
	const float midY = m_boardRect.position.y + m_boardRect.size.y / 2.f;

	for (int index = 0; index < 24; ++index) {
		const bool isTop = index >= 12;
		const int pointInRow = isTop ? index - 12 : 11 - index;
		const bool isLeftHalf = pointInRow < 6;
		const int column = isLeftHalf ? pointInRow : pointInRow - 6;
		const float x = (isLeftHalf ? m_boardRect.position.x : m_barRect.position.x + m_barRect.size.x) +
			column * m_pointWidth;

		sf::FloatRect area;
		if (isTop) {
			area = {sf::Vector2f(x, m_boardRect.position.y), sf::Vector2f(m_pointWidth, midY - m_boardRect.position.y)};
		} else {
			area = {sf::Vector2f(x, midY), sf::Vector2f(m_pointWidth, m_boardRect.position.y + m_boardRect.size.y - midY)};
		}

		const float centerX = x + m_pointWidth / 2.f;
		const float centerY = isTop ? topBaseY : bottomBaseY;

		m_pointSlots[index] = {area, sf::Vector2f(centerX, centerY), isTop};
	}
}

void BackgammonGame::ensureFontLoaded() {
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
