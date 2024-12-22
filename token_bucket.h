#ifndef _TOKEN_BUCKET_H_
#define _TOKEN_BUCKET_H_

#include <stdint.h>
#include <assert.h>
#include <math.h>

typedef uint32_t (*consumeCb)(uint32_t need, uint32_t curTokens);

typedef struct {
    volatile double tokenTime;  /* 最后一次更新令牌的时间戳 */
    double tokenAddRate;        /* 生成令牌的速度 */
    uint32_t burstSize;         /* 允许的最大突发流量，也是令牌桶的最大容量 */
} TokenBucket;

/**
 * type punning:
 * if a member of a union object is accessed after a value has been stored in
 * a different member of the object, the behavior is implementation-defined.
 */
#define DOUBLE_TO_UINT64(f) ( (union {double d; uint64_t u;}){.d = (f)}.u )

static inline uint32_t 
consumeNoLock(TokenBucket* tb, uint32_t num, double now, consumeCb cb) {
    uint32_t res = 0;
    double oldTokenTime, newTokenTime;
    uint32_t curTokens, newTokens;

    assert(tb && cb);

    do {
        oldTokenTime = tb->tokenTime;
        if (now < oldTokenTime) {
            return 0;
        }

        curTokens = (uint32_t)fmin((tb->tokenAddRate * (now - oldTokenTime)),
                (double)tb->burstSize);
        if (curTokens == 0) {
            return 0;
        }

        res = cb(num, curTokens);
        newTokens = curTokens - res;
        newTokenTime = now - newTokens / tb->tokenAddRate;

    } while (!__sync_bool_compare_and_swap(&DOUBLE_TO_UINT64(tb->tokenTime),
            DOUBLE_TO_UINT64(oldTokenTime),
            DOUBLE_TO_UINT64(newTokenTime)));

    return res;
}

/**
 * 初始化一个 TokenBucket 实例，参数说明如下：
 * 
 * @param rate（令牌生成速率）：
 * 该参数表示令牌生成的速率，即每秒钟生成的令牌数量。例如，如果 rate 为 100 token/s，
 * 则令牌桶每秒钟会生成 100 个令牌。
 * 
 * @param burstSize（突发大小）：
 * 该参数表示令牌桶的最大容量，即在任何给定时间点，令牌桶中最多可以有多少个令牌。
 * 突发大小允许系统在短时间内处理比速率限制更多的请求，从而支持突发流量。
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
consume(TokenBucket* tb, uint32_t num, double now) {
    return consumeNoLock(tb, num, now, consumeCallback);
}

#endif