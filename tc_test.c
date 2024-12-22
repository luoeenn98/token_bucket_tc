#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include "token_bucket.h"

#define TOKEN_ADD_RATE  5UL
#define BUCKET_SIZE     100UL
#define US_PER_SEC      1000000UL

static inline uint64_t
getTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * US_PER_SEC + tv.tv_usec;
}

void demo() {
    int i, tokens, begin_time;
    uint64_t total_bytes;
    uint32_t consumed_success_n, consumed_failed_n;
    TokenBucket tb;

    tokenBucketInit(&tb, TOKEN_ADD_RATE, BUCKET_SIZE);
    printf("rate/s:%f, bucket size:%ld\n", tb.tokenAddRate, tb.burstSize);

    tokens = 10;
    total_bytes = 0;
    consumed_success_n = 0;
    consumed_failed_n = 0;

    for (i = 0; i < 50; i++) {
        total_bytes += tokens;
        if (consume(&tb, tokens, (double)i) == tokens) {
            consumed_success_n += tokens;
            printf("第%d秒，一次性消费成功%d个令牌\n", i + 1, tokens);
        } else {
            consumed_failed_n += tokens;
            printf("第%d秒，消费失败\n", i + 1);
        }
    }

    printf("Consumed success:%lu, failed:%lu, fixed rate:%f%\n", 
            consumed_success_n, consumed_failed_n, 
            (consumed_success_n * 100.0)/total_bytes);
}

int main() {
    demo();
    return 0;
}
