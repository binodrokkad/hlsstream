#include "PlaylistParser.h"
#include "Common.h"
#include <vector>
#include <sstream>

using namespace HLS;

std::string EXTM3U = "#EXTM3U";
std::string EXT_X_VERSION = "#EXT-X-VERSION";
std::string EXT_X_STREAM_INF = "#EXT-X-STREAM-INF";
std::string EXT_X_PLAYLIST_TYPE = "#EXT-X-PLAYLIST-TYPE";
std::string EXT_X_ENDLIST = "#EXT-X-ENDLIST";
std::string EXT_X_TARGETDURATION = "#EXT-X-TARGETDURATION";
std::string EXT_X_MEDIA_SEQUENCE = "#EXT-X-MEDIA-SEQUENCE";
std::string EXTINF = "#EXTINF";

std::string BANDWIDTH = "BANDWIDTH";
std::string AVERAGE_BANDWIDTH = "AVERAGE-BANDWIDTH";
std::string RESOLUTION = "RESOLUTION";

PlaylistParser::PlaylistParser() : m_isMasterType(false),
m_isLive(false),
m_targetDuration(0)
{
}

PlaylistParser::~PlaylistParser()
{
}

std::vector<std::string> splitter(const std::string& str, const char& character)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, character))
    {
        tokens.push_back(token);
    }
    return tokens;
}

bool tagAvailable(std::vector<std::string> lines, std::string& input)
{
    for (auto& line : lines)
    {
        if (line.find(input) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::string getLine(std::vector<std::string> lines, std::string& input)
{
    for (auto& line : lines)
    {
        if (line.find(input) != std::string::npos)
        {
            return line;
        }
    }
    return std::string();
}

int PlaylistParser::parse(std::string& data)
{
    m_lines = splitter(data, '\n');

    //first line must contain EXTM3U
    if (m_lines.empty()
        || m_lines.at(0).compare(EXTM3U) != 0
        || !tagAvailable(m_lines, EXT_X_VERSION))
    {
        return STATUS_UNKNOWN_TYPE;
    }

    //determine type master or media
    if (tagAvailable(m_lines, EXT_X_STREAM_INF))
    {
        m_isMasterType = true;
    }
    else
    {
        if (tagAvailable(m_lines, EXT_X_PLAYLIST_TYPE))
        {
            auto line = getLine(m_lines, EXT_X_PLAYLIST_TYPE);
            auto type = line.substr(line.find(":") + 1);
            m_isLive = (type.compare("VOD") == 0);
        }
        else if (tagAvailable(m_lines, EXT_X_ENDLIST))
        {
            m_isLive = false;
        }
        else
        {
            m_isLive = true;
        }
    }
    
    if (tagAvailable(m_lines, EXT_X_TARGETDURATION))
    {
        auto line = getLine(m_lines, EXT_X_TARGETDURATION);
        m_targetDuration = std::stod(line.substr(line.find(":") + 1));
    }

	return 0;
}

bool HLS::PlaylistParser::isMasterType()
{
    return m_isMasterType;
}

bool HLS::PlaylistParser::isLiveType()
{
    return m_isLive;
}

std::vector<StreamInf> HLS::PlaylistParser::getStreamList()
{
    std::vector<StreamInf> streamInfs;
    int lineIdx = 0;
    for (auto& line : m_lines)
    {
        if (line.find(EXT_X_STREAM_INF) != std::string::npos)
        {
            std::string attrStr = line.substr(line.find(":") + 1);
            auto attrs = splitter(attrStr, ',');
            StreamInf streamInf;
            for (auto& attr : attrs)
            {
                if (attr.find(BANDWIDTH) != std::string::npos)
                {
                    streamInf.m_bps = std::stoi(attr.substr(attr.find("=") + 1));
                }
                if (attr.find(AVERAGE_BANDWIDTH) != std::string::npos)
                {
                    streamInf.m_averageBps = std::stoi(attr.substr(attr.find("=") + 1));
                }
                if (attr.find(RESOLUTION) != std::string::npos)
                {
                    std::string resolutions = attr.substr(attr.find("=") + 1);
                    auto resStr = splitter(resolutions, 'x');
                    streamInf.m_width = std::stoi(resStr.at(0));
                    streamInf.m_height = std::stoi(resStr.at(1));
                }
            }

            if (m_lines.size() > lineIdx + 1)
            {
                streamInf.m_playlistUrl = m_lines.at(lineIdx + 1);
            }

            streamInfs.push_back(streamInf);
        }
        lineIdx++;
    }
    return streamInfs;
}

std::map<int, Segment>& PlaylistParser::getSegments()
{
    if (!m_segments.empty())
    {
        return m_segments;
    }
    int lineIdx = 0, sequenceId = 0;
    for (auto& line : m_lines)
    {
        if (line.find(EXTINF) != std::string::npos)
        {
            std::string attrStr = line.substr(line.find(":") + 1);
            auto attrs = splitter(attrStr, ',');
            Segment segment;
            if (!attrs.empty())
            {
                segment.m_duration = std::stod(attrs[0]);
            }
            if (m_lines.size() > lineIdx + 1)
            {
                segment.m_filePath = m_lines.at(lineIdx + 1);
            }
            segment.m_sequenceId = sequenceId;
            m_segments[sequenceId] = segment;
            sequenceId++;
        }
        lineIdx++;
    }
    return m_segments;
}

double HLS::PlaylistParser::getTargetDuration()
{
    return m_targetDuration;
}


