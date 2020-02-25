#pragma once

#include <math.h>

struct Matrix2x3
{
public:
    Matrix2x3(void);
    virtual ~Matrix2x3();

    void ScaleInit(float scale_x, float scale_y) {
        a_[0][0] = scale_x; a_[0][1] = 0.0f; a_[0][2] = 0.0f;
        a_[1][0] = 0.0f; a_[1][1] = scale_y; a_[1][2] = 0.0f;
    }

    void TranslateInit(float trans_x, float trans_y) {
        a_[0][0] = 1.0f; a_[0][1] = 0.0f; a_[0][2] = trans_x;
        a_[1][0] = 0.0f; a_[1][1] = 1.0f; a_[1][2] = trans_y;
    }

    void RotationInit(float radians) {
        a_[0][0] = cosf(radians); a_[0][1] = sinf(radians); a_[0][2] = 0.0f;
        a_[1][0] = -sinf(radians); a_[1][1] = cosf(radians); a_[1][2] = 0.0f;
    }

    void Set(float radians, float scale_x, float scale_y, float trans_x, float trans_y);

    void Set(float a11, float a12, float a13, float a21, float a22, float a23) {
        a_[0][0] = a11; a_[0][1] = a12; a_[0][2] = a13;
        a_[1][0] = a21; a_[1][1] = a22; a_[1][2] = a23;
    }

    void Multiply(const Matrix2x3 *first, const Matrix2x3 *second);
    float SquareSize(void) const;

public:
    float a_[2][3];
};

struct FunctionDefinition
{
public:
    void CreateMatrix(void) {
        matrix.Set(parameters[0] * 3.1415f, parameters[1], parameters[2], parameters[3], parameters[4]);
    }

    void Set(float p1, float p2, float p3, float p4, float p5) {
        parameters[0] = p1;
        parameters[1] = p2;
        parameters[2] = p3;
        parameters[3] = p4;
        parameters[4] = p5;
        CreateMatrix();
    }

public:
    Matrix2x3 matrix;
    float parameters[5];
};

class Fractal
{
public:
    Fractal();
    virtual ~Fractal();

    // Draw ImGUI window from generated list
    void ImGUIDraw(float min_size);

    // Draw a control window for setting values
    void ImGUIControl(void);

    // Render the fractal to audio and play the wav file
    void Play(float min_size);

public:
    // Functions can be set from outside the class to determine shape
    static constexpr int kNumFunctions = 2;
    FunctionDefinition function_[kNumFunctions];
    float final_rotation_ = 0.0f;
    float final_scale_[2] = {1.0f, 1.0f};
    float final_translation_[2] = {0.0f, 0.0f};

    // Generate IFS fractal in point_ array
    void Generate(float min_size, int max_num_points);

private:
    // Create a line from one point in the fractal. Returns the width of the line
    float GetLine(int index, float *start_x, float *start_y, float *end_x, float *end_y);

private:
    static constexpr int kMaxNumPoints = 100000;
    static constexpr int kNumDrawPoints = 10000;
    Matrix2x3 point_[kMaxNumPoints];
    // The number of points in point_ array that should be considered for drawing
    int num_active_points_ = 0;
    bool draw_point_[kMaxNumPoints];
};

