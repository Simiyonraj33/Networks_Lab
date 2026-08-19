#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TABLE_SIZE      10
#define MAX_MSG         500
#define MAX_BITS        (MAX_MSG * 8 + 1)
#define MAX_PACKETS     ((MAX_BITS / 16) + 2)
#define SYN_BYTE        0x16
#define PACKET_SIZE     16
#define FRAMES_PER_PKT  2

#define CRC_GEN         "10011"   /* x^4 + x + 1 */
#define CRC_GEN_LEN     5
#define CRC_BITS        4

#define BODY_LEN        104   /* srcIP(32)+dstIP(32)+srcPort(16)+dstPort(16)+data(8) */
#define CODEWORD_LEN    (BODY_LEN + CRC_BITS)   /* 108 */

/* ---------------- URL / IP / MAC hash table ---------------- */
static char urlTable[TABLE_SIZE][50];
static char ipTable[TABLE_SIZE][20];
static char macTable[TABLE_SIZE][20];
static int  used[TABLE_SIZE] = {0};

static int hashFunction(char url[])
{
    int sum = 0, i;
    for (i = 0; url[i] != '\0'; i++) sum += url[i];
    return sum % TABLE_SIZE;
}

static void insertURL(char url[], char ip[], char mac[])
{
    int index = hashFunction(url);
    int count = 0;
    while (used[index] == 1 && count < TABLE_SIZE)
        {
        if (strcmp(urlTable[index], url) == 0) return;
        index = (index + 1) % TABLE_SIZE;
        count++;
    }
    strcpy(urlTable[index], url);
    strcpy(ipTable[index], ip);
    strcpy(macTable[index], mac);
    used[index] = 1;
}

static int searchURL(char url[])
{
    int index = hashFunction(url);
    int count = 0;
    while (count < TABLE_SIZE)
        {
        if (used[index] == 1 && strcmp(urlTable[index], url) == 0) return index;
        index = (index + 1) % TABLE_SIZE;
        count++;
    }
    return -1;
}

static void printTable()
{
    int i;
    printf("---------------------------------------------------\n");
    printf(" HASH TABLE (URL -> IP -> MAC)\n");
    printf("---------------------------------------------------\n");
    for (i = 0; i < TABLE_SIZE; i++)
        {
        if (used[i] == 1)
                {
            printf(" Slot %d : %-15s | %-15s | %s\n",i, urlTable[i], ipTable[i], macTable[i]);
        }
    }
    printf("---------------------------------------------------\n\n");
}

/* ---------------- binary conversion helpers ---------------- */
static void byteToBinary(int number, char result[])
{
    int i;
    for (i = 7; i >= 0; i--)
        result[7 - i] = ((number >> i) & 1) ? '1' : '0';
    result[8] = '\0';
}

static void numberToBinary16(int number, char result[])
{
    int i;
    for (i = 15; i >= 0; i--)
        result[15 - i] = ((number >> i) & 1) ? '1' : '0';
    result[16] = '\0';
}

static void numberToBinaryN(int number, int numBits, char result[])
{
    int i;
    for (i = numBits - 1; i >= 0; i--)
        result[numBits - 1 - i] = ((number >> i) & 1) ? '1' : '0';
    result[numBits] = '\0';
}

static void ipToBinary32(char ip[], char result[])
{
    int a, b, c, d;
    char piece[9];
    sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
    result[0] = '\0';
    byteToBinary(a, piece); strcat(result, piece);
    byteToBinary(b, piece); strcat(result, piece);
    byteToBinary(c, piece); strcat(result, piece);
    byteToBinary(d, piece); strcat(result, piece);
}

