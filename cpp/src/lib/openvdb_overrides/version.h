// STUB

#ifndef OPENVDB_VERSION_HAS_BEEN_INCLUDED
#define OPENVDB_VERSION_HAS_BEEN_INCLUDED

#include "openvdb/Platform.h"

#include <cstddef> // size_t
#include <cstdint> // uint32_t

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Auto generated build configurations from CMake

///@{
/// @brief Library major, minor and patch version numbers
/// @hideinitializer
#define OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER 10
/// @hideinitializer
#define OPENVDB_LIBRARY_MINOR_VERSION_NUMBER 10
/// @hideinitializer
#define OPENVDB_LIBRARY_PATCH_VERSION_NUMBER 10
///@}

/// @note  This ifndef exists for compatibility with older versions of OpenVDB.
///   This value should never be different from the value configured when
///   OpenVDB was built, but this previously needed to be defined by downstream
///   software. Redefining it here would cause build failures, so this allows
///   users to transition and remove the define in their build systems.
#ifndef OPENVDB_ABI_VERSION_NUMBER
/// @brief The ABI version that OpenVDB was built with
/// @hideinitializer
#define OPENVDB_ABI_VERSION_NUMBER 10
#endif

/// @brief Library version number string of the form "<major>.<minor>.<patch>"
/// @details This is a macro rather than a static constant because we typically
/// want the compile-time version number, not the runtime version number
/// (although the two are usually the same).
/// @hideinitializer
#define OPENVDB_LIBRARY_VERSION_STRING "10.10.10"

/// @brief Library version number string of the form "<major>.<minor>.<patch>abi<abi>"
/// @details This is a macro rather than a static constant because we typically
/// want the compile-time version number, not the runtime version number
/// (although the two are usually the same).
/// @hideinitializer
#define OPENVDB_LIBRARY_ABI_VERSION_STRING "10.10.10abi10"

/// @brief Library version number as a packed integer ("%02x%02x%04x", major, minor, patch)
/// @hideinitializer
#define OPENVDB_LIBRARY_VERSION_NUMBER 10

/// @brief Where this version was compiled from if it comes from a
/// git repo.
#define OPENVDB_PACKAGE_URL "@OPENVDB_PACKAGE_URL@"
#define OPENVDB_PACKAGE_REVISION "@OPENVDB_PACKAGE_REVISION@"

/// @brief The version namespace name for this library version
/// @hideinitializer
///
/// When the ABI version number matches the library major version number,
/// symbols are named as in the following examples:
/// - @b openvdb::vX_Y::Vec3i
/// - @b openvdb::vX_Y::io::File
/// - @b openvdb::vX_Y::tree::Tree
///
/// where X and Y are the major and minor version numbers.
///
/// When the ABI version number does not match the library major version number,
/// symbol names include the ABI version:
/// - @b openvdb::vX_YabiN::Vec3i
/// - @b openvdb::vX_YabiN::io::File
/// - @b openvdb::vX_YabiN::tree::Tree
///
/// where X, Y and N are the major, minor and ABI version numbers, respectively.
#if OPENVDB_ABI_VERSION_NUMBER == OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER
#define OPENVDB_VERSION_NAME v10_10
#else
#define OPENVDB_VERSION_NAME v10_10abi10
#endif

/* Defines the macros for explicit template declarations. */
#define OPENVDB_INSTANTIATE extern template OPENVDB_TEMPLATE_IMPORT
#define OPENVDB_INSTANTIATE_CLASS extern template class OPENVDB_TEMPLATE_IMPORT
#define OPENVDB_INSTANTIATE_STRUCT extern template struct OPENVDB_TEMPLATE_IMPORT

/* Defines the macros for explicit template instantiations. */
#define OPENVDB_REAL_TREE_INSTANTIATE(Function) @OPENVDB_REAL_TREE_INSTANTIATIONS @
#define OPENVDB_NUMERIC_TREE_INSTANTIATE(Function) @OPENVDB_NUMERIC_TREE_INSTANTIATIONS @
#define OPENVDB_VEC3_TREE_INSTANTIATE(Function) @OPENVDB_VEC3_TREE_INSTANTIATIONS @
#define OPENVDB_VOLUME_TREE_INSTANTIATE(Function) @OPENVDB_VOLUME_TREE_INSTANTIATIONS @
#define OPENVDB_ALL_TREE_INSTANTIATE(Function) @OPENVDB_ALL_TREE_INSTANTIATIONS @

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if OPENVDB_ABI_VERSION_NUMBER > OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER
// If using a future OPENVDB_ABI_VERSION_NUMBER, issue a message directive.
// This can be suppressed by defining OPENVDB_USE_FUTURE_ABI_<VERSION>=ON.
// Note that, whilst the VDB CMake does not allow this option to be hit,
// it exists to propagate this message to downstream targets
#if OPENVDB_ABI_VERSION_NUMBER == 11
#ifndef OPENVDB_USE_FUTURE_ABI_11
PRAGMA(message("NOTE: ABI = 11 is still in active development and has not been finalized, "
               "define OPENVDB_USE_FUTURE_ABI_11 to suppress this message"))
#endif
#else
#error expected OPENVDB_ABI_VERSION_NUMBER <= OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER
#endif
#endif

