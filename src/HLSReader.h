#ifndef HLSReader_H
#define HLSReader_H

#include "PlaylistParser.h"
#include "HTTPHandler.h"
#include "SegmentManager.h"
#include "LiveManager.h"
#include "Common.h"

#include <mutex>

namespace HLS
{
	class HLSReaderCallback
	{
	public:
		virtual void onDuration(double duration) = 0;
		virtual void onSegmentData(std::shared_ptr<SegmentBuffer> &segmentData) = 0;
	};

	class HLSReader : public LiveManagerCallback, public SegmentManagerCallback
	{
	public:
		HLSReader(HLSReaderCallback *readerCallback);
		~HLSReader();
		int init(std::string &playlistUrl);
		void onConsumed(int id);
		HLSType hlsType();

		void destroy();

	private:
		void requestPlaylist(std::string url);
		void handleMasterPlaylist(PlaylistParser &parser);
		void handleMediaPlaylist(PlaylistParser &parser, std::string &url);
		int preparePlaylist(std::string &url);
		std::string updateDestinationUrl(std::string &url);

		int getPlaylist(std::string &url, std::string *playlistData) override;
		void onLiveSegment(std::map<int, Segment> &segments) override;
		void onLiveStarted() override;

		void onDataReady(std::shared_ptr<SegmentBuffer> &segmentBuffer) override;

	private:
		std::mutex m_httpMutex;
		std::string m_playlistUrl;
		std::string m_playlistData;
		std::string m_masterPlaylistData;
		std::string m_baseUrl;
		HLSType m_hlsType;

		std::vector<StreamInf> m_masterStreamsInf;

		HLSReaderCallback *m_readerCallback;
		HTTPHandler m_httpHandler;
		PlaylistParser m_playlistParser;
		PlaylistParser m_masterPlaylistParser;

		std::unique_ptr<SegmentManager> m_segmentManger;
		std::unique_ptr<LiveManager> m_liveManager;
	};
}
#endif