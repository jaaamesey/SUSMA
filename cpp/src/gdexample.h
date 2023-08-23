#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <openvdb/Types.h>

namespace godot
{

    class GDExample : public MeshInstance3D
    {
        GDCLASS(GDExample, MeshInstance3D)

    private:
        Vector3 brushPos;
        float brushBlend;
        struct Operation
        {
            openvdb::Vec3f point;
        };
        std::vector<Operation> operations = {};

    protected:
        static void
        _bind_methods();

    public:
        GDExample();
        ~GDExample();

        void _ready();
        void _process(double delta);
        void regenMesh(double voxelSize);
        void setBrushPos(Vector3 pos);
        void setBrushBlend(float blend);
    };

}

#endif