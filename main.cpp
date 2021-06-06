// TODO: Prevent hackers
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iomanip>
#include <filesystem>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
//#include <SDL_gpu.h>
//#include <SFML/Network.hpp>
//#include <SFML/Graphics.hpp>
#include <algorithm>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <locale>
#include <codecvt>
#ifdef __ANDROID__
#include <android/log.h> //__android_log_print(ANDROID_LOG_VERBOSE, "Ships", "Example number log: %d", number);
#include <jni.h>
#include "vendor/PUGIXML/src/pugixml.hpp"
#else
#include <pugixml.hpp>
#include <filesystem>
namespace fs = std::filesystem;
using namespace std::chrono_literals;
#endif

// NOTE: Remember to uncomment it on every release
//#define RELEASE

#if defined _MSC_VER && defined RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

//240 x 240 (smart watch)
//240 x 320 (QVGA)
//360 x 640 (Galaxy S5)
//640 x 480 (480i - Smallest PC monitor)

#define BG_COLOR 100,100,100,0
#define FIGURE_COLOR 0,255,0,0
#define CELL_SIZE 16
#define MAX_SOCKETS 65534
#define SERVER_PORT 8020

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

int windowWidth = 240;
int windowHeight = 320;
SDL_Point mousePos;
SDL_Point realMousePos;
bool keys[SDL_NUM_SCANCODES];
bool buttons[SDL_BUTTON_X2 + 1];
SDL_Renderer* renderer;

void logOutputCallback(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
	std::cout << message << std::endl;
}

int random(int min, int max)
{
	return min + rand() % ((max + 1) - min);
}

int SDL_QueryTextureF(SDL_Texture* texture, Uint32* format, int* access, float* w, float* h)
{
	int wi, hi;
	int result = SDL_QueryTexture(texture, format, access, &wi, &hi);
	if (w) {
		*w = wi;
	}
	if (h) {
		*h = hi;
	}
	return result;
}

SDL_bool SDL_PointInFRect(const SDL_Point* p, const SDL_FRect* r)
{
	return ((p->x >= r->x) && (p->x < (r->x + r->w)) &&
		(p->y >= r->y) && (p->y < (r->y + r->h))) ? SDL_TRUE : SDL_FALSE;
}

std::ostream& operator<<(std::ostream& os, SDL_FRect r)
{
	os << r.x << " " << r.y << " " << r.w << " " << r.h;
	return os;
}

std::ostream& operator<<(std::ostream& os, SDL_Rect r)
{
	SDL_FRect fR;
	fR.w = r.w;
	fR.h = r.h;
	fR.x = r.x;
	fR.y = r.y;
	os << fR;
	return os;
}

SDL_Texture* renderText(SDL_Texture* previousTexture, TTF_Font* font, SDL_Renderer* renderer, const std::string& text, SDL_Color color)
{
	if (previousTexture) {
		SDL_DestroyTexture(previousTexture);
	}
	SDL_Surface* surface;
	if (text.empty()) {
		surface = TTF_RenderUTF8_Blended(font, " ", color);
	}
	else {
		surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	}
	if (surface) {
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		return texture;
	}
	else {
		return 0;
	}
}

struct Text {
	std::string text;
	SDL_Surface* surface = 0;
	SDL_Texture* t = 0;
	SDL_FRect dstR{};
	bool autoAdjustW = false;
	bool autoAdjustH = false;
	float wMultiplier = 1;
	float hMultiplier = 1;

	~Text()
	{
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
	}

	Text()
	{

	}

	Text(const Text& rightText)
	{
		text = rightText.text;
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (rightText.surface) {
			surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
		}
		if (rightText.t) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
		dstR = rightText.dstR;
		autoAdjustW = rightText.autoAdjustW;
		autoAdjustH = rightText.autoAdjustH;
		wMultiplier = rightText.wMultiplier;
		hMultiplier = rightText.hMultiplier;
	}

