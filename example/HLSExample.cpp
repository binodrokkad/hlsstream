#include <iostream>
#include "HLSStreamer.h"

class TestHLS : public HLS::HLSStreamerCallback
{
public:
	TestHLS() : m_streamer(this){
	
	}

	void testUrl(std::string& url)
	{
		int res = 0;
		if ((res = m_streamer.open(url)) != HLS::STATUS_OK)
		{
			std::cout << "Unable to open " << url << "Reason : " << res; 
			return;
		}
		std::cout << "HLSType: " << (m_streamer.hlsType() == HLS::HLS_TYPE_LIVE) ? "Live" : "VOD";
	 }

	 void onVideoSetup(uint8_t* data, int size, HLS::VideoCodec coodec, int width, int height) override
	 {
		 std::cout << "onVideoSetup : width " << width << " - height: " << height << std::endl;

	 }
	  void onAudioSetup(uint8_t* data, int size, HLS::AudioCodec codec, int sampleRate, int channelCount) override
	 {
		 std::cout << "onVideoSetup : sampleRate " << sampleRate << " - channelCount: " << channelCount << std::endl;

	 }
	 
	 void onDuration(double durationSec) override
	 {
		 std::cout << "On duration : " << durationSec << std::endl;
	 }
	 
	 void onPacketReady(HLS::StreamType streamType, uint8_t* data, int size) override
	 {
		 std::cout << "On packet ready - Stream idx: " << streamType << " Size: " << size << std::endl;
	 }

	 virtual void onSegmentConsumed(int id) override
	 {
		 m_streamer.onConsumed(id);
	 }

private:
	HLS::HLSStreamer m_streamer;
};


int main()
{
	std::string url("https://cph-p2p-msl.akamaized.net/hls/live/2000341/test/master.m3u8");
	//std::string url("https://demo.unified-streaming.com/k8s/features/stable/video/tears-of-steel/tears-of-steel.mp4/.m3u8");
	//std::string url = "https://cph-p2p-msl.akamaized.net/hls/live/2000341/test/master.m3u8";
	TestHLS testHls;
	testHls.testUrl(url);

	std::cin.get();
	std::cout << "End";
}