#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <cstdint>

struct Graph {
    std::vector<int64_t> index_to_id;                 
    std::unordered_map<int64_t, int> id_to_index;

    std::vector<std::vector<int>> out_edges;

    int node_count() const { return static_cast<int>(index_to_id.size()); }
};


static std::string trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

static int get_or_create_index(Graph& g, int64_t raw_id) {
    auto it = g.id_to_index.find(raw_id);
    if (it != g.id_to_index.end()) {
        return it->second;
    }
    int new_index = g.node_count();
    g.id_to_index[raw_id] = new_index;
    g.index_to_id.push_back(raw_id);
    g.out_edges.emplace_back();
    return new_index;
}

static Graph parse_graph(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: could not open input file: " << path << std::endl;
        std::exit(1);
    }

    Graph g;
    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        line_number++;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue; // skip blank lines

        std::stringstream ss(trimmed);
        std::string source_str, target_str;

        if (!std::getline(ss, source_str, ',') || !std::getline(ss, target_str, ',')) {
            std::cerr << "Warning: skipping malformed line " << line_number
                      << ": \"" << line << "\"" << std::endl;
            continue;
        }

        source_str = trim(source_str);
        target_str = trim(target_str);
        if (source_str.empty() || target_str.empty()) {
            std::cerr << "Warning: skipping malformed line " << line_number
                      << ": \"" << line << "\"" << std::endl;
            continue;
        }

        int64_t source_id, target_id;
        try {
            source_id = std::stoll(source_str);
            target_id = std::stoll(target_str);
        } catch (...) {
            std::cerr << "Warning: skipping non-numeric line " << line_number
                      << ": \"" << line << "\"" << std::endl;
            continue;
        }

        int u = get_or_create_index(g, source_id);
        int v = get_or_create_index(g, target_id);
        g.out_edges[u].push_back(v);
    }

    return g;
}

enum Color { WHITE, GRAY, BLACK };

static bool is_dag(const Graph& g) {
    int n = g.node_count();
    std::vector<Color> color(n, WHITE);

    for (int start = 0; start < n; start++) {
        if (color[start] != WHITE) continue;

        std::vector<int> node_stack;
        std::vector<size_t> edge_pos_stack;
        node_stack.push_back(start);
        edge_pos_stack.push_back(0);
        color[start] = GRAY;

        while (!node_stack.empty()) {
            int u = node_stack.back();
            size_t& pos = edge_pos_stack.back();

            if (pos < g.out_edges[u].size()) {
                int v = g.out_edges[u][pos];
                pos++;

                if (color[v] == GRAY) {
                    return false; // back-edge found -> cycle
                } else if (color[v] == WHITE) {
                    color[v] = GRAY;
                    node_stack.push_back(v);
                    edge_pos_stack.push_back(0);
                }
                // if BLACK, already fully explored, nothing to do
            } else {
                color[u] = BLACK;
                node_stack.pop_back();
                edge_pos_stack.pop_back();
            }
        }
    }

    return true;
}

static void compute_degrees(const Graph& g, int& max_in_degree, int& max_out_degree) {
    int n = g.node_count();
    std::vector<int> in_degree(n, 0);
    std::vector<int> out_degree(n, 0);

    for (int u = 0; u < n; u++) {
        out_degree[u] = static_cast<int>(g.out_edges[u].size());
        for (int v : g.out_edges[u]) {
            in_degree[v]++;
        }
    }

    max_in_degree = 0;
    max_out_degree = 0;
    for (int i = 0; i < n; i++) {
        max_in_degree = std::max(max_in_degree, in_degree[i]);
        max_out_degree = std::max(max_out_degree, out_degree[i]);
    }
}

static void compute_pagerank(const Graph& g, double& pr_max, double& pr_min) {
    const double d = 0.85;
    const int iterations = 20;
    int n = g.node_count();

    if (n == 0) {
        pr_max = 0.0;
        pr_min = 0.0;
        return;
    }

    std::vector<double> pr(n, 1.0 / n);   // uniform initial distribution
    std::vector<double> out_degree(n, 0.0);
    for (int i = 0; i < n; i++) out_degree[i] = static_cast<double>(g.out_edges[i].size());

    for (int iter = 0; iter < iterations; iter++) {
        std::vector<double> next(n, 0.0);

        double dangling_sum = 0.0;
        for (int i = 0; i < n; i++) {
            if (out_degree[i] == 0.0) dangling_sum += pr[i];
        }

        double base = (1.0 - d) / n + d * (dangling_sum / n);
        for (int j = 0; j < n; j++) next[j] = base;

        for (int i = 0; i < n; i++) {
            if (out_degree[i] == 0.0) continue; // already handled above
            double share = d * pr[i] / out_degree[i];
            for (int v : g.out_edges[i]) {
                next[v] += share;
            }
        }

        pr = next;
    }

    pr_max = pr[0];
    pr_min = pr[0];
    for (int i = 1; i < n; i++) {
        pr_max = std::max(pr_max, pr[i]);
        pr_min = std::min(pr_min, pr[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " path/to/graph.csv" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    Graph g = parse_graph(path);

    bool dag = is_dag(g);

    int max_in_degree = 0, max_out_degree = 0;
    compute_degrees(g, max_in_degree, max_out_degree);

    double pr_max = 0.0, pr_min = 0.0;
    compute_pagerank(g, pr_max, pr_min);

    std::cout << "is_dag: " << (dag ? "true" : "false") << "\n";
    std::cout << "max_in_degree: " << max_in_degree << "\n";
    std::cout << "max_out_degree: " << max_out_degree << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "pr_max: " << pr_max << "\n";
    std::cout << "pr_min: " << pr_min << "\n";

    return 0;
}
