#!/bin/bash -x

for i in {0..7};
do
   cpufreq-set -c $i -g performance
done