	Text& operator=(const Text& rightText)
	{
		text = rightText.text;
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (rightText.surface) {
			surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
		}
		if (rightText.t) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
		dstR = rightText.dstR;
		autoAdjustW = rightText.autoAdjustW;
		autoAdjustH = rightText.autoAdjustH;
		wMultiplier = rightText.wMultiplier;
		hMultiplier = rightText.hMultiplier;
		return *this;
	}

	void setText(SDL_Renderer* renderer, TTF_Font* font, std::string text, SDL_Color c = { 255,255,255 })
	{
		this->text = text;
#if 1 // NOTE: renderText
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (text.empty()) {
			surface = TTF_RenderUTF8_Blended(font, " ", c);
		}
		else {
			surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
		}
		if (surface) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
#endif
		if (autoAdjustW) {
			SDL_QueryTextureF(t, 0, 0, &dstR.w, 0);
		}
		if (autoAdjustH) {
			SDL_QueryTextureF(t, 0, 0, 0, &dstR.h);
		}
		dstR.w *= wMultiplier;
		dstR.h *= hMultiplier;
	}

	void setText(SDL_Renderer* renderer, TTF_Font* font, int value, SDL_Color c = { 255,255,255 })
	{
		setText(renderer, font, std::to_string(value), c);
	}

	void draw(SDL_Renderer* renderer)
	{
		if (t) {
			SDL_RenderCopyF(renderer, t, 0, &dstR);
		}
	}
};

int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;


	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}


