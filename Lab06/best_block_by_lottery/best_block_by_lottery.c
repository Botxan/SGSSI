#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <openssl/sha.h>

#define MIN_ZERO_PREFIX 1
#define MAX_ZERO_PREFIX SHA256_DIGEST_LENGTH
#define MIN_COINBASE 1
#define MAX_COINBASE 200
#define COINBASE_RANGE MAX_COINBASE - MIN_COINBASE

#define MAX_FILE_NAME_SIZE 100
#define MAX_CANDIDATES 100

typedef struct candidate {
    char filename[MAX_FILE_NAME_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    int coinbase;
    int zero_prefix_length;
    long *lottery_numbers;
    int lottery_number_amount;
} Candidate;

typedef struct candidates {
    Candidate candidate[MAX_CANDIDATES];
    long num_candidates;
} Candidates;

int generate_random_number(int min, int max) {
    int random_number = rand();

    return (random_number % (max - min + 1)) + min;
}

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

int is_valid(FILE *block_file, FILE *candidate_file, unsigned char hash[SHA256_DIGEST_LENGTH], int *coinbase) {
    char block_line[256], candidate_line[256], random_hex[9], hex_id[3];
    int ch1, ch2;
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
        if (sscanf(candidate_line, "%8[0-9a-f] %2[0-9a-f] %3d", random_hex, hex_id, coinbase) != 3) {
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

int get_lottery_numbers(int prefix_length, int coinbase) {
    return (int) (pow(2, prefix_length) - pow(2, prefix_length) * ((double) coinbase / (COINBASE_RANGE)));
}

void assign_lottery_numbers(long *current_number, int amount, long *numbers) {
    int i;

    for (i = 0; i < amount; i++) {
        numbers[i] = *current_number;
        (*current_number)++;
    }
}

void print_candidate_data(Candidate *candidate) {
    int k;

    printf("Name: %s\n", candidate->filename);
    printf("Hash: ");
    print_hash(candidate->hash);
    printf("Zero prefix length: %d\n", candidate->zero_prefix_length);
    printf("Coinbase: %d\n", candidate->coinbase);
    printf("Lottery_numbers: %d\n", candidate->lottery_number_amount);
    printf("Numbers: ");
    for (k = 0; k < candidate->lottery_number_amount-1; k++) {
        printf("%ld, ", candidate->lottery_numbers[k]);
    }
    printf("%ld\n", candidate->lottery_numbers[k]);
}

Candidate *find_winner_candidate(Candidates *candidates, long winner) {
    long i;
    int j, amount;

    for (i = 0; i < candidates->num_candidates; i++) {
        amount = candidates->candidate[i].lottery_number_amount;
        for (j = 0; j < candidates->candidate[i].lottery_number_amount; j++) {
            if (candidates->candidate[i].lottery_numbers[j] == winner) {
                return &(candidates->candidate[i]);
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    char *dir_name, *block_name, *candidate_name, best_candidate_name[MAX_FILE_NAME_SIZE];
    int best_zero_prefix_len, coinbase;
    long current_lottery_number = 0, winner_number;
    float weight, best_weight;
    char full_path[PATH_MAX];
    unsigned char hash[SHA256_DIGEST_LENGTH], best_hash[SHA256_DIGEST_LENGTH];
    struct dirent *entry;
    FILE *block_file, *candidate_file;
    DIR *dir;
    Candidates candidates;
    Candidate *winner_candidate;

    if (argc != 3) {
        printf("Usage: %s <block_file> <directory_with_candidates>\n", argv[0]);
        return 1;
    }

    block_name = argv[1];
    dir_name = argv[2];

    srand(time(NULL));
    candidates.num_candidates = 0;

    block_file = fopen(block_name, "r");
    dir = opendir(dir_name);

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            // check if file is .txt
            if (strlen(entry->d_name) > 4 && strcmp(entry->d_name + strlen(entry->d_name) - 4, ".txt") == 0) {
                fseek(block_file, 0, SEEK_SET);

                snprintf(full_path, PATH_MAX, "%s/%s", dir_name, entry->d_name);
                candidate_file = fopen(full_path, "r");

                if (candidate_file) {
                    // check if starts with same content
                    if (is_valid(block_file, candidate_file, hash, &coinbase)) {
                        candidates.num_candidates++;

                        // new valid candidate, fill struct
                        memcpy(candidates.candidate[ candidates.num_candidates].filename, entry->d_name, MAX_FILE_NAME_SIZE);
                        memcpy(candidates.candidate[ candidates.num_candidates].hash, hash, SHA256_DIGEST_LENGTH);
                        candidates.candidate[ candidates.num_candidates].zero_prefix_length = count_zero_prefix_len(hash);
                        candidates.candidate[ candidates.num_candidates].coinbase = coinbase;
                        candidates.candidate[ candidates.num_candidates].lottery_number_amount = get_lottery_numbers(candidates.candidate[ candidates.num_candidates].zero_prefix_length, coinbase);

                        candidates.candidate[ candidates.num_candidates].lottery_numbers = (long *) malloc(candidates.candidate[ candidates.num_candidates].lottery_number_amount * sizeof(long));
                        assign_lottery_numbers(&current_lottery_number, candidates.candidate[ candidates.num_candidates].lottery_number_amount, candidates.candidate[ candidates.num_candidates].lottery_numbers);

                        print_candidate_data(&(candidates.candidate[candidates.num_candidates]));
                        printf("------------------------------------------------\n");
                    }
                } else {
                    perror("Error while opening candidate file");
                }
            }
        }
        closedir(dir);

        // choose random winner
        winner_number = generate_random_number(0, current_lottery_number);
        winner_candidate = find_winner_candidate(&candidates, winner_number);

        printf("Amount of lottery numbers: %ld\n", current_lottery_number);
        printf("Winner number: %ld\n", winner_number);
        printf("\nWinner candidate: \n");
        print_candidate_data(winner_candidate);

    } else {
        perror("Error opening the directory");
        return 1;
    }

    return 0;
}
