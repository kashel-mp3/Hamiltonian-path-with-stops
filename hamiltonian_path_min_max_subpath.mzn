include "alldifferent.mzn";
int: n;
array[1..n] of bool: stops;
array[1..n, 1..n] of int: graph;

n = 5;
stops = [true, false, true, false, true];
graph = [| 0, 1, 0, 0, 0 
         | 2, 0, 1, 0, 0 
         | 0, 4, 0, 7, 0 
         | 0, 0, 3, 0, 1 
         | 0, 0, 0, 3, 0 |];

array[1..n] of var 1..n: path;

predicate is_stop(var int: index, array[int] of var bool: arr) =
    arr[index] = true;
    
function var bool: is_feasible_path(array[int] of var int: path) =
    forall(i in 1..n-1) (
        graph[path[i], path[i+1]] > 0
    );

function var int: compute_path_value(array[int] of var int: path, var int: i, var int: j) =
    let {
        var int: total_distance = 0;
    } in
    if is_stop(path[i], stops) /\ is_stop(path[j], stops) /\ i < j /\
        forall(l in i+1..j-1) (
            not is_stop(path[l], stops)
        ) then
        sum(k in i..(j-1)) (
            graph[path[k], path[k+1]]
        )
        + graph[path[j], path[j-1]]
    else
        0
    endif;
    
constraint alldifferent(path);
constraint is_stop(path[1], stops);
constraint is_stop(path[n], stops);
constraint is_feasible_path(path);

var int: max_distance = max([compute_path_value(path, i, j) | i, j in 1..n]);

solve minimize max_distance;

















