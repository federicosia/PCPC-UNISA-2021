#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/utils.h"

#define STRLEN 50

typedef struct {
    char name[STRLEN];
    int occurrences;
} dict_entry;

typedef struct {
    int num_entries;
    int size;
    dict_entry *entry;
} dict_v, *dict_p;

dict_p dict_create();
void dict_increase_size(dict_p dict);
int dict_find_index(dict_p dict, const char *key);
int dict_find(dict_p dict, const char *key);
void dict_add(dict_p dict, const char *key);
void dict_print(dict_p dict);
dict_p merge_dict(dict_p global_dict);
void print_csv(dict_p dict);