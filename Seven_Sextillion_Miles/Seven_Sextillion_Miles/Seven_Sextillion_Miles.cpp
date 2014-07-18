// Seven_Sextillion_Miles.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "Seven_Sextillion_Miles.h"
#include "GLGraphics.h"
#include "global.h"
#include "PlayerShip.h"
#include "Camera.h"
#include "GameClock.h"

#define MAX_LOADSTRING 100

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

// Very evil global variable to be able to access graphics...
GLGraphics *graphicsPtr;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Hier Code einfügen.
	MSG msg;

	// Globale Zeichenfolgen initialisieren
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SEVEN_SEXTILLION_MILES, szWindowClass, MAX_LOADSTRING);
	//MyRegisterClass(hInstance);

	GLGraphics graphics;
	if (graphics.init(640, 480, WndProc) < 0)
	{
		exit(-1);
	}
	graphicsPtr = &graphics;

	// Hauptnachrichtenschleife:
	bool exitGame = false;

	// The in-game objects, will probably put them into a game object of
	// some sort in the future
	Camera camera;
	PlayerShip playerShip;
	playerShip.setPos(LargeInt(182000), LargeInt(525000));
	GameClock clock;

	float simulationTime = 0.0f; // Time that has yet to be calculated

	while (!exitGame)
	{
		char errorString[MAX_ERROR_LENGTH + 1];

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) exitGame = true;
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Do physics

		simulationTime += clock.getDeltaTime();
		// 1/180 ms time steps???
		float timeStepSize = 1.0f / 180.0f;
		while (simulationTime > TIME_STEP_SIZE)
		{
			playerShip.setAcceleration(0.1f);
			playerShip.timeStep();
			simulationTime -= TIME_STEP_SIZE;
			playerShip.setRotAcc(0.0001f);
			//playerShip.setRot(1.6f);
		}

		// Render
		graphics.clear();

		camera.setPos(LargeInt(180000), LargeInt(530000));
		//playerShip.setRot(0.3f);
		//void setSpeed(float x, float y) { loc.setSpeed(x, y); }

		if (playerShip.draw(&graphics, &camera, errorString) != 0)
		{
			graphics.handleError(errorString);
			exit(-1);
		}

		// Swap for next frame
		graphics.swap();
	}
	graphicsPtr = NULL;

	// De-initilialize Graphics: (TODO: Shall I???)

	return (int) msg.wParam;
}

//
//  FUNKTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ZWECK:  Verarbeitet Meldungen vom Hauptfenster.
//
//  WM_COMMAND	- Verarbeiten des Anwendungsmenüs
//  WM_PAINT	- Zeichnen des Hauptfensters
//  WM_DESTROY	- Beenden-Meldung anzeigen und zurückgeben
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Menüauswahl bearbeiten:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_SIZE:
		if (graphicsPtr)
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			graphicsPtr->resize(width, height);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}