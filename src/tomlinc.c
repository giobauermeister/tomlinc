#include "tomlinc.h"
#include "tomlinc_internal.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

TomlTable *tomlinc_open_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    TomlTable *root = NULL;
    TomlTable *current_table = NULL;

    char line[256];

    while (fgets(line, sizeof(line), file)) {
        char *trimmed = trim_whitespace(line);

        // Skip empty lines and comments
        if (*trimmed == '\0' || *trimmed == '#') continue;

        if (*trimmed == '[') {
            // Check if it's [[...]]
            if (trimmed[1] == '[') {
                // array-of-tables
                char *end = strstr(trimmed, "]]");
                if (!end) continue; // malformed
                *end = '\0'; 
                char *table_name = trim_whitespace(trimmed + 2);
                current_table = find_or_create_array_of_tables(&root, table_name);
            } else {
                // single table
                char *end_bracket = strchr(trimmed, ']');
                if (!end_bracket) continue;
                *end_bracket = '\0';
                char *table_name = trim_whitespace(trimmed + 1);
                current_table = find_or_create_table(&root, table_name);
            }
        } else if (current_table) {
            // key-value pairs
            TomlPair *pair = parse_pair(trimmed, file);
            if (pair) {
                if (!current_table->pairs) {
                    current_table->pairs = pair;
                } else {
                    TomlPair *last_pair = current_table->pairs;
                    while (last_pair->next) last_pair = last_pair->next;
                    last_pair->next = pair;
                }
            }
        }
    }

    fclose(file);
    return root;
}

void tomlinc_close_file(TomlTable *table) {
    if (!table) return;

    TomlPair *pair = table->pairs;
    while (pair) {
        TomlPair *next = pair->next;
        if (pair->type == TOML_VALUE_ARRAY) {
            free_array((TomlArray *)pair->value);
        } else {
            free(pair->value);
        }
        free(pair->key);
        free(pair);
        pair = next;
    }

    TomlTable *subtable = table->subtables;
    while (subtable) {
        TomlTable *next = subtable->next;
        free_table(subtable);
        subtable = next;
    }

    // Free array-of-tables
    TomlTable *aot = table->array_of_tables;
    while (aot) {
        TomlTable *next = aot->next;
        free_table(aot);
        aot = next;
    }

    free(table->name);
    free(table);
}

int tomlinc_save_file(const TomlTable *root, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file for writing");
        return 0;
    }

    write_table_to_file(file, root, 0, NULL);

    fclose(file);
    return 1;
}

void tomlinc_print_table(const TomlTable *table, int indent) {
    static char current_path[1024] = ""; // Static buffer to hold the current path

    while (table) {
        char full_path[1024] = {0};

        // Append the current table name to the path
        if (current_path[0] != '\0') {
            if (snprintf(full_path, sizeof(full_path), "%s.%s", current_path, table->name) >= sizeof(full_path)) {
                fprintf(stderr, "Path too long, truncating: %s.%s\n", current_path, table->name);
                return;
            }
        } else {
            if (snprintf(full_path, sizeof(full_path), "%s", table->name) >= sizeof(full_path)) {
                fprintf(stderr, "Path too long, truncating: %s\n", table->name);
                return;
            }
        }

        // Print the table header
        for (int i = 0; i < indent; i++) printf("  ");
        if (table->is_array_of_tables_element) {
            printf("[[%s]]\n", full_path);
        } else {
            printf("[%s]\n", full_path);
        }

        // Print key-value pairs
        TomlPair *pair = table->pairs;
        while (pair) {
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("%s = ", pair->key);

            if (pair->type == TOML_VALUE_ARRAY) {
                TomlArray *array = (TomlArray *)pair->value;
                printf("[");
                for (size_t i = 0; i < array->count; i++) {
                    if (i > 0) printf(", ");
                    if (array->types[i] == TOML_VALUE_STRING) {
                        printf("\"%s\"", (char *)array->values[i]);
                    } else if (array->types[i] == TOML_VALUE_INT) {
                        printf("%d", *(int *)array->values[i]);
                    } else if (array->types[i] == TOML_VALUE_FLOAT) {
                        // Dynamically adjust float precision
                        int precision = array->float_precisions ? array->float_precisions[i] : 3; // Default to 3 if not set
                        printf("%.*f", precision, *(float *)array->values[i]);
                    } else if (array->types[i] == TOML_VALUE_BOOL) {
                        printf("%s", *(int *)array->values[i] ? "true" : "false");
                    }
                }
                printf("]\n");
            } else if (pair->type == TOML_VALUE_STRING) {
                printf("\"%s\"\n", (char *)pair->value);
            } else if (pair->type == TOML_VALUE_INT) {
                printf("%d\n", *(int *)pair->value);
            } else if (pair->type == TOML_VALUE_FLOAT) {
                // Dynamically adjust float precision for non-array floats
                printf("%.3f\n", *(float *)pair->value);
            } else if (pair->type == TOML_VALUE_BOOL) {
                printf("%s\n", *(int *)pair->value ? "true" : "false");
            }

            pair = pair->next;
        }


        // Update current path for nested tables
        char previous_path[1024] = {0};
        strncpy(previous_path, current_path, sizeof(previous_path) - 1);
        strncpy(current_path, full_path, sizeof(current_path) - 1);

        // Print subtables
        if (table->subtables) {
            tomlinc_print_table(table->subtables, indent + 1);
        }

        // Restore previous path for sibling tables
        strncpy(current_path, previous_path, sizeof(current_path) - 1);

        // Move to the next table
        table = table->next;
    }
}

