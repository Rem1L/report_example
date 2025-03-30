#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep
#include <fcntl.h>     // For open, O_RDWR
#include <termios.h>   // For terminal control
#include <errno.h>     // For errno
#include "util_ril.h"
#include <iostream>
#include <cstring>
#include <cstdlib> // for rand, srand
#include <ctime>   // for time
#include <string> // Added to use std::string for accumulating responses
#include <fstream>
#include <chrono>
#include <unordered_set>

bool is_bad_system_state(const char* state) {
    const char* bad_states[] = {
        "OUT_OF_SERVICE",
        "mChannelNumber=-1",
        "Unknown",
        "NOT_REG_OR_SEARCHING",
        "UNKNOWN",
        "availableServices=[]"
    };

    printf("[is_bad_system_state] Bad state: %s\n", state);

    for (const char* bad_state : bad_states) {
        if (strstr(state, bad_state) != NULL) {
            fprintf(stderr, "Detected bad state: %s\n", bad_state);
            return true;
        }
    }
    return false;
}

char* get_telephony_registry_state() {
    // Use adb shell to execute dumpsys command on the connected Android device
    char* state = execute_command("adb shell dumpsys telephony.registry | grep mServiceState | sed -n 2p | sed 's/MobileVoice=OUT_OF_SERVICE, //'");
    printf("[get_telephony_registry_state] state:%s\n", state);
    if (state == NULL) {
        fprintf(stderr, "Error: get_telephony_registry_state failed to execute adb command.\n");
        return NULL;
    }

    // Check if adb command was successful (not containing "error" or "device not found")
    if (strstr(state, "error") != NULL || strstr(state, "device not found") != NULL) {
        fprintf(stderr, "Error: No Android device connected or adb command failed.\n");
        free(state);
        return NULL;
    }

    return state;
}

bool write_hex_data_to_file(const uint8_t* data, size_t size) {
    // Write to temporary file on host first
    const char* host_tmp_path = "hex_data.txt";
    FILE* hex_file = fopen(host_tmp_path, "wb"); // Changed to binary mode
    if (hex_file == NULL) {
        fprintf(stderr, "Error opening hex data file for writing on host\n");
        return false;
    }

    // Write raw binary data directly
    size_t written = fwrite(data, 1, size, hex_file);
    fclose(hex_file);

    if (written != size) {
        fprintf(stderr, "Failed to write all data to file\n");
        return false;
    }

    // Push the file to Android device using adb
    char adb_command[256];
    snprintf(adb_command, sizeof(adb_command),
             "adb push %s /data/local/tmp/hex_data.txt", host_tmp_path);

    char* result = execute_command(adb_command);
    if (!result || strstr(result, "error") != NULL) {
        fprintf(stderr, "Failed to push hex data file to device\n");
        free(result);
        return false;
    }
    free(result);
    return true;
}

char* execute_command(const char* command) {
    char buffer[128];
    char* result = NULL;
    size_t result_len = 0;
    FILE* pipe = NULL;
    pipe = popen(command, "r");
    if (!pipe) {
        perror("popen failed");
        fprintf(stderr, "Failed to execute command: %s\n", command); // More informative error
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t chunk_len = strlen(buffer);
        char* temp = static_cast<char*>(realloc(result, result_len + chunk_len + 1));
        if (!temp) {
            perror("realloc failed");
            fprintf(stderr, "Memory allocation failed while processing command: %s\n", command); // More informative error
            free(result);
            pclose(pipe);
            return NULL;
        }
        result = temp;
        strcpy(result + result_len, buffer);
        result_len += chunk_len;
    }

    int pclose_status = pclose(pipe);

    if (result == NULL) {
        result = (char*)malloc(1);
        if (result) {
            result[0] = '\0';
        }
    }
    return result;
}

bool is_android_device_connected() {
    char* result = execute_command("adb devices | grep -v List");
    if (result == NULL) {
        fprintf(stderr, "Error: Failed to execute adb devices command.\n");
        return false;
    }

    // Check if any device is listed
    bool connected = strlen(result) > 0 && strstr(result, "device") != NULL;
    free(result);
    return connected;
}

bool execute_ril_function(const uint8_t *data, size_t size) {
    // Check input validity
    if (!data || size == 0) {
        fprintf(stderr, "Invalid input parameters\n");
        return false;
    }

    // Check if Android device is connected
    if (!is_android_device_connected()) {
        fprintf(stderr, "No Android device connected\n");
        return false;
    }

    // Write fuzzing data to temporary file and push to device
    if (!write_hex_data_to_file(data, size)) {
        return false;
    }

    // Use adb to write data to IPC device
    const char* ipc_device = "/dev/umts_ipc1";
    bool write_success = false;

    char adb_write_cmd[512];
    snprintf(adb_write_cmd, sizeof(adb_write_cmd),
            "adb shell su -c 'dd if=/data/local/tmp/hex_data.txt of=%s bs=512'", ipc_device);

    char* result = execute_command(adb_write_cmd);

    if (result && strstr(result, "error") == NULL) {
        write_success = true;
    }
    
    free(result);

    if (!write_success) {
        fprintf(stderr, "Failed to write to IPC device on Android\n");
        return false;
    }
    // Clean up temporary file on device
    execute_command("adb shell rm /data/local/tmp/hex_data.txt");
    return true;
}

std::vector<std::pair<uint8_t*, size_t>> split_input_by_AA(uint8_t* input, size_t len) {
    std::vector<std::pair<uint8_t*, size_t>> chunks;

    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == 0xAA) {  // Changed from ' ' to 0xAA
            if (i > start) {
                uint8_t* chunk = (uint8_t*)malloc(i - start);
                memcpy(chunk, input + start, i - start);
                chunks.push_back({chunk, i - start});
            }
            start = i + 1;
        }
    }

    // Handle the last chunk
    if (start < len) {
        uint8_t* chunk = (uint8_t*)malloc(len - start);
        memcpy(chunk, input + start, len - start);
        chunks.push_back({chunk, len - start});
    }

    return chunks;
}



