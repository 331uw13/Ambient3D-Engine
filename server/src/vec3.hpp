#ifndef AMBIENT3D_VECTOR3_HPP
#define AMBIENT3D_VECTOR3_HPP


namespace AM {

    struct Vec3 {
        float x { 0 };
        float y { 0 };
        float z { 0 };

        Vec3() {}
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}


        float length()                   const;
        float distance(const Vec3& rhs)  const;
        float dot(const Vec3& rhs)       const;
        Vec3  cross(const Vec3& rhs)     const;
        Vec3  normalize()                const;
        Vec3  negate()                   const;
        Vec3& normalize_self();
        Vec3& negate_self();


        Vec3  operator-(const Vec3& rhs) const;
        Vec3  operator+(const Vec3& rhs) const;
        Vec3  operator/(const Vec3& rhs) const;
        Vec3  operator*(const Vec3& rhs) const;
        Vec3& operator-=(const Vec3& rhs);
        Vec3& operator+=(const Vec3& rhs);
        Vec3& operator/=(const Vec3& rhs);
        Vec3& operator*=(const Vec3& rhs);
    
        Vec3  operator-(float rhs) const;
        Vec3  operator+(float rhs) const;
        Vec3  operator/(float rhs) const;
        Vec3  operator*(float rhs) const;
        Vec3& operator-=(float rhs);
        Vec3& operator+=(float rhs);
        Vec3& operator/=(float rhs);
        Vec3& operator*=(float rhs);
    
    };


};




#endif
