#include "tomlinc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Trim leading and trailing whitespace
char *trim_whitespace(char *str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

TomlPair *parse_pair(const char *line, FILE *file) {
    char *eq_pos = strchr(line, '=');
    if (!eq_pos) return NULL; // Not a valid pair

    *eq_pos = '\0'; 
    char *key = trim_whitespace((char *)line);
    char *value_str = trim_whitespace(eq_pos + 1);

    TomlPair *pair = malloc(sizeof(TomlPair));
    if (!pair) return NULL;

    pair->key = strdup(key);
    pair->next = NULL;

    if (*value_str == '[') {
        // Parse array
        pair->value = parse_array(value_str, file);
        pair->type = TOML_VALUE_ARRAY;
        if (!pair->value) {
            free(pair->key);
            free(pair);
            return NULL;
        }
    } else if ((*value_str == '"' && value_str[strlen(value_str) - 1] == '"') ||
               (*value_str == '\'' && value_str[strlen(value_str) - 1] == '\'')) {
        // String
        value_str[strlen(value_str) - 1] = '\0';
        value_str++;
        pair->value = strdup(value_str);
        pair->type = TOML_VALUE_STRING;
    } else if (strchr(value_str, '.')) {
        // Float
        float *fvalue = malloc(sizeof(float));
        *fvalue = atof(value_str);
        pair->value = fvalue;
        pair->type = TOML_VALUE_FLOAT;
    } else if (isdigit((unsigned char)*value_str) || (*value_str == '-' && isdigit((unsigned char)value_str[1]))) {
        // Integer
        int *ivalue = malloc(sizeof(int));
        *ivalue = atoi(value_str);
        pair->value = ivalue;
        pair->type = TOML_VALUE_INT;
    } else if (strcmp(value_str, "true") == 0 || strcmp(value_str, "false") == 0) {
        // Bool
        int *bvalue = malloc(sizeof(int));
        *bvalue = (strcmp(value_str, "true") == 0);
        pair->value = bvalue;
        pair->type = TOML_VALUE_BOOL;
    } else {
        // Unsupported
        free(pair->key);
        free(pair);
        return NULL;
    }

    return pair;
}

static TomlArray *parse_array(const char *line, FILE *file) {
    TomlArray *array = malloc(sizeof(TomlArray));
    if (!array) {
        return NULL;
    }

    array->values = NULL;
    array->types = NULL;
    array->float_precisions = NULL;
    array->count = 0;

    size_t buffer_size = strlen(line) + 1;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        free(array);
        return NULL;
    }
    strcpy(buffer, line);

    // Balance brackets
    size_t open_brackets = 0, close_brackets = 0;
    for (size_t i = 0; i < buffer_size; i++) {
        if (buffer[i] == '[') open_brackets++;
        if (buffer[i] == ']') close_brackets++;
    }

    while (open_brackets > close_brackets) {
        char temp[256];
        if (!fgets(temp, sizeof(temp), file)) {
            free(buffer);
            free(array);
            return NULL;
        }

        temp[strcspn(temp, "\n")] = '\0'; // Remove newline
        char *new_buffer = realloc(buffer, buffer_size + strlen(temp) + 1);
        if (!new_buffer) {
            free(buffer);
            free(array);
            return NULL;
        }
        buffer = new_buffer;
        strcat(buffer, temp);
        buffer_size += strlen(temp);

        for (size_t i = 0; i < strlen(temp); i++) {
            if (temp[i] == '[') open_brackets++;
            if (temp[i] == ']') close_brackets++;
        }
    }

    // Strip outer brackets
    if (strlen(buffer) < 2) {
        free(buffer);
        free(array);
        return NULL;
    }

    buffer[strlen(buffer) - 1] = '\0'; // remove trailing ']'
    char *content = buffer + 1; // skip leading '['

    while (content && *content) {
        content = trim_whitespace(content);
        if (!content || !*content) {
            break;
        }

        if (*content == '[') {
            // Nested array
            const char *start = content;
            size_t nested_balance = 1, nested_length = 1;
            while (nested_balance > 0) {
                char c = content[nested_length++];
                if (c == '[') nested_balance++;
                if (c == ']') nested_balance--;
                if (c == '\0') {
                    free(buffer);
                    free_array(array);
                    return NULL;
                }
            }

            char *nested_content = strndup(start, nested_length);
            if (!nested_content) {
                free(buffer);
                free_array(array);
                return NULL;
            }

            TomlArray *nested_array = parse_array(nested_content, file);
            free(nested_content);

            if (!nested_array) {
                free(buffer);
                free_array(array);
                return NULL;
            }

            void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
            TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
            if (!new_values || !new_types) {
                if (new_values) array->values = new_values;
                free_array(nested_array);
                free(buffer);
                free_array(array);
                return NULL;
            }

            array->values = new_values;
            array->types = new_types;

            array->values[array->count] = nested_array;
            array->types[array->count] = TOML_VALUE_ARRAY;
            array->count++;

            content += nested_length;
        } else if ((*content == '"' && strchr(content + 1, '"'))) {
            // String
            char quote_char = *content;
            char *end_quote = strchr(content + 1, quote_char);
            if (!end_quote) {
                free(buffer);
                free_array(array);
                return NULL;
            }

            size_t length = end_quote - content - 1;
            char *value = strndup(content + 1, length);
            if (!value) {
                free(buffer);
                free_array(array);
                return NULL;
            }

            void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
            TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
            if (!new_values || !new_types) {
                free(value);
                if (new_values) array->values = new_values;
                free(buffer);
                free_array(array);
                return NULL;
            }

            array->values = new_values;
            array->types = new_types;

            array->values[array->count] = value;
            array->types[array->count] = TOML_VALUE_STRING;
            array->count++;

            content = end_quote + 1;
        } else if ((*content == 't' && strncmp(content, "true", 4) == 0) || 
                   (*content == 'f' && strncmp(content, "false", 5) == 0)) {
            int *value = malloc(sizeof(int));
            if (!value) {
                free(buffer);
                free_array(array);
                return NULL;
            }
            *value = (content[0] == 't') ? 1 : 0;

            void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
            TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
            if (!new_values || !new_types) {
                free(value);
                if (new_values) array->values = new_values;
                free(buffer);
                free_array(array);
                return NULL;
            }

            array->values = new_values;
            array->types = new_types;

            array->values[array->count] = value;
            array->types[array->count] = TOML_VALUE_BOOL;
            array->count++;

            content += (content[0] == 't') ? 4 : 5; // Move past "true" or "false"
        } else if (isdigit((unsigned char)*content) || (*content == '-' && isdigit((unsigned char)content[1]))) {
            // Find the end of this token (comma or end of line)
            char *comma = strchr(content, ',');
            size_t token_length = comma ? (size_t)(comma - content) : strlen(content);

            // Extract the first token
            char *token_str = strndup(content, token_length);
            if (!token_str) {
                free(buffer);
                free_array(array);
                return NULL;
            }

            // Determine if it's float or int by checking only token_str
            if (strchr(token_str, '.')) {
                // Parse as float
                float *value = malloc(sizeof(float));
                if (!value) {
                    free(token_str);
                    free(buffer);
                    free_array(array);
                    return NULL;
                }
                *value = strtof(token_str, NULL);

                // Calculate the precision
                size_t precision = 0;
                char *dot = strchr(token_str, '.');
                if (dot) {
                    precision = strlen(dot + 1); // Count characters after the dot
                }

                // Reallocate precision array
                size_t *new_precisions = realloc(array->float_precisions, sizeof(size_t) * (array->count + 1));
                void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
                TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
                if (!new_values || !new_types || !new_precisions) {
                    free(value);
                    if (new_values) array->values = new_values;
                    if (new_precisions) array->float_precisions = new_precisions;
                    free(token_str);
                    free(buffer);
                    free_array(array);
                    return NULL;
                }

                array->values = new_values;
                array->types = new_types;
                array->float_precisions = new_precisions;

                array->values[array->count] = value;
                array->types[array->count] = TOML_VALUE_FLOAT;
                array->float_precisions[array->count] = precision; // Store precision
                array->count++;
            } else {
                // Parse as integer
                int *value = malloc(sizeof(int));
                if (!value) {
                    free(token_str);
                    free(buffer);
                    free_array(array);
                    return NULL;
                }
                *value = atoi(token_str);

                void **new_values = realloc(array->values, sizeof(void *) * (array->count + 1));
                TomlValueType *new_types = realloc(array->types, sizeof(TomlValueType) * (array->count + 1));
                if (!new_values || !new_types) {
                    free(value);
                    if (new_values) array->values = new_values;
                    free(token_str);
                    free(buffer);
                    free_array(array);
                    return NULL;
                }

                array->values = new_values;
                array->types = new_types;

                array->values[array->count] = value;
                array->types[array->count] = TOML_VALUE_INT;
                array->count++;
            }

            free(token_str);

            // Move content past the comma if present
            if (comma) {
                content = trim_whitespace(comma + 1);
            } else {
                content = NULL;
            }
        } else if (*content == ',') {
            // Skip commas
            content++;
        } else {
            break;
        }
    }

    free(buffer);
    return array;
}