static void macToBinary48(char mac[], char result[])
{
    int b0, b1, b2, b3, b4, b5;
    char piece[9];
    sscanf(mac, "%x:%x:%x:%x:%x:%x", &b0, &b1, &b2, &b3, &b4, &b5);
    result[0] = '\0';
    byteToBinary(b0, piece); strcat(result, piece);
    byteToBinary(b1, piece); strcat(result, piece);
    byteToBinary(b2, piece); strcat(result, piece);
    byteToBinary(b3, piece); strcat(result, piece);
    byteToBinary(b4, piece); strcat(result, piece);
    byteToBinary(b5, piece); strcat(result, piece);
}

static void makeRandomIP(char ip[])
{
    int a = 1 + rand() % 223, b = rand() % 256, c = rand() % 256, d = 1 + rand() % 254;
    sprintf(ip, "%d.%d.%d.%d", a, b, c, d);
}

static void makeRandomMAC(char mac[])
{
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
            rand() % 256, rand() % 256, rand() % 256,
            rand() % 256, rand() % 256, rand() % 256);
}

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

static void crcEncode(char dataword[], int k, char crcOut[], char codewordOut[])
{
    char augmented[300];
    strcpy(augmented, dataword);
    strcat(augmented, "0000");
    modulo2Divide(augmented, k + CRC_BITS, crcOut);
    strcpy(codewordOut, dataword);
    strcat(codewordOut, crcOut);
}

