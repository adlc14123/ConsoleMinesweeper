/**
 * ============================================================================
 * File: ConsoleMinesweeper.cpp
 * Description: A cross-platform, console-based Minesweeper game written strictly
 *              in standard C++17 with full input validation, QUIT support,
 *              and cell chording capability.
 * 
 * Standards & Guidelines:
 * - C++17 Standard only.
 * - No C standard library headers (e.g., <stdio.h>, <stdlib.h>, <time.h>).
 * - Clean compilation under GCC, Clang (-Wall -Wextra -pedantic -std=c++17) 
 *   and MSVC (/W4 /std:c++17).
 * - Cross-platform support for Windows, Linux, FreeBSD, and macOS.
 * ============================================================================
 */

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Difficulty levels available in the game
enum class Difficulty {
    Easy = 1,   // 8 rows x 10 cols (10 mines)
    Medium = 2, // 14 rows x 18 cols (40 mines)
    Hard = 3,   // 20 rows x 24 cols (99 mines)
    Quit = 4    // User requested program termination
};

// Represents the state of an individual grid cell
struct Cell {
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int adjacentMines = 0;
};

// Command data structure representing a parsed user action
struct UserCommand {
    char action = '\0'; // 'D' for Dig, 'F' for Flag, 'Q' for Quit
    int row = -1;       // 0-indexed row
    int col = -1;       // 0-indexed column
    bool valid = false;
    bool isQuit = false;
    std::string errorMessage;
};

// Helper function to trim leading and trailing whitespace from a string
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    const std::size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    const std::size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Primary board class managing grid layout, mine distribution, chording, and game logic
class MinesweeperBoard {
private:
    int rows = 0;
    int cols = 0;
    int totalMines = 0;
    int flagsCount = 0;
    bool firstMoveDone = false;
    bool gameOver = false;
    bool gameWon = false;

    std::vector<std::vector<Cell>> grid;
    std::mt19937 rng; // High-quality C++ Mersenne Twister PRNG engine

public:
    MinesweeperBoard() : rng(std::random_device{}()) {}

    // Configures grid dimensions and mine count based on selected difficulty
    void setup(Difficulty diff) {
        switch (diff) {
            case Difficulty::Easy:
                rows = 8;
                cols = 10;
                totalMines = 10;
                break;
            case Difficulty::Medium:
                rows = 14;
                cols = 18;
                totalMines = 40;
                break;
            case Difficulty::Hard:
                rows = 20;
                cols = 24;
                totalMines = 99;
                break;
            case Difficulty::Quit:
                return;
        }

        flagsCount = 0;
        firstMoveDone = false;
        gameOver = false;
        gameWon = false;

        grid.assign(static_cast<std::size_t>(rows),
                    std::vector<Cell>(static_cast<std::size_t>(cols)));
    }

    int getRows() const { return rows; }
    int getCols() const { return cols; }
    int getRemainingMines() const { return totalMines - flagsCount; }
    bool isGameOver() const { return gameOver; }
    bool isGameWon() const { return gameWon; }
    bool isFirstMoveDone() const { return firstMoveDone; }

