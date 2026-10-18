#include "solver.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--daily-fee" && i + 1 < argc) {
            cfg.daily_fee = std::stod(argv[++i]);
        } else if (arg == "--start" && i + 1 < argc) {
            cfg.start_vertex = std::stoi(argv[++i]) - 1;
        }
    }

    Graph g = Graph::read(std::cin);
    if (g.n == 0) {
        return 0;
    }

    Solver solver(std::move(g), cfg);
    solver.run();

    return 0;
}
