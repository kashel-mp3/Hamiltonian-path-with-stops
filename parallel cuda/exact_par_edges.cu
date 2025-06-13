#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <climits>
#include <chrono>
#include <iomanip>
#include <cuda_runtime.h>
#include "utils.h"
#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define CUDA_CHECK(err)                                                    \
  do                                                                       \
  {                                                                        \
    cudaError_t err_ = (err);                                              \
    if (err_ != cudaSuccess)                                               \
    {                                                                      \
      std::cerr << "CUDA error in " << __FILE__ << " at line " << __LINE__ \
                << ": " << cudaGetErrorString(err_) << std::endl;          \
      exit(EXIT_FAILURE);                                                  \
    }                                                                      \
  } while (0)

const int MAX_N_IN_KERNEL = 32;

struct DFS_Task
{
  int start_node;
  int second_node;
};

struct DFS_State
{
  int current_node_val;
  int path_pos;
  int cur_l;
  int max_l;
  int child_idx_to_try;
};

__global__ void solve_kernel_decoupled_work(
    const int *d_graph,
    const bool *d_stop_vertices_check,
    int n,
    const DFS_Task *d_tasks,
    int num_tasks,
    int *d_min_max_l_shared_for_pruning,
    int *d_all_tasks_min_max_l,
    int *d_all_tasks_paths)
{
  int task_idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (task_idx >= num_tasks)
  {
    return;
  }

  DFS_Task my_task = d_tasks[task_idx];
  int start_node_idx = my_task.start_node;
  int second_node_val = my_task.second_node;

  int current_path[MAX_N_IN_KERNEL];
  bool visited[MAX_N_IN_KERNEL];
  DFS_State dfs_stack[MAX_N_IN_KERNEL];
  int stack_top = -1;

  int thread_local_min_max_l_val = INT_MAX;
  int thread_local_best_path_arr[MAX_N_IN_KERNEL];

  for (int i = 0; i < n; ++i)
  {
    visited[i] = false;
    thread_local_best_path_arr[i] = -1;
  }

  current_path[0] = start_node_idx;
  current_path[1] = second_node_val;
  visited[start_node_idx] = true;
  visited[second_node_val] = true;

  int initial_edge_weight = d_graph[start_node_idx * n + second_node_val];
  int initial_cur_l = initial_edge_weight;
  int initial_max_l = 0;

  if (d_stop_vertices_check[second_node_val])
  {
    initial_max_l = initial_cur_l;
    initial_cur_l = 0;
  }

  dfs_stack[++stack_top] = {second_node_val, 2, initial_cur_l, initial_max_l, 0};

  int global_best_for_pruning_snapshot;

  while (stack_top != -1)
  {
    DFS_State u_state = dfs_stack[stack_top];
    global_best_for_pruning_snapshot = *d_min_max_l_shared_for_pruning;

    int effective_max_l = max(u_state.max_l, u_state.cur_l);
    int current_pruning_threshold = min(thread_local_min_max_l_val, global_best_for_pruning_snapshot);

    if (effective_max_l >= current_pruning_threshold && u_state.path_pos < n)
    {
      stack_top--;
      visited[u_state.current_node_val] = false;
      continue;
    }

    if (u_state.path_pos == n)
    {
      if (u_state.max_l < thread_local_min_max_l_val)
      {
        thread_local_min_max_l_val = u_state.max_l;
        for (int k = 0; k < n; ++k)
        {
          thread_local_best_path_arr[k] = current_path[k];
        }
        atomicMin(d_min_max_l_shared_for_pruning, thread_local_min_max_l_val);
      }
      stack_top--;
      visited[u_state.current_node_val] = false;
      continue;
    }

    bool found_child_to_explore = false;
    for (int v_node_candidate_idx = u_state.child_idx_to_try; v_node_candidate_idx < n; ++v_node_candidate_idx)
    {
      if (d_graph[u_state.current_node_val * n + v_node_candidate_idx] > 0 && !visited[v_node_candidate_idx])
      {
        dfs_stack[stack_top].child_idx_to_try = v_node_candidate_idx + 1;

        current_path[u_state.path_pos] = v_node_candidate_idx;
        visited[v_node_candidate_idx] = true;

        DFS_State v_state;
        v_state.current_node_val = v_node_candidate_idx;
        v_state.path_pos = u_state.path_pos + 1;
        v_state.child_idx_to_try = 0;

        int edge_weight = d_graph[u_state.current_node_val * n + v_node_candidate_idx];
        int accumulated_segment_len_at_v = u_state.cur_l + edge_weight;
        v_state.max_l = u_state.max_l;

        bool v_is_designated_stop = d_stop_vertices_check[v_node_candidate_idx];
        bool v_completes_hamiltonian_path = (v_state.path_pos == n);

        if (v_is_designated_stop || v_completes_hamiltonian_path)
        {
          v_state.max_l = max(v_state.max_l, accumulated_segment_len_at_v);
          v_state.cur_l = 0;
        }
        else
        {
          v_state.cur_l = accumulated_segment_len_at_v;
        }

        current_pruning_threshold = min(thread_local_min_max_l_val, global_best_for_pruning_snapshot);
        int v_effective_max_l = max(v_state.max_l, v_state.cur_l);

        if (v_effective_max_l >= current_pruning_threshold && !v_completes_hamiltonian_path)
        {
          visited[v_node_candidate_idx] = false;
          continue;
        }

        dfs_stack[++stack_top] = v_state;
        found_child_to_explore = true;
        break;
      }
    }

    if (!found_child_to_explore)
    {
      stack_top--;
      visited[u_state.current_node_val] = false;
    }
  }

  d_all_tasks_min_max_l[task_idx] = thread_local_min_max_l_val;
  if (thread_local_min_max_l_val != INT_MAX)
  {
    for (int k = 0; k < n; ++k)
    {
      d_all_tasks_paths[task_idx * n + k] = thread_local_best_path_arr[k];
    }
  }
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <path_to_json_data_file> [num_cuda_threads_per_block (optional, default 1024)]" << std::endl;
    return 1;
  }
  std::string test_data_path = argv[1];
  int threads_per_block = 1024;
  if (argc > 2)
  {
    try
    {
      threads_per_block = std::stoi(argv[2]);
      if (threads_per_block <= 0 || threads_per_block > 1024 || (threads_per_block & (threads_per_block - 1)) != 0)
      {
        std::cerr << "Threads per block must be > 0, <= 1024, and a power of 2. Using default 1024." << std::endl;
        threads_per_block = 1024;
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Invalid argument for threads per block. Using default 1024. Error: " << e.what() << std::endl;
      threads_per_block = 1024;
    }
  }

  Utils utils;
  int n, s_num_designated_stops_in_list;
  int **h_graph_2d = nullptr;
  int *h_designated_stop_vertices_indices = nullptr;

  utils.read_data_from_json_to_arrays(test_data_path, n, s_num_designated_stops_in_list,
                                      h_graph_2d, h_designated_stop_vertices_indices);

  if (n == 0 || h_graph_2d == nullptr)
  {
    std::cout << "-1 (empty or invalid graph)" << std::endl;
    if (h_graph_2d)
      utils.release_allocated_memory(n, h_graph_2d, h_designated_stop_vertices_indices);
    return 0;
  }
  if (n > MAX_N_IN_KERNEL)
  {
    std::cerr << "Error: Number of vertices n=" << n
              << " exceeds MAX_N_IN_KERNEL=" << MAX_N_IN_KERNEL
              << ". Please recompile with a larger MAX_N_IN_KERNEL." << std::endl;
    utils.release_allocated_memory(n, h_graph_2d, h_designated_stop_vertices_indices);
    return 1;
  }

  std::vector<DFS_Task> h_tasks;
  for (int i = 0; i < n; ++i)
  {
    for (int j = 0; j < n; ++j)
    {
      if (i != j && h_graph_2d[i][j] > 0)
      {
        h_tasks.push_back({i, j});
      }
    }
  }

  if (h_tasks.empty())
  {
      std::cout << "-1 (no edges in graph, cannot form a path)" << std::endl;
      utils.release_allocated_memory(n, h_graph_2d, h_designated_stop_vertices_indices);
      return 0;
  }


  int *h_graph_flat = new int[n * n];
  for (int i = 0; i < n; ++i)
  {
    for (int j = 0; j < n; ++j)
    {
      h_graph_flat[i * n + j] = h_graph_2d[i][j];
    }
  }

  bool *h_stop_vertices_check = new bool[n]();
  for (int i = 0; i < s_num_designated_stops_in_list; ++i)
  {
    if (h_designated_stop_vertices_indices[i] >= 0 && h_designated_stop_vertices_indices[i] < n)
    {
      h_stop_vertices_check[h_designated_stop_vertices_indices[i]] = true;
    }
  }

  if (n > 0 && !utils.is_connected_arrays(n, h_graph_2d))
  {
    std::cout << "-2 (graph not connected)" << std::endl;
    delete[] h_graph_flat;
    delete[] h_stop_vertices_check;
    utils.release_allocated_memory(n, h_graph_2d, h_designated_stop_vertices_indices);
    return 0;
  }

  auto chrono_solve_start = std::chrono::high_resolution_clock::now();

  int *d_graph_flat;
  bool *d_stop_vertices_check_gpu;
  int *d_min_max_l_shared_for_pruning;
  DFS_Task *d_tasks;
  int *d_all_tasks_min_max_l;
  int *d_all_tasks_paths;

  int num_tasks = h_tasks.size();

  CUDA_CHECK(cudaMalloc((void **)&d_graph_flat, n * n * sizeof(int)));
  CUDA_CHECK(cudaMalloc((void **)&d_stop_vertices_check_gpu, n * sizeof(bool)));
  CUDA_CHECK(cudaMalloc((void **)&d_min_max_l_shared_for_pruning, sizeof(int)));
  CUDA_CHECK(cudaMalloc((void **)&d_tasks, num_tasks * sizeof(DFS_Task)));
  CUDA_CHECK(cudaMalloc((void **)&d_all_tasks_min_max_l, num_tasks * sizeof(int)));
  CUDA_CHECK(cudaMalloc((void **)&d_all_tasks_paths, num_tasks * n * sizeof(int)));

  CUDA_CHECK(cudaMemcpy(d_graph_flat, h_graph_flat, n * n * sizeof(int), cudaMemcpyHostToDevice));
  CUDA_CHECK(cudaMemcpy(d_stop_vertices_check_gpu, h_stop_vertices_check, n * sizeof(bool), cudaMemcpyHostToDevice));
  CUDA_CHECK(cudaMemcpy(d_tasks, h_tasks.data(), num_tasks * sizeof(DFS_Task), cudaMemcpyHostToDevice));

  int h_initial_global_pruning_val = INT_MAX;
  CUDA_CHECK(cudaMemcpy(d_min_max_l_shared_for_pruning, &h_initial_global_pruning_val, sizeof(int), cudaMemcpyHostToDevice));

  std::vector<int> h_initial_min_max_l_for_tasks(num_tasks, INT_MAX);
  CUDA_CHECK(cudaMemcpy(d_all_tasks_min_max_l, h_initial_min_max_l_for_tasks.data(), num_tasks * sizeof(int), cudaMemcpyHostToDevice));

  int num_blocks = (num_tasks + threads_per_block - 1) / threads_per_block;

  std::cout << n << ' ';

  cudaEvent_t kernel_start_event, kernel_stop_event;
  CUDA_CHECK(cudaEventCreate(&kernel_start_event));
  CUDA_CHECK(cudaEventCreate(&kernel_stop_event));
  CUDA_CHECK(cudaEventRecord(kernel_start_event));

  solve_kernel_decoupled_work<<<num_blocks, threads_per_block>>>(
      d_graph_flat, d_stop_vertices_check_gpu, n,
      d_tasks, num_tasks,
      d_min_max_l_shared_for_pruning,
      d_all_tasks_min_max_l, d_all_tasks_paths);

  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaDeviceSynchronize());

  CUDA_CHECK(cudaEventRecord(kernel_stop_event));
  CUDA_CHECK(cudaEventSynchronize(kernel_stop_event));
  float kernel_milliseconds = 0;
  CUDA_CHECK(cudaEventElapsedTime(&kernel_milliseconds, kernel_start_event, kernel_stop_event));

  std::vector<int> h_all_tasks_min_max_l(num_tasks);
  std::vector<int> h_all_tasks_paths(num_tasks * n);
  CUDA_CHECK(cudaMemcpy(h_all_tasks_min_max_l.data(), d_all_tasks_min_max_l, num_tasks * sizeof(int), cudaMemcpyDeviceToHost));
  CUDA_CHECK(cudaMemcpy(h_all_tasks_paths.data(), d_all_tasks_paths, num_tasks * n * sizeof(int), cudaMemcpyDeviceToHost));

  int global_best_min_max_l = INT_MAX;
  int best_task_idx = -1;

  for (int i = 0; i < num_tasks; ++i)
  {
    if (h_all_tasks_min_max_l[i] < global_best_min_max_l)
    {
      global_best_min_max_l = h_all_tasks_min_max_l[i];
      best_task_idx = i;
    }
  }
  
  auto chrono_solve_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> host_solve_duration_ms = chrono_solve_end - chrono_solve_start;

  if (best_task_idx == -1)
  {
    std::cout << "-1" << std::endl;
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      std::cout << h_all_tasks_paths[best_task_idx * n + i] << (i == n - 1 ? "" : " ");
    }
    std::cout << " " << global_best_min_max_l;
    std::cout << " " << std::fixed << std::setprecision(3) << kernel_milliseconds;
    std::cout << " " << std::fixed << std::setprecision(3) << host_solve_duration_ms.count() << std::endl;
  }

  delete[] h_graph_flat;
  delete[] h_stop_vertices_check;
  utils.release_allocated_memory(n, h_graph_2d, h_designated_stop_vertices_indices);

  CUDA_CHECK(cudaFree(d_graph_flat));
  CUDA_CHECK(cudaFree(d_stop_vertices_check_gpu));
  CUDA_CHECK(cudaFree(d_min_max_l_shared_for_pruning));
  CUDA_CHECK(cudaFree(d_tasks));
  CUDA_CHECK(cudaFree(d_all_tasks_min_max_l));
  CUDA_CHECK(cudaFree(d_all_tasks_paths));
  CUDA_CHECK(cudaEventDestroy(kernel_start_event));
  CUDA_CHECK(cudaEventDestroy(kernel_stop_event));

  return 0;
}