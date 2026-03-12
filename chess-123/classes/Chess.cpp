#include "Chess.h"
#include "evaluate.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    bit->setGameTag((playerNumber == 0) ? (int)piece : (128 + (int)piece));
    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    _gameOptions.AIMAXDepth = 4;   // search 4 plies deep

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::setUpBoardWithAI()
{
    setUpBoard();
    setAIPlayer(1);   // BLACK is the AI
}

void Chess::FENtoBoard(const std::string& fen)
{
    // Reset game state
    _enPassantSquare = -1;
    _castlingRights  = 15;   // all four rights by default

    // Split FEN into fields
    std::string boardField = fen;
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        boardField = fen.substr(0, spacePos);

        size_t p = spacePos + 1;
        // Skip active color field
        p = fen.find(' ', p);
        if (p != std::string::npos) {
            p++;
            // Castling field
            size_t p2 = fen.find(' ', p);
            std::string castleStr = (p2 != std::string::npos) ? fen.substr(p, p2 - p) : fen.substr(p);
            _castlingRights = 0;
            for (char c : castleStr) {
                if (c == 'K') _castlingRights |= 1;
                if (c == 'Q') _castlingRights |= 2;
                if (c == 'k') _castlingRights |= 4;
                if (c == 'q') _castlingRights |= 8;
            }
            // En-passant field
            if (p2 != std::string::npos) {
                p = p2 + 1;
                size_t p3 = fen.find(' ', p);
                std::string epStr = (p3 != std::string::npos) ? fen.substr(p, p3 - p) : fen.substr(p);
                if (epStr.size() >= 2 && epStr[0] != '-') {
                    int epX = epStr[0] - 'a';
                    int epY = 8 - (epStr[1] - '0');
                    if (epX >= 0 && epX < 8 && epY >= 0 && epY < 8)
                        _enPassantSquare = epY * 8 + epX;
                }
            }
        }
    }

    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });

    int x = 0;
    int y = 0;

    auto charToPiece = [](char c) -> ChessPiece {
        switch (std::tolower((unsigned char)c)) {
            case 'p': return Pawn;
            case 'n': return Knight;
            case 'b': return Bishop;
            case 'r': return Rook;
            case 'q': return Queen;
            case 'k': return King;
            default:  return Pawn;
        }
    };

    for (size_t i = 0; i < boardField.size(); i++) {
        char c = boardField[i];

        if (c == '/') {
            y++;
            x = 0;
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            x += (c - '0');
            continue;
        }

        if (x < 0 || x >= 8 || y < 0 || y >= 8) {
            continue;
        }

        int playerNumber = std::isupper((unsigned char)c) ? 0 : 1;
        ChessPiece piece = charToPiece(c);

        ChessSquare* square = _grid->getSquare(x, y);
        if (square) {
            square->setBit(PieceForPlayer(playerNumber, piece));
        }

        x++;
    }

    regenerateLegalMoves();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::isWhiteBit(const Bit& bit) const
{
    return (bit.gameTag() & 128) == 0;
}

ChessPiece Chess::bitToPiece(const Bit& bit) const
{
    int tag = bit.gameTag();
    int piece = tag & 127;
    if (piece < 0 || piece > 6) return NoPiece;
    return (ChessPiece)piece;
}

int Chess::holderToIndex(BitHolder& h) const
{
    int found = -1;
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if ((BitHolder*)sq == &h) {
            found = y * 8 + x;
        }
    });
    return found;
}

bool Chess::isOccupied(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;
    return _grid->getSquare(x, y)->bit() != nullptr;
}

bool Chess::isOccupiedByWhite(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;

    Bit* b = _grid->getSquare(x, y)->bit();
    return b && isWhiteBit(*b);
}

bool Chess::isOccupiedByBlack(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;

    Bit* b = _grid->getSquare(x, y)->bit();
    return b && !isWhiteBit(*b);
}

std::vector<BitMove> Chess::generateAllMoves()
{
    regenerateLegalMoves();
    return _legalMoves;
}


