// Usages of IMath in OpenVDB don't seem to work without this?
#define IMATH_HALF_NO_LOOKUP_TABLE

#include "gdexample.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <vector>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/Grid.h>
#include <string>
#include <iostream>
#include "utils.cpp"

using namespace godot;

inline double lerp(double start, double end, double t)
{
    return start + t * (end - start);
}

inline double opSmoothUnionSDF(double a, double b, double k)
{
    double h = std::clamp(0.5 + 0.5 * (a - b) / k, 0.0, 1.0);
    return lerp(a, b, h) - k * h * (1.0 - h);
}

inline double opSmoothSubtractionSDF(double a, double b, double k)
{
    double h = std::clamp(0.5 - 0.5 * (a + b) / k, 0.0, 1.0);
    return lerp(a, -b, h) + k * h * (1.0 - h);
}

inline double sphereSDF(const openvdb::Vec3d &p, double radius)
{
    return p.length() - radius;
}

inline double cubeSDF(const openvdb::Vec3d &p, double halfWidth)
{
    openvdb::Vec3d q = openvdb::Vec3d(std::abs(p.x()) - halfWidth, std::abs(p.y()) - halfWidth, std::abs(p.z()) - halfWidth);
    return openvdb::Vec3d(std::max(q.x(), 0.0), std::max(q.y(), 0.0), std::max(q.z(), 0.0)).length() + std::min(std::max(q.x(), std::max(q.y(), q.z())), 0.0);
}

inline double triSDF(const openvdb::Vec3d &p, const openvdb::Vec3d &a, const openvdb::Vec3d &b, const openvdb::Vec3d &c)
{
    auto ba = b - a;
    auto pa = p - a;
    auto cb = c - b;
    auto pb = p - b;
    auto ac = a - c;
    auto pc = p - c;
    auto nor = ba.cross(ac);

    return std::sqrt(
        (nor.dot(pa.cross(ba)) >= 0.0 &&
         nor.dot(pb.cross(cb)) >= 0.0 &&
         nor.dot(pc.cross(ac)) >= 0.0)
            ? std::min(std::min(
                           (ba * (ba.dot(pa) / ba.lengthSqr()) - pa).lengthSqr(),
                           (cb * (cb.dot(pb) / cb.lengthSqr()) - pb).lengthSqr()),
                       (ac * (ac.dot(pc) / ac.lengthSqr()) - pc).lengthSqr())
            : nor.dot(pa) * nor.dot(pa) / nor.lengthSqr()) - 0.1;
}

inline double distanceBetweenVectors(const openvdb::Vec3d &v1, const openvdb::Vec3d &v2)
{
    double dx = v1.x() - v2.x();
    double dy = v1.y() - v2.y();
    double dz = v1.z() - v2.z();
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline double signedDistanceBetweenVectors(const openvdb::Vec3d &v1, const openvdb::Vec3d &v2)
{
    auto sign = v1.dot(v2) < 0 ? -1 : 1;
    return sign * distanceBetweenVectors(v1, v2);
}

inline openvdb::CoordBBox sphereBBox(const openvdb::Vec3d &p, double radius, double voxelSize)
{
    auto safetyMultiplier = 1.5; // For accounting for blending between spheres
    auto halfLength = safetyMultiplier * radius;
    return openvdb::CoordBBox((p.x() - halfLength) / voxelSize, (p.y() - halfLength) / voxelSize, (p.z() - halfLength) / voxelSize,
                              (p.x() + halfLength) / voxelSize, (p.y() + halfLength) / voxelSize, (p.z() + halfLength) / voxelSize);
}

void GDExample::regenMesh(double voxelSize)
{
    // Build level set grid
    bool shouldBuildGridFromScratch = !grid || voxelSize != lastVoxelSize;

    if (shouldBuildGridFromScratch)
    {
        if (grid)
        {
            grid.reset();
        }
        // Use 1 as a background value (points of a signed distance field not in a shape are positive)
        grid = openvdb::DoubleGrid::create(1);
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxelSize));
        grid->setGridClass(openvdb::GRID_LEVEL_SET);
        grid->setName("result");
    }

    bool canSkipRemesh = !shouldBuildGridFromScratch && pendingOperations.empty();
    if (canSkipRemesh)
    {
        return;
    }

    auto accessor = grid->getAccessor();
    auto operations = shouldBuildGridFromScratch ? allOperations : pendingOperations;

    for (auto operation : operations)
    {
        auto bbox = sphereBBox(operation.point, operation.brushSize, voxelSize);
        for (auto iter = bbox.begin(); iter != bbox.end(); ++iter)
        {
            openvdb::Vec3d worldCoord = grid->indexToWorld(*iter);
            auto sdf = accessor.getValue(*iter);
            auto point = operation.scale * operation.rotation.rotateVector(worldCoord - operation.point);
            double shapeSdf;
            switch (operation.shape)
            {
            case OperationShape::SPHERE:
                shapeSdf = sphereSDF(point, operation.brushSize);
                break;
            case OperationShape::CUBE:
                shapeSdf = cubeSDF(point, operation.brushSize);
                break;
            case OperationShape::TRIANGLE:
                shapeSdf = triSDF(point, operation.triPoint1, operation.triPoint2, operation.triPoint3);
                break;
            default:
                throw std::exception("Invalid shape");
            }
            switch (operation.type)
            {
            case OperationType::ADD:
                sdf = opSmoothUnionSDF(sdf, shapeSdf, operation.brushBlend);
                break;
            case OperationType::SUBTRACT:
                sdf = opSmoothSubtractionSDF(sdf, shapeSdf, operation.brushBlend);
                break;
            default:
                throw std::exception("Invalid operation");
            }
            accessor.setValueOn(*iter, sdf);
        }
    }

    pendingOperations.clear();

    // Output mesh to Godot
    std::vector<openvdb::Vec3s> points = {};
    std::vector<openvdb::Vec4I> quads = {};
    openvdb::tools::volumeToMesh(*grid, points, quads);

    auto outputVerts = PackedVector3Array();
    auto outputNormals = PackedVector3Array();
    for (auto point : points)
    {
        auto vert = Vector3(point.x(), point.y(), point.z());
        outputVerts.push_back(vert);
        outputNormals.push_back(vert.normalized());
    }

    auto outputTris = PackedInt32Array();
    for (auto quad : quads)
    {
        outputTris.push_back(quad[0]);
        outputTris.push_back(quad[1]);
        outputTris.push_back(quad[2]);

        outputTris.push_back(quad[0]);
        outputTris.push_back(quad[2]);
        outputTris.push_back(quad[3]);
    }

    auto surfaceArray = Array();
    surfaceArray.resize(Mesh::ArrayType::ARRAY_MAX);
    surfaceArray.insert(Mesh::ArrayType::ARRAY_VERTEX, outputVerts);
    surfaceArray.insert(Mesh::ArrayType::ARRAY_NORMAL, outputNormals);
    surfaceArray.insert(Mesh::ArrayType::ARRAY_INDEX, outputTris);
    surfaceArray.resize(Mesh::ArrayType::ARRAY_MAX);

    ArrayMesh mesh = ArrayMesh();
    mesh.add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surfaceArray);

    set_mesh(Ref(&mesh));

    lastVoxelSize = voxelSize;
}

