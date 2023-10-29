#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <openssl/sha.h>

#define MAX_FILE_NAME_SIZE 100

void print_hash(unsigned char *hash) {
    int i;

    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int count_zero_prefix_len(const unsigned char *hash) {
    int i, count = 0;
    unsigned char high_nibble, low_nibble;

    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        high_nibble = (hash[i] >> 4) & 0x0F;
        low_nibble = hash[i] & 0x0F;
        
        if (high_nibble == 0x00) count++;
        else break;

        if (low_nibble == 0x00) count++;
        else break;  
    }
    return count;
}

int is_valid(FILE *block_file, FILE *candidate_file, unsigned char hash[SHA256_DIGEST_LENGTH]) {
    char block_line[256], candidate_line[256], random_hex[9], hex_id[3];
    int ehuskoins, ch1, ch2;
    SHA256_CTX sha256;

    // compare each line until one of them reaches EOF
    while ((ch1 = fgetc(block_file)) != EOF) {
        while(ch1 == '\r' || ch1 == '\n') ch1 = fgetc(block_file);
        
        ch2 = fgetc(candidate_file);
        while(ch2 == '\r' || ch2 == '\n') ch2 = fgetc(candidate_file);

        if (ch1 != ch2) {
            return 0;
        }
    }

    // verify line break
    if (fgetc(candidate_file) != '\n') return 0;

    // verify last line
    if (fgets(candidate_line, sizeof(candidate_line), candidate_file)) {
        if (sscanf(candidate_line, "%8[0-9a-f] %2[0-9a-f] %d", random_hex, hex_id, &ehuskoins) != 3) { 
            return 0;
        } 
    }

    fseek(candidate_file, 0, SEEK_SET);

    // calculate hash and verify if starts with 0's
    SHA256_Init(&sha256);

    while (fgets(candidate_line, sizeof(candidate_line), candidate_file)) {
        SHA256_Update(&sha256, candidate_line, strlen(candidate_line));
    }

    SHA256_Final(hash, &sha256);

    if (hash[0] != 0) return 0;

    return 1;
}

int main(int argc, char *argv[]) {
    char *dir_name, *block_name, *candidate_name, best_candidate_name[MAX_FILE_NAME_SIZE];
    int candidate_name_len, zero_prefix_len, best_zero_prefix_len = 0;
    char full_path[PATH_MAX];
    unsigned char hash[SHA256_DIGEST_LENGTH], best_hash[SHA256_DIGEST_LENGTH];
    struct dirent *entry;
    FILE *block_file, *candidate_file;
    DIR *dir;

    if (argc != 3) {
        printf("Usage: %s <block_file> <directory_with_candidates>\n", argv[0]);
        return 1;
    }

    block_name = argv[1];
    dir_name = argv[2];

    block_file = fopen(block_name, "r");
    dir = opendir(dir_name);

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            candidate_name = entry->d_name;
            candidate_name_len = strlen(candidate_name);

            // check if file is .txt
            if (candidate_name_len > 4 && strcmp(candidate_name + candidate_name_len - 4, ".txt") == 0) {
                fseek(block_file, 0, SEEK_SET);
                snprintf(full_path, PATH_MAX, "%s/%s", dir_name, candidate_name);
                candidate_file = fopen(full_path, "r");
                if (candidate_file) {
                    // check if starts with same content
                    if (is_valid(block_file, candidate_file, hash)) {
                        zero_prefix_len = count_zero_prefix_len(hash);
                        printf("%s: %d leading zeros\n", candidate_name, zero_prefix_len);
                        printf("hash: ");
                        print_hash(hash);
                        printf("------\n");

                        // update best PoW
                        if (zero_prefix_len > best_zero_prefix_len) {
                            memcpy(best_hash, hash, SHA256_DIGEST_LENGTH);
                            best_zero_prefix_len = zero_prefix_len;
                            memcpy(best_candidate_name, candidate_name, MAX_FILE_NAME_SIZE);
                        }
                    }
                } else {
                    perror("Error while opening candidate file");
                }
            }
        }
        closedir(dir);
        
        printf("\nLargest zero prefix file: %s\n", best_candidate_name);
        printf("Hash: ");
        print_hash(best_hash);
        printf("Leading zeros: %d\n", best_zero_prefix_len);
    } else {
        perror("Error opening the directory");
        return 1;
    }

    return 0;
}
