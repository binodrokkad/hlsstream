#ifndef Common_H
#define Common_H

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
namespace HLS 
{
	enum Status
	{
		STATUS_OK = 0,
		STATUS_UNKNOWN_TYPE,
		STATUS_TIMEOUT_TASK,
		STATUS_NOT_INITIALIZED,
		STATUS_DATA_READ_ERROR,
		STATUS_UNKNOWN_STREAM_IDX,
	};

	enum HLSType 
	{
		HLS_TYPE_UNKNOWN = 0,
		HLS_TYPE_LIVE,
		HLS_TYPE_VOD,
	};

	enum StreamType
	{
		STREAM_UNKNOWN = -1,
		STREAM_VIDEO,
		STREAM_AUDIO,
	};

	enum VideoCodec
	{
		VIDEO_CODEC_UNKNOWN = 0,
		VIDEO_CODEC_H264,
		VIDEO_CODEC_H265,
	};

	enum AudioCodec
	{
		AUDIO_CODEC_UNKNOWN = 0,
		AUDIO_CODEC_AAC,
		AUDIO_CODEC_MP3,
	};
}
#endif // !
