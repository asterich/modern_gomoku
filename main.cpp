import std;
import game_modes;

int main() {
    std::cout << " ---------------------------------------\n";
    std::cout << "|             Wuziqi Game               |\n";
    std::cout << "|                                       |\n";
    std::cout << "|         Welcome to the game!          |\n";
    std::cout << "|  The game has two modes: PVP and PVM  |\n";
    std::cout << " ---------------------------------------\n\n";
    std::cout << "Please press Enter to continue...";
    std::cout.flush();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (true) {
        std::cout << "Please choose the game mode:\n";
        std::cout << "PVP  :  1\n";
        std::cout << "PVM  :  2\n";
        std::cout << "Quit :  q\n\n";
        std::cout << "Your choice : ";

        std::string mode;
        if (!(std::cin >> mode)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Input error detected. Exiting...\n";
            break;
        }

        if (mode == "1") {
            game_modes::run_pvp();
        } else if (mode == "2") {
            game_modes::run_pvm();
        } else if (mode == "q") {
            break;
        } else {
            std::cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}
