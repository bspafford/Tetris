#include <windows.h>
#include <iostream>
#include <chrono>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>

using namespace std;

#include "main.h"

static piece* activePiece;
static HDC hdc;

static void* memory;
static BITMAPINFO bitmap_info;

static bool running;


float fallspeed = 1/3.f; // seconds till fall again
float fallTimer = fallspeed;

// fps
int fps;
float timeDiffSec;

static char nextPiece;

static piece* boardList[20][10];
static ghostPiece* ghostList[20][10];
static piece* nextPieceList[4][4];

main::main() {
	
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	main::WinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

int WINAPI main::WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	//AllocConsole();
	//FILE* fDummy;
	//freopen_s(&fDummy, "CONOUT$", "w", stdout);

	running = true;
	const wchar_t* CLASS_NAME = L"tetris";

	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = windowProc;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND window = CreateWindow(wc.lpszClassName, L"Tetris", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, screenWidth, screenHeight, 0, 0, hInstance, 0);
	hdc = GetDC(window);

	Start();

	// fps
	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point currentTime;

	MSG msg;
	while (running) {
		while (PeekMessage(&msg, window, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// fps
		currentTime = std::chrono::steady_clock::now();
		float timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - lastTime).count();
		timeDiffSec = timeDiff / 1000000000;
		lastTime = currentTime;
		fps = round(1 / timeDiffSec);

		Update();
	}

	return 0;
}

LRESULT CALLBACK main::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);	

		EndPaint(hwnd, &ps);
		break;
	}
	case WM_SIZE: {
		RECT rect;
		GetClientRect(hwnd, &rect);
		//screenWidth = rect.right - rect.left;
		//screenHeight = rect.bottom - rect.top;

		int size = screenWidth * screenHeight * sizeof(unsigned int);
		if (memory) VirtualFree(memory, 0, MEM_RELEASE);
		memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
		bitmap_info.bmiHeader.biWidth = screenWidth;
		bitmap_info.bmiHeader.biHeight = screenHeight;
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		break;
	}
	case WM_DESTROY: {
		running = false;
		PostQuitMessage(0);
		return 0;
	}
	case WM_KEYDOWN: {
		std::cout << wParam << "\n";
		if (wParam == 66 && !activePiece) { // b
			spawnPiece();
		} else if (wParam == 37) { // left
			moveActivePiece(-1);
		} else if (wParam == 39) { // right
			moveActivePiece(1);
		} else if (wParam == 40) {// down
			// shmove faster
		} else if (wParam == 90) {
			rotatePiece(1);// z
		} else if (wParam == 88) { // x
			rotatePiece(-1);
			/*} else if (wParam == 67) { // c
			for (int i = 0; i < 10; i++) {
				activePiece = new piece('l', 0x00ff00);
				boardList[19][i] = activePiece;
			}
			for (int i = 0; i < 9; i++) {
				activePiece = new piece('l', 0x00ff00);
				boardList[18][i] = activePiece;
			}
			setAllPiecesInactive();
			*/
		} else if (wParam == 38) { // space
			destroyGhostBlocks();
			dropPiece();
		}
		break;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void main::dropPiece() {
	bool canFall = true;
	while (canFall) {
		for (int y = 0; y < size(boardList); y++) {
			for (int x = 0; x < size(boardList[y]); x++) {
				if (boardList[y][x] != NULL && boardList[y][x]->active && (y + 1 >= 20 || (boardList[y + 1][x] != NULL && !boardList[y + 1][x]->active))) {
					calcRows();
					setAllPiecesInactive();
					canFall = false;
					activePiece = NULL;
				}
			}
		}

		for (int y = size(boardList) - 1; y >= 0; y--) {
			for (int x = 0; x < size(boardList[y]); x++) {
				if (boardList[y][x] != NULL && boardList[y][x]->active) {
					if (canFall) {
						boardList[y + 1][x] = boardList[y][x];
						boardList[y][x]->yLoc += 1;
						boardList[y][x] = NULL;
					}
				}
			}
		}
	}

	spawnPiece();
}

void main::spawnGhostBlocks(int moveDir) {
	destroyGhostBlocks();
	
	// spawn ghost pieces
	for (int y = 0; y < size(boardList); y++) {
		for (int x = 0; x < size(boardList[y]); x++) {
			if (boardList[y][x] && boardList[y][x]->active) {
				unsigned int color = boardList[y][x]->color;
				int red = ((color >> 16) & 0xFF) / 5;
				int green = ((color >> 8) & 0xFF) / 5;
				int blue = ((color) & 0xFF) / 5;

				std::stringstream ss;
				ss << "0x" << std::hex << (red << 16 | green << 8 | blue);
				unsigned int newCol = std::stoul(ss.str(), nullptr, 0);

				ghostList[y][x + moveDir] = new ghostPiece(newCol);
			}
		}
	}


	// move them to the bottom
	bool canFall = true;
	//for (int i = 0; i < 4; i++) {
	while (canFall) {
		for (int y = 0; y < size(ghostList); y++) {
			for (int x = 0; x < size(ghostList[y]); x++) {
				if (ghostList[y][x] && (y + 1 >= 20 || (boardList[y + 1][x] != NULL && !boardList[y + 1][x]->active))) {
					canFall = false;
				}
			}
		}
		for (int y = size(ghostList) - 1; y >= 0; y--) {
			for (int x = 0; x < size(ghostList[y]); x++) {
				if (ghostList[y][x] != NULL) {
					if (canFall) {
						ghostList[y + 1][x] = ghostList[y][x];
						ghostList[y][x] = NULL;
					}
				}
			}
		}
	}
	
}

void main::destroyGhostBlocks() {
	for (int y = 0; y < size(ghostList); y++) {
		for (int x = 0; x < size(ghostList[y]); x++) {
			ghostList[y][x] = NULL;
		}
	}
}

void main::spawnPiece() {
	unsigned int pieceCol;
	int** pieceLocList = calcPiece(pieceCol);

	for (int y = 0; y < 4; y++) {
		activePiece = new piece(nextPiece, pieceCol);
		boardList[pieceLocList[y][1]][pieceLocList[y][0]] = activePiece;
		activePiece->rotNum = 0;
		activePiece->xLoc = 0;
		activePiece->yLoc = 0;
	}
	spawnGhostBlocks(0);
	std::cout << "current piece: " << nextPiece << "\n";
	calcNextRandPiece();
	std::cout << "next piece: " << nextPiece << "\n";

	// remove all stuff in nextPieceList
	for (int y = 0; y < size(nextPieceList); y++) {
		for (int x = 0; x < size(nextPieceList[y]); x++) {
			nextPieceList[y][x] = NULL;
		}
	}

	pieceLocList = calcPiece(pieceCol);
	for (int i = 0; i < 4; i++) {
		nextPieceList[pieceLocList[i][1]][pieceLocList[i][0]] = new piece(nextPiece, pieceCol);

	}
}

int** main::calcPiece(unsigned int &pieceCol) { // color is messed up

	int I[4][2] = { {1, 0}, {1, 1}, {1, 2}, {1, 3} };
	int O[4][2] = { {0, 0}, {0, 1}, {1, 0}, {1, 1} };
	int T[4][2] = { {1, 0}, {0, 1}, {1, 1}, {2, 1} };
	int S[4][2] = { {1, 0}, {2, 0}, {0, 1}, {1, 1} };
	int Z[4][2] = { {0, 0}, {1, 0}, {1, 1}, {2, 1} };
	int J[4][2] = { {1, 0}, {1, 1}, {1, 2}, {0, 2} };
	int L[4][2] = { {0, 0}, {1, 0}, {1, 1}, {1, 2} };

	unsigned int colorList[7] = { 0x00ffff, 0xffff00, 0xff00ff, 0x00ff00, 0xff0000, 0x0000ff, 0xffA500 };
	unsigned int color = 0x000000;
	int** pieceLocList = new int*[4];

	switch (nextPiece) {
	case 'I':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = I[y][x];
				pieceCol = 0x00ffff;
			}
		}
		break;
	case 'O':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = O[y][x];
				pieceCol = 0xffff00;
			}
		}
		break;
	case 'T':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = T[y][x];
				pieceCol = 0xff00ff;
			}
		}
		break;
	case 'S':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = S[y][x];
				pieceCol = 0x00ff00;
			}
		}
		break;
	case 'Z':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = Z[y][x];
				pieceCol = 0xff0000;
			}
		}
		break;
	case 'J':
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = J[y][x];
				pieceCol = 0x0000ff;
			}
		}
		break;
	default:
		for (int y = 0; y < 4; y++) {
			pieceLocList[y] = new int[2];
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = L[y][x];
				pieceCol = 0xffA500;
			}
		}
		break;
	}

	return pieceLocList;
}

