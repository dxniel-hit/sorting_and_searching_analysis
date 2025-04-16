set terminal pngcairo size 1024,768 enhanced font 'Verdana,10'
set output 'sorting_results.png'
set style data histograms
set style histogram rowstacked
set style fill solid border -1
set boxwidth 0.75
set title 'Tiempos de Ejecución de Algoritmos de Ordenamiento'
set ylabel 'Tiempo (segundos)'
set xtics rotate by -45
plot 'chart_data.dat' using 2:xtic(1) notitle
