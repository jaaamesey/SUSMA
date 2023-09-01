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
#include <openvdb/tools/SignedFloodFill.h>
#include <openvdb/Grid.h>
#include <string>
#include <iostream>
#include "utils.cpp"

using namespace godot;

inline float lerp(float start, float end, float t)
{
    return start + t * (end - start);
}

inline float opSmoothUnionSDF(float a, float b, float k)
{
    float h = std::clamp(0.5 + 0.5 * (a - b) / k, 0.0, 1.0);
    return lerp(a, b, h) - k * h * (1.0 - h);
}

inline float opSmoothSubtractionSDF(float a, float b, float k)
{
    float h = std::clamp(0.5 - 0.5 * (a + b) / k, 0.0, 1.0);
    return lerp(a, -b, h) + k * h * (1.0 - h);
}

inline float sphereSDF(const openvdb::Vec3f &p, float radius)
{
    return (p - openvdb::Vec3f(0.0)).length() - radius;
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
        grid = openvdb::FloatGrid::create();
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxelSize));
        grid->setGridClass(openvdb::GRID_LEVEL_SET);
        grid->setName("result");
    }

    bool canSkipRemesh = !shouldBuildGridFromScratch && pendingOperations.empty();
    if (canSkipRemesh)
    {
        return;
    }

    openvdb::CoordBBox bbox(openvdb::Coord(-1.0 / voxelSize), openvdb::Coord(1.0 / voxelSize));
    auto accessor = grid->getAccessor();
    auto operations = shouldBuildGridFromScratch ? allOperations : pendingOperations;

    // TODO: Invert this nested loop and instead iterate through operation bboxes
    for (auto iter = bbox.begin(); iter != bbox.end(); ++iter)
    {
        openvdb::Vec3f worldCoord = grid->indexToWorld(*iter);
        auto sdf = shouldBuildGridFromScratch ? sphereSDF(worldCoord, 0.9) : accessor.getValue(*iter);
        // TODO: In theory, we shouldn't need to run every operation for every voxel
        for (auto operation : operations)
        {
            switch (operation.type)
            {
            case OperationType::ADD:
                sdf = opSmoothUnionSDF(sdf, sphereSDF(worldCoord - operation.point, operation.brushSize), operation.brushBlend);
                break;
            case OperationType::SUBTRACT:
                sdf = opSmoothSubtractionSDF(sdf, sphereSDF(worldCoord - operation.point, operation.brushSize), operation.brushBlend);
                break;
            default:
                throw "Invalid operation";
            }
        }
        accessor.setValueOn(*iter, sdf);
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

void GDExample::pushOperation(Vector3 brushPos, int type, float brushSize, float brushBlend)
{
    // TODO: This isn't really the right function to start storing operations - we should not be sampling each frame of the brush being used as its own operation.
    // A lot to improve here.
    struct Operation operation;
    operation.point = openvdb::Vec3f(brushPos.x, brushPos.y, brushPos.z);
    operation.type = (OperationType)type;
    operation.brushSize = brushSize;
    operation.brushBlend = brushBlend;
    allOperations.push_back(operation);
    pendingOperations.push_back(operation);
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

    ClassDB::bind_method(D_METHOD("regen_mesh", "voxel_size"), &GDExample::regenMesh);
}