int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;


	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {

		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
			x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
			x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
			x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
			x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

int eventWatch(void* userdata, SDL_Event* event)
{
	// WARNING: Be very careful of what you do in the function, as it may run in a different thread
	if (event->type == SDL_APP_TERMINATING || event->type == SDL_APP_WILLENTERBACKGROUND) {

	}
	return 0;
}

struct Cell {
	SDL_Rect r{};
	SDL_Color c{ BG_COLOR };
};

struct Figure {
	SDL_Rect r{};
	SDL_Color c{ FIGURE_COLOR };
};

bool operator==(SDL_Color left, SDL_Color right)
{
	return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

bool operator!=(SDL_Color left, SDL_Color right)
{
	return left.r != right.r || left.g != right.g || left.b != right.b || left.a != right.a;
}

enum class State {
	ShipPlacement,
	WaitForPlayer,
	YourTurnGameplay,
	NotYoursTurnGameplay,
};

struct WaitForPlayer {
	Text waitingText;
	bool first = true;
};

struct YourTurnGameplay {
	std::vector<Cell> board;
	std::vector<Text> hBoardTexts;
	std::vector<Text> vBoardTexts;
	Text text;
};

struct NotYoursTurnGameplay {
	std::vector<Cell> board;
	std::vector<Text> hBoardTexts;
	std::vector<Text> vBoardTexts;
	Text text;
};

struct Client {
	TCPsocket socket = 0;
	State state = State::ShipPlacement;
	int gameId = -1;
	bool playerTurn = false;
	std::vector<Cell> board;
	std::vector<Cell> opponentBoard;
	bool gameEnd = false;
};

int genUniqueGameId(const std::vector<Client>& clients)
{
	int uniqueGameId = 0;
genGameIdBegin:
	for (int i = 0; i < clients.size(); ++i) {
		if (clients[i].gameId == uniqueGameId) {
			++uniqueGameId;
			goto genGameIdBegin;
		}
	}
	return uniqueGameId;
}

#define MAX_LENGTH 1024

int send(TCPsocket socket, std::string msg)
{
	return SDLNet_TCP_Send(socket, msg.c_str(), msg.size() + 1);
}

int receive(TCPsocket socket, std::string& msg)
{
	// TODO: Is it going to work properly ??? What will happen if I will send less than MAX_LENGTH characters?
	char message[MAX_LENGTH]{};
	int result = SDLNet_TCP_Recv(socket, message, MAX_LENGTH);
	msg = message;
	return result;
}

void runServer()
{
	IPaddress ip;
	SDLNet_ResolveHost(&ip, 0, SERVER_PORT); // TODO: What to do if this port is already used ???
	TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
	SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS + 1); // TODO: Do something when the server is full
	SDLNet_TCP_AddSocket(socketSet, serverSocket);
	bool running = true;
	std::vector<Client> clients;
	while (running) {
		if (SDLNet_CheckSockets(socketSet, 0) > 0) {
			if (SDLNet_SocketReady(serverSocket)) {
				clients.push_back(Client());
				clients.back().socket = SDLNet_TCP_Accept(serverSocket);
				SDLNet_TCP_AddSocket(socketSet, clients.back().socket);
				clients.back().board.resize(100);
				clients.back().opponentBoard.resize(100);
			}
			for (int i = 0; i < clients.size(); ++i) {
				if (SDLNet_SocketReady(clients[i].socket)) {
					std::string msg;
					int received = receive(clients[i].socket, msg);
					if (received > 0) {
						if (msg.size() >= 12 && msg.substr(0, 12) == "changeState ") {
							clients[i].state = State::WaitForPlayer;
							std::string indexes = msg.substr(12);
							std::stringstream ss(indexes);
							std::string index;
							while (std::getline(ss, index, ' ')) {
								int cellIndex = std::stoi(index);
								clients[i].board[cellIndex].c = { FIGURE_COLOR };
							}
						}
						else if (msg == "checkStatus") {
							if (clients[i].state == State::YourTurnGameplay) {
								clients[i].playerTurn = true;
								send(clients[i].socket, "Gameplay");
							}
							else {
								bool found = false;
								for (int j = 0; j < clients.size(); ++j) {
									if (j != i) {
										if (clients[j].state == State::WaitForPlayer) {
											clients[i].state = State::YourTurnGameplay;
											clients[i].gameId = genUniqueGameId(clients);
											clients[i].opponentBoard = clients[j].board;
											clients[j].state = State::YourTurnGameplay;
											clients[j].gameId = clients[i].gameId;
											clients[j].opponentBoard = clients[i].board;
											send(clients[i].socket, "Gameplay");
											found = true;
											break;
										}
									}
								}
								if (!found) {
									send(clients[i].socket, "WaitForPlayer");
								}
							}
						}
						else if (msg == "checkTurn") {
							if (clients[i].playerTurn) {
								send(clients[i].socket, "your");
							}
							else {
								send(clients[i].socket, "notYours");
							}
						}
						else if (msg.size() >= 6 && msg.substr(0, 6) == "pick: ") {
							int index = std::stoi(msg.substr(6));
							if (clients[i].opponentBoard[index].c == SDL_Color{ 0,255,0,0 }) {
								clients[i].opponentBoard[index].c = { 255,0,0 };
							}
							else {
								clients[i].opponentBoard[index].c = { 255,255,255 };
							}
							int redCount = 0;
							for (int j = 0; j < clients[i].opponentBoard.size(); ++j) {
								if (clients[i].opponentBoard[j].c == SDL_Color({ 255,0,0 })) {
									++redCount;
								}
							}
							if (redCount == 18) {
								send(clients[i].socket, "gameEnd");
								clients[i].state = State::ShipPlacement;
								for (int j = 0; j < clients.size(); ++j) {
									if (j != i && clients[j].gameId == clients[i].gameId) {
										clients[j].gameEnd = true;
									}
								}
								clients[i].board.clear();
								clients[i].opponentBoard.clear();
								clients[i].board.resize(100);
								clients[i].opponentBoard.resize(100);
							}
							else {
								if (clients[i].opponentBoard[index].c == SDL_Color({ 255,0,0 })) {
									send(clients[i].socket, "hit");
								}
								else if (clients[i].opponentBoard[index].c == SDL_Color({ 255,255,255 })) {
									send(clients[i].socket, "miss");
									clients[i].playerTurn = !clients[i].playerTurn;
									for (int j = 0; j < clients.size(); ++j) {
										if (j != i && clients[j].gameId == clients[i].gameId) {
											clients[j].playerTurn = !clients[j].playerTurn;
										}
									}
								}
							}
						}
						else if (msg == "isMyTurn") {
							if (clients[i].gameEnd) {
								clients[i].gameEnd = false;
								clients[i].state = State::ShipPlacement;
								clients[i].board.clear();
								clients[i].opponentBoard.clear();
								clients[i].board.resize(100);
								clients[i].opponentBoard.resize(100);
								send(clients[i].socket, "gameEnd");
							}
							else if (clients[i].playerTurn) {
								send(clients[i].socket, "yourTurn");
							}
							else {
								std::string indexes;
								for (int j = 0; j < clients.size(); ++j) {
									if (j != i && clients[j].gameId == clients[i].gameId) {
										for (int k = 0; k < clients[j].opponentBoard.size(); ++k) {
											if (clients[j].opponentBoard[k].c != SDL_Color({ BG_COLOR })) {
												indexes += std::to_string(k) + " ";
											}
										}
										if (!indexes.empty() && indexes.back() == ' ') {
											indexes.pop_back();
										}
										break;
									}
								}
								send(clients[i].socket, "notYoursTurn " + indexes);
							}
						}
					}
					else {
						/*
						NOTE:
							An error may have occured, but sometimes you can just ignore it
							It may be good to disconnect sock because it is likely invalid now.
						*/
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	/*
	TODO:
	Attribute:
	Freepik
	*/
	std::srand(std::time(0));
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	SDL_LogSetOutputFunction(logOutputCallback, 0);
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	SDLNet_Init();
#ifndef __ANDROID__
	std::thread serverThread(runServer);
	serverThread.detach();
#endif
	IPaddress ip;
	SDLNet_ResolveHost(&ip, "192.168.1.10", SERVER_PORT); // TODO: Set ip address to public one
	TCPsocket socket = SDLNet_TCP_Open(&ip); // TODO: Error handling
	SDL_GetMouseState(&mousePos.x, &mousePos.y);
	SDL_Window* window = SDL_CreateWindow("Ships", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
	SDL_AddEventWatch(eventWatch, 0);
	bool running = true;
gameBegin:
	State state = State::ShipPlacement;
	std::vector<Text> hBoardTexts;
	std::vector<Text> vBoardTexts;
	std::vector<Cell> board;
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			board.push_back(Cell());
			board.back().r.w = CELL_SIZE;
			board.back().r.h = CELL_SIZE;
			board.back().r.x = j * board.back().r.w + 40;
			board.back().r.y = (i + 1) * board.back().r.h;
			if (i == 0) {
				hBoardTexts.push_back(Text());
				char letter = (char)((int)('A') + j);
				hBoardTexts.back().setText(renderer, robotoF, std::string(1, letter));
				hBoardTexts.back().dstR.w = CELL_SIZE / 1.4;
				hBoardTexts.back().dstR.h = CELL_SIZE / 2.f;
				hBoardTexts.back().dstR.x = board.back().r.x + CELL_SIZE / 2 - hBoardTexts.back().dstR.w / 2;
				hBoardTexts.back().dstR.y = board.back().r.y - hBoardTexts.back().dstR.h;
			}
			if (j == 0) {
				vBoardTexts.push_back(Text());
				char letter = (char)((int)('1') + i);
				if (i == 9) {
					vBoardTexts.back().setText(renderer, robotoF, "10");
				}
				else {
					vBoardTexts.back().setText(renderer, robotoF, std::string(1, letter));
				}
				vBoardTexts.back().dstR.w = CELL_SIZE / 2.f;
				vBoardTexts.back().dstR.h = CELL_SIZE / 1.4;
				vBoardTexts.back().dstR.x = board.back().r.x - vBoardTexts.back().dstR.w;
				vBoardTexts.back().dstR.y = board.back().r.y + CELL_SIZE / 2 - hBoardTexts.back().dstR.h / 2;
			}
		}
	}
	std::vector<Figure> figures;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE;
	figures.back().r.x = 0;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE;
	figures.back().r.x = figures[0].r.x + figures[0].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE * 2;
	figures.back().r.x = figures[1].r.x + figures[1].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE * 2;
	figures.back().r.x = figures[2].r.x + figures[2].r.w * 2 + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE * 3;
	figures.back().r.x = figures[3].r.x + figures[3].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE * 4;
	figures.back().r.x = figures[4].r.x + figures[4].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = CELL_SIZE;
	figures.back().r.h = CELL_SIZE * 5;
	figures.back().r.x = figures[5].r.x + figures[5].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	SDL_Rect rotateBtnR;
	rotateBtnR.w = 32;
	rotateBtnR.h = 32;
	rotateBtnR.x = windowWidth - rotateBtnR.w;
	rotateBtnR.y = windowHeight - rotateBtnR.h;
	SDL_Texture* rotateT = IMG_LoadTexture(renderer, "res/rotate.png");
	int selectedFigureIndex = -1;
	bool rotated = false;
	WaitForPlayer wfp;
	wfp.waitingText.setText(renderer, robotoF, "Searching for players");
	wfp.waitingText.dstR.w = 200;
	wfp.waitingText.dstR.h = 30;
	wfp.waitingText.dstR.x = windowWidth / 2 - wfp.waitingText.dstR.w / 2;
	wfp.waitingText.dstR.y = windowHeight / 2 - wfp.waitingText.dstR.h / 2;
	wfp.waitingText.autoAdjustW = true;
	wfp.waitingText.wMultiplier = 0.25;
	YourTurnGameplay ytg;
	ytg.board = board;
	ytg.hBoardTexts = hBoardTexts;
	ytg.vBoardTexts = vBoardTexts;
	ytg.text.setText(renderer, robotoF, "Your turn");
	ytg.text.dstR.w = 100;
	ytg.text.dstR.h = 25;
	ytg.text.dstR.x = windowWidth / 2 - ytg.text.dstR.w / 2;
	ytg.text.dstR.y = windowHeight - ytg.text.dstR.h - 20;
	NotYoursTurnGameplay nytg;
	nytg.board = board;
	nytg.hBoardTexts = hBoardTexts;
	nytg.vBoardTexts = vBoardTexts;
	nytg.text.setText(renderer, robotoF, "Not yours turn");
	nytg.text.dstR.w = 100;
	nytg.text.dstR.h = 25;
	nytg.text.dstR.x = windowWidth / 2 - nytg.text.dstR.w / 2;
	nytg.text.dstR.y = windowHeight - nytg.text.dstR.h - 20;
	while (running) {
		if (state == State::ShipPlacement) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					running = false;
					// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
				}
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
				}
				if (event.type == SDL_KEYDOWN) {
					keys[event.key.keysym.scancode] = true;
				}
				if (event.type == SDL_KEYUP) {
					keys[event.key.keysym.scancode] = false;
				}
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					buttons[event.button.button] = true;
					if (SDL_PointInRect(&mousePos, &rotateBtnR)) {
						for (int i = 0; i < figures.size(); ++i) {
							int tmp = figures[i].r.w;
							figures[i].r.w = figures[i].r.h;
							figures[i].r.h = tmp;
						}
						rotated = !rotated;
					}
					for (int i = 0; i < figures.size(); ++i) {
						figures[i].c = { FIGURE_COLOR };
					}
					for (int i = 0; i < figures.size(); ++i) {
						if (SDL_PointInRect(&mousePos, &figures[i].r)) {
							figures[i].c = { 255,255,0,0 };
							selectedFigureIndex = i;
						}
					}
					if (selectedFigureIndex != -1) {
						for (int i = 0; i < board.size(); ++i) {
							if (SDL_PointInRect(&mousePos, &board[i].r)) {
								if (board[i].c != SDL_Color({ FIGURE_COLOR })) {
									int size = std::max(figures[selectedFigureIndex].r.w, figures[selectedFigureIndex].r.h) / CELL_SIZE;
									int row = i / 10;
									int col = i % 10;
									bool figureColor = false;
									if (!rotated) {
										int size = std::max(figures[selectedFigureIndex].r.w, figures[selectedFigureIndex].r.h) / CELL_SIZE;
										bool shouldPlaceShip = true;
										if (((row + size - 1) * 10) + col < board.size()) {
											for (int i = 0; i < size; ++i) {
												int index = (row + i) * 10 + col;
												if (board[index].c == SDL_Color({ FIGURE_COLOR })) {
													shouldPlaceShip = false;
													break;
												}
											}
										}
										else {
											int leftCellsToFill = size;
											for (int i = 0; i < size; ++i) {
												int index = (row + i) * 10 + col;
												if (index >= board.size()) {
													break;
												}
												if (board[index].c == SDL_Color{ FIGURE_COLOR }) {
													shouldPlaceShip = false;
													break;
												}
												--leftCellsToFill;
											}
											if (shouldPlaceShip) {
												for (int i = 0; i < leftCellsToFill; ++i) {
													int index = (row - (i + 1)) * 10 + col;
													if (board[index].c == SDL_Color{ FIGURE_COLOR }) {
														shouldPlaceShip = false;
														break;
													}
												}
											}
										}
										if (shouldPlaceShip) {
											// NOTE: If it will go out of border than reverse the direction
											if (((row + size - 1) * 10) + col < board.size()) {
												for (int i = 0; i < size; ++i) {
													int index = (row + i) * 10 + col;
													board[index].c = { FIGURE_COLOR };
												}
											}
											else {
												// NOTE: Go as much as possible down and than go up to fill rest from size
												int leftCellsToFill = size;
												for (int i = 0; i < size; ++i) {
													int index = (row + i) * 10 + col;
													if (index >= board.size()) {
														break;
													}
													board[index].c = { FIGURE_COLOR };
													--leftCellsToFill;
												}
												for (int i = 0; i < leftCellsToFill; ++i) {
													int index = (row - (i + 1)) * 10 + col;
													board[index].c = { FIGURE_COLOR };
												}
											}
											figures.erase(figures.begin() + selectedFigureIndex);
											selectedFigureIndex = -1;
										}
									}
									else {
										int size = std::max(figures[selectedFigureIndex].r.w, figures[selectedFigureIndex].r.h) / CELL_SIZE;
										bool shouldPlaceShip = true;
										int leftCellsToFill = size;
										for (int i = 0; i < size; ++i) {
											int index = row * 10 + (col + i);
											int newRow = index / 10;
											if (row == newRow) {
												if (board[index].c == SDL_Color{ FIGURE_COLOR }) {
													shouldPlaceShip = false;
													break;
												}
												--leftCellsToFill;
											}
										}
										if (shouldPlaceShip) {
											for (int i = 0; i < leftCellsToFill; ++i) {
												int index = row * 10 + (col - (i + 1));
												if (board[index].c == SDL_Color{ FIGURE_COLOR }) {
													shouldPlaceShip = false;
												}
											}
										}
										if (shouldPlaceShip) {
											int leftCellsToFill = size;
											for (int i = 0; i < size; ++i) {
												int index = row * 10 + (col + i);
												int newRow = index / 10;
												if (row == newRow) {
													board[index].c = { FIGURE_COLOR };
													--leftCellsToFill;
												}
											}
											for (int i = 0; i < leftCellsToFill; ++i) {
												int index = row * 10 + (col - (i + 1));
												board[index].c = { FIGURE_COLOR };
											}
											figures.erase(figures.begin() + selectedFigureIndex);
											selectedFigureIndex = -1;
										}
									}
								}
								break;
							}
						}
					}
				}
				if (event.type == SDL_MOUSEBUTTONUP) {
					buttons[event.button.button] = false;
				}
				if (event.type == SDL_MOUSEMOTION) {
					float scaleX, scaleY;
					SDL_RenderGetScale(renderer, &scaleX, &scaleY);
					mousePos.x = event.motion.x / scaleX;
					mousePos.y = event.motion.y / scaleY;
					realMousePos.x = event.motion.x;
					realMousePos.y = event.motion.y;
				}
			}
			if (figures.empty()) {
				state = State::WaitForPlayer;
				std::vector<int> greenIndexes;
				for (int i = 0; i < board.size(); ++i) {
					if (board[i].c == SDL_Color({ 0,255,0,0 })) {
						greenIndexes.push_back(i);
					}
				}
				std::string indexes = std::to_string(greenIndexes.front());
				for (int i = 1; i < greenIndexes.size(); ++i) {
					indexes += " " + std::to_string(greenIndexes[i]);
				}
				send(socket, "changeState " + indexes);
			}
			SDL_SetRenderDrawColor(renderer, BG_COLOR);
			SDL_RenderClear(renderer);
			for (int i = 0; i < hBoardTexts.size(); ++i) {
				hBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < vBoardTexts.size(); ++i) {
				vBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < board.size(); ++i) {
				SDL_SetRenderDrawColor(renderer, board[i].c.r, board[i].c.g, board[i].c.b, board[i].c.a);
				SDL_RenderFillRect(renderer, &board[i].r);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_RenderDrawRect(renderer, &board[i].r);
			}
			for (int i = 0; i < figures.size(); ++i) {
				SDL_SetRenderDrawColor(renderer, figures[i].c.r, figures[i].c.g, figures[i].c.b, figures[i].c.a);
				SDL_RenderFillRect(renderer, &figures[i].r);
			}
			SDL_RenderCopy(renderer, rotateT, 0, &rotateBtnR);
			SDL_RenderPresent(renderer);
		}
		else if (state == State::WaitForPlayer) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					running = false;
					// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
				}
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
				}
				if (event.type == SDL_KEYDOWN) {
					keys[event.key.keysym.scancode] = true;
				}
				if (event.type == SDL_KEYUP) {
					keys[event.key.keysym.scancode] = false;
				}
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					buttons[event.button.button] = true;
				}
				if (event.type == SDL_MOUSEBUTTONUP) {
					buttons[event.button.button] = false;
				}
				if (event.type == SDL_MOUSEMOTION) {
					float scaleX, scaleY;
					SDL_RenderGetScale(renderer, &scaleX, &scaleY);
					mousePos.x = event.motion.x / scaleX;
					mousePos.y = event.motion.y / scaleY;
					realMousePos.x = event.motion.x;
					realMousePos.y = event.motion.y;
				}
			}
			wfp.waitingText.setText(renderer, robotoF, wfp.waitingText.text + ".");
			if (wfp.waitingText.text[wfp.waitingText.text.size() - 1] == '.'
				&& wfp.waitingText.text[wfp.waitingText.text.size() - 2] == '.'
				&& wfp.waitingText.text[wfp.waitingText.text.size() - 3] == '.'
				&& wfp.waitingText.text[wfp.waitingText.text.size() - 4] == '.') {
				wfp.waitingText.setText(renderer, robotoF, "Searching for players");
			}
			if (!wfp.first) {
				send(socket, "checkStatus");
				std::string answer;
				receive(socket, answer);
				if (answer == "Gameplay") {
					send(socket, "checkTurn");
					std::string answer;
					receive(socket, answer);
					if (answer == "your") {
						state = State::YourTurnGameplay;
					}
					else if (answer == "notYours") {
						state = State::NotYoursTurnGameplay;
					}
				}
			}
			wfp.first = false;
			SDL_SetRenderDrawColor(renderer, BG_COLOR);
			SDL_RenderClear(renderer);
			wfp.waitingText.draw(renderer);
			SDL_RenderPresent(renderer);
			SDL_Delay(300);
		}
		else if (state == State::YourTurnGameplay) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					running = false;
					// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
				}
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
				}
				if (event.type == SDL_KEYDOWN) {
					keys[event.key.keysym.scancode] = true;
				}
				if (event.type == SDL_KEYUP) {
					keys[event.key.keysym.scancode] = false;
				}
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					buttons[event.button.button] = true;
					for (int i = 0; i < ytg.board.size(); ++i) {
						if (SDL_PointInRect(&mousePos, &ytg.board[i].r)) {
							send(socket, "pick: " + std::to_string(i));
							std::string answer;
							receive(socket, answer);
							if (answer == "gameEnd") {
								goto gameBegin;
							}
							else if (answer == "hit") {
								ytg.board[i].c = { 255,0,0,0 };
							}
							else if (answer == "miss") {
								ytg.board[i].c = { 255,255,255,0 };
								state = State::NotYoursTurnGameplay;
							}
							break;
						}
					}
				}
				if (event.type == SDL_MOUSEBUTTONUP) {
					buttons[event.button.button] = false;
				}
				if (event.type == SDL_MOUSEMOTION) {
					float scaleX, scaleY;
					SDL_RenderGetScale(renderer, &scaleX, &scaleY);
					mousePos.x = event.motion.x / scaleX;
					mousePos.y = event.motion.y / scaleY;
					realMousePos.x = event.motion.x;
					realMousePos.y = event.motion.y;
				}
			}
			SDL_SetRenderDrawColor(renderer, BG_COLOR);
			SDL_RenderClear(renderer);
			for (int i = 0; i < ytg.hBoardTexts.size(); ++i) {
				ytg.hBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < ytg.vBoardTexts.size(); ++i) {
				ytg.vBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < ytg.board.size(); ++i) {
				SDL_SetRenderDrawColor(renderer, ytg.board[i].c.r, ytg.board[i].c.g, ytg.board[i].c.b, ytg.board[i].c.a);
				SDL_RenderFillRect(renderer, &ytg.board[i].r);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_RenderDrawRect(renderer, &ytg.board[i].r);
			}
			ytg.text.draw(renderer);
			SDL_RenderPresent(renderer);
		}
		else if (state == State::NotYoursTurnGameplay) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					running = false;
					// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
				}
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
				}
				if (event.type == SDL_KEYDOWN) {
					keys[event.key.keysym.scancode] = true;
				}
				if (event.type == SDL_KEYUP) {
					keys[event.key.keysym.scancode] = false;
				}
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					buttons[event.button.button] = true;
				}
				if (event.type == SDL_MOUSEBUTTONUP) {
					buttons[event.button.button] = false;
				}
				if (event.type == SDL_MOUSEMOTION) {
					float scaleX, scaleY;
					SDL_RenderGetScale(renderer, &scaleX, &scaleY);
					mousePos.x = event.motion.x / scaleX;
					mousePos.y = event.motion.y / scaleY;
					realMousePos.x = event.motion.x;
					realMousePos.y = event.motion.y;
				}
			}
			send(socket, "isMyTurn");
			std::string answer;
			receive(socket, answer);
			if (answer == "yourTurn") {
				state = State::YourTurnGameplay;
			}
			else if (answer.size() >= 13 && answer.substr(0, 13) == "notYoursTurn ") {
				nytg.board = board;
				std::string indexes = answer.substr(13);
				std::stringstream ss(indexes);
				std::string index;
				while (std::getline(ss, index, ' ')) {
					int i = std::stoi(index);
					if (nytg.board[i].c == SDL_Color({ 0,255,0 })) {
						nytg.board[i].c = { 255,0,0 };
					}
					else {
						nytg.board[i].c = { 255,255,255 };
					}
				}
			}
			else if (answer == "gameEnd") {
				goto gameBegin;
			}
			SDL_SetRenderDrawColor(renderer, BG_COLOR);
			SDL_RenderClear(renderer);
			for (int i = 0; i < nytg.hBoardTexts.size(); ++i) {
				nytg.hBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < nytg.vBoardTexts.size(); ++i) {
				nytg.vBoardTexts[i].draw(renderer);
			}
			for (int i = 0; i < nytg.board.size(); ++i) {
				SDL_SetRenderDrawColor(renderer, nytg.board[i].c.r, nytg.board[i].c.g, nytg.board[i].c.b, nytg.board[i].c.a);
				SDL_RenderFillRect(renderer, &nytg.board[i].r);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_RenderDrawRect(renderer, &nytg.board[i].r);
			}
			nytg.text.draw(renderer);
			SDL_RenderPresent(renderer);
		}
	}
	// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
	return 0;
}