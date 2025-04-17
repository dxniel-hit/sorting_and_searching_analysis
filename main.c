#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define RESULTS_FILE "sorting_result.csv"
#define SEARCH_RESULTS_FILE "searching_result.csv"
#define MAX_ALGORITHMS 10
#define MAX_NAME_LENGTH 50

// struct declarations
typedef struct {
    int *arr;
    int low;
    int cnt;
    int dir;
    int *progress;
    int total_elements;
} BitonicParams;

typedef struct {
    char algorithm[MAX_NAME_LENGTH];
    int size;
    double time;
} SortResult;

// Function declarations
void generateFileOfNumbers(const char *numbers, int n);
int *loadArrayFromFile(const char *filename, int *n);
int checkFileExists(const char *filename);
void measure_bubble_sort(int *arr, int n);
void measure_quick_sort(int *arr, int n);
void measure_stooge_sort(int *arr, int n);
void measure_radix_sort(int *arr, int n);
void measure_merge_sort(int *arr, int n);
void measure_bitonic_sort(int *arr, int n);
void measure_linear_search(int *arr, int n, int goal);
void measure_binary_search(int *arr, int n, int goal);
void measure_ternary_search(int *arr, int n, int goal);
void measure_jumping_search(int *arr, int n, int goal);
void fileFiller();
void sortingBenchmark();
void searchBenchmark();
void menu();

int compare_results(const void *a, const void *b) {
    const SortResult *ra = (const SortResult *)a;
    const SortResult *rb = (const SortResult *)b;
    if (ra->time < rb->time) return 1;
    if (ra->time > rb->time) return -1;
    return 0;
}

void show_results_chart_py() {
    system("python3 sorting_visualization.py sorting_result.csv");
    #ifdef _WIN32
        system("start sorting_results.png");
    #elif __APPLE__
        system("open sorting_results.png");
    #else
        system("xdg-open sorting_results.png");
    #endif
}

void show_results_search_py() {
    system("python3 search_visualization.py searching_result.csv");
    #ifdef _WIN32
        system("start search_results.png");
    #elif __APPLE__
        system("open search_results.png");
    #else
        system("xdg-open search_results.png");
    #endif
}

