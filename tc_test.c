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

int main() {
    int i, tokens, begin_time;
    uint64_t total_bytes;
    uint32_t consumed_success_n, consumed_failed_n;
    TokenBucket tb;

    /**
     * 在初始化一个 TokenBucket 实例时，会传递以下三个参数：rate、burstSize 和 startTokens。
     * 每个参数的具体含义如下：
     * 
     * rate（令牌生成速率）：
     * 该参数表示令牌生成的速率，即每秒钟生成的令牌数量。例如，如果 rate 为 100 token/s，则令牌桶每
     * 秒钟会生成 100 个令牌。这个速率控制了令牌桶的再生速度，从而限制了系统的处理能力。通过设置不同
     * 的速率，可以控制系统在单位时间内允许的操作数量。
     * 
     * burstSize（突发大小）：
     * 该参数表示令牌桶的最大容量，即在任何给定时间点，令牌桶中最多可以有多少个令牌。如果 burstSize
     * 为 50，则令牌桶中最多可以同时存在 50 个令牌。
     * 突发大小允许系统在短时间内处理比速率限制更多的请求，从而支持突发流量。如果某一时刻的请求量突然
     * 增加，而桶中有足够的令牌，则可以立即处理这些请求。
     * 
     * startTokens（初始令牌数量）：
     * 该参数表示在令牌桶初始化时，桶中有多少个令牌。例如，如果 startTokens 为 10，则令牌桶在初始化时
     * 会有 10 个令牌。这个参数允许在系统启动时即具备处理一定数量请求的能力，而不需要等待令牌逐渐生成。
     */
    tokenBucketInit(&tb, TOKEN_ADD_RATE, BUCKET_SIZE, 0);
    printf("rate/s:%f, bucket size:%ld, token time:%lld\n",
            tb.tokenAddRate, tb.burstSize, tb.tokenTime);

    tokens = 10;
    total_bytes = 0;
    consumed_success_n = 0;
    consumed_failed_n = 0;
    begin_time = tokens / TOKEN_ADD_RATE;
    for (i = begin_time; i < 100 + begin_time; i++) {
        total_bytes += tokens;
        if (consume(&tb, tokens, i) == tokens)
            consumed_success_n += tokens;
        else
            consumed_failed_n += tokens;
    }

    printf("Consumed success:%lu, failed:%lu, fixed rate:%f%\n", 
            consumed_success_n, consumed_failed_n, 
            (consumed_success_n * 100.0)/total_bytes);
    
    return 0;
}
