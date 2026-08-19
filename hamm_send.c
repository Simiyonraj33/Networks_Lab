#include <stdio.h>
#include <math.h>
#include <string.h>

void processHammingCode(char originalText[], char dataBitsOnly[]);
void calculateHammingSender(int data[], int dataLen, int encoded[], int *totalBits);
void verifyHammingReceiver(int received[], int totalBits);

void calculateHammingSender(int data[], int dataLen, int encoded[], int *totalBits) {
    int r = 0;
    int j = 0;
    int i, k;
    int parityPos, parityVal;

    while ((1 << r) < (dataLen + r + 1)) {
        r++;
    }
    *totalBits = dataLen + r;

    for (i = 1; i <= *totalBits; i++) {
        if ((i & (i - 1)) == 0) {
            encoded[i] = 0;
        } else {
            encoded[i] = data[j++];
        }
    }

    for (i = 0; i < r; i++) {
        parityPos = (1 << i);
        parityVal = 0;

        for (k = 1; k <= *totalBits; k++) {
            if (k & parityPos) {
                parityVal ^= encoded[k];
            }
        }
        encoded[parityPos] = parityVal;
    }
}

void verifyHammingReceiver(int received[], int totalBits) {
    int errorPos = 0;
    int r = 0;
    int i, k;
    int parityPos, parityVal;

    while ((1 << r) <= totalBits) {
        r++;
    }

    for (i = 0; i < r; i++) {
        parityPos = (1 << i);
        parityVal = 0;

        for (k = 1; k <= totalBits; k++) {
            if (k & parityPos) {
                parityVal ^= received[k];
            }
        }

        if (parityVal != 0) {
            errorPos += parityPos;
        }
    }

    if (errorPos == 0) {
        printf(">>>> HAMMING VERDICT: Data received WITHOUT ANY ERROR.\n");
    } else {
        printf(">>>> HAMMING VERDICT: ERROR DETECTED at bit position %d!\n", errorPos);
        printf("Correcting bit %d from %d to %d...\n", errorPos, received[errorPos], received[errorPos] ^ 1);
        
        received[errorPos] ^= 1;

        printf("Corrected Stream : ");
        for (i = 1; i <= totalBits; i++) {
            printf("%d", received[i]);
        }
        printf("\n");
    }
}

void processHammingCode(char originalText[], char dataBitsOnly[]) {
    int i;
    int dataLen;
    int data[1024];
    int encoded[2048] = {0};
    int totalBits = 0;
    char choice;
    int rxBuffer[2048];
    int errPos;

    printf("\n==============================================================\n");
    printf("            6. HAMMING CODE (ERROR CORRECTION)                \n");
    printf("==============================================================\n");
    printf("Original Payload Text : \"%s\"\n", originalText);

    dataLen = strlen(dataBitsOnly);
    for (i = 0; i < dataLen; i++) {
        data[i] = dataBitsOnly[i] - '0';
    }

    printf("\n[SENDER SIDE] Generating Hamming Code for %d Data Bits...\n", dataLen);
    calculateHammingSender(data, dataLen, encoded, &totalBits);

    printf("Encoded Transmitted Stream : ");
    for (i = 1; i <= totalBits; i++) {
        printf("%d", encoded[i]);
    }
    printf(" (Total Bits: %d)\n\n", totalBits);

    printf("Do you want to inject an error? (y/n): ");
    scanf(" %c", &choice);

    memcpy(rxBuffer, encoded, sizeof(encoded));

    if (choice == 'y' || choice == 'Y') {
        printf("Enter bit position to inject error (1 to %d): ", totalBits);
        scanf("%d", &errPos);

        if (errPos >= 1 && errPos <= totalBits) {
            rxBuffer[errPos] ^= 1;
            
            printf("\n[INJECTING ERROR] Bit at position %d flipped!\n", errPos);
            printf("Corrupted Received Stream  : ");
            for (i = 1; i <= totalBits; i++) {
                printf("%d", rxBuffer[i]);
            }
            printf("\n\n");
        } else {
            printf("\n[WARNING] Invalid bit position! Transmitting without error injection.\n\n");
        }
    } else {
        printf("\n[NO ERROR INJECTED] Transmitting clean stream...\n\n");
    }

    printf("[RECEIVER SIDE] Verifying Received Stream...\n");
    verifyHammingReceiver(rxBuffer, totalBits);
}

int main()
{
    char message[] = "Sample";
    char dataBitsOnly[] = "1000001";

    processHammingCode(message, dataBitsOnly);
    
    return 0;
}
