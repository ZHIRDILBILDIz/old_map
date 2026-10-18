#pragma once

#include <iostream>
#include <string>
#include <vector>

enum class EdgeState {
    Unknown,
    Intact,
    Collapsed
};

struct Edge {
    int id = 0;
    int u = 0;
    int v = 0;
    double prob = 0.0;
    EdgeState state = EdgeState::Unknown;

    int other(int from) const {
        return (from == u) ? v : u;
    }

    double current_prob() const {
        if (state == EdgeState::Intact) return 1.0;
        if (state == EdgeState::Collapsed) return 0.0;
        return prob;
    }
};

struct Graph {
    int n = 0;
    int m = 0;
    std::vector<double> profit;
    std::vector<Edge> edges;           // 1-индексация (1..m)
    std::vector<std::vector<int>> adj; // списки id ребер для каждой вершины (0..n-1)

    static Graph read(std::istream& in) {
        Graph g;
        if (!(in >> g.n >> g.m)) {
            return g;
        }

        g.profit.resize(g.n);
        for (int i = 0; i < g.n; ++i) {
            std::string tok;
            in >> tok;
            if (!tok.empty() && tok.back() == ',') {
                tok.pop_back();
            }
            g.profit[i] = std::stod(tok);
        }

        g.edges.resize(g.m + 1);
        g.adj.resize(g.n);

        for (int id = 1; id <= g.m; ++id) {
            int u, v;
            double p;
            in >> u >> v >> p;
            --u;
            --v;
            g.edges[id] = Edge{id, u, v, p, EdgeState::Unknown};
            g.adj[u].push_back(id);
            if (u != v) {
                g.adj[v].push_back(id);
            }
        }
        return g;
    }
};
