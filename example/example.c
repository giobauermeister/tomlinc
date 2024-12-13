#include "tomlinc.h"
#include "stdbool.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Input TOML file path> <Output TOML file path> \n", argv[0]);
        return 1;
    }

    TomlTable *toml_file = tomlinc_open_file(argv[1]);
    if (!toml_file) {
        fprintf(stderr, "Failed to open TOML file: %s\n", argv[1]);
        return 1;
    }

    tomlinc_print_table(toml_file, 0);

    printf("\n");

    /*
        [general]
        log_level=4
        log_json=false
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

    int log_json;
    if (tomlinc_get_bool_value(toml_file, "general", "log_json", &log_json)) {
        printf("[general] get log_json: %s\n", log_json ? "true" : "false");
    } else {
        printf("[general] Failed to get  log_json.\n");
    }
    log_json = true;
    if (tomlinc_set_bool_value(toml_file, "general", "log_json", log_json)) {
        printf("[general] set log_json: %s\n", log_json ? "true" : "false");
    } else {
        printf("[general] Failed to set log_json value.\n");
    }

    /*
        [logging]
        level="debug"
        json=false
    */
    const char *get_logging_level = tomlinc_get_string_value(toml_file, "logging", "level");
    if (get_logging_level) {
        printf("[logging] get level: %s\n", get_logging_level);
    } else {
        printf("[logging] Failed to get level\n");
    }
    const char *set_logging_level = "trace";
    if (tomlinc_set_string_value(toml_file, "logging", "level", set_logging_level)) {
        printf("[logging] set level: %s\n", set_logging_level);
    } else {
        printf("[logging] Failed to set level\n");
    }

    int json;
    if (tomlinc_get_bool_value(toml_file, "logging", "json", &json)) {
        printf("[logging] get json: %s\n", json ? "true" : "false");
    } else {
        printf("[logging] Failed to get json.\n");
    }
    json = true;
    if (tomlinc_set_bool_value(toml_file, "logging", "json", json)) {
        printf("[logging] set json: %s\n", json ? "true" : "false");
    } else {
        printf("[logging] Failed to set json value.\n");
    }

    /*
        [sqlite]
        path="sqlite:///mnt/chirpstack/db/chirpstack.sqlite"
        max_open_connections=4
    */
    const char *get_sqlite_path = tomlinc_get_string_value(toml_file, "sqlite", "path");
    if (get_sqlite_path) {
        printf("[sqlite] get path: %s\n", get_sqlite_path);
    } else {
        printf("[sqlite] Failed to get path\n");
    }
    const char *set_sqlite_path = "sqlite:///home/root/mydb.sqlite";
    if (tomlinc_set_string_value(toml_file, "sqlite", "path", set_sqlite_path)) {
        printf("[sqlite] set path: %s\n", set_sqlite_path);
    } else {
        printf("[sqlite] Failed to set path\n");
    }

    int max_open_connections;
    if (tomlinc_get_int_value(toml_file, "sqlite", "max_open_connections", &max_open_connections)) {
        printf("[sqlite] get max_open_connections: %d\n", max_open_connections);
    } else {
        printf("[sqlite] Failed to get max_open_connections.\n");
    }
    max_open_connections = 10;
    if (tomlinc_set_int_value(toml_file, "sqlite", "max_open_connections", max_open_connections)) {
        printf("[sqlite] set max_open_connections: %d\n", max_open_connections);
    } else {
        printf("[sqlite] Failed to set max_open_connections value.\n");
    }

    /*
        [integration]
          enabled = [
            "mqtt",
          ]
    */

    // Retrieve the "enabled" array from the "integration" table
    void *enabled_array = tomlinc_get_array_from_table(toml_file, "integration.settings", "mixed");
    if (enabled_array) {
        printf("[integration] enabled: [");
        for (size_t i = 0; i < tomlinc_array_size(enabled_array); i++) {
            if (i > 0) printf(", ");
            if (tomlinc_array_value_is_string(enabled_array, i)) {
                printf("\"%s\"", tomlinc_array_get_string(enabled_array, i));
            } else if (tomlinc_array_value_is_int(enabled_array, i)) {
                printf("%d", tomlinc_array_get_int(enabled_array, i));
            } else if (tomlinc_array_value_is_float(enabled_array, i)) {
                printf("%.3f", tomlinc_array_get_float(enabled_array, i));
            } else if (tomlinc_array_value_is_bool(enabled_array, i)) {
                printf("%s", tomlinc_array_get_bool(enabled_array, i) ? "true" : "false");
            }
        }
        printf("]\n");
    } else {
        printf("[integration] enabled not found\n");
    }

    const char *new_string = "postgres";
    if (tomlinc_array_set_value(toml_file, "integration", "enabled", 0, (void *)new_string, "string")) {
        printf("[integration] enabled: %s\n", new_string);
    } else {
        printf("[integration] Failed to update enabled\n");
    }

    /*
      [integration.mqtt]
        server="tcp://localhost:1883/"
        qos=0
    */
    const char *get_mqtt_server = tomlinc_get_string_value(toml_file, "integration.mqtt", "server");
    if (get_mqtt_server) {
        printf("[integration.mqtt] get mqtt server: %s\n", get_mqtt_server);
    } else {
        printf("[integration.mqtt] Failed to get mqtt server\n");
    }
    const char *set_mqtt_server = "tcp://tomlinc.server:1883/";
    if (tomlinc_set_string_value(toml_file, "integration.mqtt", "server", set_mqtt_server)) {
        printf("[integration.mqtt] set mqtt server: %s\n", set_mqtt_server);
    } else {
        printf("[integration.mqtt] Failed to set mqtt server\n");
    }

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

    /*
      [integration.mqtt.client]
        ca_cert="my_cert_file.ca"
        enabled=true
    */
    const char *get_mqtt_client_ca_cert = tomlinc_get_string_value(toml_file, "integration.mqtt.client", "ca_cert");
    if (get_mqtt_client_ca_cert) {
        printf("[integration.mqtt.client] get mqtt ca_cert: %s\n", get_mqtt_client_ca_cert);
    } else {
        printf("[integration.mqtt.client] Failed to get mqtt ca_cert\n");
    }
    const char *set_mqtt_client_ca_cert = "another_cert_file.ca";
    if (tomlinc_set_string_value(toml_file, "integration.mqtt.client", "ca_cert", set_mqtt_client_ca_cert)) {
        printf("[integration.mqtt.client] set mqtt ca_cert: %s\n", set_mqtt_client_ca_cert);
    } else {
        printf("[integration.mqtt.client] Failed to set mqtt ca_cert\n");
    }

    int enabled;
    if (tomlinc_get_bool_value(toml_file, "integration.mqtt.client", "enabled", &enabled)) {
        printf("[integration.mqtt.client] get enabled: %s\n", enabled ? "true" : "false");
    } else {
        printf("[integration.mqtt.client] Failed to get enabled.\n");
    }
    enabled = false;
    if (tomlinc_set_bool_value(toml_file, "integration.mqtt.client", "enabled", enabled)) {
        printf("[integration.mqtt.client] set enabled: %s\n", enabled ? "true" : "false");
    } else {
        printf("[integration.mqtt.client] Failed to set enabled value.\n");
    }

    /*
      [backend]
        type="basic_station"

          [backend.basic_station]
            frequency=915000000
    */
    const char *get_backend_type = tomlinc_get_string_value(toml_file, "backend", "type");
    if (get_backend_type) {
        printf("[backend] get type: %s\n", get_backend_type);
    } else {
        printf("[backend] Failed to get type\n");
    }
    const char *set_backend_type = "concentratord";
    if (tomlinc_set_string_value(toml_file, "backend", "type", set_backend_type)) {
        printf("[backend] set type: %s\n", set_backend_type);
    } else {
        printf("[backend] Failed to set type\n");
    }

    int frequency;
    if (tomlinc_get_int_value(toml_file, "backend.basic_station", "frequency", &frequency)) {
        printf("[backend.basic_station] get frequency: %d\n", frequency);
    } else {
        printf("[backend.basic_station] Failed to get frequency\n");
    }
    frequency = 928000000;
    if (tomlinc_set_int_value(toml_file, "backend.basic_station", "frequency", frequency)) {
        printf("[backend.basic_station] set frequency: %d\n", frequency);
    } else {
        printf("[backend.basic_station] Failed to set frequency\n");
    }

    printf("\n");

    // Save updated TOML to file
    if (tomlinc_save_file(toml_file, argv[2])) {
        printf("TOML file updated successfully\n");
    }

    tomlinc_close_file(toml_file);

    return 0;
}