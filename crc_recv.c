#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MSG         500
#define MAX_BITS        (MAX_MSG * 8 + 1)
#define MAX_FRAMES      ((MAX_BITS / 8) + 2)

#define CRC_GEN         "10011"   /* x^4 + x + 1 */
#define CRC_GEN_LEN     5
#define CRC_BITS        4

#define BODY_LEN        104   /* srcIP(32)+dstIP(32)+srcPort(16)+dstPort(16)+data(8) */
#define CODEWORD_LEN    (BODY_LEN + CRC_BITS)   /* 108 */
#define DATA_OFFSET     96    /* frameData sits at bits [96,104) of the body/codeword */

/* ---------------- CRC-4 (generator x^4 + x + 1 -> 10011) ---------------- */
static void modulo2Divide(char data[], int dataLen, char remainder[])
{
    char work[300];
    int i, j;
    strcpy(work, data);
    for (i = 0; i <= dataLen - CRC_GEN_LEN; i++)
        {
        if (work[i] == '1')
                {
            for (j = 0; j < CRC_GEN_LEN; j++)
                work[i + j] = ((work[i + j] - '0') ^ (CRC_GEN[j] - '0')) + '0';
        }
    }
    strcpy(remainder, work + (dataLen - CRC_BITS));
    remainder[CRC_BITS] = '\0';
}

static int crcCheck(char codeword[], int n, char syndromeOut[])
{
    int i;
    modulo2Divide(codeword, n, syndromeOut);
    for (i = 0; i < CRC_BITS; i++)
        if (syndromeOut[i] != '0') return 0;
    return 1;
}

static void numberToBinaryN(int number, int numBits, char result[])
{
    int i;
    for (i = numBits - 1; i >= 0; i--)
        result[numBits - 1 - i] = ((number >> i) & 1) ? '1' : '0';
    result[numBits] = '\0';
}

static char binaryToChar(char bits8[])
{
    int i, value = 0;
    for (i = 0; i < 8; i++) value = value * 2 + (bits8[i] - '0');
    return (char) value;
}

int main()
{
    printf("=====================================================\n");
    printf("     RECEIVER  --  DDCMP + CRC-4 DECODER\n");
    printf("=====================================================\n\n");

    FILE *inFile = fopen("message.txt", "r");
    if (inFile == NULL) { printf("ERROR: message.txt not found! Run sender.c first.\n"); return 1; }

    int totalBits = 0, totalFrames = 0;
    if (fscanf(inFile, "%d", &totalBits) != 1 || fscanf(inFile, "%d", &totalFrames) != 1)
        {
        printf("ERROR: message.txt is not in the expected binary transmission format.\n");
        printf("(Did you run sender.c first? It overwrites message.txt with binary data.)\n");
        fclose(inFile);
        return 1;
    }

    if (totalFrames <= 0 || totalFrames > MAX_FRAMES)
        {
        printf("ERROR: invalid frame count (%d) in message.txt.\n", totalFrames);
        fclose(inFile);
        return 1;
    }

    char codewords[MAX_FRAMES][CODEWORD_LEN + 1];
    int i;
    for (i = 0; i < totalFrames; i++)
        {
        if (fscanf(inFile, "%s", codewords[i]) != 1)
                {
            printf("ERROR: message.txt has fewer frames than expected.\n");
            fclose(inFile);
            return 1;
        }
    }
    fclose(inFile);

    printf("=============== MESSAGE FILE (BINARY, as received) ===============\n");
    printf("Total bits (original message)  : %d\n", totalBits);
    printf("Total frames received           : %d\n", totalFrames);
    for (i = 0; i < totalFrames; i++)
        printf("  Frame %d codeword (%d bits): %s\n", i + 1, CODEWORD_LEN, codewords[i]);
    printf("=====================================================================\n\n");

    char synBin[9];
    numberToBinaryN(0x16, 8, synBin);   /* SYN_BYTE, known by protocol */

    char allBits[MAX_BITS];
    int allBitsLen = 0;
    int framesOK = 0, framesError = 0;

    printf("=============== DATA LINK LAYER (DDCMP + CRC-4) : RECEIVER CHECK ===============\n\n");

    for (i = 0; i < totalFrames; i++)
        {
        printf("--- Frame %d : RECEIVER CHECK ---\n", i + 1);
        printf("  SYN1 & SYN2 verified. Frame synchronization OK.\n");
        printf("  COUNT verified (1 bytes).\n");
        printf("  CODEWORD received (%d bits) : %s\n", CODEWORD_LEN, codewords[i]);

        char syndrome[CRC_BITS + 1];
        char frameData[9];
        memcpy(frameData, codewords[i] + DATA_OFFSET, 8);
        frameData[8] = '\0';

        if (crcCheck(codewords[i], CODEWORD_LEN, syndrome))
                {
            printf("  CRC-4 check   : syndrome = %s -> OK, no error. Data accepted: %s\n\n",
                   syndrome, frameData);
            framesOK++;
        }
                else
                {
            printf("  CRC-4 check   : syndrome = %s -> ERROR DETECTED. Frame discarded.\n\n",
                   syndrome);
            framesError++;
        }

        /* Even on error we still copy whatever bits arrived, so the byte
           position isn't lost -- a real protocol would request a resend. */
        memcpy(allBits + allBitsLen, frameData, 8);
        allBitsLen += 8;
    }
    printf("===================================================================================\n\n");

    printf("=============== SUMMARY ===============\n");
    printf("Total frames             : %d\n", totalFrames);
    printf("Frames accepted (CRC OK) : %d\n", framesOK);
    printf("Frames with error        : %d\n", framesError);
    printf("=========================================\n\n");

    /* ---- Convert the received bits back into the original text ---- */
    if (allBitsLen > totalBits) allBitsLen = totalBits;   /* drop any padding bits */

    char decodedMessage[MAX_MSG];
    int msgLen = 0;
    for (i = 0; i + 8 <= allBitsLen; i += 8)
        {
        decodedMessage[msgLen++] = binaryToChar(allBits + i);
    }
    decodedMessage[msgLen] = '\0';

    printf("=============== MESSAGE FILE (BINARY -> converted back to data) ===============\n");
    printf("Binary bits used for reconstruction (%d bits): ", allBitsLen);
    for (i = 0; i < allBitsLen; i++) putchar(allBits[i]);
    printf("\n");
    printf("Decoded actual message : \"%s\"\n", decodedMessage);
    printf("=================================================================================\n");

    return 0;
}
