#include "BackgammonGame.hpp"
#include "BackgammonGameInternal.hpp"

#include <algorithm>
#include <cstdlib>

using namespace backgammon_internal;

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
