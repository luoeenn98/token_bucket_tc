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