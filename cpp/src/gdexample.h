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

        struct Operation
        {
            openvdb::Vec3d point;
            OperationType type;
            double brushSize;
            double brushBlend;
        };
        std::vector<Operation> allOperations = {};
        std::vector<Operation> pendingOperations = {};
        openvdb::tools::MultiResGrid<openvdb::DoubleGrid> *grid;
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
        void pushOperation(Vector3 pos, int type, double brushSize, double brushBlend);
    };

}

#endif