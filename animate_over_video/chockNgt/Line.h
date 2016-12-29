#pragma once

#include <vector>
#include <tuple>

class Line
{
public:
    Line();
    virtual ~Line();

    void AddNode(float x, float y, bool make_fancy);
    void MakeFancy(void);
    
    int Save(FILE *file);
    int Export(FILE *file);
    int Load(FILE *file);

    // Assumes:
    //  - Texture line.png set
    //  - No culling
    void Draw(float alpha);
    void DrawFancy(void);

private:
    static const unsigned int kMagicNumber = 0x239841fb;
    static const unsigned int kExportMagicNumber = 0x773fbca2;
    static const float kLineWidth;
    static const float kMinLineWidth;
    // Distance up to which neighboring points have an influence on the position of the current node
    static const float kNeighborInterpolationDistance;
    
    std::vector< std::pair<float, float> >nodes_;
    std::vector< std::pair<float, float> >fancy_nodes_;
};

