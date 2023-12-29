#ifndef INT_RING_BUFFER_H
#define INT_RING_BUFFER_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Structure for the ring buffer
typedef struct {
    int *buffer;
    size_t capacity;

    // Shared state positions
    _Atomic size_t writePos;
    _Atomic size_t readPos;

    // Local state positions (not shared between threads)
    size_t localWritePos;
    size_t localReadPos;

    SemaphoreHandle_t dataAvailableSemaphore;
} IntRingBuffer;

// Function declarations
IntRingBuffer *createRingBuffer(size_t capacity);
void deleteRingBuffer(IntRingBuffer *rb);
bool writeToBuffer(IntRingBuffer *rb, int *data, size_t len);
bool readFromBuffer(IntRingBuffer *rb, int *data, size_t len);
void waitForData(IntRingBuffer *rb, size_t len);

#endif // INT_RING_BUFFER_H
