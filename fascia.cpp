// Copyright (c) 2013, The Pennsylvania State University.
// All rights reserved.
// 
// See COPYING for license.

using namespace std;

#include <stdio.h>
#include <cstdlib>
#include <assert.h>
#include <fstream>
#include <math.h>
#include <sys/time.h>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>
#include <climits>
#include <numeric>

#include <stdlib.h>
#include <random>
#include <string>
#include <tuple>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "fascia.h"
#include "graph.hpp"
#include "util.hpp"
#include "output.hpp"
#include "dynamic_table.hpp"
#include "dynamic_table_array.hpp"
#include "partitioner.hpp"
#if SIMPLE
  #include "colorcount_simple.hpp"
#else
  #include "colorcount.hpp"
#endif

bool timing = false;

void print_info_short(char* name)
{
  printf("\nTo run: %s [-g graphfile] [-t template || -b batchfile] [options]\n", name);
  printf("Help: %s -h\n\n", name);

  exit(0);
}

void print_info(char* name)
{
  printf("\nTo run: %s [-g graphfile] [-t template || -b batchfile] [options]\n\n", name);

  printf("\tgraphfile = \n");
  printf("\t\tn\n");
  printf("\t\tm\n");
  printf("\t\tv0 v1\n");
  printf("\t\tv0 v2\n");
  printf("\t\t...\n");
  printf("\t\t(zero indexed)\n\n");

  printf("\tgraphfile (if labeled) = \n");
  printf("\t\tn\n");
  printf("\t\tm\n");
  printf("\t\tlabel_v0\n");
  printf("\t\tlabel_v1\n");
  printf("\t\t...\n");
  printf("\t\tv0 v1\n");
  printf("\t\tv0 v2\n");
  printf("\t\t...\n");
  printf("\t\t(zero indexed)\n\n"); 

  printf("\ttemplate =\n");
  printf("\t\tsame format as graphfile\n\n");

  printf("\tbatchfile =\n");
  printf("\t\ttemplate1\n");
  printf("\t\ttemplate2\n");
  printf("\t\t...\n");
  printf("\t\t(must supply only one of template file or batchfile)\n\n");

  printf("\toptions = \n");
  printf("\t\t-m  [#], compute counts for motifs of size #\n");
  printf("\t\t-o  Use outerloop parallelization\n");
  printf("\t\t-l  Graph and template are labeled\n");
  printf("\t\t-i  [# iterations], default: 1\n");
  printf("\t\t-c  Output per-vertex counts to [template].vert\n");
  printf("\t\t-d  Output graphlet degree distribution to [template].gdd\n");
  printf("\t\t-a  Do not calculate automorphism of template\n");
  printf("\t\t\t(recommended when template size > 10)\n");
  printf("\t\t-r  Report runtime\n");
  printf("\t\t-v  Verbose output\n");
  printf("\t\t-h  Print this\n\n");

  exit(0);
}

void read_in_graph(Graph& g, char* graph_file, bool labeled,
  int*& srcs_g, int*& dsts_g, int*& labels_g)
{
  ifstream file_g;
  string line;
  
  file_g.open(graph_file);
  
  int n_g;
  int m_g;    

  getline(file_g, line);
  n_g = atoi(line.c_str());
  getline(file_g, line);
  m_g = atoi(line.c_str());
  
  srcs_g = new int[m_g];
  dsts_g = new int[m_g];

  if (labeled)
  {
    labels_g = new int[n_g];
    for (int i = 0; i < n_g; ++i)
    {
      getline(file_g, line);
      labels_g[i] = atoi(line.c_str());
    }
  }
  else
  {
    labels_g = NULL;
  }
  
  for (int  i = 0; i < m_g; ++i)
  {
    getline(file_g, line, ' ');   
    srcs_g[i] = atoi(line.c_str());
    getline(file_g, line);  
    dsts_g[i] = atoi(line.c_str());
  } 
  file_g.close();
  
  g.init(n_g, m_g, srcs_g, dsts_g);
  //print_my_graph(g);
}