char *tomlinc_get_string_value(TomlTable *root_table, const char *table_path, const char *key) {
    if (!root_table || !table_path || !key) return NULL;

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Search for the key in the found table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            return (char *)pair->value;
        }
        pair = pair->next;
    }

    return NULL; // Key not found
}

int tomlinc_set_string_value(TomlTable *root_table, const char *table_path, const char *key, const char *new_value) {
    if (!root_table || !table_path || !key || !new_value) {
        return 0;
    }

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Update the key-value pair in the located table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            // Free the old value and update with the new one
            free(pair->value);
            pair->value = strdup(new_value);
            return 1; // Successfully updated
        }
        pair = pair->next;
    }

    return 0; // Key not found
}

int tomlinc_get_int_value(TomlTable *root_table, const char *table_path, const char *key, int *result) {
    if (!root_table || !table_path || !key || !result) return 0;

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Update the key-value pair in the located table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_INT) {
            *result = *(int *)pair->value;
            return 1; // Successfully retrieved the integer value
        }
        pair = pair->next;
    }

    return 0; // Key not found or not an integer
}

int tomlinc_set_int_value(TomlTable *root_table, const char *table_path, const char *key, int new_value) {
    if (!root_table || !table_path || !key) return 0;

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Update the key-value pair in the located table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_INT) {
            // Update the integer value
            int *int_value = malloc(sizeof(int));
            if (!int_value) return 0; // Memory allocation failed

            *int_value = new_value;
            free(pair->value); // Free the old value
            pair->value = int_value;
            return 1; // Successfully updated
        }
        pair = pair->next;
    }

    return 0; // Key not found or not an integer
}

int tomlinc_get_bool_value(TomlTable *root_table, const char *table_path, const char *key, int *result) {
    if (!root_table || !table_path || !key || !result) return 0;

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Update the key-value pair in the located table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_BOOL) {
            *result = *(int *)pair->value;
            return 1; // Successfully retrieved the boolean value
        }
        pair = pair->next;
    }

    return 0; // Key not found or not a boolean
}

int tomlinc_set_bool_value(TomlTable *root_table, const char *table_path, const char *key, int new_value) {
    if (!root_table || !table_path || !key) return 0;

    // Tokenize the table path (e.g., "integration.mqtt")
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    TomlTable *current_table = root_table;

    // Traverse the hierarchy
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0;
    }

    // Update the key-value pair in the located table
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_BOOL) {
            // Update the boolean value
            int *bool_value = malloc(sizeof(int));
            if (!bool_value) return 0; // Memory allocation failed

            *bool_value = new_value;
            free(pair->value); // Free the old value
            pair->value = bool_value;
            return 1; // Successfully updated
        }
        pair = pair->next;
    }

    return 0; // Key not found or not a boolean
}

void *tomlinc_get_array_from_table(const TomlTable *root_table, const char *table_path, const char *key) {
    if (!root_table || !table_path || !key) return NULL;

    char *path_copy = strdup(table_path);
    if (!path_copy) return NULL;

    char *token = strtok(path_copy, ".");
    const TomlTable *current_table = root_table;

    while (token && current_table) {
        current_table = find_table_recursive((TomlTable *)current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) return NULL;

    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_ARRAY) {
            return (void *)pair->value;
        }
        pair = pair->next;
    }
    return NULL;
}

size_t tomlinc_array_size(void *array_handle) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    return array->count;
}

int tomlinc_array_value_is_string(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count) return 0;
    return array->types[index] == TOML_VALUE_STRING;
}

int tomlinc_array_value_is_int(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count) return 0;
    return array->types[index] == TOML_VALUE_INT;
}

int tomlinc_array_value_is_float(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count) return 0;
    return array->types[index] == TOML_VALUE_FLOAT;
}

int tomlinc_array_value_is_bool(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count) return 0;
    return array->types[index] == TOML_VALUE_BOOL;
}

const char *tomlinc_array_get_string(void *array_handle, size_t index) {
    if (!array_handle) return NULL;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count || array->types[index] != TOML_VALUE_STRING) return NULL;
    return (const char *)array->values[index];
}

