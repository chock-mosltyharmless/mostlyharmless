#pragma once

class Thread {
public:
    Thread();
    ~Thread();

    // Returns true if data could be set.
    bool SetData(float x1, float y1, int thread_index1,
                 float x2, float y2, int thread_index2);

    // Access Reference point locations
    float refernce_point_1_x_(void) {
        return static_cast<float>(kReferencePointLocation[reference_point_index_[0]][0]);
    }
    float refernce_point_1_y_(void) {
        return static_cast<float>(kReferencePointLocation[reference_point_index_[0]][1]);
    }
    float refernce_point_2_x_(void) {
        return static_cast<float>(kReferencePointLocation[reference_point_index_[1]][0]);
    }
    float refernce_point_2_y_(void) {
        return static_cast<float>(kReferencePointLocation[reference_point_index_[1]][1]);
    }

    // Assumes additive drawing with glBegin(TRIANGLES)
    void Draw(float red, float green, float blue, float width);

    int GetMaxNumReferencePoints(void);

    // x, y in screen space, each in range [-1..1]
    float screen_location_[2][2];

    // First and second reference point index, ordered as screen_location_
    int reference_point_index_[2];

private:
    // First coordinate is from front, second is from top
    static const double kReferencePointLocation[][2];
};

class ThreadInformation {
public:
    ThreadInformation();
    virtual ~ThreadInformation();
    
    // Retrieve the number of threads
    int GetNumThreads() {
        return static_cast<int>(thread_container_.size());
    }

    // Get screen space location of threads
    // Does not check whether the index is valid [may result in memory error]
    float GetScreenSpaceX1(int thread_index) {
        return thread_container_[thread_index].screen_location_[0][0];
    }
    float GetScreenSpaceY1(int thread_index) {
        return thread_container_[thread_index].screen_location_[0][1];
    }
    float GetScreenSpaceX2(int thread_index) {
        return thread_container_[thread_index].screen_location_[1][0];
    }
    float GetScreenSpaceY2(int thread_index) {
        return thread_container_[thread_index].screen_location_[1][1];
    }

    // Adds one thread. May return false on error.
    // x1, y1, x2, y2: Screen space locations of thread ends in range [-1..1]
    // thread_index1, thread_index2: 
    bool AddThread(float x1, float y1, int thread_index1,
                   float x2, float y2, int thread_index2);

    void Draw(float red, float green, float blue, float width);

private:
    // First coordinate is from front, second is from top
    static const double kReferencePointLocation[][2];

    // The actual threads
    std::vector<Thread> thread_container_;
};

