#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#define SHA256_DIGEST_LENGTH 32

int compare_files(FILE *file1, FILE *file2)
{
    char line1[256], line2[256], random_hex[9], hex_id[3];
    int ehuskoins;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;

    // compare each line until one of them reaches EOF
    while (fgets(line1, sizeof(line1), file1) && fgets(line2, sizeof(line2), file2))
    {
        printf("hey\n");
        if (strcmp(line1, line2) != 0)
        {
            return 0; // Las líneas no son idénticas
        }
    }

    // verify last line
    int result;
    if (fgets(line2, sizeof(line2), file2))
    {
        if ((result = sscanf(line2, "%8[0-9a-f] %2[0-9a-f] %d", random_hex, hex_id, &ehuskoins)) != 3)
        {
            return 0;
        }
    }

    fseek(file2, 0, SEEK_SET);

    // calculate hash and verify if starts with 0's
    SHA256_Init(&sha256);

    while (fgets(line2, sizeof(line2), file2))
    {
        SHA256_Update(&sha256, line2, strlen(line2));
    }

    SHA256_Final(hash, &sha256);

    if (hash[0] != 0)
        return 0;

    return 1;
}

int main(int argc, char *argv[])
{
    int result;
    FILE *file1, *file2;

    if (argc != 3)
    {
        printf("Usage: %s <file 1> <file 2>\n", argv[0]);
        return 1;
    }

    file1 = fopen(argv[1], "r");
    file2 = fopen(argv[2], "r");

    if (file1 == NULL || file2 == NULL)
    {
        perror("Error while opening files");
        return 1;
    }

    result = compare_files(file1, file2);

    fclose(file1);
    fclose(file2);

    if (result)
        printf("The conditions are met.\n");
    else
        printf("Conditions are not met.\n");

    return 0;
}