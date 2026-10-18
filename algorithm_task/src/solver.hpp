#pragma once

#include "graph.hpp"
#include <optional>
#include <vector>

struct Config {
    double daily_fee = 10.0;
    int start_vertex = 0;
    double alpha = 0.7;      // вес нижней оценки вероятности
    double discount = 0.97;  // дисконтирование для длинных путей
};

class Solver {
public:
    Solver(Graph graph, Config config);

    void run();

private:
    struct Candidate {
        int edge_id;
        int target_v;
        double ev;
    };

    struct ClusterResult {
        double ev;
        std::vector<double> p_low;
    };

    Graph g_;
    Config cfg_;
    int curr_v_ = 0;
    std::vector<bool> visited_;
    std::vector<bool> looked_;

    void do_look(int u);
    bool do_move(int edge_id);
    void do_clear(int edge_id);
    void do_end();

    void dfs_explore(int u);
    void move_along_open(int target);
    std::vector<int> get_open_path(int src, int dst) const;

    std::optional<Candidate> find_best_clear() const;
    ClusterResult calc_cluster(int root) const;
    double calc_corridor_bonus(
        int start_v,
        const std::vector<double>& cluster_ev,
        const std::vector<double>& root_p_low,
        double comp_limit) const;
};
