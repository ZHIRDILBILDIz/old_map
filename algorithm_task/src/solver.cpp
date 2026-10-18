#include "solver.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <unordered_map>

constexpr double kEpsProb = 1e-3;

Solver::Solver(Graph graph, Config config)
    : g_(std::move(graph)),
      cfg_(config),
      curr_v_(config.start_vertex),
      visited_(g_.n, false),
      looked_(g_.n, false) {}

void Solver::run() {
    while (true) {
        dfs_explore(curr_v_);

        auto cand = find_best_clear();
        if (!cand) {
            do_end();
            break;
        }

        const auto& e = g_.edges[cand->edge_id];
        int from_v = visited_[e.u] ? e.u : e.v;

        move_along_open(from_v);
        do_clear(cand->edge_id);

        // новый день начинается со входа
        curr_v_ = cfg_.start_vertex;
        move_along_open(cand->target_v);
    }
}

void Solver::do_look(int u) {
    std::cout << "LOOK" << std::endl;
    for (int eid : g_.adj[u]) {
        int status = 0;
        if (std::cin >> status) {
            g_.edges[eid].state = (status == 1) ? EdgeState::Intact : EdgeState::Collapsed;
        }
    }
    looked_[u] = true;
}

bool Solver::do_move(int edge_id) {
    std::cout << "MOVE " << edge_id << std::endl;
    int ok = 0;
    if (!(std::cin >> ok)) {
        return false;
    }
    g_.edges[edge_id].state = (ok == 1) ? EdgeState::Intact : EdgeState::Collapsed;
    return ok == 1;
}

void Solver::do_clear(int edge_id) {
    std::cout << "CLEAR " << edge_id << std::endl;
    g_.edges[edge_id].state = EdgeState::Intact;
}

void Solver::do_end() {
    std::cout << "END" << std::endl;
}

void Solver::dfs_explore(int u) {
    visited_[u] = true;
    if (!looked_[u]) {
        do_look(u);
    }

    for (int eid : g_.adj[u]) {
        int v = g_.edges[eid].other(u);
        if (visited_[v] || g_.edges[eid].state != EdgeState::Intact) {
            continue;
        }

        if (do_move(eid)) {
            curr_v_ = v;
            dfs_explore(v);

            if (curr_v_ != u) {
                do_move(eid);
                curr_v_ = u;
            }
        }
    }
}

void Solver::move_along_open(int target) {
    if (curr_v_ == target) return;

    for (int eid : get_open_path(curr_v_, target)) {
        int nxt = g_.edges[eid].other(curr_v_);
        if (do_move(eid)) {
            curr_v_ = nxt;
            visited_[curr_v_] = true;
        } else {
            break;
        }
    }
}

std::vector<int> Solver::get_open_path(int src, int dst) const {
    std::vector<int> pv(g_.n, -1);
    std::vector<int> pe(g_.n, -1);
    std::queue<int> q;

    q.push(src);
    pv[src] = src;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        if (u == dst) break;

        for (int eid : g_.adj[u]) {
            if (g_.edges[eid].state != EdgeState::Intact) continue;
            int v = g_.edges[eid].other(u);
            if (pv[v] == -1) {
                pv[v] = u;
                pe[v] = eid;
                q.push(v);
            }
        }
    }

    std::vector<int> path;
    if (pv[dst] == -1) return path;

    for (int v = dst; v != src; v = pv[v]) {
        path.push_back(pe[v]);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::optional<Solver::Candidate> Solver::find_best_clear() const {
    // берем по одному заваленному ребру на каждую граничную вершину
    std::unordered_map<int, int> border;
    for (int u = 0; u < g_.n; ++u) {
        if (!visited_[u]) continue;
        for (int eid : g_.adj[u]) {
            int v = g_.edges[eid].other(u);
            if (!visited_[v] && g_.edges[eid].state == EdgeState::Collapsed) {
                border.try_emplace(v, eid);
            }
        }
    }

    if (border.empty()) return std::nullopt;

    // считаем суммарный профит в непосещенных компонентах связности
    std::vector<int> comp_id(g_.n, -1);
    std::vector<double> comp_sum;
    for (const auto& [sv, _] : border) {
        if (comp_id[sv] != -1) continue;
        int cid = static_cast<int>(comp_sum.size());
        double s = 0.0;

        std::queue<int> q;
        q.push(sv);
        comp_id[sv] = cid;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            s += g_.profit[u];
            for (int eid : g_.adj[u]) {
                int nxt = g_.edges[eid].other(u);
                if (!visited_[nxt] && comp_id[nxt] == -1) {
                    comp_id[nxt] = cid;
                    q.push(nxt);
                }
            }
        }
        comp_sum.push_back(s);
    }

    std::vector<double> cluster_ev(g_.n, 0.0);
    for (int u = 0; u < g_.n; ++u) {
        if (!visited_[u] && comp_id[u] != -1 && comp_sum[comp_id[u]] > cfg_.daily_fee) {
            cluster_ev[u] = calc_cluster(u).ev;
        }
    }

    std::optional<Candidate> best;

    for (const auto& [v, eid] : border) {
        int cid = comp_id[v];
        if (cid == -1 || comp_sum[cid] <= cfg_.daily_fee) continue;

        auto res = calc_cluster(v);
        double base_ev = res.ev - cfg_.daily_fee;
        double bonus = calc_corridor_bonus(v, cluster_ev, res.p_low, comp_sum[cid]);
        double total = base_ev + std::max(0.0, bonus);

        if (total > 0.0 && (!best || total > best->ev)) {
            best = Candidate{eid, v, total};
        }
    }

    return best;
}

