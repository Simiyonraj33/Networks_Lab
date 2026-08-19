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

void printBinaryMatrix(char data[MAX_ROWS][COLS], int rowCount)
{
    int i, j;
    printf("\n--------------------------------------------------------\n");
    printf("               BINARY DATA BITS MATRIX                  \n");
    printf("--------------------------------------------------------\n");
    for (i = 0; i < rowCount; i++)
    {
        printf(" Row %2d ('%c'): ", i + 1, binaryToChar(data[i]));
        for (j = 0; j < COLS; j++)
        {
            printf("%c ", data[i][j]);
        }
        printf("\n");
    }
    printf("--------------------------------------------------------\n");
}

int readInputFile(const char *filename, char data[MAX_ROWS][COLS], char text[MAX_ROWS])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        return -1;
    }

    int rowCount = 0;
    int ch;
    int bit;

    while ((ch = fgetc(file)) != EOF && rowCount < MAX_ROWS)
    {
        if (ch == '\r' || ch == '\n')
        {
            continue;
        }

        text[rowCount] = (char)ch;

        for (bit = 7; bit >= 0; bit--)
        {
            data[rowCount][7 - bit] = ((ch >> bit) & 1) ? '1' : '0';
        }
        rowCount++;
    }

    fclose(file);
    text[rowCount] = '\0';
    return rowCount;
}

void generateParity(char data[MAX_ROWS][COLS], int rowCount, int rowParity[], int colParity[], int *intersectParity)
{
    int i, j, count;

    for (i = 0; i < rowCount; i++)
    {
        count = 0;
        for (j = 0; j < COLS; j++)
        {
            if (data[i][j] == '1') count++;
        }
        rowParity[i] = getParity(count);
    }

    for (j = 0; j < COLS; j++)
    {
        count = 0;
        for (i = 0; i < rowCount; i++)
        {
            if (data[i][j] == '1') count++;
        }
        colParity[j] = getParity(count);
    }

    count = 0;
    for (i = 0; i < rowCount; i++)
    {
        if (rowParity[i] == 1) count++;
    }
    *intersectParity = getParity(count);
}

void sendData(const char *filename, char data[MAX_ROWS][COLS], int rowCount, int rowParity[], int colParity[], int intersectParity)
{
    int i, j;
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error: Could not save transmission file!\n");
        return;
    }

    fprintf(file, "%d\n", rowCount);

    for (i = 0; i < rowCount; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            fprintf(file, "%c ", data[i][j]);
        }
        fprintf(file, "%d\n", rowParity[i]);
    }

    for (j = 0; j < COLS; j++)
    {
        fprintf(file, "%d ", colParity[j]);
    }
    fprintf(file, "%d\n", intersectParity);

    fclose(file);
    printf("\n[Sender]: Full payload stream saved to '%s'\n", filename);
}

int main()
{
    char data[MAX_ROWS][COLS];
    char text[MAX_ROWS] = {0};
    int rowParity[MAX_ROWS];
    int colParity[COLS];
    int intersectParity = 0;
    char choice;
    int targetRow, targetCol, r, c;

    printf("========================================================\n");
    printf("     SENDER 2D PARITY ERROR CORRECTION PROCESSING      \n");
    printf("========================================================\n");

    int rowCount = readInputFile("input.txt", data, text);
    if (rowCount <= 0)
    {
        printf("Error: Could not read 'input.txt'. Make sure the file exists.\n");
        return 1;
    }

    printf("Loaded Message   : \"%s\"\n", text);
    printf("Matrix Dimension : %d Rows x %d Columns\n", rowCount, COLS);

    printBinaryMatrix(data, rowCount);

    generateParity(data, rowCount, rowParity, colParity, &intersectParity);

    printf("\nDo you want to inject a single-bit error? (y/n): ");
    if (scanf(" %c", &choice) == 1 && (choice == 'y' || choice == 'Y'))
    {
        printf("Enter Row (1 to %d): ", rowCount);
        scanf("%d", &targetRow);
        printf("Enter Column (1 to %d): ", COLS);
        scanf("%d", &targetCol);

        r = targetRow - 1;
        c = targetCol - 1;

        if (r >= 0 && r < rowCount && c >= 0 && c < COLS)
        {
            data[r][c] = (data[r][c] == '1') ? '0' : '1';
            printf("\n[NET SIM]: Injected bit error at Row %d, Column %d.\n", targetRow, targetCol);
        }
        else
        {
            printf("\n[NET SIM]: Invalid Row or Column position!\n");
        }
    }
    else
    {
        printf("\n[NET SIM]: No error injected...\n");
    }

    sendData("transmitted.txt", data, rowCount, rowParity, colParity, intersectParity);

    printf("========================================================\n");
    return 0;
}