void show_results_chart() {
    FILE *file = fopen(RESULTS_FILE, "r");
    if (!file) {
        printf("No se encontraron resultados para mostrar.\n");
        return;
    }

    SortResult results[MAX_ALGORITHMS];
    int count = 0;
    char line[256];

    // read all results
    while (fgets(line, sizeof(line), file) && count < MAX_ALGORITHMS) {
        sscanf(line, "%49[^,],%d,%lf", results[count].algorithm, &results[count].size, &results[count].time);
        count++;
    }
    fclose(file);

    // merge result per time
    qsort(results, count, sizeof(SortResult), compare_results);

    // datafile for gnuplot
    FILE *gnuplot_data = fopen("chart_data.dat", "w");
    if (!gnuplot_data) {
        printf("Error al crear archivo de datos para el gráfico.\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        // label creation, algo name and time it took
        char label[100];
        snprintf(label, sizeof(label), "%s (%d)", results[i].algorithm, results[i].size);

        fprintf(gnuplot_data, "\"%s\" %f\n", label, results[i].time);
    }
    fclose(gnuplot_data);

    // script creation for gnuplot
    FILE *gnuplot_script = fopen("plot_script.gp", "w");
    if (!gnuplot_script) {
        printf("Error al crear script de GNUplot.\n");
        return;
    }

    fprintf(gnuplot_script,
        "set terminal pngcairo size 1024,768 enhanced font 'Verdana,10'\n"
        "set output 'sorting_results.png'\n"
        "set style data histograms\n"
        "set style histogram rowstacked\n"
        "set style fill solid border -1\n"
        "set boxwidth 0.75\n"
        "set title 'Tiempos de Ejecución de Algoritmos de Ordenamiento'\n"
        "set ylabel 'Tiempo (segundos)'\n"
        "set xtics rotate by -45\n"
        "plot 'chart_data.dat' using 2:xtic(1) notitle\n"
    );
    fclose(gnuplot_script);

    // execute gnuplot
    system("gnuplot plot_script.gp");

    printf("\nGráfico generado como 'sorting_results.png'\n");

    // opens the image automatically
    #ifdef _WIN32
        system("start sorting_results.png");
    #elif __APPLE__
        system("open sorting_results.png");
    #else
    // this is assumable
        system("xdg-open sorting_results.png");
    #endif
}

void write_result(const char *algorithm, int size, double time) {
    FILE *temp = fopen("temp_results.csv", "w");
    FILE *original = fopen(RESULTS_FILE, "r");
    int exists = 0;
    // write all the entries except for the updated one
    if (original) {
        char line[256];
        while (fgets(line, sizeof(line), original)) {
            char buf_alg[50];
            int buf_size;
            sscanf(line, "%49[^,],%d,", buf_alg, &buf_size);

            if (strcmp(buf_alg, algorithm) == 0 && buf_size == size) {
                fprintf(temp, "%s,%d,%.6f\n", algorithm, size, time);
                exists = 1;
            } else {
                fprintf(temp, "%s", line);
            }
        }
        fclose(original);
    }

    if (!exists) fprintf(temp, "%s,%d,%.6f\n", algorithm, size, time);
    fclose(temp);
    remove(RESULTS_FILE);
    rename("temp_results.csv", RESULTS_FILE);
}

void write_search_result(const char *algorithm, int size, double time) {
    FILE *temp = fopen("temp_search_results.csv", "w");
    FILE *original = fopen(SEARCH_RESULTS_FILE, "r");
    int exists = 0;
    if (original) {
        char line[256];
        while (fgets(line, sizeof(line), original)) {
            char buf_alg[50];
            int buf_size;
            sscanf(line, "%49[^,],%d,", buf_alg, &buf_size);

            if (strcmp(buf_alg, algorithm) == 0 && buf_size == size) {
                fprintf(temp, "%s,%d,%.6f\n", algorithm, size, time);
                exists = 1;
            } else {
                fprintf(temp, "%s", line);
            }
        }
        fclose(original);
    }

    if (!exists) fprintf(temp, "%s,%d,%.6f\n", algorithm, size, time);
    fclose(temp);
    remove(SEARCH_RESULTS_FILE);
    rename("temp_search_results.csv", SEARCH_RESULTS_FILE);
}

int read_result(const char *algorithm, int size, double *time) {
    FILE *file = fopen(RESULTS_FILE, "r");
    if (!file) return 0;

    char line[100];
    char saved_alg[50];
    int saved_size;
    double saved_time;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%49[^,],%d,%lf", saved_alg, &saved_size, &saved_time) == 3) {
            if (strcmp(saved_alg, algorithm) == 0 && saved_size == size) {
                *time = saved_time;
                fclose(file);
                return 1;
            }
        }
    }

    fclose(file);
    return 0;
}

int read_search_result(const char *algorithm, int size, double *time) {
    FILE *file = fopen(SEARCH_RESULTS_FILE, "r");
    if (!file) return 0;

    char line[100];
    char saved_alg[50];
    int saved_size;
    double saved_time;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%49[^,],%d,%lf", saved_alg, &saved_size, &saved_time) == 3) {
            if (strcmp(saved_alg, algorithm) == 0 && saved_size == size) {
                *time = saved_time;
                fclose(file);
                return 1;
            }
        }
    }

    fclose(file);
    return 0;
}

// I need to generate 8 digit random numbers, 1 million of them. Then load those numbers in a file.
void generateFileOfNumbers(const char *numbers, const int n) {

    FILE *result = fopen(numbers, "w");
    if (result == NULL) {
        printf("Error al crear el archivo\n");
        exit(1); // Shouldn't do this, but I am toooo lazyyy
    }

    srand(time(NULL)); // Seed for random numbers, a "key", even though it generates a predictable sequence of values
    for (int i = 0; i < n; i++) {
        int num = 10000000 + (rand() % 90000000);
        fprintf(result, "%d\n", num);
    }

    fclose(result);
    printf("--------------------------------------------------\n");
    printf("El archivo '%s' ahora tiene %d números.\n", numbers, n);
}

int *loadArrayFromFile(const char *filename, int *n) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error al abrir el archivo %s\n", filename);
        return NULL;
    }

    // Count number of lines
    int count = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') count++;
    }
    rewind(file);

    // Allocate memory
    int *arr = malloc(count * sizeof(int));
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read numbers
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%d", &arr[i]) != 1) {
            printf("Error reading file at line %d\n", i+1);
            free(arr);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);
    *n = count;
    return arr;
}

int checkFileExists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