void Chess::addRayMoves(int fromX, int fromY, int dx, int dy, ChessPiece piece)
{
    int x = fromX + dx;
    int y = fromY + dy;

    int from = fromY * 8 + fromX;

    while (x >= 0 && x < 8 && y >= 0 && y < 8)
    {
        int to = y * 8 + x;

        // friendly blocks immediately
        if (_whiteToMove && isOccupiedByWhite(to)) return;
        if (!_whiteToMove && isOccupiedByBlack(to)) return;

        if (!isOccupied(to))
        {
            _legalMoves.push_back(BitMove(from, to, piece));
        }
        else
        {
            // enemy capture allowed, then stop
            if (_whiteToMove && isOccupiedByBlack(to))
                _legalMoves.push_back(BitMove(from, to, piece));
            else if (!_whiteToMove && isOccupiedByWhite(to))
                _legalMoves.push_back(BitMove(from, to, piece));

            return;
        }

        x += dx;
        y += dy;
    }
}

void Chess::regenerateLegalMoves()
{
    _legalMoves.clear();

    _whiteToMove = (getCurrentPlayer()->playerNumber() == 0);

    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        Bit* b = sq->bit();
        if (!b) return;

        bool pieceIsWhite = isWhiteBit(*b);
        if (pieceIsWhite != _whiteToMove) return;

        ChessPiece piece = bitToPiece(*b);

        if (piece == NoPiece) return;

        int from = y * 8 + x;

        // PAWNS 
        if (piece == Pawn)
        {
            int dir = _whiteToMove ? -8 : +8;

            int one = from + dir;
            if (one >= 0 && one < 64)
            {
                if (!isOccupied(one))
                {
                    _legalMoves.push_back(BitMove(from, one, Pawn));

                    bool onStartRank = _whiteToMove ? (y == 6) : (y == 1);
                    if (onStartRank)
                    {
                        int two = from + 2 * dir;
                        if (two >= 0 && two < 64 && !isOccupied(two))
                        {
                            _legalMoves.push_back(BitMove(from, two, Pawn));
                        }
                    }
                }
            }

            int capL = from + dir - 1;
            int capR = from + dir + 1;

            bool canCapL = (x > 0);
            bool canCapR = (x < 7);

            if (canCapL && capL >= 0 && capL < 64)
            {
                if (_whiteToMove && isOccupiedByBlack(capL)) _legalMoves.push_back(BitMove(from, capL, Pawn));
                if (!_whiteToMove && isOccupiedByWhite(capL)) _legalMoves.push_back(BitMove(from, capL, Pawn));
            }

            if (canCapR && capR >= 0 && capR < 64)
            {
                if (_whiteToMove && isOccupiedByBlack(capR)) _legalMoves.push_back(BitMove(from, capR, Pawn));
                if (!_whiteToMove && isOccupiedByWhite(capR)) _legalMoves.push_back(BitMove(from, capR, Pawn));
            }

            // EN PASSANT
            if (_enPassantSquare != -1)
            {
                int epX = _enPassantSquare % 8;
                int epY = _enPassantSquare / 8;
                int captureRow = _whiteToMove ? epY + 1 : epY - 1;
                if (y == captureRow && std::abs(x - epX) == 1)
                    _legalMoves.push_back(BitMove(from, _enPassantSquare, Pawn));
            }
        }

        // KNIGHTS 
        if (piece == Knight)
        {
            const int dx[8] = { 1, 2,  2,  1, -1, -2, -2, -1 };
            const int dy[8] = { 2, 1, -1, -2, -2, -1,  1,  2 };

            for (int i = 0; i < 8; i++)
            {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;

                int to = ny * 8 + nx;

                if (_whiteToMove && isOccupiedByWhite(to)) continue;
                if (!_whiteToMove && isOccupiedByBlack(to)) continue;

                _legalMoves.push_back(BitMove(from, to, Knight));
            }
        }

        // KING
        if (piece == King)
        {
            for (int oy = -1; oy <= 1; oy++)
            {
                for (int ox = -1; ox <= 1; ox++)
                {
                    if (ox == 0 && oy == 0) continue;

                    int nx = x + ox;
                    int ny = y + oy;
                    if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;

                    int to = ny * 8 + nx;

                    if (_whiteToMove && isOccupiedByWhite(to)) continue;
                    if (!_whiteToMove && isOccupiedByBlack(to)) continue;

                    _legalMoves.push_back(BitMove(from, to, King));
                }
            }

            // CASTLING 
            if (_whiteToMove)
            {
                if ((_castlingRights & 1) && !isOccupied(61) && !isOccupied(62))
                    _legalMoves.push_back(BitMove(60, 62, King));
                if ((_castlingRights & 2) && !isOccupied(57) && !isOccupied(58) && !isOccupied(59))
                    _legalMoves.push_back(BitMove(60, 58, King));
            }
            else
            {
                if ((_castlingRights & 4) && !isOccupied(5) && !isOccupied(6))
                    _legalMoves.push_back(BitMove(4, 6, King));
                if ((_castlingRights & 8) && !isOccupied(1) && !isOccupied(2) && !isOccupied(3))
                    _legalMoves.push_back(BitMove(4, 2, King));
            }
        }

        // ROOK 
        if (piece == Rook)
        {
            addRayMoves(x, y,  1,  0, Rook);
            addRayMoves(x, y, -1,  0, Rook);
            addRayMoves(x, y,  0,  1, Rook);
            addRayMoves(x, y,  0, -1, Rook);
        }

        // BISHOP
        if (piece == Bishop)
        {
            addRayMoves(x, y,  1,  1, Bishop);
            addRayMoves(x, y, -1,  1, Bishop);
            addRayMoves(x, y,  1, -1, Bishop);
            addRayMoves(x, y, -1, -1, Bishop);
        }

        // QUEEN 
        if (piece == Queen)
        {
            // rook directions
            addRayMoves(x, y,  1,  0, Queen);
            addRayMoves(x, y, -1,  0, Queen);
            addRayMoves(x, y,  0,  1, Queen);
            addRayMoves(x, y,  0, -1, Queen);

            // bishop directions
            addRayMoves(x, y,  1,  1, Queen);
            addRayMoves(x, y, -1,  1, Queen);
            addRayMoves(x, y,  1, -1, Queen);
            addRayMoves(x, y, -1, -1, Queen);
        }
    });
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    bool pieceIsWhite = isWhiteBit(bit);
    return pieceIsWhite == _whiteToMove;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessPiece piece = bitToPiece(bit);

    if (piece == NoPiece) return false;

    int from = holderToIndex(src);
    int to   = holderToIndex(dst);
    if (from < 0 || to < 0) return false;

    regenerateLegalMoves();

    for (const BitMove& m : _legalMoves)
    {
        if (m.from == from && m.to == to && m.piece == piece)
        {
            return true;
        }
    }

    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    bool whiteKingAlive = false;
    bool blackKingAlive = false;

    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        Bit* b = sq->bit();
        if (!b) return;
        int tag = b->gameTag();
        if (tag == King)              whiteKingAlive = true;  // white king = 6
        if (tag == (128 + (int)King)) blackKingAlive = true;  // black king = 134
    });

    if (!whiteKingAlive) return getPlayerAt(1);   // black wins
    if (!blackKingAlive) return getPlayerAt(0);   // white wins
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });

    regenerateLegalMoves();
}

