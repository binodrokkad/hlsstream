#include "HLSStreamer.h"
#include "Common.h"

#include <iostream>

using namespace HLS;

HLSStreamer::HLSStreamer(HLSStreamerCallback *callback) : m_processInterrupted(false),
														  m_hlsReader(this),
														  m_callback(callback),
														  m_demuxer(this)
{
}

HLSStreamer::~HLSStreamer()
{
	destroy();
}

int HLSStreamer::open(std::string &url)
{
	m_mainUrl = url;
	destroy();
	m_parseThread = std::thread(&HLSStreamer::parseProcessor, this);
	return m_hlsReader.init(url);
}

void HLSStreamer::onConsumed(int id)
{
	m_hlsReader.onConsumed(id);
}

HLSType HLSStreamer::hlsType()
{
	return m_hlsReader.hlsType();
}

void HLSStreamer::destroy()
{
	m_processInterrupted = true;
	m_waiter.signal();
	if (m_parseThread.joinable())
	{
		m_parseThread.join();
	}
}

void HLSStreamer::parseProcessor()
{
	m_processInterrupted = false;
	while (!m_processInterrupted)
	{
		m_waiter.timedWait(2000);
		int result = STATUS_OK;
		while (!m_processInterrupted && result == STATUS_OK)
		{
			uint8_t *buffer = nullptr;
			int bufSize = 0;
			int64_t pts = 0;
			StreamType streamIdx = STREAM_UNKNOWN;
			m_mutex.lock();
			result = m_demuxer.getPacket(&buffer, &bufSize, &pts, &streamIdx);
			m_mutex.unlock();
			if (result == STATUS_OK &&
				buffer &&
				bufSize &&
				streamIdx != STREAM_UNKNOWN)
			{
				if (m_callback)
				{
					m_callback->onPacketReady(streamIdx, buffer, bufSize, pts);
				}
				free(buffer);
			}
		}
	}
}

void HLSStreamer::onDuration(double duration)
{
	if (m_callback)
	{
		m_callback->onDuration(duration);
	}
}

void HLSStreamer::onSegmentData(std::shared_ptr<HLS::SegmentBuffer> &segmentData)
{
	m_mutex.lock();
	m_demuxer.enqueueSegment(segmentData);
	m_mutex.unlock();
	m_waiter.signal();
}

void HLSStreamer::onDemuxerReady()
{
	if (m_callback)
	{
		m_callback->onVideoSetup(!m_demuxer.videoConfigData().empty() ? &m_demuxer.videoConfigData()[0] : nullptr,
								 m_demuxer.videoConfigData().size(),
								 m_demuxer.videoCodec(),
								 m_demuxer.width(),
								 m_demuxer.height(),
								 m_demuxer.frameRate());
		m_callback->onAudioSetup(
			!m_demuxer.audioConfigData().empty() ? &m_demuxer.audioConfigData()[0] : nullptr,
			m_demuxer.audioConfigData().size(),
			m_demuxer.audioCodec(),
			m_demuxer.sampleRate(),
			m_demuxer.channelCount());
	}
}

void HLSStreamer::onSegmentConsumed(int id)
{
	if (m_callback)
	{
		m_callback->onSegmentConsumed(id);
	}
}
