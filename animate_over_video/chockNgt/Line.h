#pragma once

#include <vector>
#include <tuple>

class Line
{
public:
    Line();
    virtual ~Line();

    void AddNode(float x, float y);
    
    int Save(FILE *file);
    int Load(FILE *file);

    // Assumes:
    //  - Texture line.png set
    //  - No culling
    void Draw(float alpha);
    void DrawFancy(void);

private:
    static const unsigned int kMagicNumber = 0x239841fb;
    static const float kLineWidth;
    static const float kMinLineWidth;
    
    std::vector< std::pair<float, float> >nodes_;
};

