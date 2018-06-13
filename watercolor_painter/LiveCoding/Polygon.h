#pragma once

#include <vector>

// Somehow an edge always has a direction?
class Edge {
public:
    // Returns leftmost corner x coordinate
    float GetLeft(void) const {
        return (position_[0][0] < position_[1][0])
            ? position_[0][0]
            : position_[1][0];
    }

    // Return rightmost corner x coordinate
    float GetRight(void) const {
    return (position_[0][0] > position_[1][0])
        ? position_[0][0]
        : position_[1][0];
    }

public:
    float position_[2][2];
    bool is_border_;
    Edge *left_neightbor_;  // Only border edges have neighbors
    Edge *right_neighbor_;  // Only border edges have neighbors
};

class Trapezoid {
public:
    Trapezoid();
    virtual ~Trapezoid();

    float GetTop(void) const {
        return top_.position_[0][1];
    };
    float GetBottom(void) const {
        return bottom_.position_[0][1];;
    }
    float GetLeft(void) const {
        return left_.GetLeft();
    }
    float GetRight(void) const {
        return right_.GetRight();
    }

private:
    Edge top_;  // Always horizontal
    Edge bottom_;  // Always horizontal
    Edge left_;
    Edge right_;
};

class Polygon {
public:
    Polygon();
    virtual ~Polygon();

private:
    std::vector<Trapezoid> blocks;
};

