#ifndef COMMON_H
#define COMMON_H

#include <cstdint>

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef int64_t int64;
typedef uint64_t uint64;

template<typename T>
struct HeapArray
{
	T* A;
	size_t Length;

	static HeapArray<T> Init(size_t length)
	{
		HeapArray<T> result = { new T[length], length };
		return result;
	}

	static HeapArray<T> Init(T* ptr, size_t length)
	{
		HeapArray<T> result = { ptr, length };
		return result;
	}
};

#endif // COMMON_H