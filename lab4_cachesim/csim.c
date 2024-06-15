#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

typedef struct
{
    int dirty;
    int valid;
    int tag;
    int TimeStamp;
} Cacheline_with_dirty;
typedef struct
{
    int S;
    int E;
    int B;
    Cacheline_with_dirty **lines;
} Cache;

int S = 0, E = 0, B = 0;
int total_size = 0;
Cache *i_cache;
Cache *d_cache;
int verbose = 0;
int replace_policy = -1, miss_cost = -1, write_policy = -1;

int lines_cnt = 0;
int W_cnt = 0;
int R_cnt = 0;
int d_hit_r = 0, d_hit_w = 0;
int i_hit = 0;

int runtime_total = 0;
double avg_latency = 0.0;

int log_2(int S)
{
    int z = 0;
    int s1 = S;
    if (s1 == 1)
        return 0;
    while (s1 > 1)
    {
        s1 = s1 / 2;
        z++;
    }
    return z;
}

void init_cache(int S, int E, int B)
{
    i_cache = (Cache *)malloc(sizeof(Cache));
    i_cache->S = S; // 组相联的组数
    i_cache->E = E; // 每组的行数
    i_cache->B = B; // B = 2^b = block size    b = #block bits
    Cacheline_with_dirty **mycachelines = (Cacheline_with_dirty **)malloc(sizeof(Cacheline_with_dirty *) * S);
    for (int i = 0; i < S; i++)
    {
        mycachelines[i] = (Cacheline_with_dirty *)malloc(sizeof(Cacheline_with_dirty) * E);
        for (int j = 0; j < E; j++)
        {
            mycachelines[i][j].dirty = 0;
            mycachelines[i][j].valid = 0; // 初始时，高速缓存是空的
            mycachelines[i][j].tag = -1;
            mycachelines[i][j].TimeStamp = 0;
        }
        if (mycachelines[i] == NULL)
            exit(EXIT_FAILURE);
    }
    i_cache->lines = mycachelines;
    // printf("INFO-- I-CACHE MEMORY ALLOCATED\n");

    d_cache = (Cache *)malloc(sizeof(Cache));
    d_cache->S = S; // 组相联的组数
    d_cache->E = E; // 每组的行数
    d_cache->B = B; // B = 2^b = block size    b = #block bits
    Cacheline_with_dirty **d_mycachelines = (Cacheline_with_dirty **)malloc(sizeof(Cacheline_with_dirty *) * S);
    for (int i = 0; i < S; i++)
    {
        d_mycachelines[i] = (Cacheline_with_dirty *)malloc(sizeof(Cacheline_with_dirty) * E);
        for (int j = 0; j < E; j++)
        {
            d_mycachelines[i][j].dirty = 0;
            d_mycachelines[i][j].valid = 0; // 初始时，高速缓存是空的
            d_mycachelines[i][j].tag = -1;
            d_mycachelines[i][j].TimeStamp = 0;
        }
        if (d_mycachelines[i] == NULL)
            exit(EXIT_FAILURE);
    }
    d_cache->lines = d_mycachelines;
    // printf("INFO-- D-CACHE MEMORY ALLOCATED\n");
}

void free_cache()
{
    for (int i = 0; i < S; i++)
    {
        free(i_cache->lines[i]);
    }
    free(i_cache->lines);
    free(i_cache);

    for (int i = 0; i < S; i++)
    {
        free(d_cache->lines[i]);
    }
    free(d_cache->lines);
    free(d_cache);
}

