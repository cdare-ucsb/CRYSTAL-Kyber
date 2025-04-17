#include <iostream>
#include <cstdlib>  // for system()
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sodium.h> // or another crypto-safe RNG

#include <openssl/evp.h>  // For SHAKE128,256

#include <string>
#include <iomanip> // For std::setw and std::setfill



#include "ModularMatrix.hpp"
#include "ModularInt.hpp"
#include "ModularPoly.hpp"
#include "NTTUtils.hpp"
#include "KeyGen.hpp"




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
    uint32_t Q;
    size_t N;
    uint32_t uroot;
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
    std::array<uint8_t, 32> seed;    // optional: original entropy
    std::array<uint8_t, 32> rho;     // public matrix seed
    std::array<uint8_t, 32> sigma;   // secret noise seed
    KyberParams params;
    ModularMatrix matrix;
    NTTContext ntt_ctx;
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

void print_matrix_menu(AppState& state) { 
    std::cout << "\033[2J\033[H"; // Clear + reset cursor position
    std::cout << "\033[32m";
    std::cout << R"(

 _____ ________   _______ _____ ___   _        _   __      _               
/  __ \| ___ \ \ / /  ___|_   _/ _ \ | |      | | / /     | |              
| /  \/| |_/ /\ V /\ `--.  | |/ /_\ \| |      | |/ / _   _| |__   ___ _ __ 
| |    |    /  \ /  `--. \ | ||  _  || |      |    \| | | | '_ \ / _ \ '__|
| \__/\| |\ \  | | /\__/ / | || | | || |____  | |\  \ |_| | |_) |  __/ |   
\_____/\_| \_| \_/ \____/  \_/\_| |_/\_____/  \_| \_/\__, |_.__/ \___|_|   
                                                     __/ |                
                                                    |___/                 
   )" << "\033[0m";

    
    generate_modular_matrix(state.matrix, state.rho);


    ModularPoly upper_left = state.matrix(0, 0); // Assuming '()' is the correct operator for accessing elements
    ModularPoly upper_right = state.matrix(0, state.params.k-1);
    ModularPoly lower_left = state.matrix(state.params.k-1, 0);
    ModularPoly lower_right = state.matrix(state.params.k-1, state.params.k-1);

    std::string upper_matrix_str = "\t\t|" + upper_left.to_string() + "\t\t...\t\t" + upper_right.to_string() + "|\n";
    std::string lower_matrix_str = "\t\t|" + lower_left.to_string() + "\t\t...\t\t" + lower_right.to_string() + "|\n";
    size_t upper_matrix_str_len = upper_matrix_str.length();

    std::cout << "\n\t\t\tMatrix generated:\n\n";

    std::cout << "\n"
              << "\t\t /" << std::string(upper_matrix_str_len+14, ' ') << "\\ \n"
              << upper_matrix_str 
              << "\t\t|" << std::string(upper_matrix_str_len+16, ' ') << "|\n"
              << "\t\t|" << std::string(upper_matrix_str_len+16, ' ') << "|\n"
              << "\t\t|     ." << std::string(upper_matrix_str_len+4, ' ') << ".     |\n"
              << "\t\t|     ." << std::string(upper_matrix_str_len+4, ' ') << ".     |\n"
              << "\t\t|     ." << std::string(upper_matrix_str_len+4, ' ') << ".     |\n"
              << "\t\t|" << std::string(upper_matrix_str_len+16, ' ') << "|\n"
              << "\t\t|" << std::string(upper_matrix_str_len+16, ' ') << "|\n"
              << lower_matrix_str
              << "\t\t \\" << std::string(upper_matrix_str_len+14, ' ') << "/ \n"
              << "\n\n";



    
    std::cout << "> \033[1m" << "Produce public key" << "\033[0m\n";

}

