#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <openvdb/Types.h>
#include <openvdb/openvdb.h>

namespace godot
{

    class GDExample : public MeshInstance3D
    {
        GDCLASS(GDExample, MeshInstance3D)

    private:
        enum OperationType
        {
            ADD,
            SUBTRACT,
        };
        enum OperationShape
        {
            SPHERE,
            CUBE,
            TRIANGLE,
        };

        struct Operation
        {
            openvdb::Vec3d point;
            openvdb::Quatd rotation;
            openvdb::Vec3d scale;
            OperationType type;
            OperationShape shape;
            double brushSize;
            double brushBlend;

            // Used for triangle only
            openvdb::Vec3d triPoint1;
            openvdb::Vec3d triPoint2;
            openvdb::Vec3d triPoint3;
        };
        std::vector<Operation> allOperations = {};
        std::vector<Operation> pendingOperations = {};
        openvdb::DoubleGrid::Ptr grid;
        double lastVoxelSize;
        std::vector<openvdb::Vec3s> *tempStartingMeshVerts = new std::vector<openvdb::Vec3s>();
        std::vector<openvdb::Vec3I> *tempStartingMeshTris = new std::vector<openvdb::Vec3I>();

    protected:
        static void
        _bind_methods();

    public:
        GDExample();
        ~GDExample();

        void _ready();
        void _process(double delta);
        void regenMesh(double voxelSize);
        void pushOperation(Vector3 pos, Quaternion rotation, Vector3 scale, int type, int shape, double brushSize, double brushBlend);
        void pushTriangle(Vector3 p1, Vector3 p2, Vector3 p3);
        void tempSetStartingMesh(PackedVector3Array verts, PackedVector3Array tris);
    };

}

#endif