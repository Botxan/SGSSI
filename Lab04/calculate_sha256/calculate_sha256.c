#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

int main(int argc, char *argv[]) {
    const char *filename;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    size_t bytesRead;

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
    
    SHA256_Init(&sha256);

    while ((bytesRead = fread(hash, 1, SHA256_DIGEST_LENGTH, file)) > 0) {
        SHA256_Update(&sha256, hash, bytesRead);
    }

    SHA256_Final(hash, &sha256);

    printf("SHA-256 checksum: %s:\n", filename);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    fclose(file);

    return 0;
}