int main()
{
    srand(time(NULL));

    printf("=====================================================\n");
    printf("     SENDER  --  4-LAYER NETWORK SIMULATOR (in binary)\n");
    printf("=====================================================\n\n");

    insertURL("google.com",    "142.250.193.14",  "3C:5A:B4:1D:9F:02");
    insertURL("youtube.com",   "142.250.72.14",   "A4:5E:60:D3:2B:19");
    insertURL("facebook.com",  "157.240.22.35",   "F0:2F:74:6B:88:11");
    insertURL("amazon.com",    "205.251.242.103", "B8:27:EB:9A:3C:44");
    insertURL("wikipedia.org", "208.80.154.224",  "00:1A:2B:3C:4D:5E");
    printTable();

    char srcUrl[50], dstUrl[50];
    printf("Enter SOURCE URL (example: google.com): ");
    scanf("%49s", srcUrl);
    printf("Enter DESTINATION URL (example: youtube.com): ");
    scanf("%49s", dstUrl);

    int srcIndex = searchURL(srcUrl);
    if (srcIndex == -1)
        {
        char newIp[20], newMac[20];
        makeRandomIP(newIp); makeRandomMAC(newMac);
        insertURL(srcUrl, newIp, newMac);
        srcIndex = searchURL(srcUrl);
        printf("\n(\"%s\" was not in the table, so a new IP/MAC was created)\n", srcUrl);
    }
    int dstIndex = searchURL(dstUrl);
    if (dstIndex == -1)
        {
        char newIp[20], newMac[20];
        makeRandomIP(newIp); makeRandomMAC(newMac);
        insertURL(dstUrl, newIp, newMac);
        dstIndex = searchURL(dstUrl);
        printf("\n(\"%s\" was not in the table, so a new IP/MAC was created)\n", dstUrl);
    }

    char srcIP[20], dstIP[20], srcMAC[20], dstMAC[20];
    strcpy(srcIP,  ipTable[srcIndex]);
    strcpy(dstIP,  ipTable[dstIndex]);
    strcpy(srcMAC, macTable[srcIndex]);
    strcpy(dstMAC, macTable[dstIndex]);

    char srcIPBin[40], dstIPBin[40], srcMACBin[60], dstMACBin[60];
    ipToBinary32(srcIP, srcIPBin);
    ipToBinary32(dstIP, dstIPBin);
    macToBinary48(srcMAC, srcMACBin);
    macToBinary48(dstMAC, dstMACBin);

    printf("\n----------------- ADDRESS RESOLUTION -----------------\n");
    printf("SOURCE      : %s -> IP %s -> MAC %s\n", srcUrl, srcIP, srcMAC);
    printf("   IP  binary  (32 bits): %s\n", srcIPBin);
    printf("   MAC binary  (48 bits): %s\n", srcMACBin);
    printf("DESTINATION : %s -> IP %s -> MAC %s\n", dstUrl, dstIP, dstMAC);
    printf("   IP  binary  (32 bits): %s\n", dstIPBin);
    printf("   MAC binary  (48 bits): %s\n", dstMACBin);
    printf("-------------------------------------------------------\n\n");

    /* ---- Read the ORIGINAL message.txt (plain text) ---- */
    FILE *inFile = fopen("message.txt", "r");
    if (inFile == NULL) { printf("ERROR: message.txt not found!\n"); return 1; }

    char message[MAX_MSG];
    int msgLength = 0, c;
    while ((c = fgetc(inFile)) != EOF && msgLength < MAX_MSG - 1)
        {
        if (c == '\n' || c == '\r') continue;
        message[msgLength++] = (char) c;
    }
    message[msgLength] = '\0';
    fclose(inFile);

    if (msgLength == 0) { printf("ERROR: message.txt is empty.\n"); return 1; }

    printf("=============== MESSAGE FILE (BEFORE) ===============\n");
    printf("Original text content read from message.txt: \"%s\"\n", message);
    printf("=======================================================\n\n");

    printf("=============== APPLICATION LAYER ===============\n");
    char bits[MAX_BITS];
    bits[0] = '\0';
    int i;
    for (i = 0; i < msgLength; i++)
        {
        char oneByte[9];
        byteToBinary((int)(unsigned char)message[i], oneByte);
        strcat(bits, oneByte);
        printf("  '%c'  ->  %s\n", message[i], oneByte);
    }
    int totalBits = msgLength * 8;
    printf("\nFull bitstream (%d bits): %s\n", totalBits, bits);
    printf("===================================================\n\n");

    printf("=============== MESSAGE FILE (AFTER conversion to binary) ===============\n");
    printf("Binary form that will overwrite message.txt: %s\n", bits);
    printf("============================================================================\n\n");

    /* LAYER 2: TRANSPORT LAYER */
    int srcPort = 1024 + rand() % (65535 - 1024 + 1);
    int dstPort = 1024 + rand() % (65535 - 1024 + 1);
    char srcPortBin[17], dstPortBin[17];
    numberToBinary16(srcPort, srcPortBin);
    numberToBinary16(dstPort, dstPortBin);

    printf("=============== TRANSPORT LAYER ===============\n");
    printf("%s%s%s", srcPortBin, dstPortBin, bits);
    printf("=================================================\n\n");

    /* LAYER 3: NETWORK LAYER */
    int numPackets = totalBits / PACKET_SIZE;
    if (totalBits % PACKET_SIZE != 0) numPackets++;

    char packets[MAX_PACKETS][17];
    printf("=============== NETWORK LAYER ===============\n");
    printf("Total packets: %d\n\n", numPackets);

    int p;
    for (p = 0; p < numPackets; p++)
        {
        int start = p * PACKET_SIZE, b;
        for (b = 0; b < PACKET_SIZE; b++)
                {
            int pos = start + b;
            packets[p][b] = (pos < totalBits) ? bits[pos] : '0';
        }
        packets[p][PACKET_SIZE] = '\0';
        printf("Packet %d:\n", p + 1);
        printf("%s%s%s%s%s\n", srcIPBin, dstIPBin, srcPortBin, dstPortBin, packets[p]);
    }
    printf("===============================================\n\n");

    /* LAYER 4: DATA LINK LAYER (DDCMP + CRC-4) */
    int halfSize    = PACKET_SIZE / FRAMES_PER_PKT;   /* 8 bits per frame */
    int totalFrames = numPackets * FRAMES_PER_PKT;

    printf("=============== DATA LINK LAYER (DDCMP + CRC-4) ===============\n");
    printf("CRC generator polynomial : x^4 + x + 1  ->  %s\n", CRC_GEN);
    printf("Each packet is split into %d frames (%d bits of data each)\n", FRAMES_PER_PKT, halfSize);
    printf("Total frames: %d\n\n", totalFrames);

    int corruptFrame = 0;
    printf("If you'd like to simulate a CRC error, enter the frame number (1-%d)\n", totalFrames);
    printf("to corrupt -- its 8th bit will be flipped -- or enter 0 for no corruption: ");
    scanf("%d", &corruptFrame);
    printf("\n");

    char codewords[MAX_PACKETS * FRAMES_PER_PKT][CODEWORD_LEN + 1];
    int frameNum = 0;

    for (p = 0; p < numPackets; p++)
        {
        int h;
        for (h = 0; h < FRAMES_PER_PKT; h++)
                {
            frameNum++;

            char frameData[9];
            memcpy(frameData, packets[p] + (h * halfSize), halfSize);
            frameData[halfSize] = '\0';

            int byteCount = halfSize / 8;
            char countBin[15];
            numberToBinaryN(byteCount, 14, countBin);

            char synBin[9], classBin[9];
            numberToBinaryN(SYN_BYTE, 8, synBin);
            numberToBinaryN(1, 8, classBin);

            char headerBin[120];
            strcpy(headerBin, srcMACBin);
            strcat(headerBin, dstMACBin);

            char bodyBin[150];
            strcpy(bodyBin, srcIPBin);
            strcat(bodyBin, dstIPBin);
            strcat(bodyBin, srcPortBin);
            strcat(bodyBin, dstPortBin);
            strcat(bodyBin, frameData);

            char crcBin[CRC_BITS + 1];
            char codewordBin[CODEWORD_LEN + 1];
            crcEncode(bodyBin, BODY_LEN, crcBin, codewordBin);

            int wasCorrupted = 0;
            if (frameNum == corruptFrame)
                        {
                codewordBin[7] = (codewordBin[7] == '0') ? '1' : '0';  /* flip bit 8 (index 7) */
                wasCorrupted = 1;
            }

            strcpy(codewords[frameNum - 1], codewordBin);

            printf("--- Frame %d (half %d of Packet %d) : TRANSMITTED ---\n", frameNum, h + 1, p + 1);
            printf("  SYN1    : %s\n", synBin);
            printf("  SYN2    : %s\n", synBin);
            printf("  CLASS   : %s\n", classBin);
            printf("  COUNT   : %s  (byte count = %d)\n", countBin, byteCount);
            printf("  HEADER  : %s\n", headerBin);
            printf("  BODY(dataword, %d bits) : %s\n", BODY_LEN, bodyBin);
            printf("  CRC-4 (remainder, %d bits) : %s\n", CRC_BITS, crcBin);
            printf("  CODEWORD sent (BODY+CRC, %d bits) : %s\n", CODEWORD_LEN, codewordBin);
            if (wasCorrupted)
                        printf("  >>> Bit 8 manually corrupted for this frame (simulating transmission error) <<<\n");
            printf("\n");
        }
    }
    printf("=========================================================\n\n");

    /* ---- Overwrite message.txt with the binary transmission data ---- */
    FILE *outFile = fopen("message.txt", "w");
    if (outFile == NULL) { printf("ERROR: could not open message.txt for writing!\n"); return 1; }

    fprintf(outFile, "%d\n", totalBits);
    fprintf(outFile, "%d\n", totalFrames);
    for (i = 0; i < totalFrames; i++)
        fprintf(outFile, "%s\n", codewords[i]);
    fclose(outFile);

    printf("=============== SUMMARY ===============\n");
    printf("Original message : \"%s\"\n", message);
    printf("Total bits       : %d\n", totalBits);
    printf("Total packets    : %d\n", numPackets);
    printf("Total frames     : %d\n", totalFrames);
    printf(">>> message.txt has been overwritten with the binary transmission data <<<\n");
    printf("    (totalBits, totalFrames, then one %d-bit codeword per frame)\n", CODEWORD_LEN);
    printf("=========================================\n");

    return 0;
}