void measure_bubble_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Bubble Sort";

    // As 1 million is to big to iterate, just estimate
    if (n == 1000000) {
        double time_100k;

        if (!read_result(alg_name, 100000, &time_100k)) {
                printf("Ejecuta primero las pruebas de 100 mil elementos.\n");
                return;
        }

        /*----------------------------------------------------------
          Explicación de la estimación:
          - Bubble Sort es O(n²) → Tiempo ∝ n²
          - Relación de tamaños: 1,000,000 / 100,000 = 10
          - Factor de escala: 10² = 100
        ----------------------------------------------------------*/
        double factor = 100.0;
        time_taken = time_100k * factor;
        write_result(alg_name, n, time_taken);
        printf("Tamaño: %d | Tiempo estimado: %.6f segundos\n", n, time_taken);
        return;
    }

    // Allocate memory and prepare the variables for the progress bar.
    int *temp_arr = malloc(n * sizeof(int));
    memcpy(temp_arr, arr, n * sizeof(int));

    int total_passes = n-1; // External for iteration total
    int update_interval = total_passes / 100; // Update every 1%
    if (update_interval < 1) update_interval = 1;

    // Start the count
    const clock_t start = clock();
    printf("\nProgreso: [");
    fflush(stdout);

    // Bubble Sort
    for (int i = 0; i < n-1; i++) {
        // Update the progress
        if (i % update_interval == 0) {
            int percent = (i * 100) / total_passes;
            printf("\rProgreso: [");
            for (int p = 0; p < 50; p++) {
                if (p < percent/2) printf("=");
                else printf(" ");
            }
            printf("] %d%%", percent);
            fflush(stdout);
        }
        // Algorithm itself
        for (int j = 0; j < n-i-1; j++) {
            if (temp_arr[j] > temp_arr[j+1]) {
                int temp = temp_arr[j];
                temp_arr[j] = temp_arr[j+1];
                temp_arr[j+1] = temp;
            }
        }
    }

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    const clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

void quick_sort_recursive(int *arr, int left, int right, int *progress, int total_elements) {
    if (left >= right) return;

    int pivot = arr[(left + right) / 2];
    int i = left, j = right;

    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) {
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
            i++;
            j--;
        }
    }

    // Update progress (approximation based on partitions)
    int current_progress = (i * 100) / total_elements;
    if (current_progress > *progress) {
        *progress = current_progress;
        printf("\rProgreso: [");
        for (int p = 0; p < 50; p++) {
            if (p < current_progress/2) printf("=");
            else printf(" ");
        }
        printf("] %d%%", current_progress);
        fflush(stdout);
    }

    quick_sort_recursive(arr, left, j, progress, total_elements);
    quick_sort_recursive(arr, i, right, progress, total_elements);
}

