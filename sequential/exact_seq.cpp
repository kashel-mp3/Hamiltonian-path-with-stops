#include <nlohmann/json.hpp>
#include "../utils.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <climits>
#include <filesystem>

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
  return max_subpath;
}

void check_all_possible_paths(int pos, int cur_l, int max_l, int used_s, int *path, bool *visited,
                              int &min_max_l, int *opt_path, int n, int s, bool *stops, int **graph)
{
  if (pos == n)
  {
    if (max_l < min_max_l)
    {
      min_max_l = max_l;
      std::copy(path, path + n, opt_path);
    }
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
        if (new_used_s == s && pos != n - 1)
        {
          visited[v] = false;
          continue;
        }
        if (new_max_l < new_cur_l)
        {
          if (new_cur_l >= min_max_l)
          {
            visited[v] = false;
            continue;
          }
          new_max_l = new_cur_l;
        }
        new_cur_l = 0;
      }
      check_all_possible_paths(pos + 1, new_cur_l, new_max_l, new_used_s, path, visited, min_max_l, opt_path, n, s, stops, graph);
      visited[v] = false;
    }
  }
}

int *solve(int n, int s, int **graph, bool *stop_vertices_check)
{
  int *opt_path = new int[n];
  int min_max_subpath = INT_MAX;

  int *local_opt_path = new int[n];
  int local_min_max = INT_MAX;

  for (int i = 0; i < n; ++i)
  {
    int *path = new int[n];
    bool *visited = new bool[n]();
    visited[i] = true;
    path[0] = i;
    check_all_possible_paths(1, 0, 0, 1, path, visited, local_min_max, local_opt_path,
                             n, s, stop_vertices_check, graph);
    delete[] path;
    delete[] visited;

    if (local_min_max < min_max_subpath)
    {
      min_max_subpath = local_min_max;
      std::copy(local_opt_path, local_opt_path + n, opt_path);
    }
  }

  delete[] local_opt_path;

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
  int *solution = solve(n, s, graph, stop_vertices_check);
  if (solution)
  {
    for (int i = 0; i < n; ++i)
    {
      std::cout << solution[i] << ' ';
    }
    std::cout << max_subpath(solution, n, stop_vertices_check, graph) << '\n';
    delete[] solution;
  }
  else
  {
    std::cout << "No solution found\n";
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