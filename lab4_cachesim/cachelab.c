/*
 * cachelab.c - Cache Lab helper functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cachelab.h"
#include <time.h>

trans_func_t func_list[MAX_TRANS_FUNCS];
int func_counter = 0;

/*
 * printSummary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded.
 */
void printSummary(const char *output_file, double hit_total, double hit_load, double hit_store, int runtime_total, double avg_latency)
{
    FILE *output_fp = fopen("output_file", "w");
    assert(output_fp);
    fprintf(output_fp, "Total Hit Race:%.4f%%\n", hit_total);
    fprintf(output_fp, "Load Hit Race:%.2f%%\n", hit_load);
    fprintf(output_fp, "Store Hit Race:%.2f%%\n", hit_store);
    fprintf(output_fp, "Total Run Time:%d\n", runtime_total);
    fprintf(output_fp, "AVG MA Latency:%.2f\n", avg_latency);

    fclose(output_fp);
}

/*
 * initMatrix - Initialize the given matrix
 */
void initMatrix(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            // A[i][j] = i+j;  /* The matrix created this way is symmetric */
            A[i][j] = rand();
            B[j][i] = rand();
        }
    }
}

void randMatrix(int M, int N, int A[N][M])
{
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            // A[i][j] = i+j;  /* The matrix created this way is symmetric */
            A[i][j] = rand();
        }
    }
}

/*
 * correctTrans - baseline transpose function used to evaluate correctness
 */
void correctTrans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerTransFunction - Add the given trans function into your list
 *     of functions to be tested
 */
void registerTransFunction(void (*trans)(int M, int N, int[N][M], int[M][N]),
                           char *desc)
{
    func_list[func_counter].func_ptr = trans;
    func_list[func_counter].description = desc;
    func_list[func_counter].correct = 0;
    func_list[func_counter].num_hits = 0;
    func_list[func_counter].num_misses = 0;
    func_list[func_counter].num_evictions = 0;
    func_counter++;
}
