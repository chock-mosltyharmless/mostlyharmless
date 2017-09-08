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

void Thread::Draw(float red, float green, float blue, float width) {
    float along[2];
    along[0] = screen_location_[1][0] - screen_location_[0][0];
    along[1] = screen_location_[1][1] - screen_location_[0][1];
    float length = sqrtf(along[0] * along[0] + along[1] * along[1]);
    along[0] /= length;
    along[1] /= length;
    along[0] *= width;
    along[1] *= width;
    float normal[2];
    normal[0] = -along[1];
    normal[1] = along[0];

    // left end triangle
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[0][0] - along[0] - normal[0],
               screen_location_[0][1] - along[1] - normal[1]);
    glVertex2f(screen_location_[0][0] - along[0] + normal[0],
               screen_location_[0][1] - along[1] + normal[1]);
    glColor3f(red, green, blue);
    glVertex2f(screen_location_[0][0], screen_location_[0][1]);

    // right end triangle
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1]);
    glVertex2f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1]);
    glColor3f(red, green, blue);
    glVertex2f(screen_location_[1][0], screen_location_[1][1]);

    // Bottom rect
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[0][0] - along[0] + normal[0],
        screen_location_[0][1] - along[1] + normal[1]);
    glVertex2f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1]);
    glColor3f(red, green, blue);
    glVertex2f(screen_location_[0][0], screen_location_[0][1]);

    glVertex2f(screen_location_[1][0], screen_location_[1][1]);
    glVertex2f(screen_location_[0][0], screen_location_[0][1]);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[1][0] + along[0] + normal[0],
        screen_location_[1][1] + along[1] + normal[1]);

    // Top rect
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[0][0] - along[0] - normal[0],
        screen_location_[0][1] - along[1] - normal[1]);
    glVertex2f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1]);
    glColor3f(red, green, blue);
    glVertex2f(screen_location_[0][0], screen_location_[0][1]);

    glVertex2f(screen_location_[1][0], screen_location_[1][1]);
    glVertex2f(screen_location_[0][0], screen_location_[0][1]);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screen_location_[1][0] + along[0] - normal[0],
        screen_location_[1][1] + along[1] - normal[1]);

}

int Thread::GetMaxNumReferencePoints(void) {
    return static_cast<int>(sizeof(kReferencePointLocation) /
        sizeof(kReferencePointLocation[0]));
}

bool Thread::SetData(float x1, float y1, int thread_index1, float x2, float y2, int thread_index2) {
    if (thread_index1 < 0 || thread_index1 >= GetMaxNumReferencePoints()) return false;
    if (thread_index2 < 0 || thread_index2 >= GetMaxNumReferencePoints()) return false;

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

void ThreadInformation::Draw(float red, float green, float blue, float width) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLES);
    int num_threads = GetNumThreads();
    for (int i = 0; i < num_threads; i++) {
        thread_container_[i].Draw(red, green, blue, width);
    }
    glEnd();
}
