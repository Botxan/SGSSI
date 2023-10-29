#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

// convert byte to hex
void byteToHexStr(const unsigned char byte, char *hexStr) {
    sprintf(hexStr, "%02x", byte);
}

int main(int argc, char *argv[]) {
    const char *inputFilename, *outputFilename;
    unsigned char buffer[BUFSIZ], hash[SHA256_DIGEST_LENGTH];
    char hexHash[2 * SHA256_DIGEST_LENGTH + 1];
    FILE *inputFile, *outputFile;
    size_t bytesRead;
    SHA256_CTX sha256;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input file name> <output file name>\n", argv[0]);
        return 1;
    }

    inputFilename = argv[1];
    outputFilename = argv[2];
    inputFile = fopen(inputFilename, "rb");

    if (inputFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    outputFile = fopen(outputFilename, "wb");

    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return 1;
    }

    SHA256_Init(&sha256);

    // copy content of the input file to the output file
    while ((bytesRead = fread(buffer, 1, BUFSIZ, inputFile)) > 0) {
        fwrite(buffer, 1, bytesRead, outputFile);
        SHA256_Update(&sha256, buffer, bytesRead);
    }

    SHA256_Final(hash, &sha256);

    // convert hash to hex
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        byteToHexStr(hash[i], &hexHash[i * 2]);
    }
    hexHash[2 * SHA256_DIGEST_LENGTH] = '\0';

    // append hash
    fprintf(outputFile, "%s", hexHash);;

    fclose(inputFile);
    fclose(outputFile);

    printf("Output file '%s' created succesfully with SHA-256 checksum appended.\n", outputFilename);

    return 0;
}