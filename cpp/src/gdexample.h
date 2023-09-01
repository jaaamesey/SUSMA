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

        float brushBlend;
        struct Operation
        {
            openvdb::Vec3f point;
            OperationType type;
            float brushSize;
            float brushBlend;
        };
        std::vector<Operation> allOperations = {};
        std::vector<Operation> pendingOperations = {};
        openvdb::FloatGrid::Ptr grid;
        double lastVoxelSize;

    protected:
        static void
        _bind_methods();

    public:
        GDExample();
        ~GDExample();

        void _ready();
        void _process(double delta);
        void regenMesh(double voxelSize);
        void pushOperation(Vector3 pos, int type, float brushSize, float brushBlend);
    };

}

#endif