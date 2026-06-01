#pragma once

#include <cstdio>
#include <string>


class VideoWriter {
public:
    VideoWriter(int width, int height, int fps, const std::string& outPath);
    ~VideoWriter();

    VideoWriter(const VideoWriter&) = delete;
    VideoWriter& operator=(const VideoWriter&) = delete;

    void writeFrame(const unsigned char* data, size_t bytes);

private:
    FILE* m_pipe;
};