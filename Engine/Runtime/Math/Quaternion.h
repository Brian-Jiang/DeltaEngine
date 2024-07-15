#include <iostream>
#include <cmath>

namespace DeltaEngine
{

struct Quaternion {
    float w, x, y, z;

    // Default constructor
    Quaternion() : w(1), x(0), y(0), z(0) {}

    // Parameterized constructor
    Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    // Copy constructor
    Quaternion(const Quaternion& q) : w(q.w), x(q.x), y(q.y), z(q.z) {}

    // Addition
    Quaternion operator+(const Quaternion& q) const {
        return Quaternion(w + q.w, x + q.x, y + q.y, z + q.z);
    }

    // Multiplication
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        );
    }

    // Scalar multiplication
    Quaternion operator*(float scalar) const {
        return Quaternion(w * scalar, x * scalar, y * scalar, z * scalar);
    }

    // Norm (magnitude)
    float norm() const {
        return std::sqrt(w * w + x * x + y * y + z * z);
    }

    // Normalize
    Quaternion normalize() const {
        float n = norm();
        return Quaternion(w / n, x / n, y / n, z / n);
    }

    // Conjugate
    Quaternion conjugate() const {
        return Quaternion(w, -x, -y, -z);
    }

    // Inverse
    Quaternion inverse() const {
        return conjugate() * (1.0f / norm());
    }

    // Rotate a vector (x, y, z) by this quaternion
    Quaternion rotate(float vx, float vy, float vz) const {
        Quaternion qv(0, vx, vy, vz);
        Quaternion qr = (*this) * qv * inverse();
        return qr;
    }

    // Print quaternion
    void print() const {
        std::cout << "Quaternion(" << w << ", " << x << ", " << y << ", " << z << ")\n";
    }
};

}
