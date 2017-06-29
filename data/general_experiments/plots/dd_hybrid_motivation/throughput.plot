set terminal png size 1000,800 enhanced font "monochrome 26"
set output "throughput.png"

set style data histogram
set style histogram cluster gap 1 errorbars

set style fill solid border rgb "black"

set xrange [-0.5:2.5]
set yrange [0:400]
set datafile separator ","

set key font ",16"
set xtics font ",16"
set ytics font ",16"
set ylabel font ",18"
set xlabel font ",18"
set xlabel "Cache allotment"
set ylabel "Application throughput (ops)"
set ytic auto
set boxwidth 1 relative

plot 'data.out' using 2:3:xtic(1) title col(2) fillstyle pattern 12, \
		'' using 4:5 title col(5) fillstyle pattern 1
