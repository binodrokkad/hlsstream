#include "HLSReader.h"
#include "Common.h"
#include <future>
#include <iostream>

using namespace HLS;

HLSReader::HLSReader(HLSReaderCallback *readerCallback) : m_readerCallback(readerCallback)
{
}

HLSReader::~HLSReader()
{
	destroy();
}

int HLSReader::init(std::string &playlistUrl)
{
	m_playlistUrl = playlistUrl;
	m_baseUrl = playlistUrl.substr(0, playlistUrl.find_last_of('/'));
	return preparePlaylist(playlistUrl);
}

void HLSReader::destroy()
{
	m_readerCallback = nullptr;
}

void HLSReader::onConsumed(int id)
{
	m_segmentManger->checkLoadNext(id);
}

HLSType HLSReader::hlsType()
{
	return m_hlsType;
}

int HLSReader::getPlaylist(std::string &url, std::string *playlistData)
{
	url = updateDestinationUrl(url);

	auto request = std::async(&HLSReader::requestPlaylist, this, url);
	auto status = request.wait_for(std::chrono::seconds(20));

	if (status != std::future_status::ready)
	{
		return STATUS_TIMEOUT_TASK;
	}
	if (m_playlistData.empty())
	{
		return STATUS_UNKNOWN_TYPE;
	}
	*playlistData = m_playlistData;
	return STATUS_OK;
}

void HLSReader::requestPlaylist(std::string url)
{
	m_playlistData.clear();
	std::vector<std::pair<std::string, std::string>> args;
	m_httpHandler.getRequest(url, args, [&](void *data, size_t size)
							 {
		std::string result((char*)data);
		//std::cout << result << "\n";
		m_playlistData += result;
		return; });
}

int HLSReader::preparePlaylist(std::string &url)
{
	url = updateDestinationUrl(url);

	std::string playlistData;
	int result = getPlaylist(url, &playlistData);
	if (result != STATUS_OK)
	{
		return result;
	}

	PlaylistParser parser;
	result = parser.parse(playlistData);
	if (result != STATUS_OK)
	{
		return result;
	}

	if (parser.isMasterType())
	{
		// handle master type
		m_masterPlaylistData = playlistData;
		handleMasterPlaylist(parser);
	}
	else
	{
		// handle media type
		handleMediaPlaylist(parser, url);
	}
	return STATUS_OK;
}

std::string HLS::HLSReader::updateDestinationUrl(std::string &url)
{
	if (!(url.find("http") != std::string::npos))
	{
		url = m_baseUrl + "/" + url;
	}
	return url;
}

void HLSReader::handleMasterPlaylist(PlaylistParser &parser)
{
	m_masterStreamsInf = parser.getStreamList();
	m_masterPlaylistParser = parser;
	preparePlaylist(m_masterStreamsInf.at(3).m_playlistUrl);
}

void HLSReader::handleMediaPlaylist(PlaylistParser &parser, std::string &url)
{
	m_playlistParser = parser;
	std::cout << "Media Desc, is Live: " << parser.isLiveType() << " duration: "
			  << parser.getTargetDuration() << std::endl;
	m_segmentManger = std::make_unique<SegmentManager>(this);
	m_segmentManger->setBaseUrl(m_baseUrl);
	m_segmentManger->setTargetDuration(parser.getTargetDuration());
	if (parser.isLiveType())
	{
		m_hlsType = HLS_TYPE_LIVE;
		m_liveManager = std::make_unique<LiveManager>(this, url);
		m_liveManager->init();
	}
	else
	{
		m_hlsType = HLS_TYPE_VOD;
		auto segments = parser.getSegments();
		double duration = 0;
		for (auto &segment : segments)
		{
			duration += segment.second.m_duration;
			segment.second.m_filePath = updateDestinationUrl(segment.second.m_filePath);
		}
		if (m_readerCallback)
		{
			m_readerCallback->onDuration(duration);
		}
		m_segmentManger->setTotalDuration(duration);
		m_segmentManger->setSegments(segments);
	}
}

void HLS::HLSReader::onLiveSegment(std::map<int, Segment> &segments)
{
	for (auto &segment : segments)
	{
		segment.second.m_filePath = updateDestinationUrl(segment.second.m_filePath);
		std::cout << "onLiveSegment " << segment.second.m_sequenceId << " "
				  << segment.second.m_filePath << std::endl;
	}
	m_segmentManger->addSegment(segments);
}

void HLS::HLSReader::onLiveStarted()
{
	m_segmentManger->checkLoadNext(0);
}

void HLS::HLSReader::onDataReady(std::shared_ptr<SegmentBuffer> &segmentBuffer)
{
	if (m_readerCallback)
	{
		m_readerCallback->onSegmentData(segmentBuffer);
	}
}
