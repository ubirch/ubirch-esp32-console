/*!
 * @file    cmd_ubirch.c
 * @brief   Custom Console commands.
 *
 * @author Waldemar Gruenwald
 * @date   2018-11-14
 *
 * @copyright &copy; 2018 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */


#include <esp_log.h>
#include <esp_console.h>
#include <argtable3/argtable3.h>
#include <string.h>

//#include "key_handling.h"
#include "networking.h"

#include "cmd_ubirch.h"
#include "storage.h"
#include "key_handling.h"

/*
 * 'exit' command exits the console and runs the rest of the program
 */
static int exit_console(int argc, char **argv) {
    ESP_LOGI(__func__, "Exiting Console");
    fflush(stdin);
    return EXIT_CONSOLE;
}

void register_exit() {
    const esp_console_cmd_t cmd = {
            .command = "exit",
            .help = "Exit the console and resume with program",
            .hint = NULL,
            .func = &exit_console,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/*
 * 'run' command runs the rest of the program
 */
static int run_status(int argc, char **argv) {
    esp_err_t err;
    struct Wifi_login wifi = {0};

    // show the Hardware device ID
    unsigned char *hw_ID = NULL;
    size_t hw_ID_len = 0;

    printf("UBIRCH device status:\r\n");

    // show hardware device id
    kv_load("device-status", "hw-dev-id", (void **) &hw_ID, &hw_ID_len);
	printf("Hardware-Device-ID: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n",
	       hw_ID[0], hw_ID[1], hw_ID[2], hw_ID[3], hw_ID[4], hw_ID[5], hw_ID[6], hw_ID[7],
	       hw_ID[8], hw_ID[9], hw_ID[10], hw_ID[11], hw_ID[12], hw_ID[13], hw_ID[14], hw_ID[15]);
    free(hw_ID);

    // show the public key, if available
    unsigned char *key;
    size_t key_size = 0;
    err = kv_load("key_storage", "public_key", (void **) &key, &key_size);
    if (!memory_error_check(err)) {
        printf("Public key: ");
        for (int i = 0; i < key_size; ++i) {
	        printf("%02x", key[i]);
        }
        printf("\r\n");
        free(key);
    } else {
        printf("Public key not available.\r\n");
    }

    // show backend key
    char keybuffer[PUBLICKEY_BASE64_STRING_LENGTH + 1];
    if (get_backend_public_key(keybuffer, sizeof(keybuffer)) == ESP_OK) {
        printf("Backend public key: %s\n", keybuffer);
    } else {
        printf("Backend public key not available.\r\n");
    }

    // show the wifi login information, if available
    err = kv_load("wifi_data", "wifi_ssid", (void **) &wifi.ssid, &wifi.ssid_length);
    if (err == ESP_OK) {
        ESP_LOGD(__func__, "%s", wifi.ssid);
        kv_load("wifi_data", "wifi_pwd", (void **) &wifi.pwd, &wifi.pwd_length);
        ESP_LOGD(__func__, "%s", wifi.pwd);
        printf("Wifi SSID : %.*s\r\n", wifi.ssid_length, wifi.ssid);
        ESP_LOGD("Wifi PWD", "%.*s", wifi.pwd_length, wifi.pwd);
        free(wifi.ssid);
        free(wifi.pwd);
    } else {
        ESP_LOGE("status", "Wifi not configured yet!\r\ntype join to do so\r\n");
    }

    time_t now;
    struct tm timeinfo = {0};

    time(&now);
    localtime_r(&now, &timeinfo);
    printf("Current time: %02d.%02d.%04d %02d:%02d:%02d\r\n",
           timeinfo.tm_mday, timeinfo.tm_mon + 1, (1900 + timeinfo.tm_year),
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    return ESP_OK;
}

void register_status() {
    const esp_console_cmd_t cmd = {
            .command = "status",
            .help = "Get the current status of the system",
            .hint = NULL,
            .func = &run_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/*
 * 'update_backendkey' command to set backend verification key or load default value into flash
 */

/*
 * Arguments used by 'update_backendkey' function
 */
static struct {
    struct arg_str *backendkey;
    struct arg_end *end;
} update_backendkey_args;

static int update_backendkey(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &update_backendkey_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, update_backendkey_args.end, argv[0]);
        return 1;
    }

    if (update_backendkey_args.backendkey->count == 0 && set_backend_default_public_key() == ESP_OK) {
        return 0;
    } else if (update_backendkey_args.backendkey->count == 1
            && set_backend_public_key(update_backendkey_args.backendkey->sval[0]) == ESP_OK) {
        return 0;
    }
    return 1;
}

void register_update_backendkey() {
    update_backendkey_args.backendkey = arg_str0(NULL, NULL, "<backendkey>", "Backend public key in base64 format");
    update_backendkey_args.backendkey->sval[0] = ""; // default value
    update_backendkey_args.end = arg_end(2);

    const esp_console_cmd_t update_backendkey_cmd = {
            .command = "update_backendkey",
            .help = "Update backend key or reset to default key if <backendkey>-option is empty.",
            .hint = NULL,
            .func = &update_backendkey,
            .argtable = &update_backendkey_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&update_backendkey_cmd));
}

/*
 * 'join' command to connect with wifi network
 */

/*
 * Arguments used by 'join' function
 */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

static int connect(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    // create a struct for the wifi login and copy the input parameters into it
    char wifi_ssid[strlen(join_args.ssid->sval[0])];
    char wifi_pwd[strlen(join_args.password->sval[0])];
    struct Wifi_login wifi = {
            .ssid = &wifi_ssid[0],
            .pwd = &wifi_pwd[0],
            .ssid_length = strlen(join_args.ssid->sval[0]),
            .pwd_length = strlen(join_args.password->sval[0])
    };
    strncpy(wifi.ssid, join_args.ssid->sval[0], wifi.ssid_length);
    strncpy(wifi.pwd, join_args.password->sval[0], wifi.pwd_length);

    ESP_LOGI(__func__, "Connecting to '%.*s'", wifi.ssid_length, join_args.ssid->sval[0]);

    esp_err_t err = wifi_join(wifi, join_args.timeout->ival[0]);
    if (err != ESP_OK) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    // Store the wifi login data

    memory_error_check(kv_store("wifi_data", "wifi_ssid", wifi.ssid, wifi.ssid_length));
    memory_error_check(kv_store("wifi_data", "wifi_pwd", wifi.pwd, wifi.pwd_length));
    return 0;
}

void register_wifi() {
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.timeout->ival[0] = 20000; // set default value
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
            .command = "join",
            .help = "Join WiFi AP as a station",
            .hint = NULL,
            .func = &connect,
            .argtable = &join_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
}