int tomlinc_array_get_int(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count || array->types[index] != TOML_VALUE_INT) return 0;
    return *(int *)array->values[index];
}

float tomlinc_array_get_float(void *array_handle, size_t index, int *precision) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count || array->types[index] != TOML_VALUE_FLOAT) return 0;

    if (precision) {
        *precision = array->float_precisions[index];
    }

    return *(float *)array->values[index];
}

int tomlinc_array_get_bool(void *array_handle, size_t index) {
    if (!array_handle) return 0;
    TomlArray *array = (TomlArray *)array_handle;
    if (index >= array->count || array->types[index] != TOML_VALUE_BOOL) return 0;
    return *(int *)array->values[index];
}

int tomlinc_array_set_value(TomlTable *root_table, const char *table_path, const char *key, size_t index, void *new_value, TomlValueType value_type) {
    if (!root_table || !table_path || !key || !new_value) {
        return 0; // Invalid parameters
    }

    // Find the target table
    TomlTable *current_table = root_table;
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0; // Memory allocation failed
    }

    char *token = strtok(path_copy, ".");
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0; // Table not found
    }

    // Find the array
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_ARRAY) {
            TomlArray *array = (TomlArray *)pair->value;

            if (index >= array->count) {
                return 0; // Index out of bounds
            }

            // Free the old value
            if (array->types[index] == TOML_VALUE_STRING) {
                free(array->values[index]);
            } else if (array->types[index] == TOML_VALUE_ARRAY) {
                free_array((TomlArray *)array->values[index]);
            }

            // Update the value based on the provided value_type
            void *new_entry = NULL;
            switch (value_type) {
                case TOML_VALUE_STRING:
                    new_entry = strdup((char *)new_value);
                    break;
                case TOML_VALUE_INT:
                    new_entry = malloc(sizeof(int));
                    if (new_entry) *(int *)new_entry = *(int *)new_value;
                    break;
                case TOML_VALUE_FLOAT:
                    new_entry = malloc(sizeof(float));
                    if (new_entry) *(float *)new_entry = *(float *)new_value;
                    break;
                case TOML_VALUE_BOOL:
                    new_entry = malloc(sizeof(int)); // Booleans stored as integers
                    if (new_entry) *(int *)new_entry = *(int *)new_value;
                    break;
                default:
                    return 0; // Unsupported type
            }

            if (!new_entry) {
                return 0; // Memory allocation failed
            }

            // Update the array
            array->values[index] = new_entry;
            array->types[index] = value_type; // Update the type
            return 1; // Successfully updated
        }
        pair = pair->next;
    }

    return 0; // Key not found or not an array
}

int tomlinc_array_add_value(TomlTable *root_table, const char *table_path, const char *key, void *new_value, TomlValueType value_type) {
    if (!root_table || !table_path || !key || !new_value) {
        return 0; // Invalid parameters
    }

    // Find the target table
    TomlTable *current_table = root_table;
    char *path_copy = strdup(table_path);
    if (!path_copy) {
        return 0;
    }

    char *token = strtok(path_copy, ".");
    while (token && current_table) {
        current_table = find_table_recursive(current_table, token);
        token = strtok(NULL, ".");
    }

    free(path_copy);

    if (!current_table) {
        return 0; // Table not found
    }

    // Find the array
    TomlPair *pair = current_table->pairs;
    while (pair) {
        if (strcmp(pair->key, key) == 0 && pair->type == TOML_VALUE_ARRAY) {
            TomlArray *array = (TomlArray *)pair->value;

            // Extend the array
            void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
            TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
            if (!new_values || !new_types) {
                if (new_values) array->values = new_values;
                return 0; // Memory allocation failed
            }

            array->values = new_values;
            array->types = new_types;

            // Add the new value based on its type
            void *new_entry = NULL;
            switch (value_type) {
                case TOML_VALUE_STRING:
                    new_entry = strdup((char *)new_value);
                    break;
                case TOML_VALUE_INT:
                    new_entry = malloc(sizeof(int));
                    if (new_entry) {
                        *(int *)new_entry = *(int *)new_value;
                    }
                    break;
                case TOML_VALUE_FLOAT:
                    new_entry = malloc(sizeof(float));
                    if (new_entry) {
                        *(float *)new_entry = *(float *)new_value;
                    }
                    break;
                case TOML_VALUE_BOOL:
                    new_entry = malloc(sizeof(int));
                    if (new_entry) {
                        *(int *)new_entry = *(int *)new_value;
                    }
                    break;
                default:
                    return 0; // Unsupported type
            }

            if (!new_entry) {
                return 0; // Memory allocation failed
            }

            array->values[array->count] = new_entry;
            array->types[array->count] = value_type;
            array->count++; // Increment the count
            return 1; // Successfully added
        }
        pair = pair->next;
    }

    return 0; // Key not found or not an array
}