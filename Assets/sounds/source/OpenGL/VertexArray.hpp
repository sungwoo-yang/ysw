/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "GLConstants.hpp"
#include "GLTypes.hpp"
#include "Handle.hpp"
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace OpenGL
{
    /**
     * \brief Descriptive alias for OpenGL buffer object handles
     *
     * BufferHandle provides a more specific and readable name for the generic
     * OpenGL handle type when referring to buffer objects. This improves code
     * clarity without adding compile-time type safety.
     */
    using BufferHandle = Handle;

    /**
     * \brief Descriptive alias for OpenGL vertex array object handles
     *
     * VertexArrayHandle provides a more specific and readable name for the generic
     * OpenGL handle type when referring to vertex array objects (VAOs). This improves
     * code clarity without adding compile-time type safety.
     */
    using VertexArrayHandle = Handle;

    namespace Attribute
    {
        /**
         * \brief Compact vertex attribute descriptor for efficient OpenGL vertex specification
         *
         * Type provides a space-efficient way to describe vertex attributes by packing
         * all the necessary OpenGL vertex attribute information into a single 32-bit value.
         * This includes the data type, component count, size, normalization settings,
         * and instancing divisor values.
         *
         * The bit-packed design allows for:
         * - Efficient storage of attribute specifications
         * - Fast comparison and sorting of attribute layouts
         * - Compile-time computation of attribute configurations
         * - Support for both integer and float attribute types
         * - Instanced rendering through divisor values
         *
         * Bit Layout:
         * - Bits 15-0:  OpenGL component type (GL_FLOAT, GL_UNSIGNED_BYTE, etc.)
         * - Bits 18-16: Component count (1-4 components per attribute)
         * - Bits 23-19: Attribute size in bytes (1-16 bytes)
         * - Bit 24:     Normalization flag for integer-to-float conversion
         * - Bit 25:     Integer attribute flag (glVertexAttribIPointer vs glVertexAttribPointer)
         * - Bits 31-26: Instancing divisor (0-63) for per-instance attributes
         *
         * This design enables both standard per-vertex attributes and advanced
         * instanced rendering techniques while maintaining compatibility with
         * OpenGL's vertex attribute specification requirements.
         */
        struct Type
        {
            uint16_t GLType         : 16; // Bits 15-0   (16 bits): OpenGL component type (GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, etc.)
            uint8_t  ComponentCount : 3;  // Bits 18-16   (3 bits): Component count (1, 2, 3, 4)
            uint8_t  SizeBytes      : 5;  // Bits 23-19   (5 bits): Attribute size in bytes (max size is 16 - vec4)
            bool     Normalize      : 1;  // Bit  24      (1 bit) : Normalized flag (0 = false, 1 = true)
            bool     IntAttribute   : 1;  // Bit  25      (1 bit) : Integer attribute flag (0 = use glVertexAttribPointer, 1 = use glVertexAttribIPointer)
            uint8_t  Divisor        : 6;  // Bits 26-31   (6 bits): Divisor Value to support instancing (0-63)

            constexpr auto operator<=>(const Type&) const noexcept = default;

            /**
             * \brief Set the instancing divisor for per-instance attributes
             * \param divisor_value Divisor for instanced rendering (0 = per-vertex, >0 = per-instance)
             * \return Reference to this Type for method chaining
             *
             * Configures the attribute for instanced rendering by setting how frequently
             * the attribute advances during instanced drawing calls. A divisor of 0 means
             * the attribute advances once per vertex (standard behavior), while values
             * greater than 0 cause the attribute to advance once per N instances.
             *
             * Common divisor patterns:
             * - 0: Per-vertex data (positions, normals, texture coordinates)
             * - 1: Per-instance data (transformation matrices, colors, IDs)
             * - N: Per-N-instances data (shared data across multiple instances)
             *
             * This enables efficient instanced rendering where certain attributes
             * remain constant across multiple instances of the same geometry.
             */
            constexpr Type& WithDivisor(uint8_t divisor_value) noexcept
            {
                Divisor = divisor_value & 0x3F; // only 6 bits
                return *this;
            }
        };

        static_assert(sizeof(Type) == sizeof(uint32_t));
    }

    /**
     * \brief Layout specification for vertex attributes within a buffer
     *
     * BufferLayout describes how vertex attributes are organized within a single
     * buffer object, including their types, order, and optional starting offset.
     * This enables flexible vertex data organization and supports interleaved
     * vertex formats, multiple attribute streams, and complex data layouts.
     *
     * The layout system supports:
     * - Interleaved vertex data (position, normal, texture coordinates in sequence)
     * - Multiple attribute streams from the same buffer
     * - Buffer sub-regions with custom starting offsets
     * - Mixed attribute types and sizes within the same buffer
     *
     * Common vertex layout patterns:
     * - Simple: {Float3, Float2} for position + texture coordinates
     * - Complete: {Float3, Float3, Float2} for position + normal + UV
     * - Packed: {UByte4ToNormalized} for compressed color attributes
     * - Instanced: Mix of per-vertex and per-instance attributes
     *
     * The starting byte offset enables using sub-regions of larger buffers
     * or skipping headers in complex buffer formats.
     */
    struct BufferLayout
    {
        /** \brief Byte offset from buffer start where attribute data begins */
        uint32_t BufferStartingByteOffset = 0;

        /** \brief Ordered list of attribute types in this buffer layout */
        std::vector<Attribute::Type> Attributes{};

        BufferLayout() = default;

        /**
         * \brief Create layout with attributes starting at buffer beginning
         * \param attributes Initializer list of attribute types in order
         */
        BufferLayout(std::initializer_list<Attribute::Type> attributes) : BufferStartingByteOffset{ 0 }, Attributes{ attributes }
        {
        }

        /**
         * \brief Create layout with custom starting offset
         * \param starting_byte_offset Byte offset from buffer start
         * \param attributes Initializer list of attribute types in order
         */
        BufferLayout(uint32_t starting_byte_offset, std::initializer_list<Attribute::Type> attributes) : BufferStartingByteOffset{ starting_byte_offset }, Attributes{ attributes }
        {
        }
    };

    /**
     * \brief Complete vertex buffer specification with handle and layout information
     *
     * VertexBuffer pairs an OpenGL buffer object with its corresponding layout
     * description, providing all the information needed to configure vertex
     * attributes for rendering. This combination ensures that buffer data
     * and its interpretation are kept together as a cohesive unit.
     *
     * The structure enables:
     * - Self-describing vertex buffers with embedded layout information
     * - Easy sharing of buffer configurations across rendering operations
     * - Type-safe vertex attribute setup through layout specifications
     * - Support for multiple vertex streams with different layouts
     *
     * Usage patterns:
     * - Single buffer with interleaved vertex data
     * - Multiple buffers with different attribute types
     * - Instanced rendering with per-vertex and per-instance streams
     * - Dynamic vertex buffers with consistent layouts
     *
     * The layout information is used during Vertex Array Object creation
     * to automatically configure the appropriate vertex attribute pointers
     * and enable the correct attribute locations.
     */
    struct VertexBuffer
    {
        /** \brief Handle to the OpenGL buffer object containing vertex data */
        BufferHandle Handle{ 0 };

        /** \brief Layout specification describing how attributes are organized */
        BufferLayout Layout{};
    };

    /**
     * \brief Create Vertex Array Object (VAO) from multiple vertex buffers
     * \param vertices Initializer list of vertex buffers with their layouts
     * \param index_buffer Optional element buffer for indexed rendering (default: 0)
     * \return Handle to the created and configured Vertex Array Object
     *
     * Creates a complete Vertex Array Object that encapsulates the vertex attribute
     * configuration for multiple vertex buffers. This enables complex vertex setups
     * with multiple attribute streams, different data types, and sophisticated
     * rendering techniques like instanced rendering.
     *
     * The function performs comprehensive VAO setup:
     * - Creates and binds a new Vertex Array Object
     * - Configures vertex attributes for each buffer according to its layout
     * - Calculates appropriate strides and offsets for interleaved data
     * - Sets up instancing divisors for per-instance attributes
     * - Binds optional index buffer for indexed rendering
     * - Enables all configured vertex attribute arrays
     *
     * Multi-buffer capabilities:
     * - Separate buffers for different attribute types (positions, normals, UVs)
     * - Mixed per-vertex and per-instance attribute streams
     * - Different data formats optimized for specific attribute types
     * - Independent update frequencies for dynamic vs. static data
     *
     * Attribute configuration:
     * Each buffer's layout is processed to determine the correct OpenGL vertex
     * attribute setup, including proper use of glVertexAttribPointer() for
     * floating-point data and glVertexAttribIPointer() for integer data.
     *
     * The resulting VAO can be bound once for rendering, eliminating the need
     * to reconfigure vertex attributes on every draw call.
     */
    VertexArrayHandle CreateVertexArrayObject(std::initializer_list<VertexBuffer> vertices, BufferHandle index_buffer = 0);

    /**
     * \brief Create Vertex Array Object (VAO) from a single vertex buffer
     * \param vertices Single vertex buffer with its layout specification
     * \param index_buffer Optional element buffer for indexed rendering (default: 0)
     * \return Handle to the created and configured Vertex Array Object
     *
     * Creates a Vertex Array Object for the common case of a single vertex buffer
     * containing all required vertex attributes. This is a convenience wrapper
     * around the multi-buffer version, ideal for simple rendering scenarios
     * with interleaved vertex data.
     *
     * Single-buffer advantages:
     * - Simplified memory management with one buffer object
     * - Better cache coherency with interleaved vertex data
     * - Reduced OpenGL state changes during attribute setup
     * - Lower memory overhead for simple vertex formats
     *
     * Common interleaved patterns:
     * - Position + Color: {Float3, Float4} or {Float2, UByte4ToNormalized}
     * - Position + UV: {Float3, Float2} for textured geometry
     * - Complete vertex: {Float3, Float3, Float2} for position + normal + UV
     * - Sprite data: {Float2, Float2} for position + texture coordinates
     *
     * The function delegates to the multi-buffer version with a single-element
     * initializer list, ensuring consistent behavior and implementation while
     * providing a cleaner API for simple use cases.
     */
    VertexArrayHandle CreateVertexArrayObject(VertexBuffer vertices, BufferHandle index_buffer = 0);

    namespace Attribute
    {
        namespace details
        {
            // Constants for encoding
            constexpr bool NORMALIZE    = true;
            constexpr bool NO_NORMALIZE = false;
            constexpr bool TO_INT       = true;  // Use glVertexAttribIPointer
            constexpr bool TO_FLOAT     = false; // Use glVertexAttribPointer

        }

        /**
         * \brief Predefined vertex attribute types for common data formats
         *
         * This collection provides pre-configured attribute types for all common
         * vertex data formats, eliminating the need to manually specify OpenGL
         * types, component counts, and conversion settings. Each attribute type
         * is optimized for its specific use case and shader input requirements.
         *
         * Naming Convention:
         * - Base types: Bool, Byte, Short, Int, UByte, UShort, UInt, Float
         * - Vector types: Type2, Type3, Type4 (e.g., Float2, Int3, UByte4)
         * - Conversions: TypeToFloat, TypeToNormalized (e.g., ByteToFloat, UByteToNormalized)
         *
         * Conversion Types:
         * - ToFloat: Convert integer types to float without normalization
         * - ToNormalized: Convert integer types to normalized float ranges
         *   - Signed types: [-1, 1] range (Byte, Short, Int)
         *   - Unsigned types: [0, 1] range (UByte, UShort, UInt)
         *
         * Memory Optimization:
         * - Use smaller integer types (Byte, UByte) for packed data
         * - Use normalized conversions for color and normal data
         * - Use native Float types for precise calculations
         *
         * Common Usage Patterns:
         * - Positions: Float2, Float3
         * - Colors: UByte4ToNormalized (compact), Float4 (precise)
         * - Normals: Float3, Byte3ToNormalized (compact)
         * - Texture Coordinates: Float2
         * - Indices: UShort, UInt (in index buffers)
         *
         * Instancing Support:
         * All attribute types can be modified with .WithDivisor(N) to create
         * per-instance attributes for instanced rendering techniques.
         */

        constexpr Type None                = { 0, 0, 0, 0, 0, 0 };
        constexpr Type Bool                = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool -> bool
        constexpr Type Bool2               = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[2] -> bvec2
        constexpr Type Bool3               = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[3] -> bvec3
        constexpr Type Bool4               = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[4] -> bvec4
        constexpr Type Byte                = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t -> int
        constexpr Type Byte2               = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[2] -> ivec2
        constexpr Type Byte2ToFloat        = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[2] -> vec2
        constexpr Type Byte2ToNormalized   = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[2] -> vec2 [-1, 1]
        constexpr Type Byte3               = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[3] -> ivec3
        constexpr Type Byte3ToFloat        = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[3] -> vec3
        constexpr Type Byte3ToNormalized   = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[3] -> vec3 [-1, 1]
        constexpr Type Byte4               = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[4] -> ivec4
        constexpr Type Byte4ToFloat        = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[4] -> vec4
        constexpr Type Byte4ToNormalized   = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[4] -> vec4 [-1, 1]
        constexpr Type ByteToFloat         = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t -> float
        constexpr Type ByteToNormalized    = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t -> float [-1, 1]
        constexpr Type Float               = { GL_FLOAT, 1, 1 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float -> float
        constexpr Type Float2              = { GL_FLOAT, 2, 2 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[2] -> vec2
        constexpr Type Float3              = { GL_FLOAT, 3, 3 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[3] -> vec3
        constexpr Type Float4              = { GL_FLOAT, 4, 4 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[4] -> vec4
        constexpr Type Int                 = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int -> int
        constexpr Type Int2                = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[2] -> ivec2
        constexpr Type Int2ToFloat         = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[2] -> vec2
        constexpr Type Int2ToNormalized    = { GL_INT, 2, 2 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[2] -> vec2 [-1, 1]
        constexpr Type Int3                = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[3] -> ivec3
        constexpr Type Int3ToFloat         = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[3] -> vec3
        constexpr Type Int3ToNormalized    = { GL_INT, 3, 3 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[3] -> vec3 [-1, 1]
        constexpr Type Int4                = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[4] -> ivec4
        constexpr Type Int4ToFloat         = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[4] -> vec4
        constexpr Type Int4ToNormalized    = { GL_INT, 4, 4 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[4] -> vec4 [-1, 1]
        constexpr Type IntToFloat          = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int -> float
        constexpr Type IntToNormalized     = { GL_INT, 1, 1 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int -> float [-1, 1]
        constexpr Type Short               = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short -> int
        constexpr Type Short2              = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[2] -> ivec2
        constexpr Type Short2ToFloat       = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[2] -> vec2
        constexpr Type Short2ToNormalized  = { GL_SHORT, 2, 2 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[2] -> vec2 [-1, 1]
        constexpr Type Short3              = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[3] -> ivec3
        constexpr Type Short3ToFloat       = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[3] -> vec3
        constexpr Type Short3ToNormalized  = { GL_SHORT, 3, 3 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[3] -> vec3 [-1, 1]
        constexpr Type Short4              = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[4] -> ivec4
        constexpr Type Short4ToFloat       = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[4] -> vec4
        constexpr Type Short4ToNormalized  = { GL_SHORT, 4, 4 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[4] -> vec4 [-1, 1]
        constexpr Type ShortToFloat        = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short -> float
        constexpr Type ShortToNormalized   = { GL_SHORT, 1, 1 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short -> float [-1, 1]
        constexpr Type UByte               = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t -> uint
        constexpr Type UByte2              = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[2] -> uvec2
        constexpr Type UByte2ToFloat       = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[2] -> vec2
        constexpr Type UByte2ToNormalized  = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[2] -> vec2 [0, 1]
        constexpr Type UByte3              = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[3] -> uvec3
        constexpr Type UByte3ToFloat       = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[3] -> vec3
        constexpr Type UByte3ToNormalized  = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[3] -> vec3 [0, 1]
        constexpr Type UByte4              = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[4] -> uvec4
        constexpr Type UByte4ToFloat       = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[4] -> vec4
        constexpr Type UByte4ToNormalized  = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[4] -> vec4 [0, 1]
        constexpr Type UByteToFloat        = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t -> float
        constexpr Type UByteToNormalized   = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t -> float [0, 1]
        constexpr Type UInt                = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint -> uint
        constexpr Type UInt2               = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[2] -> uvec2
        constexpr Type UInt2ToFloat        = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[2] -> vec2
        constexpr Type UInt2ToNormalized   = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[2] -> vec2 [0, 1]
        constexpr Type UInt3               = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[3] -> uvec3
        constexpr Type UInt3ToFloat        = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[3] -> vec3
        constexpr Type UInt3ToNormalized   = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[3] -> vec3 [0, 1]
        constexpr Type UInt4               = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[4] -> uvec4
        constexpr Type UInt4ToFloat        = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[4] -> vec4
        constexpr Type UInt4ToNormalized   = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[4] -> vec4 [0, 1]
        constexpr Type UIntToFloat         = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint -> float
        constexpr Type UIntToNormalized    = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint -> float [0, 1]
        constexpr Type UShort              = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort -> uint
        constexpr Type UShort2             = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[2] -> uvec2
        constexpr Type UShort2ToFloat      = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[2] -> vec2
        constexpr Type UShort2ToNormalized = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[2] -> vec2 [0, 1]
        constexpr Type UShort3             = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[3] -> uvec3
        constexpr Type UShort3ToFloat      = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[3] -> vec3
        constexpr Type UShort3ToNormalized = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[3] -> vec3 [0, 1]
        constexpr Type UShort4             = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[4] -> uvec4
        constexpr Type UShort4ToFloat      = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[4] -> vec4
        constexpr Type UShort4ToNormalized = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[4] -> vec4 [0, 1]
        constexpr Type UShortToFloat       = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort -> float
        constexpr Type UShortToNormalized  = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort -> float [0, 1]
    };


}