KyberParams print_kyber_param_menu() {
    uint32_t Q, uroot;
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
                else if (Q >= std::numeric_limits<uint32_t>::max()) {
                    std::cout << "\033[31mError: Q must be less than " << std::numeric_limits<uint32_t>::max() << ".\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (Q <= 2) {
                    std::cout << "\033[31mError: Q must be greater than 2.\033[0m\n";
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

                ModularInt w(uroot, Q);

                if (!w.is_primitive_nth_root(N)) {
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

                if (k <= 0) {
                    std::cout << "\033[31mError: The matrix dimension must be positive.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (k >= std::numeric_limits<uint8_t>::max()) {
                    std::cout << "\033[31mError: The matrix dimension must be less than " << std::numeric_limits<uint8_t>::max() << ".\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }


                valid_k = true;
            }

            if (!valid_eta1) {
                std::cout << "\tη1\t(coefficient bound 1)\t\t\t:\t";
                std::getline(std::cin, input);
                eta1 = std::stoi(input);

                if (eta1 > Q) {
                    std::cout << "\033[31mError: The size bound cannot be larger than the modulus.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (eta1 <= 0) {
                    std::cout << "\033[31mError: The size bound must be positive.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (eta1 >= std::numeric_limits<uint8_t>::max()) {
                    std::cout << "\033[31mError: The size bound must be less than " << std::numeric_limits<uint8_t>::max() << ".\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }

                valid_eta1 = true;
            }

            if (!valid_eta2) {
                std::cout << "\tη2\t(coefficient bound 2)\t\t\t:\t";
                std::getline(std::cin, input);
                eta2 = std::stoi(input);
                if (eta2 > Q) {
                    std::cout << "\033[31mError: The size bound cannot be larger than the modulus.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (eta2 <= 0) {
                    std::cout << "\033[31mError: The size bound must be positive.\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }
                else if (eta2 >= std::numeric_limits<uint8_t>::max()) {
                    std::cout << "\033[31mError: The size bound must be less than " << std::numeric_limits<uint8_t>::max() << ".\033[0m\n";
                    std::cout << "Press ENTER to try again...";
                    std::getline(std::cin, input); // wait
                    continue;
                }


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



std::array<uint8_t, 32> print_seed_input_menu() { 
    std::string input;
    std::array<uint8_t, 32> result = {};

    set_raw_mode(false);  // Disable raw mode for full std::cin interaction

    
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

    std::cout << "\nEnter seed value (length at most 32): ";
    std::getline(std::cin, input);  // Safely read entire line
    
    // Convert the string to a byte array
    std::copy_n(input.begin(), std::min(input.size(), size_t(32)), result.begin());

    set_raw_mode(true);  // Re-enable raw mode when done

    // Clear any leftover characters in the stdin buffer
    tcflush(STDIN_FILENO, TCIFLUSH);

    return result;
}


void print_public_key_menu() {

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


}


void handle_main_menu(AppState& state, char c) {
    InputKey key = get_input_key(c);

    switch (key) {
        case InputKey::Enter:
            if (state.main_menu_selection == 0) {
                state.current = Page::SeedMenu;
                state.params = KyberParams{3329, 256, 17, 3, 2, 2};
                state.ntt_ctx = NTTContext(state.params.Q, state.params.N, state.params.uroot);
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

                randombytes_buf(state.seed.data(), state.seed.size());  // uniformly random seed

                uint8_t out[32];  // 32 bytes output
                EVP_MD_CTX* ctx = EVP_MD_CTX_new();
                EVP_DigestInit_ex(ctx, EVP_shake256(), nullptr);
                EVP_DigestUpdate(ctx, state.seed.data(), state.seed.size());
                EVP_DigestFinalXOF(ctx, out, 32);
                EVP_MD_CTX_free(ctx);

                std::copy(out, out + 32, state.rho.begin());

                std::cout << "\t\t\tSeed ";
                for (uint8_t byte : state.seed) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(byte);
                }
                std::cout << " stored in state......\n";
                std::cout << "\t\t\tPress any key to continue...\n";

                while (read(STDIN_FILENO, &c, 1) == 1) {
                    if (get_input_key(c) == InputKey::Escape) {
                        state.current = Page::Exit;
                        return;
                    }
                    else if (get_input_key(c) == InputKey::Enter) {
                        break;
                    }
                }

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
    state.ntt_ctx = NTTContext(state.params.Q, state.params.N, state.params.uroot);
    state.current = Page::SeedMenu;
}



void handle_seed_input(AppState& state) {
    state.rho = print_seed_input_menu();

    std::cout << "\n\n\n\t\t\tSeed ";
    for (uint8_t byte : state.rho) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte);
    }
    
    
    std::cout << " stored in state......\n";
    std::cout << "\t\t\tPress any key to continue...\n";

    state.current = Page::MatrixGenerated;
}

void handle_matrix_generated(AppState& state) {

    state.matrix = ModularMatrix(state.params.k, state.params.k, state.params.Q, state.params.N);

    print_matrix_menu(state);  
    // Convert matrix to NTT domain for faster multipliciation
    for (auto& entry : state.matrix.data) {
        entry.ctx = &state.ntt_ctx;
        entry.NTT();
    }

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (get_input_key(c) == InputKey::Escape) {
            state.current = Page::Exit;
            return;
        }
        else if (get_input_key(c) == InputKey::Enter) {
            state.current = Page::MainMenu;
            return;
        }
    }
}

void handle_public_key(AppState& state) {

    std::array<uint8_t, 32> sigma;

    uint8_t out[32];  // 32 bytes output
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_shake256(), nullptr);
    EVP_DigestUpdate(ctx, state.seed.data(), state.seed.size());
    EVP_DigestFinalXOF(ctx, out, 32);
    EVP_MD_CTX_free(ctx);

    std::copy(out, out + 32, sigma.begin());

    auto [vec_s, vec_e] = generate_secret_and_error_vectors(state.sigma, state.params.eta1, state.params.k, state.params.N, state.params.Q);

    for (size_t i = 0; i < vec_s.size(); ++i) {
        vec_s[i].ctx = &state.ntt_ctx;
        vec_s[i].NTT();

        vec_e[i].ctx = &state.ntt_ctx;
        vec_e[i].NTT();
    }

    std::vector<ModularPoly> vec_t = state.matrix.apply_transform(vec_s);

    for (size_t i = 0; i < vec_t.size(); ++i) {
        vec_t[i].ctx = &state.ntt_ctx;
        vec_t[i] = vec_t[i] + vec_e[i];
        vec_t[i].INTT();
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