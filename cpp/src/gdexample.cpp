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

inline double distanceBetweenVectors(const openvdb::Vec3d &v1, const openvdb::Vec3d& v2) {
    double dx = v1.x() - v2.x();
    double dy = v1.y() - v2.y();
    double dz = v1.z() - v2.z();
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline double signedDistanceBetweenVectors(const openvdb::Vec3d &v1, const openvdb::Vec3d& v2) {
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
        grid = openvdb::DoubleGrid::create();
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxelSize));
        grid->setGridClass(openvdb::GRID_LEVEL_SET);
        grid->setName("result");

        openvdb::CoordBBox bbox(openvdb::Coord(-10.0 / voxelSize), openvdb::Coord(10.0 / voxelSize));
        auto accessor = grid->getAccessor();
        // for (auto iter = bbox.begin(); iter != bbox.end(); ++iter) {
        //     for (auto vert : *tempStartingMeshVerts) {
        //         openvdb::Vec3d worldCoord = grid->indexToWorld(*iter);
        //         auto sdf = (distanceBetweenVectors(worldCoord, vert) > voxelSize) ? 1 : -1;
        //         accessor.setValueOn(*iter, sdf);
        //     }
        // }

        // // Insert initial sphere
        // openvdb::CoordBBox bbox(openvdb::Coord(-10.0 / voxelSize), openvdb::Coord(10.0 / voxelSize));
        // auto accessor = grid->getAccessor();
        // for (auto iter = bbox.begin(); iter != bbox.end(); ++iter)
        // {
        //     openvdb::Vec3d worldCoord = grid->indexToWorld(*iter);
        //     auto sdf = sphereSDF(worldCoord, 0.9);
        //     accessor.setValueOn(*iter, sdf);
        // }
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
            switch (operation.type)
            {
            case OperationType::ADD:
                sdf = opSmoothUnionSDF(sdf, sphereSDF(worldCoord - operation.point, operation.brushSize), operation.brushBlend);
                break;
            case OperationType::SUBTRACT:
                sdf = opSmoothSubtractionSDF(sdf, sphereSDF(worldCoord - operation.point, operation.brushSize), operation.brushBlend);
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

void GDExample::pushOperation(Vector3 brushPos, int type, double brushSize, double brushBlend)
{
    struct Operation operation;
    operation.point = openvdb::Vec3d(brushPos.x, brushPos.y, brushPos.z);
    operation.type = (OperationType)type;
    operation.brushSize = brushSize;
    operation.brushBlend = brushBlend;
    allOperations.push_back(operation);
    pendingOperations.push_back(operation);
}

void GDExample::tempSetStartingMesh(PackedVector3Array verts, PackedVector3Array tris)
{
    for (auto vert : verts) {
        tempStartingMeshVerts->push_back(openvdb::Vec3s(vert.x, vert.y, vert.z));
    }
    for (auto tri : tris) {
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

    auto pushOperation = D_METHOD("push_operation");
    pushOperation.args = {"pos", "type", "brushSize", "brushBlend"};
    ClassDB::bind_method(pushOperation, &GDExample::pushOperation);

    auto setStartingMeshOp = D_METHOD("set_starting_mesh");
    setStartingMeshOp.args = {"verts", "tris"};
    ClassDB::bind_method(setStartingMeshOp, &GDExample::tempSetStartingMesh);

    ClassDB::bind_method(D_METHOD("regen_mesh", "voxel_size"), &GDExample::regenMesh);
}