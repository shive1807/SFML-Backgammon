#include "BackgammonGame.hpp"

#include <random>

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
