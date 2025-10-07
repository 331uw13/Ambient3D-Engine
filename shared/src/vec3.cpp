#include <cmath>

#include "../include/vec3.hpp"


float AM::Vec3::length() const {
    return sqrtf(this->x*this->x + this->y*this->y + this->z*this->z);
}
        
AM::Vec3 AM::Vec3::negate() const {
    return AM::Vec3(-this->x, -this->y, -this->z);
}

float AM::Vec3::distance(const Vec3& b) const {
    const float dx = this->x - b.x;
    const float dy = this->y - b.y;
    const float dz = this->z - b.z;

    return sqrtf(dx*dx + dy*dy + dz*dz);
}

float AM::Vec3::dot(const Vec3& rhs) const {
    return (this->x * rhs.x
          + this->y * rhs.y
          + this->z * rhs.z);
}

AM::Vec3 AM::Vec3::cross(const Vec3& rhs) const {
    return AM::Vec3(
            this->y * rhs.z - this->z * rhs.y,
            this->z * rhs.x - this->x * rhs.z,
            this->x * rhs.y - this->y * rhs.x);
}

AM::Vec3 AM::Vec3::normalize() const {
    const float len = this->length();
    return AM::Vec3(
            this->x / len,
            this->y / len,
            this->z / len);
}

AM::Vec3& AM::Vec3::normalize_self() {
    const float len = this->length();
    this->x /= len;
    this->y /= len;
    this->z /= len;
    return *this;
}

AM::Vec3& AM::Vec3::negate_self() {
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;
    return *this;
}



AM::Vec3 AM::Vec3::operator-(const Vec3& rhs) const {
    return AM::Vec3(
            this->x - rhs.x,
            this->y - rhs.y,
            this->z - rhs.z);
}

AM::Vec3 AM::Vec3::operator+(const Vec3& rhs) const {
    return AM::Vec3(
            this->x + rhs.x,
            this->y + rhs.y,
            this->z + rhs.z);
}

AM::Vec3 AM::Vec3::operator/(const Vec3& rhs) const {
    return AM::Vec3(
            this->x / rhs.x,
            this->y / rhs.y,
            this->z / rhs.z);
}

AM::Vec3 AM::Vec3::operator*(const Vec3& rhs) const {
    return AM::Vec3(
            this->x * rhs.x,
            this->y * rhs.y,
            this->z * rhs.z);
}

AM::Vec3& AM::Vec3::operator-=(const Vec3& rhs) {
    this->x -= rhs.x;
    this->y -= rhs.y;
    this->z -= rhs.z;
    return *this;
}

AM::Vec3& AM::Vec3::operator+=(const Vec3& rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    this->z += rhs.z;
    return *this;
}

AM::Vec3& AM::Vec3::operator/=(const Vec3& rhs) {
    this->x /= rhs.x;
    this->y /= rhs.y;
    this->z /= rhs.z;
    return *this;
}

AM::Vec3& AM::Vec3::operator*=(const Vec3& rhs) {
    this->x *= rhs.x;
    this->y *= rhs.y;
    this->z *= rhs.z;
    return *this;
}


AM::Vec3 AM::Vec3::operator-(float rhs) const {
    return AM::Vec3(
            this->x - rhs,
            this->y - rhs,
            this->z - rhs);
}

AM::Vec3 AM::Vec3::operator+(float rhs) const {
    return AM::Vec3(
            this->x + rhs,
            this->y + rhs,
            this->z + rhs);
}

AM::Vec3 AM::Vec3::operator/(float rhs) const {
    return AM::Vec3(
            this->x / rhs,
            this->y / rhs,
            this->z / rhs);
}

AM::Vec3 AM::Vec3::operator*(float rhs) const {
    return AM::Vec3(
            this->x * rhs,
            this->y * rhs,
            this->z * rhs);
}

AM::Vec3& AM::Vec3::operator-=(float rhs) {
    this->x -= rhs;
    this->y -= rhs;
    this->z -= rhs;
    return *this;
}

AM::Vec3& AM::Vec3::operator+=(float rhs) {
    this->x += rhs;
    this->y += rhs;
    this->z += rhs;
    return *this;
}

AM::Vec3& AM::Vec3::operator/=(float rhs) {
    this->x /= rhs;
    this->y /= rhs;
    this->z /= rhs;
    return *this;
}

AM::Vec3& AM::Vec3::operator*=(float rhs) {
    this->x *= rhs;
    this->y *= rhs;
    this->z *= rhs;
    return *this;
}