Solver::ClusterResult Solver::calc_cluster(int root) const {
    std::vector<double> p_low(g_.n, 0.0);
    std::vector<double> p_high(g_.n, 0.0);
    std::vector<bool> used(g_.n, false);

    using P = std::pair<double, int>;
    std::priority_queue<P> pq;

    p_low[root] = 1.0;
    p_high[root] = 1.0;
    pq.emplace(1.0, root);

    while (!pq.empty()) {
        auto [prob, u] = pq.top();
        pq.pop();

        if (used[u]) continue;
        used[u] = true;

        for (int eid : g_.adj[u]) {
            int v = g_.edges[eid].other(u);
            if (visited_[v]) continue;

            double ep = g_.edges[eid].current_prob();
            double np = prob * ep;
            if (np < kEpsProb) continue;

            if (np > p_low[v]) {
                p_low[v] = np;
                pq.emplace(p_low[v], v);
            }

            if (!used[v]) {
                p_high[v] = 1.0 - (1.0 - p_high[v]) * (1.0 - p_high[u] * ep);
            }
        }
    }

    double ev = 0.0;
    for (int v = 0; v < g_.n; ++v) {
        if (!visited_[v] && used[v]) {
            double p = cfg_.alpha * p_low[v] + (1.0 - cfg_.alpha) * p_high[v];
            ev += g_.profit[v] * p;
        }
    }

    return {ev, std::move(p_low)};
}

double Solver::calc_corridor_bonus(
    int start_v,
    const std::vector<double>& cluster_ev,
    const std::vector<double>& root_p_low,
    double comp_limit) const {
    constexpr double kInf = std::numeric_limits<double>::infinity();
    std::vector<double> dist(g_.n, kInf);

    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<>> pq;

    dist[start_v] = 0.0;
    pq.emplace(0.0, start_v);

    double best_bonus = 0.0;

    while (!pq.empty()) {
        auto [days, u] = pq.top();
        pq.pop();

        if (days > dist[u] + 1e-12) continue;
        if ((1.0 + days) * cfg_.daily_fee >= comp_limit) break;

        if (u != start_v && days > 1e-9) {
            double rem_prob = std::max(0.0, 1.0 - root_p_low[u]);
            double gain = std::pow(cfg_.discount, days) * rem_prob * cluster_ev[u] - cfg_.daily_fee * days;
            best_bonus = std::max(best_bonus, gain);
        }

        for (int eid : g_.adj[u]) {
            int v = g_.edges[eid].other(u);
            if (visited_[v]) continue;

            double w = 1.0 - g_.edges[eid].current_prob();
            if (days + w < dist[v]) {
                dist[v] = days + w;
                pq.emplace(dist[v], v);
            }
        }
    }

    return best_bonus;
}
