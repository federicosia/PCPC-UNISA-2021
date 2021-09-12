#include "../include/word_counter.h"

int main(int argc, char* argv[])
{   
    dict_p dict = word_counting(argc, argv);

    print_csv(dict);
}