void main::calcNextRandPiece() {
	switch (rand() % 7) {
	case 0:
		nextPiece = 'I';
		break;
	case 1:
		nextPiece = 'O';
		break;
	case 2:
		nextPiece = 'T';
		break;
	case 3:
		nextPiece = 'S';
		break;
	case 4:
		nextPiece = 'Z';
		break;
	case 5:
		nextPiece = 'J';
		break;
	default:
		nextPiece = 'L';
		break;
	}
}

void main::Start() {
	
	srand(time(NULL));

	calcNextRandPiece();
}

void main::Update() {

	clear_screen(0x101010);

	bool canFall = true;

	if (activePiece != NULL) {
		fallTimer -= timeDiffSec;
		if (fallTimer <= 0) {
			// move active piece down a space
			// go through the y part of the list backwars and pulldown any piece that is active // cant go through the list forwards because then two active pieces might overlap

			// loop through and see if active piece is touching the ground or another piece if it is then set all the pieces inactive
			for (int y = 0; y < size(boardList); y++) {
				for (int x = 0; x < size(boardList[y]); x++) {
					if (boardList[y][x] != NULL && boardList[y][x]->active && (y + 1 >= 20 || (boardList[y + 1][x] != NULL && !boardList[y + 1][x]->active))) {
						calcRows();
						setAllPiecesInactive();
						canFall = false;
						activePiece = NULL;
						// spawn next piece
						spawnPiece();
					}
				}
			}

			for (int y = size(boardList) - 1; y >= 0; y--) {
				for (int x = 0; x < size(boardList[y]); x++) {
					if (boardList[y][x] != NULL && boardList[y][x]->active) {
						if (canFall) {
							boardList[y + 1][x] = boardList[y][x];
							boardList[y][x]->yLoc += 1;
							boardList[y][x] = NULL;
						}
					}
				}
			}
			fallTimer = fallspeed;
		}
	}

	// draw non playable field
	for (int y = 0; y < size(boardList) + 1; y++) {
		for (int x = 0; x < 6; x++) {
			if ((y <= 1 || y >= 6) || (x <= 0 || x >= 5))
				draw_rect_in_pixels(x * tileSize + (10 * tileSize), screenHeight - (y * tileSize + tileSize), x * tileSize + tileSize + (10 * tileSize), screenHeight - (y * tileSize), 0x000000, 0xffffff, 2);
		}
	}

	// draws next piece
	for (int y = 0; y < size(nextPieceList); y++) {
		for (int x = 0; x < size(nextPieceList[y]); x++) {
			if (nextPieceList[y][x] != NULL) {
				draw_rect_in_pixels(x * tileSize + (12 * tileSize), screenHeight - ((y + 3) * tileSize), x * tileSize + tileSize + (12 * tileSize), screenHeight - ((y + 2) * tileSize), nextPieceList[y][x]->color, 0xffffff, 2);
			}
		}
	}

	// draw pieces
	for (int y = 0; y < size(boardList); y++) {
		for (int x = 0; x < size(boardList[y]); x++) {
			piece* currentSpot = boardList[y][x];
			if (ghostList[y][x] != NULL) {
				//draw ghost piece
				draw_rect_in_pixels(x * tileSize, screenHeight - (y * tileSize + tileSize), x * tileSize + tileSize, screenHeight - (y * tileSize), ghostList[y][x]->color, 0x333333, 2);
			}
			
			if (currentSpot != NULL) {
					
				// draw piece
				draw_rect_in_pixels(x * tileSize, screenHeight - (y * tileSize + tileSize), x * tileSize + tileSize, screenHeight - (y * tileSize), currentSpot->color, 0xffffff, 2);
			}
		}
	}

	// render
	StretchDIBits(hdc, 0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, memory, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

void main::calcRows() {
	// more optimized of doing this would be to have a list of rows that the block landed then only cycle through those

	for (int y = 0; y < size(boardList); y++) {
		bool fullRow = true;
		for (int x = 0; x < size(boardList[y]); x++) {
			if (boardList[y][x] == NULL) {
				fullRow = false;
				break;
			}
		}

		if (fullRow) {
			// delete rows
			for (int x = 0; x < size(boardList[y]); x++) {
				boardList[y][x] = NULL;
			}

			// move all pieces down starting at y then go down
			for (int i = y; i >= 0; i--) {
				for (int x = 0; x < size(boardList[i]); x++) {
					if (i > 0)
						boardList[i][x] = boardList[i - 1][x];
				}
			}
		}
	}
}

void main::setAllPiecesInactive() {
	for (int y = 0; y < size(boardList); y++) {
		for (int x = 0; x < size(boardList[y]); x++) {
			if (boardList[y][x] != NULL) {
				boardList[y][x]->active = false;
			}
		}
	}
}
void main::moveActivePiece(int dir) {
	if (!activePiece)
		return;

	// if dir is positive(moving right) then go through the x backwards
	// else go through x normally
	bool canMoveRight = true;
	bool canMoveLeft = true;

	// look at all active pieces, if one is touching another piece or if too far on the edge then dont move any over
	for (int y = 0; y < size(boardList); y++) {
		for (int x = 0; x < size(boardList[y]); x++) {
			if (boardList[y][x] && boardList[y][x]->active) {
				if (dir > 0 && (x + 1 > 9 || (boardList[y][x + 1] != NULL && !boardList[y][x + 1]->active))) {
					canMoveRight = false;
				}
				else if (dir < 0 && (x - 1 < 0 || (boardList[y][x - 1] != NULL && !boardList[y][x - 1]->active))) {
					canMoveLeft = false;
				}
			}
		}
	}

	
	if (dir > 0 && canMoveRight) {
		// ghost block spawn here
		spawnGhostBlocks(dir);
		for (int y = 0; y < size(boardList); y++) {
			for (int x = size(boardList[y]) - 1; x >= 0; x--) {
				if (boardList[y][x] != NULL && boardList[y][x+1] == NULL && boardList[y][x]->active && x + 1 <= 9) {
					
					boardList[y][x + 1] = boardList[y][x];
					boardList[y][x]->xLoc += 1;
					boardList[y][x] = NULL;
				}
			}
		}
		
	} else if (dir < 0 && canMoveLeft) {
		// ghost block spawn here
		spawnGhostBlocks(dir);
		for (int y = 0; y < size(boardList); y++) {
			for (int x = 0; x < size(boardList[y]); x++) {
				if (boardList[y][x] != NULL && boardList[y][x - 1] == NULL && boardList[y][x]->active && x - 1 >= 0) {
					boardList[y][x - 1] = boardList[y][x];

					boardList[y][x]->xLoc -= 1;

					boardList[y][x] = NULL;
				}
			}
		}
	}
}

void main::rotatePiece(int rotDir) {
	
	if (!activePiece)
		return;

	// 4x4
	int IRot[4][4][2] = { { {1, 0}, {1, 1}, {1, 2}, {1, 3} },
						  { {0, 1}, {1, 1}, {2, 1}, {3, 1} }, 
						  { {2, 0}, {2, 1}, {2, 2}, {2, 3} }, 
						  { {0, 2}, {1, 2}, {2, 2}, {3, 2} }};

	// 2x2
	int ORot[4][4][2] = { { {0, 0}, {0, 1}, {1, 0}, {1, 1} },
						  { {0, 0}, {0, 1}, {1, 0}, {1, 1} },
						  { {0, 0}, {0, 1}, {1, 0}, {1, 1} },
						  { {0, 0}, {0, 1}, {1, 0}, {1, 1} } };

	// 3x3
	int TRot[4][4][2] = { { {1, 0}, {0, 1}, {1, 1}, {2, 1} },
						  { {1, 0}, {1, 1}, {1, 2}, {2, 1} },
						  { {0, 1}, {1, 1}, {2, 1}, {1, 2} },
						  { {1, 0}, {0, 1}, {1, 1}, {1, 2} } };

	// 3x3
	int SRot[4][4][2] = { { {1, 0}, {2, 0}, {0, 1}, {1, 1} },
						  { {1, 0}, {1, 1}, {2, 1}, {2, 2} },
						  { {1, 1}, {2, 1}, {0, 2}, {1, 2} },
						  { {0, 0}, {0, 1}, {1, 1}, {1, 2} } };

	// 3x3
	int ZRot[4][4][2] = { { {0, 0}, {1, 0}, {1, 1}, {2, 1} },
						  { {2, 0}, {1, 1}, {2, 1}, {1, 2} },
						  { {0, 1}, {1, 1}, {1, 2}, {2, 2} },
						  { {1, 0}, {0, 1}, {1, 1}, {0, 2} } };

	// 3x3
	int JRot[4][4][2] = { { {1, 0}, {1, 1}, {1, 2}, {0, 2} },
						  { {0, 0}, {0, 1}, {1, 1}, {2, 1} },
						  { {1, 0}, {2, 0}, {1, 1}, {1, 2} },
						  { {0, 1}, {1, 1}, {2, 1}, {2, 2} } };

	// 3x3
	int LRot[4][4][2] = { { {0, 0}, {1, 0}, {1, 1}, {1, 2} },
						  { {0, 1}, {1, 1}, {2, 1}, {2, 0} },
						  { {1, 0}, {1, 1}, {1, 2}, {2, 2} },
						  { {0, 1}, {1, 1}, {2, 1}, {0, 2} } };
						 

	// problems: can rotate outside of screen
	// can rotate pieces over other pieces

	// find top left of piece? since the I is a 4x4 then you would find the top left of that square and then add the location of that (IRot1 + location)
	char pieceName = activePiece->pieceName;
	unsigned int color;
	int xLoc = activePiece->xLoc;
	int yLoc = activePiece->yLoc;
	int rotNum = activePiece->rotNum + rotDir;

	if (xLoc < 0) xLoc = 0;
	if (xLoc > 7 && pieceName != 'I' && pieceName != 'O') xLoc = 7;
	if (xLoc > 6 && pieceName == 'I') xLoc = 6;

	int pieceLocList[4][2];

	if (rotNum > 3)
		rotNum = 0;
	else if (rotNum < 0)
		rotNum = 3;

	// then delete active piece
	for (int y = activePiece->yLoc; y < activePiece->yLoc + 4; y++) {
		for (int x = activePiece->xLoc; x < activePiece->xLoc + 4; x++) {
			if (boardList[y][x] != NULL && boardList[y][x]->active) {
				boardList[y][x] = NULL;

				if (activePiece == boardList[y][x])
					activePiece = NULL;
			}
		}
	}

	unsigned int colorList[7] = { 0x00ffff, 0xffff00, 0xff00ff, 0x00ff00, 0xff0000, 0x0000ff, 0xffA500 };

	switch (pieceName) {
	case 'I':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = IRot[rotNum][y][x];
				color = 0x00ffff;
			}
		}
		break;
	case 'O':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = ORot[rotNum][y][x];
				color = 0xffff00;
			}
		}
		break;
	case 'T':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = TRot[rotNum][y][x];
				color = 0xff00ff;
			}
		}
		break;
	case 'S':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = SRot[rotNum][y][x];
				color = 0x00ff00;
			}
		}
		break;
	case 'Z':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = ZRot[rotNum][y][x];
				color = 0xff0000;
			}
		}
		break;
	case 'J':
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = JRot[rotNum][y][x];
				color = 0x0000ff;
			}
		}
		break;
	default:
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 2; x++) {
				pieceLocList[y][x] = LRot[rotNum][y][x];
				color = 0xffA500;
			}
		}
		break;
	}

	// spawn in rotated piece
	for (int i = 0; i < 4; i++) {

		int x = pieceLocList[i][0] + xLoc;
		int y = pieceLocList[i][1] + yLoc;

		activePiece = new piece(pieceName, color);
		boardList[y][x] = activePiece;
		activePiece->rotNum = rotNum;
		activePiece->xLoc = xLoc;
		activePiece->yLoc = yLoc;
	}

	spawnGhostBlocks(0);
}

