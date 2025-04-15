#include "ModularInt.hpp"
#include <iostream>
#include <cstdlib>  // for system()
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


void set_raw_mode(bool enable) {
    static struct termios oldt, newt;

    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);  // Save current settings
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // Disable buffering and echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore settings
    }
}

void print_initial_menu(int selected) {
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "\033[32m";
    std::cout << R"(

 _____ ________   _______ _____ ___   _        _   __      _               
/  __ \| ___ \ \ / /  ___|_   _/ _ \ | |      | | / /     | |              
| /  \/| |_/ /\ V /\ `--.  | |/ /_\ \| |      | |/ / _   _| |__   ___ _ __ 
| |    |    /  \ /  `--. \ | ||  _  || |      |    \| | | | '_ \ / _ \ '__|
| \__/\| |\ \  | | /\__/ / | || | | || |____  | |\  \ |_| | |_) |  __/ |   
 \____/\_| \_| \_/ \____/  \_/\_| |_/\_____/  \_| \_/\__, |_.__/ \___|_|   
                                                      __/ |                
                                                     |___/                 
)" << "\033[0m";

    std::cout << "Use \033[1mUP\033[0m / \033[1mDOWN\033[0m arrows to move, \033[1mENTER\033[0m to select, \033[1mESC\033[0m to exit:\n\n";

    const char* options[2] = {
        "Use default ML-KEM-768 Parameters",
        "Use custom parameters"
    };

    for (int i = 0; i < 2; ++i) {
        if (i == selected)
            std::cout << "> \033[1m" << options[i] << "\033[0m\n";
        else
            std::cout << "  " << options[i] << "\n";
    }
}

void print_seed_menu(int selected) {
    std::cout << "\033[2J\033[H"; // Clear + reset cursor position
    std::cout << "\033[32m";
    std::cout << R"(

    _____ ________   _______ _____ ___   _        _   __      _               
   /  __ \| ___ \ \ / /  ___|_   _/ _ \ | |      | | / /     | |              
   | /  \/| |_/ /\ V /\ `--.  | |/ /_\ \| |      | |/ / _   _| |__   ___ _ __ 
   | |    |    /  \ /  `--. \ | ||  _  || |      |    \| | | | '_ \ / _ \ '__|
   | \__/\| |\ \  | | /\__/ / | || | | || |____  | |\  \ |_| | |_) |  __/ |   
    \____/\_| \_| \_/ \____/  \_/\_| |_/\_____/  \_| \_/\__, |_.__/ \___|_|   
                                                         __/ |                
                                                        |___/                 
   )" << "\033[0m";

    // Keep printed parameters above the seed menu
    std::cout << "\n\n\n\n Using values:\n"
              << "\t\tQ=3329\n"
              << "\t\tN=256\n"
              << "\t\tk=3\n"
              << "\t\tη1=2\n"
              << "\t\tη2=2\n\n";

    // Now show the seed selection menu
    std::cout << "Select seed type:\n\n";

    const char* seed_options[2] = {
        "Use random seed",
        "Use pre-set seed"
    };

    for (int i = 0; i < 2; ++i) {
        if (i == selected)
            std::cout << "> \033[1m" << seed_options[i] << "\033[0m\n";
        else
            std::cout << "  " << seed_options[i] << "\n";
    }
}


int main() {
    std::cout << "\033[?1049h\033[?25l";
    set_raw_mode(true);

    int selected = 0;
    print_initial_menu(selected);

    bool show_seed_menu = false;
    int seed_choice = 0;

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 27) {
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

            char seq[2];
            ssize_t n = read(STDIN_FILENO, &seq[0], 1);
            if (n == -1) break;  // Standalone ESC

            read(STDIN_FILENO, &seq[1], 1);
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up
                    if (show_seed_menu) seed_choice = (seed_choice - 1 + 2) % 2;
                    else selected = (selected - 1 + 2) % 2;
                } else if (seq[1] == 'B') { // Down
                    if (show_seed_menu) seed_choice = (seed_choice + 1) % 2;
                    else selected = (selected + 1) % 2;
                }
            }

            fcntl(STDIN_FILENO, F_SETFL, flags);
        } else if (c == '\n') {
            if (!show_seed_menu && selected == 0) {
                std::cout << "\n\n\n\n Using values:\n"
                          << "\t\tQ=3329\n"
                          << "\t\tN=256\n"
                          << "\t\tk=3\n"
                          << "\t\tη1=2\n"
                          << "\t\tη2=2\n\n";
                show_seed_menu = true;
                print_seed_menu(seed_choice);
            } 
            else if (show_seed_menu) {
                std::cout << "\033[2J\033[H"; // Clear screen
                std::cout << "Seed selection complete.\n";
                std::cout << "You selected: \033[1m" 
                          << (seed_choice == 0 ? "Random seed" : "Pre-set seed") 
                          << "\033[0m.\n\n";
                std::cout << "Press ESC to exit." << std::endl;
            
                // Wait for ESC before quitting
                while (read(STDIN_FILENO, &c, 1) == 1) {
                    if (c == 27) {
                        break;
                    }
                }
            
                break; // Now exit after ESC
            }
            
        }

        // Redraw screen
        if (!show_seed_menu) {
            print_initial_menu(selected);
        } else {
            print_seed_menu(seed_choice);
        }
    }

    set_raw_mode(false);
    std::cout << "\033[?25h\033[?1049l";
    return 0;
}