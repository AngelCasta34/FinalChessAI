#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <iostream>

enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class BitboardElement {
  public:
    // Constructors
    BitboardElement()
        : _data(0) { }
    BitboardElement(uint64_t data)
        : _data(data) { }

    // Getters and Setters
    uint64_t getData() const { return _data; }
    void setData(uint64_t data) { _data = data; }

    // Method to loop through each bit in the element and perform an operation on it.
    template <typename Func>
    void forEachBit(Func func) const {
        if (_data != 0) {
            uint64_t tempData = _data;
            while (tempData) {
                int index = bitScanForward(tempData);
                func(index);
                tempData &= tempData - 1;
            }
        }
    }

    BitboardElement& operator|=(const uint64_t other) {
        _data |= other;
        return *this;
    }

    void printBitboard() {
        std::cout << "\n  a b c d e f g h\n";
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << (rank + 1) << " ";
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                if (_data & (1ULL << square)) {
                    std::cout << "X ";
                } else {
                    std::cout << ". ";
                }
            }
            std::cout << (rank + 1) << "\n";
            std::cout << std::flush;
        }
        std::cout << "  a b c d e f g h\n";
        std::cout << std::flush;
    }

private:
    uint64_t    _data;

    inline int bitScanForward(uint64_t bb) const {
#if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
#else
        return __builtin_ffsll(bb) - 1;
#endif
    };

};

struct BitMove {
    uint8_t from;
    uint8_t to;
    uint8_t piece;
    
    BitMove(int from, int to, ChessPiece piece)
        : from(from), to(to), piece(piece) { }
        
    BitMove() : from(0), to(0), piece(NoPiece) { }
    
    bool operator==(const BitMove& other) const {
        return from == other.from && 
               to == other.to && 
               piece == other.piece;
    }
};

class BitBoard {
public:
    BitBoard() : _bb(0) {}
    explicit BitBoard(uint64_t data) : _bb(data) {}

    void clear() { _bb = 0; }
    uint64_t data() const { return _bb; }

    void set(int idx) { _bb |= (1ULL << idx); }
    void reset(int idx) { _bb &= ~(1ULL << idx); }
    bool get(int idx) const { return (_bb & (1ULL << idx)) != 0; }

    BitBoard operator|(const BitBoard& other) const { return BitBoard(_bb | other._bb); }
    BitBoard operator&(const BitBoard& other) const { return BitBoard(_bb & other._bb); }
    BitBoard operator~() const { return BitBoard(~_bb); }

    // iterate set bits using your BitboardElement helper
    struct Iterator {
        uint64_t bb;
        Iterator(uint64_t b) : bb(b) {}
        bool operator!=(const Iterator& other) const { return bb != other.bb; }
        int operator*() const {
#if defined(_MSC_VER) && !defined(__clang__)
            unsigned long index;
            _BitScanForward64(&index, bb);
            return (int)index;
#else
            return __builtin_ffsll(bb) - 1;
#endif
        }
        Iterator& operator++() { bb &= bb - 1; return *this; }
    };

    Iterator begin() const { return Iterator(_bb); }
    Iterator end() const { return Iterator(0); }

private:
    uint64_t _bb;
};
