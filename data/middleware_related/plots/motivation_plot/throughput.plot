set term postscript eps enhanced monochrome 20 dashed dashlength 1 lw 2
set output "throughput.eps"

set style data histogram
set style histogram cluster gap 1 errorbars

set style fill solid border rgb "black"
set auto x
set yrange [0:1200]
set datafile separator ","

set key font ",12"
set xtics font ",28"
set ytics font ",28"
set ylabel font ",28"
set xlabel font ",28"
set xlabel "Workload"
set ylabel "Application throughput (ops)"
set yrange[0:]
set ytic auto
set boxwidth 1 relative

set label font ",10" "x10 = 10406 ops" at 0.4,1100

plot 'data.out' using 2:3:xtic(1) title col(2) fillstyle pattern 12,\
        '' using 4:5 title col(5) fillstyle pattern 14, \
        '' using 6:7 title col(8) fillstyle pattern 5, \
        '' using 8:9 title col(11) fillstyle pattern 7, \
        '' using 10:11 title col(14) fillstyle pattern 10, \
        '' using 12:13 title col(17) fillstyle pattern 8, \
        '' using 14:15 title col(20) fillstyle pattern 2, \
        '' using 16:17 title col(23) fillstyle pattern 3

