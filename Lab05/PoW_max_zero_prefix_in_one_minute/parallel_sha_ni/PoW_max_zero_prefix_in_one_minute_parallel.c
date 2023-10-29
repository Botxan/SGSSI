#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <math.h>

#include "./sha256/sha256.h"

#define HEX_STRING_LENGTH 8
#define ONE_MINUTE 60
#define SHA256_DIGEST_LENGTH 32

void print_hash(unsigned char *hash) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int has_greater_zero_prefix(const unsigned char *new_hash, const unsigned char *best_hash) {
    if (new_hash[0] != 0) return 0;

    // check characters one by one
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        if (new_hash[i] < best_hash[i]) {
            return 1; 
        } else if (new_hash[i] > best_hash[i]) {
            return 0;
        }
        // otherwise keep checking the hash
    }

    return 0; // we found the all 0's hash
}

void generate_hex_string(char *hex_string, int length, long long index) {
    snprintf(hex_string, length + 1, "%08x", index);
}

void calculate_sha256(const unsigned char *buffer, size_t length, unsigned char *hash) {
    SHA256_CTX sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, buffer, length);
    sha256_final(&sha256, hash);
}

int main(int argc, char *argv[]) {
    long long total_iterations, found_zero_prefix = 0;
    long input_file_size;
    char *global_input_file_content, *best_global_line_to_append, *hex_id, *requested_ehuskoins;
    unsigned char best_global_hash[SHA256_DIGEST_LENGTH];
    int nthreads, tid, added_space;
    double elapsed_time;
    FILE *input_file, *output_file;
    struct timespec startTime, currentTime;

    if (argc != 5) {
        printf("Usage: %s <input_file> <output_file> <hex_id> <requested_ehuskoins>\n", argv[0]);
        return 1;
    }

    hex_id = argv[3];
    requested_ehuskoins = argv[4];

    input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        perror("Error while opening input file");
        return 1;
    }

    // get input file size
    fseek(input_file, 0, SEEK_END);
    input_file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    // copy input file to a buffer
    added_space = HEX_STRING_LENGTH + strlen(hex_id) + strlen(requested_ehuskoins) + 4; // '\n' + hex_string + ' ' + hex_id + ' ' + ehuskoins + '\0'
    best_global_line_to_append = (char *) malloc(added_space);
    global_input_file_content = (char *) malloc(input_file_size + added_space);

    if (global_input_file_content == NULL) {
        perror("Error while assigning memory to buffer\n");
        fclose(input_file);
    }

    fread(global_input_file_content, 1, input_file_size, input_file);
    fclose(input_file);

    clock_gettime(CLOCK_REALTIME, &startTime);
    clock_gettime(CLOCK_REALTIME, &currentTime);

    best_global_hash[0] = 0x10; // dummy starting value
    total_iterations = pow(16, HEX_STRING_LENGTH);

    #pragma omp parallel default(none) shared(nthreads, startTime, currentTime, elapsed_time, total_iterations, input_file_size, best_global_hash, best_global_line_to_append, global_input_file_content, hex_id, requested_ehuskoins, added_space)
    {
        int tid;
        unsigned char hash[SHA256_DIGEST_LENGTH], best_local_hash[SHA256_DIGEST_LENGTH];
        char hex_string[HEX_STRING_LENGTH+1], *local_input_file_content, *line_to_append, *best_local_line_to_append;
        long long start_index, end_index;

        line_to_append = (char *) malloc(added_space);
        best_local_line_to_append = (char *) malloc(added_space);
        local_input_file_content = (char *) malloc(input_file_size + added_space);
        memcpy(local_input_file_content, global_input_file_content, input_file_size);

        best_local_hash[0] = 0x10; // dummy starting value
        #pragma omp single
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();

        // calculate starting index for each thread
        start_index = (total_iterations / nthreads) * tid;
        end_index = (tid == nthreads - 1) ? total_iterations : (start_index + (total_iterations / nthreads));

        do {
            #pragma single nowait
            {
                clock_gettime(CLOCK_REALTIME, &currentTime);
                elapsed_time = difftime(currentTime.tv_sec, startTime.tv_sec);
            }
            
            generate_hex_string(hex_string, HEX_STRING_LENGTH, start_index++);
            snprintf(line_to_append, added_space, "\n%s %s %s", hex_string, hex_id, requested_ehuskoins);
            line_to_append[added_space-1] = '\0';
            strcpy(&local_input_file_content[input_file_size], line_to_append);

            calculate_sha256(local_input_file_content, strlen(local_input_file_content), hash);

            if (has_greater_zero_prefix(hash, best_local_hash)) {
                memcpy(best_local_line_to_append, line_to_append, added_space);
                memcpy(best_local_hash, hash, SHA256_DIGEST_LENGTH);
            }
        } while (elapsed_time < ONE_MINUTE && start_index < end_index);
      
        // get best hash and best line to append among all threads
        #pragma omp critical
        {  
            // print best hash calculated by each thread
            printf("[%d] Best local hash: ", tid);
            print_hash(best_local_hash);
            printf("[%d] Best local PoW: %s\n", tid, best_local_line_to_append + 1);
            if (has_greater_zero_prefix(best_local_hash, best_global_hash)) {
                memcpy(best_global_line_to_append, best_local_line_to_append, added_space);
                memcpy(best_global_hash, best_local_hash, SHA256_DIGEST_LENGTH);
            }
        }
        free(local_input_file_content);
    };

    strcpy(&global_input_file_content[input_file_size], best_global_line_to_append);
    // store result into output file
    output_file = fopen(argv[2], "w");

    if (output_file == NULL) {
        perror("Error while opening output file");
        return 1;
    }

    fprintf(output_file, "%s", global_input_file_content);

    printf("\nBest global hash: ");
    print_hash(best_global_hash);

    fclose(output_file);

    free(global_input_file_content);
    return 0;
}