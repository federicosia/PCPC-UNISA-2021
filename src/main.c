#include <time.h>
#include "../headers/word_counter.h"

int main(int argc, char* argv[])
{   
    clock_t start, end;
    double time_used;

    start = clock();
    dict_p dict = word_counting(argc, argv);
    end = clock();

    time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    print_csv(dict);
    printf("Time elapsed: %.2f sec(s)\n", time_used);
}