void run_single(char* graph_file, char* template_file, bool labeled,
                bool do_vert, bool do_gdd,
                int iterations, 
                bool do_outerloop, bool calc_auto, bool verbose)
{
  Graph g;
  Graph t;
  int* srcs_g;
  int* dsts_g;
  int* labels_g;
  int* srcs_t;
  int* dsts_t;
  int* labels_t;
  char* vert_file = new char[1024];
  char* gdd_file = new char[1024];

  if (do_vert) {
    strcat(vert_file, template_file);
    strcat(vert_file, ".vert");
  }
  if (do_gdd) {
    strcat(gdd_file, template_file);
    strcat(gdd_file, ".gdd");
  }

  read_in_graph(g, graph_file, labeled, srcs_g, dsts_g, labels_g);
  read_in_graph(t, template_file, labeled, srcs_t, dsts_t, labels_t);

  double elt = 0.0;
  if (timing || verbose) {
    elt = timer();
  }
  double full_count = 0.0;  
  if (do_outerloop)
  {
    int num_threads = omp_get_max_threads();
    int iter = ceil( (double)iterations / (double)num_threads + 0.5);
    
    colorcount* graph_count = new colorcount[num_threads];
    for (int tid = 0; tid < num_threads; ++tid) {
      graph_count[tid].init(g, labels_g, labeled, 
                            calc_auto, do_gdd, do_vert, verbose);
    }

    double** vert_counts;
    if (do_gdd || do_vert)
      vert_counts = new double*[num_threads];

#pragma omp parallel reduction(+:full_count)
{
    int tid = omp_get_thread_num();
    full_count += graph_count[tid].do_full_count(&t, labels_t, iter, false, 0);
    if (do_gdd || do_vert)
      vert_counts[tid] = graph_count[tid].get_vert_counts();
}   
    full_count /= (double)num_threads;
    if (do_gdd || do_vert)
    {
      output out(vert_counts, num_threads, g.num_vertices());
      if (do_gdd) {
        out.output_gdd(gdd_file);
        free(gdd_file);
      } 
      if (do_vert) {        
        out.output_verts(vert_file);
        free(vert_file);
      }
    }
  }
  else
  {
    colorcount graph_count;
    graph_count.init(g, labels_g, labeled, 
                      calc_auto, do_gdd, do_vert, verbose);
    full_count += graph_count.do_full_count(&t, labels_t, iterations, false, 0);

    if (do_gdd || do_vert)
    {
      double* vert_counts = graph_count.get_vert_counts();
      output out(vert_counts, g.num_vertices());
      if (do_gdd)
      {
        out.output_gdd(gdd_file);
        free(gdd_file);
      }
      if (do_vert)
      {
        out.output_verts(vert_file);
        free(vert_file);
      }
    }
  }

  printf("%e\n", full_count);

if (timing || verbose) {
  elt = timer() - elt;
  printf("Total time:\n\t%9.6lf seconds\n", elt);
}

  delete [] srcs_g;
  delete [] dsts_g;
  delete [] labels_g;
  delete [] srcs_t;
  delete [] dsts_t;
  delete [] labels_t;
}


std::vector<double> run_batch(char* graph_file, char* batch_file, bool labeled,
                bool do_vert, bool do_gdd,
                int iterations, 
                bool do_outerloop, bool calc_auto, bool verbose, bool random_graphs, float p)
{
  Graph g;
  Graph t;
  int* srcs_g;
  int* dsts_g;
  int* labels_g;
  int* srcs_t;
  int* dsts_t;
  int* labels_t;
  char* vert_file;
  char* gdd_file;

  std::vector<double> full_count_arr;

  read_in_graph(g, graph_file, labeled, srcs_g, dsts_g, labels_g);

  double elt = 0.0;
  if (timing || verbose) {
    elt = timer();
  }

  ifstream if_batch;
  string line;
  if_batch.open(batch_file);
  while (getline(if_batch, line))
  {   
    char* template_file = strdup(line.c_str());
    read_in_graph(t, template_file, labeled, srcs_t, dsts_t, labels_t);

    double full_count = 0.0;
    if (do_outerloop)
    {
      int num_threads = omp_get_max_threads();
      int iter = ceil( (double)iterations / (double)num_threads + 0.5);
      
      colorcount* graph_count = new colorcount[num_threads];
      for (int i = 0; i < num_threads; ++i) {
        graph_count[i].init(g, labels_g, labeled, 
                            calc_auto, do_gdd, do_vert, verbose);
      }

    
      double** vert_counts;
      if (do_gdd || do_vert)
        vert_counts = new double*[num_threads];

#pragma omp parallel reduction(+:full_count)
{
      int tid = omp_get_thread_num();
      full_count += graph_count[tid].do_full_count(&t, labels_t, iter, random_graphs, p);
      if (do_gdd || do_vert)
        vert_counts[tid] = graph_count[tid].get_vert_counts();
}   
      full_count /= (double)num_threads;
      if (do_gdd || do_vert)
      {
        output out(vert_counts, num_threads, g.num_vertices());
        if (do_gdd) {
          gdd_file = strdup(template_file);
          strcat(gdd_file, ".gdd");
          out.output_gdd(gdd_file);
          free(gdd_file);
        }
        if (do_vert) {
          vert_file = strdup(template_file);
          strcat(vert_file, ".vert");
          out.output_verts(vert_file);
          free(vert_file);
        }
      }
    }
    else
    {
      colorcount graph_count;
      graph_count.init(g, labels_g, labeled, 
                        calc_auto, do_gdd, do_vert, verbose);
      full_count += graph_count.do_full_count(&t, labels_t, iterations, random_graphs, p);
    }

    // printf("%e\n", full_count);  
    // check count_automorphissms
    // printf("num of automorphisms: %d\n", count_automorphisms(t));
    full_count_arr.push_back(full_count * sqrt(count_automorphisms(t)));


    delete [] srcs_t;
    delete [] dsts_t;
    delete [] labels_t;
    delete [] template_file;
  }

if (timing || verbose) {
  elt = timer() - elt;
  printf("Total time:\n\t%9.6lf seconds\n", elt);
}

  delete [] srcs_g;
  delete [] dsts_g;
  delete [] labels_g;

  return full_count_arr;
}


