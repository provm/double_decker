set term postscript eps enhanced monochrome 20 dashed dashlength 1 lw 2
set output "sdc_dd.eps"

set style data histogram
set style histogram cluster gap 1

set style fill solid border rgb "black"
set auto x
#set yrange [1:700]
set datafile separator ","

set logscale y 2

set key font ",12"
set xtics font ",20"
set ytics font ",20"
set ylabel font ",20"
set xlabel font ",20"
set xlabel "Workload"
set ylabel "Application throughput (ops)"
#set yrange[0:]
set ytic auto
set boxwidth 1 relative

plot 'data.out' using 2:xtic(1) title col(2) fillstyle pattern 12,\
        '' using 3 title col(5) fillstyle pattern 3, \
        '' using 4 title col(8) fillstyle pattern 7

