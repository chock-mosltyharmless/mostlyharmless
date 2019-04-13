#include "TimeLine.h"

KeyFrame::KeyFrame(float time)
{
    time_ = time;

    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        value_[i] = 0.5f;
    }
}

TimeLine::TimeLine()
{
}

TimeLine::~TimeLine()
{
}

void TimeLine::Init(float duration)
{
    keyframe_.push_back(KeyFrame(0.0f));
    keyframe_.push_back(KeyFrame(duration));
}

void TimeLine::GetValues(float time, float value[KF_NUM_VALUES])
{
    int start_index = 0;
    while (start_index < (int)(keyframe_.size()) - 1 &&
           keyframe_[start_index + 1].time() < time)
    {
        start_index++;
    }

    float t = (time - keyframe_[start_index].time()) /
        (keyframe_[start_index + 1].time() - keyframe_[start_index].time() + 0.00001f);

    // Here I want higher order approximation.
    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        value[i] = (1.0f - t) * keyframe_[start_index].value(i) +
            t * keyframe_[start_index + 1].value(i);
    }
}

bool TimeLine::AddKeyFrame(int id)
{
    if (id == keyframe_.size() - 1)
    {
        // rather insert it before;
        return AddKeyFrame(id - 1);
    }

    if (id < 0 || id >= (int)(keyframe_.size()) - 1)
    {
        // Either a bug or trying to append after last.
        return false;
    }

    // At first, push one to the back
    keyframe_.push_back(KeyFrame(1.0));

    // Copy everything after id
    for (int i = (int)(keyframe_.size()) - 1; i > id; i--)
    {
        // I hope the values are copied?
        keyframe_[i] = keyframe_[i - 1];
    }

    // Create an interpolated keyframe
    float t2 = keyframe_[id + 2].time();
    float t1 = keyframe_[id].time();
    keyframe_[id + 1].SetTime(0.5f * (t1 + t2));
    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        float v2 = keyframe_[id + 2].value(i);
        float v1 = keyframe_[id].value(i);
        keyframe_[id + 1].SetValue(i, 0.5f * (v1 + v2));
    }

    return true;
}

void TimeLine::SetKeyFrameTime(int id, float time)
{
    if (id <= 0 || id >= keyframe_.size() - 1) return;

    float min = keyframe_[id - 1].time() + 0.01f;
    float max = keyframe_[id + 1].time() - 0.01f;
    if (time < min) time = min;
    if (time > max) time = max;
    keyframe_[id].SetTime(time);
}