    bool isValidCoordinate(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }

private:
    // Distributes mines randomly ensuring the first click area is safe
    void populateMines(int safeRow, int safeCol) {
        std::vector<std::pair<int, int>> availablePositions;
        availablePositions.reserve(static_cast<std::size_t>(rows * cols));

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                // Ensure safe zone around the first clicked cell (safe cell + adjacent cells)
                if (std::abs(r - safeRow) <= 1 && std::abs(c - safeCol) <= 1) {
                    continue;
                }
                availablePositions.emplace_back(r, c);
            }
        }

        // Fallback safety check if safe area leaves too few cells for total mines
        if (static_cast<int>(availablePositions.size()) < totalMines) {
            availablePositions.clear();
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    if (r == safeRow && c == safeCol) continue;
                    availablePositions.emplace_back(r, c);
                }
            }
        }

        // Secure shuffle using standard C++ PRNG engine
        std::shuffle(availablePositions.begin(), availablePositions.end(), rng);

        // Place mines
        for (int i = 0; i < totalMines; ++i) {
            auto [r, c] = availablePositions[static_cast<std::size_t>(i)];
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)].isMine = true;
        }

        // Calculate neighboring mine count for each cell
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)].isMine) {
                    continue;
                }

                int mineCount = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int nr = r + dr;
                        int nc = c + dc;
                        if (isValidCoordinate(nr, nc) &&
                            grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)].isMine) {
                            mineCount++;
                        }
                    }
                }
                grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)].adjacentMines = mineCount;
            }
        }
    }

    // Recursive flood fill to open adjacent empty cells
    void floodFill(int r, int c) {
        if (!isValidCoordinate(r, c)) return;

        Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
        if (cell.isRevealed || cell.isFlagged || cell.isMine) return;

        cell.isRevealed = true;

        if (cell.adjacentMines == 0) {
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    floodFill(r + dr, c + dc);
                }
            }
        }
    }

    // Executes Chording on an already revealed cell when flag count matches adjacent mine count
    void executeChording(int r, int c) {
        Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];

        // Count adjacent flagged cells
        int flaggedCount = 0;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr;
                int nc = c + dc;
                if (isValidCoordinate(nr, nc)) {
                    if (grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)].isFlagged) {
                        flaggedCount++;
                    }
                }
            }
        }

        // Chording only triggers if flag count matches number of adjacent mines
        if (flaggedCount != cell.adjacentMines) {
            std::cout << "\n[!] Chording requires " << cell.adjacentMines 
                      << " adjacent flag(s) (currently " << flaggedCount << ").\n";
            return;
        }

        // Check unflagged, unrevealed neighbors for mines
        bool hitMine = false;
        std::vector<std::pair<int, int>> safeNeighbors;

        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr;
                int nc = c + dc;
                if (isValidCoordinate(nr, nc)) {
                    Cell& neighbor = grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)];
                    if (!neighbor.isRevealed && !neighbor.isFlagged) {
                        if (neighbor.isMine) {
                            neighbor.isRevealed = true;
                            hitMine = true;
                        } else {
                            safeNeighbors.emplace_back(nr, nc);
                        }
                    }
                }
            }
        }

        // If a mine was hit due to incorrect flagging, terminate immediately without uncovering safe cells
        if (hitMine) {
            gameOver = true;
            gameWon = false;
            revealAllMines();
            return;
        }

        // Safe to uncover all unflagged neighbors
        for (const auto& [nr, nc] : safeNeighbors) {
            floodFill(nr, nc);
        }

        checkVictoryCondition();
    }

    // Checks if all non-mine cells have been uncovered
    void checkVictoryCondition() {
        int unrevealedCount = 0;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
                if (!cell.isRevealed) {
                    unrevealedCount++;
                }
            }
        }

        if (unrevealedCount == totalMines) {
            gameWon = true;
            gameOver = true;

            // Automatically flag all remaining mines on victory for a complete board
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
                    if (cell.isMine) {
                        cell.isFlagged = true;
                    }
                }
            }
            flagsCount = totalMines;
        }
    }

public:
    // Dig action at target cell coordinates (handles standard dig & chording)
    void digCell(int r, int c) {
        if (!isValidCoordinate(r, c)) return;

        Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];

        if (cell.isFlagged) {
            std::cout << "\n[!] Cell is flagged. Unflag it before digging.\n";
            return;
        }

        // Handle Chording if digging an already revealed numbered cell
        if (cell.isRevealed) {
            if (cell.adjacentMines > 0) {
                executeChording(r, c);
            }
            return;
        }

        // Guarantee first click is safe by generating mines on the first move
        if (!firstMoveDone) {
            populateMines(r, c);
            firstMoveDone = true;
        }

        if (cell.isMine) {
            cell.isRevealed = true;
            gameOver = true;
            gameWon = false;
            revealAllMines();
            return;
        }

        floodFill(r, c);
        checkVictoryCondition();
    }

    // Toggle flag status on an unrevealed cell
    void toggleFlag(int r, int c) {
        if (!isValidCoordinate(r, c)) return;

        Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
        if (cell.isRevealed) {
            std::cout << "\n[!] Cannot flag an already revealed cell.\n";
            return;
        }

        if (cell.isFlagged) {
            cell.isFlagged = false;
            flagsCount--;
        } else {
            cell.isFlagged = true;
            flagsCount++;
        }
    }

    // Reveals all mines on loss
    void revealAllMines() {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
                if (cell.isMine) {
                    cell.isRevealed = true;
                }
            }
        }
    }

    // Renders board grid and game HUD to console
    void display(long long elapsedSeconds) const {
        std::cout << "\n============================================\n";
        std::cout << " Time: " << elapsedSeconds << "s | Remaining Mines: " << getRemainingMines() << "\n";
        std::cout << "============================================\n\n";

        // Column header (A, B, C, ...)
        std::cout << "    ";
        for (int c = 0; c < cols; ++c) {
            char colChar = static_cast<char>('A' + c);
            std::cout << " " << colChar << "  ";
        }
        std::cout << "\n";

        // Upper border
        std::cout << "   +";
        for (int c = 0; c < cols; ++c) {
            std::cout << "----";
        }
        std::cout << "+\n";

        // Rows content
        for (int r = 0; r < rows; ++r) {
            std::cout << std::setw(2) << (r + 1) << " |";
            for (int c = 0; c < cols; ++c) {
                const Cell& cell = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];

                if (gameOver && !gameWon && cell.isFlagged && !cell.isMine) {
                    std::cout << "[X]"; // Misplaced flag indicated on game over
                } else if (cell.isFlagged) {
                    std::cout << "[F]";
                } else if (!cell.isRevealed) {
                    std::cout << "[ ]";
                } else if (cell.isMine) {
                    std::cout << "[*]";
                } else if (cell.adjacentMines > 0) {
                    std::cout << "[" << cell.adjacentMines << "]";
                } else {
                    std::cout << "[.]";
                }
                std::cout << " ";
            }
            std::cout << "|\n";
        }

        // Lower border
        std::cout << "   +";
        for (int c = 0; c < cols; ++c) {
            std::cout << "----";
        }
        std::cout << "+\n";
    }
};

