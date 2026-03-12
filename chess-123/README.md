# Chess AI – CMPM 123

## Overview

A two-player chess game with an optional negamax AI opponent built on top of an ImGui / DirectX 11 rendering framework.

## How to Play

| Mode | Button |
|------|--------|
| Two human players | **Start Chess (2 Player)** |
| Human (White) vs AI (Black) | **Start Chess vs AI** |

Drag and drop pieces to move. The AI responds automatically after each human move.

---

## AI Design

### Algorithm
**Negamax with alpha-beta pruning** – a standard variant of minimax that simplifies the implementation by always maximising from the current player's perspective and negating the score when recursing.

Reference: [Wikipedia – Negamax](https://en.wikipedia.org/wiki/Negamax)

### Search Depth
The AI searches **4 plies** (half-moves) deep by default.
With alpha-beta pruning this is equivalent in quality to roughly 6–7 plies of pure minimax in well-ordered positions, and typically responds in under a second.

### Evaluation Function (`evaluate.h`)
The static board evaluation sums, for every piece on the board:

```
score += material_value + piece_square_table_bonus
```

| Piece  | Material value |
|--------|---------------|
| Pawn   | 100 cp |
| Knight | 320 cp |
| Bishop | 330 cp |
| Rook   | 500 cp |
| Queen  | 900 cp |
| King   | 20 000 cp |

**Piece-square tables (PSTs)** reward positional play:
- Pawns are encouraged to advance and control the centre.
- Knights gain bonuses for central squares and lose penalties for rim squares.
- Bishops prefer open diagonals.
- Rooks earn a bonus on the 7th rank.
- The king is penalised for exposure in the middle game and rewarded for staying behind pawns in the corner.

Score is computed from White's perspective (positive = White is better). Negamax then automatically handles flipping the perspective for Black.

### Alpha-Beta Pruning
Moves that cannot improve the current best score are pruned (beta cut-off), dramatically reducing the number of nodes evaluated compared to plain minimax.

---

## Challenges

1. **Make/unmake without corrupting the visual board** – The existing `BitHolder::setBit()` deletes the occupying piece before placing a new one. To keep the GUI board intact during the AI search the search runs on a *lightweight `std::array<int,64>` snapshot* of the board; the actual grid is only modified once when the best move is committed. A new `BitHolder::takeBit()` method was added to safely detach a piece from its square without deleting it.

2. **Coordinate system** – The internal grid uses `(x=0, y=0) = a8` (top-left / black's back rank). PSTs written from White's perspective are mirrored vertically for Black pieces (`mirrorIdx = (7 - y) * 8 + x`).

3. **No check / checkmate detection** – The move generator is pseudo-legal (it does not verify whether a move leaves the king in check). The AI treats positions with no legal moves as a loss, which approximates checkmate / stalemate detection well enough for a class project.

4. **Single-threaded search** – The search blocks the render loop for the duration of the search. At depth 4 with alpha-beta pruning this is imperceptible for most positions.

---

## How Well the AI Plays

At depth 4 the AI:
- Consistently captures hanging pieces.
- Avoids obvious blunders (leaving pieces en prise).
- Develops pieces toward the centre.
- Demonstrates basic tactical awareness (winning material exchanges, threatening forks).

It is roughly equivalent to a casual beginner-to-intermediate human player. It fully handles castling, en passant, and promotion (auto-queen), so it can castle for king safety, exploit en passant opportunities, and queen pawns when they reach the back rank.

---

## File Summary

| File | Purpose |
|------|---------|
| `classes/Chess.h` / `Chess.cpp` | Game logic, move generation, negamax AI |
| `classes/evaluate.h` | Piece values and piece-square tables |
| `classes/BitHolder.h` / `BitHolder.cpp` | Added `takeBit()` for safe piece removal |
| `Application.cpp` | Added "Start Chess vs AI" button |
