#pragma once

#include "chess.hpp"
#include <climits>

using namespace chess;

const int PAWN_VALUE = 1;
const int KNIGHT_VALUE = 3;
const int BISHOP_VALUE = 3;
const int ROOK_VALUE = 5;
const int QUEEN_VALUE = 9;
const int KING_VALUE = INT_MAX;

int evaluate(const Board& board);
