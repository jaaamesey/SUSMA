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
            DRAG,
        };
        enum OperationShape
        {
            SPHERE,
            CUBE,
        };

        struct Operation
        {
            openvdb::Vec3d point;
            openvdb::Quatd rotation;
            openvdb::Vec3d scale;
            openvdb::Vec3d direction;
            OperationType type;
            OperationShape shape;
            double brushSize;
            double brushBlend;
        };
        std::vector<Operation> allOperations = {};
        std::vector<Operation> pendingOperations = {};
        openvdb::DoubleGrid::Ptr grid;
        double lastVoxelSize;
        std::vector<openvdb::Vec3s> *tempStartingMeshVerts = new std::vector<openvdb::Vec3s>();
        std::vector<openvdb::Vec3I> *tempStartingMeshTris = new std::vector<openvdb::Vec3I>();;

    protected:
        static void
        _bind_methods();

    public:
        GDExample();
        ~GDExample();

        void _ready();
        void _process(double delta);
        void regenMesh(double voxelSize);
        void pushOperation(Vector3 pos, Quaternion rotation, Vector3 scale, Vector3 direction, int type, int shape, double brushSize, double brushBlend);
        void tempSetStartingMesh(PackedVector3Array verts, PackedVector3Array tris);
    };

}

#endif