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
#include <atomic> // Można rozważyć dla min_max_subpath, ale critical jest prostsze

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
// Zmodyfikowana funkcja check_all_possible_paths
// Przyjmuje referencję do globalnego min_max_l i wskaźnik do globalnego opt_path
// Wprowadzono sekcję krytyczną do aktualizacji globalnego optimum
void check_all_possible_paths(int pos, int cur_l, int max_l, int used_s, int *path, bool *visited,
                              int &min_max_l, int *opt_path, int n, int s, bool *stops, int **graph,
                              omp_lock_t *lock) // Dodajemy blokadę jako argument
{
  // Pruning na podstawie bieżącej ścieżki (max_l) i globalnego minimum (min_max_l)
  // Odczyt min_max_l nie musi być chroniony, ale może być lekko nieaktualny.
  // Jeśli chcemy najświeższą wartość, można użyć #pragma omp atomic read
  // ale często nie jest to konieczne.
  if (max_l >= min_max_l)
  {
    return; // Przycinanie na podstawie już osiągniętego max_l
  }

  if (pos == n)
  {
    // Znaleziono kompletną ścieżkę, sprawdzamy czy jest lepsza
    omp_set_lock(lock); // Zablokuj przed dostępem do współdzielonych zmiennych
    if (max_l < min_max_l)
    {
      min_max_l = max_l;
      std::copy(path, path + n, opt_path);
    }
    omp_unset_lock(lock); // Odblokuj
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
        // Sprawdzenie warunku liczby przystanków (jeśli jest wymagane 'dokładnie s')
        if (new_used_s > s || (new_used_s == s && pos != n - 1))
        {
          visited[v] = false;
          continue; // Ścieżka nie spełnia warunków przystanków
        }

        if (new_max_l < new_cur_l)
        {
          // Sprawdzenie przycinania *przed* rekurencyjnym wywołaniem
          // Porównujemy potencjalny nowy max_l (czyli new_cur_l) z globalnym min_max_l
          if (new_cur_l >= min_max_l) // Odczyt min_max_l
          {
            visited[v] = false;
            continue; // Ta gałąź nie może dać lepszego wyniku
          }
          new_max_l = new_cur_l; // Aktualizuj max_l dla tej ścieżki
        }
        new_cur_l = 0; // Resetuj licznik długości podścieżki
      }
      // Kontynuuj rekurencję z zaktualizowanymi wartościami
      // Przekazujemy te same globalne zmienne min_max_l i opt_path oraz blokadę
      check_all_possible_paths(pos + 1, new_cur_l, new_max_l, new_used_s, path, visited, min_max_l, opt_path, n, s, stops, graph, lock);
      visited[v] = false; // Backtracking
    }
  }
}

// Zmodyfikowana funkcja solve
int *solve(int n, int s, int **graph, bool *stop_vertices_check, int n_threads = 8)
{
  int *opt_path = new int[n];
  // Inicjalizacja globalnego minimum - można użyć std::atomic<int> jeśli chcemy unikać blokad przy odczycie
  int min_max_subpath = INT_MAX;

  omp_set_num_threads(n_threads);

  // Inicjalizacja blokady OpenMP
  omp_lock_t writelock;
  omp_init_lock(&writelock);

// Usunięto 'shared(opt_path, min_max_subpath)' - dostęp będzie zarządzany przez blokadę
// Usunięto 'private(chunk_size)' - nie jest używane w pętli for
#pragma omp parallel shared(graph, stop_vertices_check, n, s, min_max_subpath, opt_path, writelock)
  {
    // Każdy wątek ma swoje lokalne kopie do eksploracji
    int *path = new int[n];
    bool *visited = new bool[n];

// Pętla for bez lokalnych zmiennych min/opt - operuje na globalnych
// Usunięto 'private(path, visited, local_opt_path, local_min_max)' - path i visited są lokalne wątku, reszta globalna
// schedule(dynamic) może być dobrym wyborem przy nierównomiernym czasie obliczeń dla różnych 'i'
#pragma omp for schedule(dynamic)
    for (int i = 0; i < n; ++i)
    {
      std::fill(visited, visited + n, false);
      visited[i] = true;
      path[0] = i;

      // Wywołujemy rekurencję, przekazując GLOBALNE min_max_subpath, opt_path i blokadę
      check_all_possible_paths(1, 0, 0, (stop_vertices_check[i] ? 1 : 0), path, visited,
                               min_max_subpath, opt_path,
                               n, s, stop_vertices_check, graph, &writelock);
      // Nie potrzebujemy już lokalnego agregowania wyników ani sekcji krytycznej tutaj
      // Aktualizacje globalnego optimum dzieją się wewnątrz check_all_possible_paths
    }
    // Sprzątanie pamięci lokalnej wątku
    delete[] path;
    delete[] visited;
  } // Koniec regionu równoległego

  // Zniszczenie blokady
  omp_destroy_lock(&writelock);

  // Sprawdzenie, czy znaleziono jakiekolwiek rozwiązanie
  if (min_max_subpath == INT_MAX)
  {
    delete[] opt_path;
    return nullptr;
  }
  return opt_path;
}

// Funkcja main pozostaje bez zmian, poza ewentualnym uwzględnieniem zmian w solve (np. brak chunk_size)
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
  // Pobranie liczby wątków z argumentu lub ustawienie domyślnej
  int num_threads = 8; // Domyślnie
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

  // Wywołanie solve z podaną liczbą wątków
  int *solution = solve(n, s, graph, stop_vertices_check, num_threads);

  if (solution)
  {
    // Obliczanie max_subpath dla znalezionego rozwiązania
    int final_max_subpath = max_subpath(solution, n, stop_vertices_check, graph);

    for (int i = 0; i < n; ++i)
    {
      // Poprawka: Wypisywanie ścieżki - oryginalny kod miał błąd logiczny w warunku break
      std::cout << solution[i] << " ";
    }
    // Wypisanie obliczonej maksymalnej długości podścieżki
    std::cout << final_max_subpath << '\n';
    delete[] solution;
  }
  else
  {
    std::cout << "-1\n"; // Zgodnie z wymaganiami wielu zadań tego typu
  }

  // Sprzątanie pamięci
  delete[] stop_vertices;
  for (int i = 0; i < n; ++i)
  {
    delete[] graph[i];
  }
  delete[] graph;
  delete[] stop_vertices_check;
  return 0;
}