#ifndef AMBIENT3D_SERVER_INT_VECTOR2_HPP
#define AMBIENT3D_SERVER_INT_VECTOR2_HPP


namespace AM {

    struct iVec2 {
        int x { 0 };
        int y { 0 };
    
        iVec2(){}
        iVec2(int _x, int _y) : x(_x), y(_y) {}
    };

};



#endif