void Chess::bitMovedFromTo(Bit& bit, BitHolder& src, BitHolder& dst)
{
    int fromIdx = holderToIndex(src);
    int toIdx   = holderToIndex(dst);
    if (fromIdx < 0 || toIdx < 0) { endTurn(); return; }

    int fromX = fromIdx % 8, fromY = fromIdx / 8;
    int toX   = toIdx   % 8, toY   = toIdx   / 8;
    bool isWhite = isWhiteBit(bit);
    ChessPiece piece = bitToPiece(bit);

    // EN PASSANT capture
    if (piece == Pawn && toIdx == _enPassantSquare && fromX != toX)
    {
        int capRow = isWhite ? toY + 1 : toY - 1;
        ChessSquare* capSq = _grid->getSquare(toX, capRow);
        if (capSq) capSq->destroyBit();
    }

    // Update en-passant target for next turn
    _enPassantSquare = -1;
    if (piece == Pawn && std::abs(toIdx - fromIdx) == 16)
        _enPassantSquare = (fromIdx + toIdx) / 2;

    // CASTLING
    if (piece == King && std::abs(toX - fromX) == 2)
    {
        int rookSrcX = (toX == 6) ? 7 : 0;
        int rookDstX = (toX == 6) ? 5 : 3;
        ChessSquare* rSrc = _grid->getSquare(rookSrcX, fromY);
        ChessSquare* rDst = _grid->getSquare(rookDstX, fromY);
        if (rSrc && rDst)
        {
            Bit* rook = rSrc->takeBit();
            if (rook) rDst->setBit(rook);
        }
    }

    // Update castling rights
    if (piece == King) { if (isWhite) _castlingRights &= ~3; else _castlingRights &= ~12; }
    if (piece == Rook)
    {
        if (fromIdx == 63) _castlingRights &= ~1;
        if (fromIdx == 56) _castlingRights &= ~2;
        if (fromIdx == 7)  _castlingRights &= ~4;
        if (fromIdx == 0)  _castlingRights &= ~8;
    }
    if (toIdx == 63) _castlingRights &= ~1;
    if (toIdx == 56) _castlingRights &= ~2;
    if (toIdx == 7)  _castlingRights &= ~4;
    if (toIdx == 0)  _castlingRights &= ~8;

    // PROMOTION 
    if (piece == Pawn && ((isWhite && toY == 0) || (!isWhite && toY == 7)))
    {
        int playerNum = isWhite ? 0 : 1;
        ChessSquare* dstSq = dynamic_cast<ChessSquare*>(&dst);
        if (dstSq)
            dstSq->setBit(PieceForPlayer(playerNum, Queen));
    }

    endTurn();
}