void measure_quick_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Quick Sort";

    // Create a temporary copy to preserve original array
    int *temp_arr = malloc(n * sizeof(int));
    if (temp_arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    memcpy(temp_arr, arr, n * sizeof(int));

    printf("\nProgreso: [");
    for (int p = 0; p < 50; p++) printf(" ");
    printf("] 0%%");
    fflush(stdout);

    int progress = 0;
    clock_t start = clock();

    // Call the recursive quick sort
    quick_sort_recursive(temp_arr, 0, n-1, &progress, n);

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

void stooge_sort_recursive(int *arr, int l, int h, int *progress, int total_elements) {
    if (l >= h) return;

    // If first element is smaller than last, swap them
    if (arr[l] > arr[h]) {
        int temp = arr[l];
        arr[l] = arr[h];
        arr[h] = temp;
    }

    // If there are more than 2 elements in the array
    if (h - l + 1 > 2) {
        int t = (h - l + 1) / 3;

        // Recursively sort first 2/3 elements
        stooge_sort_recursive(arr, l, h - t, progress, total_elements);

        // Recursively sort last 2/3 elements
        stooge_sort_recursive(arr, l + t, h, progress, total_elements);

        // Recursively sort first 2/3 elements again
        stooge_sort_recursive(arr, l, h - t, progress, total_elements);
    }

    // Update progress (very rough approximation)
    int current_progress = ((h - l) * 100) / total_elements;
    if (current_progress > *progress) {
        *progress = current_progress;
        printf("\rProgreso: [");
        for (int p = 0; p < 50; p++) {
            if (p < current_progress/2) printf("=");
            else printf(" ");
        }
        printf("] %d%%", current_progress);
        fflush(stdout);
    }
}

void measure_stooge_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Stooge Sort";

    // For large arrays, estimate based on 10k elements
    if (n == 100000 || n == 1000000) {
        double time_10k;

        if (!read_result(alg_name, 10000, &time_10k)) {
            printf("Ejecuta primero las pruebas de 10 mil elementos.\n");
            return;
        }

        /*----------------------------------------------------------
          Explicación de la estimación:
          - Stooge Sort es O(n^2.7)
          - Para 100,000: (100000/10000)^2.7 ≈ 10^2.7 ≈ 501
          - Para 1,000,000: (1000000/10000)^2.7 ≈ 100^2.7 ≈ 100,000
        ----------------------------------------------------------*/
        double factor;
        if (n == 100000) factor = 501.0;
        else factor = 100000.0;

        time_taken = time_10k * factor;
        write_result(alg_name, n, time_taken);
        printf("Tamaño: %d | Tiempo estimado: %.6f segundos\n", n, time_taken);
        return;
    }

    // Create a temporary copy to preserve original array
    int *temp_arr = malloc(n * sizeof(int));
    if (temp_arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    memcpy(temp_arr, arr, n * sizeof(int));

    printf("\nProgreso: [");
    for (int p = 0; p < 50; p++) printf(" ");
    printf("] 0%%");
    fflush(stdout);

    int progress = 0;
    clock_t start = clock();

    // Call the recursive stooge sort
    stooge_sort_recursive(temp_arr, 0, n-1, &progress, n);

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

int get_max(int *arr, int n) {
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

int get_min(int a, int b) {
    if (a > b){
        return a;
    }
    return b;
}

void counting_sort(int *arr, int n, int exp, int *progress, int total_passes, int current_pass) {
    int *output = malloc(n * sizeof(int));
    int count[10] = {0};

    // count each digit ocurrencnes
    for (int i = 0; i < n; i++) {
        count[(arr[i] / exp) % 10]++;
    }

    // changing count[i] for it to store the current pos
    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    // build output array
    for (int i = n - 1; i >= 0; i--) {
        output[count[(arr[i] / exp) % 10] - 1] = arr[i];
        count[(arr[i] / exp) % 10]--;
    }

    // copy the output array to the original array
    for (int i = 0; i < n; i++) {
        arr[i] = output[i];
    }

    free(output);

    // thx gpt, update the progress
    int current_progress = (current_pass * 100) / total_passes;
    if (current_progress > *progress) {
        *progress = current_progress;
        printf("\rProgreso: [");
        for (int p = 0; p < 50; p++) {
            if (p < current_progress/2) printf("=");
            else printf(" ");
        }
        printf("] %d%%", current_progress);
        fflush(stdout);
    }
}

void measure_radix_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Radix Sort";

    // ccreate a temporary copy to preserve original array
    int *temp_arr = malloc(n * sizeof(int));
    if (temp_arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    memcpy(temp_arr, arr, n * sizeof(int));

    printf("\nProgreso: [");
    for (int p = 0; p < 50; p++) printf(" ");
    printf("] 0%%");
    fflush(stdout);

    int progress = 0;
    clock_t start = clock();

    int max = get_max(temp_arr, n);

    // not necessary to find the digit amount of the biggest element as we know is 8
    // counting sort per digit
    int current_pass = 0;
    for (int exp = 1; max / exp > 0; exp *= 10) {
        const int total_passes = 8;
        current_pass++;
        counting_sort(temp_arr, n, exp, &progress, total_passes, current_pass);
    }

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

// merge from mergesort, famous
void merge(int *arr, int l, int m, int r, int *progress, int total_elements) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // first copy each subarray to a separate queue then copy the values into the arrays
    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));

    if (L == NULL || R == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // merge the arrays
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // copy left array leftover elements
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // copy right array leftover elements
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);

    // update progress (processed range processed)
    int current_progress = ((r - l) * 100) / total_elements;
    if (current_progress > *progress) {
        *progress = current_progress;
        printf("\rProgreso: [");
        for (int p = 0; p < 50; p++) {
            if (p < current_progress/2) printf("=");
            else printf(" ");
        }
        printf("] %d%%", current_progress);
        fflush(stdout);
    }
}

// "divide-and-conquer" mergesort routine plus progress sfollowup
void merge_sort_recursive(int *arr, int l, int r, int *progress, int total_elements) {

    int middle;
    if (l < r) {
        middle = l + (r - l) / 2;
        merge_sort_recursive(arr, l, middle, progress, total_elements);
        merge_sort_recursive(arr, middle + 1, r, progress, total_elements);
        merge(arr, l, middle, r, progress, total_elements);
    }
}

void measure_merge_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Merge Sort";

    // Create a temporary copy to preserve original array
    int *temp_arr = malloc(n * sizeof(int));
    if (temp_arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    memcpy(temp_arr, arr, n * sizeof(int));

    printf("\nProgreso: [");
    for (int p = 0; p < 50; p++) printf(" ");
    printf("] 0%%");
    fflush(stdout);

    int progress = 0;
    clock_t start = clock();

    // Call the recursive merge sort, this like a parent function, kinda broke ma head
    merge_sort_recursive(temp_arr, 0, n - 1, &progress, n);

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

void compare_and_swap(int *a, int *b, int dir) {
    if ((*a > *b && dir) || (*a < *b && !dir)) {
        int temp = *a;
        *a = *b;
        *b = temp;
    }
}

// merge bitonic "sequences"
void bitonic_merge(int *arr, int low, int cnt, int dir, int *progress, int total_elements) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            compare_and_swap(&arr[i], &arr[i + k], dir);
        }
        bitonic_merge(arr, low, k, dir, progress, total_elements);
        bitonic_merge(arr, low + k, k, dir, progress, total_elements);

        // update progress here, I can not tell if I am doing this righht
        int current_progress = ((low + cnt) * 100) / total_elements;
        if (current_progress > *progress) {
            *progress = current_progress;
            printf("\rProgreso: [");
            for (int p = 0; p < 50; p++) {
                if (p < current_progress/2) printf("=");
                else printf(" ");
            }
            printf("] %d%%", current_progress);
            fflush(stdout);
        }
    }
}

