# TOMLinC

A C library for reading and writing values in TOML files.

## Minimal example

```
#include "tomlinc.h"
#include <stdint.h>

int main(int argc, char *argv[]) {
    TomlTable *toml_file = tomlinc_open_file("my_file.toml");
    if (!toml_file) {
        fprintf(stderr, "Failed to open TOML file: %s\n", argv[1]);
        return 1;
    }

    tomlinc_print_table(toml_file, 0);

    /*
        [general]
        log_level=4
    */
    int log_level;
    if (tomlinc_get_int_value(toml_file, "general", "log_level", &log_level)) {
        printf("[general] get log_level: %d\n", log_level);
    } else {
        printf("[general] Failed to get log_level.\n");
    }
    log_level = 3;
    if (tomlinc_set_int_value(toml_file, "general", "log_level", log_level)) {
        printf("[general] set log_level: %d\n", log_level);
    } else {
        printf("[general] Failed to set log_level value.\n");
    }

    if (tomlinc_save_file(toml_file, "output.toml")) {
        printf("TOML file updated successfully\n");
    }

    tomlinc_close_file(toml_file);

    return 0;
}
```

You can also get values from a subtable using the table path for example:

```
  [integration.mqtt]
    qos=3
```

```
int qos;
if (tomlinc_get_int_value(toml_file, "integration.mqtt", "qos", &qos)) {
    printf("[integration.mqtt] get qos: %d\n", qos);
} else {
    printf("[integration.mqtt] Failed to get qos\n");
}
qos = 1;
if (tomlinc_set_int_value(toml_file, "integration.mqtt", "qos", qos)) {
    printf("[integration.mqtt] set qos: %d\n", qos);
} else {
    printf("[integration.mqtt] Failed to set qos\n");
}
```

## Library API

### TOML file operations

- Open and close a TOML file
```
TomlTable *tomlinc_open_file(const char *filename);
void tomlinc_close_file(TomlTable *table);
```

- Save TOML table to a file
```
int tomlinc_save_file(const TomlTable *root, const char *filename);
```

- Print the full TOML file
```
void tomlinc_print_table(const TomlTable *table, int indent);
```

### TOML value operations

- Getters and setters
```
char *tomlinc_get_string_value(TomlTable *root_table, const char *table_path, const char *key);
int tomlinc_set_string_value(TomlTable *root_table, const char *table_path, const char *key, const char *new_value);
int tomlinc_get_int_value(TomlTable *root_table, const char *table_path, const char *key, int *result);
int tomlinc_set_int_value(TomlTable *root_table, const char *table_path, const char *key, int new_value);
int tomlinc_get_bool_value(TomlTable *root_table, const char *table_path, const char *key, int *result);
int tomlinc_set_bool_value(TomlTable *root_table, const char *table_path, const char *key, int new_value);
```

- Arrays getters and setters
```
void *tomlinc_get_array_from_table(const TomlTable *root_table, const char *table_path, const char *key);
size_t tomlinc_get_array_size(void *array_handle);
int tomlinc_array_value_is_string(void *array_handle, size_t index);
int tomlinc_array_value_is_int(void *array_handle, size_t index);
int tomlinc_array_value_is_float(void *array_handle, size_t index);
int tomlinc_array_value_is_bool(void *array_handle, size_t index);
const char *tomlinc_array_get_string(void *array_handle, size_t index);
int tomlinc_array_get_int(void *array_handle, size_t index);
float tomlinc_array_get_float(void *array_handle, size_t index, int *precision);
int tomlinc_array_get_bool(void *array_handle, size_t index);
int tomlinc_array_set_value(TomlTable *root_table, const char *table_path, const char *key, size_t index, void *new_value, TomlValueType value_type);
int tomlinc_array_add_value(TomlTable *root_table, const char *table_path, const char *key, void *new_value, TomlValueType value_type);
```

## Compiling and running example

At the root of project run the following

```
rm -rf build && cmake -B build -S . && cmake --build build
```

Run example

```
./build/bin/parse_toml_file example/example.toml example/output.toml
```