#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <time.h>

#define HEX_STRING_LENGTH 8
#define ONE_MINUTE (60 * CLOCKS_PER_SEC)

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

void generate_hex_string(char *hex_string, int length, int index) {
    snprintf(hex_string, length + 1, "%08x", index);
}

int main(int argc, char *argv[]) {
    int index = 0, found_zero_prefix = 0, added_space;
    long input_file_size;
    char *input_file_content, hex_string[HEX_STRING_LENGTH], *line_to_append, *best_line_to_append, *hex_id, *requested_ehuskoins;
    unsigned char hash[SHA256_DIGEST_LENGTH], best_hash[SHA256_DIGEST_LENGTH];
    FILE *input_file, *output_file;
    clock_t startTime, currentTime;

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
    line_to_append = (char *) malloc(added_space);
    best_line_to_append = (char *) malloc(added_space);
    input_file_content = (char *) malloc(input_file_size + added_space); 

    if (input_file_content == NULL) {
        perror("Error while assigning memory to buffer\n");
        fclose(input_file);
    }

    fread(input_file_content, 1, input_file_size, input_file);
    fclose(input_file);

    best_hash[0] = 0x10; // dummy starting value
    currentTime = startTime = clock();
    while ((currentTime - startTime) < ONE_MINUTE) {
        generate_hex_string(hex_string, HEX_STRING_LENGTH, index++);
        snprintf(line_to_append, added_space, "\n%s %s %s", hex_string, hex_id, requested_ehuskoins);

        strcpy(&input_file_content[input_file_size], line_to_append);

        SHA256(input_file_content, strlen(input_file_content), hash);

        if (has_greater_zero_prefix(hash, best_hash)) {
            strcpy(best_line_to_append, line_to_append);
            memcpy(best_hash, hash, SHA256_DIGEST_LENGTH);
        }

        currentTime = clock();
    }

    strcpy(&input_file_content[input_file_size], best_line_to_append);

    // store result into output file
    output_file = fopen(argv[2], "w");

    if (output_file == NULL) {
        perror("Error while opening output file");
        return 1;
    }

    fprintf(output_file, "%s", input_file_content);

    printf("PoW: ");
    print_hash(best_hash);

    fclose(output_file);

    free(input_file_content);
    return 0;
}