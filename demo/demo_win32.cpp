#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "oglplot.h"

HGLRC hglrc = NULL;
BOOL contextIsCurrent = FALSE;

float scale = 10.0;
Plot *pPlot = nullptr;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.lpszClassName = "DemoWindow";
	wc.lpfnWndProc = WndProc;

	if (!RegisterClass(&wc)) {
		return 1;
	}

	HWND hWnd = CreateWindow("DemoWindow", "OpenGL Plot", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return 1;
	}

	Plot plot;

	SeriesData data;
	for (int i = 0; i < 7; i++) data.push_back({ (float)i,sin((float)i) });
	plot.addSeries(data);
	SeriesData data2;
	for (int i = 0; i < 7; i++) data2.push_back({ (float)i,exp(-(float)i) });
	plot.addSeries(data2);
	plot.limits(0.0f, 6.0f, -1.0f, 2.0f);
	
	plot.xticks(0.5f, 0.1f);
	plot.yticks(0.5f, 0.1f);

	pPlot = &plot;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;

}

LRESULT OnCreate(HWND hWnd)
{
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER |
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC hdc = GetDC(hWnd);
	if (hdc == NULL) {
		return -1;
	}

	int pixelFormat = ChoosePixelFormat(hdc, &pfd);

	if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
		return -1;
	}

	if ((hglrc = wglCreateContext(hdc)) == NULL) {
		return -1;
	}

	if (!(contextIsCurrent = wglMakeCurrent(hdc, hglrc))) {
		return -1;
	}

	if (glewInit() != GLEW_OK) {
		return -1;
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		return OnCreate(hWnd);
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			RECT rt;
			GetClientRect(hWnd, &rt);
			glViewport(0, 0, rt.right - rt.left, rt.bottom - rt.top);

			if (pPlot) {
				pPlot->draw();
			}

			SwapBuffers(hdc);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_OEM_PLUS:
			{
				scale++;
				pPlot->limits(-scale, scale, -scale, scale);
				pPlot->draw();
				HDC hdc = GetDC(hWnd);
				SwapBuffers(hdc);
				ReleaseDC(hWnd, hdc);
			}
			break;
		case VK_OEM_MINUS:
			{
				scale--;
				pPlot->limits(-scale, scale, -scale, scale);
				pPlot->draw();
				HDC hdc = GetDC(hWnd);
				SwapBuffers(hdc);
				ReleaseDC(hWnd, hdc);
			}
			break;
		case VK_ESCAPE:
			{
				pPlot->limits();
				pPlot->draw();
				HDC hdc = GetDC(hWnd);
				SwapBuffers(hdc);
				ReleaseDC(hWnd, hdc);
			}
			break;
		}
		break;
	case WM_DESTROY:
		if (contextIsCurrent) {
			wglMakeCurrent(NULL, NULL);
		}
		if (hglrc) {
			wglDeleteContext(hglrc);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}