void main::draw_rect_in_pixels(int x0, int y0, int x1, int y1, unsigned int color, unsigned int outlineCol, int outlineSize) {
	x0 = clamp(0, x0, screenWidth);
	x1 = clamp(0, x1, screenWidth);
	y0 = clamp(0, y0, screenHeight);
	y1 = clamp(0, y1, screenHeight);

	for (int y = y0; y < y1; y++) {
		unsigned int* pixel = (unsigned int*)memory + x0 + y * screenWidth;
		for (int x = x0; x < x1; x++) {
			// makes outline on inside of shape
			if ((x >= x0 && x < x0 + outlineSize) || (x <= x1 && x > x1 - 1 - outlineSize) || (y >= y0 && y < y0 + outlineSize) || (y <= y1 && y > y1 - 1 - outlineSize)) {
				*pixel++ = outlineCol;
			}
			else {
				*pixel++ = color;
			}
		}
	}
}

void main::clear_screen(unsigned int color) {
	unsigned int* pixel = (unsigned int*)memory;
	for (int y = 0; y < screenHeight; y++) {
		for (int x = 0; x < screenWidth; x++) {
			*pixel++ = color;
		}
	}
}

int main::clamp(int min, int val, int max) {
	if (val < min) return min;
	if (val > max) return max;
	return val;
}