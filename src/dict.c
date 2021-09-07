#include "../headers/dict.h"

dict_p dict_create(){
    dict_v supp_dict = {0, 10, malloc(10 * sizeof(dict_entry))};
    dict_p dict = malloc(sizeof(dict_v));
    *dict = supp_dict;
    return dict;
}

//Allocate extra memory for the dict based on his size
// - dict_p dict: dictonary that needs extra memory
void dict_increase_size(dict_p dict){
    dict->entry = realloc(dict->entry, dict->size * sizeof(dict_entry));
}

//Find the index of key in dict, if not present returns -1, otherwise retuns the entry number in dict
int dict_find_index(dict_p dict, const char *key){
    for(int i = 0; i < dict->num_entries; i++){
        if(!strcmp(dict->entry[i].name, key))
            return i;
    }
    return -1;
}

//Find the occurrencies of key in dict, if not present returns -1
int dict_find(dict_p dict, const char *key){
    int index = dict_find_index(dict, key);
    return index != -1 ? index : dict->entry[index].occurrences; 
}

//Add a key in dict, if already present increase the it's counter, otherwise create a new entry with occurrences to 1
void dict_add(dict_p dict, const char *key){
    int index = dict_find_index(dict, key);
    if(index != -1){
        dict->entry[index].occurrences++;
        return;
    }
    //check if the max size is reached, if true allocate extra memory
    if(dict->num_entries == dict->size){
        dict->size += 10;
        dict->entry = realloc(dict->entry, dict->size * sizeof(dict_entry));
    }
    strncpy(dict->entry[dict->num_entries].name, key, STRLEN);
    dict->entry[dict->num_entries].occurrences = 1;
    dict->num_entries++;
}

//print dict components
void dict_print(dict_p dict){
    for(int i = 0; i < dict->num_entries; i++){
        printf("\t%s - %d\n", dict->entry[i].name, dict->entry[i].occurrences);    
    }
}

int dict_entry_occurrences_comparator(const void *v1, const void *v2){
    const dict_entry *entries1 = (dict_entry *) v1;
    const dict_entry *entries2 = (dict_entry *) v2;

    if(entries1->occurrences > entries2->occurrences)
        return +1;
    else return -1;
}

int dict_entry_name_comparator(const void *v1, const void *v2){
    const dict_entry *entries1 = (dict_entry *) v1;
    const dict_entry *entries2 = (dict_entry *) v2;

    int result = strcmp(entries1->name, entries2->name);
    if(result > 0)
        return +1;
    else if(result < 0)
        return -1;
    else return 0;
}

void merge(dict_entry *entries, int x, int y, int z){
    int size = z - x + 1;
    dict_entry *data = malloc(size * sizeof(dict_entry));
    int merge_pos = 0;
    int left = x, right = y + 1;

    while(left <= y && right <= z){
        //if(entries[left].occurrences < entries[right].occurrences)
        if(strcmp(entries[left].name, entries[right].name) < 0)
            data[merge_pos++] = entries[left++];
        else
            data[merge_pos++] = entries[right++];
    }
    while(left <= y)
        data[merge_pos++] = entries[left++];
    while(right <= z)
        data[merge_pos++] = entries[right++];

    for(int i = 0; i < size; i++)
        entries[x + i] = data[i];
    

    free(data);
}

void mergeSort(dict_entry *entries, int x, int z){
    if(x < z){
        int y = (x + z) / 2;

        mergeSort(entries, x, y);
        mergeSort(entries, y + 1, z);

        merge(entries, x, y, z);
    }
}

//sort and merge entries of the global dictionary. The sort is made on name and occurrences of the entries
// - dict_p global_dict: contains all dicts of the processes
// - int size: number of dictionaries gave in input
dict_p merge_dict(dict_p global_dict){
    dict_p result = dict_create();

    //sort
    qsort(global_dict->entry, global_dict->num_entries, sizeof(dict_entry), dict_entry_name_comparator);
    //mergeSort(global_dict->entry, 0, global_dict->num_entries - 1);

    //merge
    char name_entry[50] = "";
    int j = -1;
    for(int i = 0; i < global_dict->num_entries; i++){
        if(strcmp(name_entry, global_dict->entry[i].name) != 0){
            j++;
            dict_add(result, global_dict->entry[i].name);
            result->entry[j].occurrences = global_dict->entry[i].occurrences;
            strncpy(name_entry, global_dict->entry[i].name, 50);
        }
        else {
            result->entry[j].occurrences += global_dict->entry[i].occurrences;
        }
    }
    
    free(global_dict->entry);
    free(global_dict);
    return result;
    //return global_dict;
}

/* Prints a formatted csv file showing the result obtained by word counter.
* - dict_p dict : the dictionary to be printed
* -> returns a csv file containg the data present in the dictionary.  
*/
void print_csv(dict_p dict){
    FILE *file;
    char path[] = "../results/result.csv";
    
    file = fopen(path, "w");
    fprintf(file, "Name, Occurrences\n");
    for(int i = 0; i < dict->num_entries; i++){
        fprintf(file, "%s, %d\n", dict->entry[i].name, dict->entry[i].occurrences);
    }
    fclose(file);
    printf("\nFile %s created\n", "result.csv");
}