std::vector<double> run_motif(char* graph_file, int motif, 
                bool do_vert, bool do_gdd, 
                int iterations, 
                bool do_outerloop, bool calc_auto, bool verbose, bool random_graphs, float p)
{
  char* motif_batchfile = NULL;

  switch(motif)
  {
    case(3):
      motif_batchfile = strdup("motif/graphs_n3_1/batchfile");
      break;
    case(4):
      motif_batchfile = strdup("motif/graphs_n4_2/batchfile");
      break;
    case(5):
      motif_batchfile = strdup("motif/graphs_n5_3/batchfile");
      break;
    case(6):
      motif_batchfile = strdup("motif/graphs_n6_6/batchfile");
      break;
    case(7):
      motif_batchfile = strdup("motif/graphs_n7_11/batchfile");
      break;
    case(8):
      motif_batchfile = strdup("motif/graphs_n8_23/batchfile");
      break;
    case(9):
      motif_batchfile = strdup("motif/graphs_n9_47/batchfile");
      break;
    case(10):
      motif_batchfile = strdup("motif/graphs_n10_106/batchfile");
      break;
    default:
      break;
  }

  return run_batch(graph_file, motif_batchfile, false,
            do_vert, do_gdd,
            iterations, 
            do_outerloop, calc_auto, verbose, random_graphs, p);
}

void run_compare_graphs(char* graph_fileA, char* graph_fileB, int motif, 
                bool do_vert, bool do_gdd, 
                int iterations, 
                bool do_outerloop, bool calc_auto, bool verbose, bool random_graphs, float p)
{
  std::vector<double> a = run_motif(graph_fileA, motif, do_vert, do_gdd, iterations, do_outerloop, calc_auto, verbose, random_graphs, p);
  std::vector<double> b = run_motif(graph_fileB, motif, do_vert, do_gdd, iterations, do_outerloop, calc_auto, verbose, random_graphs, p);

  double stat = std::inner_product(std::begin(a), std::end(a), std::begin(b), 0.0);
  printf("%e", stat);
}

void generate_graph(int n, float p, char filename[50])
{
// generates erdos-renyi graph with n nodes, probability p, saves it to a file with number file_num

    ofstream file(filename);

    int count = 0;
    vector <tuple<int, int>> edges;
    tuple<int, int> tup;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float r = ((float) rand() / (RAND_MAX));
            if (r < p) {
                ++count;
                tup = make_tuple(i, j);
                edges.push_back(tup);
            }
        }
    }

    file << n << '\n';
    file << count;

    for (auto it = edges.begin(); it != edges.end(); ++it) {
        file << '\n' << get<0>(*it) << ' ' << get<1>(*it);
    }

    file.close();

}

