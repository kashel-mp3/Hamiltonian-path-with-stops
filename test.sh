#!/bin/bash
n1=1
n2=11
step=1
echo "^ \$n\$ ^ \$T_s(n)\$ ^ \$T_r(n)\$ ^ \$T_s(n) \\over T_r(n)\$ ^"
while [ $n1 -le $n2 ]; do
 #echo n1=$n1
 #env OMP_NUM_THREADS=$n1 DRUK=Tak ./permutacje2 $n1
 #env OMP_NUM_THREADS=$n1 DRUK=Nie ./permutacje2 $n1 
 ./permutacje3 $n1
 let n1=n1+step
done