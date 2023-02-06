#include <jni.h>
#include <string>
#include "HLSStreamer.h"

class HLSStreamImp : public HLS::HLSStreamerCallback{
public:
    HLSStreamImp(jobject jObj) : m_javaObj(jObj), m_streamer(this), m_isLiveType(false){}
    ~HLSStreamImp() {
       JNIEnv *env = getJniEnv();
       if (m_javaObj) {
           env->DeleteGlobalRef(m_javaObj);
           m_javaObj = nullptr;
       }
    }

    int open(std::string& url)
    {
        return m_streamer.open(url);
    }

    bool isLiveType()
    {
        return m_streamer.hlsType() == HLS::HLS_TYPE_LIVE;
    }

    double getDuration()
    {
        return m_duration;
    }

    void onVideoSetup(uint8_t *data, int size, HLS::VideoCodec codec, int width, int height,
                      float frameRate) override
    {
        JNIEnv *env = getJniEnv();
        jclass  jcls = env->GetObjectClass(m_javaObj);
        jmethodID  mid = env->GetMethodID(jcls, "onVideoSetup","([BIIIF)V" );
        if (data) {
            jbyteArray bytes = env->NewByteArray(size);
            env->SetByteArrayRegion(bytes, 0, size, (jbyte *) data);
            env->CallVoidMethod(m_javaObj, mid, bytes, codec, width, height, frameRate);
            env->ReleaseByteArrayElements(bytes, (jbyte *) data, 0);
        }
        else
        {
            env->CallVoidMethod(m_javaObj, mid, nullptr, codec, width, height, frameRate);
        }
    }

    void onAudioSetup(uint8_t* data, int size, HLS::AudioCodec codec, int sampleRate, int channelCount) override
    {
        JNIEnv *env = getJniEnv();
        jclass  jcls = env->GetObjectClass(m_javaObj);
        jmethodID  mid = env->GetMethodID(jcls, "onAudioSetup","([BIII)V" );
        if (data) {
            jbyteArray bytes = env->NewByteArray(size);
            env->SetByteArrayRegion(bytes, 0, size, (jbyte *) data);
            env->CallVoidMethod(m_javaObj, mid, bytes, codec, sampleRate, channelCount);
            env->ReleaseByteArrayElements(bytes, (jbyte*)data, 0);
        }
        else
        {
            env->CallVoidMethod(m_javaObj, mid, nullptr, codec, sampleRate, channelCount);
        }
    }

    void onDuration(double durationSec) override
    {
        m_duration = durationSec;
    }

    void onPacketReady(HLS::StreamType streamType, uint8_t *data, int size, int64_t pts) override
    {
        JNIEnv *env = getJniEnv();
        jclass  jcls = env->GetObjectClass(m_javaObj);
        jbyteArray bytes = env->NewByteArray(size);
        env->SetByteArrayRegion(bytes, 0, size, (jbyte*)data);
        jmethodID mid = nullptr;
        if (streamType == HLS::STREAM_VIDEO)
        {
            mid = env->GetMethodID(jcls, "onVideoPacketReady","([BJ)V" );
        }
        else if (streamType == HLS::STREAM_AUDIO)
        {
            mid = env->GetMethodID(jcls, "onAudioPacketReady","([BJ)V" );
        }
        if (mid)
        {
            env->CallVoidMethod(m_javaObj, mid, bytes, pts);
        }
        env->DeleteLocalRef(bytes);
        //env->ReleaseByteArrayElements(bytes, nullptr, 0);
    }

    void onSegmentConsumed(int id) override
    {
        m_streamer.onConsumed(id);
    }

    double getDuration(double durationSec)
    {
        return m_duration;
    }

    static JNIEnv* getJniEnv() {
        if(!g_jvm)
        {
            return nullptr;
        }

        JNIEnv *jniEnv=nullptr;
        int nEnvStat = g_jvm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6);

        if (nEnvStat == JNI_EDETACHED) {
            JavaVMAttachArgs args;
            args.version = JNI_VERSION_1_6; // choose your JNI version
            args.name = nullptr;
            args.group = nullptr;

            if (g_jvm->AttachCurrentThread(&jniEnv, &args) != 0) {
                return nullptr;
            }
        }
        else if (nEnvStat == JNI_EVERSION) {
            return nullptr;
        }
        return jniEnv;
    }

public:
    static JavaVM *g_jvm;

private:
    HLS::HLSStreamer m_streamer;
    jobject m_javaObj;
    double m_duration{};
    bool m_isLiveType;
};

JavaVM *HLSStreamImp::g_jvm = nullptr;

__unused jint JNI_OnLoad(
        JavaVM *__unused vm,
        void *__unused reserved
) {
    HLSStreamImp::g_jvm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_binodrokkad_hlsstream_HLSStreamer_cppInit(JNIEnv *env, jobject thiz) {
    jobject jobj = env->NewGlobalRef(thiz);
    auto *imp = new HLSStreamImp(jobj);
    return (jlong)imp;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_binodrokkad_hlsstream_HLSStreamer_cppLoad(JNIEnv *env, jobject thiz, jlong ptr,jstring playlist_url) {
    auto *hlsStreamImp = (HLSStreamImp*)ptr;
    const char *nativeString = env->GetStringUTFChars(playlist_url, 0);
    std::string url(nativeString);
    int ret = hlsStreamImp->open(url);
    env->ReleaseStringUTFChars(playlist_url, nativeString);
    return ret;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_binodrokkad_hlsstream_HLSStreamer_cppDestroy(JNIEnv *env, jobject thiz, jlong pointer) {
    auto *hlsStreamImp = (HLSStreamImp*)pointer;
    if (hlsStreamImp)
    {
        delete hlsStreamImp;
    }
    return 0;
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_binodrokkad_hlsstream_HLSStreamer_cppIsLiveType(JNIEnv *env, jobject thiz, jlong pointer) {
    auto *hlsStreamImp = (HLSStreamImp*)pointer;
    return hlsStreamImp->isLiveType();
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_binodrokkad_hlsstream_HLSStreamer_cppGetDuration(JNIEnv *env, jobject thiz,
                                                          jlong pointer) {
    auto *hlsStreamImp = (HLSStreamImp*)pointer;
    return hlsStreamImp->getDuration();
}