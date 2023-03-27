#ifndef WaitCond_H
#define WaitCond_H

#include <mutex>
#include <condition_variable>

namespace HLS
{
    class WaitCond
    {
    public:
        WaitCond(bool in_autoReset = true)
            : m_autoReset(in_autoReset),
              m_condIsTrue(false)
        {
        }

        void signal()
        {
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                m_condIsTrue = true;
            }
            m_conditionVar.notify_one();
        }

        bool isTrue()
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            return m_condIsTrue;
        }

        void wait()
        {
            std::unique_lock<std::mutex> uLock(m_mutex);
            m_conditionVar.wait(uLock, [&]
                                { return m_condIsTrue; });
            if (m_autoReset)
                m_condIsTrue = false;
        }

        bool timedWait(long in_timeoutMs)
        {
            std::chrono::milliseconds timeoutPeriod(in_timeoutMs);
            std::unique_lock<std::mutex> uLock(m_mutex);
            if (m_conditionVar.wait_for(uLock, timeoutPeriod,
                                        [&]
                                        { return m_condIsTrue; }))
            {
                if (m_autoReset)
                    m_condIsTrue = false;
                return true;
            }
            else // timeout
            {
                return false;
            }
        }

    private:
        bool m_condIsTrue;

        std::mutex m_mutex;

        std::condition_variable m_conditionVar;

        bool m_autoReset;
    };
}
#endif // !
