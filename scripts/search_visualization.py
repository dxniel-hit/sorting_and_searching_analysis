import matplotlib.pyplot as plt
import csv
import sys

def generate_search_chart(csv_file, output_file='images/search_results.png'):
    # Organizar datos por algoritmo
    algorithms = {}

    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            if len(row) >= 3:  # Asegurar que hay al menos 3 columnas
                algorithm, size, time = row[0], int(row[1]), float(row[2])
                if algorithm not in algorithms:
                    algorithms[algorithm] = {'sizes': [], 'times': []}
                algorithms[algorithm]['sizes'].append(size)
                algorithms[algorithm]['times'].append(time)

    # Crear gráfico con subplots para mejor visualización
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 12))

    # Gráfico lineal con escala logarítmica
    for algorithm in algorithms:
        sizes = algorithms[algorithm]['sizes']
        times = algorithms[algorithm]['times']
        ax1.loglog(sizes, times, marker='o', label=algorithm, linewidth=2, markersize=8)

    ax1.set_title('Comparación de Tiempos de Ejecución de Algoritmos de Búsqueda (Escala Logarítmica)')
    ax1.set_xlabel('Tamaño del Dataset (elementos)')
    ax1.set_ylabel('Tiempo (segundos)')
    ax1.grid(True, which="both", ls="--")
    ax1.legend()

    # Gráfico de barras agrupadas para mejor comparación por tamaño
    bar_width = 0.15
    sizes = sorted(list({size for algo in algorithms.values() for size in algo['sizes']}))
    x = range(len(sizes))

    # Crear una lista de colores consistente para ambos gráficos
    colors = plt.cm.tab10.colors  # Usar la misma paleta de colores que matplotlib usa por defecto

    for i, algorithm in enumerate(algorithms):
        times = [
            algorithms[algorithm]['times'][algorithms[algorithm]['sizes'].index(size)]
            if size in algorithms[algorithm]['sizes'] else 0
            for size in sizes
        ]
        # Usar el mismo color que en el primer gráfico para consistencia
        color = colors[i % len(colors)]
        ax2.bar([pos + i*bar_width for pos in x], times, width=bar_width,
                label=algorithm, color=color)

    ax2.set_title('Comparación por Tamaño de Dataset')
    ax2.set_xlabel('Tamaño del Dataset')
    ax2.set_ylabel('Tiempo (segundos)')
    ax2.set_xticks([pos + bar_width*(len(algorithms)/2 - 0.5) for pos in x])
    ax2.set_xticklabels(sizes)
    ax2.set_yscale('log')
    ax2.grid(True, which="both", axis='y', ls="--")
    ax2.legend()

    plt.tight_layout()

    # Guardar imagen
    plt.savefig(output_file)
    print(f"Gráfico generado como '{output_file}'")

    # Mostrar imagen (opcional)
    # plt.show()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        generate_search_chart(sys.argv[1])
    else:
        print("Uso: python search_visualization.py archivo_resultados.csv")