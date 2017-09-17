#include "stdafx.h"
#include "ThreadInformation.h"

Thread::Thread(void) {
    screen_location_[0][0] = -1.0f;
    screen_location_[0][1] = -0.5f;
    screen_location_[1][0] = 1.0f;
    screen_location_[1][1] = 0.5f;
    reference_point_index_[0] = 0;
    reference_point_index_[1] = 50;
}

Thread::~Thread() {
}

const double Thread::kReferencePointLocation[][2] = {
    {3.9, 5.4},    // 0
    {10.4, 3.4},   // 1
    {17.0, 5.3},   // 2
    {20.2, 4.9},   // 3
    {26.4, 7.0},   // 4
    {29.9, 4.3},   // 5
    {13.1, 10.0},  // 6
    {29.8, 9.8},   // 7
    {5.0, 13.1},   // 8
    {10.2, 13.9},  // 9
    {18.7, 12.5},  // 10
    {25.2, 11.6},  // 11
    {32.4, 13.2},  // 12
    {3.4, 16.3},   // 13
    {4.5, 21.2},   // 14
    {13.4, 19.9},  // 15
    {25.3, 18.2},  // 16
    {20.1, 21.8},  // 17
    {31.2, 20.4},  // 18
    {9.7, 24.3},   // 19
    {11.1, 29.4},  // 20
    {11.1, 29.4},  // 21
    {15.4, 27.7},  // 22
    {19.0, 27.0},  // 23
    {27.0, 27.5},  // 24
    {24.5, 29.9},  // 25
    {29.9, 31.8},  // 26
    {6.6, 35.0},   // 27
    {11.7, 37.0},  // 28
    {19.9, 36.1},  // 29
    {27.8, 38.8},  // 30
    {},  // 31
    {},  // 32
    {},  // 33
    {},  // 34
    {},  // 35
    {},  // 36
    {},  // 37
    {},  // 38
    {},  // 39
    {},  // 40
    {},  // 41
    {},  // 42
    {},  // 43
    {},  // 44
    {},  // 45
    {},  // 46
    {},  // 47
    {},  // 48
    {},  // 49
    {30.2, 5.3},   // 50
    {24.6, 2.8},   // 51
    {16.5, 5.6},   // 52
    {11.0, 4.5},   // 53
    {4.5, 3.3},    // 54
    {32.9, 9.8},   // 55
    {23.2, 11.0},  // 56
    {14.3, 13.1},  // 57
    {7.7, 9.8},    // 58
    {2.4, 10.5},   // 59
    {29.2, 17.8},  // 60
    {23.3, 19.2},  // 61
    {17.8, 15.8},  // 62
    {7.9, 15.1},   // 63
    {2.9, 18.2},   // 64
    {14.2, 20.0},  // 65
    {26.1, 23.2},  // 66
    {19.8, 25.4},  // 67
    {12.1, 25.5},  // 68
    {9.1, 22.5},   // 69
    {3.8, 25.8},   // 70
    {33.7, 26.8},  // 71
    {30.2, 30.6},  // 72
    {23.8, 31.9},  // 73
    {19.2, 34.5},  // 74
    {9.5, 32.0},   // 75
    {5.1, 34.5},   // 76
    {28.3, 36.0},  // 77
    {32.3, 39.2}  // 78
};

void Thread::Draw(float red, float green, float blue, float width,
                  bool invisible) {
    float depth1 = 0.0f;
    float depth2 = 0.0f;
    float alpha = 1.0f;

    if (reference_point_index_[0] >= 0) {
        depth1 = 1.0f - static_cast<float>(
            kReferencePointLocation[reference_point_index_[0]][1] * kDepthScaling);
    }
    if (reference_point_index_[1] >= 0) {
        depth2 = 1.0f - static_cast<float>(
            kReferencePointLocation[reference_point_index_[1]][1] * kDepthScaling);
    }
    if (invisible) {
        constexpr float kDepthBias = 1e-4f;
        depth1 += kDepthBias;
        depth2 += kDepthBias;
        red = green = blue = alpha = 0.0f;
    }

    float along[2];
    along[0] = screen_location_[1][0] - screen_location_[0][0];
    along[1] = screen_location_[1][1] - screen_location_[0][1];
    float length = sqrtf(along[0] * along[0] + along[1] * along[1]);
    if (length > 1e-7f) {
        along[0] /= length;
        along[1] /= length;
        along[0] *= width;
        along[1] *= width;
    } else {
        along[0] = width;
        along[1] = 0.0f;
    }
    float normal[2];
    normal[0] = -along[1];
    normal[1] = along[0];

    // left end triangle
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[0][0] - along[0] - normal[0],
               screen_location_[0][1] - along[1] - normal[1], depth1);
    glVertex3f(screen_location_[0][0] - along[0] + normal[0],
               screen_location_[0][1] - along[1] + normal[1], depth1);
    glColor4f(red, green, blue, alpha);
    glVertex3f(screen_location_[0][0], screen_location_[0][1], depth1);

    // right end triangle
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1], depth2);
    glVertex3f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1], depth2);
    glColor4f(red, green, blue, alpha);
    glVertex3f(screen_location_[1][0], screen_location_[1][1], depth2);

    // Bottom rect
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[0][0] - along[0] + normal[0],
        screen_location_[0][1] - along[1] + normal[1], depth1);
    glVertex3f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1], depth2);
    glColor4f(red, green, blue, alpha);
    glVertex3f(screen_location_[0][0], screen_location_[0][1], depth1);

    glVertex3f(screen_location_[1][0], screen_location_[1][1], depth2);
    glVertex3f(screen_location_[0][0], screen_location_[0][1], depth1);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1], depth2);

    // Top rect
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[0][0] - along[0] - normal[0],
        screen_location_[0][1] - along[1] - normal[1], depth1);
    glVertex3f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1], depth2);
    glColor4f(red, green, blue, alpha);
    glVertex3f(screen_location_[0][0], screen_location_[0][1], depth1);

    glVertex3f(screen_location_[1][0], screen_location_[1][1], depth2);
    glVertex3f(screen_location_[0][0], screen_location_[0][1], depth1);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1], depth2);
}

