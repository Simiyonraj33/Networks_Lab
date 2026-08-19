#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 128
#define COLS 8

int getParity(int count)
{
    return (count % 2 == 0) ? 0 : 1;
}

char binaryToChar(const char bits[COLS])
{
    int j;
    char character = 0;
    for (j = 0; j < COLS; j++)
    {
        character = (character << 1) | (bits[j] - '0');
    }
    return character;
}

int receiveData(const char *filename, char data[MAX_ROWS][COLS], int rowParity[], int colParity[], int *intersectParity)
{
    int i, j;
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        return -1;
    }

    int rowCount = 0;
    if (fscanf(file, "%d", &rowCount) != 1)
    {
        fclose(file);
        return -1;
    }

    for (i = 0; i < rowCount; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            fscanf(file, " %c", &data[i][j]);
        }
        fscanf(file, "%d", &rowParity[i]);
    }

    for (j = 0; j < COLS; j++)
    {
        fscanf(file, "%d", &colParity[j]);
    }

    fscanf(file, "%d", intersectParity);

    fclose(file);
    return rowCount;
}

void checkAndFix(char data[MAX_ROWS][COLS], int rowCount, int rowParity[], int colParity[], int intersectParity)
{
    int i, j, count;
    int calcRowParity, calcColParity;
    int errorRow = -1;
    int errorCol = -1;

    printf("\n--------------------------------------------------------\n");
    printf("        2D PARITY ERROR DETECTION & CORRECTION CHECK    \n");
    printf("--------------------------------------------------------\n");

    for (i = 0; i < rowCount; i++)
    {
        count = 0;
        for (j = 0; j < COLS; j++)
        {
            if (data[i][j] == '1') count++;
        }
        calcRowParity = getParity(count);
        if (calcRowParity != rowParity[i])
        {
            errorRow = i;
            printf(" -> Parity Mismatch at Row %d (Recv: %d, Calc: %d)\n", i + 1, rowParity[i], calcRowParity);
        }
    }

    for (j = 0; j < COLS; j++)
    {
        count = 0;
        for (i = 0; i < rowCount; i++)
        {
            if (data[i][j] == '1') count++;
        }
        calcColParity = getParity(count);
        if (calcColParity != colParity[j])
        {
            errorCol = j;
            printf(" -> Parity Mismatch at Column %d (Recv: %d, Calc: %d)\n", j + 1, colParity[j], calcColParity);
        }
    }

    if (errorRow != -1 && errorCol != -1)
    {
        printf("\n========================================================\n");
        printf(" [ERROR DETECTED]: \n");
        printf("========================================================\n");
        printf(">>> ERROR LOCATION DETAILS <<<\n");
        printf("  Corrupted Row Index    : Row #%d (1-indexed)\n", errorRow + 1);
        printf("  Corrupted Column Index : Bit #%d (1-indexed)\n", errorCol + 1);
        printf("  Corrupted Value        : '%c'\n", data[errorRow][errorCol]);

        data[errorRow][errorCol] = (data[errorRow][errorCol] == '1') ? '0' : '1';

        printf("  Corrected Value        : '%c'\n", data[errorRow][errorCol]);
        printf("========================================================\n");
        printf(" [CORRECTED]ry.\n");
    }
    else if (errorRow == -1 && errorCol == -1)
    {
        printf("\n========================================================\n");
        printf(" [ACCEPTED]: Error Check Passed! Codeword valid.\n");
        printf("========================================================\n");
    }
    else
    {
        printf("\n========================================================\n");
        printf(" [DISCARDED]: Unresolvable or multi-bit parity error pattern.\n");
        printf("========================================================\n");
    }

    printf("\n========================================================\n");
    printf("  Decoded Payload Restored: \"");
    for (i = 0; i < rowCount; i++)
    {
        printf("%c", binaryToChar(data[i]));
    }
    printf("\"\n========================================================\n");
}

int main()
{
    char data[MAX_ROWS][COLS];
    int rowParity[MAX_ROWS];
    int colParity[COLS];
    int intersectParity = 0;

    printf("========================================================\n");
    printf("     RECEIVER 2D PARITY PROCESSING & CORRECTION        \n");
    printf("========================================================\n");

    int rowCount = receiveData("transmitted.txt", data, rowParity, colParity, &intersectParity);
    if (rowCount <= 0)
    {
        printf("Error: Could not read 'transmitted.txt'. Make sure sender.c runs first.\n");
        return 1;
    }

    checkAndFix(data, rowCount, rowParity, colParity, intersectParity);

    return 0;
}
