#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

int main(int argc, char *argv[]) {
    int contentIdentical = 1, foundHashLine = 0;
    unsigned char hash1[SHA256_DIGEST_LENGTH], finalHash[SHA256_DIGEST_LENGTH];
    const char *filename1, *filename2;
    char c1, c2, hexHash1[2 * SHA256_DIGEST_LENGTH + 1], line[65]; // size for a hash line in hexadecimal format + null character
    FILE *file1, *file2;
    SHA256_CTX sha256;
    size_t bytesRead;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <file 1> <file 1 with hash>\n", argv[0]);
        return 1;
    }

    filename1 = argv[1];
    filename2 = argv[2];

    // open file 1
    file1 = fopen(filename1, "rb");
    if (file1 == NULL) {
        perror("Error opening file 1");
        return 1;
    }

    // open file 2
    file2 = fopen(filename2, "rb");
    if (file2 == NULL) {
        perror("Error opening file 2");
        fclose(file1);
        return 1;
    }

    // compare the contents of both files
    while ((c1 = fgetc(file1)) != EOF && (c2 = fgetc(file2)) != EOF) {
        if (c1 != c2) {
            contentIdentical = 0;
            break;
        }
    }

    // check if file 2 contains SHA-256 hash of file 1 at the end
    fseek(file2, -64, SEEK_END); // back 65 characters from the end
    if (fgets(line, sizeof(line), file2) != NULL) {
        // Calcular el hash SHA-256 del archivo 1
        SHA256_Init(&sha256);

        fseek(file1, 0, SEEK_SET);
        while ((bytesRead = fread(hash1, 1, SHA256_DIGEST_LENGTH, file1)) > 0) {
            SHA256_Update(&sha256, hash1, bytesRead);
        }

        SHA256_Final(finalHash, &sha256);

        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(hexHash1 + (i * 2), "%02x", finalHash[i]);
        }
        hexHash1[64] = '\0';

        // compare the hash of file 1 with the line in file 2
        if (strcmp(line, hexHash1) == 0) {
            foundHashLine = 1;
        }
    }

    // close files
    fclose(file1);
    fclose(file2);

    // verify results
    if (contentIdentical && foundHashLine) {
        printf("The content of file 2 is identical to file 1 and contains the correct SHA-256 hash.\n");
    } else {
        printf("File 2 does not meet the requirements.\n");
    }

    return 0;
}