bool Chess::gameHasAI()
{
    return _gameOptions.AIPlaying;
}

Chess::BoardArray Chess::boardToArray() const
{
    BoardArray board;
    board.fill(0);
    board[64] = _enPassantSquare;
    board[65] = _castlingRights;
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        Bit* b = sq->bit();
        if (b) board[y * 8 + x] = b->gameTag();
    });
    return board;
}

void Chess::addRayMovesForArray(const BoardArray& board,
                                std::vector<BitMove>& moves,
                                int fromX, int fromY,
                                int dx, int dy,
                                ChessPiece piece,
                                bool whiteToMove) const
{
    int x = fromX + dx;
    int y = fromY + dy;
    int from = fromY * 8 + fromX;

    while (x >= 0 && x < 8 && y >= 0 && y < 8)
    {
        int to = y * 8 + x;
        int tag = board[to];

        bool friendly = whiteToMove ? (tag > 0 && tag < 128) : (tag >= 128);
        if (friendly) break;

        moves.push_back(BitMove(from, to, piece));

        if (tag != 0) break;   // enemy capture then stop

        x += dx;
        y += dy;
    }
}


std::vector<BitMove> Chess::generateMovesForArray(const BoardArray& board,
                                                   bool whiteToMove) const
{
    std::vector<BitMove> moves;
    moves.reserve(40);

    for (int idx = 0; idx < 64; idx++)
    {
        int tag = board[idx];
        if (tag == 0) continue;

        bool pieceIsWhite = (tag > 0 && tag < 128);
        if (pieceIsWhite != whiteToMove) continue;

        int piece = tag & 127;   // strip colour bit
        int x = idx % 8;
        int y = idx / 8;
        int from = idx;

        // PAWNS 
        if (piece == Pawn)
        {
            int dir = whiteToMove ? -8 : +8;
            int one = from + dir;
            if (one >= 0 && one < 64 && board[one] == 0)
            {
                moves.push_back(BitMove(from, one, Pawn));
                bool onStart = whiteToMove ? (y == 6) : (y == 1);
                if (onStart)
                {
                    int two = from + 2 * dir;
                    if (two >= 0 && two < 64 && board[two] == 0)
                        moves.push_back(BitMove(from, two, Pawn));
                }
            }
            // captures
            int capL = from + dir - 1;
            int capR = from + dir + 1;
            if (x > 0 && capL >= 0 && capL < 64)
            {
                int ct = board[capL];
                bool enemy = whiteToMove ? (ct >= 128) : (ct > 0 && ct < 128);
                if (enemy) moves.push_back(BitMove(from, capL, Pawn));
            }
            if (x < 7 && capR >= 0 && capR < 64)
            {
                int ct = board[capR];
                bool enemy = whiteToMove ? (ct >= 128) : (ct > 0 && ct < 128);
                if (enemy) moves.push_back(BitMove(from, capR, Pawn));
            }

            // EN PASSANT
            int epSq = board[64];
            if (epSq != -1)
            {
                int epX = epSq % 8;
                int captureRow = whiteToMove ? epSq / 8 + 1 : epSq / 8 - 1;
                if (y == captureRow && std::abs(x - epX) == 1)
                    moves.push_back(BitMove(from, epSq, Pawn));
            }
        }

        // KING 
        if (piece == Knight)
        {
            const int dx[8] = { 1, 2,  2,  1, -1, -2, -2, -1 };
            const int dy[8] = { 2, 1, -1, -2, -2, -1,  1,  2 };
            for (int i = 0; i < 8; i++)
            {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;
                int to = ny * 8 + nx;
                int tt = board[to];
                bool friendly = whiteToMove ? (tt > 0 && tt < 128) : (tt >= 128);
                if (!friendly) moves.push_back(BitMove(from, to, Knight));
            }
        }

        if (piece == King)
        {
            for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++)
            {
                if (ox == 0 && oy == 0) continue;
                int nx = x + ox, ny = y + oy;
                if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;
                int to = ny * 8 + nx;
                int tt = board[to];
                bool friendly = whiteToMove ? (tt > 0 && tt < 128) : (tt >= 128);
                if (!friendly) moves.push_back(BitMove(from, to, King));
            }

            // Castling 
            int castle = board[65];
            if (whiteToMove)
            {
                if ((castle & 1) && board[61] == 0 && board[62] == 0)
                    moves.push_back(BitMove(60, 62, King));
                if ((castle & 2) && board[57] == 0 && board[58] == 0 && board[59] == 0)
                    moves.push_back(BitMove(60, 58, King));
            }
            else
            {
                if ((castle & 4) && board[5] == 0 && board[6] == 0)
                    moves.push_back(BitMove(4, 6, King));
                if ((castle & 8) && board[1] == 0 && board[2] == 0 && board[3] == 0)
                    moves.push_back(BitMove(4, 2, King));
            }
        }

        //ROOK 
        if (piece == Rook)
        {
            addRayMovesForArray(board, moves, x, y,  1,  0, Rook, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1,  0, Rook, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  0,  1, Rook, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  0, -1, Rook, whiteToMove);
        }

        //BISHOP 
        if (piece == Bishop)
        {
            addRayMovesForArray(board, moves, x, y,  1,  1, Bishop, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1,  1, Bishop, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  1, -1, Bishop, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1, -1, Bishop, whiteToMove);
        }

        // QUEEN 
        if (piece == Queen)
        {
            addRayMovesForArray(board, moves, x, y,  1,  0, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1,  0, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  0,  1, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  0, -1, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  1,  1, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1,  1, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y,  1, -1, Queen, whiteToMove);
            addRayMovesForArray(board, moves, x, y, -1, -1, Queen, whiteToMove);
        }
    }

    return moves;
}
// evaluateArray – static evaluation in centipawns 

