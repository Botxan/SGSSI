#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <time.h>

#define ONE_MINUTE (60 * CLOCKS_PER_SEC)

int main(int argc, char *argv[]) {
    const char *filename;
    unsigned char buffer[SHA256_DIGEST_LENGTH], hash[SHA256_DIGEST_LENGTH];
    int i, iterations = 0;
    size_t bytesRead;
    clock_t startTime, currentTime;
    FILE *file;
    SHA256_CTX sha256;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file name>\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    file = fopen(filename, "rb");

    if (file == NULL) {
        perror("Error opening the file");
        return 1;
    }

    // read the content and calculate the hash
    SHA256_Init(&sha256);

    while ((bytesRead = fread(buffer, 1, SHA256_DIGEST_LENGTH, file)) > 0) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }

    SHA256_Final(hash, &sha256);

    fclose(file);

    /* printf("Document SHA-256 checksum: ");
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n"); */

    
    currentTime = startTime = clock();
    while ((currentTime - startTime) < ONE_MINUTE) {
        memcpy(buffer, hash, SHA256_DIGEST_LENGTH);

        SHA256_Init(&sha256);
        SHA256_Update(&sha256, buffer, SHA256_DIGEST_LENGTH);
        SHA256_Final(hash, &sha256);

        iterations++;

        currentTime = clock();
        /* printf("Hash[%d]: ", iterations);
        for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n"); */
    }

    printf("Total iterations: %d\n", iterations);

    return 0;
}