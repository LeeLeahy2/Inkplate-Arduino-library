/**
 **************************************************
 * @file        Atomic.ino
 * @brief       Atomic operation support routines for Web_Frame
 *
 * @details     Code copied from R4A_ESP32 library.
 *
 * @author      Lee Leahy
 * @date        2026-11-06
 **************************************************/

//*********************************************************************
int32_t r4aAtomicAdd32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_fetch_add_4(obj, value, moBefore);
}

//*********************************************************************
int32_t r4aAtomicAnd32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_fetch_and_4(obj, value, moBefore);
}

//*********************************************************************
// See https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange
bool r4aAtomicCompare32(int32_t * obj,
                        int32_t * expected,
                        int32_t value,
                        bool unknown,
                        int moSuccess,
                        int moFailure)
{
    return __atomic_compare_exchange_4(obj, expected, value, unknown, moSuccess, moFailure);
}

//*********************************************************************
int32_t r4aAtomicExchange32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_exchange_4(obj, value, moBefore);
}

//*********************************************************************
int32_t r4aAtomicLoad32(int32_t * obj, int moBefore)
{
    return __atomic_load_4(obj, moBefore);
}

//*********************************************************************
int32_t r4aAtomicOr32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_fetch_or_4(obj, value, moBefore);
}

//*********************************************************************
void r4aAtomicStore32(int32_t * obj, int32_t value, int moBefore)
{
    __atomic_store_4(obj, value, moBefore);
}

//*********************************************************************
int32_t r4aAtomicSub32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_fetch_sub_4(obj, value, moBefore);
}

//*********************************************************************
int32_t r4aAtomicXor32(int32_t * obj, int32_t value, int moBefore)
{
    return __atomic_fetch_xor_4(obj, value, moBefore);
}

//*********************************************************************
// Take out a lock
void r4aLockAcquire(volatile int32_t * lock, int moBefore, int moAfter)
{
    uint32_t expected;

    // Single thread access to the lock
    expected = 0;
    while (!__atomic_compare_exchange_4(lock,
                                        &expected,
                                        xPortGetCoreID() + 1,
                                        false,
                                        moBefore,
                                        moAfter))
    {
        expected = 0;
    }
}

//*********************************************************************
// Release a lock
void r4aLockRelease(volatile int32_t * lock, int moBefore)
{
    __atomic_store_4(lock, 0, moBefore);
}
