set term postscript eps enhanced monochrome 20 dashed dashlength 1 lw 2
set output "throughput.eps"

set style data histogram
set style histogram cluster gap 1 errorbars

set style fill solid border rgb "black"
set auto x
set xrange [0.5:8.5]
set yrange [0:1200]
set key font ",18"
set xtics font ",20"
set ytics font ",20"
set ylabel font ",20"
set xlabel font ",20"
set ylabel "Application throughput (ops/s)"
set xlabel "Memory allocation ratios"  
set ytic auto
set boxwidth 1 relative
set xtics ("8:0" 1, "7:1" 2, "6:2" 3, "5:3" 4, "4:4" 5, "3:5" 6, "2:6" 7, "1:7" 8)
set key on box at 8.2,800

#set label "Redis 2.00/0.00" at 1.1,1320 font ",10"
#set label "Actual value (10406) = plot-value x 8" at 1.1,1280 font ",10"

plot  	 'webserver.dat' using 1:2 with lines title 'Webserver' linetype 22 lw 2, "" using 1:2:3 with errorbars notitle lt 1,\
 	'redis.dat' using 1:2 with lines title 'Redis' linetype 1, "" using 1:2:3 with errorbars notitle lt 1,\
	'mysql.dat' using 1:2 with lines title 'MySQL' linetype 3 lw 2, "" using 1:2:3 with errorbars notitle lt 1,\
	'mongo.dat' using 1:2 with lines title 'MongoDB' linetype 14 lw 2, "" using 1:2:3 with errorbars notitle lt 1

