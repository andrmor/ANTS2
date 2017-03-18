#ifndef AGRIDELEMENTRECORD
#define AGRIDELEMENTRECORD

class AGridElementRecord
{
public:
    AGridElementRecord(int shape, double dx, double dy) : shape(shape), dx(dx), dy(dy) {}
    AGridElementRecord() {}

    int shape;  // 0 - rectangular (dx by dy); 1 - hexagonal (dx is the side)
    double dx, dy;
};

#endif // AGRIDELEMENTRECORD

