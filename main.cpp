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
	SDL_GetMouseState(&mousePos.x, &mousePos.y);
	SDL_Window* window = SDL_CreateWindow("Ships", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
	SDL_AddEventWatch(eventWatch, 0);
	bool running = true;
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
	figures.back().r.w = 16;
	figures.back().r.h = 16;
	figures.back().r.x = 0;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 16;
	figures.back().r.x = figures[0].r.x + figures[0].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 32;
	figures.back().r.x = figures[1].r.x + figures[1].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 32;
	figures.back().r.x = figures[2].r.x + figures[2].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 48;
	figures.back().r.x = figures[3].r.x + figures[3].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 64;
	figures.back().r.x = figures[4].r.x + figures[4].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	figures.push_back(Figure());
	figures.back().r.w = 16;
	figures.back().r.h = 80;
	figures.back().r.x = figures[5].r.x + figures[5].r.w + 5;
	figures.back().r.y = windowHeight - figures.back().r.h;
	SDL_Rect rotateBtnR;
	rotateBtnR.w = 32;
	rotateBtnR.h = 32;
	rotateBtnR.x = windowWidth - rotateBtnR.w;
	rotateBtnR.y = windowHeight - rotateBtnR.h;
	SDL_Texture* rotateT = IMG_LoadTexture(renderer, "res/rotate.png");
	while (running) {
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
				}
				for (int i = 0; i < figures.size(); ++i) {
					figures[i].c = { FIGURE_COLOR };
				}
				for (int i = 0; i < figures.size(); ++i) {
					if (SDL_PointInRect(&mousePos, &figures[i].r)) {
						figures[i].c = { 255,255,0,0 };
					}
				}
				for (int i = 0; i < board.size(); ++i) {
					if (SDL_PointInRect(&mousePos, &board[i].r)) {
						board[i].c = { FIGURE_COLOR };
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
	// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
	return 0;
}