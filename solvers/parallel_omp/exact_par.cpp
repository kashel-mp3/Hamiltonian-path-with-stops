#include <nlohmann/json.hpp>
#include "../utils.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <climits>
#include <filesystem>
#include <omp.h>
#include <atomic> 

using json = nlohmann::json;

int max_subpath(int *path, int path_size, bool *stop_vertices_check, int **graph)
{
  int max_subpath = 0;
  int cur_subpath = 0;
  if (path_size == 0)
  {
    return -1;
  }
  for (int i = 1; i < path_size; ++i)
  {
    cur_subpath += graph[path[i - 1]][path[i]];
    if (stop_vertices_check[path[i]])
    {
      if (max_subpath < cur_subpath)
      {
        max_subpath = cur_subpath;
      }
      cur_subpath = 0;
    }
  }
  if (max_subpath < cur_subpath) {
      max_subpath = cur_subpath;
  }
  return max_subpath;
}

void check_all_possible_paths(int pos, int cur_l, int max_l, int used_s, int *path, bool *visited,
                              int &min_max_l, int *opt_path, int n, int s, bool *stops, int **graph,
                              omp_lock_t *lock) 
{
  
  int current_min_max_top;
  #pragma omp atomic read
  current_min_max_top = min_max_l;
  if (max_l >= current_min_max_top)
  {
    return; 
  }

  if (pos == n)
  {
    
    omp_set_lock(lock); 
    if (max_l < min_max_l)
    {
      min_max_l = max_l;
      std::copy(path, path + n, opt_path);
    }
    omp_unset_lock(lock); 
    return;
  }

  for (int v = 0; v < n; v++)
  {
    if (graph[path[pos - 1]][v] && !visited[v])
    {
      path[pos] = v;
      visited[v] = true;
      int new_max_l = max_l;
      int new_cur_l = cur_l + graph[path[pos - 1]][v];
      int new_used_s = used_s;

      if (stops[v])
      {
        ++new_used_s;
        
        if (new_used_s > s || (new_used_s == s && pos != n - 1))
        {
          visited[v] = false;
          continue; 
        }

        if (new_max_l < new_cur_l)
        {
          
          int current_min_max_inner;
          #pragma omp atomic read
          current_min_max_inner = min_max_l;
          if (new_cur_l >= current_min_max_inner) 
          {
            visited[v] = false;
            continue; 
          }
          new_max_l = new_cur_l; 
        }
        new_cur_l = 0; 
      }
      
      check_all_possible_paths(pos + 1, new_cur_l, new_max_l, new_used_s, path, visited, min_max_l, opt_path, n, s, stops, graph, lock);
      visited[v] = false; 
    }
  }
}

int *solve(int n, int s, int **graph, bool *stop_vertices_check, int n_threads = 8)
{
  int *opt_path = new int[n];
  
  int min_max_subpath = INT_MAX;

  omp_set_num_threads(n_threads);

  omp_lock_t writelock;
  omp_init_lock(&writelock);

#pragma omp parallel shared(graph, stop_vertices_check, n, s, min_max_subpath, opt_path, writelock)
  {
    
    int *path = new int[n];
    bool *visited = new bool[n];

#pragma omp for schedule(dynamic)
    for (int i = 0; i < n; ++i)
    {
      if (!stop_vertices_check[i]) continue;
      std::fill(visited, visited + n, false);
      visited[i] = true;
      path[0] = i;

      check_all_possible_paths(1, 0, 0, (stop_vertices_check[i] ? 1 : 0), path, visited,
                               min_max_subpath, opt_path,
                               n, s, stop_vertices_check, graph, &writelock);
      
    }
    
    delete[] path;
    delete[] visited;
  } 

  omp_destroy_lock(&writelock);

  if (min_max_subpath == INT_MAX)
  {
    delete[] opt_path;
    return nullptr;
  }
  return opt_path;
}

int main(int argc, char **argv)
{
  Utils utils = Utils();
  if (argc < 2)
  {
    std::cerr << "path to data file not provided\n";
    return 1;
  }
  std::string test_data_path = argv[1];
  int n_threads = 8;
  if (argc > 2 && atoi(argv[2]) > 0 && atoi(argv[2]) <= 8)
    n_threads = atoi(argv[2]);
  int n, s;
  int *stop_vertices;
  int **graph;
  utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);
  bool *stop_vertices_check = new bool[n]();
  for (int i = 0; i < s; ++i)
  {
    stop_vertices_check[stop_vertices[i]] = true;
  }
  if (!utils.is_connected_arrays(n, graph))
  {
    std::cout << "-2 \n";
    delete[] stop_vertices;
    for (int i = 0; i < n; ++i)
    {
      delete[] graph[i];
    }
    delete[] graph;
    delete[] stop_vertices_check;
    return 0;
  }
  
  int num_threads = 8; 
  if (argc >= 3)
  {
    try
    {
      num_threads = std::stoi(argv[2]);
    }
    catch (const std::invalid_argument &ia)
    {
      std::cerr << "Invalid number of threads: " << argv[2] << std::endl;
      return 1;
    }
    catch (const std::out_of_range &oor)
    {
      std::cerr << "Number of threads out of range: " << argv[2] << std::endl;
      return 1;
    }
  }
  if (num_threads <= 0)
  {
    std::cerr << "Number of threads must be positive." << std::endl;
    return 1;
  }

  int *solution = solve(n, s, graph, stop_vertices_check, num_threads);

  if (solution)
  {
    
    int final_max_subpath = max_subpath(solution, n, stop_vertices_check, graph);

    for (int i = 0; i < n; ++i)
    {
      
      std::cout << solution[i] << " ";
    }
    
    std::cout << final_max_subpath << '\n';
    delete[] solution;
  }
  else
  {
    std::cout << "-1\n"; 
  }

  delete[] stop_vertices;
  for (int i = 0; i < n; ++i)
  {
    delete[] graph[i];
  }
  delete[] graph;
  delete[] stop_vertices_check;
  return 0;
}