#include <stdio.h>
#include <stdlib.h>
#include "../include/utils.h"
#include "../include/dict.h"

typedef struct{
    int index;
} file;

typedef struct{
    int size;
    int start;
    int end;
    int num_files;
    file *files;
} files_info;

files_info files_info_init(int files);
void word_counter(files_info data, dict_p dict, char* argv[]);//, int file_counter);
char* files_info_print(int rank, files_info data);