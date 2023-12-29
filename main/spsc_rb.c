#include "spsc_rb.h"

// Function to create a new ring buffer
IntRingBuffer *createRingBuffer(size_t capacity) {
    IntRingBuffer *rb = (IntRingBuffer *)malloc(sizeof(IntRingBuffer));
    if (!rb) return NULL;

    rb->buffer = (int *)malloc(capacity * sizeof(int));
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    atomic_store(&rb->writePos, 0);
    atomic_store(&rb->readPos, 0);
    rb->localWritePos = 0;
    rb->localReadPos = 0;
    rb->dataAvailableSemaphore = xSemaphoreCreateBinary();
    return rb;
}

// Function to delete the ring buffer
void deleteRingBuffer(IntRingBuffer *rb) {
    if (rb) {
        free(rb->buffer);
        free(rb);
    }
}

// Producer function to write data to the buffer
bool writeToBuffer(IntRingBuffer *rb, int *data, size_t len) {
    size_t currentReadPos = atomic_load(&rb->readPos);
    size_t availableSpace = (currentReadPos + rb->capacity - rb->localWritePos - 1) % rb->capacity;
    
    if (availableSpace < len){
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        rb->buffer[(rb->localWritePos + i) % rb->capacity] = data[i];
    }

    rb->localWritePos = (rb->localWritePos + len) % rb->capacity;

    // Update shared write position
    atomic_store(&rb->writePos, rb->localWritePos);
    xSemaphoreGive(rb->dataAvailableSemaphore);
    return true;
}


// Consumer function to read data from the buffer
bool readFromBuffer(IntRingBuffer *rb, int *data, size_t len) {
    if (len == 0) return true;
    waitForData(rb, len);

    for (size_t i = 0; i < len; ++i) {
        data[i] = rb->buffer[(rb->localReadPos + i) % rb->capacity];
    }

    rb->localReadPos = (rb->localReadPos + len) % rb->capacity;

    // Update shared read position
    atomic_store(&rb->readPos, rb->localReadPos);

    return true;
}

void waitForData(IntRingBuffer *rb, size_t len){
    size_t currentWritePos;
    size_t availableData;
    while (true){
        currentWritePos = atomic_load(&rb->writePos);
        availableData = (currentWritePos + rb->capacity - rb->localReadPos) % rb->capacity;
        if (len <= availableData){
            break;
        }
        xSemaphoreTake(rb->dataAvailableSemaphore, pdMS_TO_TICKS(1000));
    }
}