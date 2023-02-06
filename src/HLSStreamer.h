#ifndef HLSStreamer_H
#define HLSStreamer_H

#include "HLSReader.h"
#include "Demuxer.h"
#include "Common.h"

namespace HLS
{
	class HLSStreamerCallback
	{
	public:
		virtual void
        onVideoSetup(uint8_t *configData, int size, VideoCodec codec, int width, int height,
                     float frameRate) = 0;
		virtual void onAudioSetup(uint8_t* configData, int size, AudioCodec codec, int sampleRate, int channelCount) = 0;
		virtual void onDuration(double durationSec) = 0;
		virtual void onPacketReady(StreamType streamType, uint8_t *data, int size, int64_t i) = 0;
		virtual void onSegmentConsumed(int id) = 0;
	};

	class HLSStreamer : public HLSReaderCallback, public DemuxerCallback
	{
	public:
		HLSStreamer(HLSStreamerCallback *callback);
		~HLSStreamer();
		int open(std::string& url);
		void onConsumed(int id);
		HLSType hlsType();

	private:
		void destroy();
		void parseProcessor();

		void onDuration(double duration) override;
		void onSegmentData(std::shared_ptr<HLS::SegmentBuffer>& segmentData) override;
	
		void onDemuxerReady() override;
		void onSegmentConsumed(int id) override;
	private:
		bool m_processInterrupted;
		std::string m_mainUrl;
		HLSReader m_hlsReader;
		Demuxer m_demuxer;
		std::mutex m_mutex;
		HLSType m_hlsType;

		std::thread m_parseThread;
		WaitCond m_waiter;
		HLSStreamerCallback* m_callback;
	};
}
#endif // !
