#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char binaryToChar(const char *binStr) {
    char ch = 0;
    for (int i = 0; i < 8; i++) {
        ch = (ch << 1) | (binStr[i] - '0');
    }
    return ch;
}


int verifyChecksum(const char *payload16, const char *checksum16) {
    unsigned int dataVal = 0, chkVal = 0;

    for (int i = 0; i < 16; i++) {
        dataVal = (dataVal << 1) | (payload16[i] - '0');
        chkVal  = (chkVal << 1)  | (checksum16[i] - '0');
    }

    
    unsigned int sum = dataVal + chkVal;

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    
    return (sum == 0xFFFF);
}

int main() {
    FILE *file = fopen("transmission.txt", "r");
    if (!file) {
        printf("Error: 'transmission.txt' file not found! Run sender code first.\n");
        return 1;
    }

    printf("=====================================================\n");
    printf("                RECEIVER SIDE PROCESSING             \n");
    printf("=====================================================\n\n");

    char frameLine[100];
    char extractedBitstream[16384] = "";
    int frameNum = 1;
    int errorDetected = 0;

   
    while (fscanf(file, "%s", frameLine) != EOF) {
        if (strlen(frameLine) < 32) continue;

        char payload16[17];
        char checksum16[17];

        
        strncpy(payload16, frameLine, 16);
        payload16[16] = '\0';

        strncpy(checksum16, frameLine + 16, 16);
        checksum16[16] = '\0';

       
        int isValid = verifyChecksum(payload16, checksum16);

        printf("--- Processing Frame %d ---\n", frameNum);
        printf("Received Frame  : %s\n", frameLine);
        printf("16-bit Payload  : %s\n", payload16);
        printf("16-bit Checksum : %s\n", checksum16);

        if (isValid) {
            printf("Checksum Status : [PASSED] No Error Detected!\n\n");
            strcat(extractedBitstream, payload16);
        } else {
            printf("Checksum Status : [FAILED] Data Corrupted!\n\n");
            errorDetected = 1;
        }

        frameNum++;
    }

    fclose(file);

    printf("=====================================================\n");
    printf("                RECONSTRUCTED MESSAGE               \n");
    printf("=====================================================\n");

    if (errorDetected) {
        printf("Error: Frame corruption detected! Cannot reconstruct message reliably.\n");
        return 1;
    }

    printf("Extracted Binary Stream:\n%s\n\n", extractedBitstream);

   
    int bitLen = strlen(extractedBitstream);
    char reconstructedMsg[1024] = "";
    int msgIdx = 0;

    for (int i = 0; i < bitLen; i += 8) {
        char byteBin[9];
        strncpy(byteBin, extractedBitstream + i, 8);
        byteBin[8] = '\0';

        char ch = binaryToChar(byteBin);
        
        
        if (ch != '\0') {
            reconstructedMsg[msgIdx++] = ch;
        }
    }
    reconstructedMsg[msgIdx] = '\0';

    printf("Decoded Text Message: \"%s\"\n", reconstructedMsg);
    printf("=====================================================\n");

    return 0;
}
