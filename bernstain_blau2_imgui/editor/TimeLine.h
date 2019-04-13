#pragma once

#include <vector>

#define KF_NUM_VALUES 18

class KeyFrame
{
public:
    KeyFrame(float time);

    float time() const {return time_;}
    void SetTime(float time) {time_ = time;}
    float value(int i) const {if (i < 0 || i >= KF_NUM_VALUES) return -1; return value_[i];}
    float *values() {return &(value_[0]);}
    void SetValue(int index, float value) {if (index >= 0 && index < KF_NUM_VALUES) value_[index] = value;}
    void Save(FILE *fid);
    void Load(FILE *fid);

private:
    float time_;  // between 0 and music_length_
    float value_[KF_NUM_VALUES];  // top-row, then bottom row
};

class TimeLine
{
public:
    TimeLine();
    virtual ~TimeLine();

    void Init(float duration);
    void GetValues(float time, float value[KF_NUM_VALUES]);
    float value(int id, int index) const {
        if (id < 0 || id >= (int)keyframe_.size()) return -1.0f;
        return keyframe_[id].value(index);
    }
    void SetValue(int id, int index, float value) {
        if (id >= 0 && id < (int)keyframe_.size()) keyframe_[id].SetValue(index, value);
    }
    bool AddKeyFrame(int id);  // Adds one keyframe after the specified one
    float time(int id) {if (id < 0 || id >= (int)keyframe_.size()) return -1.0f; return keyframe_[id].time();}
    void SetKeyFrameTime(int id, float time);

    void Save(FILE *fid);
    void Load(FILE *fid);

    int NumKeyFrames(void) {return keyframe_.size();}

private:
    std::vector<KeyFrame> keyframe_;
};

