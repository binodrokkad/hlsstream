#ifndef LiveManager_H
#define LiveManager_H

#include "PlaylistParser.h"
#include <thread>

namespace HLS
{
	class LiveManagerCallback {
	public:
		virtual int getPlaylist(std::string &url, std::string * playlistData) = 0;
		virtual void onLiveStarted() = 0;
		virtual void onLiveSegment(std::map<int, Segment>& segments) = 0;
	};
	class LiveManager
	{
	public:
		LiveManager(LiveManagerCallback *callback, 
			std::string & url);
		~LiveManager();
		void init();
	private:
		void reloadProcessor();

	private:
		double m_targetDuration;
		bool m_processInterrupted;
		int currentSegmentId;
		std::thread m_reloadThread;
		std::string m_url;
		PlaylistParser m_currentParser;
		LiveManagerCallback * m_callback;
	};
}
#endif // !LiveManager_H
