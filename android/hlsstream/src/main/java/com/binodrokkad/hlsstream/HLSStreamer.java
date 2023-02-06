package com.binodrokkad.hlsstream;

public class HLSStreamer {

    public interface HLSStreamListener {
        void onVideoSetup(byte[] configBytes, int codec, int width, int height, float frameRate);
        void onAudioSetup(byte[] configBytes, int codec, int sampleRate, int channelCount);
        void onVideoPacketReady(byte[] bytes, long pts);
        void onAudioPacketReady(byte[] bytes, long pts);
    }

    private long m_cppPointer = 0;
    private final HLSStreamListener m_listener;

    public HLSStreamer(HLSStreamListener listener){
        this.m_listener = listener;
    }

    public void destroy ()
    {
        cppDestroy(m_cppPointer);
        m_cppPointer = 0;
    }

    public int load(String playlistUrl) {
        m_cppPointer = cppInit();
        if (m_cppPointer == 0)
        {
            return -1;
        }
        return cppLoad(m_cppPointer, playlistUrl);
    }

    public boolean isLiveType()
    {
        if (m_cppPointer == 0)
        {
            return false;
        }
        return cppIsLiveType(m_cppPointer);
    }

    public double getDuration()
    {
        if (m_cppPointer == 0)
        {
            return 0.0;
        }
        return cppGetDuration(m_cppPointer);
    }

    public void onVideoSetup(byte[] configBytes, int codec, int width, int height, float frameRate) {
        m_listener.onVideoSetup(configBytes, codec, width, height, frameRate);
    }

    public void onAudioSetup(byte[] configBytes, int codec, int sampleRate, int channelCount) {
        m_listener.onAudioSetup(configBytes, codec, sampleRate, channelCount);
    }

    void onVideoPacketReady(byte[] bytes, long pts) {
        m_listener.onVideoPacketReady(bytes, pts);
    }

    void onAudioPacketReady(byte[] bytes, long pts) {
        m_listener.onAudioPacketReady(bytes, pts);
    }

    private native long cppInit();
    private native long cppDestroy(long pointer);
    private native int cppLoad(long pointer, String playlistUrl);
    private native boolean cppIsLiveType(long pointer);
    private native double cppGetDuration(long pointer);

    static {
        System.loadLibrary("hlsstream");
    }
}