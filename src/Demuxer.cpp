#include "Demuxer.h"
#include <iostream>
extern "C"
{
#include <libavformat/avformat.h>
}
using namespace HLS;

#define AVIO_CTX_BUFSIZE 1024 * 32

static int avioContextRead(void *context, uint8_t *buf, int bufSize)
{
    Demuxer *demuxer = (Demuxer *)context;
    if (demuxer == nullptr)
    {
        return 0;
    }
    return demuxer->readBuffer(buf, bufSize);
}

Demuxer::Demuxer(DemuxerCallback *callback) : m_parserStarted(false),
                                              m_queuebufferOffset(0),
                                              m_videoStreamIdx(-1),
                                              m_audioStreamIdx(-1),
                                              m_audioSR(0),
                                              m_audioChannelNum(0),
                                              m_width(0),
                                              m_height(0),
                                              m_frameRate(0.f),
                                              m_formatContext(nullptr),
                                              m_avioContext(nullptr),
                                              m_avioCtxBuffer(nullptr),
                                              m_callback(callback)
{
}

Demuxer::~Demuxer()
{
}

void Demuxer::enqueueSegment(std::shared_ptr<SegmentBuffer> &buffer)
{
    m_mutex.lock();
    m_segmentQueue.push(buffer);
    m_mutex.unlock();
    if (!m_parserStarted)
    {
        configParser();
    }
}

void Demuxer::configParser()
{
    m_avioCtxBuffer = static_cast<uint8_t *>(av_malloc(AVIO_CTX_BUFSIZE));
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext)
    {
        std::cout << "Unable to avformat_alloc_context" << std::endl;
        return;
    }

    m_avioContext = avio_alloc_context(m_avioCtxBuffer, AVIO_CTX_BUFSIZE, 0, this, &avioContextRead, nullptr, nullptr);
    if (!m_avioContext)
    {
        std::cout << "Unable to m_avio_ctx" << std::endl;
        return;
    }
    auto *formatContext = (AVFormatContext *)m_formatContext;
    formatContext->pb = static_cast<AVIOContext *>(m_avioContext);
    formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    int ret = avformat_open_input(&formatContext, nullptr, nullptr, nullptr);
    if (ret < 0)
    {
        std::cout << "Unable to avformat_open_input" << std::endl;
        return;
    }
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0)
    {
        std::cout << "Unable to avformat_find_stream_info" << std::endl;
        return;
    }

    m_videoStreamIdx = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStreamIdx >= 0)
    {
        AVCodecParameters *codecParameter = formatContext->streams[m_videoStreamIdx]->codecpar;
        m_width = codecParameter->width;
        m_height = codecParameter->height;
        m_frameRate = formatContext->streams[m_videoStreamIdx]->avg_frame_rate.num /
                      (float)formatContext->streams[m_videoStreamIdx]->avg_frame_rate.den;
        m_videoCodec = codecParameter->codec_id == AV_CODEC_ID_H264 ? VIDEO_CODEC_H264 : codecParameter->codec_id == AV_CODEC_ID_H265 ? VIDEO_CODEC_H265
                                                                                                                                      : VIDEO_CODEC_UNKNOWN;
        if (codecParameter->extradata)
        {
            m_videoConfigData = std::vector<uint8_t>(codecParameter->extradata_size);
            memcpy(&m_videoConfigData[0], codecParameter->extradata, codecParameter->extradata_size);
        }
    }
    else
    {
        std::cout << "Stream doesn't have video stream" << std::endl;
    }

    m_audioStreamIdx = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_audioStreamIdx >= 0)
    {
        AVCodecParameters *codecParameter = formatContext->streams[m_audioStreamIdx]->codecpar;
        m_audioChannelNum = codecParameter->channels;
        m_audioSR = codecParameter->sample_rate;
        m_audioCodec = codecParameter->codec_id == AV_CODEC_ID_AAC ? AUDIO_CODEC_AAC : codecParameter->codec_id == AV_CODEC_ID_MP3 ? AUDIO_CODEC_MP3
                                                                                                                                   : AUDIO_CODEC_UNKNOWN;

        if (codecParameter->extradata)
        {
            m_audioConfigData = std::vector<uint8_t>(codecParameter->extradata_size);
            memcpy(&m_audioConfigData[0], codecParameter->extradata, codecParameter->extradata_size);
        }
    }
    else
    {
        std::cout << "Stream doesn't have audio stream" << std::endl;
    }
    m_parserStarted = true;
    m_callback->onDemuxerReady();
}