// Strict command parser for game actions (e.g., "DC3", "FC3", or "QUIT" / "Q")
UserCommand parseCommand(const std::string& rawInput, int maxRows, int maxCols) {
    UserCommand cmd;
    std::string trimmedInput = trim(rawInput);

    if (trimmedInput.empty()) {
        cmd.errorMessage = "Input cannot be empty.";
        return cmd;
    }

    // Check for QUIT or Q command (case-insensitive)
    std::string uppercaseInput;
    for (char ch : trimmedInput) {
        uppercaseInput.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }

    if (uppercaseInput == "QUIT" || uppercaseInput == "Q") {
        cmd.isQuit = true;
        cmd.valid = true;
        cmd.action = 'Q';
        return cmd;
    }

    // Remove internal spaces for coordinate parsing (e.g., D C 3 -> DC3)
    std::string s;
    for (char ch : trimmedInput) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            s.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }

    if (s.empty()) {
        cmd.errorMessage = "Input cannot consist only of whitespace.";
        return cmd;
    }

    // Also check if stripped string is QUIT or Q (e.g., "Q U I T")
    if (s == "QUIT" || s == "Q") {
        cmd.isQuit = true;
        cmd.valid = true;
        cmd.action = 'Q';
        return cmd;
    }

    // Action prefix validation
    cmd.action = s[0];
    if (cmd.action != 'D' && cmd.action != 'F') {
        cmd.errorMessage = "Invalid command '" + trimmedInput + 
                           "'. Must start with 'D' (Dig), 'F' (Flag), or be 'QUIT' / 'Q'.";
        return cmd;
    }

    // Column letter validation
    if (s.length() < 2 || !std::isalpha(static_cast<unsigned char>(s[1]))) {
        cmd.errorMessage = "Missing or invalid column letter after action prefix.";
        return cmd;
    }

    char colChar = s[1];
    cmd.col = colChar - 'A';

    char maxColChar = static_cast<char>('A' + maxCols - 1);
    if (cmd.col < 0 || cmd.col >= maxCols) {
        cmd.errorMessage = "Column letter '" + std::string(1, colChar) + 
                           "' out of range. Valid columns are A through " + std::string(1, maxColChar) + ".";
        return cmd;
    }

    // Row number validation
    if (s.length() < 3) {
        cmd.errorMessage = "Missing row number after column letter.";
        return cmd;
    }

    std::string rowStr = s.substr(2);
    for (char c : rowStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            cmd.errorMessage = "Row specification '" + rowStr + "' contains non-digit characters.";
            return cmd;
        }
    }

    try {
        int parsedRow = std::stoi(rowStr);
        cmd.row = parsedRow - 1; // 0-indexed internal conversion
    } catch (...) {
        cmd.errorMessage = "Row number '" + rowStr + "' is invalid or out of integer range.";
        return cmd;
    }

    if (cmd.row < 0 || cmd.row >= maxRows) {
        cmd.errorMessage = "Row number " + std::to_string(cmd.row + 1) + 
                           " out of range. Valid rows are 1 through " + std::to_string(maxRows) + ".";
        return cmd;
    }

    cmd.valid = true;
    return cmd;
}

