/**
 **************************************************
 * @file        Atomic.h
 * @brief       Atomic operation support routine declarations
 *
 * @details     Code copied from R4A_Robot library.
 *
 * @author      Lee Leahy
 * @date        2026-11-06
 **************************************************/

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

//****************************************
// Atomic support
//****************************************

// Perform an operation on an object
// Inputs:
//   obj: Address of an object to be adjusted
//   value: Value used in the operation
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value before the operation
int32_t r4aAtomicAdd32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicAnd32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicExchange32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicOr32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicSub32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicXor32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);

// Perform an operation on an object
// Inputs:
//   obj: Address of an object to be compared and modified
//   expected: Address of an object for the comparison
//   value: Value to store in obj if the comparison is successful
//   unknown:
//   moSuccess: Memory order to use when the comparison is successful
//   moFailure: Memory order to use after comparison failure
// Returns:
//   Returns the true if the two objects were equal and false otherwise
bool r4aAtomicCompare32(int32_t * obj,
                        int32_t * expected,
                        int32_t value,
                        bool unknown = false,
                        int moBefore = __ATOMIC_RELAXED,
                        int moAfter = __ATOMIC_RELAXED);

// Get the value of an atomic object
// Inputs:
//   obj: Address of an object to be read
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value contained in the object
int32_t r4aAtomicLoad(int32_t * obj, int moBefore = __ATOMIC_RELAXED);

// Set the value of an atomic object
// Inputs:
//   obj: Address of an object to be written
//   value: Object's new value
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value before the operation
void r4aAtomicStore32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);

#endif  __ATOMIC_H__
