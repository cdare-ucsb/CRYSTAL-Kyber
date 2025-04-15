#include "ModularInt.hpp"
#include <iostream>
#include <cstdlib>  // for system()
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "ModularMatrix.hpp"
#include "SmallestUInt.hpp"
#include "ModularArith.hpp"
#include "ModularInt.hpp"
#include "NTT.hpp"
#include "NTTUtils.hpp"


enum class Page {
    MainMenu,
    SeedMenu,
    SeedInput,
    KyberParamInput,
    MatrixGenerated,
    Exit
};

enum class InputKey {
    Up,
    Down,
    Enter,
    Escape,
    Other
};

struct KyberParams {
    uint64_t Q;
    size_t N;
    uint64_t uroot;
    uint8_t k;
    uint8_t eta1;
    uint8_t eta2;
};


struct AppState {
    Page current;
    Page previous;
    int main_menu_selection = 0;
    int seed_menu_selection = 0;
    std::string user_seed;
    int seed = 0;
    KyberParams params;
};





void handle_main_menu(AppState& state, char key);
void handle_seed_menu(AppState& state, char key);
void handle_seed_input(AppState& state);
void handle_matrix_generated(AppState& state);
void handle_kyber_param_input(AppState& state);

bool is_prime(uint64_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (uint64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }

    return true;
}

bool is_power_of_two(uint64_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}


InputKey get_input_key(char first_char) {
    if (first_char == '\n') return InputKey::Enter;
    if (first_char == 27) {
        // Non-blocking peek for escape sequence
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        char seq[2];
        ssize_t n = read(STDIN_FILENO, &seq[0], 1);
        if (n <= 0) {
            // Standalone ESC
            fcntl(STDIN_FILENO, F_SETFL, flags);
            return InputKey::Escape;
        }

        read(STDIN_FILENO, &seq[1], 1);

        fcntl(STDIN_FILENO, F_SETFL, flags);

        if (seq[0] == '[') {
            if (seq[1] == 'A') return InputKey::Up;
            if (seq[1] == 'B') return InputKey::Down;
        }

        return InputKey::Other;  // Could be other sequences
    }

    return InputKey::Other;
}


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
            std::cout << "> \033[1m" << options[i] << "\033[0m\n"; // Wrap selected in bold
        else
            std::cout << "  " << options[i] << "\n";
    }
}

