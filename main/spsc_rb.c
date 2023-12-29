#include "spsc_rb.h"

// Function to create a new ring buffer
RingBuffer *createRingBuffer(size_t capacity) {
    RingBuffer *rb = (RingBuffer *)malloc(sizeof(RingBuffer));
    if (!rb) return NULL;

    rb->buffer = (char *)malloc(capacity * sizeof(char));
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
#ifdef __cplusplus
    rb->writePos.store(0, std::memory_order_relaxed);
    rb->readPos.store(0, std::memory_order_relaxed);
#else
    atomic_store(&rb->writePos, 0);
    atomic_store(&rb->readPos, 0);
#endif
    rb->localWritePos = 0;
    rb->localReadPos = 0;
    rb->dataAvailableSemaphore = xSemaphoreCreateBinary();
    return rb;
}

// Function to delete the ring buffer
void deleteRingBuffer(RingBuffer *rb) {
    if (rb) {
        free(rb->buffer);
        free(rb);
    }
}

// Producer function to write data to the buffer
bool writeToBuffer(RingBuffer *rb, char *data, size_t len) {
#ifdef __cplusplus
    size_t currentReadPos = rb->readPos.load(std::memory_order_relaxed);
#else
    size_t currentReadPos = atomic_load(&rb->readPos);
#endif
    size_t availableSpace = (currentReadPos + rb->capacity - rb->localWritePos - 1) % rb->capacity;
    
    if (availableSpace < len){
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        rb->buffer[(rb->localWritePos + i) % rb->capacity] = data[i];
    }

    rb->localWritePos = (rb->localWritePos + len) % rb->capacity;

    // Update shared write position
#ifdef __cplusplus
    rb->writePos.store(rb->localWritePos, std::memory_order_relaxed);
#else
    atomic_store(&rb->writePos, rb->localWritePos);
#endif
    xSemaphoreGive(rb->dataAvailableSemaphore);
    return true;
}


// Consumer function to read data from the buffer
bool readFromBuffer(RingBuffer *rb, char *data, size_t len) {
    if (len == 0) return true;
    waitForData(rb, len);

    for (size_t i = 0; i < len; ++i) {
        data[i] = rb->buffer[(rb->localReadPos + i) % rb->capacity];
    }

    rb->localReadPos = (rb->localReadPos + len) % rb->capacity;

    // Update shared read position
#ifdef __cplusplus
    rb->readPos.store(rb->localReadPos, std::memory_order_relaxed);
#else
    atomic_store(&rb->readPos, rb->localReadPos);
#endif
    return true;
}

void waitForData(RingBuffer *rb, size_t len){
    size_t currentWritePos;
    size_t availableData;
    while (true){
#ifdef __cplusplus
        currentWritePos = rb->writePos.load(std::memory_order_relaxed);
#else
        currentWritePos = atomic_load(&rb->writePos);
#endif
        availableData = (currentWritePos + rb->capacity - rb->localReadPos) % rb->capacity;
        if (len <= availableData){
            break;
        }
        xSemaphoreTake(rb->dataAvailableSemaphore, pdMS_TO_TICKS(1000));
    }
}