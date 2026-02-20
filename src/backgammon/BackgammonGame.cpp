#include "BackgammonGame.hpp"

#include <algorithm>
#include <random>
#include <string>

namespace {
constexpr float kMargin = 40.f;
constexpr float kOffAreaWidth = 60.f;
constexpr float kBarWidthRatio = 0.08f;
constexpr float kMidGap = 24.f;
constexpr float kDiceSize = 48.f;
constexpr float kDicePadding = 10.f;
constexpr int kMaxCheckers = 15;

sf::Color colorForPlayer(BackgammonGame::Player player) {
	return player == BackgammonGame::Player::White ? sf::Color(230, 230, 230)
												  : sf::Color(40, 40, 40);
}

sf::Color outlineForPlayer(BackgammonGame::Player player) {
	return player == BackgammonGame::Player::White ? sf::Color(50, 50, 50)
												  : sf::Color(200, 200, 200);
}
} // namespace

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

	if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>()) {
		if (mouse->button != sf::Mouse::Button::Left) {
			return;
		}
		const sf::Vector2f position(static_cast<float>(mouse->position.x),
			static_cast<float>(mouse->position.y));

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
	}
}

void BackgammonGame::update(float dtSeconds) {
	if (m_gameOver) {
		return;
	}

	if (m_animating) {
		m_animTime += dtSeconds;
		if (m_animTime >= m_animDuration) {
			m_animating = false;
		}
		return;
	}

	if (currentPlayer() != m_aiPlayer) {
		return;
	}

	m_aiMoveCooldown -= dtSeconds;
	if (m_aiMoveCooldown > 0.f) {
		return;
	}

	if (m_moves.empty()) {
		rollDice();
		checkAutoPass();
		m_aiMoveCooldown = 0.25f;
		return;
	}

	auto moves = legalMoves(m_aiPlayer);
	if (moves.empty()) {
		switchTurn();
		return;
	}

	auto scoreMove = [&](const MoveOption& move) {
		int score = 0;
		if (move.bearOff) {
			score += 100;
		}
		if (!move.bearOff) {
			const int targetCount = m_points[move.toIndex];
			if (m_aiPlayer == Player::White && targetCount == -1) {
				score += 50;
			} else if (m_aiPlayer == Player::Black && targetCount == 1) {
				score += 50;
			}
		}
		score += move.dieValue;
		return score;
	};

	auto bestMove = moves.front();
	int bestScore = scoreMove(bestMove);
	for (const auto& move : moves) {
		const int score = scoreMove(move);
		if (score > bestScore) {
			bestScore = score;
			bestMove = move;
		}
	}

	applyMove(bestMove);
	m_aiMoveCooldown = 0.2f;
}

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

void BackgammonGame::rollDice() {
	std::random_device device;
	std::mt19937 rng(device());
	std::uniform_int_distribution<int> dist(1, 6);

	const int dieA = dist(rng);
	const int dieB = dist(rng);

	m_moves.clear();
	if (dieA == dieB) {
		m_moves = {dieA, dieA, dieA, dieA};
	} else {
		m_moves = {dieA, dieB};
	}

	m_dieValueA = dieA;
	m_dieValueB = dieB;
	if (dieA == dieB) {
		m_dieUsesA = 2;
		m_dieUsesB = 2;
	} else {
		m_dieUsesA = 1;
		m_dieUsesB = 1;
	}
}

void BackgammonGame::switchTurn() {
	m_moves.clear();
	m_selectedPoint.reset();
	m_currentPlayer = (m_currentPlayer == Player::White) ? Player::Black : Player::White;
}

