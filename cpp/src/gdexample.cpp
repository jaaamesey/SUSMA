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

inline int sign(double val)
{
    if (val == 0.0)
        return 0;
    return val > 0 ? 1 : -1;
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

inline double solidAngleSDF(const openvdb::Vec3d &p, double radius)
{
    auto angleRad = .35; 
    auto c = openvdb::Vec2d(std::sin(angleRad), std::cos(angleRad));
    auto q = openvdb::Vec2d(openvdb::Vec2d(p.x(), p.z()).length(), p.y());
    auto l = q.length() - radius;
    auto m = (q - c * std::clamp(q.dot(c), 0.0, radius)).length();
    return std::max(l, m * sign(c.y() * q.x() - c.x() * q.y()));
}

inline double cubeSDF(const openvdb::Vec3d &p, double halfWidth)
{
    openvdb::Vec3d q = openvdb::Vec3d(std::abs(p.x()) - halfWidth, std::abs(p.y()) - halfWidth, std::abs(p.z()) - halfWidth);
    return openvdb::Vec3d(std::max(q.x(), 0.0), std::max(q.y(), 0.0), std::max(q.z(), 0.0)).length() + std::min(std::max(q.x(), std::max(q.y(), q.z())), 0.0);
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
        // Use 1.0 as a background value (i.e. value of an empty SDF)
        grid = openvdb::DoubleGrid::create(1.0);
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
        openvdb::DoubleGrid::Ptr prevGrid = grid->deepCopy();
        auto prevAccessor = prevGrid->getAccessor();
        auto bbox = sphereBBox(operation.point, operation.brushSize, voxelSize);
        for (auto iter = bbox.begin(); iter != bbox.end(); ++iter)
        {
            openvdb::Vec3d worldCoord = grid->indexToWorld(*iter);
            auto sdf = prevAccessor.getValue(*iter);
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
            case OperationShape::SOLID_ANGLE:
                shapeSdf = solidAngleSDF(point, operation.brushSize);
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
            case OperationType::DRAG:
            {
                auto influenceAmount = std::max(0.0, -sphereSDF(point, operation.brushSize));
                auto transformedCoord = worldCoord + influenceAmount * operation.direction;
                auto indexCoord = grid->worldToIndex(transformedCoord);
                sdf = prevAccessor.getValue(openvdb::Coord(indexCoord.x(), indexCoord.y(), indexCoord.z()));
                break;
            }
            default:
                throw std::exception("Invalid operation");
            }
            accessor.setValueOn(*iter, sdf);
        }
        prevGrid.reset();
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

void GDExample::pushOperation(Vector3 brushPos, Quaternion brushRotation, Vector3 brushScale, Vector3 direction, int type, int shape, double brushSize, double brushBlend)
{
    struct Operation operation;
    operation.point = openvdb::Vec3d(brushPos.x, brushPos.y, brushPos.z);
    operation.rotation = openvdb::Quatd(brushRotation.x, brushRotation.y, brushRotation.z, brushRotation.w);
    operation.scale = openvdb::Vec3d(brushScale.x, brushScale.y, brushScale.z);
    operation.direction = openvdb::Vec3d(direction.x, direction.y, direction.z);
    operation.type = (OperationType)type;
    operation.shape = (OperationShape)shape;
    operation.brushSize = brushSize;
    operation.brushBlend = brushBlend;
    allOperations.push_back(operation);
    pendingOperations.push_back(operation);
}

double GDExample::getGridVal(Vector3 pos)
{
    auto accessor = grid->getAccessor();
    auto indexVec = grid->worldToIndex(openvdb::Vec3d(pos.x, pos.y, pos.z));
    return accessor.getValue(openvdb::Coord(indexVec.x(), indexVec.y(), indexVec.z()));
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
    ClassDB::bind_integer_constant("GDExample", "OPERATION_TYPE", "DRAG", OperationType::DRAG);

    ClassDB::bind_integer_constant("GDExample", "OPERATION_SHAPE", "SPHERE", OperationShape::SPHERE);
    ClassDB::bind_integer_constant("GDExample", "OPERATION_SHAPE", "CUBE", OperationShape::CUBE);
    ClassDB::bind_integer_constant("GDExample", "OPERATION_SHAPE", "SOLID_ANGLE", OperationShape::SOLID_ANGLE);

    auto pushOperation = D_METHOD("push_operation");
    pushOperation.args = {"pos", "rotation", "scale", "direction", "type", "shape", "brushSize", "brushBlend"};
    ClassDB::bind_method(pushOperation, &GDExample::pushOperation);

    auto setStartingMeshOp = D_METHOD("set_starting_mesh");
    setStartingMeshOp.args = {"verts", "tris"};
    ClassDB::bind_method(setStartingMeshOp, &GDExample::tempSetStartingMesh);
    
    ClassDB::bind_method(D_METHOD("get_grid_val", "pos"), &GDExample::getGridVal);

    ClassDB::bind_method(D_METHOD("regen_mesh", "voxel_size"), &GDExample::regenMesh);
}