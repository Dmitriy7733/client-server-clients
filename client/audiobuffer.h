// audiobuffer.h
#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <vector>
#include <mutex>
#include <cstdint>
#include <algorithm>
class AudioBuffer {
public:
    explicit AudioBuffer(size_t capacity);
    size_t write(const void* data, size_t bytes);
    size_t read(void* data, size_t bytes);
    size_t available();
    void clear();

private:
    std::vector<uint8_t> buffer_;
    size_t capacity_;
    size_t readPos_;
    size_t writePos_;
    size_t size_;
    std::mutex mutex_;
};

#endif // AUDIOBUFFER_H
