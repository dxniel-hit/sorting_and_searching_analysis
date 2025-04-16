import matplotlib.pyplot as plt
import csv
import sys

def generate_chart(csv_file):
    data = []

    # Leer el archivo CSV manualmente
    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            if len(row) >= 3:  # Asegurarse que hay al menos 3 columnas
                algorithm, size, time = row[0], row[1], float(row[2])
                data.append({
                    'label': f"{algorithm} ({size})",
                    'time': time
                })

    # Ordenar por tiempo descendente
    data.sort(key=lambda x: x['time'], reverse=True)

    # Extraer etiquetas y tiempos
    labels = [item['label'] for item in data]
    times = [item['time'] for item in data]

    # Crear gráfico
    plt.figure(figsize=(12, 8))
    bars = plt.bar(labels, times)

    # Añadir valores encima de las barras
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                 f'{height:.4f}',
                 ha='center', va='bottom')

    plt.title('Tiempos de Ejecución de Algoritmos de Ordenamiento')
    plt.ylabel('Tiempo (segundos)')
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()

    # Guardar imagen
    plt.savefig('sorting_results.png')
    print("Gráfico generado como 'sorting_results.png'")

    # Mostrar imagen (opcional)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        generate_chart(sys.argv[1])
    else:
        print("Uso: python script.py archivo_resultados.csv")