#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util_ril.h"


int main(int argc, char** argv) {
    printf("Running RIL IPC fuzzer tests...\n");
    FILE* fp = fopen(argv[1], "rb");
    // Get file size
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("file size: %zu\n", file_size);
    // Read file content
    uint8_t* input = (uint8_t*)malloc(file_size);
    if (!input) {
        printf("Memory allocation failed\n");
        fclose(fp);
    }
    
    size_t bytes_read = fread(input, 1, file_size, fp);
    fclose(fp);
    std::vector<std::pair<uint8_t*, size_t>> chunks = split_input_by_AA(input, bytes_read);
    printf("bytes read: %zu\n", bytes_read);
    printf("chunks size: %zu\n", chunks.size());
    for (size_t i = 0; i < chunks.size(); i++) {
        // if (i != 45 && i != 48) continue;
        // print the input and i
        printf("Input %zu: ", i);
        printf("Input length: %zu bytes\n", chunks[i].second);
        for (size_t j = 0; j < chunks[i].second; j++) {
            printf("%02x ", chunks[i].first[j]);
        }
        printf("\n");
        execute_ril_function(chunks[i].first, chunks[i].second);
        bool bad_state = is_bad_system_state(get_telephony_registry_state());
        if (bad_state) {
            printf("Bad state detected\n");
        } else {
            printf("System state is good\n");
        }
        fflush(stdout);
        // bool bad_state = is_bad_system_state(get_telephony_registry_state());
        // if (bad_state) {
        //     printf("Bad state found\n");
        //     printf("Input %zu: ", i);
        //     for (size_t j = 0; j < chunks[i].second; j++) {
        //         printf("%02x ", chunks[i].first[j]);
        //     }
        //     printf("\n");
        //     break;
        // }
    }

    for (size_t i = 0; i < chunks.size(); i++) {
        free(chunks[i].first);
    }
    free(input);
    for (size_t i = 0; i < 10; i++) {
        sleep(1);
        bool bad_state = is_bad_system_state(get_telephony_registry_state());
        if (bad_state) {
            printf("Bad state detected\n");
            break;
        }
    }


    printf("bytes read: %zu\n", bytes_read);
    printf("chunks size: %zu\n", chunks.size());

    // FuzzResult result = get_fuzz_result(at_cmd_file);

    // printf("Result: %s\n", result.state);

    printf("\nRIL IPC fuzzer tests completed.\n");
    
    return 0;
} 