int Chess::evaluateArray(const BoardArray& board) const
{
    const int* pst[7] = { nullptr, pawnPST, knightPST, bishopPST,
                           rookPST, queenPST, kingPST };
    const int values[7] = { 0, PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE,
                              ROOK_VALUE, QUEEN_VALUE, KING_VALUE };

    int score = 0;
    for (int idx = 0; idx < 64; idx++)
    {
        int tag = board[idx];
        if (tag == 0) continue;

        bool isWhite = (tag > 0 && tag < 128);
        int pieceType = tag & 127;
        if (pieceType < 1 || pieceType > 6) continue;

        int material = values[pieceType];

        int pstIdx = isWhite ? idx : ((7 - idx / 8) * 8 + idx % 8);
        int positional = pst[pieceType][pstIdx];

        if (isWhite)
            score += material + positional;
        else
            score -= material + positional;
    }
    return score;
}

// negamax with alpha-beta pruning
int Chess::negamax(BoardArray board, int depth, int alpha, int beta, bool whiteToMove)
{
    if (depth == 0)
    {

        int raw = evaluateArray(board);
        return whiteToMove ? raw : -raw;
    }

    auto moves = generateMovesForArray(board, whiteToMove);

    if (moves.empty())
    {
        return -50000 - depth;  
    }

    int best = -1000000;
    for (const BitMove& m : moves)
    {
        BoardArray child = board;
        int fromX = m.from % 8, fromY = m.from / 8;
        int toX   = m.to   % 8, toY   = m.to   / 8;

        // Basic move
        child[m.to]   = child[m.from];
        child[m.from] = 0;

        // EN PASSANT/ remove captured pawn behind target square
        if (m.piece == Pawn && fromX != toX && board[m.to] == 0)
        {
            int capSq = m.to + (whiteToMove ? 8 : -8);
            child[capSq] = 0;
        }

        // Update EP target for child position
        child[64] = -1;
        if (m.piece == Pawn && std::abs(m.to - m.from) == 16)
            child[64] = (m.from + m.to) / 2;

        // CASTLING also move rook in child board
        if (m.piece == King && std::abs(toX - fromX) == 2)
        {
            int rSrc = fromY * 8 + ((toX == 6) ? 7 : 0);
            int rDst = fromY * 8 + ((toX == 6) ? 5 : 3);
            child[rDst] = child[rSrc];
            child[rSrc] = 0;
        }

        // Update castling rights in child
        int cr = board[65];
        if (m.piece == King) { if (whiteToMove) cr &= ~3; else cr &= ~12; }
        if (m.piece == Rook)
        {
            if (m.from == 63) cr &= ~1; if (m.from == 56) cr &= ~2;
            if (m.from == 7)  cr &= ~4; if (m.from == 0)  cr &= ~8;
        }
        if (m.to == 63) cr &= ~1; if (m.to == 56) cr &= ~2;
        if (m.to == 7)  cr &= ~4; if (m.to == 0)  cr &= ~8;
        child[65] = cr;

        // PROMOTION replace pawn with queen
        if (m.piece == Pawn && ((whiteToMove && toY == 0) || (!whiteToMove && toY == 7)))
            child[m.to] = whiteToMove ? (int)Queen : (128 + (int)Queen);

        int score = -negamax(child, depth - 1, -beta, -alpha, !whiteToMove);

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return best;
}

// executeAIMove – apply the chosen move on the actual GUI grid
void Chess::executeAIMove(const BitMove& move)
{
    int fromX = move.from % 8, fromY = move.from / 8;
    int toX   = move.to   % 8, toY   = move.to   / 8;

    ChessSquare* src = _grid->getSquare(fromX, fromY);
    ChessSquare* dst = _grid->getSquare(toX,   toY);
    if (!src || !dst) return;

    Bit* bit = src->takeBit();   // remove without deleting
    if (!bit) return;

    // setBit handles capture deletes any existing piece at dst, then places bit
    dst->setBit(bit);

    bitMovedFromTo(*bit, *src, *dst);   // calls endTurn()
}

// updateAI – called every frame while it is the AI player's turn
void Chess::updateAI()
{
    bool whiteToMove = (getCurrentPlayer()->playerNumber() == 0);
    int depth = (_gameOptions.AIMAXDepth > 0) ? _gameOptions.AIMAXDepth : 4;

    BoardArray board = boardToArray();
    auto moves = generateMovesForArray(board, whiteToMove);
    if (moves.empty()) return;

    int bestScore = -1000000;
    BitMove bestMove = moves[0];

    for (const BitMove& m : moves)
    {
        BoardArray child = board;
        child[m.to]   = child[m.from];
        child[m.from] = 0;

        // Score from root mover's perspective
        int score = -negamax(child, depth - 1, -1000000, 1000000, !whiteToMove);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove  = m;
        }
    }

    executeAIMove(bestMove);
}
