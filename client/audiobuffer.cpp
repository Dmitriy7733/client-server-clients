// audiobuffer.cpp
#include "audiobuffer.h"
#include <algorithm> // для std::min

AudioBuffer::AudioBuffer(size_t capacity)
    : buffer_(capacity), capacity_(capacity), readPos_(0), writePos_(0), size_(0) {}

size_t AudioBuffer::write(const void* data, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t bytesWritten = 0;
    const uint8_t* src = static_cast<const uint8_t*>(data);

    while (bytesWritten < bytes && size_ < capacity_) {
        buffer_[writePos_] = *src++;
        writePos_ = (writePos_ + 1) % capacity_;
        ++size_;
        ++bytesWritten;
    }
    return bytesWritten;
}

size_t AudioBuffer::read(void* data, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t bytesRead = 0;
    uint8_t* dst = static_cast<uint8_t*>(data);

    while (bytesRead < bytes && size_ > 0) {
        dst[bytesRead] = buffer_[readPos_];
        readPos_ = (readPos_ + 1) % capacity_;
        --size_;
        ++bytesRead;
    }
    return bytesRead;
}

size_t AudioBuffer::available() {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

void AudioBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    readPos_ = 0;
    writePos_ = 0;
    size_ = 0;
}