TomlTable *find_or_create_table(TomlTable **root, const char *name) {
    if (!name || !*name) return NULL;

    char *name_copy = strdup(name);
    if (!name_copy) return NULL;
    char *token = strtok(name_copy, ".");
    TomlTable **current = root;
    TomlTable *last_table = NULL;

    while (token) {
        TomlTable *table = *current;

        // Find existing non-array-element table
        while (table) {
            if (strcmp(table->name, token) == 0 && !table->is_array_of_tables_element)
                break;
            table = table->next;
        }

        if (!table) {
            // Create a normal table
            table = calloc(1, sizeof(TomlTable));
            if (!table) {
                free(name_copy);
                return NULL;
            }
            table->name = strdup(token);

            // Append
            if (!*current) {
                *current = table;
            } else {
                TomlTable *last = *current;
                while (last->next) last = last->next;
                last->next = table;
            }
        }

        last_table = table;
        token = strtok(NULL, ".");

        // If this table is an array container and we have more tokens,
        // navigate into the last array element's subtables, NOT table->subtables
        if (table->is_array_container && table->array_of_tables_last && token) {
            current = &table->array_of_tables_last->subtables;
        } else {
            current = &table->subtables;
        }
    }

    free(name_copy);
    return last_table;
}

TomlTable *find_or_create_array_of_tables(TomlTable **root, const char *name) {
    if (!name || !*name) return NULL;

    char *name_copy = strdup(name);
    if (!name_copy) return NULL;

    char *token = strtok(name_copy, ".");
    TomlTable **current = root;
    TomlTable *last_table = NULL;

    // Create/find intermediate tables for all tokens except the last
    while (1) {
        char *next_token = strtok(NULL, ".");
        TomlTable *table = *current;

        // Find existing non-array-element table
        while (table) {
            if (strcmp(table->name, token) == 0 && !table->is_array_of_tables_element) break;
            table = table->next;
        }

        if (!table) {
            // Create an intermediate normal table
            table = calloc(1, sizeof(TomlTable));
            if (!table) {
                free(name_copy);
                return NULL;
            }
            table->name = strdup(token);
            // other fields are NULL and zero-initialized by calloc
            // is_array_of_tables_element = 0, is_array_container = 0 by default

            // Append
            if (!*current) {
                *current = table;
            } else {
                TomlTable *last = *current;
                while (last->next) last = last->next;
                last->next = table;
            }
        }

        last_table = table;

        if (!next_token) {
            // Final token: turn last_table into a container
            last_table->is_array_container = 1;

            // Create the array-of-tables element using the final token
            TomlTable *new_element = calloc(1, sizeof(TomlTable));
            if (!new_element) {
                free(name_copy);
                return NULL;
            }
            new_element->name = strdup(token);
            new_element->is_array_of_tables_element = 1;

            // Add to array_of_tables
            if (!last_table->array_of_tables) {
                last_table->array_of_tables = new_element;
                last_table->array_of_tables_last = new_element;
            } else {
                last_table->array_of_tables_last->next = new_element;
                last_table->array_of_tables_last = new_element;
            }

            free(name_copy);
            return new_element;
        }

        // Not final token, go deeper
        current = &table->subtables;
        token = next_token;
    }
}

