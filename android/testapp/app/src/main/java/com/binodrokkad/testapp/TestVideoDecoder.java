package com.binodrokkad.testapp;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

public class TestVideoDecoder {

    private static final long DEFAULT_TIMEOUT_US = 1000000;
    private MediaCodec m_mediaCodec;

    public TestVideoDecoder() {

    }

    public void init(Surface surface, byte[] conf, int width, int height)
    {
        if (surface == null)
        {
            return;
        }
        MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
        try {
            m_mediaCodec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
            m_mediaCodec.configure(format, surface, null, 0);
            m_mediaCodec.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void release() {
        if (m_mediaCodec == null)
        {return;}

        m_mediaCodec.stop();
        m_mediaCodec.release();
    }

    public void decode(byte[] input, long pts) {
        if (m_mediaCodec == null)
        {
            return;
        }
        int inputBufferIndex = m_mediaCodec.dequeueInputBuffer(DEFAULT_TIMEOUT_US);
        if (inputBufferIndex >= 0) {
            ByteBuffer inputBuffer = m_mediaCodec.getInputBuffer(inputBufferIndex);
            inputBuffer.clear();
            inputBuffer.put(input, 0, input.length);
            m_mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, pts, 0);
        }

        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        int outputBufferIndex = m_mediaCodec.dequeueOutputBuffer(bufferInfo, DEFAULT_TIMEOUT_US);

        if (outputBufferIndex >= 0) {
            m_mediaCodec.releaseOutputBuffer(outputBufferIndex, true);
        }
    }

}