int IsFull(int op_s, Cache *mycache)
{ // 全满返回-1，未满返回找到的第一个空行
    for (int i = 0; i < mycache->E; i++)
    {
        if (mycache->lines[op_s][i].valid == 0)
            return i;
    }
    return -1; //-1证明全满了
}
void update(int i, int op_s, int op_tag, Cache *mycache)
{ // LRU时间戳更新：以组为单位
    mycache->lines[op_s][i].valid = 1;
    mycache->lines[op_s][i].tag = op_tag;
    for (int k = 0; k < mycache->E; k++)
        if (mycache->lines[op_s][k].valid == 1) // 遍历每一行，组中每个时间戳都会变大，表示它离最后操作的时间变久
            mycache->lines[op_s][k].TimeStamp++;
    mycache->lines[op_s][i].TimeStamp = 0; // 将本次操作的行时间戳设置为最小，也就是0
}
int find_max_LRU(int op_s, Cache *mycache)
{
    //  int max_T = i_cache->lines[op_s][0].TimeStamp;
    int max_T = 0;
    int max_T_index = 0;
    for (int k = 0; k < mycache->E; k++)
        if (mycache->lines[op_s][k].TimeStamp > max_T)
        { // 不需要mycache->lines[op_s][k].valid == 1，因为进入find_max_LRU之前已经IsFull
            max_T = mycache->lines[op_s][k].TimeStamp;
            max_T_index = k;
        }
    return max_T_index;
}

void write_back(int op_s, int i, Cache *mycache)
{
    // 修改内存中数据
    mycache->lines[op_s][i].dirty == 0;
}

void LRU_update_R_d(int op_s, int op_tag, Cache *mycache) // 对于load指令
{                                                         // 缓存搜索及更新
    int index_get = -1;
    int i;
    for (i = 0; i < mycache->E; i++) // E不行，必须mycache->E
    {
        // 看在不在cache中
        if (mycache->lines[op_s][i].valid && mycache->lines[op_s][i].tag == op_tag)
        {
            index_get = i;
            break;
        }
    }
    if (index_get == -1)
    { // miss
        // R_miss_count++;
        int k = IsFull(op_s, mycache);
        if (k == -1)
        {
            // R_eviction_count++;
            k = find_max_LRU(op_s, mycache); // 不是index_get

            if (mycache->lines[op_s][k].dirty == 1)
                write_back(op_s, k, mycache);
        }

        update(k, op_s, op_tag, mycache);
    }
    else
    { // hit
        d_hit_r++;
        update(index_get, op_s, op_tag, mycache);
    }
}
void LRU_update_R_i(int op_s, int op_tag, Cache *mycache) // 对于load指令
{                                                         // 缓存搜索及更新
    int index_get = -1;
    int i;
    for (i = 0; i < mycache->E; i++) // E不行，必须mycache->E
    {
        // 看在不在cache中
        if (mycache->lines[op_s][i].valid && mycache->lines[op_s][i].tag == op_tag)
        {
            index_get = i;
            break;
        }
    }
    if (index_get == -1)
    { // miss
        // R_miss_count++;
        int k = IsFull(op_s, mycache);
        if (k == -1)
        {
            // R_eviction_count++;
            k = find_max_LRU(op_s, mycache); // 不是index_get

            if (mycache->lines[op_s][k].dirty == 1)
                write_back(op_s, k, mycache);
        }

        update(k, op_s, op_tag, mycache);
    }
    else
    { // hit
        i_hit++;
        update(index_get, op_s, op_tag, mycache);
    }
}

void LRU_update_W(int op_s, int op_tag, Cache *mycache) // 对于store指令
{                                                       // 缓存搜索及更新
    int index_get = -1;
    int i;
    for (i = 0; i < mycache->E; i++)
    { // E就不行，mycache->E可以
        if (mycache->lines[op_s][i].valid && mycache->lines[op_s][i].tag == op_tag)
        {
            index_get = i;
            break;
        }
    }
    if (index_get == -1)
    { // miss
        // W_miss_count++;
        int k = IsFull(op_s, mycache);
        if (k == -1)
        {
            // W_eviction_count++;
            k = find_max_LRU(op_s, mycache); // 不是index_get

            if (mycache->lines[op_s][k].dirty == 1)
                write_back(op_s, k, mycache);
        }

        update(k, op_s, op_tag, mycache);
    }
    else
    { // hit
        d_hit_w++;
        update(index_get, op_s, op_tag, mycache);
    }
}