// Prompt and validate difficulty selection or QUIT command
Difficulty getValidDifficultySelection() {
    while (true) {
        std::cout << "\n============================================\n";
        std::cout << "        C++17 CONSOLE MINESWEEPER           \n";
        std::cout << "============================================\n";
        std::cout << "Select difficulty level:\n";
        std::cout << " 1. Easy   (8x10,  10 mines)\n";
        std::cout << " 2. Medium (14x18, 40 mines)\n";
        std::cout << " 3. Hard   (20x24, 99 mines)\n";
        std::cout << "Type QUIT or Q to exit\n";
        std::cout << "Choice (1-3 or QUIT): ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput stream interrupted. Exiting.\n";
            return Difficulty::Quit;
        }

        std::string input = trim(line);
        std::string upperInput;
        for (char ch : input) {
            upperInput.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }

        if (upperInput == "QUIT" || upperInput == "Q") return Difficulty::Quit;
        if (input == "1") return Difficulty::Easy;
        if (input == "2") return Difficulty::Medium;
        if (input == "3") return Difficulty::Hard;

        std::cout << "\n[!] Invalid input: '" << input << "'. Enter 1, 2, 3, or QUIT.\n";
    }
}

// Prompt and validate game command (Dig, Flag, or Quit)
UserCommand getValidUserCommand(const MinesweeperBoard& board) {
    while (true) {
        std::cout << "\nEnter command (e.g., DC3 to Dig C3, FC3 to Flag C3, or QUIT / Q to exit): ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nInput stream interrupted.\n";
            UserCommand cmd;
            cmd.isQuit = true;
            cmd.valid = true;
            cmd.action = 'Q';
            return cmd;
        }

        UserCommand cmd = parseCommand(input, board.getRows(), board.getCols());
        if (cmd.valid) {
            return cmd;
        }

        std::cout << "\n[!] Error: " << cmd.errorMessage << "\n    Please try again.\n";
    }
}

// Prompt and validate restart / quit choice
bool getValidPlayAgainChoice() {
    while (true) {
        std::cout << "\nWould you like to play again? (R = Restart, Q or QUIT = Quit): ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;
        }

        std::string input = trim(line);
        std::string upperInput;
        for (char ch : input) {
            upperInput.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }

        if (upperInput == "QUIT" || upperInput == "Q") return false;
        if (upperInput == "R" || upperInput == "RESTART") return true;

        std::cout << "\n[!] Invalid selection: '" << input << "'. Enter 'R' to restart or 'QUIT' to exit.\n";
    }
}

int main() {
    bool keepPlaying = true;

    while (keepPlaying) {
        Difficulty diff = getValidDifficultySelection();
        if (diff == Difficulty::Quit) {
            break;
        }

        MinesweeperBoard board;
        board.setup(diff);

        // Timer will start once the first move is completed
        bool timerStarted = false;
        auto startTime = std::chrono::steady_clock::now();

        while (!board.isGameOver()) {
            long long elapsedSeconds = 0;
            if (timerStarted) {
                auto currentTime = std::chrono::steady_clock::now();
                elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - startTime).count();
            }

            board.display(elapsedSeconds);

            UserCommand cmd = getValidUserCommand(board);
            if (cmd.isQuit) {
                keepPlaying = false;
                break;
            }

            if (cmd.action == 'D') {
                board.digCell(cmd.row, cmd.col);
            } else if (cmd.action == 'F') {
                board.toggleFlag(cmd.row, cmd.col);
            }

            if (!timerStarted && board.isFirstMoveDone()) {
                startTime = std::chrono::steady_clock::now();
                timerStarted = true;
            }
        }

        if (!keepPlaying) break;

        // Calculate total time spent
        long long totalSeconds = 0;
        if (timerStarted) {
            auto endTime = std::chrono::steady_clock::now();
            totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                endTime - startTime).count();
        }

        board.display(totalSeconds);

        if (board.isGameWon()) {
            std::cout << "\n============================================\n";
            std::cout << "  CONGRATULATIONS! You cleared all mines!   \n";
            std::cout << "  Total Time: " << totalSeconds << " seconds.\n";
            std::cout << "============================================\n";
        } else {
            std::cout << "\n============================================\n";
            std::cout << "  BOOM! You hit a mine. Game Over!         \n";
            std::cout << "============================================\n";
        }

        keepPlaying = getValidPlayAgainChoice();
    }

    std::cout << "\nThank you for playing C++17 Console Minesweeper!\n";
    return 0;
}
