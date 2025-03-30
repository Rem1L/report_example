#ifndef UTIL_RIL_H
#define UTIL_RIL_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <stdio.h>  // for fprintf and stderr
#include <chrono>
#include <ctime>
bool write_hex_data_to_file(const uint8_t* data, size_t size);

bool execute_ril_function(const uint8_t *data, size_t size);

bool is_android_device_connected();

std::vector<std::pair<uint8_t*, size_t>> split_input_by_AA(uint8_t* input, size_t len);

char* execute_command(const char* command);

bool is_bad_system_state(const char* state);

char* get_telephony_registry_state();

#endif // UTIL_RIL_H