void free_table(TomlTable *table) {
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

void free_array(TomlArray *array) {
    if (!array) return;
    for (size_t i = 0; i < array->count; i++) {
        if (array->types[i] == TOML_VALUE_ARRAY) {
            free_array((TomlArray *)array->values[i]);
        } else {
            free(array->values[i]);
        }
    }
    free(array->values);
    free(array->types);
    free(array->float_precisions);
    free(array);
}

TomlTable *find_table(TomlTable *root, const char *name) {
    while (root) {
        if (strcmp(root->name, name) == 0) {
            return root;
        }
        root = root->next;
    }
    return NULL;
}

TomlTable *find_table_recursive(TomlTable *root, const char *name) {
    while (root) {
        if (strcmp(root->name, name) == 0) {
            return root;
        }

        // Search in subtables if not found at the current level
        TomlTable *found_in_subtable = find_table_recursive(root->subtables, name);
        if (found_in_subtable) {
            return found_in_subtable;
        }

        root = root->next;
    }
    return NULL;
}

void write_table_to_file(FILE *file, const TomlTable *table, int indent, const char *parent_name) {
    if (!file || !table) return; // Ensure valid pointers

    while (table) {
        char full_name[512] = {0};

        // Build the full table name
        if (parent_name && *parent_name) {
            if (snprintf(full_name, sizeof(full_name), "%s.%s", parent_name, table->name) >= sizeof(full_name)) {
                fprintf(stderr, "DEBUG: Full name truncated, potential overflow.\n");
                return;
            }
        } else {
            strncpy(full_name, table->name, sizeof(full_name) - 1);
        }

        // Print table header
        for (int i = 0; i < indent; i++) fprintf(file, "  ");
        fprintf(file, "[%s]\n", full_name);

        // Print key-value pairs
        TomlPair *pair = table->pairs;
        while (pair) {
            if (!pair->key) {
                fprintf(stderr, "DEBUG: Encountered a pair with a NULL key.\n");
                break;
            }

            for (int i = 0; i < indent + 1; i++) fprintf(file, "  ");
            fprintf(file, "%s = ", pair->key);

            switch (pair->type) {
                case TOML_VALUE_STRING:
                    fprintf(file, "\"%s\"\n", (char *)pair->value);
                    break;
                case TOML_VALUE_INT:
                    fprintf(file, "%d\n", *(int *)pair->value);
                    break;
                case TOML_VALUE_FLOAT:
                    fprintf(file, "%.3f\n", *(float *)pair->value);
                    break;
                case TOML_VALUE_BOOL:
                    fprintf(file, "%s\n", *(int *)pair->value ? "true" : "false");
                    break;
                case TOML_VALUE_ARRAY: {
                    TomlArray *array = (TomlArray *)pair->value;
                    fprintf(file, "[");
                    for (size_t i = 0; i < array->count; i++) {
                        if (i > 0) fprintf(file, ", ");
                        if (array->types[i] == TOML_VALUE_STRING) {
                            fprintf(file, "\"%s\"", (char *)array->values[i]);
                        } else if (array->types[i] == TOML_VALUE_INT) {
                            fprintf(file, "%d", *(int *)array->values[i]);
                        } else if (array->types[i] == TOML_VALUE_FLOAT) {
                            fprintf(file, "%.3f", *(float *)array->values[i]);
                        } else if (array->types[i] == TOML_VALUE_BOOL) {
                            fprintf(file, "%s", *(int *)array->values[i] ? "true" : "false");
                        }
                    }
                    fprintf(file, "]\n");
                    break;
                }
                default:
                    fprintf(stderr, "DEBUG: Unknown pair type: %d\n", pair->type);
            }
            pair = pair->next;
        }

        // Print subtables
        if (table->subtables) {
            write_table_to_file(file, table->subtables, indent + 1, full_name);
        }

        // Print array-of-tables
        if (table->array_of_tables) {
            write_table_to_file(file, table->array_of_tables, indent + 1, full_name);
        }

        table = table->next;
    }
}