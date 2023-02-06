#ifndef PlaylistParser_H
#define PlaylistParser_H

#include <string>
#include <vector>
#include <map>

namespace HLS
{
	class StreamInf {
	public:
		int64_t m_bps = 0;
		int64_t m_averageBps = 0;
		int m_width = 0;
		int m_height = 0;
		std::string m_codec;
		std::string m_playlistUrl;
	};

	class Segment {
	public:
		int m_sequenceId = 0;
		double m_duration = 0;
		std::string m_filePath;
	};

	class PlaylistParser
	{
	public:
		PlaylistParser();
		~PlaylistParser();
		int parse(std::string& data);
		bool isMasterType();
		bool isLiveType();
		std::vector<StreamInf> getStreamList();
		std::map<int, Segment>& getSegments();
		double getTargetDuration();

	private:
		bool m_isMasterType;
		bool m_isLive;
		double m_targetDuration;
		std::vector<std::string> m_lines;
		std::map<int, Segment> m_segments;
	};
}
#endif