// recursuve bitonic sort call (supposed to be a sequential version)
void bitonic_sort_recursive(int *arr, int low, int cnt, int dir, int *progress, int total_elements) {
    if (cnt > 1) {
        int k = cnt / 2;

        // ascendant ordering
        bitonic_sort_recursive(arr, low, k, 1, progress, total_elements);

        // desdendant
        bitonic_sort_recursive(arr, low + k, k, 0, progress, total_elements);

        // merge the sequence, very similir a merge sort
        bitonic_merge(arr, low, cnt, dir, progress, total_elements);
    }
}

// thread function for the sort
void* bitonic_sort_thread(void *arg) {
    BitonicParams *params = (BitonicParams *)arg;
    bitonic_sort_recursive(params->arr, params->low, params->cnt, params->dir,
                          params->progress, params->total_elements);
    return NULL;
}

void concurrent_bitonic_sort(int *arr, int n, int *progress) {
    const int num_threads = 4; // Número de hilos a usar
    pthread_t threads[num_threads];
    BitonicParams params[num_threads];

    int chunk_size = n / num_threads;
    for (int i = 0; i < num_threads; i++) {
        params[i].arr = arr;
        params[i].low = i * chunk_size;
        params[i].cnt = (i == num_threads - 1) ? (n - i * chunk_size) : chunk_size;
        params[i].dir = 1;
        params[i].progress = progress;
        params[i].total_elements = n;

        pthread_create(&threads[i], NULL, bitonic_sort_thread, &params[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    bitonic_merge(arr, 0, n, 1, progress, n);
}

void measure_bitonic_sort(int *arr, int n) {
    double time_taken;
    const char *alg_name = "Bitonic Sort";

    // Para arrays grandes, estimar basado en 10k elementos
    // if (n == 100000 || n == 1000000) {
    if (n == 0) {

        double time_10k;

        if (!read_result(alg_name, 10000, &time_10k)) {
            printf("Ejecuta primero las pruebas de 10 mil elementos.\n");
            return;
        }

        /*----------------------------------------------------------
          Explicación de la estimación:
          - Bitonic Sort es O(n log²n)
          - Para 100,000: (100000 log²100000)/(10000 log²10000) ≈ 14.5
          - Para 1,000,000: (1000000 log²1000000)/(10000 log²10000) ≈ 217
        ----------------------------------------------------------*/
        double factor;
        if (n == 100000) factor = 14.5;
        else factor = 217.0;

        time_taken = time_10k * factor;
        write_result(alg_name, n, time_taken);
        printf("Tamaño: %d | Tiempo estimado: %.6f segundos\n", n, time_taken);
        return;
    }

    // ccreate a temporary copy to preserve original array
    int *temp_arr = malloc(n * sizeof(int));
    if (temp_arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    memcpy(temp_arr, arr, n * sizeof(int));

    printf("\nProgreso: [");
    for (int p = 0; p < 50; p++) printf(" ");
    printf("] 0%%");
    fflush(stdout);

    int progress = 0;
    clock_t start = clock();

    // choose between sequential or concurrential version
    if (n >= 10000) {
        // generally used for "big" arrays
        concurrent_bitonic_sort(temp_arr, n, &progress);
    } else {
        // used for "small" arrays
        bitonic_sort_recursive(temp_arr, 0, n, 1, &progress, n);
    }

    printf("\rProgreso: [");
    for (int p = 0; p < 50; p++) printf("=");
    printf("] 100%%\n");

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    write_result(alg_name, n, time_taken);
    free(temp_arr);
    printf("Tamaño: %d | Tiempo: %.6f segundos\n", n, time_taken);
}

void measure_linear_search(int *arr, int n, int goal) {
    double time_taken;
    const char *alg_name = "Linear Search";

    clock_t start = clock();

    int found = 0;
    int position = -1;
    for (int i = 0; i < n; i++) {
        if (arr[i] == goal) {
            found = 1;
            position = i;
            break;
        }
    }

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    write_search_result(alg_name, n, time_taken);

    printf("Algoritmo: Búsqueda Lineal\n");
    printf("Elemento %d %s\n", goal, found ? "encontrado" : "no encontrado");
    if (found) printf("Posición: %d\n", position);
    printf("Tiempo: %.6f segundos\n", time_taken);
}

void measure_binary_search(int *arr, int n, int goal) {
    double time_taken;
    const char *alg_name = "Binary Search";

    clock_t start = clock();

    int left = 0, right = n - 1;
    int found = 0, position = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (arr[mid] == goal) {
            found = 1;
            position = mid;
            break;
        }

        if (arr[mid] < goal) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    write_search_result(alg_name, n, time_taken);

    printf("Algoritmo: Búsqueda Binaria\n");
    printf("Elemento %d %s\n", goal, found ? "encontrado" : "no encontrado");
    if (found) printf("Posición: %d\n", position);
    printf("Tiempo: %.6f segundos\n", time_taken);
}

void measure_ternary_search(int *arr, int n, int goal) {
    double time_taken;
    const char *alg_name = "Ternary Search";

    clock_t start = clock();

    int left = 0, right = n - 1;
    int found = 0, position = -1;

    while (left <= right) {
        int mid1 = left + (right - left) / 3;
        int mid2 = right - (right - left) / 3;

        if (arr[mid1] == goal) {
            found = 1;
            position = mid1;
            break;
        }

        if (arr[mid2] == goal) {
            found = 1;
            position = mid2;
            break;
        }

        if (goal < arr[mid1]) {
            right = mid1 - 1;
        } else if (goal > arr[mid2]) {
            left = mid2 + 1;
        } else {
            left = mid1 + 1;
            right = mid2 - 1;
        }
    }

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    write_search_result(alg_name, n, time_taken);

    printf("Algoritmo: Búsqueda Ternaria\n");
    printf("Elemento %d %s\n", goal, found ? "encontrado" : "no encontrado");
    if (found) printf("Posición: %d\n", position);
    printf("Tiempo: %.6f segundos\n", time_taken);
}

void measure_jumping_search(int *arr, int n, int goal) {
    double time_taken;
    const char *alg_name = "Jumping Search";

    clock_t start = clock();

    int step = sqrt(n);
    int prev = 0;
    int found = 0, position = -1;

    while (arr[get_min(step, n) - 1] < goal) {
        prev = step;
        step += sqrt(n);
        if (prev >= n) break;
    }

    while (arr[prev] < goal) {
        prev++;
        if (prev == get_min(step, n)) break;
    }

    if (arr[prev] == goal) {
        found = 1;
        position = prev;
    }

    clock_t end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    write_search_result(alg_name, n, time_taken);

    printf("Algoritmo: Búsqueda por Saltos\n");
    printf("Elemento %d %s\n", goal, found ? "encontrado" : "no encontrado");
    if (found) printf("Posición: %d\n", position);
    printf("Tiempo: %.6f segundos\n", time_taken);
}

// handles the user input
void fileFiller() {
    char input[100];
    char *endptr;

    while (1) {
        printf("--------------------------------------------------\n");
        printf("\n--- Generador de Archivos con Números Aleatorios ---\n");
        printf("1. Generar 10,000 números (archivo: datos_10k.txt)\n");
        printf("2. Generar 100,000 números (archivo: datos_100k.txt)\n");
        printf("3. Generar 1,000,000 números (archivo: datos_1M.txt)\n");
        printf("4. Salir\n");
        printf("Seleccione una opción (1-4): ");

        // Read input as string
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }

        // Reset errno before conversion, we then validate the conversion
        errno = 0;
        const long int option = strtol(input, &endptr, 10);
        if (endptr == input) {
            printf("--------------------------------------------------\n");
            printf("Error: No se ingresó un número. Intente de nuevo.\n");
            continue;
        }
        if (*endptr != '\n' && *endptr != '\0') {
            printf("--------------------------------------------------\n");
            printf("Error: Entrada inválida (caracteres no numéricos). Intente de nuevo.\n");
            continue;
        }
        if (errno == ERANGE || option < INT_MIN || option > INT_MAX) {
            printf("--------------------------------------------------");
            printf("Error: Número fuera de rango. Intente de nuevo.\n");
            continue;
        }
        if (option < 1 || option > 4) {
            printf("--------------------------------------------------\n");
            printf("Error: La opción debe ser entre 1 y 4. Intente de nuevo.\n");
            continue;
        }

        // Input valid, process the option, no default as WE do NOT respect switch
        switch (option) {
            case 1:
                generateFileOfNumbers("datos_10k.txt", 10000);
                break;
            case 2:
                generateFileOfNumbers("datos_100k.txt", 100000);
                break;
            case 3:
                generateFileOfNumbers("datos_1M.txt", 1000000);
                break;
            case 4:
                printf("\n");
                printf("Volviendo al menú principal...\n");
                return;
        }
    }
}

void menu() {

    char input[100];
    char *endptr;

    while (1) {
        printf("\n=== MENÚ PRINCIPAL ===\n");
        printf("1. Generador de archivos\n");
        printf("2. Benchmark de algoritmos de ordenamiento\n");
        printf("3. Benchmark de algoritmos de búsqueda\n");
        printf("4. Mostrar resultados gráficos de ordenamiento\n");
        printf("5. Mostrar resultados gráficos de búsqueda y captura\n");
        printf("6. Salir\n");
        printf("Seleccione una opción (1-6): ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error leyendo entrada.\n");
            continue;
        }

        errno = 0;
        const long int option = strtol(input, &endptr, 10);

        if (endptr == input || (*endptr != '\n' && *endptr != '\0') ||
            errno == ERANGE || option < 1 || option > 6) {
            printf("Entrada inválida. Intente de nuevo.\n");
            continue;
            }

        switch (option) {
            case 1:
                fileFiller();
                break;
            case 2:
                sortingBenchmark();
                break;
            case 3:
                searchBenchmark();
                break;
            case 4:
                show_results_chart_py();
                break;
            case 5:
                show_results_search_py();
                break;
            case 6:
                printf("Saliendo del programa...\n");
                exit(0);
        }
    }
}

int get_random_number_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    int count = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') count++;
    }

    if (count == 0) {
        fclose(file);
        return -1;
    }

    rewind(file);
    int random_index = rand() % count;
    int number = -1;

    for (int i = 0; i <= random_index; i++) {
        if (fscanf(file, "%d", &number) != 1) {
            number = -1;
            break;
        }
    }

    fclose(file);
    return number;
}