void GDExample::pushOperation(Vector3 brushPos, Quaternion brushRotation, Vector3 brushScale, int type, int shape, double brushSize, double brushBlend)
{
    struct Operation operation;
    operation.point = openvdb::Vec3d(brushPos.x, brushPos.y, brushPos.z);
    operation.rotation = openvdb::Quatd(brushRotation.x, brushRotation.y, brushRotation.z, brushRotation.w);
    operation.scale = openvdb::Vec3d(brushScale.x, brushScale.y, brushScale.z);
    operation.type = (OperationType)type;
    operation.shape = (OperationShape)shape;
    operation.brushSize = brushSize;
    operation.brushBlend = brushBlend;
    allOperations.push_back(operation);
    pendingOperations.push_back(operation);
}

void GDExample::pushTriangle(Vector3 p1, Vector3 p2, Vector3 p3)
{
    struct Operation operation;
    operation.point = openvdb::Vec3d(
        (p1.x + p2.x + p3.x) / 3.0,
        (p1.y + p2.y + p3.y) / 3.0,
        (p1.z + p2.z + p3.z) / 3.0
    );
    operation.rotation = openvdb::Quatd();
    operation.scale = openvdb::Vec3d(1, 1, 1);
    operation.type = OperationType::ADD;
    operation.shape = OperationShape::TRIANGLE;
    operation.brushBlend = 0;
    operation.triPoint1 = openvdb::Vec3d(p1.x, p1.y, p1.z);
    operation.triPoint2 = openvdb::Vec3d(p2.x, p2.y, p2.z);
    operation.triPoint3 = openvdb::Vec3d(p3.x, p3.y, p3.z);
    operation.brushSize = std::max(std::max((operation.point - operation.triPoint1).lengthSqr(), (operation.point - operation.triPoint2).lengthSqr()), (operation.point - operation.triPoint3).lengthSqr());
    allOperations.push_back(operation);
    pendingOperations.push_back(operation);
}

void GDExample::tempSetStartingMesh(PackedVector3Array verts, PackedVector3Array tris)
{
    for (auto vert : verts)
    {
        tempStartingMeshVerts->push_back(openvdb::Vec3s(vert.x, vert.y, vert.z));
    }
    for (auto tri : tris)
    {
        tempStartingMeshTris->push_back(openvdb::Vec3I(tri.x, tri.y, tri.z));
    }
}

void GDExample::_ready()
{
}

GDExample::GDExample()
{
}

GDExample::~GDExample()
{
}

void GDExample::_process(double delta) {}

void GDExample::_bind_methods()
{
    ClassDB::bind_integer_constant("GDExample", "OPERATION_TYPE", "ADD", OperationType::ADD);
    ClassDB::bind_integer_constant("GDExample", "OPERATION_TYPE", "SUBTRACT", OperationType::SUBTRACT);

    ClassDB::bind_integer_constant("GDExample", "OPERATION_SHAPE", "SPHERE", OperationShape::SPHERE);
    ClassDB::bind_integer_constant("GDExample", "OPERATION_SHAPE", "CUBE", OperationShape::CUBE);

    auto pushOperation = D_METHOD("push_operation");
    pushOperation.args = {"pos", "rotation", "scale", "type", "shape", "brushSize", "brushBlend"};
    ClassDB::bind_method(pushOperation, &GDExample::pushOperation);

    auto pushTriOperation = D_METHOD("push_triangle");
    pushTriOperation.args = {"p1", "p2", "p3"};
    ClassDB::bind_method(pushTriOperation, &GDExample::pushTriangle);

    auto setStartingMeshOp = D_METHOD("set_starting_mesh");
    setStartingMeshOp.args = {"verts", "tris"};
    ClassDB::bind_method(setStartingMeshOp, &GDExample::tempSetStartingMesh);

    ClassDB::bind_method(D_METHOD("regen_mesh", "voxel_size"), &GDExample::regenMesh);
}