int Thread::GetMaxNumReferencePoints(void) {
    return static_cast<int>(sizeof(kReferencePointLocation) /
        sizeof(kReferencePointLocation[0]));
}

bool Thread::Load(FILE * fid) {
    int file_version;
    fread(&file_version, sizeof(int), 1, fid);

    if (file_version != 1) return false;

    fread(screen_location_[0], sizeof(float), 4, fid);
    fread(reference_point_index_, sizeof(int), 2, fid);

    return true;
}

bool Thread::Save(FILE * fid) {
    const int kFileVersion = 1;

    // Rudementary header
    fwrite(&kFileVersion, sizeof(int), 1, fid);

    // Data
    fwrite(screen_location_[0], sizeof(float), 4, fid);
    fwrite(reference_point_index_, sizeof(int), 2, fid);

    return true;
}

bool Thread::SetData(float x1, float y1, int thread_index1, float x2, float y2, int thread_index2) {
    if (thread_index1 >= GetMaxNumReferencePoints()) return false;
    if (thread_index2 >= GetMaxNumReferencePoints()) return false;

    // Sort thread index according to x, making left-right correct
    if (x1 < x2 && thread_index1 < thread_index2) {
        int temp = thread_index2;
        thread_index2 = thread_index1;
        thread_index1 = temp;
    }

    screen_location_[0][0] = x1;
    screen_location_[0][1] = y1;
    screen_location_[1][0] = x2;
    screen_location_[1][1] = y2;
    reference_point_index_[0] = thread_index1;
    reference_point_index_[1] = thread_index2;

    return true;
}


ThreadInformation::ThreadInformation() {
}

ThreadInformation::~ThreadInformation() {
}

bool ThreadInformation::AddThread(float x1, float y1, int thread_index1, float x2, float y2, int thread_index2)
{
    Thread thread;
    if (!thread.SetData(x1, y1, thread_index1, x2, y2, thread_index2)) return false;

    thread_container_.push_back(thread);
    return true;
}

void ThreadInformation::DrawDepthMask(float width) {
    glDepthMask(true);
    
    int num_threads = GetNumThreads();
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < num_threads; i++) {
        thread_container_[i].Draw(0.0f, 0.0f, 0.0f, width, true);
    }
    glEnd();

    glDepthMask(false);
}

void ThreadInformation::Draw(float red, float green, float blue, float width) {    
    int num_threads = GetNumThreads();
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < num_threads; i++) {
        thread_container_[i].Draw(red, green, blue, width, false);
    }
    glEnd();
}

bool ThreadInformation::Load(FILE *fid) {    
    thread_container_.clear();

    int file_version;
    fread(&file_version, sizeof(int), 1, fid);
    if (file_version != 1) return false;
    
    int num_threads;
    fread(&num_threads, sizeof(int), 1, fid);

    for (int i = 0; i < num_threads; i++) {
        Thread load_thread;
        if (!load_thread.Load(fid)) {
            thread_container_.clear();
            return false;
        }
        thread_container_.push_back(load_thread);
    }

    return true;
}

bool ThreadInformation::Save(FILE *fid) {
    const int kFileVersion = 1;
    
    // Rudementary header
    fwrite(&kFileVersion, sizeof(int), 1, fid);

    // Data
    int num_threads = GetNumThreads();
    fwrite(&num_threads, sizeof(int), 1, fid);

    for (int i = 0; i < num_threads; i++) {
        if (!thread_container_[i].Save(fid)) return false;
    }

    return true;
}