int Demuxer::readBuffer(uint8_t *buf, int bufSize)
{
    m_mutex.lock();
    if (m_segmentQueue.empty())
    {
        m_mutex.unlock();
        return 0;
    }
    auto segmentBuffer = m_segmentQueue.front();
    int segmentBufferSize = segmentBuffer->m_dataSize - m_queuebufferOffset;
    int readSize = MIN(segmentBufferSize, bufSize);
    memcpy(buf, segmentBuffer->m_data + m_queuebufferOffset, readSize);
    m_queuebufferOffset += readSize;
    if (m_queuebufferOffset >= segmentBuffer->m_dataSize)
    {
        m_queuebufferOffset = 0;
        m_segmentQueue.pop();
        m_callback->onSegmentConsumed(segmentBuffer->m_sequenceIdx);
    }
    m_mutex.unlock();
    return readSize;
}

int Demuxer::getPacket(uint8_t **outBuffer, int *outSize, int64_t *pts, StreamType *streamIdx)
{
    if (!m_formatContext)
    {
        return STATUS_NOT_INITIALIZED;
    }
    auto *formatContext = (AVFormatContext *)m_formatContext;
    AVPacket packet;
    int result = av_read_frame(formatContext, &packet);
    if (result < 0)
    {
        return STATUS_DATA_READ_ERROR;
    }

    if (packet.stream_index == m_videoStreamIdx)
    {
        *streamIdx = STREAM_VIDEO;
    }
    else if (packet.stream_index == m_audioStreamIdx)
    {
        *streamIdx = STREAM_AUDIO;
    }
    else
    {
        *streamIdx = STREAM_UNKNOWN;
        return STATUS_UNKNOWN_STREAM_IDX;
    }

    AVStream *stream = formatContext->streams[packet.stream_index];
    *outSize = packet.size;
    *outBuffer = (uint8_t *)malloc(*outSize);
    memcpy(*outBuffer, packet.data, *outSize);

    int64_t ptsVal = 0;
    if (packet.pts != AV_NOPTS_VALUE)
    {
        ptsVal = (1000000 * (packet.pts * ((float)stream->time_base.num / stream->time_base.den)));
    }
    else if (packet.dts != AV_NOPTS_VALUE)
    {
        ptsVal = (1000000 * (packet.dts * ((float)stream->time_base.num / stream->time_base.den)));
    }
    *pts = ptsVal;
    av_packet_unref(&packet);
    return STATUS_OK;
}

std::vector<uint8_t> &HLS::Demuxer::audioConfigData()
{
    return m_audioConfigData;
}

std::vector<uint8_t> &HLS::Demuxer::videoConfigData()
{
    return m_videoConfigData;
}

int HLS::Demuxer::width()
{
    return m_width;
}

int HLS::Demuxer::height()
{
    return m_height;
}

float Demuxer::frameRate()
{
    return m_frameRate;
}

int HLS::Demuxer::sampleRate()
{
    return m_audioSR;
}

int HLS::Demuxer::channelCount()
{
    return m_audioChannelNum;
}

AudioCodec HLS::Demuxer::audioCodec()
{
    return m_audioCodec;
}

VideoCodec HLS::Demuxer::videoCodec()
{
    return m_videoCodec;
}
