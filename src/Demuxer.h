#ifndef Demuxer_H
#define Demuxer_H

#include "SegmentManager.h"
#include "Common.h"

namespace HLS
{
	class DemuxerCallback
	{
	public:
		virtual void onDemuxerReady() = 0;
		virtual void onSegmentConsumed(int id) = 0;
	};

	class Demuxer
	{
	public:
		Demuxer(DemuxerCallback *callback);
		~Demuxer();
		void enqueueSegment(std::shared_ptr<SegmentBuffer> &buffer);
		void configParser();
		int readBuffer(uint8_t *buf, int bufSize);
		int getPacket(uint8_t **outBuffer, int *outSize, int64_t *pts, StreamType *streamIdx);
		std::vector<uint8_t> &audioConfigData();
		std::vector<uint8_t> &videoConfigData();
		int width();
		int height();
		float frameRate();
		int sampleRate();
		int channelCount();
		AudioCodec audioCodec();
		VideoCodec videoCodec();

	private:
		bool m_parserStarted;
		int m_queuebufferOffset;
		std::queue<std::shared_ptr<SegmentBuffer>> m_segmentQueue;
		int m_videoStreamIdx, m_audioStreamIdx;
		int m_audioSR, m_audioChannelNum;
		int m_width, m_height;
		float m_frameRate;
		VideoCodec m_videoCodec;
		AudioCodec m_audioCodec;

		std::mutex m_mutex;
		std::vector<uint8_t> m_audioConfigData;
		std::vector<uint8_t> m_videoConfigData;

		void *m_formatContext;
		void *m_avioContext;
		uint8_t *m_avioCtxBuffer;
		DemuxerCallback *m_callback;
	};
}
#endif // !Demuxer_H
