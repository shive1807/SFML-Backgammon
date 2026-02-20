#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <vector>

class BackgammonGame {
public:
	enum class Player { White, Black };

	explicit BackgammonGame(sf::Vector2u windowSize);

	void handleEvent(const sf::Event& event);
	void update(float dtSeconds);
	void render(sf::RenderWindow& window) const;

private:

	struct PointSlot {
		sf::FloatRect area;
		sf::Vector2f center;
		bool isTop;
	};

	struct MoveOption {
		int fromIndex;
		int toIndex;
		bool fromBar;
		bool bearOff;
		int dieIndex;
		int dieValue;
	};

	void setupBoard();
	void rebuildLayout();
	void rollDice();
	void switchTurn();
	void checkAutoPass();
	void ensureFontLoaded();

	void trySelectPoint(int pointIndex);
	bool tryMoveToPoint(int pointIndex);
	void tryBearOff();
	bool tryMoveFromBar(int pointIndex);
	bool tryMoveChecker(int fromIndex, std::optional<int> toIndex);
	bool applyMove(const MoveOption& move);

	bool isLegalDestination(Player player, int pointIndex) const;
	bool canBearOff(Player player) const;
	bool hasCheckersBehind(Player player, int fromIndex) const;
	int findUsableDie(int distance, bool allowLarger, int fromIndex) const;
	int pipCount(Player player) const;
	std::vector<MoveOption> legalMoves(Player player) const;
	std::vector<MoveOption> legalMovesFromPoint(int pointIndex, Player player) const;
	std::vector<MoveOption> legalBarMoves(Player player) const;
	std::vector<MoveOption> legalBearOffMovesFromPoint(int pointIndex, Player player) const;

	Player currentPlayer() const;
	int barCount(Player player) const;
	int& barCountRef(Player player);
	int& offCountRef(Player player);
	int pointCount(int pointIndex) const;
	std::optional<int> pointFromMouse(sf::Vector2f mouse) const;
	bool isOnDice(sf::Vector2f mouse) const;
	bool isOnOffArea(sf::Vector2f mouse, Player player) const;

	void drawBoard(sf::RenderWindow& window) const;
	void drawCheckers(sf::RenderWindow& window) const;
	void drawDice(sf::RenderWindow& window) const;
	void drawOffArea(sf::RenderWindow& window) const;
	void drawSelection(sf::RenderWindow& window) const;
	void drawHighlights(sf::RenderWindow& window) const;
	void drawHud(sf::RenderWindow& window) const;
	void drawAnimatedChecker(sf::RenderWindow& window) const;

	sf::Vector2u m_windowSize;
	sf::FloatRect m_boardRect;
	sf::FloatRect m_barRect;
	sf::FloatRect m_diceRect;
	sf::FloatRect m_offRectWhite;
	sf::FloatRect m_offRectBlack;
	float m_pointWidth;
	float m_pointHeight;
	float m_checkerRadius;
	float m_midGap;

	std::array<int, 24> m_points;
	int m_barWhite;
	int m_barBlack;
	int m_offWhite;
	int m_offBlack;

	Player m_currentPlayer;
	Player m_humanPlayer;
	Player m_aiPlayer;
	std::vector<int> m_moves;
	std::optional<int> m_selectedPoint;
	float m_aiMoveCooldown;

	mutable std::vector<PointSlot> m_pointSlots;

	bool m_animating;
	float m_animTime;
	float m_animDuration;
	Player m_animPlayer;
	bool m_animFromBar;
	bool m_animBearOff;
	int m_animFromIndex;
	int m_animToIndex;
	sf::Vector2f m_animStart;
	sf::Vector2f m_animEnd;

	int m_dieValueA;
	int m_dieValueB;
	int m_dieUsesA;
	int m_dieUsesB;

	int m_matchTarget;
	int m_scoreWhite;
	int m_scoreBlack;
	bool m_gameOver;

	sf::Font m_font;
	bool m_hasFont;
};