void sim1() {
    //runs simulation 1

    float p = 0.01;
    char tree_file[] = "template.graph";

    int iterations = 1000;
    int m = 20;

    std::vector<int> m_sizes;

    m_sizes.push_back(100);
    m_sizes.push_back(500);
    m_sizes.push_back(1000);
    m_sizes.push_back(10000);

  

    for (const int& i : m_sizes) {
      cout << i << "\n";
      cout << "\n[";
      for(int j = 0; j < m; ++j) {
        char graph_file [50];
        sprintf (graph_file, "graphs/%d.txt", i);
        generate_graph(i, p, graph_file);
        
        run_single(graph_file, tree_file, false, false, false, iterations, true, true, false);
        cout << ", ";
        cout.flush();   


      }
      cout<<'\b';
      cout<<'\b';
      cout<<"]\n";
      cout.flush();      
    }
}

void select_edges(float s, int m_rep, char in [50], char out [50]) {

    int n;
    int num_edges;

    ifstream og_erd_ren(in);
    og_erd_ren >> n;
    og_erd_ren >> num_edges;

    int count = 0;
    vector <tuple<int, int>> edges;
    int edge_1;
    int edge_2;
    tuple<int, int> tup;

    while (og_erd_ren >> edge_1 >> edge_2) {
        float r = ((float) rand() / (RAND_MAX));
        if (r < s) {
            ++count;
            tup = make_tuple(edge_1, edge_2);
            edges.push_back(tup);
            }
    }
    og_erd_ren.close();

    ofstream out_file(out);

    out_file << n << '\n';
    out_file << count;

    for (auto it = edges.begin(); it != edges.end(); ++it) {
        out_file << '\n' << get<0>(*it) << ' ' << get<1>(*it);
    }
    out_file.close();

}

void generate_corr_graphs(int n, float p, float s, int m_rep) {
    const char folder [] = "sim2_corr/";
    char in [50];
    sprintf(in, "%s%d_og.txt", folder, m_rep);
    char graphA [100];
    sprintf(graphA, "%s%d_A_%.5f_%.5f_corr.txt", folder, m_rep, p, s);
    char graphB [100];
    sprintf(graphB, "%s%d_B_%.5f_%.5f_corr.txt", folder, m_rep, p, s);
    
    generate_graph(n, p, in);
    select_edges(s, m_rep, in, graphA);
    select_edges(s, m_rep, in, graphB);
}

void generate_ind_graphs(int n, float p, float s, int m_rep) {
    const char folder [] = "sim2_ind/";
    char graphA [100];
    sprintf(graphA, "%s%d_A_%.5f_%.5f_ind.txt", folder, m_rep, p, s);
    char graphB [100];
    sprintf(graphB, "%s%d_B_%.5f_%.5f_ind.txt", folder, m_rep, p, s);
    
    generate_graph(n, p, graphA);
    generate_graph(n, p, graphB);

}

void sim2(int n, float p, float s, int K, int m, int iterations) {

    // double r = (double) factorial(tree_len+1) / pow(tree_len+1, tree_len+1);
    // int t = floor(1/ pow(r,2));

    for(int m_rep = 1; m_rep < m+1; ++m_rep) {
      generate_corr_graphs(n, p, s, m_rep);
      generate_ind_graphs(n, p, s, m_rep);

    }


    for(int k = 7; k < 10; ++k) {
      cout << "\n" << k-1;
      cout << "\ncorr";
      cout << "\n[";
      for (int m_rep = 1; m_rep < m + 1; ++m_rep) {
        const char folderCorr [] = "sim2_corr/";
        char graphA [100];
        sprintf(graphA, "%s%d_A_%.5f_%.5f_corr.txt", folderCorr, m_rep, p, s);
        char graphB [100];
        sprintf(graphB, "%s%d_B_%.5f_%.5f_corr.txt", folderCorr, m_rep, p, s);

        run_compare_graphs(graphA, graphB, k, false, false, iterations, false, true, false, true, p);

        cout << ", ";
        cout.flush();

      }
      cout<<'\b';
      cout<<'\b';
      cout<<"]\n";

      cout << "\nind";
      cout << "\n[";
      for (int m_rep = 1; m_rep < m + 1; ++m_rep) {
          const char folderInd [] = "sim2_ind/";
          char graphA [100];
          sprintf(graphA, "%s%d_A_%.5f_%.5f_ind.txt", folderInd, m_rep, p, s);
          char graphB [100];
          sprintf(graphB, "%s%d_B_%.5f_%.5f_ind.txt", folderInd, m_rep, p, s);

          run_compare_graphs(graphA, graphB, k, false, false, iterations, false, true, false, true, p);

          cout << ", ";
          cout.flush();
      }
      cout<<'\b';
      cout<<'\b';
      cout<<"]\n";
      cout.flush();
    }



}