int compare_ints(const void *a, const void *b) {
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

void searchBenchmark() {
    char input[100];
    char *endptr;
    int goal = 0;
    int use_random = 0;
    const char *filenames[] = {"datos_10k.txt", "datos_100k.txt", "datos_1M.txt"};

    while (1) {
        printf("\n=== MENÚ DE BÚSQUEDA ===\n");
        printf("1. Búsqueda Lineal\n");
        printf("2. Búsqueda Binaria (requiere array ordenado)\n");
        printf("3. Búsqueda Ternaria (requiere array ordenado)\n");
        printf("4. Búsqueda por Saltos\n");
        printf("5. Volver al menú principal\n");
        printf("Seleccione un algoritmo (1-5): ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error leyendo entrada.\n");
            continue;
        }

        errno = 0;
        const long int option = strtol(input, &endptr, 10);

        if (endptr == input || (*endptr != '\n' && *endptr != '\0') ||
            errno == ERANGE || option < 1 || option > 5) {
            printf("Entrada inválida. Intente de nuevo.\n");
            continue;
        }

        if (option == 5) return;

        while (1) {
            printf("\n=== SELECCIÓN DEL NÚMERO A BUSCAR ===\n");
            printf("1. Usar número aleatorio de datos_10k.txt\n");
            printf("2. Usar número aleatorio de datos_100k.txt\n");
            printf("3. Usar número aleatorio de datos_1M.txt\n");
            printf("4. Ingresar número manualmente (8 dígitos)\n");
            printf("Seleccione una opción (1-4): ");

            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("Error leyendo entrada de selección.\n");
                continue;
            }

            errno = 0;
            const long int search_option = strtol(input, &endptr, 10);

            if (endptr == input || (*endptr != '\n' && *endptr != '\0') ||
                errno == ERANGE || search_option < 1 || search_option > 4) {
                printf("Entrada inválida. Intente de nuevo.\n");
                continue;
            }

            if (search_option >= 1 && search_option <= 3) {
                goal = get_random_number_from_file(filenames[search_option - 1]);
                if (goal == -1) {
                    printf("Error al obtener número aleatorio. Genere los archivos primero.\n");
                    continue;
                }
                use_random = 1;
                printf("Número aleatorio seleccionado: %d\n", goal);
            } else {
                printf("Ingrese el número a buscar (8 dígitos): ");
                if (fgets(input, sizeof(input), stdin) == NULL) {
                    printf("Error leyendo entrada.\n");
                    continue;
                }

                errno = 0;
                goal = strtol(input, &endptr, 10);
                if (endptr == input || (*endptr != '\n' && *endptr != '\0') ||
                    errno == ERANGE || goal < 10000000 || goal > 99999999) {
                    printf("Error: Debe ingresar un número de 8 dígitos.\n");
                    continue;
                }
                use_random = 0;
            }
            break;
        }

        printf("\n=== RESULTADOS ===\n");

        for (int i = 0; i < 3; i++) {
            if (!checkFileExists(filenames[i])) {
                printf("\nArchivo %s no encontrado. Genere los archivos primero.\n", filenames[i]);
                continue;
            }

            int n;
            int *arr = loadArrayFromFile(filenames[i], &n);
            if (arr == NULL) continue;

            // ordered array is needed
            if (option == 2 || option == 3 || option == 4) {
                printf("\nOrdenando el array para búsqueda binaria/ternaria/saltos...\n");
                int *temp_arr = malloc(n * sizeof(int));
                if (temp_arr == NULL) {
                    printf("Error: No se pudo asignar memoria para el array temporal\n");
                    free(arr);
                    continue;
                }
                memcpy(temp_arr, arr, n * sizeof(int));
                qsort(temp_arr, n, sizeof(int), compare_ints);
                free(arr);
                arr = temp_arr;
                printf("Array ordenado correctamente.\n");
            }

            printf("\n--- Archivo: %s ---\n", filenames[i]);

            switch (option) {
                case 1:
                    measure_linear_search(arr, n, goal);
                    break;
                case 2:
                    measure_binary_search(arr, n, goal);
                    break;
                case 3:
                    measure_ternary_search(arr, n, goal);
                    break;
                case 4:
                    measure_jumping_search(arr, n, goal);
                    break;
            }
            free(arr);
            // using the same random number
            // if (use_random) break;
        }
    }
}


