// miniRayTracer — a tiny ray tracer from scratch
// Build:  g++ -O2 -o raytracer main.cpp
// Run:    ./raytracer > image.ppm
// View:   open image.ppm in an image viewer (or convert to PNG)

#include <cmath>
#include <iostream>
#include <limits>

// ---------------------------------------------------------------------------
// Vec3 — points, directions, and colors all use the same 3 floats
// ---------------------------------------------------------------------------
struct Vec3 {
    double x, y, z;

    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double t) const { return Vec3(x * t, y * t, z * t); }
    Vec3 operator/(double t) const { return *this * (1.0 / t); }

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    double length_squared() const { return x * x + y * y + z * z; }
};

inline Vec3 operator*(double t, const Vec3& v) { return v * t; }
inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 unit_vector(const Vec3& v) { return v / v.length(); }

using Point3 = Vec3;
using Color  = Vec3;

void write_color(std::ostream& out, const Color& c) {
    // Map [0,1] float color -> [0,255] byte
    int r = static_cast<int>(255.999 * c.x);
    int g = static_cast<int>(255.999 * c.y);
    int b = static_cast<int>(255.999 * c.z);
    out << r << ' ' << g << ' ' << b << '\n';
}

// ---------------------------------------------------------------------------
// Ray — origin + direction. Point along the ray: P(t) = origin + t * direction
// ---------------------------------------------------------------------------
struct Ray {
    Point3 origin;
    Vec3   direction;

    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction)
        : origin(origin), direction(direction) {}

    Point3 at(double t) const { return origin + t * direction; }
};

// ---------------------------------------------------------------------------
// Sphere hit test
//
// A point P is on the sphere if |P - center|^2 = radius^2
// Substitute the ray P(t) = O + tD and you get a quadratic in t:
//     (D·D)t² + 2D·(O-C)t + (O-C)·(O-C) - r² = 0
// If there is a real root with t > 0, the ray hits the sphere.
// ---------------------------------------------------------------------------
bool hit_sphere(const Point3& center, double radius, const Ray& r, double& t_hit) {
    Vec3 oc = r.origin - center;
    double a = dot(r.direction, r.direction);
    double b = 2.0 * dot(oc, r.direction);
    double c = dot(oc, oc) - radius * radius;
    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    // Smallest positive root (closest hit in front of the camera)
    double sqrt_d = std::sqrt(discriminant);
    double t = (-b - sqrt_d) / (2.0 * a);
    if (t <= 0) {
        t = (-b + sqrt_d) / (2.0 * a);
        if (t <= 0) return false;
    }

    t_hit = t;
    return true;
}

// ---------------------------------------------------------------------------
// Color a ray: hit the sphere → paint by surface normal; else a sky gradient
// ---------------------------------------------------------------------------
Color ray_color(const Ray& r) {
    Point3 sphere_center(0, 0, -1);
    double sphere_radius = 0.5;
    double t_hit;

    if (hit_sphere(sphere_center, sphere_radius, r, t_hit)) {
        // Surface normal at the hit point (points outward from the sphere)
        Point3 hit_point = r.at(t_hit);
        Vec3 normal = unit_vector(hit_point - sphere_center);
        // Map normal from [-1,1] to [0,1] so we can see it as a color
        return 0.5 * Color(normal.x + 1, normal.y + 1, normal.z + 1);
    }

    // Background: soft blue sky gradient based on ray's up/down direction
    Vec3 unit_dir = unit_vector(r.direction);
    double t = 0.5 * (unit_dir.y + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
}

int main() {
    // Image
    const int image_width  = 400;
    const int image_height = 225; // 16:9

    // Camera: a virtual screen in front of the eye at the origin
    // Looking down -Z (into the screen). Y is up, X is right.
    const double aspect_ratio = static_cast<double>(image_width) / image_height;
    const double viewport_height = 2.0;
    const double viewport_width  = aspect_ratio * viewport_height;
    const double focal_length    = 1.0;

    const Point3 origin(0, 0, 0);
    const Vec3 horizontal(viewport_width, 0, 0);
    const Vec3 vertical(0, viewport_height, 0);
    const Point3 lower_left_corner =
        origin - horizontal / 2 - vertical / 2 - Vec3(0, 0, focal_length);

    // PPM header (P3 = ASCII RGB)
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    // For each pixel: build a ray through it, shade it, write the color
    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            double u = static_cast<double>(i) / (image_width - 1);
            double v = static_cast<double>(j) / (image_height - 1);

            Ray ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);
            Color pixel = ray_color(ray);
            write_color(std::cout, pixel);
        }
    }

    std::cerr << "\nDone. Redirect stdout to a .ppm file to save the image.\n";
    return 0;
}
