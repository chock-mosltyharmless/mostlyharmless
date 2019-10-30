#pragma once

struct Matrix2x3
{
public:
    Matrix2x3(void);
    virtual ~Matrix2x3();

    void Set(float a11, float a12, float a13, float a21, float a22, float a23) {
        a_[0][0] = a11; a_[0][1] = a12; a_[0][2] = a13;
        a_[1][0] = a21; a_[1][1] = a22; a_[1][2] = a23;
    }

    void Multiply(const Matrix2x3 *first, const Matrix2x3 *second);
    float SquareSize(void) const;

public:
    float a_[2][3];
};

class Fractal
{
public:
    Fractal();
    virtual ~Fractal();

    // Draw ImGUI window from generated list
    void ImGUIDraw(void);

    // Draw a control window for setting values
    void ImGUIControl(void);

    // Generate IFS fractal in point_ array
    void Generate(float min_size);

    void Play(void);

public:
    // Functions can be set from outside the class to determine shape
    static constexpr int kNumFunctions = 2;
    Matrix2x3 function_[kNumFunctions];
    float final_rotation_ = 0.0f;
    float final_scale_[2] = {1.0f, 1.0f};
    float final_translation_[2] = {0.0f, 0.0f};

private:
    static constexpr int kMaxNumPoints = 10000;
    Matrix2x3 point_[kMaxNumPoints];
    // The number of points in point_ array that should be considered for drawing
    int num_active_points_ = 0;
    bool draw_point_[kMaxNumPoints];
};

