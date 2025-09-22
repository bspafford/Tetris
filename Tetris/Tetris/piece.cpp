#include "piece.h"

piece::piece(char name, unsigned int col) {
	active = true;
	color = col;
	pieceName = name;
}