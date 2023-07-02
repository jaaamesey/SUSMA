#include <chrono>
#include <godot_cpp/variant/utility_functions.hpp>

namespace Utils
{
    inline long long getCurrentTimeNs()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    class PerfTimer
    {
        long long time_started;

    public:
        PerfTimer()
        {
            time_started = getCurrentTimeNs();
        }

        // broken?
        //  long long getDurationNs()
        //  {
        //      return getCurrentTimeNs() - time_started;
        //  }
    };

    inline void gdPrint(const char *str)
    {
        godot::UtilityFunctions::print(*str);
    }
    inline void gdPrint(const godot::Variant *variant)
    {
        godot::UtilityFunctions::print(variant);
    }
}