// If using an OPENVDB_ABI_VERSION_NUMBER that has been deprecated, issue a message
// directive. This can be suppressed by defining OPENVDB_USE_DEPRECATED_ABI_<VERSION>.
// Note that, whilst the VDB CMake does not allow this option to be hit,
// it exists to propagate this message to downstream targets
#ifndef OPENVDB_USE_DEPRECATED_ABI_8
#if OPENVDB_ABI_VERSION_NUMBER == 8
PRAGMA(message("NOTE: ABI = 8 is deprecated, define OPENVDB_USE_DEPRECATED_ABI_7 "
               "to suppress this message"))
#endif
#endif
#ifndef OPENVDB_USE_DEPRECATED_ABI_9
#if OPENVDB_ABI_VERSION_NUMBER == 9
PRAGMA(message("NOTE: ABI = 9 is deprecated, define OPENVDB_USE_DEPRECATED_ABI_8 "
               "to suppress this message"))
#endif
#endif

/// By default, the @b OPENVDB_REQUIRE_VERSION_NAME macro is undefined, and
/// symbols from the version namespace are promoted to the top-level namespace
/// so that, for example, @b openvdb::v5_0::io::File can be referred to
/// simply as @b openvdb::io::File.
///
/// When @b OPENVDB_REQUIRE_VERSION_NAME is defined, symbols must be
/// fully namespace-qualified.
/// @hideinitializer
#ifdef OPENVDB_REQUIRE_VERSION_NAME
#define OPENVDB_USE_VERSION_NAMESPACE
#else
// The empty namespace clause below ensures that OPENVDB_VERSION_NAME
// is recognized as a namespace name.
#define OPENVDB_USE_VERSION_NAMESPACE \
    namespace OPENVDB_VERSION_NAME    \
    {                                 \
    }                                 \
    using namespace OPENVDB_VERSION_NAME;
#endif

namespace openvdb
{
    OPENVDB_USE_VERSION_NAMESPACE
    namespace OPENVDB_VERSION_NAME
    {

        /// @brief The magic number is stored in the first four bytes of every VDB file.
        /// @details This can be used to quickly test whether we have a valid file or not.
        const int32_t OPENVDB_MAGIC = 0x56444220;

        /// Library major, minor and patch version numbers
        /// @hideinitializer
        const uint32_t
            OPENVDB_LIBRARY_MAJOR_VERSION = OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER,
            OPENVDB_LIBRARY_MINOR_VERSION = OPENVDB_LIBRARY_MINOR_VERSION_NUMBER,
            OPENVDB_LIBRARY_PATCH_VERSION = OPENVDB_LIBRARY_PATCH_VERSION_NUMBER;
        /// Library version number as a packed integer ("%02x%02x%04x", major, minor, patch)
        /// @hideinitializer
        const uint32_t OPENVDB_LIBRARY_VERSION = OPENVDB_LIBRARY_VERSION_NUMBER;
        /// ABI version number
        /// @hideinitializer
        const uint32_t OPENVDB_ABI_VERSION = OPENVDB_ABI_VERSION_NUMBER;

        /// @brief The current version number of the VDB file format
        /// @details  This can be used to enable various backwards compatibility switches
        /// or to reject files that cannot be read.
        const uint32_t OPENVDB_FILE_VERSION = 224;

        /// Notable file format version numbers
        enum
        {
            OPENVDB_FILE_VERSION_ROOTNODE_MAP = 213,
            OPENVDB_FILE_VERSION_INTERNALNODE_COMPRESSION = 214,
            OPENVDB_FILE_VERSION_SIMPLIFIED_GRID_TYPENAME = 215,
            OPENVDB_FILE_VERSION_GRID_INSTANCING = 216,
            OPENVDB_FILE_VERSION_BOOL_LEAF_OPTIMIZATION = 217,
            OPENVDB_FILE_VERSION_BOOST_UUID = 218,
            OPENVDB_FILE_VERSION_NO_GRIDMAP = 219,
            OPENVDB_FILE_VERSION_NEW_TRANSFORM = 219,
            OPENVDB_FILE_VERSION_SELECTIVE_COMPRESSION = 220,
            OPENVDB_FILE_VERSION_FLOAT_FRUSTUM_BBOX = 221,
            OPENVDB_FILE_VERSION_NODE_MASK_COMPRESSION = 222,
            OPENVDB_FILE_VERSION_BLOSC_COMPRESSION = 223,
            OPENVDB_FILE_VERSION_POINT_INDEX_GRID = 223,
            OPENVDB_FILE_VERSION_MULTIPASS_IO = 224
        };

        /// Return a library version number string of the form "<major>.<minor>.<patch>".
        inline constexpr const char *getLibraryVersionString() { return OPENVDB_LIBRARY_VERSION_STRING; }
        /// Return a library version number string of the form "<major>.<minor>.<patch>abi<abi>".
        inline constexpr const char *getLibraryAbiVersionString()
        {
            return OPENVDB_LIBRARY_ABI_VERSION_STRING;
        }
        inline constexpr const char *getPackageUrl() { return OPENVDB_PACKAGE_URL; }
        inline constexpr const char *getPackageRevision() { return OPENVDB_PACKAGE_REVISION; }

        struct VersionId
        {
            uint32_t first, second;
            VersionId() : first(0), second(0) {}
            VersionId(uint32_t major, uint32_t minor) : first(major), second(minor) {}
        };

    } // namespace OPENVDB_VERSION_NAME
} // namespace openvdb

#endif // OPENVDB_VERSION_HAS_BEEN_INCLUDED
