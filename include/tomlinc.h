#ifndef TOMLINC_H
#define TOMLINC_H

#include <stdio.h>

typedef struct TomlTable TomlTable;
typedef struct TomlPair TomlPair;
typedef struct TomlArray TomlArray;

// API for users
TomlTable *tomlinc_open_file(const char *filename);
void tomlinc_close_file(TomlTable *table);
int tomlinc_save_file(const TomlTable *root, const char *filename);
void tomlinc_print_table(const TomlTable *table, int indent);

char *tomlinc_get_string_value(TomlTable *root_table, const char *table_path, const char *key);
int tomlinc_set_string_value(TomlTable *root_table, const char *table_path, const char *key, const char *new_value);
int tomlinc_get_int_value(TomlTable *root_table, const char *table_path, const char *key, int *result);
int tomlinc_set_int_value(TomlTable *root_table, const char *table_path, const char *key, int new_value);
int tomlinc_get_bool_value(TomlTable *root_table, const char *table_path, const char *key, int *result);
int tomlinc_set_bool_value(TomlTable *root_table, const char *table_path, const char *key, int new_value);
void *tomlinc_get_array_from_table(const TomlTable *root_table, const char *table_path, const char *key);
size_t tomlinc_array_size(void *array_handle);
int tomlinc_array_value_is_string(void *array_handle, size_t index);
int tomlinc_array_value_is_int(void *array_handle, size_t index);
int tomlinc_array_value_is_float(void *array_handle, size_t index);
int tomlinc_array_value_is_bool(void *array_handle, size_t index);
const char *tomlinc_array_get_string(void *array_handle, size_t index);
int tomlinc_array_get_int(void *array_handle, size_t index);
float tomlinc_array_get_float(void *array_handle, size_t index);
int tomlinc_array_get_bool(void *array_handle, size_t index);
int tomlinc_array_set_value(TomlTable *root_table, const char *table_path, const char *key, size_t index, void *new_value, const char *value_type);

#endif // TOMLINC_H