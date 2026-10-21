#pragma once
#include <deque>

namespace Fast {
// A time window, not a fixed frame count: low targets must not retain many
// seconds of samples from the previous target. Samples come from completed swaps.
class PresentationRate {
  public:
    void Record(double now, int target, int mode, bool vsync) {
        if (target != mTarget || mode != mMode || vsync != mVsync ||
            (!mTimes.empty() && now <= mTimes.back())) {
            mTimes.clear();
        }
        mTarget = target;
        mMode = mode;
        mVsync = vsync;
        mTimes.push_back(now);
        while (mTimes.size() > 2 && mTimes[1] <= now - 1.0) mTimes.pop_front();
    }
    double Rate(int target, int mode, bool vsync) const {
        if (target != mTarget || mode != mMode || vsync != mVsync || mTimes.size() < 2) return 0;
        return (mTimes.size() - 1) / (mTimes.back() - mTimes.front());
    }
  private:
    std::deque<double> mTimes;
    int mTarget = 0, mMode = -2;
    bool mVsync = false;
};
inline PresentationRate graphicsPresentationRate;
}
