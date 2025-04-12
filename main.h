#pragma once

#include "piece.h"
#include "ghostPiece.h"

static class main {
public:
	main();

	// functions
	static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
	static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void clear_screen(unsigned int color);
	static void draw_rect_in_pixels(int x0, int y0, int x1, int y1, unsigned int color, unsigned int outlineCol, int outlineSize);
	static int clamp(int min, int val, int max);
	static void spawnPiece();
	static void Update();
	static void Start();
	static void moveActivePiece(int dir);
	static void rotatePiece(int rotDir);
	static int** calcPiece(unsigned int &pieceCol);
	static void calcNextRandPiece();
	static void setAllPiecesInactive();
	static void calcRows(); // removes row if full of pieces
	static void dropPiece();
	static void spawnGhostBlocks(int moveDir);
	static void destroyGhostBlocks();

	static const int screenWidth = 815; // 525
	static const int screenHeight = 1035; // 105
	static const int tileSize = 50;


};

// to do:
// rotation glitch (other object clipping)
// score
// holding peices
// show next piece