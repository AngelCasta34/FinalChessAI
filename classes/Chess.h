#pragma once

#include "Game.h"
#include "Bitboard.h"
#include "Grid.h"
#include <vector>
#include <array>
#include <map>


constexpr int pieceSize = 80;

/*enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};
*/

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;
    void setUpBoardWithAI();   // setUpBoard() / makes black the AI player

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

    std::vector<BitMove> generateAllMoves();

    // AI interface
    bool gameHasAI() override;
    void updateAI() override;

    // Tournament support methods
    void setBoardFromFEN(const std::string& fen);
    BitMove getLastAIMove() const { return _lastAIMove; }
    std::string getFEN() const;
    int getCurrentPlayerColor() const { return _whiteToMove ? 1 : -1; }
    BitMove _lastAIMove;

private:
    
    using BoardArray = std::array<int, 66>;

    BoardArray boardToArray() const;

    // Move generation on lightweight array 
    std::vector<BitMove> generateMovesForArray(const BoardArray& board, bool whiteToMove) const;
    void addRayMovesForArray(const BoardArray& board, std::vector<BitMove>& moves,
                             int fromX, int fromY, int dx, int dy,
                             ChessPiece piece, bool whiteToMove) const;

    // Static evaluation 
    int evaluateArray(const BoardArray& board) const;

    // Negamax with alpha-beta pruning
    // Returns score relative to the side to move (positive = good for that side)
    int negamax(BoardArray board, int depth, int alpha, int beta, bool whiteToMove);

    // Execute a move on the real grid and advance the turn
    void executeAIMove(const BitMove& move);
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    bool _whiteToMove = true;
    std::vector<BitMove> _legalMoves;
    std::map<std::string, int> _positionCount;

    // Extra game state
    int _enPassantSquare = -1;    
    int _castlingRights  = 15;    

    void regenerateLegalMoves();
    void addRayMoves(int fromX, int fromY, int dx, int dy, ChessPiece piece);

    int holderToIndex(BitHolder& h) const;
    bool isWhiteBit(const Bit& bit) const;
    ChessPiece bitToPiece(const Bit& bit) const;
    bool isOccupied(int idx) const;
    bool isOccupiedByWhite(int idx) const;
    bool isOccupiedByBlack(int idx) const;
    Grid* _grid;
};
