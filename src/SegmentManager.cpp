#include "SegmentManager.h"
#include "Common.h"
#include <future>
#include <iostream>

using namespace HLS;

const int CACHE_DURATION_LIMIT = 30;

SegmentManager::SegmentManager(SegmentManagerCallback * callback) : m_nextReadySegment(0),
m_targetDuration(0),
m_maxCacheSegment(0),
m_processInterrupted(false),
m_callback(callback)
{
	m_downloadThread = std::thread(&SegmentManager::downloadProcessor, this);
}

SegmentManager::~SegmentManager()
{
	m_processInterrupted = true;
	m_condWait.signal();
	if (m_downloadThread.joinable())
	{
		m_downloadThread.join();
	}
	m_cachedBuffers.clear();
}

void SegmentManager::downloadProcessor()
{
	HTTPHandler httpHandler;
	while (!m_processInterrupted)
	{
		m_condWait.timedWait(2000);

		if (m_queuedSegments.empty())
		{
			continue;
		}
		int removeRangeLower = m_nextReadySegment - 2 * m_maxCacheSegment;
		int removeRangeUpper = m_nextReadySegment + m_maxCacheSegment;

		std::vector<int> itemsToRemove;
		m_mutex.lock();
		for (auto& buffer : m_cachedBuffers)
		{
			if (buffer.first < removeRangeLower)
			{
				int itemId = buffer.first;
				if (m_cachedBuffers.count(itemId))
				{
					std::cout << "Removing buffer cached " << itemId << std::endl;
					m_cachedBuffers.erase(itemId);
					break;
				}
			}
		}

		auto segment = m_queuedSegments.front();
		m_queuedSegments.pop();
		// check if available in cache
		if (m_cachedBuffers.count(segment.m_sequenceId))
		{
			m_mutex.unlock();
			continue;
		}
		m_mutex.unlock();

		std::vector<std::pair<std::string, std::string>> args;
		std::vector<uint8_t*> inData;
		std::vector<int> sizes;

		int dataSize = 0;
		int res = httpHandler.getRequest(segment.m_filePath, args, [&](void* data, size_t size) {
			uint8_t* copy = (uint8_t*)calloc(1, size);
			if (copy != nullptr)
			{
				memcpy(copy, data, size);
				inData.push_back(copy);
				sizes.push_back(size);
				dataSize += size;
			}
			});

		if (res == STATUS_OK)
		{
			SegmentBuffer* buffer = new SegmentBuffer(segment.m_sequenceId, dataSize);
			int idx = 0;
			int offset = 0;
			for (auto& data : inData)
			{
				memcpy(buffer->m_data + offset, data, sizes.at(idx));
				offset += sizes.at(idx);
				idx++;
				free(data);
			}
			buffer->m_segment = segment;
			m_cachedBuffers[segment.m_sequenceId] = std::shared_ptr<SegmentBuffer>(buffer);
			std::cout << "Request buffer cached " << segment.m_sequenceId << " dataLen "
				<< dataSize << std::endl;
			m_callback->onDataReady(m_cachedBuffers[segment.m_sequenceId]);
		}
		else
		{
			std::cout << "Request failed " << segment.m_sequenceId;
		}
	}
}

void SegmentManager::setBaseUrl(std::string& baseUrl)
{
	m_baseUrl = baseUrl;
}

void SegmentManager::setSegments(std::map<int, Segment> segments)
{
	m_segments = segments;
	m_nextReadySegment = 0;
	checkLoadNext(0);
}


void HLS::SegmentManager::addSegment(std::map<int, Segment> segments)
{
	for (auto& segment : segments)
	{
		m_segments.insert(segment);
	}
	m_mutex.lock();
	if (m_queuedSegments.empty())
	{
		m_mutex.unlock();
		enqueueSegments();
	}
	else {
		m_mutex.unlock();
	}
}

void HLS::SegmentManager::setTargetDuration(double targetDuration)
{
	m_targetDuration = targetDuration;
	m_maxCacheSegment = m_targetDuration < CACHE_DURATION_LIMIT ?
		(CACHE_DURATION_LIMIT / m_targetDuration) : m_targetDuration;
}

void HLS::SegmentManager::seek(double seekSec)
{
	if (m_totalDuration <= 0)
	{
		return;
	}

	int removeRangeLower = m_nextReadySegment - 2 * m_maxCacheSegment;
	int removeRangeUpper = m_nextReadySegment + m_maxCacheSegment;

	std::vector<int> itemsToRemove;
	m_mutex.lock();
	for (auto& buffer : m_cachedBuffers)
	{
		if (buffer.first < removeRangeLower)
		{
			itemsToRemove.push_back(buffer.first);
		}
	}

	for (auto& itemId : itemsToRemove)
	{
		if (m_cachedBuffers.count(itemId))
		{
			std::cout << "Removing buffer cached on seek " << itemId << std::endl;
			m_cachedBuffers.erase(itemId);
		}
	}
	m_mutex.unlock();

	double trackSec = 0;
	int requiredSequence = 0;
	for (auto& segment : m_segments)
	{
		if (trackSec > seekSec)
		{
			break;
		}
		requiredSequence = segment.first;
		trackSec += segment.second.m_duration;
	}
	if (requiredSequence < m_segments.size())
	{
		m_nextReadySegment = requiredSequence;
		enqueueSegments();
	}
}

void SegmentManager::setTotalDuration(double duration)
{
	m_totalDuration = duration;
}

void SegmentManager::checkLoadNext(int runningSequenceId)
{
	if (m_nextReadySegment + 1 >= m_segments.size())
	{
		return;
	}

	if ((m_nextReadySegment - runningSequenceId) < m_maxCacheSegment)
	{
		enqueueSegments();
	}
}

void SegmentManager::enqueueSegments()
{
	int enqueueNum = MIN(m_segments.size() - m_nextReadySegment, m_maxCacheSegment);
	for (int i = m_nextReadySegment; i < (m_nextReadySegment + enqueueNum); i++)
	{
		m_mutex.lock();
		m_queuedSegments.push(m_segments[i]);
		m_mutex.unlock();
	}
	m_nextReadySegment += enqueueNum;
	m_condWait.signal();
}