void monitor(char *trace_file, int s, int E, int b)
{ // 注意这里应是s，不是S
    FILE *pFile;
    pFile = fopen(trace_file, "r");
    // if (pFile == NULL)
    //     printf("failed to open trace file\n");
    // else
    //     printf("successfully open trace file\n");
    unsigned long long int i_address;
    char identifier;
    unsigned long long int data_addr;
    while (fscanf(pFile, "%llx: %c %llx", &i_address, &identifier, &data_addr) > 0) // fscanf 在处理格式字符串时默认跳过空白符
    {
        // printf("0x%llx: %c, 0x%llx", i_address, identifier, data_addr);
        int op_tag = data_addr >> (s + b);
        int op_s = (data_addr >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s)); // 无符号数右移, 只保留最后s位
        int i_op_tag = i_address >> (s + b);
        int i_op_s = (i_address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s));

        switch (identifier)
        {
        case 'R':
        {
            LRU_update_R_d(op_s, op_tag, d_cache);
            LRU_update_R_i(i_op_s, i_op_tag, i_cache);

            lines_cnt++;       // 总共多少条指令
            R_cnt = R_cnt + 2; // 总共多少load指令
            break;
        }
        case 'W':
        {
            LRU_update_W(op_s, op_tag, d_cache);
            LRU_update_R_i(i_op_s, i_op_tag, i_cache);

            lines_cnt++;
            W_cnt++; // 总共多少store指令
            R_cnt++;
            break;
        }
        }
    }
    fclose(pFile);
    return;
}

void monitor_for_S_0(char *trace_file, int E, int b) // 全相联
{                                                    // 注意这里应是s，不是S
    FILE *pFile;
    pFile = fopen(trace_file, "r");
    // if (pFile == NULL)
    //     printf("failed to open trace file\n");
    // else
    //     printf("successfully open trace file\n");
    unsigned long long int i_address;
    char identifier;
    unsigned long long int data_addr;
    while (fscanf(pFile, "%llx: %c %llx", &i_address, &identifier, &data_addr) > 0) // fscanf 在处理格式字符串时默认跳过空白符
    {
        // printf("0x%llx: %c, 0x%llx", i_address, identifier, data_addr);
        int op_tag = data_addr >> b;
        int i_op_tag = i_address >> b;

        switch (identifier)
        {
        case 'R':
        {
            LRU_update_R_d(0, op_tag, d_cache);
            LRU_update_R_i(0, i_op_tag, i_cache);

            lines_cnt++;       // 总共多少条指令
            R_cnt = R_cnt + 2; // 总共多少load指令
            break;
        }
        case 'W':
        {
            LRU_update_W(0, op_tag, d_cache);
            LRU_update_R_i(0, i_op_tag, i_cache);

            lines_cnt++;
            W_cnt++; // 总共多少store指令
            R_cnt++;
            break;
        }
        }
    }
    fclose(pFile);
    return;
}