void sortingBenchmark() {
    char input[100];
    char *endptr;

    while (1) {
        printf("\n=== SELECCIONE ALGORITMO ===\n");
        printf("1. Bubble Sort\n");
        printf("2. Quick Sort\n");
        printf("3. Stooge Sort\n");
        printf("4. Radix Sort\n");
        printf("5. Merge Sort\n");
        printf("6. Bitonic Sort\n");
        printf("7. Volver al menú principal\n");
        printf("Seleccione una opción (1-7): ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error leyendo entrada.\n");
            continue;
        }

        errno = 0;
        const long int option = strtol(input, &endptr, 10);

        if (endptr == input || (*endptr != '\n' && *endptr != '\0') ||
            errno == ERANGE || option < 1 || option > 7) {
            printf("Entrada inválida. Intente de nuevo.\n");
            continue;
            }

        if (option == 7) return;

        const char *filenames[] = {"datos_10k.txt", "datos_100k.txt", "datos_1M.txt"};

        printf("\n=== RESULTADOS ===\n");

        for (int i = 0; i < 3; i++) {
            if (!checkFileExists(filenames[i])) {
                printf("\nArchivo %s no encontrado. Genere los archivos primero.\n", filenames[i]);
                return;
            }

            int n;
            int *arr = loadArrayFromFile(filenames[i], &n);
            if (arr == NULL) continue;

            switch (option) {
                case 1:
                    measure_bubble_sort(arr, n);
                    break;
                case 2:
                    measure_quick_sort(arr, n);
                    break;
                case 3:
                    measure_stooge_sort(arr, n);
                    break;
                case 4:
                    measure_radix_sort(arr, n);
                    break;
                case 5:
                    measure_merge_sort(arr, n);
                    break;
                case 6:
                    measure_bitonic_sort(arr, n);
                    break;
            }
            free(arr);
        }
    }
}

int main(void) {
    menu();
    return 0;
}