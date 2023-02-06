package com.binodrokkad.testapp;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

import com.binodrokkad.hlsstream.HLSStreamer;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private final String TAG = "MainActivity";

    private HLSStreamer m_streamer;
    private Surface m_surface;
    private VideoThread m_videoThread;
    private boolean m_isLoaded = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);

        surfaceView.getHolder().addCallback(this);
        m_videoThread = new VideoThread();
        m_videoThread.start();
        m_streamer = new HLSStreamer(new HLSStreamer.HLSStreamListener() {
            @Override
            public void onVideoSetup(byte[] configBytes, int codec, int width, int height, float frameRate) {
                Log.d(TAG, "onVideoSetup");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        int mainWidth = getWindow().getDecorView().getWidth();
                        ViewGroup.LayoutParams layoutParams = surfaceView.getLayoutParams();
                        double aRatio = width / (double)height;
                        layoutParams.height = (int) (mainWidth / aRatio);
                        layoutParams.width = mainWidth;
                        surfaceView.setLayoutParams(layoutParams);
                        surfaceView.requestLayout();
                    }
                });
                m_videoThread.setParams(width, height, frameRate);
                m_videoThread.init();
            }

            @Override
            public void onAudioSetup(byte[] configBytes, int codec, int sampleRate, int channelCount) {
                Log.d(TAG, "onAudioSetup");
            }

            @Override
            public void onVideoPacketReady(byte[] bytes, long pts) {
                Log.d(TAG, "onVideoPacketReady " + pts);
                m_videoThread.decode(bytes, pts);
            }

            @Override
            public void onAudioPacketReady(byte[] bytes, long pts) {
                Log.d(TAG, "onAudioPacketReady");
            }
        });
    }

    public void onLoadClicked(View view) {
        Button button = (Button)view;
        EditText editText = findViewById(R.id.url_txt);
        if (m_isLoaded)
        {
            m_streamer.destroy();
            m_videoThread.release();
            m_isLoaded = false;
        }
        else {
            int result = m_streamer.load(editText.getText().toString());
            if (result != 0) {
                Log.d(TAG, "Unable to load url");
                m_isLoaded = false;
            } else {
                m_isLoaded = true;
            }
        }
        if (m_isLoaded) {
            button.setText("STOP");
        } else {
            button.setText("LOAD");
        }
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        m_streamer.destroy();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
        m_surface = surfaceHolder.getSurface();
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }

    enum ThreadMessageType {
        INIT,
        DECODE,
        DESTROY,
    }

    public class ThreadMessage {
        public ThreadMessageType messageType;
        public byte[] m_bytes;
        public ArrayList<Double> otherData = new ArrayList<>();
    }

    public class VideoThread extends Thread
    {
        private TestVideoDecoder m_videoDecoder;
        private Queue<ThreadMessage> m_threadMessages;

        int m_width, m_height;
        float m_frameRate = 30.0f;

        public VideoThread(){
            m_threadMessages = new LinkedList<>();
        }

        public void setParams(int width, int height, float frameRate) {
            this.m_width = width;
            this.m_height = height;
            this.m_frameRate = Math.max(frameRate, 1);
        }

        public void init()
        {
            ThreadMessage message= new ThreadMessage();
            message.messageType =ThreadMessageType.INIT;
            message.otherData.add((double) m_width);
            message.otherData.add((double) m_height);
            message.otherData.add((double) m_frameRate);
            m_threadMessages.add(message);
        }

        public void decode(byte[] bytes, long pts)
        {
            ThreadMessage message= new ThreadMessage();
            message.messageType =ThreadMessageType.DECODE;
            message.m_bytes = bytes;
            message.otherData.add((double) pts);
            m_threadMessages.add(message);
        }

        public void release() {
            m_threadMessages.clear();
            ThreadMessage message= new ThreadMessage();
            message.messageType =ThreadMessageType.DESTROY;
            m_threadMessages.add(message);
        }

        @Override
        public void run() {
            super.run();
            m_videoDecoder = new TestVideoDecoder();
            while (!Thread.interrupted())
            {
                if (m_threadMessages.isEmpty())
                {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    continue;
                }
                ThreadMessage message = m_threadMessages.poll();
                switch (message.messageType)
                {
                    case INIT:
                    {
                        List<Double> otherData =  message.otherData;
                        m_videoDecoder.init(m_surface, message.m_bytes,
                                otherData.get(0).intValue(), otherData.get(1).intValue());
                    }
                        break;
                    case DECODE:
                    {
                        List<Double> otherData =  message.otherData;
                        m_videoDecoder.decode(message.m_bytes, otherData.get(0).longValue());
                    }
                        break;
                    case DESTROY:
                    {
                        m_videoDecoder.release();
                    }
                        break;
                }
                try {
                    int ms = (int) (1000 / m_frameRate);
                    int ns = (int) (1000 / m_frameRate - ms);
                    Thread.sleep(ms, ns);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

    }
}