int main(int argc, char** argv)
{
  // remove buffer so all outputs show up before crash
  setbuf(stdout, NULL);


  char* graph_fileA = NULL;
  char* graph_fileB = NULL;
  char* template_file = NULL;
  char* batch_file = NULL;
  int iterations = 1;
  bool do_outerloop = false;
  bool calculate_automorphism = true;
  bool labeled = false;
  bool do_gdd = false;
  bool do_vert = false;
  bool verbose = false;
  bool compare_graphs = false;
  bool sim_1 = false;
  bool sim_2 = false;
  int motif = 0;
  int n = 0;
  float p = 0.0;
  float s = 0.0;
  int m = 0;

  char c;
  while ((c = getopt (argc, argv, "g:f:t:b:m:n:p:s:i:k:uwqacdvrohl")) != -1)
  {
    switch (c)
    {
      case 'h':
        print_info(argv[0]);
        break;
      case 'l':
        labeled = true;
        break;
      case 'g':
        graph_fileA = strdup(optarg);
        break;
      case 'f':
        graph_fileB = strdup(optarg);
        break;
      case 't':
        template_file = strdup(optarg);
        break;
      case 'b':
        batch_file = strdup(optarg);
        break;
      case 'm':
        m = atoi(optarg);
        break;
      case 'n':
        n = atoi(optarg);
        break;
      case 'p':
        p = atof(optarg);
        break;
      case 's':
        s = atof(optarg);
        break;
      case 'i':
        iterations = atoi(optarg);
        break;
      case 'k':
        motif = atoi(optarg);
        break;
      case 'u':
        sim_1 = true;
        break;
      case 'w':
        sim_2 = true;
        break;
      case 'q':
        compare_graphs = true;
        break;
      case 'a':
        calculate_automorphism = false; 
        break;
      case 'c':
        do_vert = true;
        break;
      case 'd':
        do_gdd = true;
        break;
      case 'o':
        do_outerloop = true;
        break;
      case 'v':
        verbose = true;
        break;
      case 'r':
        timing = true;
        break;
      case '?':
        if (optopt == 'g' || optopt == 't' || optopt == 'b' || optopt == 'i' || optopt == 'm')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n",
      optopt);
        print_info(argv[0]);
      default:
        abort();
    }
  } 
  if(!sim_1 & !sim_2)
  {
    if(argc < 3)
    {
        printf("%d",sim_1);
        print_info_short(argv[0]);
    }

    if (motif && (motif < 3 || motif > 10))
    {
      printf("\nMotif option must be between [3,10]\n");    
      print_info(argv[0]);
    }
    else if (graph_fileA == NULL)
    { 
      printf("\nMust supply graph file\n");    
      print_info(argv[0]);
    }
    else if (template_file == NULL && batch_file == NULL && !motif)
    {
      printf("\nMust supply template XOR batchfile or -m option\n");
      print_info(argv[0]);
    }  
    else if (template_file != NULL && batch_file != NULL)
    {
      printf("\nMust only supply template file XOR batch file\n");
      print_info(argv[0]);
    }
    else if (iterations < 1)
    {
      printf("\nNumber of iterations must be positive\n");    
      print_info(argv[0]);
    }
  }

  if(sim_1) {
    sim1();
  }
  else if(sim_2) {
    if(n && p && s && m && iterations) {
        sim2(n, p, s, motif, m, iterations);
    }
    else{
      printf("\nMissing Arguments\n");
      printf("%d %f %f %d %d %d", n, p, s, motif, m, iterations);
    }

  }
  else if(compare_graphs && motif) {
    run_compare_graphs(graph_fileA, graph_fileB, motif,
              do_vert, do_gdd, 
              iterations, do_outerloop, calculate_automorphism, 
              verbose, false, 0);
  }
  else if (motif)
  {
    run_motif(graph_fileA, motif, 
              do_vert, do_gdd, 
              iterations, do_outerloop, calculate_automorphism, 
              verbose, false, 0);
  }
  else if (template_file != NULL)
  {
    run_single(graph_fileA, template_file, labeled,                
                do_vert, do_gdd,
                iterations, do_outerloop, calculate_automorphism,
                verbose);
  }
  else if (batch_file != NULL)
  {
    run_batch(graph_fileA, batch_file, labeled,
                do_vert, do_gdd,
                iterations, do_outerloop, calculate_automorphism,
                verbose, false, 0);
  }

  return 0;
}