void BackgammonGame::checkAutoPass() {
	if (m_moves.empty()) {
		return;
	}
	if (legalMoves(currentPlayer()).empty()) {
		switchTurn();
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

bool BackgammonGame::applyMove(const MoveOption& move) {
	if (m_moves.empty()) {
		return false;
	}

	const Player player = currentPlayer();
	if (move.fromBar) {
		barCountRef(player) -= 1;
	} else {
		m_points[move.fromIndex] += (player == Player::White) ? -1 : 1;
	}

	if (move.bearOff) {
		offCountRef(player) += 1;
	} else {
		const int targetCount = m_points[move.toIndex];
		if (player == Player::White) {
			if (targetCount == -1) {
				m_points[move.toIndex] = 1;
				m_barBlack += 1;
			} else {
				m_points[move.toIndex] += 1;
			}
		} else {
			if (targetCount == 1) {
				m_points[move.toIndex] = -1;
				m_barWhite += 1;
			} else {
				m_points[move.toIndex] -= 1;
			}
		}
	}

	if (move.dieIndex >= 0 && move.dieIndex < static_cast<int>(m_moves.size())) {
		const int usedValue = m_moves[move.dieIndex];
		m_moves.erase(m_moves.begin() + move.dieIndex);
		if (usedValue == m_dieValueA && m_dieUsesA > 0) {
			m_dieUsesA -= 1;
		} else if (usedValue == m_dieValueB && m_dieUsesB > 0) {
			m_dieUsesB -= 1;
		} else if (m_dieUsesA > 0) {
			m_dieUsesA -= 1;
		} else if (m_dieUsesB > 0) {
			m_dieUsesB -= 1;
		}
	}

	m_selectedPoint.reset();
	m_animating = true;
	m_animTime = 0.f;
	m_animPlayer = player;
	m_animFromBar = move.fromBar;
	m_animBearOff = move.bearOff;
	m_animFromIndex = move.fromIndex;
	m_animToIndex = move.toIndex;

	auto stackSpacing = [&](int count, float maxStackHeight) {
		float spacing = m_checkerRadius * 2.f - 2.f;
		if (count > 1 && maxStackHeight > 0.f) {
			spacing = std::min(spacing, maxStackHeight / static_cast<float>(count - 1));
		}
		return spacing;
	};

	auto pointStackPosition = [&](int index, int countBefore, bool isTop) {
		const float maxStackHeight = std::max(0.f, m_pointHeight - m_checkerRadius * 2.f);
		const float spacing = stackSpacing(countBefore, maxStackHeight);
		const float offset = (countBefore > 0) ? static_cast<float>(countBefore) * spacing : 0.f;
		const sf::Vector2f base = m_pointSlots[index].center;
		return sf::Vector2f(base.x, base.y + (isTop ? offset : -offset));
	};

	if (move.fromBar) {
		const int countBefore = barCount(player) + 1;
		const float maxStackHeight = std::max(0.f, m_barRect.size.y - m_checkerRadius * 2.f);
		const float spacing = stackSpacing(countBefore, maxStackHeight);
		const float offset = (countBefore > 0) ? static_cast<float>(countBefore - 1) * spacing : 0.f;
		const float baseY = (player == Player::White)
			? m_barRect.position.y + m_barRect.size.y - m_checkerRadius
			: m_barRect.position.y + m_checkerRadius;
		m_animStart = {m_barRect.position.x + m_barRect.size.x / 2.f,
			(player == Player::White) ? baseY - offset : baseY + offset};
	} else {
		const int countBefore = std::abs(m_points[move.fromIndex]) + 1;
		const bool isTop = m_pointSlots[move.fromIndex].isTop;
		const float maxStackHeight = std::max(0.f, m_pointHeight - m_checkerRadius * 2.f);
		const float spacing = stackSpacing(countBefore, maxStackHeight);
		const float offset = (countBefore > 0) ? static_cast<float>(countBefore - 1) * spacing : 0.f;
		const sf::Vector2f base = m_pointSlots[move.fromIndex].center;
		m_animStart = {base.x, base.y + (isTop ? offset : -offset)};
	}

	if (move.bearOff) {
		const sf::FloatRect& offRect = (player == Player::White) ? m_offRectWhite : m_offRectBlack;
		const int countBefore = offCountRef(player) - 1;
		const float spacing = m_checkerRadius * 1.6f;
		const float offset = (countBefore > 0) ? static_cast<float>(countBefore) * spacing : 0.f;
		const float baseY = (player == Player::White)
			? offRect.position.y + offRect.size.y - m_checkerRadius
			: offRect.position.y + m_checkerRadius;
		m_animEnd = {offRect.position.x + offRect.size.x / 2.f,
			(player == Player::White) ? baseY - offset : baseY + offset};
	} else {
		const int countBefore = std::max(0, std::abs(m_points[move.toIndex]) - 1);
		const bool isTop = m_pointSlots[move.toIndex].isTop;
		m_animEnd = pointStackPosition(move.toIndex, countBefore, isTop);
	}

	if (m_offWhite == kMaxCheckers || m_offBlack == kMaxCheckers) {
		if (player == Player::White) {
			m_scoreWhite += 1;
		} else {
			m_scoreBlack += 1;
		}

		if (m_scoreWhite >= m_matchTarget || m_scoreBlack >= m_matchTarget) {
			m_gameOver = true;
			m_moves.clear();
			return true;
		}

		m_barWhite = 0;
		m_barBlack = 0;
		m_offWhite = 0;
		m_offBlack = 0;
		m_moves.clear();
		m_dieUsesA = 0;
		m_dieUsesB = 0;
		m_selectedPoint.reset();
		setupBoard();
		m_currentPlayer = (player == Player::White) ? Player::Black : Player::White;
		return true;
	}

	if (m_moves.empty()) {
		m_dieUsesA = 0;
		m_dieUsesB = 0;
		switchTurn();
		return true;
	}

	if (legalMoves(currentPlayer()).empty()) {
		switchTurn();
	}

	return true;
}

bool BackgammonGame::isLegalDestination(Player player, int pointIndex) const {
	const int count = m_points[pointIndex];
	if (count == 0) {
		return true;
	}
	if (player == Player::White && count > 0) {
		return true;
	}
	if (player == Player::Black && count < 0) {
		return true;
	}
	return std::abs(count) == 1;
}

bool BackgammonGame::canBearOff(Player player) const {
	if (barCount(player) > 0) {
		return false;
	}

	if (player == Player::White) {
		for (int i = 6; i < 24; ++i) {
			if (m_points[i] > 0) {
				return false;
			}
		}
	} else {
		for (int i = 0; i < 18; ++i) {
			if (m_points[i] < 0) {
				return false;
			}
		}
	}

	return true;
}

bool BackgammonGame::hasCheckersBehind(Player player, int fromIndex) const {
	if (player == Player::White) {
		for (int i = fromIndex + 1; i <= 5; ++i) {
			if (m_points[i] > 0) {
				return true;
			}
		}
	} else {
		for (int i = 18; i < fromIndex; ++i) {
			if (m_points[i] < 0) {
				return true;
			}
		}
	}
	return false;
}

int BackgammonGame::findUsableDie(int distance, bool allowLarger, int fromIndex) const {
	if (m_moves.empty()) {
		return -1;
	}

	for (size_t i = 0; i < m_moves.size(); ++i) {
		if (m_moves[i] == distance) {
			return static_cast<int>(i);
		}
	}

	if (!allowLarger) {
		return -1;
	}

	if (!canBearOff(currentPlayer()) || fromIndex < 0 || hasCheckersBehind(currentPlayer(), fromIndex)) {
		return -1;
	}

	int bestIndex = -1;
	int bestValue = 7;
	for (size_t i = 0; i < m_moves.size(); ++i) {
		if (m_moves[i] > distance && m_moves[i] < bestValue) {
			bestValue = m_moves[i];
			bestIndex = static_cast<int>(i);
		}
	}
	return bestIndex;
}

int BackgammonGame::pipCount(Player player) const {
	int total = 0;
	for (int i = 0; i < 24; ++i) {
		const int count = m_points[i];
		if (player == Player::White && count > 0) {
			total += count * (i + 1);
		}
		if (player == Player::Black && count < 0) {
			total += (-count) * (24 - i);
		}
	}
	total += barCount(player) * 25;
	return total;
}

std::vector<BackgammonGame::MoveOption> BackgammonGame::legalMoves(Player player) const {
	if (m_moves.empty()) {
		return {};
	}

	if (barCount(player) > 0) {
		return legalBarMoves(player);
	}

	std::vector<MoveOption> moves;
	for (int i = 0; i < 24; ++i) {
		const int count = m_points[i];
		if (player == Player::White && count > 0) {
			auto pointMoves = legalMovesFromPoint(i, player);
			moves.insert(moves.end(), pointMoves.begin(), pointMoves.end());
		}
		if (player == Player::Black && count < 0) {
			auto pointMoves = legalMovesFromPoint(i, player);
			moves.insert(moves.end(), pointMoves.begin(), pointMoves.end());
		}
	}
	return moves;
}

std::vector<BackgammonGame::MoveOption> BackgammonGame::legalMovesFromPoint(int pointIndex, Player player) const {
	std::vector<MoveOption> moves;
	if (m_moves.empty()) {
		return moves;
	}

	const int direction = (player == Player::White) ? -1 : 1;
	const int distanceToOff = (player == Player::White) ? pointIndex + 1 : 24 - pointIndex;

	for (size_t i = 0; i < m_moves.size(); ++i) {
		const int die = m_moves[i];
		const int target = pointIndex + direction * die;

		if (target >= 0 && target < 24) {
			if (isLegalDestination(player, target)) {
				moves.push_back({pointIndex, target, false, false, static_cast<int>(i), die});
			}
			continue;
		}

		if (!canBearOff(player)) {
			continue;
		}

		if (die == distanceToOff || (die > distanceToOff && !hasCheckersBehind(player, pointIndex))) {
			moves.push_back({pointIndex, -1, false, true, static_cast<int>(i), die});
		}
	}

	return moves;
}

std::vector<BackgammonGame::MoveOption> BackgammonGame::legalBarMoves(Player player) const {
	std::vector<MoveOption> moves;
	if (m_moves.empty() || barCount(player) == 0) {
		return moves;
	}

	for (size_t i = 0; i < m_moves.size(); ++i) {
		const int die = m_moves[i];
		const int target = (player == Player::White) ? 24 - die : die - 1;
		if (target < 0 || target >= 24) {
			continue;
		}
		if (isLegalDestination(player, target)) {
			moves.push_back({-1, target, true, false, static_cast<int>(i), die});
		}
	}
	return moves;
}

std::vector<BackgammonGame::MoveOption> BackgammonGame::legalBearOffMovesFromPoint(int pointIndex, Player player) const {
	std::vector<MoveOption> moves;
	if (!canBearOff(player)) {
		return moves;
	}

	const int distanceToOff = (player == Player::White) ? pointIndex + 1 : 24 - pointIndex;
	for (size_t i = 0; i < m_moves.size(); ++i) {
		const int die = m_moves[i];
		if (die == distanceToOff || (die > distanceToOff && !hasCheckersBehind(player, pointIndex))) {
			moves.push_back({pointIndex, -1, false, true, static_cast<int>(i), die});
		}
	}
	return moves;
}

BackgammonGame::Player BackgammonGame::currentPlayer() const {
	return m_currentPlayer;
}

int BackgammonGame::barCount(Player player) const {
	return player == Player::White ? m_barWhite : m_barBlack;
}

int& BackgammonGame::barCountRef(Player player) {
	return player == Player::White ? m_barWhite : m_barBlack;
}

int& BackgammonGame::offCountRef(Player player) {
	return player == Player::White ? m_offWhite : m_offBlack;
}

int BackgammonGame::pointCount(int pointIndex) const {
	return std::abs(m_points[pointIndex]);
}

std::optional<int> BackgammonGame::pointFromMouse(sf::Vector2f mouse) const {
	for (int i = 0; i < 24; ++i) {
		if (m_pointSlots[i].area.contains(mouse)) {
			return i;
		}
	}
	return std::nullopt;
}

bool BackgammonGame::isOnDice(sf::Vector2f mouse) const {
	return m_diceRect.contains(mouse);
}

bool BackgammonGame::isOnOffArea(sf::Vector2f mouse, Player player) const {
	const sf::FloatRect& rect = (player == Player::White) ? m_offRectWhite : m_offRectBlack;
	return rect.contains(mouse);
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

