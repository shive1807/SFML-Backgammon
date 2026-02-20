#pragma once

#include <SFML/Graphics.hpp>

#include "BackgammonGame.hpp"

namespace backgammon_internal {
constexpr float kMargin = 40.f;
constexpr float kOffAreaWidth = 60.f;
constexpr float kBarWidthRatio = 0.08f;
constexpr float kMidGap = 24.f;
constexpr float kDiceSize = 48.f;
constexpr float kDicePadding = 10.f;
constexpr int kMaxCheckers = 15;

inline sf::Color colorForPlayer(BackgammonGame::Player player) {
	return player == BackgammonGame::Player::White ? sf::Color(230, 230, 230)
											  : sf::Color(40, 40, 40);
}

inline sf::Color outlineForPlayer(BackgammonGame::Player player) {
	return player == BackgammonGame::Player::White ? sf::Color(50, 50, 50)
											  : sf::Color(200, 200, 200);
}
} // namespace backgammon_internal
