#include "LiveManager.h"
#include "Common.h"

using namespace HLS;
LiveManager::LiveManager(LiveManagerCallback *callback, std::string &url) : m_targetDuration(0),
																			m_processInterrupted(false),
																			currentSegmentId(0),
																			m_url(url),
																			m_callback(callback)
{
}

LiveManager::~LiveManager()
{
	m_processInterrupted = true;
	if (m_reloadThread.joinable())
	{
		m_reloadThread.join();
	}
}

void HLS::LiveManager::reloadProcessor()
{
	std::string playlistData;
	int reloadGaps = 0;
	int sleepMS = 100;
	bool firstLoad = true;
	while (!m_processInterrupted)
	{
		if (!firstLoad)
		{
			reloadGaps += (m_targetDuration / 2 * 1000) / sleepMS;
			if (reloadGaps < m_targetDuration / 2 * 1000)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
				continue;
			}
			reloadGaps = 0;
		}
		PlaylistParser parser;

		int result = m_callback->getPlaylist(m_url, &playlistData);
		if (result != STATUS_OK)
		{
			// retry
			continue;
		}
		parser.parse(playlistData);
		if (result != STATUS_OK)
		{
			// invalid data
			break;
		}
		m_targetDuration = parser.getTargetDuration();
		auto segments = m_currentParser.getSegments();
		if (segments.empty())
		{
			// m_currentPlaylist is not set yet
			m_currentParser = parser;
			auto newSegments = parser.getSegments();
			int size = MIN(newSegments.size(), 3);
			currentSegmentId = 0;
			std::map<int, Segment> updatedSegments;

			for (auto it = newSegments.rbegin();
				 it != newSegments.rend(); ++it)
			{
				if (currentSegmentId >= size)
				{
					break;
				}

				it->second.m_sequenceId = (size - 1) - currentSegmentId;
				updatedSegments[it->second.m_sequenceId] = it->second;
				currentSegmentId++;
			}
			m_callback->onLiveSegment(updatedSegments);
			m_callback->onLiveStarted();
			continue;
		}
		auto it = segments.end();
		it--;
		Segment lastSegment = it->second;

		auto newSegments = parser.getSegments();

		it = newSegments.end();
		it--;
		Segment newLastSegment = it->second;

		if (lastSegment.m_filePath.compare(newLastSegment.m_filePath) != 0)
		{
			bool foundInList = false;
			std::map<int, Segment> updatedSegments;

			for (auto &segment : newSegments)
			{
				if (foundInList)
				{
					segment.second.m_sequenceId = currentSegmentId;
					updatedSegments[currentSegmentId] = segment.second;
					currentSegmentId++;
				}
				if (segment.second.m_filePath.compare(lastSegment.m_filePath) == 0)
				{
					foundInList = true;
				}
			}
			if (!foundInList)
			{
				for (auto &segment : newSegments)
				{
					segment.second.m_sequenceId = currentSegmentId;
					updatedSegments[currentSegmentId] = segment.second;
					currentSegmentId++;
				}
			}
			m_callback->onLiveSegment(updatedSegments);
		}
		if (firstLoad)
		{
			firstLoad = false;
		}
		m_currentParser = parser;
	}
}

void HLS::LiveManager::init()
{
	m_reloadThread = std::thread(&LiveManager::reloadProcessor, this);
}