void read_config(char *file_path, char *trace_file)
{
    FILE *file = fopen(file_path, "r");

    fscanf(file, "%d", &B);          // 块大小：以字节为单位的缓存块(或cache line)大小，该值应该是2的幂次；
    fscanf(file, "%d", &S);          // 关联性：指定缓存的关联性。值“1”表示直接映射缓存，而值“0”表示完全关联，可以为2的非负幂次，代表2^k^组相联；
    fscanf(file, "%d", &total_size); // 数据大小：指定缓存中数据的总大小。这不包括任何开销（例如标记大小），以KB为单位且是2的非负幂。值“64”表示64KB缓存。
    total_size = total_size * 1024;
    fscanf(file, "%d", &replace_policy); // 替换政策：指定要使用的替换策略。值“0”表示随机替换，值“1”表示LRU,其余值无效；
    fscanf(file, "%d", &miss_cost);      // 非命中开销：指定缓存未命中时的额外周期数，可以是任何正整数
    fscanf(file, "%d", &write_policy);   // 写入分配：在缓存非命中时的策略。值“0”表示非写分配，值“1”表示写入分配，其余值无效。
    E = total_size / B;                  // 总行数
    if (S == 1)
    { // 直接映射
        S = E;
        E = 1;
        init_cache(S, E, B); // 初始化一个cache

        int s = log_2(S);
        int b = log_2(B);
        printf("CHECK-- B:%d,S:%d,total_size:%d,E:%d\n", B, S, total_size, E);
        printf("CHECK-- s:%d, b:%d\n", s, b);
        printf("-----\n");

        monitor(trace_file, s, E, b);
        // printf("CHECK-- miss cost: %d\n", miss_cost);
    }
    else if (S == 0)
    {
        S = 1;               // 全相联：每个主存块映射到Cache的任一行
        init_cache(S, E, B); // 初始化一个cache

        int s = log_2(S);
        int b = log_2(B);
        printf("CHECK-- B:%d,S:%d,total_size:%d,E:%d\n", B, S, total_size, E);
        printf("CHECK-- s:%d, b:%d\n", s, b);
        printf("-----\n");

        monitor_for_S_0(trace_file, E, b);
        // printf("CHECK-- miss cost: %d\n", miss_cost);
    }
    else
    {                        // 组相联
        E = E / S;           // 每组行数
        init_cache(S, E, B); // 初始化一个cache

        int s = log_2(S);
        int b = log_2(B);
        printf("CHECK-- B:%d,S:%d,total_size:%d,E:%d\n", B, S, total_size, E);
        printf("CHECK-- s:%d, b:%d\n", s, b);
        printf("-----\n");

        monitor(trace_file, s, E, b);
        // printf("CHECK-- miss cost: %d\n", miss_cost);
    }
}

int main(int argc, char *argv[])
{
    char opt;
    extern int E, S, B, miss_cost;
    char *config_file;
    char *trace_file;
    char *output_file = NULL; // 初始化为NULL或默认值
    double hit_total = 0, hit_load = 0, hit_store = 0, avg_latency = 0;
    int runtime_total = 0;

    while ((opt = getopt(argc, argv, "c:t:o:")) != -1) // getopt 函数每次调用时都会返回下一个被解析的选项字符，如果所有选项都已经被处理完毕，则返回 -1
    {
        switch (opt)
        {
        case 'c':
            config_file = optarg; // 当使用 getopt 函数来解析命令行参数时，optarg 是一个全局变量，用于存储当前选项的参数值。
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -c config_file -t trace_file -o output_file\n", argv[0]);
            return -1;
        }
    }
    if (!config_file || !trace_file)
    {
        fprintf(stderr, "All options -c, -t, and -o must be provided\n");
        return 1;
    }
    read_config(config_file, trace_file);

    hit_total = (double)(d_hit_r + d_hit_w) / lines_cnt * 100; // 除法操作涉及到一个 double 类型，整个表达式的结果将以 double 类型进行计算和返回
    hit_load = (double)(d_hit_r + i_hit) / R_cnt * 100;
    hit_store = (double)d_hit_w / W_cnt * 100;
    runtime_total = (R_cnt + W_cnt) + (R_cnt + W_cnt - (d_hit_r + d_hit_w + i_hit)) * miss_cost; // 程序的总运行时间，其单位应为周期数；

    avg_latency = (double)runtime_total / (R_cnt + W_cnt);

    // printSummary(output_file, hit_total, hit_load, hit_store, runtime_total, avg_latency);
    FILE *output_fp;
    if (output_file)
    {
        output_fp = fopen(output_file, "w");
        if (!output_fp)
        {
            perror("Failed to open output file");
            return 1;
        }
    }
    else
    {
        output_fp = stdout; // 直接指定为标准输出
    }

    fprintf(output_fp, "Total Hit Rate:%.4f%%\n", hit_total);
    fprintf(output_fp, "Load Hit Rate:%.2f%%\n", hit_load);
    fprintf(output_fp, "Store Hit Rate:%.2f%%\n", hit_store);
    fprintf(output_fp, "Total Run Time:%d\n", runtime_total);
    fprintf(output_fp, "AVG MA Latency:%.2f\n", avg_latency);

    // 只在打开的是文件时才关闭它，避免关闭标准输出
    if (output_fp != stdout)
    {
        fclose(output_fp);
    }

    free_cache();

    return 0;
}