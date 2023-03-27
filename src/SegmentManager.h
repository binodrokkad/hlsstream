#ifndef SegmentManager_H
#define SegmentManager_H
#include "HTTPHandler.h"
#include "PlaylistParser.h"
#include "WaitCond.h"

#include <queue>
#include <thread>

namespace HLS
{
	class SegmentBuffer
	{
	public:
		SegmentBuffer(int sequenceIdx, size_t buffSize) : m_dataSize(buffSize),
														  m_sequenceIdx(sequenceIdx)
		{
			m_data = (uint8_t *)calloc(1, buffSize);
		}

		~SegmentBuffer()
		{
			if (m_data)
			{
				free(m_data);
				m_data = nullptr;
			}
		}

		uint8_t *m_data;
		size_t m_dataSize;
		int m_sequenceIdx;
		Segment m_segment;
	};

	class SegmentManagerCallback
	{
	public:
		virtual void onDataReady(std::shared_ptr<SegmentBuffer> &segmentBuffer) = 0;
	};

	class SegmentManager
	{
	public:
		SegmentManager(SegmentManagerCallback *callback);
		~SegmentManager();
		void setBaseUrl(std::string &baseUrl);
		void setSegments(std::map<int, Segment> segments);
		void setTargetDuration(double targetDuration);
		void seek(double seekSec);
		void setTotalDuration(double duration);
		void checkLoadNext(int runningSequenceId);
		void addSegment(std::map<int, Segment> segments);

	private:
		void downloadProcessor();
		void enqueueSegments();

	private:
		int m_nextReadySegment;
		int m_maxCacheSegment;
		double m_targetDuration;
		double m_totalDuration;
		bool m_processInterrupted;
		std::string m_baseUrl;
		std::mutex m_mutex;
		std::map<int, Segment> m_segments;
		std::map<int, std::shared_ptr<SegmentBuffer>> m_cachedBuffers;
		std::thread m_downloadThread;
		WaitCond m_condWait;
		std::queue<Segment> m_queuedSegments;
		SegmentManagerCallback *m_callback;
	};
}
#endif