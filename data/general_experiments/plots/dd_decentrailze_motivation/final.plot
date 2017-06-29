set terminal png size 800,500 enhanced font "monochrome 26"
set output "throughput.png"

set style data histogram
set style histogram cluster gap 1 errorbars

set style fill solid border rgb "black"
set auto x
set xrange [0.5:8.5]
set yrange [0:1100]
set key font ",18"
set xtics font ",16"
set ytics font ",16"
set ylabel font ",16"
set xlabel font ",16"
set ylabel "Application throughput (ops/s)"
set xlabel "Memory allocation ratios (In-memory:Cache)"  
set ytic auto
set boxwidth 1 relative
set xtics ("8:0" 1, "7:1" 2, "6:2" 3, "5:3" 4, "4:4" 5, "3:5" 6, "2:6" 7, "1:7" 8)
set key on at 8.2,800

plot 'webserver.dat' using 1:2 with lines title 'Webserver' linetype 1 lw 2 lc rgb "blue", "" using 1:2:3 with errorbars notitle lt 1 lc rgb "blue",\
 	'redis.dat' using 1:2 with lines title 'Redis' linetype 1 lw 2 lc rgb "red", "" using 1:2:3 with errorbars notitle lt 1 lc rgb "red",\
	'mysql.dat' using 1:2 with lines title 'MySQL' linetype 1 lw 2 lc rgb "orange", "" using 1:2:3 with errorbars notitle lt 1 lc rgb "orange",\
	'mongo.dat' using 1:2 with lines title 'MongoDB' linetype 1 lw 2 lc rgb "dark-green", "" using 1:2:3 with errorbars notitle lt 1 lc rgb "dark-green"

