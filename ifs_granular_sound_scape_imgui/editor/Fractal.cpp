#include "Fractal.h"

#include <math.h>
#include <memory.h>

Matrix2x3::Matrix2x3(void)
{
    memset(a_, 0, sizeof(a_));
    a_[0][0] = 1.0f;
    a_[1][1] = 1.0f;
}

Matrix2x3::~Matrix2x3()
{
}

void Matrix2x3::Multiply(const Matrix2x3 *first, const Matrix2x3 *second)
{
#if 0
    a_[0][0] = first->a_[0][0] * second->a_[0][0] + first->a_[0][1] * second->a_[1][0];
    a_[0][1] = first->a_[0][0] * second->a_[0][1] + first->a_[0][1] * second->a_[1][1];
    a_[0][2] = first->a_[0][0] * second->a_[0][2] + first->a_[0][1] * second->a_[1][2] + first->a_[0][2];
    a_[1][0] = first->a_[1][0] * second->a_[1][0] + first->a_[1][1] * second->a_[1][0];
    a_[1][1] = first->a_[1][0] * second->a_[1][1] + first->a_[1][1] * second->a_[1][1];
    a_[1][2] = first->a_[1][0] * second->a_[1][2] + first->a_[1][1] * second->a_[1][2] + first->a_[1][2];
#else
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            a_[j][i] = 0.0f;

            for (int k = 0; k < 2; k++)
            {
                a_[j][i] += first->a_[j][k] * second->a_[k][i];
            }
        }
        a_[j][2] += first->a_[j][2];
    }
#endif
}

float Matrix2x3::Size(void) const
{
    float cross_product = a_[0][0] * a_[1][1] - a_[1][0] * a_[0][1];
    return fabsf(cross_product);
}

Fractal::Fractal()
{
}


Fractal::~Fractal()
{
}

void Fractal::Generate(void)
{
    point_[0].Set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    draw_point_[0] = true;
    num_active_points_ = 1;

    for (int read_point_index = 0; read_point_index < num_active_points_; read_point_index++)
    {
        // Pre-emptive quit if nothing more can be done
        if (num_active_points_ == kMaxNumPoints) break;

        // Check for size [TODO: Implement the size check (with parameter?)]
        if (true)
        {
            for (int function_id = 0; function_id < kNumFunctions; function_id++)
            {
                // Pre-emptive quit if nothing more can be done
                if (num_active_points_ == kMaxNumPoints) break;

                point_[num_active_points_].Multiply(&(point_[read_point_index]), &(function_[function_id]));
                draw_point_[num_active_points_] = true;
                num_active_points_++;

                // The point is getting split. So no need to draw it.
                draw_point_[read_point_index] = false;
            }
        }
    }
}
