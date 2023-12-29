#ifndef CHAR_RING_BUFFER_H
#define CHAR_RING_BUFFER_H

#ifdef __cplusplus
extern "C" {
#include <atomic>
}
#else
#include <stdatomic.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Structure for the ring buffer
typedef struct {
    char *buffer;
    size_t capacity;
    
#ifdef __cplusplus
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
#else
    _Atomic size_t writePos;
    _Atomic size_t readPos;
#endif

    // Local state positions (not shared between threads)
    size_t localWritePos;
    size_t localReadPos;

    SemaphoreHandle_t dataAvailableSemaphore;
} RingBuffer;

// Function declarations
RingBuffer *createRingBuffer(size_t capacity);
void deleteRingBuffer(RingBuffer *rb);
bool writeToBuffer(RingBuffer *rb, char *data, size_t len);
bool readFromBuffer(RingBuffer *rb, char *data, size_t len);
void waitForData(RingBuffer *rb, size_t len);

#endif // CHAR_RING_BUFFER_H
