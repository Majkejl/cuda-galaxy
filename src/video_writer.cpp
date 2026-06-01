
#include "video_writer.h"

#include <format>
#include <stdexcept>

VideoWriter::VideoWriter(int width, int height, int fps, const std::string& outPath) {
    auto cmd = std::format(
        "ffmpeg -y -f rawvideo -pixel_format rgba -video_size {}x{} "
        "-framerate {} -i - -vf vflip -c:v libx264 -preset slow "
        "-crf 18 -pix_fmt yuv420p \"{}\"", width, height, fps, outPath
    );

    m_pipe = _popen(cmd.c_str(), "wb");
    if (m_pipe == nullptr) throw std::runtime_error("pipe not opened");
}

VideoWriter::~VideoWriter() {
    if (m_pipe) _pclose(m_pipe);
}

void VideoWriter::writeFrame(const unsigned char* data, size_t bytes) {
    if (fwrite(data, 1, bytes, m_pipe) != bytes) throw std::runtime_error("ffmpeg on path?");
}