# graph_solution

A small C++ command-line tool that reads a directed graph (as a CSV edge
list) and reports:

- `is_dag` — whether the graph has no cycles
- `max_in_degree` / `max_out_degree`
- `pr_max` / `pr_min` — highest and lowest PageRank scores after 20
  iterations (damping factor 0.85)

## Repo structure

```
graph_solution/
├── graph_solution 
├── Makefile
├── src/
│   └── main.cpp 
├── test_cases/
│   ├── graph1.csv
│   ├── graph1_output.txt
│   ├── graph2.csv
│   ├── graph2_output.txt
│   ├── graph3.csv
│   └── graph3_output.txt
├── bin/  
└── README.md
```

How the pieces fit together:

- **`src/main.cpp`** is the only source file. It parses the CSV, builds the
  graph, and computes all five metrics.
- **`Makefile`** just compiles `src/main.cpp` into `bin/graph_solution_bin`.
- **`graph_solution`** (no file extension) is the file the spec asks you to
  run. It is a tiny shell script, *not* the compiled program. All it does
  is: "if the binary doesn't exist yet, build it (via `make`), then run it
  with whatever arguments you passed." This is why you never have to type
  `g++` or `make` yourself.

## Requirements

- A `g++` compiler supporting C++17 (any reasonably recent `g++` works).
- `make` (usually already installed on Linux/macOS/WSL). If it's missing,
  the wrapper script falls back to calling `g++` directly, so `make` is
  convenient but not strictly required.

No external/third-party libraries are used — only the C++ standard library.
The PageRank algorithm and cycle-detection (DAG check) are implemented from
scratch in `src/main.cpp`, as required by the spec.

## Running it

```bash
git clone <your-repo-url>
cd graph_solution
chmod +x graph_solution        # only needed if the +x bit was lost, e.g. after zip/unzip
./graph_solution test_cases/graph1.csv
```

Expected output:

```
is_dag: true
max_in_degree: 2
max_out_degree: 2
pr_max: 0.470608
pr_min: 0.137504
```

The first run automatically compiles the program (you'll see a short
"Building binary..." message on stderr) and caches the binary in `bin/`.
Every run after that reuses the cached binary and starts instantly. If you
ever edit `src/main.cpp`, the next run will notice the source changed and
rebuild automatically.

### Checking against all provided test cases

```bash
for f in test_cases/graph*.csv; do
    name=$(basename "$f" .csv)
    echo "=== $name ==="
    diff <(./graph_solution "$f") "test_cases/${name}_output.txt" && echo "MATCH"
done
```

## Setup on Windows (VS Code)

The `graph_solution` script is a bash script, so it needs a *nix-style shell.
The two easiest options:

**Option A — WSL (recommended)**

1. Install WSL if you don't have it already: open PowerShell as
   Administrator and run `wsl --install`, then restart when prompted.
2. Install a compiler inside WSL (Ubuntu, by default):
   ```bash
   sudo apt update && sudo apt install -y build-essential
   ```
3. In VS Code, install the **WSL** extension, then use
   `File > Open Folder...` and pick "Show local" → `\\wsl$\Ubuntu\home\<you>\...`,
   or just open a WSL terminal (`View > Terminal`, then switch the terminal
   dropdown to "Ubuntu (WSL)") and `cd` to wherever you cloned the repo.
4. From that WSL terminal, run the commands in "Running it" above exactly
   as written.

**Option B — Git Bash**

If you have Git for Windows installed, it comes with Git Bash, which can
run this script directly, as long as you also have a `g++` available on
your PATH (e.g. via [MSYS2](https://www.msys2.org/) or
[MinGW-w64](https://www.mingw-w64.org/)). Open Git Bash in the project
folder and run the same commands from "Running it".

If neither is available, you can still build and run manually without the
wrapper script:

```bash
g++ -O2 -std=c++17 -o bin/graph_solution_bin src/main.cpp
./bin/graph_solution_bin test_cases/graph1.csv
```

## Input format

Each line of the input file is one directed edge:

```csv
source_node_id,target_node_id
```

Both ids are non-negative integers. Blank lines are skipped. Nodes are
inferred entirely from the edges present in the file (a node with no
edges at all can never appear, per the spec).

## Notes on the algorithms (`src/main.cpp`)

- **DAG check**: iterative depth-first search with three-color marking
  (white/gray/black). A "gray" node revisited while still on the current
  DFS path means there's a back-edge, i.e. a cycle. Iterative (not
  recursive) so it won't stack-overflow on graphs with long chains.
- **Degrees**: straightforward counting pass over the adjacency list.
- **PageRank**: `PR(j) = (1-d)/N + d * Σ_i P(i,j) * PR(i)`, run for exactly
  20 iterations starting from a uniform distribution `1/N`. Dangling nodes
  (no outgoing edges) are treated as if they link to every node equally
  (`1/N` each), so their PageRank mass is spread evenly across the whole
  graph on every iteration, keeping the transition matrix row-stochastic.
  The matrix itself is never built explicitly (it would be dense and
  wasteful for a sparse graph) — the same math is computed by pushing each
  node's current rank along its real outgoing edges, plus adding every
  node's fair share of the pooled dangling mass.
