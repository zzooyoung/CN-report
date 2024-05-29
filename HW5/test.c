#include <stdio.h>
#include <string.h>

// 두 바이트 값을 더하고, 오버플로 발생 시 처리하는 함수
unsigned short addWithCarry(unsigned short a, unsigned short b) {
    unsigned int sum = a + b;
    // 오버플로 발생 시 처리 ( 윤회식 자리올림 )
    while (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    sum = sum + 1;
    return (unsigned short)sum;
}

// 문자열로부터 2바이트 체크섬을 계산하는 함수
unsigned short calculateChecksum(const char *str, int length) {
    unsigned short checksum = 0;
    int i;

    // 문자열의 각 문자에 대해 루프를 돌면서 체크섬 계산
    for (i = 0; i < length - 1; i += 2) {
        unsigned short word = (unsigned char)str[i] | ((unsigned char)str[i + 1] << 8);
        checksum = addWithCarry(checksum, word);
    }

    // 홀수 길이의 문자열 처리
    if (length & 1) {
        unsigned short lastByte = (unsigned char)str[length - 1];
        checksum = addWithCarry(checksum, lastByte);
    }

    return checksum;
}

int main() {
    // 예제 문자열 (UTF-8 인코딩된 한글 문자열을 가정, 실제로는 UCS-2 또는 UTF-16 문자열 사용)
    char koreanString[] = "안녕하세요"; // 실제로는 2바이트 문자열 사용
    int length = strlen(koreanString);

    unsigned short checksum = calculateChecksum(koreanString, length);

    printf("Checksum: %u\n", checksum);

    // 여기서 checksum 값을 파일에 저장하거나 전송할 수 있습니다.

    return 0;
}
