#pragma once

class piece {
public:
	piece(char name, unsigned int col);
	
	// functions
	void calcRandPiece();

	//int xLoc, yLoc;
	//bool falling; // falling means its falling
	bool active; // active means you can move it back and forth
	unsigned int color;
	int rotNum;
	int xLoc;
	int yLoc;
	char pieceName;
};