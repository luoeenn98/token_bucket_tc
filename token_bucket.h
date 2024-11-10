#ifndef _TOKEN_BUCKET_H_
#define _TOKEN_BUCKET_H_

#include <stdint.h>
#include <assert.h>
#include <math.h>

typedef uint32_t (*consumeCb)(uint32_t need, uint32_t curTokens);

typedef struct {
    volatile uint64_t tokenTime; /* 最后一次更新令牌的时间戳 */
    double tokenAddRate;         /* 生成令牌的速度 */
    uint32_t burstSize;          /* 允许的最大突发流量，也是令牌桶的最大容量 */
} TokenBucket;

static inline uint32_t 
consumeNoLock(TokenBucket* tb, uint32_t num, uint64_t now, consumeCb cb) {
    uint64_t oldTokenTime, newTokenTime;
    uint32_t curTokens, newTokens;

    assert(tb && cb);

    do {
        oldTokenTime = tb->tokenTime;
        assert(now >= oldTokenTime);

        curTokens = fmin((uint64_t)(tb->tokenAddRate * (now - 
                oldTokenTime)), tb->burstSize);
        if (curTokens == 0) {
            return 0;
        }

        num = cb(num, curTokens);
        newTokens = curTokens - num;
        newTokenTime = now - newTokens / tb->tokenAddRate;

    } while (!__sync_bool_compare_and_swap(&tb->tokenTime, oldTokenTime,
            newTokenTime));

    return num;
}

/**
 * 在初始化一个 TokenBucket 实例时，会传递以下参数：rate 和 burstSize，参数说明如下：
 * 
 * @param rate（令牌生成速率）：
 * 该参数表示令牌生成的速率，即每秒钟生成的令牌数量。例如，如果 rate 为 100 token/s，则令牌桶每
 * 秒钟会生成 100 个令牌。这个速率控制了令牌桶的再生速度，从而限制了系统的处理能力。通过设置不同
 * 的速率，可以控制系统在单位时间内允许的操作数量。
 * 
 * @param burstSize（突发大小）：
 * 该参数表示令牌桶的最大容量，即在任何给定时间点，令牌桶中最多可以有多少个令牌。如果 burstSize
 * 为 50，则令牌桶中最多可以同时存在 50 个令牌。
 * 突发大小允许系统在短时间内处理比速率限制更多的请求，从而支持突发流量。如果某一时刻的请求量突然
 * 增加，而桶中有足够的令牌，则可以立即处理这些请求。
 * 
 * Note: 初始化好的令牌桶默认是空桶。
 */
static inline void 
tokenBucketInit(TokenBucket* tb, double tokenAddRate, uint32_t burstSize) {
    assert(tb);
    assert(tokenAddRate > 0);
    assert(burstSize > 0);
    tb->tokenAddRate = tokenAddRate;
    tb->burstSize = burstSize;
    tb->tokenTime = 0;
}

static inline uint32_t 
consumeCallback(uint32_t needTokens, uint32_t curTokens) {
    return needTokens > curTokens ? 0 : needTokens;
}

static inline uint32_t 
consume(TokenBucket* tb, uint32_t num, uint64_t now) {
    return consumeNoLock(tb, num, now, consumeCallback);
}

#endif