void print_seed_menu(AppState& state, int selected) {
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
              << "\t\tQ=" << state.params.Q << "\n"
              << "\t\tN="<< state.params.N << "\n"
              << "\t\tω="<< state.params.uroot << "\n"
              << "\t\tk="<< static_cast<int>(state.params.k) << "\n"
              << "\t\tη1=" << static_cast<int>(state.params.eta1) << "\n"
              << "\t\tη2=" << static_cast<int>(state.params.eta2) <<"\n\n";

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

void print_matrix_menu(int seed) { 
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
    std::cout << "Matrix A Generated" << std::endl;



    
    std::cout << "> \033[1m" << "Continue" << "\033[0m\n";

}

KyberParams print_kyber_param_menu() {
    uint64_t Q, uroot;
    size_t N;
    uint8_t k, eta1, eta2;
    bool valid_Q = false, valid_N = false, valid_root = false;
    bool valid_k = false, valid_eta1 = false, valid_eta2 = false;

    set_raw_mode(false);

    while (!(valid_Q && valid_N && valid_root && valid_k && valid_eta1 && valid_eta2)) {
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

        std::cout << "\nEnter Kyber Parameters (press ENTER to keep valid values):\n\n";

        try {
            std::string input;

            if (!valid_Q) {
                std::cout << "\tQ\t(modulus)\t\t\t\t:\t";
                std::getline(std::cin, input);
                
                Q = std::stoull(input);
                if (!is_prime(Q)) {
                    std::cout << "\033[31mError: Q must be a prime number.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                valid_Q = true;
            }

            if (!valid_N) {
                std::cout << "\tN\t(polynomial length)\t\t\t:\t";
                std::getline(std::cin, input);
                N = std::stoull(input);
                if (!is_power_of_two(N)) {
                    std::cout << "\033[31mError: N must be a power of 2.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                valid_N = true;
            }

            if (!valid_root) {
                std::cout << "\troot\t(primitive N-th root of unity mod Q)\t:\t";
                std::getline(std::cin, input);
                uroot = std::stoul(input);

                using MA = ModArith<Q>;
                if (!MA::is_primitive_nth_root(uroot, N)) {
                    std::cout << "\033[31mError: The root is not a primitive N-th root of unity.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }

                valid_root = true;
            }

            if (!valid_k) {
                std::cout << "\tk\t(matrix dimension)\t\t\t:\t";
                std::getline(std::cin, input);
                k = std::stoi(input);
                valid_k = true;
            }

            if (!valid_eta1) {
                std::cout << "\tη1\t(coefficient bound 1)\t\t\t:\t";
                std::getline(std::cin, input);

                eta1 = std::stoi(input);
                valid_eta1 = true;
            }

            if (!valid_eta2) {
                std::cout << "\tη2\t(coefficient bound 2)\t\t\t:\t";
                std::getline(std::cin, input);
                eta2 = std::stoi(input);
                valid_eta2 = true;
            }

        } catch (...) {
            std::cout << "\n\033[31mInvalid input. Please enter numeric values only.\033[0m\n";
            std::cout << "Press ENTER to retry only the failed fields...";
            std::string dummy;
            std::getline(std::cin, dummy);
        }

    }

    tcflush(STDIN_FILENO, TCIFLUSH);
    set_raw_mode(true);

    return KyberParams{Q, N, uroot, k, eta1, eta2};
}



std::string print_seed_input_menu() { 
    std::string user_seed;

    set_raw_mode(false);  // Disable raw mode for full std::cin interaction

    while (true) {
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

        std::cout << "\nEnter seed value (must be a valid integer): ";
        std::getline(std::cin, user_seed);  // Safely read entire line

        try {
            std::stoi(user_seed);
            break;  // valid input
        } catch (...) {
            std::cout << "\n\033[31mInvalid input. Please enter a valid integer.\033[0m\n";
            std::cout << "Press ENTER to try again...";
            std::string dummy;
            std::getline(std::cin, dummy);  // Wait for ENTER before retrying
        }
    }

    set_raw_mode(true);  // Re-enable raw mode when done

    // Clear any leftover characters in the stdin buffer
    tcflush(STDIN_FILENO, TCIFLUSH);

    return user_seed;
}


void handle_main_menu(AppState& state, char c) {
    InputKey key = get_input_key(c);

    switch (key) {
        case InputKey::Enter:
            if (state.main_menu_selection == 0) {
                state.current = Page::SeedMenu;
                state.params = KyberParams{3329, 256, 17, 3, 2, 2};
            } else {
                state.current = Page::KyberParamInput;
            }
            break;
        case InputKey::Escape:
            state.current = Page::Exit;
            break;
        case InputKey::Up:
            state.main_menu_selection = (state.main_menu_selection - 1 + 2) % 2;
            break;
        case InputKey::Down:
            state.main_menu_selection = (state.main_menu_selection + 1) % 2;
            break;
        default:
            break;
    }

    print_initial_menu(state.main_menu_selection);
}

void handle_seed_menu(AppState& state, char c) {
    InputKey key = get_input_key(c);  // use the helper

    switch (key) {
        case InputKey::Enter:
            if (state.seed_menu_selection == 0) {
                std::cout << "\n\nUsing random seed...\n";
                state.seed = rand() % 1000000;  // Random seed for demonstration
                state.current = Page::MatrixGenerated;
            } else {
                std::cout << "Using pre-set seed...\n";
                state.current = Page::SeedInput;
            }
            break;
        case InputKey::Escape:
            state.current = Page::Exit;
            break;
        case InputKey::Up:
            state.seed_menu_selection = (state.seed_menu_selection - 1 + 2) % 2;
            break;
        case InputKey::Down:
            state.seed_menu_selection = (state.seed_menu_selection + 1) % 2;
            break;
        default:
            break;
    }

    print_seed_menu(state, state.seed_menu_selection);
}

void handle_kyber_param_input(AppState& state) {
    state.params = print_kyber_param_menu();
    
    // Parse input

    state.current = Page::SeedMenu;
}




void handle_seed_input(AppState& state) {
    std::string user_seed = print_seed_input_menu();
    
    /// TODO: Add parsing here
    state.seed = std::stoi(user_seed);

    std::cout << "\n\n\n\t\t\tSeed " << state.seed << " stored in state......\n";
    std::cout << "\t\t\tPress any key to continue...\n";

    state.current = Page::MatrixGenerated;
}

void handle_matrix_generated(AppState& state) {

    print_matrix_menu(0);  // or 1 if using preset seed
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (get_input_key(c) == InputKey::Escape) {
            state.current = Page::Exit;
            return;
        }
    }
}


int main() {
    AppState state;
    state.current = Page::MainMenu;
    state.previous = Page::MainMenu;

    std::cout << "\033[?1049h\033[?25l";
    set_raw_mode(true);

    char c;
    bool should_redraw = true;

    while (state.current != Page::Exit) {
        if (should_redraw) {
            switch (state.current) {
                case Page::MainMenu:
                    print_initial_menu(state.main_menu_selection);
                    break;
                case Page::SeedMenu:
                    print_seed_menu(state, state.seed_menu_selection);
                    break;
                default:
                    break;
            }
            should_redraw = false; // wait until next change
        }

        if (read(STDIN_FILENO, &c, 1) != 1) break;

        state.previous = state.current;

        switch (state.current) {
            case Page::MainMenu:
                handle_main_menu(state, c);
                break;
            case Page::SeedMenu:
                handle_seed_menu(state, c);
                break;
            case Page::SeedInput:
                handle_seed_input(state);
                break;
            case Page::KyberParamInput:
                handle_kyber_param_input(state);
                break;
            case Page::MatrixGenerated:
                handle_matrix_generated(state);
                break;
            default:
                state.current = Page::Exit;
                break;
        }

        // Immediately handle page transition to input/render-only pages
        if (state.current != state.previous) {
            should_redraw = true;

            if (state.current == Page::SeedInput) {
                handle_seed_input(state);
                should_redraw = true;


                continue;                              // go to top of loop

            }

            else if (state.current == Page::KyberParamInput) {
                handle_kyber_param_input(state);
                should_redraw = true;
                continue;
            }
        
            else if (state.current == Page::MatrixGenerated) {
                handle_matrix_generated(state);         // runs immediately
                should_redraw = true;
                continue;
            }
        }
    }

    set_raw_mode(false);
    std::cout << "\033[?25h\033[?1049l";
    return 0;
}