#include <stdio.h>
#include <stdlib.h>
#include "../headers/utils.h"
#include "../headers/dict.h"

/*
size  -> bytes to analyze
start -> from the first file, starting byte
end -> last byte of the last file to analyze
file_indexes -> list of files indexes
*/
typedef struct{
    int index;
} file;

typedef struct{
    int size;
    int start;
    int end;
    int num_files;
    file *files;
} DATA;

DATA data_init(int files);
void word_counter(DATA data, dict_p dict, char* argv[]);//, int file_counter);
char* data_print(DATA data);