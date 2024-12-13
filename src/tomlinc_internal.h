#ifndef TOMLINC_INTERNAL_H
#define TOMLINC_INTERNAL_H

#include "tomlinc.h"
#include <stdio.h>
#include <stddef.h>

typedef struct TomlArray {
    void **values;
    TomlValueType *types;
    size_t *float_precisions;
    size_t count;
} TomlArray;

typedef struct TomlPair {
    char *key;
    void *value;
    TomlValueType type;
    struct TomlPair *next;
} TomlPair;

typedef struct TomlTable {
    char *name;
    TomlPair *pairs;
    struct TomlTable *subtables;
    struct TomlTable *next;

    struct TomlTable *array_of_tables;
    struct TomlTable *array_of_tables_last;

    int is_array_of_tables_element;
    int is_array_container; // Add this flag
} TomlTable;

// Private helper functions
char *trim_whitespace(char *str);
TomlPair *parse_pair(const char *line, FILE *file);
static TomlArray *parse_array(const char *line, FILE *file);
TomlTable *find_or_create_table(TomlTable **root, const char *name);
TomlTable *find_or_create_array_of_tables(TomlTable **root, const char *name);
void free_table(TomlTable *table);
void free_array(TomlArray *array);

// Used internally but also helpful for the public API implementation
TomlTable *find_table(TomlTable *root, const char *name);
TomlTable *find_table_recursive(TomlTable *root, const char *path);
void write_table_to_file(FILE *file, const TomlTable *table, int indent, const char *parent_name);

#endif // TOMLINC_INTERNAL_H
