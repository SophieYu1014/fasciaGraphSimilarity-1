using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <tuple>
#include <chrono>

/*
This file is an independent compilable file that generates both correlated and independent graphs

using bernoulli sampling method.

The parameters are

rho - relation between graphs
q - graph density
n - node count
m - generate m graphs (adjustable)

*/

void write_graph_to_file(int n, vector<tuple<int, int>> edges, char filename[100])
{
    ofstream file(filename);
    file << n << '\n';
    file << edges.size();
    for (auto it = edges.begin(); it != edges.end(); ++it) {
        file << '\n' << get<0>(*it) << ' ' << get<1>(*it);
    }
    file.close();
}

vector<float> get_chance(float rho, float q)
{
    float pos_1 = q*q + rho*(1-q)*q;
    float pos_2 = q;
    float pos_3 = 2*q - pos_1;
    return vector<float> { pos_1, pos_2, pos_3 };
}

void generate_graph_new(int n, float rho, float q, char firstFile[100], char secondFile[100])
{
    // generates erdos-renyi graph with n nodes
    // Generate both subgraph using algorithm with p (rho), q.

    vector<float> chances = get_chance(rho, q);
    vector <tuple<int, int>> firstEdges;
    vector <tuple<int, int>> secondEdges;
    tuple<int, int> tup;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float r = ((float) rand() / (RAND_MAX));
            tup = make_tuple(i, j);
            if (r < chances[0]) {
                firstEdges.push_back(tup);
                secondEdges.push_back(tup);
            } else if (r < chances[1]) {
                firstEdges.push_back(tup);
            } else if (r < chances[2]) {
                secondEdges.push_back(tup);
            }
        }
    }

    write_graph_to_file(n, firstEdges, firstFile);
    write_graph_to_file(n, secondEdges, secondFile);
}

void generate_corr_graphs(int n, float rho, float q, int m_rep) {
    const char folder [] = "correlated_graphs/";
    char graphA [100];
    sprintf(graphA, "%s%d_%dA_rho%.5f_q%.5f_corr.txt", folder, m_rep, n, rho, q);
    char graphB [100];
    sprintf(graphB, "%s%d_%dB_rho%.5f_q%.5f_corr.txt", folder, m_rep, n, rho, q);
    generate_graph_new(n, rho, q, graphA, graphB);
}

void generate_ind_graphs(int n, float q, int m_rep) {
    const char folder [] = "independent_graphs/";
    char graphA [100];
    sprintf(graphA, "%s%d_%dA_q%.5f_ind.txt", folder, m_rep, n, q);
    char graphB [100];
    sprintf(graphB, "%s%d_%dB_q%.5f_ind.txt", folder, m_rep, n, q);
    generate_graph_new(n, 0, q, graphA, graphB);
}

void generate_all_graphs(int n, float rho, float q, int m, bool i) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    auto a1 = high_resolution_clock::now();

    for(int m_rep = 1; m_rep < m+1; ++m_rep) {
	if (i) generate_ind_graphs(n, q, m_rep);
	else generate_corr_graphs(n, rho, q, m_rep);
    }

    auto a2 = high_resolution_clock::now();
    auto ms_int = duration_cast<milliseconds>(a2-a1);
    duration<double, std::milli> ms_double = a2 - a1;
    std::cout << "\nTime to generate graphs: " << ms_double.count() << "ms";
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    int n;
    int m;
    float rho;
    float q;
    bool gi = false;

    char c;
    int argCount = 0;
    while ((c = getopt(argc, argv, "n:m:r:q:i")) != -1) {
        switch(c) {
            case 'n':
                n = atoi(optarg);
                argCount++;
                printf("Generating graph of %d nodes\n", n);
                break;
            case 'm':
                m = atoi(optarg);
                argCount++;
                printf("Generating %d pair of graphs\n", m);
                break;
            case 'r':
                rho = atof(optarg);
                argCount++;
                printf("Setting parameter rho to %.5f\n", rho);
                break;
            case 'q':
                q = atof(optarg);
                argCount++;
                printf("Setting parameter q to %.5f\n", q);
                break;
	    case 'i':
		gi = true;
		argCount++;
		break;
            default:
                abort();
        }
    }
    if (argCount < 4) {
        std::cout << "YOU DID NOT PROVIDE ALL REQUIRED ARGUMENTS!" << std::endl;
        abort();
    }
    generate_all_graphs(n, rho, q, m, gi);
}
