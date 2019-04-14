#include "TimeLine.h"

KeyFrame::KeyFrame(float time)
{
    time_ = time;

    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        value_[i] = 0.5f;
    }
}

void KeyFrame::Load(FILE *fid)
{
    fscanf(fid, "%f\t%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
        &time_,
        &value_[0], &value_[1], &value_[2],
        &value_[3], &value_[4], &value_[5],
        &value_[6], &value_[7], &value_[8],
        &value_[9], &value_[10], &value_[11],
        &value_[12], &value_[13], &value_[14],
        &value_[15], &value_[16], &value_[17]);
}

void KeyFrame::Save(FILE *fid)
{
    fprintf(fid, "%.2f\t", time_);
    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        if (i != 0) fprintf(fid, " ");
        fprintf(fid, "%.5f", value_[i]);
    }
    fprintf(fid, "\n");
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

void TimeLine::Save(FILE *fid)
{
    fprintf(fid, "%d\n", (int)keyframe_.size());
    for (int i = 0; i < (int)keyframe_.size(); i++)
    {
        keyframe_[i].Save(fid);
    }
}

void TimeLine::Load(FILE *fid)
{
    int count = 0;
    fscanf(fid, "%d\n", &count);
    keyframe_.clear();
    for (int i = 0; i < count; i++)
    {
        keyframe_.push_back(KeyFrame(0.0f));
        keyframe_[i].Load(fid);
    }
}

static float smooth(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return 0.5f - 0.5f * cos(t * 3.12425f);
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
    int pp = start_index - 1;
    if (pp < 0) pp = 0;  // keyframe_.size() - 1;
    int n = start_index + 1;
    int nn = start_index + 2;
    if (nn >= (int)keyframe_.size()) nn = (int)keyframe_.size() - 1;  // nn = 0;
    for (int i = 0; i < KF_NUM_VALUES; i++)
    {
        //value[i] = (1.0f - smooth(t)) * keyframe_[start_index].value(i) +
        //    smooth(t) * keyframe_[start_index + 1].value(i);
        float k = 0.75f;
        float d = (1.0f - k) / 2.0f;
        value[i] =
            smooth(d - k * t) * keyframe_[pp].value(i) +
            smooth(1.0f - d - k * t) * keyframe_[start_index].value(i) +
            smooth(d + k * t) * keyframe_[n].value(i) +
            smooth(-k + d + k * t) * keyframe_[nn].value(i);
        float amount_sum = smooth(d - k * t) + smooth(1.0f - d - k * t) + smooth(d + k * t) + smooth(-k + d + k * t);
        value[i] /= amount_sum;
    }
}

bool TimeLine::DeleteKeyFrame(int id)
{
    if (id <= 0 || id >= keyframe_.size() - 1)
    {
        return false;
    }

    // Copy everything after id
    for (int i = id; i < keyframe_.size() - 1; i++)
    {
        // I hope the values are copied?
        keyframe_[i] = keyframe_[i + 1];
    }

    keyframe_.pop_back();
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
    if (id <= 0 || id >= (int)keyframe_.size() - 1) return;

    float min = keyframe_[id - 1].time() + 0.01f;
    float max = keyframe_[id + 1].time() - 0.01f;
    if (time < min) time = min;
    if (time > max) time = max;
    keyframe_[id].SetTime(time);
}