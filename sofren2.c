#ifndef SFT_H
#define SFT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif


/*================================================================
:: TYPE PROTOTYPES
================================================================*/

#ifdef SFT_PREFIXED_TYPES
    #define i8  sfri8_t
    #define u8  sfru8_t
    #define i16 sfri16_t
    #define u16 sfru16_t
    #define i32 sfri32_t
    #define u32 sfru32_t
    #define i64 sfri64_t
    #define u64 sfru64_t
    #define f32 sfrf32_t
    #define f64 sfrf64_t
    #define vf32 sfrvf32_t
    #define vi32 sfrvi32_t
#endif

#ifndef SFT_NO_STDINT
    #include <stdint.h>
    typedef int8_t   i8;
    typedef uint8_t  u8;
    typedef int16_t  i16;
    typedef uint16_t u16;
    typedef int32_t  i32;
    typedef uint32_t u32;
    typedef int64_t  i64;
    typedef uint64_t u64;
#else
    typedef signed char        i8;
    typedef unsigned char      u8;
    typedef signed short       i16;
    typedef unsigned short     u16;
    typedef signed int         i32;
    typedef unsigned int       u32;
    typedef signed long long   i64;
    typedef unsigned long long u64;
#endif
typedef float  f32;
typedef double f64;

// public enums
enum sft_debug_mode {
    SFT_DEBUG_NONE,
    SFT_DEBUG_WIREFRAME,
    SFT_DEBUG_TILES,        // grid outlines over tiles
    SFT_DEBUG_DEPTH,        // normalized tile depth visualization
    SFT_DEBUG_LIGHT_COUNT,  // heatmap of active point lights per tile
    SFT_DEBUG_CHECKERBOARD, // route all textures to the base checkerboard
    SFT_DEBUG_COUNT
};

enum sft_dmesh_type {
    SFT_DMESH_QUADS,
    SFT_DMESH_TRIANGLES
};

// public struct prototypes
typedef struct sftvec   sftvec;
typedef struct sftvvec  sftvvec;
typedef struct sftquat  sftquat;
typedef struct sftvquat sftvquat;
typedef union  sftmat   sftmat;
typedef union  sftvmat  sftvmat;

typedef struct sft_ray_hit sft_ray_hit;

typedef struct sft_arena sft_arena;

typedef struct sft_mesh     sft_mesh;
typedef struct sft_model    sft_model;
typedef struct sft_pose     sft_pose;
typedef struct sft_dmesh    sft_dmesh;
typedef struct sft_dmesh_qb sft_dmesh_qb;
typedef struct sft_texture  sft_texture;
typedef struct sft_material sft_material;
typedef struct sft_instance sft_instance;

typedef struct sft_directional_light sft_directional_light;
typedef struct sft_point_light       sft_point_light;

typedef u32 sft_tex_handle;
typedef u32 sft_mat_handle;

#define SFT_INVALID_HANDLE 0xFFFFFFFF

#define SFT_HANDLE_INDEX_MASK 0x0000FFFF
#define SFT_HANDLE_GEN_SHIFT  16
#define SFT_HANDLE_GEN_MASK   0xFFFF0000

#define SFT_HANDLE_INDEX(h) ((h) & SFT_HANDLE_INDEX_MASK)

// material handle layout:
// [31: Transient Flag] | [16-30: generation (static only)] | [0-15: ind]
#define SFT_MAT_TRANSIENT_BIT 0x80000000
#define SFT_MAT_GEN_SHIFT     16
#define SFT_MAT_GEN_MASK      0x7FFF0000
#define SFT_MAT_INDEX_MASK    0x0000FFFF

#define SFT_MAT_IS_TRANSIENT(h) (0 != ((h) & SFT_MAT_TRANSIENT_BIT))
#define SFT_MAT_INDEX(h)        ((h) & SFT_MAT_INDEX_MASK)


/*================================================================
:: MISC MACROS
================================================================*/

#define SFT_PI (3.14159265358979323846)
#define SFT_EPSILON (1e-6f)

#define SFT_V(a, b, c) (sftvec){ .x = (a), .y = (b), .z = (c) }

#define SFT_ARRLEN(_arr)  (sizeof(_arr) / sizeof((_arr)[0]))
#define SFT_CLAMP(_x, _min, _max) ((_x) < (_min) ? (_min) : ((_x) > (_max) ? (_max) : (_x)))
#define SFT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SFT_MAX(a, b) ((a) > (b) ? (a) : (b))

#define SFT_MAT_IDENTITY (sftmat){ .m = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} }

#define SFT_LERPF(_a, _b, _t) ((_a) + (_t) * ((_b) - (_a)))
#define SFT_SWAP(type, _a, _b) { type _swap_temp = (_a); (_a) = (_b); (_b) = _swap_temp; }

#ifndef SFT_SHADOW_LIGHT_SIZE
    // physical radius of the light source in world units (e.g. 0.4m)
    // larger values widen the penumbra transition zone
    #define SFT_SHADOW_LIGHT_SIZE 0.1f
#endif

// prime numbers for spatial hashing
#define SFT_HASH_PRIME_X 73856093
#define SFT_HASH_PRIME_Y 19349663
#define SFT_HASH_PRIME_Z 83492791

// cosine of 100 degrees, if the dot product of two normals is less than this,
// the angle between them is strictly greater than 100 degrees
#define SFT_CONE_LIMIT (-0.173648f)

// must total 31 to reserve bit 31 for clipping flag
#ifndef SFT_INSTANCE_BITS
    #define SFT_INSTANCE_BITS 6
#endif
#ifndef SFT_MESHLET_BITS
    #define SFT_MESHLET_BITS 18
#endif
#define SFT_TRIANGLE_BITS 7
#if 31 != (SFT_INSTANCE_BITS + SFT_MESHLET_BITS + SFT_TRIANGLE_BITS)
    #error "SFT ERROR: SFT_INSTANCE_BITS + SFT_MESHLET_BITS + SFT_TRIANGLE_BITS must equal 31"
#endif
#define SFT_MAX_INSTANCES_PER_FRAME (1 << SFT_INSTANCE_BITS)

#define SFT_VERTS_PER_MESHLET 64

#define SFT_MESHLETS_PER_JOB 8

#define SFT_CHUNK_CAPACITY 63

#define SFT_TILE_SHIFT 6
#define SFT_TILE_SIZE (1 << SFT_TILE_SHIFT)

#define SFT_INVALID_BITMASK_IND ((u64)-1)

#if defined(_WIN32)
    #include <malloc.h>
    #define SFT_ALIGNED_ALLOC(size, alignment) _aligned_malloc((size), (alignment))
    #define SFT_ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
    #define SFT_ALIGNED_ALLOC(size, alignment) aligned_alloc((alignment), (size))
    #define SFT_ALIGNED_FREE(ptr) free(ptr)
#endif

#define SFT_CALLOC(r, count, type) \
    (r) = (type*)calloc((count), sizeof(type)); \
    if (!(r)) { SFT_ERR_EXIT("calloc failed (%d bytes)\n", (i32)((count) * sizeof(type))); } \

#define SFT_FREE(p) \
    if (p) { free(p); (p) = NULL; }

#ifndef SFT_FUNC
    #define SFT_FUNC static
#endif

#ifndef SFT_INLINE
    #if defined(_MSC_VER)
        #define SFT_INLINE __forceinline
    #elif defined(__GNUC__) || defined(__clang__)
        #define SFT_INLINE inline __attribute__((always_inline))
    #else
        #define SFT_INLINE inline
    #endif
#endif

#if defined(_MSC_VER)
    #define SFT_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
    #define SFT_THREAD_LOCAL __thread
#else
    #define SFT_THREAD_LOCAL _Thread_local
#endif

#define SFT_ERR_EXIT(...) { \
    fprintf(stderr, "fatal sofren2 (%s) error at line %d in function '%s':\n\t", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, __VA_ARGS__); \
    exit(1); \
}
#define SFT_ERR_RET(_r, ...) { \
    fprintf(stderr, "sofren2 (%s) error at line %d in function '%s':\n\t", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, __VA_ARGS__); \
    return _r; \
}

#ifdef SFT_NO_DEBUG
    #define SFT_PRINT(...)
#else
    #define SFT_PRINT(...) { \
        fprintf(stdout, "[sofren2]: "); \
        fprintf(stdout, __VA_ARGS__); \
    }
#endif


/*================================================================
:: SIMD WRAPPERS
================================================================*/

#if defined(__AVX2__)
    #include <immintrin.h>
    
    #define SFT_SIMD_WIDTH 8
    #define SFT_BLOCK_X 4
    #define SFT_BLOCK_Y 2
    typedef __m256  vf32;
    typedef __m256i vi32;

    #define sft_vf_ddx(v) _mm256_sub_ps((v), _mm256_permute_ps((v), _MM_SHUFFLE(2, 3, 0, 1)))
    #define sft_vf_ddy(v) _mm256_sub_ps((v), _mm256_permute2f128_ps((v), (v), 0x01))

    // ---  vf32 base and memory  ---
    #define sft_vf_setzero()            _mm256_setzero_ps()
    #define sft_vf_set1(v)              _mm256_set1_ps(v)
    #define sft_vf_load(ptr)            _mm256_load_ps(ptr)
    #define sft_vf_loadu(ptr)           _mm256_loadu_ps(ptr)
    #define sft_vf_store(ptr, val)      _mm256_store_ps((ptr), (val))
    #define sft_vf_storeu(ptr, val)     _mm256_storeu_ps((ptr), (val))
    #define sft_vf_gather(base, vind)   _mm256_i32gather_ps((const f32*)(base), (vind), 4)

    // ---  vf32 arithmetic  ---
    #define sft_vf_add(a, b)            _mm256_add_ps((a), (b))
    #define sft_vf_sub(a, b)            _mm256_sub_ps((a), (b))
    #define sft_vf_mul(a, b)            _mm256_mul_ps((a), (b))
    #define sft_vf_div(a, b)            _mm256_div_ps((a), (b))
    #define sft_vf_rcp(a)               _mm256_rcp_ps(a)
    #define sft_vf_min(a, b)            _mm256_min_ps((a), (b))
    #define sft_vf_max(a, b)            _mm256_max_ps((a), (b))
    #define sft_vf_fmadd(a, b, c)       _mm256_fmadd_ps((a), (b), (c))
    #define sft_vf_fnmadd(a, b, c)      _mm256_fnmadd_ps((a), (b), (c))
    #define sft_vf_fmsub(a, b, c)       _mm256_fmsub_ps((a), (b), (c))
    #define sft_vf_sqrt(a)              _mm256_sqrt_ps(a)
    #define sft_vf_rsqrt(a)             _mm256_rsqrt_ps(a)
    #define sft_vf_floor(v)             _mm256_floor_ps(v)

    // ---  vf32 bitwise and logic  ---
    #define sft_vf_and(a, b)            _mm256_and_ps((a), (b))
    #define sft_vf_or(a, b)             _mm256_or_ps((a), (b))
    #define sft_vf_andnot(a, b)         _mm256_andnot_ps((a), (b))
    
    #define sft_vf_cmp_lt(a, b)         _mm256_cmp_ps((a), (b), _CMP_LT_OQ)
    #define sft_vf_cmp_le(a, b)         _mm256_cmp_ps((a), (b), _CMP_LE_OQ)
    #define sft_vf_cmp_gt(a, b)         _mm256_cmp_ps((a), (b), _CMP_GT_OQ)
    #define sft_vf_cmp_ge(a, b)         _mm256_cmp_ps((a), (b), _CMP_GE_OQ)
    #define sft_vf_cmp_eq(a, b)         _mm256_cmp_ps((a), (b), _CMP_EQ_OQ)
    #define sft_vf_cmp_ne(a, b)         _mm256_cmp_ps((a), (b), _CMP_NEQ_OQ)
    
    #define sft_vf_blendv(a, b, m)      _mm256_blendv_ps((b), (a), (m))
    #define sft_vf_movemask(a)          _mm256_movemask_ps(a)

    #define sft_vf_cvtepi32(v)         _mm256_cvtepi32_ps(v)

    static SFT_INLINE i32 sft_vi_hmin(vi32 v) {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i m1 = _mm_min_epi32(lo, hi);
        __m128i shuf1 = _mm_shuffle_epi32(m1, _MM_SHUFFLE(2,3,0,1));
        __m128i m2 = _mm_min_epi32(m1, shuf1);
        __m128i shuf2 = _mm_shuffle_epi32(m2, _MM_SHUFFLE(1,0,3,2));
        __m128i m3 = _mm_min_epi32(m2, shuf2);
        return _mm_cvtsi128_si32(m3);
    }
    static SFT_INLINE i32 sft_vi_hmax(vi32 v) {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i m1 = _mm_max_epi32(lo, hi);
        __m128i shuf1 = _mm_shuffle_epi32(m1, _MM_SHUFFLE(2,3,0,1));
        __m128i m2 = _mm_max_epi32(m1, shuf1);
        __m128i shuf2 = _mm_shuffle_epi32(m2, _MM_SHUFFLE(1,0,3,2));
        __m128i m3 = _mm_max_epi32(m2, shuf2);
        return _mm_cvtsi128_si32(m3);
    }

    // ---  vi32 base and memory  ---
    #define sft_vi_setzero()            _mm256_setzero_si256()
    #define sft_vi_set1(v)              _mm256_set1_epi32(v)
    #define sft_vi_load(ptr)            _mm256_load_si256((const vi32*)(ptr))
    #define sft_vi_store(ptr, val)      _mm256_store_si256((vi32*)(ptr), (val))
    #define sft_vi_maskstore(p, m, v)   _mm256_maskstore_epi32((i32 *)(p), (m), (v))
    #define sft_vi_loadu_8u_to_32i(ptr) _mm256_cvtepu8_epi32(_mm_loadl_epi64((const __m128i*)(ptr)))
    static SFT_INLINE i32 sft_vi_extract(vi32 v, i32 lane) {
        switch(lane) {
            case 0: return _mm256_extract_epi32(v, 0);
            case 1: return _mm256_extract_epi32(v, 1);
            case 2: return _mm256_extract_epi32(v, 2);
            case 3: return _mm256_extract_epi32(v, 3);
            case 4: return _mm256_extract_epi32(v, 4);
            case 5: return _mm256_extract_epi32(v, 5);
            case 6: return _mm256_extract_epi32(v, 6);
            case 7: return _mm256_extract_epi32(v, 7);
            default: return 0;
        }
    }
    #define sft_vi_gather(base, vind)   _mm256_i32gather_epi32((const int*)(base), (vind), 4)

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            _mm256_add_epi32((a), (b))
    #define sft_vi_sub(a, b)            _mm256_sub_epi32((a), (b))
    #define sft_vi_max(a, b)            _mm256_max_epi32((a), (b))
    #define sft_vi_min(a, b)            _mm256_min_epi32((a), (b))
    #define sft_vi_and(a, b)            _mm256_and_si256((a), (b))
    #define sft_vi_or(a, b)             _mm256_or_si256((a), (b))
    #define sft_vi_andnot(a, b)         _mm256_andnot_si256((a), (b))
    #define sft_vi_srai(a, imm)         _mm256_srai_epi32((a), (imm))
    #define sft_vi_srli(a, imm)         _mm256_srli_epi32((a), (imm))
    #define sft_vi_slli(a, imm)         _mm256_slli_epi32((a), (imm))
    #define sft_vi_movemask(a)          _mm256_movemask_ps(_mm256_castsi256_ps(a))
    #define sft_vi_mullo(a, b)          _mm256_mullo_epi32((a), (b))

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         _mm256_cmpgt_epi32((a), _mm256_sub_epi32((b), _mm256_set1_epi32(1)))
    #define sft_vi_cmp_gt(a, b)         _mm256_cmpgt_epi32((a), (b))
    #define sft_vi_cmp_le(a, b)         _mm256_cmpgt_epi32(_mm256_add_epi32((b), _mm256_set1_epi32(1)), (a))
    #define sft_vi_cmp_lt(a, b)         _mm256_cmpgt_epi32((b), (a))
    #define sft_vi_cmp_eq(a, b)         _mm256_cmpeq_epi32((a), (b))
    #define sft_vi_cmp_ne(a, b)         _mm256_xor_si256(_mm256_cmpeq_epi32((a), (b)), _mm256_set1_epi32(-1))

    // ---  casting  ---
    #define sft_vf_cast_vi32(a)         _mm256_castsi256_ps(a)
    #define sft_vi_cast_vf32(a)         _mm256_castps_si256(a)
    #define sft_vi_cvttps_epi32(v)      _mm256_cvttps_epi32(v)


#elif defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    
    #define SFT_SIMD_WIDTH 4
    #define SFT_BLOCK_X 2
    #define SFT_BLOCK_Y 2
    typedef float32x4_t vf32;
    typedef int32x4_t   vi32;

    #define sft_vf_ddx(v) vsubq_f32((v), vrev64q_f32(v))
    #define sft_vf_ddy(v) vsubq_f32((v), vextq_f32((v), (v), 2))
    
    // ---  vf32 base and memory  ---
    #define sft_vf_setzero()            vdupq_n_f32(0.f)
    #define sft_vf_set1(v)              vdupq_n_f32(v)
    #define sft_vf_load(ptr)            vld1q_f32(ptr)
    #define sft_vf_loadu(ptr)           vld1q_f32(ptr)
    #define sft_vf_store(ptr, val)      vst1q_f32((ptr), (val))
    #define sft_vf_storeu(ptr, val)     vst1q_f32((ptr), (val))
    static SFT_INLINE vf32 sft_vf_gather(const f32 *base, vi32 ind) {
        return (vf32){
            base[vgetq_lane_s32(ind, 0)],
            base[vgetq_lane_s32(ind, 1)],
            base[vgetq_lane_s32(ind, 2)],
            base[vgetq_lane_s32(ind, 3)]
        };
    }

    // ---  vf32 arithmetic  ---
    #define sft_vf_add(a, b)            vaddq_f32((a), (b))
    #define sft_vf_sub(a, b)            vsubq_f32((a), (b))
    #define sft_vf_mul(a, b)            vmulq_f32((a), (b))
    #define sft_vf_div(a, b)            vdivq_f32((a), (b)) // AArch64
    #define sft_vf_rcp(a)               vrecpeq_f32(a)
    #define sft_vf_min(a, b)            vminq_f32((a), (b))
    #define sft_vf_max(a, b)            vmaxq_f32((a), (b))
    #define sft_vf_fmadd(a, b, c)       vfmaq_f32((c), (a), (b))
    #define sft_vf_fnmadd(a, b, c)      vmlsq_f32((c), (a), (b))
    #define sft_vf_fmsub(a, b, c)       vfmsq_f32((c), (a), (b))
    #define sft_vf_sqrt(a)              vsqrtq_f32(a)
    #define sft_vf_rsqrt(a)             vrsqrteq_f32(a)
    #define sft_vf_floor(v)             vrndmq_f32(v)

    // ---  vf32 bitwise and logic  ---
    static SFT_INLINE vf32 sft_vf_and(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }
    static SFT_INLINE vf32 sft_vf_or(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }
    static SFT_INLINE vf32 sft_vf_andnot(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(b), vreinterpretq_u32_f32(a)));
    }

    #define sft_vf_cmp_lt(a, b)         vreinterpretq_f32_u32(vcltq_f32((a), (b)))
    #define sft_vf_cmp_le(a, b)         vreinterpretq_f32_u32(vcleq_f32((a), (b)))
    #define sft_vf_cmp_gt(a, b)         vreinterpretq_f32_u32(vcgtq_f32((a), (b)))
    #define sft_vf_cmp_ge(a, b)         vreinterpretq_f32_u32(vcgeq_f32((a), (b)))
    #define sft_vf_cmp_eq(a, b)         vreinterpretq_f32_u32(vceqq_f32((a), (b)))
    #define sft_vf_cmp_ne(a, b)         vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32((a), (b))))

    static SFT_INLINE vf32 sft_vf_blendv(vf32 a, vf32 b, vf32 mask) {
        return vbslq_f32(vreinterpretq_u32_f32(mask), a, b);
    }
    static SFT_INLINE i32 sft_vf_movemask(vf32 a) {
        uint32x4_t sgn = vshrq_n_u32(vreinterpretq_u32_f32(a), 31);
        const uint32x4_t bit_weights = {1, 2, 4, 8};
        sgn = vmulq_u32(sgn, bit_weights);
        return vaddvq_u32(sgn);
    }

    #define sft_vf_cvtepi32(v)          vcvtq_f32_s32(v)

    #define sft_vi_hmin(v)              vminvq_s32(v)
    #define sft_vi_hmax(v)              vmaxvq_s32(v)

    // ---  vi32 base and memory  ---
    #define sft_vi_setzero()            vdupq_n_s32(0)
    #define sft_vi_set1(v)              vdupq_n_s32(v)
    #define sft_vi_load(ptr)            vld1q_s32((const i32*)(ptr))
    #define sft_vi_store(ptr, val)      vst1q_s32((i32*)(ptr), (val))
    static SFT_INLINE void sft_vi_maskstore(i32 *ptr, vi32 mask, vi32 val) {
        const vi32 old = vld1q_s32(ptr);
        vst1q_s32(ptr, vbslq_s32(vreinterpretq_u32_s32(mask), val, old));
    }
    static SFT_INLINE vi32 sft_vi_loadu_8u_to_32i(const u8 *ptr) {
        return vreinterpretq_s32_u32(vmovl_u16(vmovl_u8(vld1_u8(ptr))));
    }
    static SFT_INLINE i32 sft_vi_extract(vi32 v, i32 lane) {
        switch(lane) {
            case 0: return vgetq_lane_s32(v, 0);
            case 1: return vgetq_lane_s32(v, 1);
            case 2: return vgetq_lane_s32(v, 2);
            case 3: return vgetq_lane_s32(v, 3);
            default: return 0;
        }
    }
    static SFT_INLINE vi32 sft_vi_gather(const i32 *base, vi32 ind) {
        return (vi32){ base[vgetq_lane_s32(ind, 0)], base[vgetq_lane_s32(ind, 1)], base[vgetq_lane_s32(ind, 2)], base[vgetq_lane_s32(ind, 3)] };
    }

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            vaddq_s32((a), (b))
    #define sft_vi_sub(a, b)            vsubq_s32((a), (b))
    #define sft_vi_max(a, b)            vmaxq_s32((a), (b))
    #define sft_vi_min(a, b)            vminq_s32((a), (b))
    #define sft_vi_and(a, b)            vandq_s32((a), (b))
    #define sft_vi_or(a, b)             vorrq_s32((a), (b))
    #define sft_vi_andnot(a, b)         vbicq_s32((b), (a))
    #define sft_vi_srai(a, imm)         vshrq_n_s32((a), (imm))
    #define sft_vi_srli(a, imm)         vshrq_n_u32((a), (imm))
    // TODO CHECK #define sft_vi_srli(a, imm) vreinterpretq_s32_u32(vshrq_n_u32(vreinterpretq_u32_s32(a), (imm)))
    #define sft_vi_slli(a, imm)         vshlq_n_s32((a), (imm))
    static SFT_INLINE i32 sft_vi_movemask(vi32 a) {
        const uint32x4_t sgn = vshrq_n_u32(vreinterpretq_u32_s32(a), 31);
        const uint32x4_t bit_weights = {1, 2, 4, 8};
        return vaddvq_u32(vmulq_u32(sgn, bit_weights));
    }
    #define sft_vi_mullo(a, b)          vmulq_s32((a), (b))

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         vreinterpretq_s32_u32(vcgeq_s32((a), (b)))
    #define sft_vi_cmp_gt(a, b)         vreinterpretq_s32_u32(vcgtq_s32((a), (b)))
    #define sft_vi_cmp_le(a, b)         vreinterpretq_s32_u32(vcleq_s32((a), (b)))
    #define sft_vi_cmp_lt(a, b)         vreinterpretq_s32_u32(vcltq_s32((a), (b)))
    #define sft_vi_cmp_eq(a, b)         vreinterpretq_s32_u32(vceqq_s32((a), (b)))
    #define sft_vi_cmp_ne(a, b)         vreinterpretq_s32_u32(vmvnq_u32(vceqq_s32((a), (b))))

    // ---  casting  ---
    #define sft_vf_cast_vi32(a)         vreinterpretq_f32_s32(a)
    #define sft_vi_cast_vf32(a)         vreinterpretq_s32_f32(a)
    #define sft_vi_cvttps_epi32(v)      vcvtq_s32_f32(v)


#else
    #define SFT_SIMD_WIDTH 1
    #define SFT_BLOCK_X 1
    #define SFT_BLOCK_Y 1
    typedef float vf32;
    typedef i32   vi32;
    
    #define sft_vf_ddx(v) 0.f
    #define sft_vf_ddy(v) 0.f

    // ---  vf32 base and memory  ---
    #define sft_vf_setzero()            0.f
    #define sft_vf_set1(v)              (v)
    #define sft_vf_load(ptr)            (*(ptr))
    #define sft_vf_loadu(ptr)           (*(ptr))
    #define sft_vf_store(ptr, val)      (*(ptr) = (val))
    #define sft_vf_storeu(ptr, val)     (*(ptr) = (val))
    #define sft_vf_gather(base, ind)    ((base)[(ind)])
    
    // ---  vf32 arithmetic  ---
    #define sft_vf_add(a, b)            ((a) + (b))
    #define sft_vf_sub(a, b)            ((a) - (b))
    #define sft_vf_mul(a, b)            ((a) * (b))
    #define sft_vf_div(a, b)            ((a) / (b))
    #define sft_vf_rcp(a)               (1.f / (a))
    #define sft_vf_min(a, b)            ((a) < (b) ? (a) : (b))
    #define sft_vf_max(a, b)            ((a) > (b) ? (a) : (b))
    #define sft_vf_fmadd(a, b, c)       ((a) * (b) + (c))
    #define sft_vf_fnmadd(a, b, c)      (-((a) * (b)) + (c))
    #define sft_vf_fmsub(a, b, c)       ((a) * (b) - (c))
    #define sft_vf_sqrt(a)              sqrtf(a)
    #define sft_vf_rsqrt(a)             (1.f / sqrtf(a))
    #define sft_vf_floor(v)             floorf(v)

    // ---  vf32 bitwise and logic  ---
    static SFT_INLINE vf32 sft_vf_and(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i &= ub.i; return ua.f;
    }
    static SFT_INLINE vf32 sft_vf_or(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i |= ub.i; return ua.f;
    }
    static SFT_INLINE vf32 sft_vf_andnot(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i = (~ua.i) & ub.i; return ua.f;
    }

    static SFT_INLINE vf32 sft_vf_cmp_lt(vf32 a, vf32 b) { return (a <  b) ? -0.f : 0.f; }
    static SFT_INLINE vf32 sft_vf_cmp_le(vf32 a, vf32 b) { return (a <= b) ? -0.f : 0.f; }
    static SFT_INLINE vf32 sft_vf_cmp_gt(vf32 a, vf32 b) { return (a >  b) ? -0.f : 0.f; }
    static SFT_INLINE vf32 sft_vf_cmp_ge(vf32 a, vf32 b) { return (a >= b) ? -0.f : 0.f; }
    static SFT_INLINE vf32 sft_vf_cmp_eq(vf32 a, vf32 b) { return (a == b) ? -0.f : 0.f; }
    static SFT_INLINE vf32 sft_vf_cmp_ne(vf32 a, vf32 b) { return (a != b) ? -0.f : 0.f; }

    static SFT_INLINE vf32 sft_vf_blendv(vf32 a, vf32 b, vf32 mask) {
        union { f32 f; u32 i; } u; u.f = mask;
        return (u.i & 0x80000000) ? a : b; 
    }
    static SFT_INLINE i32 sft_vf_movemask(vf32 a) {
        union { f32 f; u32 i; } u; u.f = a;
        return (u.i >> 31) & 1;
    }

    #define sft_vf_cvtepi32(v)          ((f32)(v))

    #define sft_vi_hmin(v)              (v)
    #define sft_vi_hmax(v)              (v)

    // ---  vi32 base and memory  ---
    #define sft_vi_setzero()            0
    #define sft_vi_set1(v)              (v)
    #define sft_vi_load(ptr)            (*(ptr))
    #define sft_vi_store(ptr, val)      (*(ptr) = (val))
    static SFT_INLINE void sft_vi_maskstore(i32 *ptr, vi32 mask, vi32 val) {
        if ((u32)mask >> 31) { *ptr = val; }
    }
    #define sft_vi_loadu_8u_to_32i(ptr) ((i32)*(const u8 *)(ptr))
    #define sft_vi_extract(v, lane)     (v)
    #define sft_vi_gather(base, ind)    ((base)[(ind)])

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            ((a) + (b))
    #define sft_vi_sub(a, b)            ((a) - (b))
    #define sft_vi_max(a, b)            ((a) > (b) ? (a) : (b))
    #define sft_vi_min(a, b)            ((a) < (b) ? (a) : (b))
    #define sft_vi_and(a, b)            ((a) & (b))
    #define sft_vi_or(a, b)             ((a) | (b))
    #define sft_vi_andnot(a, b)         (~(a) & (b))
    #define sft_vi_srai(a, imm)         ((a) >> (imm))
    #define sft_vi_srli(a, imm)         ((u32)(a) >> (imm))
    #define sft_vi_slli(a, imm)         ((a) << (imm))
    #define sft_vi_movemask(a)          (((u32)(a) >> 31) & 1)
    #define sft_vi_mullo(a, b)          ((a) * (b))

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         ((a) >= (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_gt(a, b)         ((a) >  (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_le(a, b)         ((a) <= (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_lt(a, b)         ((a) <  (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_eq(a, b)         ((a) == (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_ne(a, b)         ((a) != (b) ? (i32)0x80000000 : 0)

    // ---  casting  ---
    static SFT_INLINE vf32 sft_vf_cast_vi32(vi32 a) { 
        union { vi32 i; vf32 f; } u; 
        u.i = (-1 == a) ? 0x80000000 : a; 
        return u.f; 
    }
    static SFT_INLINE vi32 sft_vi_cast_vf32(vf32 a) { union { vf32 f; vi32 i; } u; u.f = a; return u.i; }
    #define sft_vi_cvttps_epi32(v)      ((i32)(v))

#endif

#if defined(__AVX2__) || defined(__ARM_NEON) || defined(__aarch64__)
    SFT_INLINE vf32 sft_vf_rcp_refined(vf32 a) {
        const vf32 approx = sft_vf_rcp(a);
        const vf32 two = sft_vf_set1(2.f);
        // approx * (2.0 - a * approx)
        return sft_vf_mul(approx, sft_vf_fnmadd(a, approx, two));
    }
#else
    // scalar is already exact
    #define sft_vf_rcp_refined(a) sft_vf_rcp(a) 
#endif


/*================================================================
:: MEMORY WRAPPERS
================================================================*/

#if defined(_WIN32)
    #define NOGDI
    #define NOUSER
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #ifdef near
        #undef near
    #endif
    #ifdef far
        #undef far
    #endif
#else
    #include <sys/mman.h>
    #ifndef MAP_ANONYMOUS
        #define MAP_ANONYMOUS 0x20
    #endif

    #if defined(__linux__)
        #include <unistd.h>
        #include <sys/syscall.h>
        #include <linux/futex.h>
        #include <limits.h>
    #endif
#endif

// 1 MB
#define SFT_ARENA_COMMIT_CHUNK (1ULL * 1024 * 1024)


/*================================================================
:: THREADING WRAPPERS
================================================================*/

typedef void (*sft_thread_func)(void* data);

struct sft_thread_payload {
    i32 thread_id;
    sft_thread_func func;
    void* user_data;
};

struct sft_barrier {
#ifdef _WIN32
    volatile i32 count, generation;
    i32 thread_count;
#else
    pthread_barrier_t b;
#endif
};

#if defined(_WIN32)
    // windows.h included in [MEMORY WRAPPERS]
    typedef HANDLE sft_thread;
    typedef HANDLE sft_sem;
#else
    #include <pthread.h>
    #include <semaphore.h>
    #include <errno.h>
    typedef pthread_t sft_thread;
    typedef sem_t sft_sem;
#endif

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #include <immintrin.h>
    #define sft_cpu_pause() _mm_pause()
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(__x86_64__) || defined(__i386__)
        #define sft_cpu_pause() __builtin_ia32_pause()
    #elif defined(__arm__) || defined(__aarch64__)
        #define sft_cpu_pause() __asm__ volatile("yield" ::: "memory")
    #else
        #define sft_cpu_pause() // fallback
    #endif
#else
    #define sft_cpu_pause() // fallback
#endif

SFT_INLINE i32 sft_atomic_add(volatile i32* ptr, i32 val);
SFT_INLINE i32 sft_atomic_cas(volatile i32* ptr, i32 cmp, i32 val);
SFT_INLINE u64 sft_atomic_cas64(volatile u64* ptr, u64 cmp, u64 val);
SFT_INLINE i32 sft_atomic_exchange(volatile i32* ptr, i32 new_val);

SFT_INLINE void sft_sem_init(sft_sem* s, i32 init_count);
SFT_INLINE void sft_sem_wait(sft_sem* s);
SFT_INLINE void sft_sem_post(sft_sem* s, i32 count);

SFT_FUNC sft_thread sft_thread_create(struct sft_thread_payload* payload);
SFT_FUNC void sft_thread_join(sft_thread thread);


/*================================================================
:: FUNCTION HEADERS
================================================================*/

// scalar math functions
SFT_INLINE sftvec sft_v_add(sftvec a, sftvec b);
SFT_INLINE sftvec sft_v_sub(sftvec a, sftvec b);
SFT_INLINE sftvec sft_v_mul(sftvec a, f32 b);
SFT_INLINE sftvec sft_v_div(sftvec a, f32 b);
SFT_INLINE f32 sft_v_dot(sftvec a, sftvec b);
SFT_INLINE sftvec sft_v_cross(sftvec a, sftvec b);
SFT_INLINE f32 sft_v_length(sftvec v);
SFT_INLINE f32 sft_v_length2(sftvec v);
SFT_INLINE sftvec sft_v_norm(sftvec v);
SFT_INLINE sftvec sft_v_normf(f32 x, f32 y, f32 z);
SFT_INLINE sftvec sft_v_face_normal(sftvec a, sftvec b, sftvec c);
SFT_INLINE sftvec sft_v_lerp(sftvec a, sftvec b, f32 t);
SFT_INLINE sftvec sft_v_perpendicular(sftvec v);
SFT_INLINE sftvec sft_v_rotate_by_quat(sftvec v, sftquat q);
SFT_INLINE sftmat sft_m_identity();
SFT_INLINE sftmat sft_m_rot_x(f32 a);
SFT_INLINE sftmat sft_m_rot_y(f32 a);
SFT_INLINE sftmat sft_m_rot_z(f32 a);
SFT_INLINE sftmat sft_m_translate(f32 x, f32 y, f32 z);
SFT_INLINE sftmat sft_m_scale(f32 x, f32 y, f32 z);
SFT_INLINE sftmat sft_m_proj(f32 fovDeg, f32 aspect, f32 near, f32 far);
SFT_INLINE sftmat sft_m_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
SFT_INLINE sftmat sft_m_mul(sftmat a, sftmat b);
SFT_INLINE sftvec sft_m_mul_vec(sftmat m, sftvec v);
SFT_INLINE sftvec sft_m_mul_quat(sftmat m, sftquat q);
SFT_INLINE sftmat sft_m_qinv(sftmat m);
SFT_INLINE sftmat sft_m_model_look_at(sftvec pos, sftvec target, sftvec up); // used for when a 3D object (e.g. a missile or spotlight) should rotate to face a target
SFT_INLINE sftmat sft_m_view_look_at(sftvec pos, sftvec target, sftvec up);  // used only for cameras, computes the inverse translation required for view spaces
SFT_INLINE void sft_m_decompose(sftmat m, sftvec* pos, sftvec* rot, sftvec* scale);
SFT_INLINE sftmat sft_m_from_quat(sftquat q);
SFT_INLINE sftquat sft_q_mul(sftquat a, sftquat b);
SFT_INLINE sftquat sft_q_slerp(sftquat a, sftquat b, f32 t);
SFT_INLINE sftquat sft_q_invert(sftquat q);
SFT_INLINE sftquat sft_q_norm(sftquat q);

// SoA math functions
SFT_INLINE sftvvec sft_vv_add(sftvvec a, sftvvec b);
SFT_INLINE sftvvec sft_vv_sub(sftvvec a, sftvvec b);
SFT_INLINE sftvvec sft_vv_scale(sftvvec a, vf32 b);
SFT_INLINE vf32 sft_vv_dot(sftvvec a, sftvvec b);
SFT_INLINE sftvvec sft_vv_cross(sftvvec a, sftvvec b);
SFT_INLINE vf32 sft_vv_length2(sftvvec v);
SFT_INLINE vf32 sft_vv_length(sftvvec v);
SFT_INLINE sftvvec sft_vv_norm(sftvvec v);
SFT_INLINE sftvvec sft_vv_norm_fast(sftvvec v);
SFT_INLINE sftvvec sft_vv_transform(sftvvec v, sftvmat vm);
SFT_INLINE sftvvec sft_vv_transform_dir(sftvvec v, sftvmat vm);
SFT_INLINE sftvmat sft_vm_broadcast(sftmat m);

// core functions
SFT_FUNC void sft_init(i32 width, i32 height, i32 requested_threads); // 0 == requested_threads => use physical core count
SFT_FUNC void sft_release(void);
SFT_FUNC void sft_present(void);
SFT_FUNC void sft_frame_begin(void);
SFT_FUNC void sft_frame_end(void);
SFT_FUNC void sft_resize(i32 width, i32 height);

// misc
SFT_FUNC void sft_set_camera(sftvec cam_pos, sftmat view, sftmat proj);
SFT_FUNC void sft_set_debug_mode(enum sft_debug_mode mode);
SFT_FUNC const char* sft_get_debug_name(enum sft_debug_mode mode);
SFT_FUNC void sft_set_clear(u8 enabled); // clear background while resolving w/o one big memset
SFT_FUNC void sft_set_clear_color(u32 color);
SFT_FUNC void sft_set_color_buffer(u32* buf, i32 pitch_in_bytes);

// drawing functions
SFT_FUNC void sft_draw_instance(const sft_instance* instance);
SFT_FUNC void sft_draw_cube(sft_dmesh* dm);
SFT_FUNC void sft_draw_cube_inv(sft_dmesh* dm);
SFT_FUNC void sft_draw_sphere(sft_dmesh* dm, i32 rings, i32 slices);
SFT_FUNC void sft_draw_capsule(sft_dmesh* dm, sftvec start_pos, sftvec end_pos, f32 radius, i32 slices, i32 rings);
SFT_FUNC void sft_draw_cylinder(sft_dmesh* dm, sftvec start_pos, sftvec end_pos, f32 start_radius, f32 end_radius, i32 sides);
SFT_FUNC sft_dmesh* sft_dmesh_begin(enum sft_dmesh_type type, sft_mat_handle material, u32 max_prims);
SFT_FUNC void sft_dmesh_push_quad(sft_dmesh* dm, sftvec p0, sftvec p1, sftvec p2, sftvec p3, f32 u0, f32 v0, f32 u1, f32 v1, sftvec normal);
SFT_FUNC void sft_dmesh_push_quad_4n(sft_dmesh* dm, sftvec p0, sftvec p1, sftvec p2, sftvec p3, f32 u0, f32 v0, f32 u1, f32 v1, sftvec n0, sftvec n1, sftvec n2, sftvec n3);
SFT_FUNC void sft_dmesh_push_tri(sft_dmesh* dm, sftvec p0, sftvec p1, sftvec p2, f32 u0, f32 v0, f32 u1, f32 v1, f32 u2, f32 v2, sftvec normal);
SFT_FUNC u8   sft_dmesh_alloc_quads(sft_dmesh* dm, u32 count, sft_dmesh_qb* out_batch);
SFT_FUNC void sft_dmesh_end(sft_dmesh* dm, const sftmat* opt_transform);
SFT_FUNC sft_pose* sft_pose_create(const sft_model* model, sft_arena* arena);
SFT_FUNC void sft_pose_update(sft_pose* pose, i32 anim_id, f32 time);

// light functions
SFT_FUNC void sft_set_ambient_light(f32 r, f32 g, f32 b);
SFT_FUNC void sft_draw_directional_light(sftvec dir, f32 r, f32 g, f32 b, f32 intensity);
//  every 'light_id' provided is expected to be unique
SFT_FUNC void sft_draw_point_light(sftvec pos, f32 r, f32 g, f32 b, f32 intensity, f32 radius, u32 light_id);
SFT_FUNC void sft_set_shadows_enabled(u8 enabled);
SFT_FUNC void sft_set_lightmap_brightness(f32 b);

// texture functions
#ifdef SFT_USE_STB_IMAGE
SFT_FUNC sft_tex_handle sft_texture_load(const char* path);
SFT_FUNC sft_tex_handle sft_texture_load_from_memory(const u8* buf, i32 length);
#endif
SFT_FUNC sft_tex_handle sft_texture_create(i32 width, i32 height, const u32* pixels);
SFT_FUNC sft_tex_handle sft_texture_create_solid(u32 color);
SFT_FUNC i32 sft_texture_get_width(sft_tex_handle tex);
SFT_FUNC i32 sft_texture_get_height(sft_tex_handle tex);
SFT_FUNC u32* sft_texture_get_linear_pixels(sft_tex_handle tex); // IMPORTANT: caller is required to free
SFT_FUNC void sft_texture_release(sft_tex_handle tex);

// material functions
SFT_FUNC sft_mat_handle sft_material_create(sft_tex_handle tex, u32 base_color, f32 shininess);
SFT_FUNC sft_mat_handle sft_material_clone(sft_mat_handle base_mat);
SFT_FUNC void sft_material_release(sft_mat_handle mat);

// arena functions
SFT_FUNC void sft_arena_init(sft_arena* arena, u64 reserve_size);
SFT_FUNC void* sft_arena_alloc(sft_arena* arena, u64 size);
SFT_INLINE void sft_arena_commit_if_needed(sft_arena* arena, u64 required_bytes);
SFT_FUNC void sft_arena_frame_begin(sft_arena* arena);

// model functions
#ifdef SFT_USE_CGLTF
SFT_FUNC sft_model* sft_model_load_glb(const char* path, sft_arena* arena, i32 uv_channel, i32 shadow_proxy_budget);
#endif
SFT_FUNC sft_model* sft_model_load_qsf(const char* path, sft_arena* arena);
// TODO copy obj loader from sofren
// SFT_FUNC sft_model* sft_model_load_obj(const char* path);


/*================================================================
:: IMPLEMENTATION & TYPE DEFINITIONS
================================================================*/

#ifdef SFT_IMPL

typedef struct sftvec {
    f32 x, y, z;
} sftvec;
typedef struct sftvvec {
    vf32 x, y, z;
} sftvvec;

typedef struct sftquat {
    f32 x, y, z, w;
} sftquat;
typedef struct sftvquat {
    vf32 x, y, z, w;
} sftvquat;

typedef union sftmat {
    sftquat rows[4]; // sftquat is just a vec4
    f32 m[4][4];
} sftmat;
typedef union sftvmat {
    sftvquat rows[4]; // sftquat is just a vec4
    vf32 m[4][4];
} sftvmat;

typedef struct sft_ray_hit {
    u8 hit;
    f32 distance;
    sftvec pos;
    sftvec normal;
} sft_ray_hit;

typedef struct sft_arena {
    u8* base;                   // pointer to the start of the virtual address
    u64 max_size;               // reservation limit
    u64 committed_size;         // how much physical ram has been given
    u64 offset;                 // current bump pointer offset
    volatile i32 commit_lock;   // spin lock for OS page commits
} sft_arena;

struct sft_aabb {
    sftvec min, max;
};

struct sft_meshlet {
    alignas(32) f32 x[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 y[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 z[SFT_VERTS_PER_MESHLET];
};

struct sft_meshlet_attribs {
    // normals
    alignas(32) f32 nx[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 ny[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 nz[SFT_VERTS_PER_MESHLET];

    // uv coords
    alignas(32) f32 u[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 v[SFT_VERTS_PER_MESHLET];

    // lightmap coords
    alignas(32) f32 lu[SFT_VERTS_PER_MESHLET];
    alignas(32) f32 lv[SFT_VERTS_PER_MESHLET];
};

struct sft_meshlet_skin {
    // 4 joints per vertex
    alignas(32) u16 j0[SFT_VERTS_PER_MESHLET];
    alignas(32) u16 j1[SFT_VERTS_PER_MESHLET];
    alignas(32) u16 j2[SFT_VERTS_PER_MESHLET];
    alignas(32) u16 j3[SFT_VERTS_PER_MESHLET];

    // 4 weights per vertex (255 == 1.f)
    alignas(32) u8 w0[SFT_VERTS_PER_MESHLET];
    alignas(32) u8 w1[SFT_VERTS_PER_MESHLET];
    alignas(32) u8 w2[SFT_VERTS_PER_MESHLET];
    alignas(32) u8 w3[SFT_VERTS_PER_MESHLET];
};

struct sft_meshlet_inds {
    // 16 batches of 8 triangles,
    // each batch: 8x v0, 8x v1, 8x v2, 8x pad = 32 bytes.
    alignas(32) u8 inds[512]; // 16 * 32 = 512 bytes
};

struct sft_meshlet_ref {
    u32 global_meshlet_ind;
    struct sft_aabb aabb;
    sftvec centroid;
};

struct sft_blas_build_item {
    u32 global_meshlet_ind;
    struct sft_aabb aabb;
    sftvec centroid;
};

struct sft_bvh_node {
    // hardcoded 8 on purpose for 8 way, not meant to be SFT_SIMD_WIDTH
    alignas(32) f32 min_x[8];
    alignas(32) f32 min_y[8];
    alignas(32) f32 min_z[8];
    alignas(32) f32 max_x[8];
    alignas(32) f32 max_y[8];
    alignas(32) f32 max_z[8];

    // child inds or leaf payload:
    // bit 31 == 1 => leaf meshlet (index == child & 0x7FFFFFFF)
    // bit 31 == 0 => child BLAS node index
    // 0xFFFFFFFF => unused slot
    u32 child_indices[8];
    u8 child_count;     // active children in this node (1..8)
    u8 is_leaf_group;   // 1 if all children are leaf meshlets, 0 if internal nodes
};

struct sft_shadow_proxy {
    sftvec posa, posb;  // start/end of capsule or center or sphere
    f32 radius;
    u32 bone_ind;
};

struct sft_light_proxy_cache {
    sftvvec pa, pb, ab, W;
    vf32 abdab;     // ab dot ab
    vf32 wdw;       // W dot W
    vf32 rb2;       // R bound 2
    vf32 pr, pr2;
    vf32 vis_mult;
};

struct sft_bone {
    sftmat inverse_bind_matrix; // from mesh space to local bone space
    sftmat local_bind_matrix;   // resting local transform
    sftvec bind_pos;            // physical resting position of the joint
    i32 parent_ind;             // ind of parent bone or -1 if root

    sftvec base_t;
    sftquat base_r;
    sftvec base_s;
};

struct sft_skeleton {
    struct sft_bone* bones;
    i32 bone_count;
};

struct sft_trs { 
    sftvec p; 
    sftquat r; 
    sftvec s; 
    u8 mask; 
};

enum sft_anim_path {
    SFT_ANIM_PATH_TRANSLATION,
    SFT_ANIM_PATH_ROTATION,
    SFT_ANIM_PATH_SCALE
} ;

struct sft_anim_sampler {
    f32* inputs;    // keyframe timestamps
    f32* outputs;   // keyframe values (flat array)
    i32 count;
    u8 is_cubic;
    u8 is_step;
};

struct sft_anim_channel {
    i32 sampler_ind;
    i32 node_ind; // index into model->skeleton.bones
    enum sft_anim_path path;
};

struct sft_animation {
    struct sft_anim_sampler* samplers;
    struct sft_anim_channel* channels;
    i32 sampler_count;
    i32 channel_count;
    f32 duration;
};

struct cluster_data {
    f32 area;
    sftvec centroid_sum;
    i32 tri_count;
};

typedef struct sft_mesh {
    // stored separately from culling data on purpose
    struct sft_meshlet* meshlets;
    struct sft_meshlet_attribs* meshlet_attribs;
    
    // ptional skinning data, NULL if mesh is static
    struct sft_meshlet_skin* meshlet_skins; 
    
    u32 meshlet_count;
    u32 model_meshlet_offset; // starting ind of this mesh's meshlets within entire model

    // pre swizzled inds for 128 triangles per meshlet
    struct sft_meshlet_inds* inds;
    
    // bounding spheres
    f32* bounds_center_x;
    f32* bounds_center_y;
    f32* bounds_center_z;
    f32* bounds_radius;

    // bounding AABBs
    f32* bounds_min_x;
    f32* bounds_max_x;
    f32* bounds_min_y;
    f32* bounds_max_y;
    f32* bounds_min_z;
    f32* bounds_max_z;

    // normal cones
    f32* cone_axis_x;
    f32* cone_axis_y;
    f32* cone_axis_z;
    f32* cone_cutoff; 

    // primary bone that drives this meshlet's culling volume
    i32* culling_bone_inds;

    sft_mat_handle material;
    u32 lightmap_page; // -1 if no lightmap
} sft_mesh;

typedef struct sft_model {
    // flat array of immutable meshes, multiple instances point to this single model
    struct sft_mesh* meshes;
    u32 mesh_count;

    u32 total_meshlets;
    u16* meshlet_mesh_inds; // maps a model relative meshlet ind to its parent mesh ind

    // 8 way BVH
    struct sft_bvh_node* blas_nodes;
    u32 blas_node_count, blas_root_ind;

    sft_tex_handle* lightmaps;
    u32 lightmap_count;

    // analytical shadow proxies
    struct sft_shadow_proxy* shadow_proxies;
    u32 shadow_proxy_count;

    // skeletal hierarchy and bind pose
    struct sft_skeleton skeleton;

    // animation tracks
    struct sft_animation* animations;
    i32 anim_count;
} sft_model;

typedef struct sft_pose {
    const struct sft_model* model;
    sftmat* global_matrices;        // flattened world space mats per bone
    sftmat* posed_bone_matrices;    // final skinning mats (global * inv_bind)
} sft_pose;

typedef struct sft_dmesh {
    enum sft_dmesh_type type;
    sft_mat_handle material;
    
    // allocated from scratch_arena
    struct sft_meshlet* meshlets;
    struct sft_meshlet_attribs* attribs;
    struct sft_meshlet_inds* inds; // static template
    
    // bounding spheres
    f32* bounds_center_x;
    f32* bounds_center_y;
    f32* bounds_center_z;
    f32* bounds_radius;
    
    // bounding AABBs
    f32* bounds_min_x;
    f32* bounds_min_y;
    f32* bounds_min_z;
    f32* bounds_max_x;
    f32* bounds_max_y;
    f32* bounds_max_z;
    
    // cone culling (will be set to {0,0,1} and cutoff 2.f to disable culling)
    f32* cone_axis_x;
    f32* cone_axis_y;
    f32* cone_axis_z;
    f32* cone_cutoff;
    
    // state tracking
    u32 max_meshlets;           // capacity
    u32 curr_meshlet;        // which meshlet currently being written to
    u32 curr_vert;           // verts written in the current meshlet
    u32 max_verts_per_chunk;    // 64 for quads, 63 for triangles
} sft_dmesh;

typedef struct sft_dmesh_qb {
    f32 *x, *y, *z;         // 64 float slices
    f32 *nx, *ny, *nz;
    f32 *u, *v;
    u32 vert_offset;        // starting index within this batch (0, 4, 8... 60)
    u32 allocated_count;    // track how many quads were actually reserved
} sft_dmesh_qb;

typedef struct sft_texture {
    u32* pixels;
    i32 w, h;

    // struct sft_rgb32 { u32 r, g, b; }* quads;

    // index 0 is for base tex, 1-N are mipmaps
    u32* all_pixels[14];
    i32 all_w[14], all_h[14];

    i32 blocks_w[14];       // how many 4x4 blocks across
    i32 stride_shift[14];   // if POT, log2(blocks_w)

    i32 mip_levels; // total levels including base
    u8 is_pot;      // is power of two
} sft_texture;

typedef struct sft_material {
    sft_texture* albedo_tex;    // albedo only for now
    u32 base_color;             // tint
    f32 shininess;              // simple blinn phong shininess for basic specular
} sft_material;

typedef struct sft_instance {
    const struct sft_model* model;
    sftmat transform;
    u32 ss_meshlet_offset;  // cache tracking
    sftmat* posed_bone_matrices; // for this specific frame and instance, NULL if no skeleton
} sft_instance;

typedef struct sft_directional_light {
    sftvec dir;   // must be normalized
    sftvec color; // RGB [0.0, 1.0], pre multiplied by intensity
} sft_directional_light;

typedef struct sft_point_light {
    sftvec pos;
    sftvec color;       // RGB [0.0, 1.0], pre multiplied by intensity
    f32 radius;         // used for tile frustum culling
    f32 inv_radius_sqr; // 1.f / (radius * radius), for attenuation
} sft_point_light;

struct sft_atomic_counter {
    alignas(64) volatile i32 count;
};

struct sft_spatial_hash {
    f32 cell_size;
    u32 table_size;

    // maps a hash ind to the first entry in the entries array
    i32* head; // size = table_size

    // linked list implemented via flat integer inds
    i32* next;      // size = max_entries
    i32* tri_ind;   // size = max_entries

    u32 entry_count, max_entries;
};

struct sft_meshlet_builder {
    // maps local meshlet vertex inds (0 to 63) back to the global raw vertex buffer
    i32 global_vert_inds[SFT_VERTS_PER_MESHLET];
    u32 vert_count;

    // stores local inds (0 to 63) for the triangles
    u8 local_inds[128 * 3];
    u32 tri_count;

    // the running average normal of the meshlet for the cone culling
    sftvec cone_axis;
};

struct sft_payload_chunk {
    // 31 slots for 64 bit batch payloads
    alignas(64) u64 payloads[31];
    
    // index pointing to the next chunk in the tile's list
    i32 next_ind;
    i32 pad; // pad to 256 bytes
};

struct sft_thread_bin_state {
    u64* write_head;    // points to next payload slot
    u64* write_end;     // points to end of the chunk to check capacity
    i32 chunk_ind;      // index of the chunk this thread is actively filling
    i32 payload_count;  // tracks payloads written by this thread for this tile
};

struct sft_geom_job {
    u16 instance_ind;
    u16 mesh_ind;
    u32 meshlet_start;
    u32 meshlet_count;
};

struct sft_clipped_triangle {
    f32 sx[3], sy[3];
    f32 inv_w[3];
    f32 u[3], v[3];
    f32 lu[3], lv[3];
    f32 px[3], py[3], pz[3];
    f32 nx[3], ny[3], nz[3];
    u32 inst_ind;
    u32 m_ind;
};

struct sft_clip_vert {
    f32 tx, ty, tz, w;
    f32 u, v;
    f32 lu, lv;
    f32 px, py, pz;
    f32 nx, ny, nz;
};

struct sft_raw_geometry {
    const f32 *vx, *vy, *vz;
    const f32 *nx, *ny, *nz;
    const f32 *u, *v;
    
    // optional skinning data
    const u16 *j0, *j1, *j2, *j3;
    const u8  *w0, *w1, *w2, *w3;
};

struct sft_meshlet_output {
    struct sft_meshlet* verts;
    struct sft_meshlet_attribs* attribs;
    struct sft_meshlet_inds* inds;
    
    // optional outputs
    struct sft_meshlet_skin* skin;
    i32* culling_bone;
    struct sft_sphere* sphere;
    struct sft_aabb* aabb;
};

struct sft_sphere {
    sftvec center;
    f32 radius;
};

struct sft_frustum {
    sftvvec normal[6];
    sftvvec abs_normal[6];
    vf32 d[6];
};

struct sft_tex_heap_node {
    u64 offset, size;
    struct sft_tex_heap_node* next;
};


/*================================================================
:: PRIVATE VARIABLES
================================================================*/

static SFT_THREAD_LOCAL i32 sft_thread_id = 0;

static struct sft_state {
    i32 thread_count;
    struct sft_thread_bin_state* thread_bin_states;

    sft_arena instance_arena;
    sft_arena job_arena;
    sft_arena chunk_arena;
    sft_arena scratch_arena;
    sft_arena clip_arena;
    sft_arena tex_heap_arena;
    sft_arena light_arena;
    sft_arena shadow_arena;

    struct {
        // pointers to virtual reservations
        sft_instance* instances;
        struct sft_geom_job* jobs;
        
        struct sft_atomic_counter instance_count;
        struct sft_atomic_counter job_count;
        struct sft_atomic_counter total_ss_meshlets; // allocated screen space meshlets

        // frame TLAS
        struct sft_bvh_node* tlas_nodes;
        u32 tlas_node_count, tlas_root_ind;
    } cmd_queue;

    struct sft_atomic_counter next_job_ind;
    struct sft_atomic_counter next_tile_ind;
    struct sft_atomic_counter next_chunk_ind;
    
    struct sft_barrier geom_barrier;
    struct sft_barrier shadow_barrier;
    struct sft_barrier raster_barrier;
    struct sft_barrier prefix_barrier;
    sft_sem sem_start;
    sft_sem sem_done;

    struct sft_tex_heap_node* tex_heap_free_list;

    // texture metadata pool
    struct {
        u32* generation;
        i32* next_free;
        
        u64* heap_offset; // base pointer offset in tex_heap_arena
        i32* w;
        i32* h;
        i32* mip_levels;
        u8*  is_pot;
        
        // mipmap offsets relative to heap_offset
        // flatted 2D array: [capacity * 14]
        u32* mip_offsets; 
        i32* mip_blocks_w;
        i32* mip_stride_shift;

        i32 free_head;
        i32 capacity;
    } tex_pool;

    // static material pool
    struct {
        u32* generation;
        i32* next_free;
        
        sft_tex_handle* albedo_tex;
        u32* base_color;
        f32* shininess;

        i32 free_head;
        i32 capacity;
    } mat_pool;

    // transient material pool bump allocated per frame
    struct {
        sft_tex_handle* albedo_tex;
        u32* base_color;
        f32* shininess;

        i32 count;
        i32 capacity;
    } mat_pool_transient;

    sftvec ambient_light;
    struct {
        sft_directional_light* directional;
        sft_point_light* point;
        struct sft_atomic_counter directional_count;
        struct sft_atomic_counter point_count;

        f32 lightmap_brightness;
    } lighting;

    struct sft_world_shadow_proxy {
        sftvec posa, posb;
        f32 radius;
        u8 light_vis[64]; // stores 0 to 3 (how much of the capsule can see the light) TODO remove / improve this
    }* world_proxies;

    struct {
        u8 enabled;
        struct sft_world_shadow_proxy* proxies;
        struct sft_atomic_counter proxy_count;
    } shadows;

    struct sft_frustum frustum;
    
    struct {
        f32 half_width, half_height;
        f32 vp_x, vp_y;

        i32 tile_cols, tile_rows;
        u32* color_buffer;
        i32 color_pitch;

        struct sft_payload_chunk* chunk_pool;

        i32* tile_heads;
        volatile i32* tile_tails;
        i32* tile_payload_counts; // global heuristic counter
        i32* sorted_tiles;

        u8 should_clear;
        u32 clear_color;

        enum sft_debug_mode debug_mode;
    } display;

    struct sft_ss_meshlet {
        alignas(32) f32 sx[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 sy[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 inv_w[SFT_VERTS_PER_MESHLET];

        // cache skinned world space positions and normals,
        // TODO it doesn't make sense for them to be here but it works
        alignas(32) f32 wx[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 wy[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 wz[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 wnx[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 wny[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 wnz[SFT_VERTS_PER_MESHLET];
    }* ss_cache;

    struct {
        struct sft_clipped_triangle* pool;
        volatile i32 count;
    } clip_cache;

    struct {
        // constant templates
        struct sft_meshlet_inds quad_inds;
        struct sft_meshlet_inds tri_inds;
    } dmesh;

    sftvec camera_pos;
    sftmat view, proj;
    sftmat view_proj;

    sft_mat_handle base_mat;
}* sft_state;


/*================================================================
:: FUNCTION DEFINITIONS (MATH)
================================================================*/
#if 1

SFT_INLINE sftvec sft_v_add(sftvec a, sftvec b) {
    sftvec r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
SFT_INLINE sftvec sft_v_sub(sftvec a, sftvec b) {
    sftvec r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}
SFT_INLINE sftvec sft_v_mul(sftvec a, f32 b) {
    sftvec r;
    r.x = a.x * b;
    r.y = a.y * b;
    r.z = a.z * b;
    return r;
}
SFT_INLINE sftvec sft_v_div(sftvec a, f32 b) {
    sftvec r;
    const f32 ib = 1.f / b;
    r.x = a.x * ib;
    r.y = a.y * ib;
    r.z = a.z * ib;
    return r;
}
SFT_INLINE f32 sft_v_dot(sftvec a, sftvec b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
SFT_INLINE f32 sft_v_length(sftvec v) {
    return sqrtf(sft_v_dot(v, v));
}
SFT_INLINE f32 sft_v_length2(sftvec v) {
    return sft_v_dot(v, v);
}
SFT_INLINE sftvec sft_v_cross(sftvec a, sftvec b) {
    sftvec r;
    r.x = a.y*b.z - a.z*b.y;
    r.y = a.z*b.x - a.x*b.z;
    r.z = a.x*b.y - a.y*b.x;
    return r;
}
SFT_INLINE sftvec sft_v_norm(sftvec v) {
    const f32 l = sft_v_length(v);
    return l > SFT_EPSILON ? sft_v_mul(v, 1.f / l) : (sftvec){0};
}
SFT_INLINE sftvec sft_v_normf(f32 x, f32 y, f32 z) {
    const sftvec v = { .x = x, .y = y, .z = z };
    return sft_v_norm(v);
}
SFT_INLINE sftvec sft_v_face_normal(sftvec a, sftvec b, sftvec c) {
    const sftvec edge1 = sft_v_sub(b, a);
    const sftvec edge2 = sft_v_sub(c, a);
    return sft_v_norm(sft_v_cross(edge1, edge2));
}
SFT_INLINE sftvec sft_v_lerp(sftvec a, sftvec b, f32 t) {
    return sft_v_add(a, sft_v_mul(sft_v_sub(b, a), t));
}
SFT_INLINE sftvec sft_v_perpendicular(sftvec v) {
    sftvec res = {0};

    f32 min = fabsf(v.x);
    sftvec cardinalAxis = { .x = 1.f, .y = 0.f, .z = 0.f };

    if (fabsf(v.y) < min) {
        min = fabsf(v.y);
        cardinalAxis = (sftvec){ .x = 0.f, .y = 1.f, .z = 0.f };
    }

    if (fabsf(v.z) < min) {
        cardinalAxis = (sftvec){ .x = 0.f, .y = 0.f, .z = 1.f };
    }

    res.x = v.y*cardinalAxis.z - v.z*cardinalAxis.y;
    res.y = v.z*cardinalAxis.x - v.x*cardinalAxis.z;
    res.z = v.x*cardinalAxis.y - v.y*cardinalAxis.x;

    return res;
}
// copied from raylib's Vector3RotateByQuaternion
SFT_INLINE sftvec sft_v_rotate_by_quat(sftvec v, sftquat q) {
    sftvec res = {0};
    res.x = v.x*(q.x*q.x + q.w*q.w - q.y*q.y - q.z*q.z) + v.y*(2*q.x*q.y - 2*q.w*q.z) + v.z*(2*q.x*q.z + 2*q.w*q.y);
    res.y = v.x*(2*q.w*q.z + 2*q.x*q.y) + v.y*(q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z) + v.z*(-2*q.w*q.x + 2*q.y*q.z);
    res.z = v.x*(-2*q.w*q.y + 2*q.x*q.z) + v.y*(2*q.w*q.x + 2*q.y*q.z)+ v.z*(q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
    return res;
}

SFT_INLINE sftmat sft_m_identity() {
    sftmat r = {0};
    r.m[0][0] = 1.f;
    r.m[1][1] = 1.f;
    r.m[2][2] = 1.f;
    r.m[3][3] = 1.f;
    return r;
}
SFT_INLINE sftmat sft_m_rot_x(f32 a) {
    sftmat r = SFT_MAT_IDENTITY;
    const f32 c = cosf(a);
    const f32 s = sinf(a);
    r.m[1][1] = c;
    r.m[1][2] = s;
    r.m[2][1] = -s;
    r.m[2][2] = c;
    return r;
}
SFT_INLINE sftmat sft_m_rot_y(f32 a) {
    sftmat r = SFT_MAT_IDENTITY;
    const f32 c = cosf(a);
    const f32 s = sinf(a);
    r.m[0][0] = c;
    r.m[0][2] = s;
    r.m[2][0] = -s;
    r.m[2][2] = c;
    return r;
}
SFT_INLINE sftmat sft_m_rot_z(f32 a) {
    sftmat r = SFT_MAT_IDENTITY;
    const f32 c = cosf(a);
    const f32 s = sinf(a);
    r.m[0][0] = c;
    r.m[0][1] = s;
    r.m[1][0] = -s;
    r.m[1][1] = c;
    return r;
}
SFT_INLINE sftmat sft_m_translate(f32 x, f32 y, f32 z) {
    sftmat r = SFT_MAT_IDENTITY;
    r.m[3][0] = x;
    r.m[3][1] = y;
    r.m[3][2] = z;
    return r;
}
SFT_INLINE sftmat sft_m_scale(f32 x, f32 y, f32 z) {
    sftmat r = SFT_MAT_IDENTITY;
    r.m[0][0] = x;
    r.m[1][1] = y;
    r.m[2][2] = z;
    return r;
}
SFT_INLINE sftmat sft_m_proj(f32 fovDeg, f32 aspect, f32 near, f32 far) {
    const f32 fov = 1.f / tanf(fovDeg * 0.5f / 180.f * SFT_PI);
    sftmat r = {0};
    r.m[0][0] = fov / aspect;
    r.m[1][1] = fov;
    r.m[2][2] = far / (far - near);
    r.m[3][2] = (-far * near) / (far - near);
    r.m[2][3] = 1.f;
    r.m[3][3] = 0.f;
    return r;
}
SFT_INLINE sftmat sft_m_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    sftmat r = {0};
    r.m[0][0] = 2.f / (right - left);
    r.m[1][1] = 2.f / (top - bottom);
    r.m[2][2] = 1.f / (far - near);
    r.m[3][0] = -(right + left) / (right - left);
    r.m[3][1] = -(top + bottom) / (top - bottom);
    r.m[3][2] = -near / (far - near);
    r.m[3][3] = 1.f;
    return r;
}
SFT_INLINE sftmat sft_m_mul(sftmat a, sftmat b) {
    sftmat r;
    for (i32 i = 0; i < 4; i += 1) {
        const f32 a0 = a.m[i][0], a1 = a.m[i][1], a2 = a.m[i][2], a3 = a.m[i][3];
        r.m[i][0] = a0 * b.m[0][0] + a1 * b.m[1][0] + a2 * b.m[2][0] + a3 * b.m[3][0];
        r.m[i][1] = a0 * b.m[0][1] + a1 * b.m[1][1] + a2 * b.m[2][1] + a3 * b.m[3][1];
        r.m[i][2] = a0 * b.m[0][2] + a1 * b.m[1][2] + a2 * b.m[2][2] + a3 * b.m[3][2];
        r.m[i][3] = a0 * b.m[0][3] + a1 * b.m[1][3] + a2 * b.m[2][3] + a3 * b.m[3][3];
    }
    return r;
}
SFT_INLINE sftvec sft_m_mul_vec(sftmat m, sftvec v) {
    sftvec r;
    r.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
    r.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
    r.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
    return r;
}
SFT_INLINE sftvec sft_m_mul_quat(sftmat m, sftquat v) {
    sftvec r;
    r.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0];
    r.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1];
    r.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2];
    return r;
}
SFT_INLINE sftmat sft_m_qinv(sftmat m) {
    sftmat r;
    r.m[0][0] = m.m[0][0];
    r.m[0][1] = m.m[1][0];
    r.m[0][2] = m.m[2][0];
    r.m[0][3] = 0.f;
    r.m[1][0] = m.m[0][1];
    r.m[1][1] = m.m[1][1];
    r.m[1][2] = m.m[2][1];
    r.m[1][3] = 0.f;
    r.m[2][0] = m.m[0][2];
    r.m[2][1] = m.m[1][2];
    r.m[2][2] = m.m[2][2];
    r.m[2][3] = 0.f;
    r.m[3][0] = -(m.m[3][0] * r.m[0][0] + m.m[3][1] * r.m[1][0] + m.m[3][2] * r.m[2][0]);
    r.m[3][1] = -(m.m[3][0] * r.m[0][1] + m.m[3][1] * r.m[1][1] + m.m[3][2] * r.m[2][1]);
    r.m[3][2] = -(m.m[3][0] * r.m[0][2] + m.m[3][1] * r.m[1][2] + m.m[3][2] * r.m[2][2]);
    r.m[3][3] = 1.f;
    return r;
}
SFT_INLINE sftmat sft_m_model_look_at(sftvec pos, sftvec target, sftvec up) {
    const sftvec forward = sft_v_norm(sft_v_sub(target, pos));
    up = sft_v_norm(sft_v_sub(up, sft_v_mul(forward, sft_v_dot(up, forward))));
    const sftvec right = sft_v_cross(up, forward);

    sftmat r;
    r.m[0][0] = right.x;
    r.m[0][1] = right.y;
    r.m[0][2] = right.z;
    r.m[0][3] = 0.f;
    r.m[1][0] = up.x;
    r.m[1][1] = up.y;
    r.m[1][2] = up.z;
    r.m[1][3] = 0.f;
    r.m[2][0] = forward.x;
    r.m[2][1] = forward.y;
    r.m[2][2] = forward.z;
    r.m[2][3] = 0.f;
    r.m[3][0] = pos.x;
    r.m[3][1] = pos.y;
    r.m[3][2] = pos.z;
    r.m[3][3] = 1.f;
    return r;
}
SFT_INLINE sftmat sft_m_view_look_at(sftvec pos, sftvec target, sftvec up) {
    const sftvec forward = sft_v_norm(sft_v_sub(target, pos));
    
    // safety check for looking straight down
    if (fabsf(forward.x) < SFT_EPSILON && fabsf(forward.z) < SFT_EPSILON) {
        up = SFT_V(0.f, 0.f, -1.f);
    }

    up = sft_v_norm(sft_v_sub(up, sft_v_mul(forward, sft_v_dot(up, forward))));
    const sftvec right = sft_v_cross(up, forward);

    sftmat r;
    // row 1
    r.m[0][0] = right.x;
    r.m[0][1] = up.x;      // transposed
    r.m[0][2] = forward.x; // transposed
    r.m[0][3] = 0.f;
    // row 2
    r.m[1][0] = right.y;   // transposed
    r.m[1][1] = up.y;
    r.m[1][2] = forward.y; // transposed
    r.m[1][3] = 0.f;
    // row 3
    r.m[2][0] = right.z;   // transposed
    r.m[2][1] = up.z;      // transposed
    r.m[2][2] = forward.z;
    r.m[2][3] = 0.f;
    // row 4 (translation inverse)
    r.m[3][0] = -sft_v_dot(pos, right);
    r.m[3][1] = -sft_v_dot(pos, up);
    r.m[3][2] = -sft_v_dot(pos, forward);
    r.m[3][3] = 1.f;
    return r;
}
SFT_INLINE void sft_m_decompose(sftmat m, sftvec* pos, sftvec* rot, sftvec* scale) {
    // extract translation (row 3)
    if (pos) {
        pos->x = m.m[3][0];
        pos->y = m.m[3][1];
        pos->z = m.m[3][2];
    }

    // extract scale (length of basis vectors)
    // row 0 is x axis, 1 is y, 2 is z
    f32 sx = sqrtf(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1] + m.m[0][2] * m.m[0][2]);
    f32 sy = sqrtf(m.m[1][0] * m.m[1][0] + m.m[1][1] * m.m[1][1] + m.m[1][2] * m.m[1][2]);
    f32 sz = sqrtf(m.m[2][0] * m.m[2][0] + m.m[2][1] * m.m[2][1] + m.m[2][2] * m.m[2][2]);

    if (scale) {
        scale->x = sx;
        scale->y = sy;
        scale->z = sz;
    }

    // extract rotation (euler xyz)
    // remove scale from the rotation matrix components
    if (rot) {
        if (sx < SFT_EPSILON) sx = 1.f;
        if (sy < SFT_EPSILON) sy = 1.f;
        if (sz < SFT_EPSILON) sz = 1.f;

        const f32 r00 = m.m[0][0] / sx;
        const f32 r01 = m.m[0][1] / sx;
        const f32 r02 = m.m[0][2] / sx;
        const f32 r11 = m.m[1][1] / sy;
        const f32 r12 = m.m[1][2] / sy;
        const f32 r21 = m.m[2][1] / sz;
        const f32 r22 = m.m[2][2] / sz;

        // decompose (rx * ry * rz)
        // m[0][2] = -sin(y)
        rot->y = -asinf(SFT_CLAMP(r02, -1.f, 1.f));

        if (cosf(rot->y) > SFT_EPSILON) {
            // standard case
            rot->x = atan2f(r12, r22);
            rot->z = atan2f(r01, r00);
        }
        else {
            // gimbal lock case (y is +- 90 degrees)
            rot->x = atan2f(-r21, r11);
            rot->z = 0.f;
        }
    }
}
SFT_INLINE sftmat sft_m_from_quat(sftquat q) {
    sftmat m = SFT_MAT_IDENTITY;
    const f32 x = q.x, y = q.y, z = q.z, w = q.w;
    const f32 x2 = x + x, y2 = y + y, z2 = z + z;
    const f32 xx = x * x2, xy = x * y2, xz = x * z2;
    const f32 yy = y * y2, yz = y * z2, zz = z * z2;
    const f32 wx = w * x2, wy = w * y2, wz = w * z2;

    m.m[0][0] = 1.f - (yy + zz);
    m.m[0][1] = xy + wz;
    m.m[0][2] = xz - wy;
    m.m[0][3] = 0.f;

    m.m[1][0] = xy - wz;
    m.m[1][1] = 1.f - (xx + zz);
    m.m[1][2] = yz + wx;
    m.m[1][3] = 0.f;

    m.m[2][0] = xz + wy;
    m.m[2][1] = yz - wx;
    m.m[2][2] = 1.f - (xx + yy);
    m.m[2][3] = 0.f;

    m.m[3][3] = 1.f;
    return m;
}

SFT_INLINE sftquat sft_q_mul(sftquat a, sftquat b) {
    sftquat r;
    r.x =  a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
    r.y = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
    r.z =  a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
    r.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;
    return r;
}
SFT_INLINE sftquat sft_q_slerp(sftquat a, sftquat b, f32 t) {
    // sft_v_dot ignores w
    f32 dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (dot < 0.f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }

    if (dot > 0.9995f) {
        // lerp and 4D norm (sft_q_norm)
        sftquat res;
        res.x = SFT_LERPF(a.x, b.x, t);
        res.y = SFT_LERPF(a.y, b.y, t);
        res.z = SFT_LERPF(a.z, b.z, t);
        res.w = SFT_LERPF(a.w, b.w, t);
        return sft_q_norm(res);
    }

    const f32 theta0 = acosf(dot);
    const f32 theta = theta0 * t;
    const f32 sinTheta = sinf(theta);
    const f32 sinTheta0 = 1.f / sinf(theta0);

    const f32 s0 = cosf(theta) - dot * sinTheta * sinTheta0;
    const f32 s1 = sinTheta * sinTheta0;

    sftquat res;
    res.x = a.x * s0 + b.x * s1;
    res.y = a.y * s0 + b.y * s1;
    res.z = a.z * s0 + b.z * s1;
    res.w = a.w * s0 + b.w * s1;
    
    return sft_q_norm(res);
}
SFT_INLINE sftquat sft_q_invert(sftquat q) {
    sftquat res = q;

    const f32 len2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (0.f != len2) {
        const f32 invLength = 1.f / len2;
        res.x *= -invLength;
        res.y *= -invLength;
        res.z *= -invLength;
        res.w *= invLength;
    }

    return res;
}
SFT_INLINE sftquat sft_q_norm(sftquat q) {
    const f32 len2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (0.f != len2) {
        const f32 invLen = 1.f / sqrtf(len2);
        q.x *= invLen;
        q.y *= invLen;
        q.z *= invLen;
        q.w *= invLen;
    }
    return q;
}


SFT_INLINE sftvvec sft_vv_add(sftvvec a, sftvvec b) {
    sftvvec r;
    r.x = sft_vf_add(a.x, b.x);
    r.y = sft_vf_add(a.y, b.y);
    r.z = sft_vf_add(a.z, b.z);
    return r;
}
SFT_INLINE sftvvec sft_vv_sub(sftvvec a, sftvvec b) {
    sftvvec r;
    r.x = sft_vf_sub(a.x, b.x);
    r.y = sft_vf_sub(a.y, b.y);
    r.z = sft_vf_sub(a.z, b.z);
    return r;
}
SFT_INLINE sftvvec sft_vv_scale(sftvvec a, vf32 b) {
    sftvvec r;
    r.x = sft_vf_mul(a.x, b);
    r.y = sft_vf_mul(a.y, b);
    r.z = sft_vf_mul(a.z, b);
    return r;
}
SFT_INLINE vf32 sft_vv_dot(sftvvec a, sftvvec b) {
    const vf32 x = sft_vf_mul(a.x, b.x);
    const vf32 y = sft_vf_mul(a.y, b.y);
    const vf32 z = sft_vf_mul(a.z, b.z);
    return sft_vf_add(x, sft_vf_add(y, z)); 
}
SFT_INLINE sftvvec sft_vv_cross(sftvvec a, sftvvec b) {
    sftvvec r;
    r.x = sft_vf_sub(sft_vf_mul(a.y, b.z), sft_vf_mul(a.z, b.y));
    r.y = sft_vf_sub(sft_vf_mul(a.z, b.x), sft_vf_mul(a.x, b.z));
    r.z = sft_vf_sub(sft_vf_mul(a.x, b.y), sft_vf_mul(a.y, b.x));
    return r;
}
SFT_INLINE vf32 sft_vv_length2(sftvvec v) {
    return sft_vv_dot(v, v);
}
SFT_INLINE vf32 sft_vv_length(sftvvec v) {
    return sft_vf_sqrt(sft_vv_dot(v, v));
}
SFT_INLINE sftvvec sft_vv_norm(sftvvec v) {
    // avoid division by zero
    const vf32 epsilon = sft_vf_set1(SFT_EPSILON);
    vf32 len = sft_vv_length(v);
    len = sft_vf_max(len, epsilon); 
    
    sftvvec r;
    r.x = sft_vf_div(v.x, len);
    r.y = sft_vf_div(v.y, len);
    r.z = sft_vf_div(v.z, len);
    return r;
}
SFT_INLINE sftvvec sft_vv_norm_fast(sftvvec v) {
    const vf32 sq_len = sft_vv_dot(v, v);
    
    const vf32 approx = sft_vf_rsqrt(sq_len);
    
    // one iteration
    const vf32 half = sft_vf_set1(0.5f);
    const vf32 three = sft_vf_set1(3.f);
    const vf32 approx_sq = sft_vf_mul(approx, approx);
    
    // computes -(sq_len * approx_sq) + 3.0
    const vf32 refined = sft_vf_mul(sft_vf_mul(half, approx), 
                                    sft_vf_fnmadd(sq_len, approx_sq, three));
    
    sftvvec r;
    r.x = sft_vf_mul(v.x, refined);
    r.y = sft_vf_mul(v.y, refined);
    r.z = sft_vf_mul(v.z, refined);
    return r;
}
SFT_INLINE sftvvec sft_vv_transform(sftvvec v, sftvmat vm) {
    sftvvec r;
    
    // X' = X*m00 + Y*m10 + Z*m20 + m30
    r.x = sft_vf_fmadd(v.z, vm.m[2][0], vm.m[3][0]);
    r.x = sft_vf_fmadd(v.y, vm.m[1][0], r.x);
    r.x = sft_vf_fmadd(v.x, vm.m[0][0], r.x);

    // Y' = X*m01 + Y*m11 + Z*m21 + m31
    r.y = sft_vf_fmadd(v.z, vm.m[2][1], vm.m[3][1]);
    r.y = sft_vf_fmadd(v.y, vm.m[1][1], r.y);
    r.y = sft_vf_fmadd(v.x, vm.m[0][1], r.y);

    // Z' = X*m02 + Y*m12 + Z*m22 + m32
    r.z = sft_vf_fmadd(v.z, vm.m[2][2], vm.m[3][2]);
    r.z = sft_vf_fmadd(v.y, vm.m[1][2], r.z);
    r.z = sft_vf_fmadd(v.x, vm.m[0][2], r.z);

    return r;
}
SFT_INLINE sftvvec sft_vv_transform_dir(sftvvec v, sftvmat vm) {
    sftvvec r;
    // X' = X*m00 + Y*m10 + Z*m20
    r.x = sft_vf_mul(v.z, vm.m[2][0]);
    r.x = sft_vf_fmadd(v.y, vm.m[1][0], r.x);
    r.x = sft_vf_fmadd(v.x, vm.m[0][0], r.x);

    // Y' = X*m01 + Y*m11 + Z*m21
    r.y = sft_vf_mul(v.z, vm.m[2][1]);
    r.y = sft_vf_fmadd(v.y, vm.m[1][1], r.y);
    r.y = sft_vf_fmadd(v.x, vm.m[0][1], r.y);

    // Z' = X*m02 + Y*m12 + Z*m22
    r.z = sft_vf_mul(v.z, vm.m[2][2]);
    r.z = sft_vf_fmadd(v.y, vm.m[1][2], r.z);
    r.z = sft_vf_fmadd(v.x, vm.m[0][2], r.z);
    return r;
}

SFT_INLINE sftvmat sft_vm_broadcast(sftmat m) {
    sftvmat vm;
    
    // column 0
    vm.m[0][0] = sft_vf_set1(m.m[0][0]);
    vm.m[1][0] = sft_vf_set1(m.m[1][0]);
    vm.m[2][0] = sft_vf_set1(m.m[2][0]);
    vm.m[3][0] = sft_vf_set1(m.m[3][0]);

    // column 1
    vm.m[0][1] = sft_vf_set1(m.m[0][1]);
    vm.m[1][1] = sft_vf_set1(m.m[1][1]);
    vm.m[2][1] = sft_vf_set1(m.m[2][1]);
    vm.m[3][1] = sft_vf_set1(m.m[3][1]);

    // column 2
    vm.m[0][2] = sft_vf_set1(m.m[0][2]);
    vm.m[1][2] = sft_vf_set1(m.m[1][2]);
    vm.m[2][2] = sft_vf_set1(m.m[2][2]);
    vm.m[3][2] = sft_vf_set1(m.m[3][2]);

    // column 3 (optional, skip if 4x4 projections arent used)
    vm.m[0][3] = sft_vf_set1(m.m[0][3]);
    vm.m[1][3] = sft_vf_set1(m.m[1][3]);
    vm.m[2][3] = sft_vf_set1(m.m[2][3]);
    vm.m[3][3] = sft_vf_set1(m.m[3][3]);

    return vm;
}

#endif


/*================================================================
:: PRIVATE FUNCTIONS
================================================================*/

/*
SFT_INLINE f64 sft__get_ms(void) {
#if defined(_WIN32)
    // windows.h is already included in memory wrappers
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (f64)counter.QuadPart / (f64)freq.QuadPart * 1000.0;
#else
    #include <time.h>
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
#endif
}
*/

static SFT_INLINE i32 sft__tzcnt(i32 mask) {
    #if defined(_MSC_VER)
        unsigned long r;
        _BitScanForward(&r, mask);
        return (i32)r;
    #else
        return __builtin_ctz(mask);
    #endif
}

static SFT_INLINE i32 sft__get_toroidal_ind(i32 res, i32 x, i32 y, i32 z) {
    // NOTE assumes that res is a power of two
    const i32 wx = x & (res - 1);
    const i32 wy = y & (res - 1);
    const i32 wz = z & (res - 1);
    
    // flatten to 1D index (Z major, Y, X)
    return (wz * res * res) + (wy * res) + wx;
}

static SFT_INLINE i32 sft__aabb_vs_sphere(sftvec b_min, sftvec b_max, sftvec center, f32 radius) {
    f32 d2 = 0.f;
    
    if (center.x < b_min.x) {
        const f32 d = center.x - b_min.x;
        d2 += d * d;
    }
    else if (center.x > b_max.x) {
        const f32 d = center.x - b_max.x;
        d2 += d * d;
    }
    
    if (center.y < b_min.y) {
        const f32 d = center.y - b_min.y;
        d2 += d * d;
    }
    else if (center.y > b_max.y) {
        const f32 d = center.y - b_max.y;
        d2 += d * d;
    }
    
    if (center.z < b_min.z) {
        const f32 d = center.z - b_min.z;
        d2 += d * d;
    }
    else if (center.z > b_max.z) {
        const f32 d = center.z - b_max.z;
        d2 += d * d;
    }
    
    return d2 <= (radius * radius);
}

static SFT_INLINE i32 sft__fast_log2(i32 val) {
    i32 res = 0;
    while (val >>= 1) {
        res += 1;
    }
    return res;
}

static SFT_INLINE void sft__swizzle_rgba_to_argb(u32* pixels, i32 count) {
    for (i32 i = 0; i < count; i += 1) {
        const u32 c = pixels[i];
        const u32 r = (c >> 0)  & 0xFF;
        const u32 g = (c >> 8)  & 0xFF;
        const u32 b = (c >> 16) & 0xFF;
        const u32 a = (c >> 24) & 0xFF;
        pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
}

static i32 sft__get_physical_core_count(void) {
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors > 0 ? (i32)sysinfo.dwNumberOfProcessors : 1;
#elif defined(__linux__) || defined(__APPLE__)
    i32 cores = sysconf(_SC_NPROCESSORS_ONLN);
    return cores > 0 ? cores : 1;
#else
    return 1;
#endif
}

static struct sft_sphere sft__sphere_from_2(sftvec a, sftvec b) {
    struct sft_sphere s;
    s.center = sft_v_mul(sft_v_add(a, b), 0.5f);
    s.radius = sft_v_length(sft_v_sub(a, b)) * 0.5f;
    return s;
}

static struct sft_sphere sft__sphere_from_3(sftvec p0, sftvec p1, sftvec p2) {
    const sftvec a = sft_v_sub(p1, p0);
    const sftvec b = sft_v_sub(p2, p0);
    const sftvec a_cross_b = sft_v_cross(a, b);
    
    const f32 denom = 2.f * sft_v_length2(a_cross_b);
    if (denom < SFT_EPSILON) {
        return sft__sphere_from_2(p0, p1); // degenerate triangle fallback
    }

    const sftvec term1 = sft_v_mul(b, sft_v_length2(a));
    const sftvec term2 = sft_v_mul(a, sft_v_length2(b));
    const sftvec cross_term = sft_v_cross(sft_v_sub(term1, term2), a_cross_b);
    
    struct sft_sphere s;
    s.center = sft_v_add(p0, sft_v_div(cross_term, denom));
    s.radius = sft_v_length(sft_v_sub(s.center, p0));
    return s;
}

static struct sft_sphere sft__sphere_from_4(sftvec p0, sftvec p1, sftvec p2, sftvec p3) {
    const sftvec r1 = sft_v_sub(p1, p0);
    const sftvec r2 = sft_v_sub(p2, p0);
    const sftvec r3 = sft_v_sub(p3, p0);

    const f32 sql1 = sft_v_length2(r1);
    const f32 sql2 = sft_v_length2(r2);
    const f32 sql3 = sft_v_length2(r3);

    const f32 det = r1.x * (r2.y*r3.z - r3.y*r2.z) - 
                    r1.y * (r2.x*r3.z - r3.x*r2.z) + 
                    r1.z * (r2.x*r3.y - r3.x*r2.y);

    if (fabsf(det) < SFT_EPSILON) {
        return sft__sphere_from_3(p0, p1, p2); // coplanar fallback
    }

    const f32 f = 0.5f / det;

    sftvec offset;
    offset.x = f * ((r2.y*r3.z - r3.y*r2.z) * sql1 - 
                    (r1.y*r3.z - r3.y*r1.z) * sql2 + 
                    (r1.y*r2.z - r2.y*r1.z) * sql3);
                    
    offset.y = f * (-(r2.x*r3.z - r3.x*r2.z) * sql1 + 
                     (r1.x*r3.z - r3.x*r1.z) * sql2 - 
                     (r1.x*r2.z - r2.x*r1.z) * sql3);
                     
    offset.z = f * ((r2.x*r3.y - r3.x*r2.y) * sql1 - 
                    (r1.x*r3.y - r3.x*r1.y) * sql2 + 
                    (r1.x*r2.y - r2.x*r1.y) * sql3);

    struct sft_sphere s;
    s.center = sft_v_add(p0, offset);
    s.radius = sft_v_length(offset);
    return s;
}

static struct sft_sphere sft__sphere_exact(sftvec* sos, i32 sos_count) {
    switch (sos_count) {
        case 0:  return (struct sft_sphere){0};
        case 1:  return (struct sft_sphere){ .center = sos[0], .radius = 0.f };
        case 2:  return sft__sphere_from_2(sos[0], sos[1]);
        case 3:  return sft__sphere_from_3(sos[0], sos[1], sos[2]);
        default: return sft__sphere_from_4(sos[0], sos[1], sos[2], sos[3]);
    }
}

static struct sft_sphere sft__welzl_recursive(sftvec* points, i32 p_count, sftvec* sos, i32 sos_count) {
    // base cases
    if (0 == p_count || 4 == sos_count) {
        return sft__sphere_exact(sos, sos_count);
    }

    // Welzl's expects random permutation, pop a random point
    const i32 rand_ind = rand() % p_count;
    const sftvec p = points[rand_ind];
    SFT_SWAP(sftvec, points[rand_ind], points[p_count - 1]);

    // recurse without the chosen point
    const struct sft_sphere s = sft__welzl_recursive(points, p_count - 1, sos, sos_count);

    // if the removed point is inside the returned sphere, good
    const f32 dist2 = sft_v_length2(sft_v_sub(p, s.center));
    if (dist2 <= (s.radius * s.radius) + SFT_EPSILON) {
        return s;
    }

    // otherwise, the point must be on the boundary
    sos[sos_count] = p;
    return sft__welzl_recursive(points, p_count - 1, sos, sos_count + 1);
}

static struct sft_sphere sft__meshlet_bounding_sphere(sftvec* meshlet_verts, i32 vert_count) {
    sftvec sos[4];
    
    // copy points to not destroy original data
    sftvec points_copy[SFT_VERTS_PER_MESHLET];
    for (i32 i = 0; i < vert_count; i += 1) {
        points_copy[i] = meshlet_verts[i];
    }

    return sft__welzl_recursive(points_copy, vert_count, sos, 0);
}

static u32 sft__hash_pos(f32 x, f32 y, f32 z, f32 cell_size, u32 table_size) {
    const i32 qx = (i32)floorf(x / cell_size);
    const i32 qy = (i32)floorf(y / cell_size);
    const i32 qz = (i32)floorf(z / cell_size);

    u32 h = ((u32)qx * SFT_HASH_PRIME_X) ^ 
            ((u32)qy * SFT_HASH_PRIME_Y) ^ 
            ((u32)qz * SFT_HASH_PRIME_Z);
            
    return h % table_size;
}

static void sft_hash_insert(struct sft_spatial_hash* hash, f32 vx, f32 vy, f32 vz, i32 triangle_id) {
    if (hash->entry_count >= hash->max_entries) {
        return;
    }
    
    const u32 h = sft__hash_pos(vx, vy, vz, hash->cell_size, hash->table_size);
    const u32 entry_ind = hash->entry_count;
    hash->entry_count += 1;
    
    // record the triangle ID
    hash->tri_ind[entry_ind] = triangle_id;
    
    // link to the previous head of this bucket
    hash->next[entry_ind] = hash->head[h];
    
    // update the bucket to point to this new entry
    hash->head[h] = entry_ind;
}

static f32 sft__builder_score_candidate(
    const struct sft_meshlet_builder* builder,
    i32 global_v0, i32 global_v1, i32 global_v2,
    sftvec candidate_normal
) {
    if (builder->tri_count >= 128) {
        return -1.f;
    }

    // if the meshlet already has a general direction, check divergence
    if (builder->tri_count > 0) {
        const f32 div = sft_v_dot(builder->cone_axis, candidate_normal);
        if (div < SFT_CONE_LIMIT) {
            return -1.f; // exceeds 100 degrees, reject
        }
    }

    // ---  check vertex reuse to enforce the 64 vert limit  ---
    u32 new_verts_needed = 3;
    u32 shared_verts = 0;
    for (u32 i = 0; i < builder->vert_count; i += 1) {
        const i32 ex_vert = builder->global_vert_inds[i];
        if (ex_vert == global_v0 || ex_vert == global_v1 || ex_vert == global_v2) {
            new_verts_needed -= 1;
            shared_verts += 1;
        }
    }
    if (builder->vert_count + new_verts_needed > SFT_VERTS_PER_MESHLET) {
        return -1.f;
    }

    // ---  calculate final score  ---
    //  prioritize tris that reuse the most verts to keep
    //  the meshlet dense add the normal divergence as a secondary
    //  weight to prefer flatter surfaces
    f32 score = (f32)shared_verts;
    if (builder->tri_count > 0) {
        score += sft_v_dot(builder->cone_axis, candidate_normal) * 0.5f;
    }

    return score;
}

static void sft__builder_add_triangle(
    struct sft_meshlet_builder* builder,
    i32 global_v0, i32 global_v1, i32 global_v2,
    sftvec triangle_normal
) {
    i32 localv[3] = { -1, -1, -1 };
    i32 globalv[3] = { global_v0, global_v1, global_v2 };

    // ---  find existing vertices or add new ones  ---
    for (i32 v = 0; v < 3; v += 1) {
        for (i32 i = 0; i < (i32)builder->vert_count; i += 1) {
            if (builder->global_vert_inds[i] == globalv[v]) {
                localv[v] = i;
                break;
            }
        }

        if (-1 == localv[v]) {
            // vertex not found in meshlet, append it
            localv[v] = builder->vert_count;
            builder->global_vert_inds[builder->vert_count] = globalv[v];
            builder->vert_count += 1;
        }
    }

    // append local inds
    const u32 ti = builder->tri_count * 3;
    builder->local_inds[ti + 0] = (u8)localv[0];
    builder->local_inds[ti + 1] = (u8)localv[1];
    builder->local_inds[ti + 2] = (u8)localv[2];

    // update the running cone axis (simple average, normalize later)
    builder->cone_axis = sft_v_add(builder->cone_axis, triangle_normal);
    if (1 == builder->tri_count) {
        builder->cone_axis = sft_v_norm(builder->cone_axis);
    }
    else {
        // renormalize occasionally or at the end to prevent precision drift
        builder->cone_axis = sft_v_norm(builder->cone_axis);
    }

    builder->tri_count += 1;
}

static void sft__builder_finalize(
    const struct sft_meshlet_builder* builder, 
    const f32* raw_vx, const f32* raw_vy, const f32* raw_vz, // global flat vertex arrays
    struct sft_meshlet* out_meshlet,
    struct sft_meshlet_inds* out_inds,
    struct sft_sphere* out_sphere,
    struct sft_aabb* out_aabb
) {
    // ---  swizzle inds into 32 byte SIMD batches  ---
    const u32 batch_count = 128 / SFT_SIMD_WIDTH;
    const u32 batch_bytes = SFT_SIMD_WIDTH * 4;
    for (u32 batch = 0; batch < batch_count; batch += 1) {
        const u32 batch_offset = batch * batch_bytes;

        for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
            const u32 tri_ind = batch * SFT_SIMD_WIDTH + lane;

            u8 v0 = 0, v1 = 0, v2 = 0;

            if (tri_ind < builder->tri_count) {
                v0 = builder->local_inds[tri_ind * 3 + 0];
                v1 = builder->local_inds[tri_ind * 3 + 1];
                v2 = builder->local_inds[tri_ind * 3 + 2];
            }

            out_inds->inds[batch_offset + (0 * SFT_SIMD_WIDTH) + lane] = v0;
            out_inds->inds[batch_offset + (1 * SFT_SIMD_WIDTH) + lane] = v1;
            out_inds->inds[batch_offset + (2 * SFT_SIMD_WIDTH) + lane] = v2;
            out_inds->inds[batch_offset + (3 * SFT_SIMD_WIDTH) + lane] = 0; // padding
        }
    }

    // temp buffer for Welzl's
    sftvec temp_verts[SFT_VERTS_PER_MESHLET];

    struct sft_aabb aabb = {
        .min = {  1e30f,  1e30f,  1e30f },
        .max = { -1e30f, -1e30f, -1e30f }
    };

    // ---  repack verts into SoA layout  ---
    for (u32 i = 0; i < SFT_VERTS_PER_MESHLET; i += 1) {
        if (i < builder->vert_count) {
            const i32 gi = builder->global_vert_inds[i];
            out_meshlet->x[i] = raw_vx[gi];
            out_meshlet->y[i] = raw_vy[gi];
            out_meshlet->z[i] = raw_vz[gi];

            temp_verts[i].x = raw_vx[gi];
            temp_verts[i].y = raw_vy[gi];
            temp_verts[i].z = raw_vz[gi];

            if (raw_vx[gi] < aabb.min.x) aabb.min.x = raw_vx[gi];
            if (raw_vy[gi] < aabb.min.y) aabb.min.y = raw_vy[gi];
            if (raw_vz[gi] < aabb.min.z) aabb.min.z = raw_vz[gi];
            if (raw_vx[gi] > aabb.max.x) aabb.max.x = raw_vx[gi];
            if (raw_vy[gi] > aabb.max.y) aabb.max.y = raw_vy[gi];
            if (raw_vz[gi] > aabb.max.z) aabb.max.z = raw_vz[gi];
        }
        else {
            out_meshlet->x[i] = 0.f;
            out_meshlet->y[i] = 0.f;
            out_meshlet->z[i] = 0.f;
        }
    }

    if (out_sphere && builder->vert_count > 0) {
        *out_sphere = sft__meshlet_bounding_sphere(temp_verts, builder->vert_count);
    }
    if (out_aabb && builder->vert_count > 0) {
        *out_aabb = aabb;
    }
}

static void sft__builder_reset(struct sft_meshlet_builder* builder) {
    builder->vert_count = 0;
    builder->tri_count = 0;
    builder->cone_axis = (sftvec){0};
}

static void sft__barrier_init(struct sft_barrier* b, i32 thread_count) {
#if defined(_WIN32)
    b->count = 0;
    b->generation = 0;
    b->thread_count = thread_count;
#else
    pthread_barrier_init(&b->b, NULL, thread_count);
#endif
}

static SFT_INLINE void sft__barrier_wait(struct sft_barrier* b) {
#if defined(_WIN32)
    const i32 local_gen = b->generation;
    const i32 arrived = sft_atomic_add(&b->count, 1) + 1;
    
    if (arrived == b->thread_count) {
        // last thread to arrive resets the count and flips the generation
        b->count = 0;
        sft_atomic_add(&b->generation, 1);
        
        // wake all threads sleeping on the generation address
        WakeByAddressAll((PVOID)&b->generation);
    }
    else {
        // sleep at the OS level until generation changes
        while (local_gen == b->generation) {
            // this puts the thread to sleep, yielding the CPU entirely,
            // it will wake up when WakeByAddressAll is called, or spuriously
            WaitOnAddress((volatile PVOID)&b->generation, (PVOID)&local_gen, sizeof(i32), INFINITE);
        }
    }
#else
    pthread_barrier_wait(&b->b);
#endif
}

static void sft__mesh_build(
    sft_mesh* out_mesh,
    sft_arena* arena,
    const f32* raw_vx, const f32* raw_vy, const f32* raw_vz,
    const i32* raw_inds, i32 raw_tri_count
) {
    // find max vertex ID to size the adjacency arrays
    i32 max_vertex = 0;
    for (i32 i = 0; i < raw_tri_count * 3; i += 1) {
        if (raw_inds[i] > max_vertex) {
            max_vertex = raw_inds[i];
        }
    }
    const i32 vertex_count = max_vertex + 1;

    // ---  adjacency structure gen  ---    
    i32* vertex_offsets; // stores the starting ind in the adjacency array for each vertex
    SFT_CALLOC(vertex_offsets, vertex_count + 1, i32);
    
    i32* adjacency; // stores a flat list of triangle IDs connected to each vertex
    SFT_CALLOC(adjacency, raw_tri_count * 3, i32);
    
    // count how many triangles touch each vertex
    for (i32 i = 0; i < raw_tri_count * 3; i += 1) {
        vertex_offsets[raw_inds[i]] += 1;
    }

    // prefix sum to convert counts into flat array offsets
    i32 prefix = 0;
    for (i32 i = 0; i < vertex_count; i += 1) {
        const i32 count = vertex_offsets[i];
        vertex_offsets[i] = prefix;
        prefix += count;
    }
    vertex_offsets[vertex_count] = prefix;

    i32* write_heads; // temporary copy of the offsets to act as write heads when populating the adjacency array
    SFT_CALLOC(write_heads, vertex_count, i32);
    memcpy(write_heads, vertex_offsets, vertex_count * sizeof(i32));

    // populate the adjacency array with triangle IDs
    for (i32 i = 0; i < raw_tri_count; i += 1) {
        for (i32 v = 0; v < 3; v += 1) {
            const i32 vert = raw_inds[i * 3 + v];
            const i32 pos = write_heads[vert];
            write_heads[vert] += 1;
            adjacency[pos] = i;
        }
    }

    // ---  meshlet allocation  ---
    // absolute worst case is every single triangle facing a different direction and being isolated
    i32 max_meshlets = raw_tri_count;
    
    // pad allocation
    max_meshlets = (max_meshlets + SFT_SIMD_WIDTH - 1) & ~(SFT_SIMD_WIDTH - 1);

    // allocate arrays for the meshlets and culling metadata
    out_mesh->meshlets = (struct sft_meshlet*)sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet));
    out_mesh->inds     = (struct sft_meshlet_inds*)sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet_inds));
    
    out_mesh->bounds_center_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_center_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_center_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_radius   = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

    out_mesh->bounds_min_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_min_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_min_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_max_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_max_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->bounds_max_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    
    out_mesh->cone_axis_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->cone_axis_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->cone_axis_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
    out_mesh->cone_cutoff = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

    u8* used_tris; // track which raw triangles have already been placed into a meshlet
    SFT_CALLOC(used_tris, raw_tri_count, u8);

    // ---  frontier queue setup  ---
    i32* frontier;
    u8* in_frontier;
    SFT_CALLOC(frontier, raw_tri_count, i32);
    SFT_CALLOC(in_frontier, raw_tri_count, u8);

    i32 unmapped_tris = raw_tri_count;
    u32 meshlet_count = 0;

    struct sft_meshlet_builder builder;

    while (unmapped_tris > 0) {
        sft__builder_reset(&builder);

        i32 frontier_head = 0;
        i32 frontier_tail = 0;

        // find a seed triangle (first unused triangle in the array)
        i32 seed_tri = -1;
        for (i32 i = 0; i < raw_tri_count; i += 1) {
            if (!used_tris[i]) {
                seed_tri = i;
                break;
            }
        }
        
        if (-1 == seed_tri) {
            break;
        }

        // push seed triangle into the builder
        const i32 v0 = raw_inds[seed_tri * 3 + 0];
        const i32 v1 = raw_inds[seed_tri * 3 + 1];
        const i32 v2 = raw_inds[seed_tri * 3 + 2];
        
        const sftvec p0 = SFT_V(raw_vx[v0], raw_vy[v0], raw_vz[v0]);
        const sftvec p1 = SFT_V(raw_vx[v1], raw_vy[v1], raw_vz[v1]);
        const sftvec p2 = SFT_V(raw_vx[v2], raw_vy[v2], raw_vz[v2]);
        const sftvec normal = sft_v_face_normal(p0, p1, p2);

        sft__builder_add_triangle(&builder, v0, v1, v2, normal);
        used_tris[seed_tri] = 1;
        unmapped_tris -= 1;

        // add the seed's connected triangles to the frontier
        const i32 seed_verts[3] = {v0, v1, v2};
        for (i32 v = 0; v < 3; v += 1) {
            const i32 vert = seed_verts[v];
            const i32 start_ind = vertex_offsets[vert];
            const i32 end_ind = vertex_offsets[vert + 1];
            
            for (i32 ai = start_ind; ai < end_ind; ai += 1) {
                const i32 adj_tri = adjacency[ai];
                if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                    frontier[frontier_tail] = adj_tri;
                    frontier_tail += 1;
                    in_frontier[adj_tri] = 1;
                }
            }
        }

        // ---  fast adjacency based greedy search  ---
        while (builder.vert_count < SFT_VERTS_PER_MESHLET && builder.tri_count < 128 && frontier_head < frontier_tail) {
            f32 best_score = -1.f;
            i32 best_tri_ind = -1;
            sftvec best_normal = {0};

            // only scan the triangles inside the frontier queue, not the whole mesh
            for (i32 i = frontier_head; i < frontier_tail; i += 1) {
                const i32 candidate_tri = frontier[i];
                if (used_tris[candidate_tri]) {
                    continue; // it was consumed earlier in this loop
                }

                const i32 cv0 = raw_inds[candidate_tri * 3 + 0];
                const i32 cv1 = raw_inds[candidate_tri * 3 + 1];
                const i32 cv2 = raw_inds[candidate_tri * 3 + 2];
                
                const sftvec cp0 = SFT_V(raw_vx[cv0], raw_vy[cv0], raw_vz[cv0]);
                const sftvec cp1 = SFT_V(raw_vx[cv1], raw_vy[cv1], raw_vz[cv1]);
                const sftvec cp2 = SFT_V(raw_vx[cv2], raw_vy[cv2], raw_vz[cv2]);
                const sftvec c_normal = sft_v_face_normal(cp0, cp1, cp2);

                const f32 score = sft__builder_score_candidate(&builder, cv0, cv1, cv2, c_normal);
                
                if (score > best_score) {
                    best_score = score;
                    best_tri_ind = i; // save the index inside the frontier array
                    best_normal = c_normal;
                }
            }

            if (-1 == best_tri_ind) {
                break; // no more triangles in the frontier fit the vertex or cone limits
            }

            // extract the winning triangle
            const i32 best_tri = frontier[best_tri_ind];
            const i32 bv0 = raw_inds[best_tri * 3 + 0];
            const i32 bv1 = raw_inds[best_tri * 3 + 1];
            const i32 bv2 = raw_inds[best_tri * 3 + 2];

            sft__builder_add_triangle(&builder, bv0, bv1, bv2, best_normal);
            used_tris[best_tri] = 1;
            unmapped_tris -= 1;

            // remove the triangle from the frontier by swapping with the head
            frontier[best_tri_ind] = frontier[frontier_head];
            frontier_head += 1; 

            // add the new triangle's neighbors to the frontier
            const i32 new_verts[3] = {bv0, bv1, bv2};
            for (i32 v = 0; v < 3; v += 1) {
                const i32 vert = new_verts[v];
                const i32 start_ind = vertex_offsets[vert];
                const i32 end_ind = vertex_offsets[vert + 1];
                
                for (i32 adj_ind = start_ind; adj_ind < end_ind; adj_ind += 1) {
                    const i32 adj_tri = adjacency[adj_ind];
                    if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                        frontier[frontier_tail] = adj_tri;
                        frontier_tail += 1;
                        in_frontier[adj_tri] = 1;
                    }
                }
            }
        }

        // clean up the trackers for the next meshlet seed
        for (i32 i = frontier_head; i < frontier_tail; i += 1) {
            in_frontier[frontier[i]] = 0;
        }

        // ---  finalize the chunk and write to the arrays  ---
        struct sft_sphere sphere;
        struct sft_aabb aabb;
        sft__builder_finalize(
            &builder, raw_vx, raw_vy, raw_vz, 
            &out_mesh->meshlets[meshlet_count], 
            &out_mesh->inds[meshlet_count], 
            &sphere, &aabb);

        out_mesh->bounds_center_x[meshlet_count] = sphere.center.x;
        out_mesh->bounds_center_y[meshlet_count] = sphere.center.y;
        out_mesh->bounds_center_z[meshlet_count] = sphere.center.z;
        out_mesh->bounds_radius[meshlet_count]   = sphere.radius;

        out_mesh->bounds_min_x[meshlet_count] = aabb.min.x;
        out_mesh->bounds_min_y[meshlet_count] = aabb.min.y;
        out_mesh->bounds_min_z[meshlet_count] = aabb.min.z;
        out_mesh->bounds_max_x[meshlet_count] = aabb.max.x;
        out_mesh->bounds_max_y[meshlet_count] = aabb.max.y;
        out_mesh->bounds_max_z[meshlet_count] = aabb.max.z;

        // calculate exact cone cutoff
        sftvec true_axis = sft_v_norm(builder.cone_axis);
        f32 min_dot = 1.f;
        
        // loop through all triangles in this meshlet to find the true max divergence
        for (u32 t = 0; t < builder.tri_count; t += 1) {
            const u8 v0 = builder.local_inds[t * 3 + 0];
            const u8 v1 = builder.local_inds[t * 3 + 1];
            const u8 v2 = builder.local_inds[t * 3 + 2];
            
            const sftvec p0 = SFT_V(out_mesh->meshlets[meshlet_count].x[v0],
                                    out_mesh->meshlets[meshlet_count].y[v0],
                                    out_mesh->meshlets[meshlet_count].z[v0]);
            const sftvec p1 = SFT_V(out_mesh->meshlets[meshlet_count].x[v1],
                                    out_mesh->meshlets[meshlet_count].y[v1],
                                    out_mesh->meshlets[meshlet_count].z[v1]);
            const sftvec p2 = SFT_V(out_mesh->meshlets[meshlet_count].x[v2],
                                    out_mesh->meshlets[meshlet_count].y[v2],
                                    out_mesh->meshlets[meshlet_count].z[v2]);
                              
            const sftvec n = sft_v_face_normal(p0, p1, p2);
            const f32 dot_val = sft_v_dot(n, true_axis);
            if (dot_val < min_dot) {
                min_dot = dot_val;
            }
        }
        
        f32 sin_alpha;
        if (min_dot < 0.f) {
            sin_alpha = 1.f; // cone exceeds a hemisphere, safely disable culling
        }
        else {
            sin_alpha = sqrtf(1.f - min_dot * min_dot);
        }

        out_mesh->cone_axis_x[meshlet_count] = true_axis.x;
        out_mesh->cone_axis_y[meshlet_count] = true_axis.y;
        out_mesh->cone_axis_z[meshlet_count] = true_axis.z;
        
        // add a slight padding for inaccuracies near the silhouette
        out_mesh->cone_cutoff[meshlet_count] = sin_alpha + 0.01f;

        meshlet_count += 1;
    }

    out_mesh->meshlet_count = meshlet_count;

    free(vertex_offsets);
    free(adjacency);
    free(write_heads);
    free(used_tris);
    free(frontier);
    free(in_frontier);
}

static void sft__dmesh_finalize_chunk(sft_dmesh* dm) {
    if (0 == dm->curr_vert) {
        return; // nothing to finalize
    }

    const u32 m_ind = dm->curr_meshlet;
    if (m_ind >= dm->max_meshlets) {
        return; // over allocation guard
    }

    struct sft_meshlet* m = &dm->meshlets[m_ind];
    struct sft_meshlet_attribs* attr = &dm->attribs[m_ind];

    // collapse trailing unused verts onto v0
    // so template will generate zero area degen tris
    for (u32 i = dm->curr_vert; i < SFT_VERTS_PER_MESHLET; i += 1) {
        m->x[i] = m->x[0], m->y[i] = m->y[0], m->z[i] = m->z[0];
        attr->nx[i] = attr->nx[0], attr->ny[i] = attr->ny[0], attr->nz[i] = attr->nz[0];
        attr->u[i] = attr->u[0], attr->v[i] = attr->v[0];
        attr->lu[i] = attr->lu[0], attr->lv[i] = attr->lv[0];
    }

    // calculate AABB
    f32 min_x = 1e30f, max_x = -1e30f;
    f32 min_y = 1e30f, max_y = -1e30f;
    f32 min_z = 1e30f, max_z = -1e30f;
    for (u32 i = 0; i < dm->curr_vert; i += 1) {
        const f32 x = m->x[i], y = m->y[i], z = m->z[i];
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        if (z < min_z) min_z = z;
        if (z > max_z) max_z = z;
    }

    // derive bounding spheres and write to arrays
    const f32 cx = (min_x + max_x) * 0.5f;
    const f32 cy = (min_y + max_y) * 0.5f;
    const f32 cz = (min_z + max_z) * 0.5f;
    const f32 dx = max_x - cx, dy = max_y - cy, dz = max_z - cz;
    const f32 r = sqrtf(dx*dx + dy*dy + dz*dz);

    dm->bounds_min_x[m_ind] = min_x, dm->bounds_max_x[m_ind] = max_x;
    dm->bounds_min_y[m_ind] = min_y, dm->bounds_max_y[m_ind] = max_y;
    dm->bounds_min_z[m_ind] = min_z, dm->bounds_max_z[m_ind] = max_z;

    dm->bounds_center_x[m_ind] = cx;
    dm->bounds_center_y[m_ind] = cy;
    dm->bounds_center_z[m_ind] = cz;
    dm->bounds_radius[m_ind] = r;

    // bypass normal cone culling
    dm->cone_axis_x[m_ind] = 0.f;
    dm->cone_axis_y[m_ind] = 0.f;
    dm->cone_axis_z[m_ind] = 1.f;
    dm->cone_cutoff[m_ind] = 2.f; // > 1.0 => "dot <= threshold" always true

    // copy template
    const struct sft_meshlet_inds* template_inds = 
        (SFT_DMESH_QUADS == dm->type) ? &sft_state->dmesh.quad_inds :
        /*SFT_DMESH_TRIS == dm->type*/  &sft_state->dmesh.tri_inds;
    memcpy(&dm->inds[m_ind], template_inds, sizeof(struct sft_meshlet_inds));

    // advance tracking state
    dm->curr_meshlet += 1;
    dm->curr_vert = 0;
}

static SFT_INLINE f32 sft__build_item_axis_value(const struct sft_blas_build_item* item, i32 axis) {
    switch (axis) {
        case 0: return item->centroid.x;
        case 1: return item->centroid.y;
        case 2: return item->centroid.z;
        default: SFT_ERR_EXIT("impossible axis '%d'\n", axis);
    }
}

// quicksort for build items along a given axis (0=X, 1=Y, 2=Z)
static void sft__sort_build_items(struct sft_blas_build_item* items, i32 count, i32 axis) {
    if (count <= 1) {
        return;
    }

    const f32 pivot = sft__build_item_axis_value(&items[count / 2], axis);

    i32 i = 0, j = count - 1;
    while (i <= j) {
        while (sft__build_item_axis_value(&items[i], axis) < pivot) {
            i += 1;
        }
        while (sft__build_item_axis_value(&items[j], axis) > pivot) {
            j -= 1;
        }

        if (i <= j) {
            SFT_SWAP(struct sft_blas_build_item, items[i], items[j]);
            i += 1;
            j -= 1;
        }
    }

    if (j > 0) {
        sft__sort_build_items(items, j + 1, axis);
    }
    if (i < count) {
        sft__sort_build_items(items + i, count - i, axis);
    }
}

static void sft__partition_to_8(
    struct sft_blas_build_item* items, u32 start, u32 count, u32 depth,
    u32* out_starts, u32* out_counts, u32* out_num_groups
) {
    if (0 == count) {
        return;
    }
    
    if (3 == depth || 1 == count) {
        const u32 ind = *out_num_groups;
        out_starts[ind] = start;
        out_counts[ind] = count;
        *out_num_groups += 1;
        return;
    }
    
    struct sft_aabb c_bounds = { .min = { 1e30f, 1e30f, 1e30f }, .max = {-1e30f, -1e30f, -1e30f} };
    for (u32 i = 0; i < count; i += 1) {
        if (items[start + i].centroid.x < c_bounds.min.x) c_bounds.min.x = items[start + i].centroid.x;
        if (items[start + i].centroid.y < c_bounds.min.y) c_bounds.min.y = items[start + i].centroid.y;
        if (items[start + i].centroid.z < c_bounds.min.z) c_bounds.min.z = items[start + i].centroid.z;
        if (items[start + i].centroid.x > c_bounds.max.x) c_bounds.max.x = items[start + i].centroid.x;
        if (items[start + i].centroid.y > c_bounds.max.y) c_bounds.max.y = items[start + i].centroid.y;
        if (items[start + i].centroid.z > c_bounds.max.z) c_bounds.max.z = items[start + i].centroid.z;
    }
    
    const f32 dx = c_bounds.max.x - c_bounds.min.x;
    const f32 dy = c_bounds.max.y - c_bounds.min.y;
    const f32 dz = c_bounds.max.z - c_bounds.min.z;
    
    const i32 axis =
        (dy > dx && dy > dz) ? 1 :
        (dz > dx && dz > dy) ? 2 : 0;

    sft__sort_build_items(items + start, (i32)count, axis);
    
    const u32 mid = count / 2;
    sft__partition_to_8(items, start, mid, depth + 1, out_starts, out_counts, out_num_groups);
    sft__partition_to_8(items, start + mid, count - mid, depth + 1, out_starts, out_counts, out_num_groups);
}

static u32 sft__bvh_build_recursive(
    struct sft_blas_build_item* items, u32 count,
    struct sft_bvh_node* out_nodes, u32* out_node_count,
    struct sft_aabb* out_node_aabb
) {
    // allocate a node slot
    const u32 node_ind = *out_node_count;
    *out_node_count += 1;
    struct sft_bvh_node* node = &out_nodes[node_ind];

    // initialize all 8 lanes with empty/inverted bounds and invalid indices
    for (i32 i = 0; i < 8; i += 1) {
        node->min_x[i] =  1e30f; node->min_y[i] =  1e30f; node->min_z[i] =  1e30f;
        node->max_x[i] = -1e30f; node->max_y[i] = -1e30f; node->max_z[i] = -1e30f;
        node->child_indices[i] = 0xFFFFFFFF;
    }

    struct sft_aabb bounds = {
        .min = {  1e30f,  1e30f,  1e30f },
        .max = { -1e30f, -1e30f, -1e30f }
    };

    // ---  1, leaf group (<= 8 meshlets)  ---
    if (count <= 8) {
        node->is_leaf_group = 1;
        node->child_count = (u8)count;

        for (u32 i = 0; i < count; i += 1) {
            const struct sft_blas_build_item* item = &items[i];
            
            node->min_x[i] = item->aabb.min.x;
            node->min_y[i] = item->aabb.min.y;
            node->min_z[i] = item->aabb.min.z;
            node->max_x[i] = item->aabb.max.x;
            node->max_y[i] = item->aabb.max.y;
            node->max_z[i] = item->aabb.max.z;

            // set bit 31 to signify a leaf meshlet payload
            node->child_indices[i] = 0x80000000 | item->global_meshlet_ind;

            // expand node overall AABB
            if (item->aabb.min.x < bounds.min.x) bounds.min.x = item->aabb.min.x;
            if (item->aabb.min.y < bounds.min.y) bounds.min.y = item->aabb.min.y;
            if (item->aabb.min.z < bounds.min.z) bounds.min.z = item->aabb.min.z;
            if (item->aabb.max.x > bounds.max.x) bounds.max.x = item->aabb.max.x;
            if (item->aabb.max.y > bounds.max.y) bounds.max.y = item->aabb.max.y;
            if (item->aabb.max.z > bounds.max.z) bounds.max.z = item->aabb.max.z;
        }

        if (out_node_aabb) {
            *out_node_aabb = bounds;
        }
        return node_ind;
    }

    // ---  2, internal node (> 8 meshlets)  ---
    node->is_leaf_group = 0;

    u32 out_starts[8] = {0};
    u32 out_counts[8] = {0};
    u32 num_children = 0;

    sft__partition_to_8(items, 0, count, 0, out_starts, out_counts, &num_children);

    node->child_count = (u8)num_children;

    for (u32 c = 0; c < num_children; c += 1) {
        const u32 start = out_starts[c];
        const u32 child_item_count = out_counts[c];

        struct sft_aabb child_aabb;
        const u32 child_node_ind = sft__bvh_build_recursive(
            &items[start], child_item_count, 
            out_nodes, out_node_count, &child_aabb);

        node->child_indices[c] = child_node_ind;
        node->min_x[c] = child_aabb.min.x;
        node->min_y[c] = child_aabb.min.y;
        node->min_z[c] = child_aabb.min.z;
        node->max_x[c] = child_aabb.max.x;
        node->max_y[c] = child_aabb.max.y;
        node->max_z[c] = child_aabb.max.z;

        if (child_aabb.min.x < bounds.min.x) bounds.min.x = child_aabb.min.x;
        if (child_aabb.min.y < bounds.min.y) bounds.min.y = child_aabb.min.y;
        if (child_aabb.min.z < bounds.min.z) bounds.min.z = child_aabb.min.z;
        if (child_aabb.max.x > bounds.max.x) bounds.max.x = child_aabb.max.x;
        if (child_aabb.max.y > bounds.max.y) bounds.max.y = child_aabb.max.y;
        if (child_aabb.max.z > bounds.max.z) bounds.max.z = child_aabb.max.z;
    }

    if (out_node_aabb) {
        *out_node_aabb = bounds;
    }
    return node_ind;
}

// gap sealing capsule fitter
static void sft__fit_capsule_to_verts(sftvec* verts, i32 count, struct sft_shadow_proxy* out_proxy) {
    if (0 == count) {
        *out_proxy = (struct sft_shadow_proxy){0};
        return;
    }

    sftvec centroid = {0};
    for (i32 i = 0; i < count; i += 1) {
        centroid = sft_v_add(centroid, verts[i]);
    }
    centroid = sft_v_mul(centroid, 1.f / count);

    f32 cov[3][3] = {0};
    for (i32 k = 0; k < count; k += 1) {
        const sftvec d = sft_v_sub(verts[k], centroid);
        const f32 a[3] = {d.x, d.y, d.z};
        for (i32 i = 0; i < 3; i += 1) {
            for (i32 j = 0; j < 3; j += 1) {
                cov[i][j] += a[i] * a[j];
            }
        }
    }
    for (i32 i = 0; i < 3; i += 1) {
        for (i32 j = 0; j < 3; j += 1) {
            cov[i][j] /= count;
        }
    }

    sftvec axis = sft_v_norm(SFT_V(1.f, 1.f, 1.f));
    for (i32 i = 0; i < 10; i += 1) {
        const sftvec next = SFT_V(
            cov[0][0]*axis.x + cov[0][1]*axis.y + cov[0][2]*axis.z,
            cov[1][0]*axis.x + cov[1][1]*axis.y + cov[1][2]*axis.z,
            cov[2][0]*axis.x + cov[2][1]*axis.y + cov[2][2]*axis.z);
        const f32 len = sft_v_length(next);
        if (len < SFT_EPSILON) {
            break;
        }

        axis = sft_v_mul(next, 1.f / len);
    }

    f32 min_t = 1e9f, max_t = -1e9f;
    for (i32 i = 0; i < count; i += 1) {
        const f32 t = sft_v_dot(sft_v_sub(verts[i], centroid), axis);
        if (t < min_t) min_t = t;
        if (t > max_t) max_t = t;
    }

    const sftvec raw_a = sft_v_add(centroid, sft_v_mul(axis, min_t));
    const sftvec raw_b = sft_v_add(centroid, sft_v_mul(axis, max_t));
    const sftvec ab = sft_v_sub(raw_b, raw_a);
    const f32 len2 = sft_v_length2(ab);
    
    // heuristic for radius to ignore extreme spikes but still respect clothing volume like ponchos
    f32 sum_r = 0.f, max_r = 0.f;
    for (i32 i = 0; i < count; i += 1) {
        const sftvec ap = sft_v_sub(verts[i], raw_a);
        const f32 t = (len2 < SFT_EPSILON) ? 0.f : SFT_CLAMP(sft_v_dot(ap, ab) / len2, 0.f, 1.f);
        const sftvec closest = sft_v_add(raw_a, sft_v_mul(ab, t));
        const f32 r = sqrtf(sft_v_length2(sft_v_sub(verts[i], closest)));
        sum_r += r;

        if (r > max_r) {
            max_r = r;
        }
    }
    
    const f32 r = SFT_MIN((sum_r / count) * 1.8f, max_r * 0.95f);
    
    // extend capsule endpoints outward by a fraction of the radius to 'seal' joints
    const f32 joint_seal = r * 0.625f;
    f32 safe_min_t = min_t - joint_seal;
    f32 safe_max_t = max_t + joint_seal;
    
    if (safe_min_t > safe_max_t) {
        const f32 mid = (min_t + max_t) * 0.5f;
        safe_min_t = safe_max_t = mid;
    }

    out_proxy->posa = sft_v_add(centroid, sft_v_mul(axis, safe_min_t));
    out_proxy->posb = sft_v_add(centroid, sft_v_mul(axis, safe_max_t));
    out_proxy->radius = r; 
}

// fallback BVH gathering (for static meshes)
static void sft__gather_bvh_verts(const sft_model* model, u32 node_ind, sftvec** out_verts, i32* capacity, i32* count) {
    const struct sft_bvh_node* node = &model->blas_nodes[node_ind];
    if (!node->is_leaf_group) {
        for (u32 c = 0; c < node->child_count; c += 1) {
            if (0xFFFFFFFF != node->child_indices[c]) {
                sft__gather_bvh_verts(model, node->child_indices[c], out_verts, capacity, count);
            }
        }
        return;
    }

    for (u32 c = 0; c < node->child_count; c += 1) {
        if (0xFFFFFFFF == node->child_indices[c]) {
            continue;
        }

        const u32 payload = node->child_indices[c] & ~0x80000000;
        const u32 m_ind = payload >> SFT_TRIANGLE_BITS;
        const u32 local_tri = payload & ((1 << SFT_TRIANGLE_BITS) - 1);

        const u16 mesh_ind = model->meshlet_mesh_inds[m_ind];
        const sft_mesh* mesh = &model->meshes[mesh_ind];
        const u32 local_m = m_ind - mesh->model_meshlet_offset;

        const struct sft_meshlet* m_verts = &mesh->meshlets[local_m];
        const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m];

        const u32 batch = local_tri / SFT_SIMD_WIDTH;
        const u32 lane = local_tri % SFT_SIMD_WIDTH;
        const u32 offset = batch * (SFT_SIMD_WIDTH * 4);

        const u8 i0 = m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
        const u8 i1 = m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
        const u8 i2 = m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];

        if (*count + 3 > *capacity) {
            *capacity = (0 == *capacity) ? 1024 : (*capacity * 2 + 3);
            *out_verts = (sftvec*)realloc(*out_verts, *capacity * sizeof(sftvec));
        }

        (*out_verts)[*count + 0] = SFT_V(m_verts->x[i0], m_verts->y[i0], m_verts->z[i0]);
        (*out_verts)[*count + 1] = SFT_V(m_verts->x[i1], m_verts->y[i1], m_verts->z[i1]);
        (*out_verts)[*count + 2] = SFT_V(m_verts->x[i2], m_verts->y[i2], m_verts->z[i2]);
        *count += 3;
    }
}

static void sft__fit_capsule_to_node(const sft_model* model, u32 node_ind, struct sft_shadow_proxy* out_proxy) {
    sftvec* verts = NULL;
    i32 capacity = 0, count = 0;
    sft__gather_bvh_verts(model, node_ind, &verts, &capacity, &count);
    
    sft__fit_capsule_to_verts(verts, count, out_proxy);
    free(verts);
    
    out_proxy->bone_ind = 0xFFFFFFFF; // mark as unbound
}

static void sft__generate_bvh_proxies(sft_model* model, sft_arena* arena, i32 budget) {
    u32* active_nodes = (u32*)malloc(budget * sizeof(u32));
    i32 node_count = 1;
    active_nodes[0] = model->blas_root_ind;

    while (node_count < budget) {
        i32 largest_ind = -1;
        f32 largest_vol = -1.f;

        for (i32 i = 0; i < node_count; i += 1) {
            const struct sft_bvh_node* n = &model->blas_nodes[active_nodes[i]];
            if (n->is_leaf_group) {
                continue;
            }

            f32 min_x = 1e30f, min_y = 1e30f, min_z = 1e30f;
            f32 max_x = -1e30f, max_y = -1e30f, max_z = -1e30f;
            for (u32 c = 0; c < n->child_count; c += 1) {
                if (n->min_x[c] < min_x) min_x = n->min_x[c];
                if (n->min_y[c] < min_y) min_y = n->min_y[c];
                if (n->min_z[c] < min_z) min_z = n->min_z[c];
                if (n->max_x[c] > max_x) max_x = n->max_x[c];
                if (n->max_y[c] > max_y) max_y = n->max_y[c];
                if (n->max_z[c] > max_z) max_z = n->max_z[c];
            }

            const f32 vol = (max_x - min_x) * (max_y - min_y) * (max_z - min_z);
            if (vol > largest_vol) {
                largest_vol = vol;
                largest_ind = i;
            }
        }

        if (-1 == largest_ind) {
            break;
        }

        const struct sft_bvh_node* split_node = &model->blas_nodes[active_nodes[largest_ind]];
        active_nodes[largest_ind] = split_node->child_indices[0];
        
        for (u32 c = 1; c < split_node->child_count && node_count < budget; c += 1) {
            if (0xFFFFFFFF != split_node->child_indices[c]) {
                active_nodes[node_count] = split_node->child_indices[c];
                node_count += 1;
            }
        }
    }

    model->shadow_proxies = (struct sft_shadow_proxy*)sft_arena_alloc(arena, node_count * sizeof(struct sft_shadow_proxy));
    model->shadow_proxy_count = node_count;

    for (i32 i = 0; i < node_count; i += 1) {
        sft__fit_capsule_to_node(model, active_nodes[i], &model->shadow_proxies[i]);
    }
    
    free(active_nodes);
}

static u16 sft__resolve_cluster(const u16* map, u16 bone) {
    u16 curr = bone;
    while (map[curr] != curr) {
        curr = map[curr]; 
    }
    return curr;
}

static void sft__generate_bone_proxies(sft_model* model, sft_arena* arena, i32 budget) {
    const i32 bone_count = model->skeleton.bone_count;
    u16* cluster_map = (u16*)malloc(bone_count * sizeof(u16));
    
    struct cluster_data* clusters = (struct cluster_data*)calloc(bone_count, sizeof(struct cluster_data));
    for (i32 i = 0; i < bone_count; i += 1) {
        cluster_map[i] = (u16)i;
    }

    // ---  triangle area evaluation  ---
    for (u32 m = 0; m < model->mesh_count; m += 1) {
        const sft_mesh* mesh = &model->meshes[m];
        if (!mesh->meshlet_skins) {
            continue;
        }

        for (u32 ml = 0; ml < mesh->meshlet_count; ml += 1) {
            const struct sft_meshlet_skin* skin = &mesh->meshlet_skins[ml];
            const struct sft_meshlet* verts = &mesh->meshlets[ml];
            const struct sft_meshlet_inds* inds = &mesh->inds[ml];

            const u32 batch_count = 128 / SFT_SIMD_WIDTH;
            for (u32 batch = 0; batch < batch_count; batch += 1) {
                const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
                for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                    const u8 i0 = inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
                    const u8 i1 = inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
                    const u8 i2 = inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];
                    
                    if (i0 == i1 && i1 == i2) {
                        continue; // skip padding
                    }

                    // determine the dominant bone for this specific triangle
                    i32 best_bone = 0;
                    f32 max_weight = -1.f;
                    const u8 tri_inds[3] = {i0, i1, i2};
                    
                    for(i32 v = 0; v < 3; v += 1) {
                        u8 ind = tri_inds[v];
                        if (skin->w0[ind] > max_weight) {
                            max_weight = skin->w0[ind];
                            best_bone = skin->j0[ind];
                        }
                        if (skin->w1[ind] > max_weight) {
                            max_weight = skin->w1[ind];
                            best_bone = skin->j1[ind];
                        }
                        if (skin->w2[ind] > max_weight) {
                            max_weight = skin->w2[ind];
                            best_bone = skin->j2[ind];
                        }
                        if (skin->w3[ind] > max_weight) {
                            max_weight = skin->w3[ind];
                            best_bone = skin->j3[ind];
                        }
                    }

                    if (best_bone < bone_count) {
                        const sftvec p0 = SFT_V(verts->x[i0], verts->y[i0], verts->z[i0]);
                        const sftvec p1 = SFT_V(verts->x[i1], verts->y[i1], verts->z[i1]);
                        const sftvec p2 = SFT_V(verts->x[i2], verts->y[i2], verts->z[i2]);
                        
                        const sftvec edge1 = sft_v_sub(p1, p0);
                        const sftvec edge2 = sft_v_sub(p2, p0);
                        const f32 area = sft_v_length(sft_v_cross(edge1, edge2)) * 0.5f;

                        const sftvec center = sft_v_mul(sft_v_add(p0, sft_v_add(p1, p2)), 0.333333f);
                        
                        clusters[best_bone].area += area;
                        clusters[best_bone].centroid_sum = sft_v_add(clusters[best_bone].centroid_sum, center);
                        clusters[best_bone].tri_count += 1;
                    }
                }
            }
        }
    }

    i32 active_clusters = 0;
    for (i32 i = 0; i < bone_count; i += 1) {
        if (clusters[i].tri_count > 0) active_clusters += 1;
    }

    // ---  hierarchical area based merging  ---
    while (active_clusters > budget && active_clusters > 1) {
        f32 min_area = 1e30f;
        i32 smallest_bone = -1;
        
        for (i32 i = 0; i < bone_count; i += 1) {
            if (clusters[i].tri_count > 0 && clusters[i].area < min_area) {
                min_area = clusters[i].area;
                smallest_bone = (i32)i;
            }
        }
        if (-1 == smallest_bone) {
            break;
        }

        i32 target_bone = -1;
        const i32 parent = model->skeleton.bones[smallest_bone].parent_ind;
        if (parent != -1) {
            target_bone = sft__resolve_cluster(cluster_map, (u16)parent);
        }
        else {
            f32 max_area = -1.f;
            for (i32 i = 0; i < bone_count; i += 1) {
                if ((i32)i != smallest_bone && clusters[i].area > max_area) {
                    max_area = clusters[i].area;
                    target_bone = (i32)i;
                }
            }
        }

        if (-1 == target_bone || target_bone == smallest_bone) {
            break;
        }

        cluster_map[smallest_bone] = (u16)target_bone;
        clusters[target_bone].area += clusters[smallest_bone].area;
        clusters[target_bone].centroid_sum = sft_v_add(clusters[target_bone].centroid_sum, clusters[smallest_bone].centroid_sum);
        clusters[target_bone].tri_count += clusters[smallest_bone].tri_count;
        
        clusters[smallest_bone].tri_count = 0; 
        active_clusters -= 1;
    }

    // ---  joint anchored capsule fitting  ---
    model->shadow_proxies = (struct sft_shadow_proxy*)sft_arena_alloc(arena, active_clusters * sizeof(struct sft_shadow_proxy));
    model->shadow_proxy_count = active_clusters;

    // temporary extraction of verts for the final radius/length pass
    sftvec** cluster_verts = (sftvec**)calloc(bone_count, sizeof(sftvec*));
    i32* cluster_capacities = (i32*)calloc(bone_count, sizeof(i32));
    i32* cluster_write_heads = (i32*)calloc(bone_count, sizeof(i32));

    for (u32 m = 0; m < model->mesh_count; m += 1) {
        const sft_mesh* mesh = &model->meshes[m];
        if (!mesh->meshlet_skins) {
            continue;
        }

        for (u32 ml = 0; ml < mesh->meshlet_count; ml += 1) {
            const struct sft_meshlet_skin* skin = &mesh->meshlet_skins[ml];
            const struct sft_meshlet* verts = &mesh->meshlets[ml];
            
            for (u32 v = 0; v < SFT_VERTS_PER_MESHLET; v += 1) {
                u8 max_w = skin->w0[v];
                u16 best_bone = skin->j0[v];
                if (skin->w1[v] > max_w) {
                    max_w = skin->w1[v];
                    best_bone = skin->j1[v];
                }
                if (skin->w2[v] > max_w) {
                    max_w = skin->w2[v];
                    best_bone = skin->j2[v];
                }
                if (skin->w3[v] > max_w) {
                    max_w = skin->w3[v];
                    best_bone = skin->j3[v];
                }

                if (max_w > 0 && best_bone < bone_count) {
                    const u16 final_bone = sft__resolve_cluster(cluster_map, best_bone);
                    if (cluster_write_heads[final_bone] >= cluster_capacities[final_bone]) {
                        cluster_capacities[final_bone] = (0 == cluster_capacities[final_bone]) ? 1024 : cluster_capacities[final_bone] * 2;
                        cluster_verts[final_bone] = (sftvec*)realloc(cluster_verts[final_bone], cluster_capacities[final_bone] * sizeof(sftvec));
                    }
                    cluster_verts[final_bone][cluster_write_heads[final_bone]++] = SFT_V(verts->x[v], verts->y[v], verts->z[v]);
                }
            }
        }
    }

    i32 proxy_ind = 0;
    for (i32 i = 0; i < bone_count; i += 1) {
        if (clusters[i].tri_count > 0 && cluster_write_heads[i] > 0) {
            const sftvec centroid = sft_v_mul(clusters[i].centroid_sum, 1.f / clusters[i].tri_count);
            const sftvec anchor = model->skeleton.bones[i].bind_pos;
            
            // the capsule's line segment is locked from the joint toward the geometric centroid
            sftvec axis = sft_v_sub(centroid, anchor);
            const f32 axis_len = sft_v_length(axis);
            if (axis_len > SFT_EPSILON) {
                axis = sft_v_mul(axis, 1.f / axis_len);
            }
            else {
                axis = SFT_V(0.f, 1.f, 0.f);
            }

            f32 min_t = 1e9f, max_t = -1e9f;
            f32 sum_r = 0.f, max_r = 0.f;

            for (i32 v = 0; v < cluster_write_heads[i]; v += 1) {
                const sftvec diff = sft_v_sub(cluster_verts[i][v], anchor);
                f32 t = sft_v_dot(diff, axis);
                if (t < min_t) min_t = t;
                if (t > max_t) max_t = t;
                
                const sftvec closest = sft_v_add(anchor, sft_v_mul(axis, t));
                const f32 r = sqrtf(sft_v_length2(sft_v_sub(cluster_verts[i][v], closest)));
                sum_r += r;
                if (r > max_r) {
                    max_r = r;
                }
            }

            const f32 r = SFT_MIN((sum_r / cluster_write_heads[i]) * 2.2f, max_r * 0.9f);
            
            // slightly expand the bounds to ensure a seal at the joints
            f32 safe_min_t = SFT_MIN(0.f, min_t - (r * 0.25f)); 
            f32 safe_max_t = SFT_MAX(0.f, max_t + (r * 0.25f));

            if (safe_min_t > safe_max_t) {
                const f32 mid = (safe_min_t + safe_max_t) * 0.5f;
                safe_min_t = safe_max_t = mid;
            }

            model->shadow_proxies[proxy_ind].posa = sft_v_add(anchor, sft_v_mul(axis, safe_min_t));
            model->shadow_proxies[proxy_ind].posb = sft_v_add(anchor, sft_v_mul(axis, safe_max_t));
            model->shadow_proxies[proxy_ind].radius = r;
            model->shadow_proxies[proxy_ind].bone_ind = i;
            proxy_ind += 1;
        }

        if (cluster_verts[i]) {
            free(cluster_verts[i]);
        }
    }

    free(cluster_verts);
    free(cluster_capacities);
    free(cluster_write_heads);
    free(cluster_map);
    free(clusters);
}

static void sft__generate_shadow_proxies(sft_model* model, sft_arena* arena, i32 budget) {
    if (budget <= 0 || model->blas_node_count == 0) {
        model->shadow_proxies = NULL;
        model->shadow_proxy_count = 0;
        return;
    }

    if (model->skeleton.bone_count > 0) {
        sft__generate_bone_proxies(model, arena, budget);
    }
    else {
        sft__generate_bvh_proxies(model, arena, budget);
    }
}

static void sft__blas_build_model(sft_model* model, sft_arena* arena) {
    if (!model || 0 == model->total_meshlets) {
        return;
    }

    // ---  count total valid triangles  ---
    u32 total_tris = 0;
    for (u32 gi = 0; gi < model->total_meshlets; gi += 1) {
        const u16 m_ind = model->meshlet_mesh_inds[gi];
        const sft_mesh* mesh = &model->meshes[m_ind];
        const u32 local_m_ind = gi - mesh->model_meshlet_offset;
        const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];

        const u32 batch_count = 128 / SFT_SIMD_WIDTH;
        for (u32 batch = 0; batch < batch_count; batch += 1) {
            const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
            for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                const u8 i0 = m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
                const u8 i1 = m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
                const u8 i2 = m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];
                
                // skip padding
                if (i0 == i1 && i1 == i2) {
                    continue; 
                }
                total_tris += 1;
            }
        }
    }

    if (0 == total_tris) {
        return;
    }

    // ---  extract and build AABBs  ---
    struct sft_blas_build_item* build_items = (struct sft_blas_build_item*)
        malloc(total_tris * sizeof(struct sft_blas_build_item));

    u32 tri_ind = 0;
    for (u32 gi = 0; gi < model->total_meshlets; gi += 1) {
        const u16 m_ind = model->meshlet_mesh_inds[gi];
        const sft_mesh* mesh = &model->meshes[m_ind];
        const u32 local_m_ind = gi - mesh->model_meshlet_offset;
        const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];
        const struct sft_meshlet* m_verts = &mesh->meshlets[local_m_ind];

        const u32 batch_count = 128 / SFT_SIMD_WIDTH;
        for (u32 batch = 0; batch < batch_count; batch += 1) {
            const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
            for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                const u8 i0 = m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
                const u8 i1 = m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
                const u8 i2 = m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];
                
                if (i0 == i1 && i1 == i2) {
                    continue;
                }

                const sftvec v0 = SFT_V(m_verts->x[i0], m_verts->y[i0], m_verts->z[i0]);
                const sftvec v1 = SFT_V(m_verts->x[i1], m_verts->y[i1], m_verts->z[i1]);
                const sftvec v2 = SFT_V(m_verts->x[i2], m_verts->y[i2], m_verts->z[i2]);

                struct sft_aabb aabb;
                aabb.min.x = SFT_MIN(v0.x, SFT_MIN(v1.x, v2.x));
                aabb.min.y = SFT_MIN(v0.y, SFT_MIN(v1.y, v2.y));
                aabb.min.z = SFT_MIN(v0.z, SFT_MIN(v1.z, v2.z));
                aabb.max.x = SFT_MAX(v0.x, SFT_MAX(v1.x, v2.x));
                aabb.max.y = SFT_MAX(v0.y, SFT_MAX(v1.y, v2.y));
                aabb.max.z = SFT_MAX(v0.z, SFT_MAX(v1.z, v2.z));

                const u32 local_tri = (batch * SFT_SIMD_WIDTH) + lane;
                build_items[tri_ind].global_meshlet_ind = (gi << SFT_TRIANGLE_BITS) | local_tri;
                build_items[tri_ind].aabb = aabb;
                build_items[tri_ind].centroid = sft_v_mul(sft_v_add(aabb.min, aabb.max), 0.5f);
                
                tri_ind += 1;
            }
        }
    }

    // ---  build tree  ---
    const u32 max_blas_nodes = (total_tris * 2) + 1;
    model->blas_nodes = (struct sft_bvh_node*)sft_arena_alloc(arena, max_blas_nodes * sizeof(struct sft_bvh_node));
    model->blas_node_count = 0;

    struct sft_aabb model_root_aabb;
    model->blas_root_ind = sft__bvh_build_recursive(
        build_items, total_tris, 
        model->blas_nodes, &model->blas_node_count, &model_root_aabb);

    free(build_items);
}

static void sft__tlas_build(void) {
    const u32 inst_count = sft_state->cmd_queue.instance_count.count;
    if (0 == inst_count) {
        return;
    }

    struct sft_blas_build_item* items = (struct sft_blas_build_item*)
        sft_arena_alloc(&sft_state->scratch_arena, inst_count * sizeof(struct sft_blas_build_item));
    u32 valid_inst_count = 0;

    // transform model root AABBs to world space
    for (u32 i = 0; i < inst_count; i += 1) {
        const sft_instance* inst = &sft_state->cmd_queue.instances[i];
        const sft_model* model = inst->model;
        
        if (0 == model->blas_node_count) {
            continue;
        }
        
        // extract root AABB from the model's BLAS
        const struct sft_bvh_node* root_blas = &model->blas_nodes[model->blas_root_ind];
        f32 min_x = 1e30f, min_y = 1e30f, min_z = 1e30f;
        f32 max_x = -1e30f, max_y = -1e30f, max_z = -1e30f;
        
        for (u32 c = 0; c < root_blas->child_count; c += 1) {
            if (root_blas->min_x[c] < min_x) min_x = root_blas->min_x[c];
            if (root_blas->min_y[c] < min_y) min_y = root_blas->min_y[c];
            if (root_blas->min_z[c] < min_z) min_z = root_blas->min_z[c];
            if (root_blas->max_x[c] > max_x) max_x = root_blas->max_x[c];
            if (root_blas->max_y[c] > max_y) max_y = root_blas->max_y[c];
            if (root_blas->max_z[c] > max_z) max_z = root_blas->max_z[c];
        }

        // 8 corners of the AABB
        sftvec corners[8] = {
            SFT_V(min_x, min_y, min_z), SFT_V(max_x, min_y, min_z),
            SFT_V(min_x, max_y, min_z), SFT_V(max_x, max_y, min_z),
            SFT_V(min_x, min_y, max_z), SFT_V(max_x, min_y, max_z),
            SFT_V(min_x, max_y, max_z), SFT_V(max_x, max_y, max_z)
        };

        struct sft_aabb w_aabb = { .min = {1e30f, 1e30f, 1e30f}, .max = {-1e30f, -1e30f, -1e30f} };
        
        for (i32 k = 0; k < 8; k += 1) {
            // apply instance transform
            const f32 tx =
                corners[k].x * inst->transform.m[0][0] +
                corners[k].y * inst->transform.m[1][0] +
                corners[k].z * inst->transform.m[2][0] +
                               inst->transform.m[3][0];
            const f32 ty =
                corners[k].x * inst->transform.m[0][1] +
                corners[k].y * inst->transform.m[1][1] +
                corners[k].z * inst->transform.m[2][1] +
                               inst->transform.m[3][1];
            const f32 tz =
                corners[k].x * inst->transform.m[0][2] +
                corners[k].y * inst->transform.m[1][2] +
                corners[k].z * inst->transform.m[2][2] +
                               inst->transform.m[3][2];
            
            if (tx < w_aabb.min.x) w_aabb.min.x = tx;
            if (ty < w_aabb.min.y) w_aabb.min.y = ty;
            if (tz < w_aabb.min.z) w_aabb.min.z = tz;
            if (tx > w_aabb.max.x) w_aabb.max.x = tx;
            if (ty > w_aabb.max.y) w_aabb.max.y = ty;
            if (tz > w_aabb.max.z) w_aabb.max.z = tz;
        }

        items[i].global_meshlet_ind = i; // overloaded to mean inst_ind for TLAS
        items[i].aabb = w_aabb;
        items[i].centroid = sft_v_mul(sft_v_add(w_aabb.min, w_aabb.max), 0.5f);
        valid_inst_count += 1;
    }

    const u32 max_tlas_nodes = (valid_inst_count * 2) + 1;
    sft_state->cmd_queue.tlas_nodes = (struct sft_bvh_node*)
        sft_arena_alloc(&sft_state->scratch_arena, max_tlas_nodes * sizeof(struct sft_bvh_node));
    sft_state->cmd_queue.tlas_node_count = 0;

    sft_state->cmd_queue.tlas_root_ind = sft__bvh_build_recursive(
        items, valid_inst_count,
        sft_state->cmd_queue.tlas_nodes, 
        &sft_state->cmd_queue.tlas_node_count, NULL);
}

static u8 sft__intersect_triangle(sftvec ro, sftvec rd, sftvec v0, sftvec v1, sftvec v2, f32* t, f32* u, f32* v) {
    const sftvec v0v1 = sft_v_sub(v1, v0);
    const sftvec v0v2 = sft_v_sub(v2, v0);
    const sftvec pvec = sft_v_cross(rd, v0v2);

    const f32 det = sft_v_dot(v0v1, pvec);
    if (fabsf(det) < SFT_EPSILON) {
        return 0;
    }
    
    const f32 inv_det = 1.f / det;

    const sftvec tvec = sft_v_sub(ro, v0);
    *u = sft_v_dot(tvec, pvec) * inv_det;
    if (*u < 0.f || *u > 1.f) {
        return 0;
    }

    const sftvec qvec = sft_v_cross(tvec, v0v1);
    *v = sft_v_dot(rd, qvec) * inv_det;
    if (*v < 0.f || *u + *v > 1.f) {
        return 0;
    }

    *t = sft_v_dot(v0v2, qvec) * inv_det;
    return (*t > SFT_EPSILON);
}

SFT_FUNC sft_ray_hit sft_raycast_instance(const sft_instance* inst, sftvec origin, sftvec dir, f32 max_dist) {
    sft_ray_hit hit = {0};
    hit.distance = max_dist;
    if (!inst || !inst->model || 0 == inst->model->blas_node_count) {
        return hit;
    }

    // transform ray to model space
    const sftmat inv_m = sft_m_qinv(inst->transform);
    const sftvec local_orig = sft_m_mul_vec(inv_m, origin);
    
    // transform direction
    sftvec local_dir;
    local_dir.x = dir.x*inv_m.m[0][0] + dir.y*inv_m.m[1][0] + dir.z*inv_m.m[2][0];
    local_dir.y = dir.x*inv_m.m[0][1] + dir.y*inv_m.m[1][1] + dir.z*inv_m.m[2][1];
    local_dir.z = dir.x*inv_m.m[0][2] + dir.y*inv_m.m[1][2] + dir.z*inv_m.m[2][2];
    
    // extract scale to adjust distance
    const f32 scale_sq = local_dir.x*local_dir.x + local_dir.y*local_dir.y + local_dir.z*local_dir.z;
    const f32 inv_scale = 1.f / sqrtf(scale_sq);
    local_dir = sft_v_mul(local_dir, inv_scale);

    const sftvec inv_dir = SFT_V(
        1.f / (fabsf(local_dir.x) > SFT_EPSILON ? local_dir.x : SFT_EPSILON * (local_dir.x < 0 ? -1 : 1)),
        1.f / (fabsf(local_dir.y) > SFT_EPSILON ? local_dir.y : SFT_EPSILON * (local_dir.y < 0 ? -1 : 1)),
        1.f / (fabsf(local_dir.z) > SFT_EPSILON ? local_dir.z : SFT_EPSILON * (local_dir.z < 0 ? -1 : 1))
    );

    u32 stack[256];
    i32 stack_ptr = 0;
    stack[stack_ptr++] = inst->model->blas_root_ind;

    f32 local_max_dist = max_dist / inv_scale;

    while (stack_ptr > 0) {
        const u32 node_ind = stack[--stack_ptr];
        const struct sft_bvh_node* node = &inst->model->blas_nodes[node_ind];

        for (u32 c = 0; c < node->child_count; c += 1) {
            if (0xFFFFFFFF == node->child_indices[c]) {
                continue;
            }

            const f32 tx1 = (node->min_x[c] - local_orig.x) * inv_dir.x;
            const f32 tx2 = (node->max_x[c] - local_orig.x) * inv_dir.x;
            const f32 ty1 = (node->min_y[c] - local_orig.y) * inv_dir.y;
            const f32 ty2 = (node->max_y[c] - local_orig.y) * inv_dir.y;
            const f32 tz1 = (node->min_z[c] - local_orig.z) * inv_dir.z;
            const f32 tz2 = (node->max_z[c] - local_orig.z) * inv_dir.z;

            const f32 tmin = SFT_MAX(SFT_MAX(SFT_MIN(tx1, tx2), SFT_MIN(ty1, ty2)), SFT_MIN(tz1, tz2));
            const f32 tmax = SFT_MIN(SFT_MIN(SFT_MAX(tx1, tx2), SFT_MAX(ty1, ty2)), SFT_MAX(tz1, tz2));

            if (tmax >= tmin && tmax > 0.f && tmin < local_max_dist) {
                if (!node->is_leaf_group) {
                    stack[stack_ptr++] = node->child_indices[c];
                    continue;
                }

                const u32 payload = node->child_indices[c] & ~0x80000000;
                const u32 m_ind = payload >> SFT_TRIANGLE_BITS;
                const u32 local_tri = payload & ((1 << SFT_TRIANGLE_BITS) - 1);
                
                const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
                const sft_mesh* mesh = &inst->model->meshes[mesh_ind];
                const u32 local_m_ind = m_ind - mesh->model_meshlet_offset;
                const struct sft_meshlet* m_verts = &mesh->meshlets[local_m_ind];
                const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];
                
                const u32 batch = local_tri / SFT_SIMD_WIDTH;
                const u32 lane = local_tri % SFT_SIMD_WIDTH;
                const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
                
                const u8 i0 = m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
                const u8 i1 = m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
                const u8 i2 = m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];
                
                const sftvec v0 = SFT_V(m_verts->x[i0], m_verts->y[i0], m_verts->z[i0]);
                const sftvec v1 = SFT_V(m_verts->x[i1], m_verts->y[i1], m_verts->z[i1]);
                const sftvec v2 = SFT_V(m_verts->x[i2], m_verts->y[i2], m_verts->z[i2]);
                
                f32 t, u, v;
                if (sft__intersect_triangle(local_orig, local_dir, v0, v1, v2, &t, &u, &v) && t < local_max_dist) {
                    local_max_dist = t;
                    hit.hit = 1;
                    hit.distance = t * inv_scale; // convert distance back to world scale
                    
                    // reconstruct local hit position and normal
                    const sftvec local_hit = sft_v_add(local_orig, sft_v_mul(local_dir, t));
                    const sftvec local_norm = sft_v_face_normal(v0, v1, v2);
                    
                    // transform hit data back to world space
                    hit.pos = sft_m_mul_vec(inst->transform, local_hit);
                    
                    // transform normal using 3x3 rotation
                    hit.normal.x = local_norm.x*inst->transform.m[0][0] + local_norm.y*inst->transform.m[1][0] + local_norm.z*inst->transform.m[2][0];
                    hit.normal.y = local_norm.x*inst->transform.m[0][1] + local_norm.y*inst->transform.m[1][1] + local_norm.z*inst->transform.m[2][1];
                    hit.normal.z = local_norm.x*inst->transform.m[0][2] + local_norm.y*inst->transform.m[1][2] + local_norm.z*inst->transform.m[2][2];
                    hit.normal = sft_v_norm(hit.normal);
                }
            }
        }
    }

    return hit;
}

static SFT_INLINE vf32 sft__ray_packet_vs_triangle_any_hit(
    sftvec v0, sftvec v1, sftvec v2,
    sftvvec ray_origin, sftvvec ray_dir,
    vf32 t_max, vf32 active_mask
) {
    // scalar edge precomputation
    const sftvec edge1_scalar = sft_v_sub(v1, v0);
    const sftvec edge2_scalar = sft_v_sub(v2, v0);

    // broadcast the scalar edges and origin to SIMD lanes
    const sftvvec e1 = { sft_vf_set1(edge1_scalar.x), sft_vf_set1(edge1_scalar.y), sft_vf_set1(edge1_scalar.z) };
    const sftvvec e2 = { sft_vf_set1(edge2_scalar.x), sft_vf_set1(edge2_scalar.y), sft_vf_set1(edge2_scalar.z) };
    const sftvvec v0_simd = { sft_vf_set1(v0.x), sft_vf_set1(v0.y), sft_vf_set1(v0.z) };

    // ray triangle cross products and determinant
    const sftvvec h = sft_vv_cross(ray_dir, e2);
    const vf32 a = sft_vv_dot(e1, h);

    // parallel ray check (two-sided intersection)
    const vf32 v_eps = sft_vf_set1(SFT_EPSILON);
    const vi32 abs_mask = sft_vi_set1(0x7FFFFFFF);
    
    // cast to int, strip sign bit, cast back to float
    const vf32 abs_a = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(a), abs_mask));
    const vf32 valid_a = sft_vf_cmp_gt(abs_a, v_eps);
    
    // mask tracking, cull lanes that are parallel or were already inactive
    vf32 mask = sft_vf_and(valid_a, active_mask);
    
    // divergence early out, if all lanes in the packet failed
    if (0 == sft_vf_movemask(mask)) {
        return sft_vf_setzero();
    }

    const vf32 f = sft_vf_rcp_refined(a);
    const sftvvec s = sft_vv_sub(ray_origin, v0_simd);

    // barycentric U parameter
    const vf32 u = sft_vf_mul(f, sft_vv_dot(s, h));
    const vf32 v_zero = sft_vf_setzero();
    const vf32 v_one = sft_vf_set1(1.f);
    
    mask = sft_vf_and(mask, sft_vf_and(sft_vf_cmp_ge(u, v_zero), sft_vf_cmp_le(u, v_one)));
    if (0 == sft_vf_movemask(mask)) {
        return sft_vf_setzero();
    }

    // barycentric V parameter
    const sftvvec q = sft_vv_cross(s, e1);
    const vf32 v = sft_vf_mul(f, sft_vv_dot(ray_dir, q));
    
    mask = sft_vf_and(mask, sft_vf_and(sft_vf_cmp_ge(v, v_zero), sft_vf_cmp_le(sft_vf_add(u, v), v_one)));
    if (0 == sft_vf_movemask(mask)) {
        return sft_vf_setzero();
    }

    // T parameter (distance)
    const vf32 t = sft_vf_mul(f, sft_vv_dot(e2, q));
    
    // intersection must strictly occur before the light source and past the self intersection bias
    const vf32 v_t_min = sft_vf_set1(SFT_EPSILON); // shadow bias
    mask = sft_vf_and(mask, sft_vf_and(sft_vf_cmp_gt(t, v_t_min), sft_vf_cmp_lt(t, t_max)));

    return mask; // 0xFFFFFFFF in lanes that hit, 0x0 in lanes that missed
}

static SFT_INLINE vf32 sft__frustum_vs_bvh_node(
    const struct sft_bvh_node* node,
    const struct sft_frustum* frustum
) {
    const vf32 v_zero = sft_vf_setzero();
    
    // initialize mask to all 1s without relying on signed integer overflow
    vf32 inside_mask = sft_vf_cmp_eq(v_zero, v_zero);

    // load the 8 AABBs from the node
    const vf32 min_x = sft_vf_load(node->min_x);
    const vf32 min_y = sft_vf_load(node->min_y);
    const vf32 min_z = sft_vf_load(node->min_z);
    
    const vf32 max_x = sft_vf_load(node->max_x);
    const vf32 max_y = sft_vf_load(node->max_y);
    const vf32 max_z = sft_vf_load(node->max_z);

    // convert AABB (min, max) to center (cx, cy, cz) and extents (ex, ey, ez)
    const vf32 v_half = sft_vf_set1(0.5f);
    
    const vf32 cx = sft_vf_mul(sft_vf_add(max_x, min_x), v_half);
    const vf32 cy = sft_vf_mul(sft_vf_add(max_y, min_y), v_half);
    const vf32 cz = sft_vf_mul(sft_vf_add(max_z, min_z), v_half);
    
    const vf32 ex = sft_vf_mul(sft_vf_sub(max_x, min_x), v_half);
    const vf32 ey = sft_vf_mul(sft_vf_sub(max_y, min_y), v_half);
    const vf32 ez = sft_vf_mul(sft_vf_sub(max_z, min_z), v_half);

    // test against all 6 planes of the frustum
    for (i32 p = 0; p < 6; p += 1) {
        // plane normal and distance
        const vf32 nx = frustum->normal[p].x;
        const vf32 ny = frustum->normal[p].y;
        const vf32 nz = frustum->normal[p].z;
        const vf32 d  = frustum->d[p];

        // precomputed absolute normals
        const vf32 abs_nx = frustum->abs_normal[p].x;
        const vf32 abs_ny = frustum->abs_normal[p].y;
        const vf32 abs_nz = frustum->abs_normal[p].z;

        // effective projected radius of the AABB onto the plane normal
        const vf32 r = sft_vf_fmadd(abs_nx, ex, sft_vf_fmadd(abs_ny, ey, sft_vf_mul(abs_nz, ez)));

        // distance from the AABB center to the plane
        const vf32 dot_c = sft_vf_fmadd(nx, cx, sft_vf_fmadd(ny, cy, sft_vf_mul(nz, cz)));
        const vf32 dist = sft_vf_add(dot_c, d);

        // if (dist + r >= 0), the AABB is partially or completely inside
        const vf32 plane_mask = sft_vf_cmp_ge(sft_vf_add(dist, r), v_zero);
        
        // accumulate intersections
        inside_mask = sft_vf_and(inside_mask, plane_mask);
        
        // early out if all 8 AABBs have been entirely culled by a plane
        if (0 == sft_vf_movemask(inside_mask)) {
            return v_zero;
        }
    }

    return inside_mask;
}

static SFT_INLINE void sft__build_packet_frustum(
    sftvvec ray_orig, sftvec light_pos, f32 light_radius, vf32 active_mask,
    struct sft_frustum* out_frustum
) {
    // extract scalar bounds of the 8 ray origins
    alignas(32) f32 sx[8], sy[8], sz[8];
    sft_vf_store(sx, ray_orig.x);
    sft_vf_store(sy, ray_orig.y);
    sft_vf_store(sz, ray_orig.z);

    f32 min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f, min_z = 1e9f, max_z = -1e9f;
    sftvec center = {0};
    
    i32 active_lanes = sft_vf_movemask(active_mask);
    i32 valid_count = 0;
    
    for (i32 i = 0; i < 8; i += 1) {
        if (0 == (active_lanes & (1 << i))) {
            continue;
        }
        
        if (sx[i] < min_x) min_x = sx[i];
        if (sx[i] > max_x) max_x = sx[i];
        if (sy[i] < min_y) min_y = sy[i];
        if (sy[i] > max_y) max_y = sy[i];
        if (sz[i] < min_z) min_z = sz[i];
        if (sz[i] > max_z) max_z = sz[i];
        
        center.x += sx[i]; center.y += sy[i]; center.z += sz[i];
        valid_count += 1;
    }
    
    if (0 == valid_count) {
        return;
    }
    
    center.x /= (f32)valid_count; 
    center.y /= (f32)valid_count; 
    center.z /= (f32)valid_count;

    // form the 8 corners of the tight pixel cluster
    sftvec corners[8] = {
        SFT_V(min_x, min_y, min_z), SFT_V(max_x, min_y, min_z),
        SFT_V(min_x, max_y, min_z), SFT_V(max_x, max_y, min_z),
        SFT_V(min_x, min_y, max_z), SFT_V(max_x, min_y, max_z),
        SFT_V(min_x, max_y, max_z), SFT_V(max_x, max_y, max_z)
    };

    // look at the cluster from the light's perspective
    const sftvec l_forward = sft_v_norm(sft_v_sub(center, light_pos));
    const sftvec arbitrary_up = (fabsf(l_forward.y) > 0.99f) ? SFT_V(1, 0, 0) : SFT_V(0, 1, 0);
    const sftmat l_view = sft_m_view_look_at(light_pos, center, arbitrary_up);

    // project corners to light view space to find the bounding side planes
    f32 min_xz = 1e9f, max_xz = -1e9f, min_yz = 1e9f, max_yz = -1e9f;
    i32 is_invalid = 0;
    
    for (i32 i = 0; i < 8; i += 1) {
        const sftvec c_lv = sft_m_mul_quat(l_view, (sftquat){corners[i].x, corners[i].y, corners[i].z, 1.f});
        if (c_lv.z < SFT_EPSILON) {
            is_invalid = 1;
            break;
        }
        if (c_lv.x / c_lv.z < min_xz) min_xz = c_lv.x / c_lv.z;
        if (c_lv.x / c_lv.z > max_xz) max_xz = c_lv.x / c_lv.z;
        if (c_lv.y / c_lv.z < min_yz) min_yz = c_lv.y / c_lv.z;
        if (c_lv.y / c_lv.z > max_yz) max_yz = c_lv.y / c_lv.z;
    }

    const sftvec lr = SFT_V(l_view.m[0][0], l_view.m[1][0], l_view.m[2][0]);
    const sftvec lu = SFT_V(l_view.m[0][1], l_view.m[1][1], l_view.m[2][1]);
    const sftvec lf = SFT_V(l_view.m[0][2], l_view.m[1][2], l_view.m[2][2]);

    // assemble the planes
    if (is_invalid) {
        for (i32 p = 0; p < 4; p += 1) {
            out_frustum->normal[p].x = sft_vf_setzero(); // fallback
        }
    }
    else {
        const sftvec n_lv[4] = {
            sft_v_norm(SFT_V( 1.f,  0.f, -min_xz)), sft_v_norm(SFT_V(-1.f,  0.f,  max_xz)),
            sft_v_norm(SFT_V( 0.f,  1.f, -min_yz)), sft_v_norm(SFT_V( 0.f, -1.f,  max_yz))
        };
        for (i32 p = 0; p < 4; p += 1) {
            const sftvec n_world = sft_v_add(
                sft_v_mul(lr, n_lv[p].x), sft_v_add(sft_v_mul(lu, n_lv[p].y), sft_v_mul(lf, n_lv[p].z)));
            out_frustum->normal[p].x = sft_vf_set1(n_world.x);
            out_frustum->normal[p].y = sft_vf_set1(n_world.y);
            out_frustum->normal[p].z = sft_vf_set1(n_world.z);
            // push side planes outward by the light emitter size
            out_frustum->d[p] = sft_vf_set1(-sft_v_dot(n_world, light_pos) + SFT_SHADOW_LIGHT_SIZE);
            out_frustum->abs_normal[p].x = sft_vf_set1(fabsf(n_world.x));
            out_frustum->abs_normal[p].y = sft_vf_set1(fabsf(n_world.y));
            out_frustum->abs_normal[p].z = sft_vf_set1(fabsf(n_world.z));
        }
    }
    
    // set near/far caps
    out_frustum->normal[4].x = sft_vf_set1(lf.x);
    out_frustum->normal[4].y = sft_vf_set1(lf.y);
    out_frustum->normal[4].z = sft_vf_set1(lf.z);
    out_frustum->d[4]        = sft_vf_set1(-sft_v_dot(lf, light_pos) + SFT_SHADOW_LIGHT_SIZE);
    out_frustum->abs_normal[4].x = sft_vf_set1(fabsf(lf.x));
    out_frustum->abs_normal[4].y = sft_vf_set1(fabsf(lf.y));
    out_frustum->abs_normal[4].z = sft_vf_set1(fabsf(lf.z));
    
    out_frustum->normal[5].x = sft_vf_set1(-lf.x);
    out_frustum->normal[5].y = sft_vf_set1(-lf.y);
    out_frustum->normal[5].z = sft_vf_set1(-lf.z);
    out_frustum->d[5] = sft_vf_set1(-sft_v_dot(SFT_V(-lf.x, -lf.y, -lf.z), light_pos) + light_radius);
    out_frustum->abs_normal[5].x = sft_vf_set1(fabsf(lf.x));
    out_frustum->abs_normal[5].y = sft_vf_set1(fabsf(lf.y));
    out_frustum->abs_normal[5].z = sft_vf_set1(fabsf(lf.z));
}


static SFT_INLINE vf32 sft__trace_shadow_packet(
    sftvvec ray_orig, sftvvec L_raw, vf32 active_mask, 
    sftvec light_pos, f32 light_radius, u8 static_only
) {
    if (0 == sft_vf_movemask(active_mask)) {
        return sft_vf_setzero(); // no active rays to trace
    }

    vf32 saturation_mask = sft_vf_setzero();
    const i32 full_saturation = sft_vf_movemask(active_mask);

    struct sft_frustum packet_frustum;
    sft__build_packet_frustum(ray_orig, light_pos, light_radius, active_mask, &packet_frustum);

    // ---  TLAS traversal (world space)  ---
    u32 tlas_stack[64];
    i32 tlas_top = 0;
    tlas_stack[tlas_top++] = sft_state->cmd_queue.tlas_root_ind;

    while (tlas_top > 0) {
        const u32 tnode_ind = tlas_stack[--tlas_top];
        const struct sft_bvh_node* tnode = &sft_state->cmd_queue.tlas_nodes[tnode_ind];

        const vf32 thit = sft__frustum_vs_bvh_node(tnode, &packet_frustum);
        i32 thit_mask = sft_vf_movemask(thit);

        while (thit_mask) {
            const i32 tlane = sft__tzcnt(thit_mask);
            thit_mask &= (thit_mask - 1);

            if (0xFFFFFFFF == tnode->child_indices[tlane]) {
                continue;
            }

            if (!tnode->is_leaf_group) {
                tlas_stack[tlas_top++] = tnode->child_indices[tlane];
                continue;
            }

            // hit an instance leaf
            const u32 inst_ind = tnode->child_indices[tlane] & ~0x80000000;
            const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
            const sft_model* model = inst->model;

            if (static_only && model->skeleton.bone_count > 0) {
                continue;
            }
            if (0 == model->blas_node_count) {
                continue;
            }

            // transform world space packet_frustum to model space
            struct sft_frustum model_frustum;
            const sftmat* M = &inst->transform;

            for (i32 p = 0; p < 6; p += 1) {
                const f32 nx = ((f32*)&packet_frustum.normal[p].x)[0];
                const f32 ny = ((f32*)&packet_frustum.normal[p].y)[0];
                const f32 nz = ((f32*)&packet_frustum.normal[p].z)[0];
                const f32 d  = ((f32*)&packet_frustum.d[p])[0];

                const f32 nmx = nx * M->m[0][0] + ny * M->m[0][1] + nz * M->m[0][2];
                const f32 nmy = nx * M->m[1][0] + ny * M->m[1][1] + nz * M->m[1][2];
                const f32 nmz = nx * M->m[2][0] + ny * M->m[2][1] + nz * M->m[2][2];

                const f32 dot_t = nx * M->m[3][0] + ny * M->m[3][1] + nz * M->m[3][2];
                const f32 dm_raw = dot_t + d;

                const f32 inv_len = 1.f / sqrtf(nmx*nmx + nmy*nmy + nmz*nmz);

                model_frustum.normal[p].x = sft_vf_set1(nmx * inv_len);
                model_frustum.normal[p].y = sft_vf_set1(nmy * inv_len);
                model_frustum.normal[p].z = sft_vf_set1(nmz * inv_len);
                model_frustum.d[p]        = sft_vf_set1(dm_raw * inv_len);

                model_frustum.abs_normal[p].x = sft_vf_set1(fabsf(nmx * inv_len));
                model_frustum.abs_normal[p].y = sft_vf_set1(fabsf(nmy * inv_len));
                model_frustum.abs_normal[p].z = sft_vf_set1(fabsf(nmz * inv_len));
            }

            // transform rays to model space
            const sftmat inv_m = sft_m_qinv(inst->transform);
            const sftvmat v_inv_m = sft_vm_broadcast(inv_m);
            const sftvvec local_orig = sft_vv_transform(ray_orig, v_inv_m);
            const sftvvec local_light = sft_vv_transform(sft_vv_add(ray_orig, L_raw), v_inv_m);
            const sftvvec local_L = sft_vv_sub(local_light, local_orig);
            
            const vf32 local_t_max = sft_vv_length(local_L);
            const sftvvec local_dir = sft_vv_norm_fast(local_L);

            // ---  BLAS traversal (model space)  ---
            u32 blas_stack[64];
            i32 blas_top = 0;
            blas_stack[blas_top++] = model->blas_root_ind;

            while (blas_top > 0) {
                const u32 bnode_ind = blas_stack[--blas_top];
                const struct sft_bvh_node* bnode = &model->blas_nodes[bnode_ind];

                const vf32 bhit = sft__frustum_vs_bvh_node(bnode, &model_frustum);
                i32 bhit_mask = sft_vf_movemask(bhit);

                while (bhit_mask) {
                    const i32 blane = sft__tzcnt(bhit_mask);
                    bhit_mask &= (bhit_mask - 1);

                    if (0xFFFFFFFF == bnode->child_indices[blane]) {
                        continue;
                    }

                    if (!bnode->is_leaf_group) {
                        blas_stack[blas_top++] = bnode->child_indices[blane];
                        continue;
                    }

                    // hit a triangle
                    const u32 payload = bnode->child_indices[blane] & ~0x80000000;
                    const u32 m_ind = payload >> 7;         // unpack global meshlet index
                    const u32 local_tri = payload & 0x7F;   // unpack the exact triagnle (0-127)

                    const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
                    const sft_mesh* mesh = &inst->model->meshes[mesh_ind];
                    const u32 local_m_ind = m_ind - mesh->model_meshlet_offset;

                    const struct sft_meshlet* m_verts = &mesh->meshlets[local_m_ind];
                    const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];

                    // fetch only the exact triangle the BVH hit
                    const u32 batch = local_tri / SFT_SIMD_WIDTH;
                    const u32 lane = local_tri % SFT_SIMD_WIDTH;
                    const u32 offset = batch * (SFT_SIMD_WIDTH * 4);

                    const u8 i0 = m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane];
                    const u8 i1 = m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane];
                    const u8 i2 = m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane];

                    const sftvec v0 = SFT_V(m_verts->x[i0], m_verts->y[i0], m_verts->z[i0]);
                    const sftvec v1 = SFT_V(m_verts->x[i1], m_verts->y[i1], m_verts->z[i1]);
                    const sftvec v2 = SFT_V(m_verts->x[i2], m_verts->y[i2], m_verts->z[i2]);

                    const vf32 current_active = sft_vf_andnot(saturation_mask, active_mask);

                    const vf32 hit = sft__ray_packet_vs_triangle_any_hit(
                        v0, v1, v2, local_orig, local_dir, local_t_max, current_active);

                    saturation_mask = sft_vf_or(saturation_mask, hit);

                    // early out if the entire packet is now shadowed
                    if (sft_vf_movemask(saturation_mask) == full_saturation) {
                        return saturation_mask; 
                    }
                }
            }
        }
    }

    return saturation_mask;
}

static SFT_INLINE struct sft_clip_vert sft__lerp_clip_vert(struct sft_clip_vert a, struct sft_clip_vert b, f32 t) {
    return (struct sft_clip_vert){
        .tx = SFT_LERPF(a.tx, b.tx, t),
        .ty = SFT_LERPF(a.ty, b.ty, t),
        .tz = SFT_LERPF(a.tz, b.tz, t),
        .w  = SFT_LERPF(a.w,  b.w,  t),
        .u  = SFT_LERPF(a.u,  b.u,  t),
        .v  = SFT_LERPF(a.v,  b.v,  t),
        .lu = SFT_LERPF(a.lu, b.lu, t),
        .lv = SFT_LERPF(a.lv, b.lv, t),
        .px = SFT_LERPF(a.px, b.px, t),
        .py = SFT_LERPF(a.py, b.py, t),
        .pz = SFT_LERPF(a.pz, b.pz, t),
        .nx = SFT_LERPF(a.nx, b.nx, t),
        .ny = SFT_LERPF(a.ny, b.ny, t),
        .nz = SFT_LERPF(a.nz, b.nz, t)
    };
}

static i32 sft__clip_triangle_near_plane(
    struct sft_clip_vert out[2][3], 
    const struct sft_clip_vert in[3]
) {
    f32 dists[3];
    i32 clip_mask = 0;

    // calculate dists to Z=0 and build a bitmask of which verts are inside
    for (i32 i = 0; i < 3; i += 1) {
        dists[i] = in[i].tz;
        clip_mask |= (dists[i] >= 0.f) << i;
    }

    // switch based on the mask topology
    switch (clip_mask) {
        case 0: { // 000, all outside
            return 0;
        }
        case 7: { // 111, all inside
            out[0][0] = in[0];
            out[0][1] = in[1];
            out[0][2] = in[2];
            return 1;
        }

        // 1 in, 2 out (tri shrinks)
        case 1: // 001, only v0 inside
        case 2: // 010, only v1 inside
        case 4: // 100, only v2 inside
        {
            const i32 in_ind = clip_mask >> 1;
            const i32 b_ind = (2 == in_ind) ? 0 : in_ind + 1;
            const i32 c_ind = (0 == in_ind) ? 2 : in_ind - 1;

            const struct sft_clip_vert v_a = in[in_ind];
            const struct sft_clip_vert v_b = in[b_ind];
            const struct sft_clip_vert v_c = in[c_ind];

            // interpolate new verts on the edges leaving the inside vertex
            const f32 d_a = dists[in_ind];
            const f32 t_ab = d_a / (d_a - dists[b_ind]);
            const f32 t_ac = d_a / (d_a - dists[c_ind]);

            out[0][0] = v_a;
            out[0][1] = sft__lerp_clip_vert(v_a, v_b, t_ab);
            out[0][2] = sft__lerp_clip_vert(v_a, v_c, t_ac);

            return 1;
        }

        // 2 in, 1 out (quad split into 2 tris)
        case 3: // 011, v2 is out
        case 5: // 101, v1 is out
        case 6: // 110, v0 is out
        {
            const i32 out_ind = (~clip_mask & 7) >> 1;
            const i32 a_ind = (0 == out_ind) ? 2 : out_ind - 1;
            const i32 c_ind = (2 == out_ind) ? 0 : out_ind + 1;

            const struct sft_clip_vert v_a = in[a_ind];
            const struct sft_clip_vert v_b = in[out_ind];
            const struct sft_clip_vert v_c = in[c_ind];

            const f32 d_a = dists[a_ind];
            const f32 d_b = dists[out_ind];
            const f32 d_c = dists[c_ind];
            const f32 t_ab = d_a / (d_a - d_b);
            const f32 t_cb = d_c / (d_c - d_b);

            const struct sft_clip_vert new_ab = sft__lerp_clip_vert(v_a, v_b, t_ab);
            const struct sft_clip_vert new_cb = sft__lerp_clip_vert(v_c, v_b, t_cb);

            // decomposition into 2 triangles
            out[0][0] = v_a;
            out[0][1] = new_ab;
            out[0][2] = v_c;
            
            out[1][0] = new_ab;
            out[1][1] = new_cb;
            out[1][2] = v_c;
            
            return 2;
        }
    }
    return 0;
}

static i32 sft__project_and_store_clipped_triangle(
    const struct sft_clip_vert tri[3], 
    i32* t_clip_base, 
    i32* t_clip_count
) {
    // if local batch is empty, atomically grab 64 slots
    if (0 == *t_clip_count) {
        *t_clip_base = sft_atomic_add(&sft_state->clip_cache.count, 64);
        *t_clip_count = 64;
        sft_arena_commit_if_needed(&sft_state->clip_arena, (*t_clip_base + 64) * sizeof(struct sft_clipped_triangle));
    }

    // take one slot from private batch
    const i32 slot = *t_clip_base;
    *t_clip_base  += 1;
    *t_clip_count -= 1;
    
    struct sft_clipped_triangle* out_tri = &sft_state->clip_cache.pool[slot];

    for (i32 i = 0; i < 3; i += 1) {
        const f32 inv_w = 1.f / tri[i].w;
        const f32 sx = (tri[i].tx * inv_w + 1.f) * sft_state->display.half_width + sft_state->display.vp_x;
        const f32 sy = (-tri[i].ty * inv_w + 1.f) * sft_state->display.half_height + sft_state->display.vp_y;
        
        out_tri->sx[i] = sx;
        out_tri->sy[i] = sy;
        out_tri->inv_w[i] = inv_w;
    }
    
    return slot;
}

static u64 sft__tex_heap_alloc(u64 size) {
    size = (size + 63ULL) & ~63ULL;

    struct sft_tex_heap_node** curr = &sft_state->tex_heap_free_list;
    while (*curr) {
        if ((*curr)->size >= size) {
            const u64 offset = (*curr)->offset;
            
            if ((*curr)->size > size + 256) {
                // split block
                (*curr)->offset += size;
                (*curr)->size -= size;
            }
            else {
                // consume entire block
                struct sft_tex_heap_node* old = *curr;
                *curr = (*curr)->next;
                free(old); 
            }
            return offset;
        }
        curr = &(*curr)->next;
    }

    // no suitable free block, bump arena forward
    const u64 offset = sft_state->tex_heap_arena.offset;
    sft_state->tex_heap_arena.offset += size;
    
    sft_arena_commit_if_needed(&sft_state->tex_heap_arena, sft_state->tex_heap_arena.offset);
    
    return offset;
}

static void sft__tex_heap_free(u64 offset, u64 size) {
    size = (size + 63ULL) & ~63ULL;

    struct sft_tex_heap_node* new_node = malloc(sizeof(struct sft_tex_heap_node));
    new_node->offset = offset;
    new_node->size = size;

    struct sft_tex_heap_node* prev = NULL;
    struct sft_tex_heap_node* curr = sft_state->tex_heap_free_list;

    // find insertion point to maintain sorted order by offset
    while (curr && curr->offset < offset) {
        prev = curr;
        curr = curr->next;
    }

    new_node->next = curr;
    if (prev) {
        prev->next = new_node;
    }
    else {
        sft_state->tex_heap_free_list = new_node;
    }

    // merge with previous if adjacent
    if (prev && prev->offset + prev->size == new_node->offset) {
        prev->size += new_node->size;
        prev->next = new_node->next;
        free(new_node);
        new_node = prev;
    }

    // merge with next if adjacent
    if (new_node->next && new_node->offset + new_node->size == new_node->next->offset) {
        struct sft_tex_heap_node* to_free = new_node->next;
        new_node->size += to_free->size;
        new_node->next = to_free->next;
        free(to_free);
    }
}

static void sft__tex_pool_grow() {
    const i32 old_cap = sft_state->tex_pool.capacity;
    const i32 new_cap = (0 == old_cap) ? 1024 : old_cap * 2;
    
    sft_state->tex_pool.generation = realloc(sft_state->tex_pool.generation, new_cap * sizeof(u32));
    sft_state->tex_pool.next_free  = realloc(sft_state->tex_pool.next_free, new_cap * sizeof(i32));
    sft_state->tex_pool.heap_offset = realloc(sft_state->tex_pool.heap_offset, new_cap * sizeof(u64));
    sft_state->tex_pool.w = realloc(sft_state->tex_pool.w, new_cap * sizeof(i32));
    sft_state->tex_pool.h = realloc(sft_state->tex_pool.h, new_cap * sizeof(i32));
    sft_state->tex_pool.mip_levels = realloc(sft_state->tex_pool.mip_levels, new_cap * sizeof(i32));
    sft_state->tex_pool.is_pot = realloc(sft_state->tex_pool.is_pot, new_cap * sizeof(u8));
    
    sft_state->tex_pool.mip_offsets = realloc(sft_state->tex_pool.mip_offsets, new_cap * 14 * sizeof(u32));
    sft_state->tex_pool.mip_blocks_w = realloc(sft_state->tex_pool.mip_blocks_w, new_cap * 14 * sizeof(i32));
    sft_state->tex_pool.mip_stride_shift = realloc(sft_state->tex_pool.mip_stride_shift, new_cap * 14 * sizeof(i32));

    for (i32 i = old_cap; i < new_cap; i += 1) {
        sft_state->tex_pool.generation[i] = 1;
        sft_state->tex_pool.next_free[i] = i + 1;
    }
    sft_state->tex_pool.next_free[new_cap - 1] = sft_state->tex_pool.free_head;
    sft_state->tex_pool.free_head = old_cap;
    sft_state->tex_pool.capacity = new_cap;
}

static void sft__mat_pool_grow_static() {
    const i32 old_cap = sft_state->mat_pool.capacity;
    const i32 new_cap = (0 == old_cap) ? 32 : old_cap * 2;
    
    sft_state->mat_pool.generation = realloc(sft_state->mat_pool.generation, new_cap * sizeof(u32));
    sft_state->mat_pool.next_free  = realloc(sft_state->mat_pool.next_free, new_cap * sizeof(i32));
    sft_state->mat_pool.albedo_tex = realloc(sft_state->mat_pool.albedo_tex, new_cap * sizeof(sft_tex_handle));
    sft_state->mat_pool.base_color = realloc(sft_state->mat_pool.base_color, new_cap * sizeof(u32));
    sft_state->mat_pool.shininess  = realloc(sft_state->mat_pool.shininess, new_cap * sizeof(f32));

    for (i32 i = old_cap; i < new_cap; i += 1) {
        sft_state->mat_pool.generation[i] = 1;
        sft_state->mat_pool.next_free[i] = i + 1;
    }
    sft_state->mat_pool.next_free[new_cap - 1] = sft_state->mat_pool.free_head;
    sft_state->mat_pool.free_head = old_cap;
    sft_state->mat_pool.capacity = new_cap;
}

static void sft__mat_pool_grow_transient() {
    const i32 old_cap = sft_state->mat_pool_transient.capacity;
    const i32 new_cap = (0 == old_cap) ? 32 : old_cap * 2;
    
    sft_state->mat_pool_transient.albedo_tex = realloc(sft_state->mat_pool_transient.albedo_tex, new_cap * sizeof(sft_tex_handle));
    sft_state->mat_pool_transient.base_color = realloc(sft_state->mat_pool_transient.base_color, new_cap * sizeof(u32));
    sft_state->mat_pool_transient.shininess  = realloc(sft_state->mat_pool_transient.shininess, new_cap * sizeof(f32));
    sft_state->mat_pool_transient.capacity = new_cap;
}


/*================================================================
:: FUNCTION DEFINITIONS (THREADING)
================================================================*/

#if defined(_WIN32)

SFT_INLINE i32 sft_atomic_add(volatile i32* ptr, i32 val) {
    return (i32)_InterlockedExchangeAdd((volatile long*)ptr, (long)val);
}
SFT_INLINE i32 sft_atomic_cas(volatile i32* ptr, i32 cmp, i32 val) {
    return (i32)_InterlockedCompareExchange((volatile long*)ptr, (long)val, (long)cmp);
}
SFT_INLINE u64 sft_atomic_cas64(volatile u64* ptr, u64 cmp, u64 val) {
    return (u64)_InterlockedCompareExchange64((volatile long long*)ptr, (long long)val, (long long)cmp);
}

SFT_INLINE void sft_sem_init(sft_sem* s, i32 init_count) {
    *s = CreateSemaphoreA(NULL, (long)init_count, 9999, NULL);
}
SFT_INLINE void sft_sem_wait(sft_sem* s) {
    WaitForSingleObject(*s, INFINITE);
}
SFT_INLINE void sft_sem_post(sft_sem* s, i32 count) {
    ReleaseSemaphore(*s, (long)count, NULL);
}

static DWORD WINAPI sft__thread_runner(LPVOID arg) {
    struct sft_thread_payload* payload = (struct sft_thread_payload*)arg;
    sft_thread_id = payload->thread_id;
    payload->func(payload->user_data);
    return 0;
}
SFT_FUNC sft_thread sft_thread_create(struct sft_thread_payload* payload) {
    if (!payload) {
        SFT_ERR_EXIT("thread payload is NULL\n");
    }

    sft_thread thread = CreateThread(NULL, 0, sft__thread_runner, payload, 0, NULL);
    if (!thread) {
        SFT_ERR_EXIT("CreateThread failed to spawn thread\n");
    }

    return thread;
}
SFT_FUNC void sft_thread_join(sft_thread thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

#else // !defined _WIN32

SFT_INLINE i32 sft_atomic_add(volatile i32* ptr, i32 val) {
    return __sync_fetch_and_add(ptr, val);
}
SFT_INLINE i32 sft_atomic_cas(volatile i32* ptr, i32 cmp, i32 val) {
    return __sync_val_compare_and_swap(ptr, cmp, val);
}
SFT_INLINE u64 sft_atomic_cas64(volatile u64* ptr, u64 cmp, u64 val) {
    return __sync_val_compare_and_swap(ptr, cmp, val);
}

SFT_INLINE void sft_sem_init(sft_sem* s, i32 init_count) {
    sem_init(s, 0, init_count);
}
SFT_INLINE void sft_sem_wait(sft_sem* s) {
    while (-1 == sem_wait(s) && EINTR == errno) {}
}
SFT_INLINE void sft_sem_post(sft_sem* s, i32 count) { 
    for (i32 i = 0; i < count; i += 1) sem_post(s); 
}

static void* sft__thread_runner(void* arg) {
    struct sft_thread_payload* payload = (struct sft_thread_payload*)arg;
    sft_thread_id = payload->thread_id;
    payload->func(payload->user_data);
    return NULL;
}
SFT_FUNC sft_thread sft_thread_create(struct sft_thread_payload* payload) {
    if (!payload) {
        SFT_ERR_EXIT("thread payload is NULL\n");
    }

    pthread_t thread;
    const i32 ret = pthread_create(&thread, NULL, sft__thread_runner, payload);
    if (0 != ret) {
        SFT_ERR_EXIT("pthread_create failed with code %d\n", ret);
    }

    return thread;
}
SFT_FUNC void sft_thread_join(sft_thread thread) {
    pthread_join(thread, NULL);
}

#endif

SFT_INLINE i32 sft_atomic_exchange(volatile i32* ptr, i32 new_val) {
    i32 old_val = *ptr;
    while (1) {
        i32 actual = sft_atomic_cas(ptr, old_val, new_val);
        if (actual == old_val) {
            return old_val;
        }
        old_val = actual; // retry with the updated tail
    }
}

static SFT_INLINE void sft__push_new_chunk(struct sft_thread_bin_state* bin, i32 tile_ind, i32* t_chunk_base, i32* t_chunk_count) {
    // if local batch is empty, atomically grab 64 more chunks
    if (0 == *t_chunk_count) {
        *t_chunk_base = sft_atomic_add(&sft_state->next_chunk_ind.count, 64);
        *t_chunk_count = 64;
        sft_arena_commit_if_needed(&sft_state->chunk_arena, (*t_chunk_base + 64) * sizeof(struct sft_payload_chunk));
    }
    
    // take one chunk from private batch
    const i32 new_chunk = *t_chunk_base;
    *t_chunk_base  += 1;
    *t_chunk_count -= 1;
    
    struct sft_payload_chunk* chunk_ptr = &sft_state->display.chunk_pool[new_chunk];
    memset(chunk_ptr->payloads, 0xFF, 31 * sizeof(u64));
    chunk_ptr->next_ind = -1;

    const i32 old_tail = sft_atomic_exchange(&sft_state->display.tile_tails[tile_ind], new_chunk);
    if (-1 != old_tail) {
        sft_state->display.chunk_pool[old_tail].next_ind = new_chunk;
    }
    else {
        sft_state->display.tile_heads[tile_ind] = new_chunk;
    }

    bin->chunk_ind = new_chunk;
    bin->write_head = chunk_ptr->payloads;
    bin->write_end = chunk_ptr->payloads + 31;
}

// pushes a payload to the correct local tile bins
static SFT_INLINE void sft__geom_bin_tiles(
    u64 payload, i32 b_tx0, i32 b_tx1, i32 b_ty0, i32 b_ty1,
    struct sft_thread_bin_state* local_bins,
    i32* t_chunk_base, i32* t_chunk_count
) {
    const i32 max_tile_x = sft_state->display.tile_cols - 1;
    const i32 max_tile_y = sft_state->display.tile_rows - 1;

    if (b_tx1 < 0 || b_tx0 > max_tile_x || b_ty1 < 0 || b_ty0 > max_tile_y) {
        return;
    }

    b_tx0 = SFT_MAX(b_tx0, 0), b_tx1 = SFT_MIN(b_tx1, max_tile_x);
    b_ty0 = SFT_MAX(b_ty0, 0), b_ty1 = SFT_MIN(b_ty1, max_tile_y);

    if (b_tx0 == b_tx1 && b_ty0 == b_ty1) {
        const i32 tile_ind = b_ty0 * sft_state->display.tile_cols + b_tx0;
        struct sft_thread_bin_state* bin = &local_bins[tile_ind];
        if (bin->write_head == bin->write_end) {
            sft__push_new_chunk(bin, tile_ind, t_chunk_base, t_chunk_count);
        }
        *bin->write_head++ = payload;
        bin->payload_count += 1;
    }
    else {
        for (i32 ty = b_ty0; ty <= b_ty1; ty += 1) {
            const i32 row_offset = ty * sft_state->display.tile_cols;
            for (i32 tx = b_tx0; tx <= b_tx1; tx += 1) {
                const i32 tile_ind = row_offset + tx;
                struct sft_thread_bin_state* bin = &local_bins[tile_ind];
                if (bin->write_head == bin->write_end) {
                    sft__push_new_chunk(bin, tile_ind, t_chunk_base, t_chunk_count);
                }
                *bin->write_head++ = payload;
                bin->payload_count += 1;
            }
        }
    }
}

// processes intersecting triangles dynamically via the scalar near plane clipper
static SFT_INLINE void sft__geom_clip_and_bin(
    i32 active_intersecting,
    vi32 ind_v0, vi32 ind_v1, vi32 ind_v2,
    const f32* clip_tx, const f32* clip_ty, const f32* clip_tz, const f32* clip_w,
    u32 inst_ind, u32 m_ind,
    struct sft_thread_bin_state* local_bins,
    i32* t_chunk_base, i32* t_chunk_count,
    i32* t_clip_base, i32* t_clip_count
) {
    while (active_intersecting) {
        const i32 lane = sft__tzcnt(active_intersecting);
        active_intersecting &= (active_intersecting - 1);

        const i32 i0 = sft_vi_extract(ind_v0, lane);
        const i32 i1 = sft_vi_extract(ind_v1, lane);
        const i32 i2 = sft_vi_extract(ind_v2, lane);

        const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
        const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
        const sft_mesh* mesh = &inst->model->meshes[mesh_ind];
        const u32 local_m_ind = m_ind - mesh->model_meshlet_offset;
        const struct sft_meshlet_attribs* m_attribs = &mesh->meshlet_attribs[local_m_ind];
        const struct sft_ss_meshlet* ss_m = &sft_state->ss_cache[inst->ss_meshlet_offset + m_ind];

        const struct sft_clip_vert in_tri[3] = { {
            .tx = clip_tx[i0], .ty = clip_ty[i0], .tz = clip_tz[i0], .w = clip_w[i0],
            .u = m_attribs->u[i0], .v = m_attribs->v[i0], .lu = m_attribs->lu[i0], .lv = m_attribs->lv[i0],
            .px = ss_m->wx[i0], .py = ss_m->wy[i0], .pz = ss_m->wz[i0],
            .nx = ss_m->wnx[i0], .ny = ss_m->wny[i0], .nz = ss_m->wnz[i0]
        }, {
            .tx = clip_tx[i1], .ty = clip_ty[i1], .tz = clip_tz[i1], .w = clip_w[i1],
            .u = m_attribs->u[i1], .v = m_attribs->v[i1], .lu = m_attribs->lu[i1], .lv = m_attribs->lv[i1],
            .px = ss_m->wx[i1], .py = ss_m->wy[i1], .pz = ss_m->wz[i1],
            .nx = ss_m->wnx[i1], .ny = ss_m->wny[i1], .nz = ss_m->wnz[i1]
        }, {
            .tx = clip_tx[i2], .ty = clip_ty[i2], .tz = clip_tz[i2], .w = clip_w[i2],
            .u = m_attribs->u[i2], .v = m_attribs->v[i2], .lu = m_attribs->lu[i2], .lv = m_attribs->lv[i2],
            .px = ss_m->wx[i2], .py = ss_m->wy[i2], .pz = ss_m->wz[i2],
            .nx = ss_m->wnx[i2], .ny = ss_m->wny[i2], .nz = ss_m->wnz[i2]
        } };

        struct sft_clip_vert out_tris[2][3];
        const i32 num_clipped = sft__clip_triangle_near_plane(out_tris, in_tri);

        for (i32 c = 0; c < num_clipped; c += 1) {
            const i32 slot = sft__project_and_store_clipped_triangle(out_tris[c], t_clip_base, t_clip_count);
            struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[slot];

            // store UVs and inds for the resolver
            ct->inst_ind = inst_ind;
            ct->m_ind = m_ind;
            for (i32 v = 0; v < 3; v += 1) {
                ct->u[v] = out_tris[c][v].u;
                ct->v[v] = out_tris[c][v].v;
                ct->lu[v] = out_tris[c][v].lu;
                ct->lv[v] = out_tris[c][v].lv;
                ct->px[v] = out_tris[c][v].px;
                ct->py[v] = out_tris[c][v].py;
                ct->pz[v] = out_tris[c][v].pz;
                ct->nx[v] = out_tris[c][v].nx;
                ct->ny[v] = out_tris[c][v].ny;
                ct->nz[v] = out_tris[c][v].nz;
            }

            const f32 area = (ct->sx[1] - ct->sx[0]) * (ct->sy[2] - ct->sy[0]) - (ct->sy[1] - ct->sy[0]) * (ct->sx[2] - ct->sx[0]);
            if (area <= 0.1f) {
                continue; 
            }
            
            const i32 min_x = (i32)floorf(SFT_MIN(ct->sx[0], SFT_MIN(ct->sx[1], ct->sx[2])));
            const i32 max_x = (i32)ceilf(SFT_MAX(ct->sx[0], SFT_MAX(ct->sx[1], ct->sx[2])));
            const i32 min_y = (i32)floorf(SFT_MIN(ct->sy[0], SFT_MIN(ct->sy[1], ct->sy[2])));
            const i32 max_y = (i32)ceilf(SFT_MAX(ct->sy[0], SFT_MAX(ct->sy[1], ct->sy[2])));
            
            const i32 b_tx0 = min_x >> SFT_TILE_SHIFT, b_tx1 = max_x >> SFT_TILE_SHIFT;
            const i32 b_ty0 = min_y >> SFT_TILE_SHIFT, b_ty1 = max_y >> SFT_TILE_SHIFT;

            const u64 clip_payload = (u64)slot;
            sft__geom_bin_tiles(clip_payload, b_tx0, b_tx1, b_ty0, b_ty1, local_bins, t_chunk_base, t_chunk_count);
        }
    }
}

// area culling, determines bounds, and routes batches to the tile binning function
static SFT_INLINE void sft__geom_setup_and_bin_batch(
    u32 batch, u32 inst_ind, u32 m_ind, const struct sft_meshlet_inds* m_inds,
    const f32* clip_tx, const f32* clip_ty, const f32* clip_tz, const f32* clip_w,
    const f32* scratch_sx, const f32* scratch_sy,
    struct sft_thread_bin_state* local_bins,
    i32* t_chunk_base, i32* t_chunk_count, i32* t_clip_base, i32* t_clip_count
) {
    const u32 batch_offset = batch * (SFT_SIMD_WIDTH * 4);
    const vi32 ind_v0 = sft_vi_loadu_8u_to_32i(&m_inds->inds[batch_offset + (0 * SFT_SIMD_WIDTH)]);
    const vi32 ind_v1 = sft_vi_loadu_8u_to_32i(&m_inds->inds[batch_offset + (1 * SFT_SIMD_WIDTH)]);
    const vi32 ind_v2 = sft_vi_loadu_8u_to_32i(&m_inds->inds[batch_offset + (2 * SFT_SIMD_WIDTH)]);

    const vi32 v_zero_i = sft_vi_setzero();
    const vi32 valid_inds_mask = sft_vi_or(ind_v0, sft_vi_or(ind_v1, ind_v2));
    const vf32 v_valid = sft_vf_cast_vi32(sft_vi_cmp_gt(valid_inds_mask, v_zero_i));
    if (0 == sft_vf_movemask(v_valid)) {
        return;
    }

    const vf32 v_zero = sft_vf_setzero();
    const vf32 tz0 = sft_vf_gather(clip_tz, ind_v0);
    const vf32 tz1 = sft_vf_gather(clip_tz, ind_v1);
    const vf32 tz2 = sft_vf_gather(clip_tz, ind_v2);

    const vf32 v_in0 = sft_vf_cmp_ge(tz0, v_zero);
    const vf32 v_in1 = sft_vf_cmp_ge(tz1, v_zero);
    const vf32 v_in2 = sft_vf_cmp_ge(tz2, v_zero);

    const vf32 v_out0 = sft_vf_cmp_lt(tz0, v_zero);
    const vf32 v_out1 = sft_vf_cmp_lt(tz1, v_zero);
    const vf32 v_out2 = sft_vf_cmp_lt(tz2, v_zero);

    const vf32 all_inside = sft_vf_and(v_in0, sft_vf_and(v_in1, v_in2));
    const vf32 all_outside = sft_vf_and(v_out0, sft_vf_and(v_out1, v_out2));
    const vf32 handled = sft_vf_or(all_inside, all_outside);
    const vf32 intersecting = sft_vf_and(v_valid, sft_vf_andnot(handled, sft_vf_cast_vi32(sft_vi_set1(-1))));

    const i32 active_intersecting = sft_vf_movemask(intersecting);
    if (active_intersecting) {
        sft__geom_clip_and_bin(
            active_intersecting, ind_v0, ind_v1, ind_v2,
            clip_tx, clip_ty, clip_tz, clip_w, inst_ind, m_ind,
            local_bins, t_chunk_base, t_chunk_count, t_clip_base, t_clip_count);
    }

    if (0 == sft_vf_movemask(all_inside)) {
        return;
    }

    const vf32 sx0 = sft_vf_gather(scratch_sx, ind_v0);
    const vf32 sy0 = sft_vf_gather(scratch_sy, ind_v0);
    const vf32 sx1 = sft_vf_gather(scratch_sx, ind_v1);
    const vf32 sy1 = sft_vf_gather(scratch_sy, ind_v1);
    const vf32 sx2 = sft_vf_gather(scratch_sx, ind_v2);
    const vf32 sy2 = sft_vf_gather(scratch_sy, ind_v2);

    const vf32 dx1 = sft_vf_sub(sx1, sx0);
    const vf32 dy1 = sft_vf_sub(sy1, sy0);
    const vf32 dx2 = sft_vf_sub(sx2, sx0);
    const vf32 dy2 = sft_vf_sub(sy2, sy0);

    const vf32 v_min_area = sft_vf_set1(0.1f);
    vf32 valid_mask = sft_vf_and(all_inside, sft_vf_cmp_gt(sft_vf_fmsub(dx1, dy2, sft_vf_mul(dy1, dx2)), v_min_area));
    if (0 == sft_vf_movemask(valid_mask)) {
        return;
    }

    const vi32 tx_min = sft_vi_srai(sft_vi_cvttps_epi32(sft_vf_min(sx0, sft_vf_min(sx1, sx2))), SFT_TILE_SHIFT);
    const vi32 tx_max = sft_vi_srai(sft_vi_cvttps_epi32(sft_vf_max(sx0, sft_vf_max(sx1, sx2))), SFT_TILE_SHIFT);
    const vi32 ty_min = sft_vi_srai(sft_vi_cvttps_epi32(sft_vf_min(sy0, sft_vf_min(sy1, sy2))), SFT_TILE_SHIFT);
    const vi32 ty_max = sft_vi_srai(sft_vi_cvttps_epi32(sft_vf_max(sy0, sft_vf_max(sy1, sy2))), SFT_TILE_SHIFT);

    const vi32 v_max_tile_x_i = sft_vi_set1(sft_state->display.tile_cols - 1);
    const vi32 v_max_tile_y_i = sft_vi_set1(sft_state->display.tile_rows - 1);

    valid_mask = sft_vf_and(valid_mask, sft_vf_cast_vi32(sft_vi_and(
        sft_vi_and(sft_vi_cmp_ge(tx_max, v_zero_i), sft_vi_cmp_le(tx_min, v_max_tile_x_i)),
        sft_vi_and(sft_vi_cmp_ge(ty_max, v_zero_i), sft_vi_cmp_le(ty_min, v_max_tile_y_i))
    )));

    const u32 valid_tris = sft_vf_movemask(valid_mask);
    if (!valid_tris) {
        return;
    }

    alignas(32) i32 s_tx0[SFT_SIMD_WIDTH], s_tx1[SFT_SIMD_WIDTH];
    alignas(32) i32 s_ty0[SFT_SIMD_WIDTH], s_ty1[SFT_SIMD_WIDTH];
    sft_vi_store(s_tx0, tx_min); sft_vi_store(s_tx1, tx_max);
    sft_vi_store(s_ty0, ty_min); sft_vi_store(s_ty1, ty_max);

    i32 b_tx0 = 9999, b_tx1 = -9999, b_ty0 = 9999, b_ty1 = -9999;
    i32 active_lanes = valid_tris;

    while (active_lanes) {
        const i32 lane = sft__tzcnt(active_lanes);
        active_lanes &= (active_lanes - 1);

        if (s_tx0[lane] < b_tx0) b_tx0 = s_tx0[lane];
        if (s_tx1[lane] > b_tx1) b_tx1 = s_tx1[lane];
        if (s_ty0[lane] < b_ty0) b_ty0 = s_ty0[lane];
        if (s_ty1[lane] > b_ty1) b_ty1 = s_ty1[lane];
    }

    const u64 batch_payload = ((u64)inst_ind) | 
                              ((u64)m_ind << 24) | 
                              ((u64)batch << 48) | 
                              ((u64)valid_tris << 56);

    sft__geom_bin_tiles(batch_payload, b_tx0, b_tx1, b_ty0, b_ty1, local_bins, t_chunk_base, t_chunk_count);
}

static SFT_INLINE sftvmat sft__gather_posed_matrix(const sftmat* posed_matrices, vi32 joint_inds) {
    sftvmat r;
    const f32* base = (const f32*)posed_matrices;
    const vi32 offset = sft_vi_slli(joint_inds, 4); // each sftmat is 16 floats
    
    for (i32 row = 0; row < 4; row += 1) {
        for (i32 col = 0; col < 4; col += 1) {
            const vi32 element_offset = sft_vi_add(offset, sft_vi_set1(row * 4 + col));
            r.m[row][col] = sft_vf_gather(base, element_offset);
        }
    }
    return r;
}

// geometry binning pipeline
static SFT_INLINE void sft__worker_phase_geom(struct sft_thread_bin_state* local_bins) {
    i32 t_chunk_base = 0, t_chunk_count = 0;
    i32 t_clip_base = 0, t_clip_count = 0;

    alignas(32) f32 clip_tx[64];
    alignas(32) f32 clip_ty[64];
    alignas(32) f32 clip_tz[64];
    alignas(32) f32 clip_w[64];
    alignas(32) f32 scratch_sx[64];
    alignas(32) f32 scratch_sy[64];

    while (1) {
        const i32 job_ind = sft_atomic_add(&sft_state->next_job_ind.count, 1);
        if (job_ind >= sft_state->cmd_queue.job_count.count) {
            break;
        }

        const struct sft_geom_job* job = &sft_state->cmd_queue.jobs[job_ind];
        const sft_instance* inst       = &sft_state->cmd_queue.instances[job->instance_ind];
        const struct sft_mesh* mesh    = &inst->model->meshes[job->mesh_ind];

        // no longer pre combine MVP, apply the view_proj to the evaluated world space verts
        const sftvmat v_vp = sft_vm_broadcast(sft_state->view_proj);
        const sftvmat v_transform = sft_vm_broadcast(inst->transform);

        const vf32 v_scale_x = sft_vf_set1(sft_state->display.half_width);
        const vf32 v_bias_x  = sft_vf_set1(sft_state->display.half_width + sft_state->display.vp_x);
        const vf32 v_scale_y = sft_vf_set1(-sft_state->display.half_height); 
        const vf32 v_bias_y  = sft_vf_set1(sft_state->display.half_height + sft_state->display.vp_y);
        const vf32 v_two = sft_vf_set1(2.f);
        const vf32 v_zero = sft_vf_setzero();

        const sftvvec v_camera_pos = {
            sft_vf_set1(sft_state->camera_pos.x),
            sft_vf_set1(sft_state->camera_pos.y),
            sft_vf_set1(sft_state->camera_pos.z)
        };

        for (u32 i = 0; i < job->meshlet_count; i += SFT_SIMD_WIDTH) {
            const u32 m_ind = job->meshlet_start + i;

            // ---  meshlet culling  ---
            sftvvec center;
            center.x = sft_vf_load(&mesh->bounds_center_x[m_ind]);
            center.y = sft_vf_load(&mesh->bounds_center_y[m_ind]);
            center.z = sft_vf_load(&mesh->bounds_center_z[m_ind]);

            sftvvec cone_axis;
            cone_axis.x = sft_vf_load(&mesh->cone_axis_x[m_ind]);
            cone_axis.y = sft_vf_load(&mesh->cone_axis_y[m_ind]);
            cone_axis.z = sft_vf_load(&mesh->cone_axis_z[m_ind]);

            // ---  skin the culling volumes  ---
            if (mesh->culling_bone_inds && inst->posed_bone_matrices) {
                alignas(32) u32 bones[SFT_SIMD_WIDTH];
                sft_vi_store((vi32*)bones, sft_vi_load((const i32*)&mesh->culling_bone_inds[m_ind]));
                const sftvmat v_bone_mat = sft_vm_broadcast(inst->posed_bone_matrices[bones[0]]); // broadcast primary bone
                center = sft_vv_transform(center, v_bone_mat);
                cone_axis = sft_vv_transform_dir(cone_axis, v_bone_mat);
                
                // bring the culling bounds into world space
                center = sft_vv_transform(center, v_transform);
                cone_axis = sft_vv_transform_dir(cone_axis, v_transform);
            }
            else {
                center = sft_vv_transform(center, v_transform);
                cone_axis = sft_vv_transform_dir(cone_axis, v_transform);
            }

            const f32 sx = sqrtf(
                inst->transform.m[0][0]*inst->transform.m[0][0] +
                inst->transform.m[0][1]*inst->transform.m[0][1] +
                inst->transform.m[0][2]*inst->transform.m[0][2]);
            const vf32 radius = sft_vf_mul(sft_vf_load(&mesh->bounds_radius[m_ind]), sft_vf_set1(sx));
            cone_axis = sft_vv_norm(cone_axis);

            // frustum and cone culling
            vf32 inside_mask = sft_vf_cast_vi32(sft_vi_set1(0x80000000)); 
            for (i32 p = 0; p < 6; p += 1) {
                const vf32 dist = sft_vf_add(sft_vv_dot(center, sft_state->frustum.normal[p]), sft_state->frustum.d[p]);
                inside_mask = sft_vf_and(inside_mask, sft_vf_cmp_ge(sft_vf_add(dist, radius), v_zero));
            }

            const sftvvec v_raw = sft_vv_sub(center, v_camera_pos);
            const vf32 d = sft_vv_length(v_raw);
            const vf32 dot_val = sft_vv_dot(v_raw, cone_axis);
            const vf32 sin_alpha = sft_vf_load(&mesh->cone_cutoff[m_ind]);
            const vf32 threshold = sft_vf_fmadd(d, sin_alpha, radius);
            inside_mask = sft_vf_and(inside_mask, sft_vf_cmp_le(dot_val, threshold));
            
            const i32 active_meshlets = sft_vf_movemask(inside_mask);
            if (0 == active_meshlets) {
                continue;
            }

            for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                if ((i + lane) >= job->meshlet_count) {
                    break;
                }
                if (0 == (active_meshlets & (1 << lane))) {
                    continue;
                }

                const u32 surviving_m_ind = m_ind + lane; // local to the mesh
                const u32 global_m_ind = mesh->model_meshlet_offset + surviving_m_ind; // global to the model

                const struct sft_meshlet* m_verts = &mesh->meshlets[surviving_m_ind];
                const struct sft_meshlet_attribs* m_attribs = &mesh->meshlet_attribs[surviving_m_ind];
                const struct sft_meshlet_inds* m_inds = &mesh->inds[surviving_m_ind];

                // ss_cache tracks meshlets globally across the instance
                const u32 ss_m_ind = inst->ss_meshlet_offset + global_m_ind;

                // ---  vertex transforms  ---
                for (u32 v_chunk = 0; v_chunk < (SFT_VERTS_PER_MESHLET / SFT_SIMD_WIDTH); v_chunk += 1) {
                    const u32 v_ind = v_chunk * SFT_SIMD_WIDTH;
                    
                    sftvvec v, n, v_world, n_world;
                    v.x = sft_vf_load(&m_verts->x[v_ind]);
                    v.y = sft_vf_load(&m_verts->y[v_ind]);
                    v.z = sft_vf_load(&m_verts->z[v_ind]);
                    n.x = sft_vf_load(&m_attribs->nx[v_ind]);
                    n.y = sft_vf_load(&m_attribs->ny[v_ind]);
                    n.z = sft_vf_load(&m_attribs->nz[v_ind]);

                    // ---  skinning  ---
                    if (mesh->meshlet_skins && inst->posed_bone_matrices) {
                        const struct sft_meshlet_skin* skin = &mesh->meshlet_skins[surviving_m_ind];
                        vi32 j0, j1, j2, j3;
                        vf32 w0, w1, w2, w3;
                        
                        alignas(32) i32 sj0[SFT_SIMD_WIDTH], sj1[SFT_SIMD_WIDTH], sj2[SFT_SIMD_WIDTH], sj3[SFT_SIMD_WIDTH];
                        alignas(32) f32 sw0[SFT_SIMD_WIDTH], sw1[SFT_SIMD_WIDTH], sw2[SFT_SIMD_WIDTH], sw3[SFT_SIMD_WIDTH];
                        const f32 inv255 = 1.f / 255.f;
                        
                        for (u32 l = 0; l < SFT_SIMD_WIDTH; l += 1) {
                            sj0[l] = skin->j0[v_ind + l];
                            sw0[l] = skin->w0[v_ind + l] * inv255;
                            sj1[l] = skin->j1[v_ind + l];
                            sw1[l] = skin->w1[v_ind + l] * inv255;
                            sj2[l] = skin->j2[v_ind + l];
                            sw2[l] = skin->w2[v_ind + l] * inv255;
                            sj3[l] = skin->j3[v_ind + l];
                            sw3[l] = skin->w3[v_ind + l] * inv255;
                        }
                        
                        j0 = sft_vi_load(sj0);
                        w0 = sft_vf_load(sw0);
                        j1 = sft_vi_load(sj1);
                        w1 = sft_vf_load(sw1);
                        j2 = sft_vi_load(sj2);
                        w2 = sft_vf_load(sw2);
                        j3 = sft_vi_load(sj3);
                        w3 = sft_vf_load(sw3);

                        const sftvmat M0 = sft__gather_posed_matrix(inst->posed_bone_matrices, j0);
                        const sftvmat M1 = sft__gather_posed_matrix(inst->posed_bone_matrices, j1);
                        const sftvmat M2 = sft__gather_posed_matrix(inst->posed_bone_matrices, j2);
                        const sftvmat M3 = sft__gather_posed_matrix(inst->posed_bone_matrices, j3);

                        const sftvvec vp0 = sft_vv_transform(v, M0);
                        const sftvvec vn0 = sft_vv_transform_dir(n, M0);
                        const sftvvec vp1 = sft_vv_transform(v, M1);
                        const sftvvec vn1 = sft_vv_transform_dir(n, M1);
                        const sftvvec vp2 = sft_vv_transform(v, M2);
                        const sftvvec vn2 = sft_vv_transform_dir(n, M2);
                        const sftvvec vp3 = sft_vv_transform(v, M3);
                        const sftvvec vn3 = sft_vv_transform_dir(n, M3);
                        
                        v_world.x = sft_vf_fmadd(vp0.x, w0, sft_vf_fmadd(vp1.x, w1, sft_vf_fmadd(vp2.x, w2, sft_vf_mul(vp3.x, w3))));
                        v_world.y = sft_vf_fmadd(vp0.y, w0, sft_vf_fmadd(vp1.y, w1, sft_vf_fmadd(vp2.y, w2, sft_vf_mul(vp3.y, w3))));
                        v_world.z = sft_vf_fmadd(vp0.z, w0, sft_vf_fmadd(vp1.z, w1, sft_vf_fmadd(vp2.z, w2, sft_vf_mul(vp3.z, w3))));

                        n_world.x = sft_vf_fmadd(vn0.x, w0, sft_vf_fmadd(vn1.x, w1, sft_vf_fmadd(vn2.x, w2, sft_vf_mul(vn3.x, w3))));
                        n_world.y = sft_vf_fmadd(vn0.y, w0, sft_vf_fmadd(vn1.y, w1, sft_vf_fmadd(vn2.y, w2, sft_vf_mul(vn3.y, w3))));
                        n_world.z = sft_vf_fmadd(vn0.z, w0, sft_vf_fmadd(vn1.z, w1, sft_vf_fmadd(vn2.z, w2, sft_vf_mul(vn3.z, w3))));
                        
                        v_world = sft_vv_transform(v_world, v_transform);
                        n_world = sft_vv_transform_dir(n_world, v_transform);
                    }
                    else {
                        v_world = sft_vv_transform(v, v_transform);
                        n_world = sft_vv_transform_dir(n, v_transform);
                    }

                    // ---  cache to "g buffer"  ---
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wx[v_ind], v_world.x);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wy[v_ind], v_world.y);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wz[v_ind], v_world.z);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wnx[v_ind], n_world.x);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wny[v_ind], n_world.y);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].wnz[v_ind], n_world.z);

                    // project using pure view_proj (since vertices are now in world space)
                    vf32 w = sft_vf_fmadd(v_world.z, v_vp.m[2][3], v_vp.m[3][3]);
                         w = sft_vf_fmadd(v_world.y, v_vp.m[1][3], w); 
                         w = sft_vf_fmadd(v_world.x, v_vp.m[0][3], w);

                    vf32 tx = sft_vf_fmadd(v_world.z, v_vp.m[2][0], v_vp.m[3][0]);
                         tx = sft_vf_fmadd(v_world.y, v_vp.m[1][0], tx); 
                         tx = sft_vf_fmadd(v_world.x, v_vp.m[0][0], tx);

                    vf32 ty = sft_vf_fmadd(v_world.z, v_vp.m[2][1], v_vp.m[3][1]);
                         ty = sft_vf_fmadd(v_world.y, v_vp.m[1][1], ty); 
                         ty = sft_vf_fmadd(v_world.x, v_vp.m[0][1], ty);

                    vf32 tz = sft_vf_fmadd(v_world.z, v_vp.m[2][2], v_vp.m[3][2]);
                         tz = sft_vf_fmadd(v_world.y, v_vp.m[1][2], tz); 
                         tz = sft_vf_fmadd(v_world.x, v_vp.m[0][2], tz);

                    const vf32 approx_w = sft_vf_rcp_refined(w);
                    const vf32 inv_w = sft_vf_mul(approx_w, sft_vf_fnmadd(w, approx_w, v_two));
                    const vf32 sx = sft_vf_fmadd(tx, sft_vf_mul(inv_w, v_scale_x), v_bias_x);
                    const vf32 sy = sft_vf_fmadd(ty, sft_vf_mul(inv_w, v_scale_y), v_bias_y);

                    sft_vf_store(&clip_tx[v_ind], tx);
                    sft_vf_store(&clip_ty[v_ind], ty);
                    sft_vf_store(&clip_tz[v_ind], tz);
                    sft_vf_store(&clip_w[v_ind], w);
                    sft_vf_store(&scratch_sx[v_ind], sx);
                    sft_vf_store(&scratch_sy[v_ind], sy);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].sx[v_ind], sx);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].sy[v_ind], sy);
                    sft_vf_store(&sft_state->ss_cache[ss_m_ind].inv_w[v_ind], inv_w);
                }

                const u32 batch_count = 128 / SFT_SIMD_WIDTH;
                for (u32 batch = 0; batch < batch_count; batch += 1) {
                    sft__geom_setup_and_bin_batch(
                        batch, job->instance_ind, global_m_ind, m_inds,
                        clip_tx, clip_ty, clip_tz, clip_w,
                        scratch_sx, scratch_sy,
                        local_bins, &t_chunk_base, &t_chunk_count, &t_clip_base, &t_clip_count);
                }
            }
        }
    }

    // ---  flush payload counts to global array  ---
    const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;
    for (i32 i = 0; i < total_tiles; i += 1) {
        if (local_bins[i].payload_count > 0) {
            sft_atomic_add(&sft_state->display.tile_payload_counts[i], local_bins[i].payload_count);
        }
    }
}

// handles both regular and clipped triangles
static SFT_INLINE void sft__rast_simd_batch(
    u64 payload, f32* depth_buf, u32* vid_buf,
    i32 tile_min_x, i32 tile_min_y,
    vf32 v_x_step, vf32 v_zero, vf32 v_tile_min_x, vf32 v_tile_max_x, vf32 v_tile_min_y, vf32 v_tile_max_y
) {
    const u32 raw_active = (u32)((payload >> 56) & 0xFF);
    
    vf32 sx0, sy0, inv_w0;
    vf32 sx1, sy1, inv_w1;
    vf32 sx2, sy2, inv_w2;
    
    u32 active_tris = 0;
    u8 is_clipped = 0;
    u32 inst_ind = 0, m_ind = 0, batch = 0, clip_slot = 0;

    if (0 == raw_active) {
        // ---  clipped triangle fetch  ---
        is_clipped = 1;
        clip_slot = (u32)(payload & 0xFFFFFFFF);
        const struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[clip_slot];

        sx0 = sft_vf_set1(ct->sx[0]), sy0 = sft_vf_set1(ct->sy[0]), inv_w0 = sft_vf_set1(ct->inv_w[0]);
        sx1 = sft_vf_set1(ct->sx[1]), sy1 = sft_vf_set1(ct->sy[1]), inv_w1 = sft_vf_set1(ct->inv_w[1]);
        sx2 = sft_vf_set1(ct->sx[2]), sy2 = sft_vf_set1(ct->sy[2]), inv_w2 = sft_vf_set1(ct->inv_w[2]);
        
        active_tris = 1; // lane 0 only
    } 
    else {
        // ---  standard batch fetch  ---
        inst_ind = (u32)(payload & 0xFFFFFF);
        m_ind    = (u32)((payload >> 24) & 0xFFFFFF);
        batch    = (u32)((payload >> 48) & 0xFF);
        active_tris = raw_active;

        const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
        
        // find the parent mesh and calculate the local meshlet ind
        const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
        const sft_mesh* mesh = &inst->model->meshes[mesh_ind];
        const u32 local_m_ind = m_ind - mesh->model_meshlet_offset;

        const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];
        const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
        
        const vi32 v_i0 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (0 * SFT_SIMD_WIDTH)]);
        const vi32 v_i1 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (1 * SFT_SIMD_WIDTH)]);
        const vi32 v_i2 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (2 * SFT_SIMD_WIDTH)]);

        const u32 ss_m_ind = inst->ss_meshlet_offset + m_ind;
        
        sx0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i0);
        sy0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i0);
        inv_w0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i0);

        sx1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i1);
        sy1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i1);
        inv_w1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i1);

        sx2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i2);
        sy2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i2);
        inv_w2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i2);
    }

    const vf32 A0 = sft_vf_sub(sy0, sy1), B0 = sft_vf_sub(sx1, sx0);
    const vf32 A1 = sft_vf_sub(sy1, sy2), B1 = sft_vf_sub(sx2, sx1);
    const vf32 A2 = sft_vf_sub(sy2, sy0), B2 = sft_vf_sub(sx0, sx2);

    // symmetric C0 for tight edges with aggressive FMA
    const vf32 m01 = sft_vf_mul(sx0, sy1), m10 = sft_vf_mul(sy0, sx1);
    const vf32 C0 = sft_vf_mul(
        sft_vf_sub(sft_vf_fmsub(sx0, sy1, m10), sft_vf_fmsub(sy0, sx1, m01)), sft_vf_set1(0.5f));

    const vf32 m12 = sft_vf_mul(sx1, sy2), m21 = sft_vf_mul(sy1, sx2);
    const vf32 C1 = sft_vf_mul(
        sft_vf_sub(sft_vf_fmsub(sx1, sy2, m21), sft_vf_fmsub(sy1, sx2, m12)), sft_vf_set1(0.5f));

    const vf32 m20 = sft_vf_mul(sx2, sy0), m02 = sft_vf_mul(sy2, sx0);
    const vf32 C2 = sft_vf_mul(
        sft_vf_sub(sft_vf_fmsub(sx2, sy0, m02), sft_vf_fmsub(sy2, sx0, m20)), sft_vf_set1(0.5f));

    const vf32 dx1 = sft_vf_sub(sx1, sx0), dx2 = sft_vf_sub(sx2, sx0);
    const vf32 dy1 = sft_vf_sub(sy1, sy0), dy2 = sft_vf_sub(sy2, sy0);
    
    // area culling
    const vf32 det = sft_vf_sub(sft_vf_mul(dx1, dy2), sft_vf_mul(dx2, dy1));
    const vf32 valid_area_mask = sft_vf_cmp_gt(det, sft_vf_set1(SFT_EPSILON));
    active_tris &= sft_vf_movemask(valid_area_mask);

    if (0 == active_tris) {
        return;
    }

    const vf32 inv_det = sft_vf_rcp_refined(det);
    const vf32 dzdx = sft_vf_mul(sft_vf_sub(sft_vf_mul(sft_vf_sub(inv_w1, inv_w0), dy2), sft_vf_mul(sft_vf_sub(inv_w2, inv_w0), dy1)), inv_det);
    const vf32 dzdy = sft_vf_mul(sft_vf_sub(sft_vf_mul(sft_vf_sub(inv_w2, inv_w0), dx1), sft_vf_mul(sft_vf_sub(inv_w1, inv_w0), dx2)), inv_det);

    const vf32 half = sft_vf_set1(0.5f);
    const vf32 c0_abs = sft_vf_fmadd(B0, half, sft_vf_fmadd(A0, half, C0));
    const vf32 c1_abs = sft_vf_fmadd(B1, half, sft_vf_fmadd(A1, half, C1));
    const vf32 c2_abs = sft_vf_fmadd(B2, half, sft_vf_fmadd(A2, half, C2));
    const vf32 z_base_abs = sft_vf_fmadd(sft_vf_sub(half, sy0), dzdy, sft_vf_fmadd(sft_vf_sub(half, sx0), dzdx, inv_w0));

    vf32 v_min_x = sft_vf_min(sx0, sft_vf_min(sx1, sx2));
    vf32 v_max_x = sft_vf_max(sx0, sft_vf_max(sx1, sx2));
    vf32 v_min_y = sft_vf_min(sy0, sft_vf_min(sy1, sy2));
    vf32 v_max_y = sft_vf_max(sy0, sft_vf_max(sy1, sy2));
    v_min_x = sft_vf_max(v_min_x, v_tile_min_x);
    v_max_x = sft_vf_min(v_max_x, v_tile_max_x);
    v_min_y = sft_vf_max(v_min_y, v_tile_min_y);
    v_max_y = sft_vf_min(v_max_y, v_tile_max_y);

    alignas(32) f32 s_A0[8], s_B0[8], s_c0_abs[8];
    alignas(32) f32 s_A1[8], s_B1[8], s_c1_abs[8];
    alignas(32) f32 s_A2[8], s_B2[8], s_c2_abs[8];
    alignas(32) f32 s_dzdx[8], s_dzdy[8], s_z_base_abs[8];
    alignas(32) i32 s_min_x[8], s_max_x[8], s_min_y[8], s_max_y[8];

    sft_vf_store(s_A0, A0), sft_vf_store(s_B0, B0), sft_vf_store(s_c0_abs, c0_abs);
    sft_vf_store(s_A1, A1), sft_vf_store(s_B1, B1), sft_vf_store(s_c1_abs, c1_abs);
    sft_vf_store(s_A2, A2), sft_vf_store(s_B2, B2), sft_vf_store(s_c2_abs, c2_abs);
    sft_vf_store(s_dzdx, dzdx), sft_vf_store(s_dzdy, dzdy), sft_vf_store(s_z_base_abs, z_base_abs);
    
    sft_vi_store(s_min_x, sft_vi_cvttps_epi32(v_min_x));
    sft_vi_store(s_max_x, sft_vi_cvttps_epi32(v_max_x));
    sft_vi_store(s_min_y, sft_vi_cvttps_epi32(v_min_y));
    sft_vi_store(s_max_y, sft_vi_cvttps_epi32(v_max_y));

    while (active_tris) {
        const u32 lane = sft__tzcnt(active_tris);
        active_tris &= (active_tris - 1);
        
        u32 vid_payload_scalar;
        if (is_clipped) {
            vid_payload_scalar = clip_slot | (1 << 31);
        }
        else {
            vid_payload_scalar = (inst_ind << (SFT_TRIANGLE_BITS + SFT_MESHLET_BITS)) | 
                                 (m_ind << SFT_TRIANGLE_BITS) | 
                                 ((batch * SFT_SIMD_WIDTH) + lane);
        }
        const vf32 v_payload = sft_vf_cast_vi32(sft_vi_set1(vid_payload_scalar));

        const i32 min_x = s_min_x[lane];
        const i32 max_x = s_max_x[lane];
        const i32 min_y = s_min_y[lane];
        const i32 max_y = s_max_y[lane];

        // if bounding box is inverted (completely off tile)
        if (min_x > max_x || min_y > max_y) {
            continue;
        }

        const f32 lane_A0 = s_A0[lane], lane_B0 = s_B0[lane], lane_c0 = s_c0_abs[lane];
        const f32 lane_A1 = s_A1[lane], lane_B1 = s_B1[lane], lane_c1 = s_c1_abs[lane];
        const f32 lane_A2 = s_A2[lane], lane_B2 = s_B2[lane], lane_c2 = s_c2_abs[lane];
        const f32 lane_dzdx = s_dzdx[lane], lane_dzdy = s_dzdy[lane], lane_z_base = s_z_base_abs[lane];

        const i32 aligned_min_x = min_x & ~(SFT_SIMD_WIDTH - 1);
        const i32 aligned_max_x = (max_x & ~(SFT_SIMD_WIDTH - 1)) + SFT_SIMD_WIDTH;

        const f32 start_e0 = lane_c0 + lane_A0 * (f32)aligned_min_x + lane_B0 * (f32)min_y;
        const f32 start_e1 = lane_c1 + lane_A1 * (f32)aligned_min_x + lane_B1 * (f32)min_y;
        const f32 start_e2 = lane_c2 + lane_A2 * (f32)aligned_min_x + lane_B2 * (f32)min_y;
        const f32 start_z  = lane_z_base + lane_dzdx * (f32)aligned_min_x + lane_dzdy * (f32)min_y;

        // ---  trivial accept  ---
        const f32 box_w = (f32)(aligned_max_x - 1 - aligned_min_x);
        const f32 box_h = (f32)(max_y - min_y);

        // absolute minimum value for each edge inside this specific bounding box
        const f32 min_e0 = start_e0 + (lane_A0 < 0.f ? lane_A0 * box_w : 0.f) + (lane_B0 < 0.f ? lane_B0 * box_h : 0.f);
        const f32 min_e1 = start_e1 + (lane_A1 < 0.f ? lane_A1 * box_w : 0.f) + (lane_B1 < 0.f ? lane_B1 * box_h : 0.f);
        const f32 min_e2 = start_e2 + (lane_A2 < 0.f ? lane_A2 * box_w : 0.f) + (lane_B2 < 0.f ? lane_B2 * box_h : 0.f);

        const i32 trivial_accept = (min_e0 >= 0.f && min_e1 >= 0.f && min_e2 >= 0.f);

        const vf32 v_dzdx_chunk = sft_vf_set1(lane_dzdx * SFT_SIMD_WIDTH);
        const vf32 v_dzdx_step = sft_vf_mul(sft_vf_set1(lane_dzdx), v_x_step);
        const vf32 v_dzdy = sft_vf_set1(lane_dzdy);
        vf32 v_z_row = sft_vf_add(sft_vf_set1(start_z), v_dzdx_step);

        if (trivial_accept) {
            // ---  fast path, Z only  ---
            for (i32 y = min_y; y <= max_y; y += 1) {
                vf32 v_z = v_z_row;
                i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (aligned_min_x - tile_min_x);

                for (i32 x = aligned_min_x; x < aligned_max_x; x += SFT_SIMD_WIDTH, p_ind += SFT_SIMD_WIDTH) {
                    const vf32 old_z = sft_vf_load(&depth_buf[p_ind]);
                    const vf32 final_mask = sft_vf_cmp_gt(v_z, old_z);
                    
                    const i32 m_int = sft_vf_movemask(final_mask);
                    if (((1 << SFT_SIMD_WIDTH) - 1) == m_int) {
                        sft_vf_store(&depth_buf[p_ind], v_z);
                        sft_vf_store((f32*)&vid_buf[p_ind], v_payload); 
                    }
                    else if (m_int) {
                        sft_vf_store(&depth_buf[p_ind], sft_vf_blendv(v_z, old_z, final_mask));
                        const vf32 old_p = sft_vf_load((f32*)&vid_buf[p_ind]);
                        sft_vf_store((f32*)&vid_buf[p_ind], sft_vf_blendv(v_payload, old_p, final_mask));
                    }
                    v_z = sft_vf_add(v_z, v_dzdx_chunk);
                }
                v_z_row = sft_vf_add(v_z_row, v_dzdy);
            }
        } 
        else {
            // ---  standard path, edge equations  ---
            const vf32 v_A0_chunk = sft_vf_set1(lane_A0 * SFT_SIMD_WIDTH);
            const vf32 v_A1_chunk = sft_vf_set1(lane_A1 * SFT_SIMD_WIDTH);
            const vf32 v_A2_chunk = sft_vf_set1(lane_A2 * SFT_SIMD_WIDTH);
            
            const vf32 v_B0 = sft_vf_set1(lane_B0);
            const vf32 v_B1 = sft_vf_set1(lane_B1);
            const vf32 v_B2 = sft_vf_set1(lane_B2);

            const vf32 v_A0_step = sft_vf_mul(sft_vf_set1(lane_A0), v_x_step);
            const vf32 v_A1_step = sft_vf_mul(sft_vf_set1(lane_A1), v_x_step);
            const vf32 v_A2_step = sft_vf_mul(sft_vf_set1(lane_A2), v_x_step);

            vf32 v_e0_row = sft_vf_add(sft_vf_set1(start_e0), v_A0_step);
            vf32 v_e1_row = sft_vf_add(sft_vf_set1(start_e1), v_A1_step);
            vf32 v_e2_row = sft_vf_add(sft_vf_set1(start_e2), v_A2_step);

            for (i32 y = min_y; y <= max_y; y += 1) {
                vf32 v_e0 = v_e0_row;
                vf32 v_e1 = v_e1_row;
                vf32 v_e2 = v_e2_row;
                vf32 v_z  = v_z_row;
                
                i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (aligned_min_x - tile_min_x);

                for (i32 x = aligned_min_x; x < aligned_max_x; x += SFT_SIMD_WIDTH, p_ind += SFT_SIMD_WIDTH) {
                    const vf32 geom_mask = sft_vf_and(sft_vf_cmp_ge(v_e0, v_zero), 
                                           sft_vf_and(sft_vf_cmp_ge(v_e1, v_zero), sft_vf_cmp_ge(v_e2, v_zero)));
                    
                    if (sft_vf_movemask(geom_mask)) {
                        const vf32 old_z = sft_vf_load(&depth_buf[p_ind]);
                        const vf32 final_mask = sft_vf_and(geom_mask, sft_vf_cmp_gt(v_z, old_z));
                        
                        const i32 m_int = sft_vf_movemask(final_mask);
                        if (((1 << SFT_SIMD_WIDTH) - 1) == m_int) {
                            sft_vf_store(&depth_buf[p_ind], v_z);
                            sft_vf_store((f32*)&vid_buf[p_ind], v_payload); 
                        }
                        else if (m_int) {
                            sft_vf_store(&depth_buf[p_ind], sft_vf_blendv(v_z, old_z, final_mask));
                            const vf32 old_p = sft_vf_load((f32*)&vid_buf[p_ind]);
                            sft_vf_store((f32*)&vid_buf[p_ind], sft_vf_blendv(v_payload, old_p, final_mask));
                        }
                    }
                    
                    v_e0 = sft_vf_add(v_e0, v_A0_chunk);
                    v_e1 = sft_vf_add(v_e1, v_A1_chunk);
                    v_e2 = sft_vf_add(v_e2, v_A2_chunk);
                    v_z  = sft_vf_add(v_z, v_dzdx_chunk);
                }
                
                v_e0_row = sft_vf_add(v_e0_row, v_B0);
                v_e1_row = sft_vf_add(v_e1_row, v_B1);
                v_e2_row = sft_vf_add(v_e2_row, v_B2);
                v_z_row  = sft_vf_add(v_z_row, v_dzdy);
            }
        }
    }
}

static SFT_INLINE i32 sft__calculate_lod(
    sft_tex_handle tex_handle,
    vf32 ddx_u, vf32 ddy_u, vf32 ddx_v, vf32 ddy_v,
    vi32 v_tri_lanes, i32 block_x, i32 block_y
) {
    if (SFT_INVALID_HANDLE == tex_handle) {
        return 0;
    }

    const u32 t_ind = SFT_HANDLE_INDEX(tex_handle);
    const i32 tex_w = sft_state->tex_pool.w[t_ind];
    const i32 tex_h = sft_state->tex_pool.h[t_ind];

    // ---  calculate scalar LOD  ---
    // extract the first active lane's gradients to compute a single LOD for the block
    alignas(32) f32 s_du_x[SFT_SIMD_WIDTH], s_du_y[SFT_SIMD_WIDTH];
    alignas(32) f32 s_dv_x[SFT_SIMD_WIDTH], s_dv_y[SFT_SIMD_WIDTH];
    sft_vf_store(s_du_x, ddx_u); sft_vf_store(s_du_y, ddy_u);
    sft_vf_store(s_dv_x, ddx_v); sft_vf_store(s_dv_y, ddy_v);
    
    const i32 first_lane = sft__tzcnt(sft_vi_movemask(v_tri_lanes));
    const f32 ux = s_du_x[first_lane] * tex_w, vx = s_dv_x[first_lane] * tex_h;
    const f32 uy = s_du_y[first_lane] * tex_w, vy = s_dv_y[first_lane] * tex_h;
    
    const f32 rho_max = SFT_MAX(SFT_MAX(fabsf(ux), fabsf(vx)), SFT_MAX(fabsf(uy), fabsf(vy)));
    
    i32 lod = 0;
    if (rho_max > 1.f) {
        // + ~20 FPS in Sponza vs using log2f
        const f32 lod_f = sft__fast_log2(rho_max); 
        
        // 2x2 bayer mat for ss dithering
        const f32 bayer[4] = { -0.375f, 0.125f, 0.375f, -0.125f };
        const i32 b_ind = ((block_y & 1) << 1) | (block_x & 1);
        
        // add dither and round to nearest int mip level
        lod = (i32)(lod_f + 0.5f + bayer[b_ind]);
    }

    return SFT_CLAMP(lod, 0, sft_state->tex_pool.mip_levels[t_ind] - 1);
}

static SFT_INLINE void sft__sample_tex_bilinear(
    sft_tex_handle tex_handle, i32 lod,
    vf32 cu, vf32 cv,
    vf32* out_r, vf32* out_g, vf32* out_b
) {
    if (SFT_INVALID_HANDLE == tex_handle) {
        *out_r = sft_vf_set1(1.f);
        *out_g = sft_vf_set1(1.f);
        *out_b = sft_vf_set1(1.f);
        return;
    }

    const u32 t_ind = SFT_HANDLE_INDEX(tex_handle);
    const i32 tex_w = sft_state->tex_pool.w[t_ind];
    const i32 tex_h = sft_state->tex_pool.h[t_ind];

    // ---  setup mipmap data  ---
    const i32 mip_ind = t_ind * 14 + lod;
    const u32 mip_offset = sft_state->tex_pool.mip_offsets[mip_ind];
    const u32* mip_pixels = (const u32*)(sft_state->tex_heap_arena.base + mip_offset);
    
    const i32 curr_w = SFT_MAX(tex_w >> lod, 1);
    const i32 curr_h = SFT_MAX(tex_h >> lod, 1);

    // ---  UV wrapping and fractional math  ---
    // scale continuous normalized UV directly to pixel coords, offset by -0.5 for bilinear
    const vf32 u_px = sft_vf_sub(sft_vf_mul(cu, sft_vf_set1((f32)curr_w)), sft_vf_set1(0.5f));
    const vf32 v_px = sft_vf_sub(sft_vf_mul(cv, sft_vf_set1((f32)curr_h)), sft_vf_set1(0.5f));

    // fractional parts for blending
    const vf32 u_floor = sft_vf_floor(u_px);
    const vf32 v_floor = sft_vf_floor(v_px);
    const vf32 u_frac = sft_vf_sub(u_px, u_floor);
    const vf32 v_frac = sft_vf_sub(v_px, v_floor);

    // integer coords for the 4 corners
    const vi32 u0 = sft_vi_cvttps_epi32(u_floor);
    const vi32 v0 = sft_vi_cvttps_epi32(v_floor);
    
    const u8 is_pot = sft_state->tex_pool.is_pot[t_ind];
    const vi32 w_max = sft_vi_set1(curr_w - 1);
    const vi32 h_max = sft_vi_set1(curr_h - 1);
    
    vi32 u0_wrap, v0_wrap, u1, v1;
    if (is_pot) {
        // fast bitwise wrapping
        u1 = sft_vi_and(sft_vi_add(u0, sft_vi_set1(1)), w_max);
        v1 = sft_vi_and(sft_vi_add(v0, sft_vi_set1(1)), h_max);
        u0_wrap = sft_vi_and(u0, w_max);
        v0_wrap = sft_vi_and(v0, h_max);
    }
    else {
        // clamp to edges
        u1 = sft_vi_min(sft_vi_add(u0, sft_vi_set1(1)), w_max);
        v1 = sft_vi_min(sft_vi_add(v0, sft_vi_set1(1)), h_max);
        u0_wrap = sft_vi_min(u0, w_max);
        v0_wrap = sft_vi_min(v0, h_max);
    }

    // ---  swizzled 4x4 address calculation  ---
    vi32 addr_00, addr_10, addr_01, addr_11;
    if (is_pot) {
        const i32 stride_shift = sft_state->tex_pool.mip_stride_shift[mip_ind];
        #define CALC_ADDR_POT(vx, vy) \
            sft_vi_add( \
                sft_vi_slli(sft_vi_add(sft_vi_slli(sft_vi_srli(vy, 2), stride_shift), sft_vi_srli(vx, 2)), 4), \
                sft_vi_add(sft_vi_slli(sft_vi_and(vy, sft_vi_set1(3)), 2), sft_vi_and(vx, sft_vi_set1(3))) \
            )
        addr_00 = CALC_ADDR_POT(u0_wrap, v0_wrap);
        addr_10 = CALC_ADDR_POT(u1, v0_wrap);
        addr_01 = CALC_ADDR_POT(u0_wrap, v1);
        addr_11 = CALC_ADDR_POT(u1, v1);
        #undef CALC_ADDR_POT
    }
    else {
        const vi32 bw = sft_vi_set1(sft_state->tex_pool.mip_blocks_w[mip_ind]);
        #define CALC_ADDR_NPOT(vx, vy) \
            sft_vi_add( \
                sft_vi_slli(sft_vi_add(sft_vi_mullo(sft_vi_srli(vy, 2), bw), sft_vi_srli(vx, 2)), 4), \
                sft_vi_add(sft_vi_slli(sft_vi_and(vy, sft_vi_set1(3)), 2), sft_vi_and(vx, sft_vi_set1(3))) \
            )
        addr_00 = CALC_ADDR_NPOT(u0_wrap, v0_wrap);
        addr_10 = CALC_ADDR_NPOT(u1, v0_wrap);
        addr_01 = CALC_ADDR_NPOT(u0_wrap, v1);
        addr_11 = CALC_ADDR_NPOT(u1, v1);
        #undef CALC_ADDR_NPOT
    }

    // ---  gather and interpolate  ---
    // gather 4 texels (TL, TR, BL, BR)
    const vi32 c00 = sft_vi_gather((const int*)mip_pixels, addr_00);
    const vi32 c10 = sft_vi_gather((const int*)mip_pixels, addr_10);
    const vi32 c01 = sft_vi_gather((const int*)mip_pixels, addr_01);
    const vi32 c11 = sft_vi_gather((const int*)mip_pixels, addr_11);

    const vf32 i255 = sft_vf_set1(1.f / 255.f);
    const vi32 mask_8 = sft_vi_set1(0xFF);

    #define BLEND_CHANNEL(shift) { \
        const vf32 f00 = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c00, shift), mask_8)), i255); \
        const vf32 f10 = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c10, shift), mask_8)), i255); \
        const vf32 f01 = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c01, shift), mask_8)), i255); \
        const vf32 f11 = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c11, shift), mask_8)), i255); \
        const vf32 top = sft_vf_add(f00, sft_vf_mul(sft_vf_sub(f10, f00), u_frac)); \
        const vf32 bot = sft_vf_add(f01, sft_vf_mul(sft_vf_sub(f11, f01), u_frac)); \
        out_##shift = sft_vf_add(top, sft_vf_mul(sft_vf_sub(bot, top), v_frac)); \
    }

    vf32 out_16, out_8, out_0; // R, G, B
    BLEND_CHANNEL(16);
    BLEND_CHANNEL(8);
    BLEND_CHANNEL(0);
    #undef BLEND_CHANNEL

    *out_r = out_16;
    *out_g = out_8;
    *out_b = out_0;
}

static SFT_INLINE void sft__sample_tex_nearest(
    sft_tex_handle tex_handle, i32 lod,
    vf32 cu, vf32 cv,
    vf32* out_r, vf32* out_g, vf32* out_b
) {
    if (SFT_INVALID_HANDLE == tex_handle) {
        *out_r = sft_vf_set1(1.f);
        *out_g = sft_vf_set1(1.f);
        *out_b = sft_vf_set1(1.f);
        return;
    }

    const u32 t_ind = SFT_HANDLE_INDEX(tex_handle);
    const i32 mip_ind = t_ind * 14 + lod;
    const u32 mip_offset = sft_state->tex_pool.mip_offsets[mip_ind];
    const u32* mip_pixels = (const u32*)(sft_state->tex_heap_arena.base + mip_offset);
    
    const i32 curr_w = SFT_MAX(sft_state->tex_pool.w[t_ind] >> lod, 1);
    const i32 curr_h = SFT_MAX(sft_state->tex_pool.h[t_ind] >> lod, 1);

    // scale UV directly to pixel coords
    const vf32 u_px = sft_vf_mul(cu, sft_vf_set1((f32)curr_w));
    const vf32 v_px = sft_vf_mul(cv, sft_vf_set1((f32)curr_h));

    // floor before truncating to properly handle negative wrapped UVs
    const vi32 u0 = sft_vi_cvttps_epi32(sft_vf_floor(u_px));
    const vi32 v0 = sft_vi_cvttps_epi32(sft_vf_floor(v_px));
    
    const u8 is_pot = sft_state->tex_pool.is_pot[t_ind];
    const vi32 w_max = sft_vi_set1(curr_w - 1);
    const vi32 h_max = sft_vi_set1(curr_h - 1);
    
    vi32 u0_wrap, v0_wrap;
    if (is_pot) {
        u0_wrap = sft_vi_and(u0, w_max);
        v0_wrap = sft_vi_and(v0, h_max);
    }
    else {
        u0_wrap = sft_vi_min(u0, w_max);
        v0_wrap = sft_vi_min(v0, h_max);
    }

    // address calculation
    vi32 addr_00;
    if (is_pot) {
        const i32 stride_shift = sft_state->tex_pool.mip_stride_shift[mip_ind];
        addr_00 = sft_vi_add(
            sft_vi_slli(sft_vi_add(sft_vi_slli(sft_vi_srli(v0_wrap, 2), stride_shift), sft_vi_srli(u0_wrap, 2)), 4),
            sft_vi_add(sft_vi_slli(sft_vi_and(v0_wrap, sft_vi_set1(3)), 2), sft_vi_and(u0_wrap, sft_vi_set1(3)))
        );
    }
    else {
        const vi32 bw = sft_vi_set1(sft_state->tex_pool.mip_blocks_w[mip_ind]);
        addr_00 = sft_vi_add(
            sft_vi_slli(sft_vi_add(sft_vi_mullo(sft_vi_srli(v0_wrap, 2), bw), sft_vi_srli(u0_wrap, 2)), 4),
            sft_vi_add(sft_vi_slli(sft_vi_and(v0_wrap, sft_vi_set1(3)), 2), sft_vi_and(u0_wrap, sft_vi_set1(3)))
        );
    }

    // one texel per lane
    const vi32 c00 = sft_vi_gather((const int*)mip_pixels, addr_00);
    const vf32 i255 = sft_vf_set1(1.f / 255.f);
    const vi32 mask_8 = sft_vi_set1(0xFF);

    *out_r = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c00, 16), mask_8)), i255);
    *out_g = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(sft_vi_srli(c00, 8), mask_8)), i255);
    *out_b = sft_vf_mul(sft_vf_cvtepi32(sft_vi_and(c00, mask_8)), i255);
}

static SFT_INLINE void sft__debug_draw_tile_overlay(
    i32 tile_min_x, i32 tile_min_y, 
    i32 limit_x, i32 limit_y, 
    i32 screen_w, f32 tile_min_z, f32 tile_max_z, 
    i32 tile_light_count,
    const f32* depth_buf
) {
    switch (sft_state->display.debug_mode) {
        case SFT_DEBUG_TILES: {
            for (i32 y = tile_min_y; y < limit_y; y += 1) {
                for (i32 x = tile_min_x; x < limit_x; x += 1) {
                    const i32 pixel_ind = y * screen_w + x;
                    // outer tile border
                    if (x == tile_min_x || x == limit_x - 1 || y == tile_min_y || y == limit_y - 1) {
                        sft_state->display.color_buffer[pixel_ind] = 0xFF00FF00;
                    }
                    // 16x16 sub tile grid
                    else if ((x % 16 == 0) || (y % 16 == 0)) {
                        const u32 bg = sft_state->display.color_buffer[pixel_ind];
                        const u32 overlay = 0xFF004400;
                        sft_state->display.color_buffer[pixel_ind] = ((bg & 0xFEFEFEFE) >> 1) + ((overlay & 0xFEFEFEFE) >> 1);
                    }
                }
            }
        } break;

        case SFT_DEBUG_DEPTH: {
            f32 depth_delta = 0.f;
            if (tile_min_z <= tile_max_z && tile_min_z > SFT_EPSILON) {
                f32 near_w = 1.f / tile_max_z;
                f32 far_w  = 1.f / tile_min_z;
                depth_delta = far_w - near_w;
            }

            const u32 border_color = (depth_delta > 16.f) ? 0xFFFF0000 : 0xFF00FF00;

            for (i32 y = tile_min_y; y < limit_y; y += 1) {
                for (i32 x = tile_min_x; x < limit_x; x += 1) {
                    const i32 pixel_ind = y * screen_w + x;

                    if (x == tile_min_x || x == limit_x - 1 || y == tile_min_y || y == limit_y - 1) {
                        sft_state->display.color_buffer[pixel_ind] = border_color;
                    }
                    else {
                        const i32 local_p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (x - tile_min_x);
                        const f32 inv_w = depth_buf[local_p_ind];

                        if (inv_w <= SFT_EPSILON) {
                            continue;
                        }

                        const f32 linear_z = 1.f / inv_w;
                        const f32 norm_z = SFT_CLAMP(linear_z / 100.f, 0.f, 1.f); 
                        const u32 intensity = (u32)(norm_z * 255.f);
                        sft_state->display.color_buffer[pixel_ind] = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
                    }
                }
            }
        } break;

        case SFT_DEBUG_LIGHT_COUNT: {
            const u32 heat_color = (0 == tile_light_count) ? 0xFF222288 :
                (tile_light_count < 4) ? 0xFF00FF00 : 0xFFFF0000;

            for (i32 y = tile_min_y; y < limit_y; y += 1) {
                for (i32 x = tile_min_x; x < limit_x; x += 1) {
                    const i32 pixel_ind = y * screen_w + x;
                    const u32 bg = sft_state->display.color_buffer[pixel_ind];
                    // 50% blend for a translucent heatmap overlay
                    sft_state->display.color_buffer[pixel_ind] = ((bg & 0xFEFEFEFE) >> 1) + ((heat_color & 0xFEFEFEFE) >> 1);
                }
            }
        } break;
        
        default: break;
    }
}

static SFT_INLINE void sft__resolve_tile(
    const f32* depth_buf, const u32* vid_buf,
    i32 tile_min_x, i32 tile_min_y
) {
    const i32 screen_w = (i32)(sft_state->display.half_width * 2.f);
    const i32 screen_h = (i32)(sft_state->display.half_height * 2.f);
    const i32 pitch = sft_state->display.color_pitch;

    const i32 limit_x = SFT_MIN(tile_min_x + SFT_TILE_SIZE, screen_w);
    const i32 limit_y = SFT_MIN(tile_min_y + SFT_TILE_SIZE, screen_h);

    vf32 v_tile_min = sft_vf_set1(1e9f);
    vf32 v_tile_max = sft_vf_set1(-1e9f);
    const vf32 v_depth_threshold = sft_vf_set1(0.0001f);
    
    // SFT_TILE_SIZE^2 pixels in SFT_SIMD_WIDTH steps
    for (i32 i = 0; i < SFT_TILE_SIZE * SFT_TILE_SIZE; i += SFT_SIMD_WIDTH) {
        const vf32 d = sft_vf_load(&depth_buf[i]);
        
        // mask out the empty background pixels (where depth is 0.0)
        const vf32 valid_mask = sft_vf_cmp_gt(d, v_depth_threshold);
        
        if (sft_vf_movemask(valid_mask)) {
            // blend in extreme values for invalid lanes so they don't corrupt the min/max
            const vf32 safe_d_min = sft_vf_blendv(d, sft_vf_set1(1e9f), valid_mask);
            const vf32 safe_d_max = sft_vf_blendv(d, sft_vf_set1(-1e9f), valid_mask);
            
            v_tile_min = sft_vf_min(v_tile_min, safe_d_min);
            v_tile_max = sft_vf_max(v_tile_max, safe_d_max);
        }
    }

    // collapse the registers into scalar values
    f32 tile_min_z = 1e9f;
    f32 tile_max_z = -1e9f;
    
    alignas(32) f32 s_min[SFT_SIMD_WIDTH];
    alignas(32) f32 s_max[SFT_SIMD_WIDTH];
    sft_vf_store(s_min, v_tile_min);
    sft_vf_store(s_max, v_tile_max);
    
    for (i32 i = 0; i < SFT_SIMD_WIDTH; i += 1) {
        if (s_min[i] < tile_min_z) tile_min_z = s_min[i];
        if (s_max[i] > tile_max_z) tile_max_z = s_max[i];
    }

    // ---  tile light culling  ---
    const i32 total_point_lights = sft_state->lighting.point_count.count;
    u16 tile_lights[16]; // TODO remove arbitrary max
    i32 tile_light_count = 0;

    if (tile_min_z <= tile_max_z && total_point_lights > 0) {
        const sftmat* vp = &sft_state->view_proj;
        const f32 hw = sft_state->display.half_width;
        const f32 hh = sft_state->display.half_height;
        const f32 vpx = sft_state->display.vp_x;
        const f32 vpy = sft_state->display.vp_y;

        for (i32 l = 0; l < total_point_lights; l += 1) {
            const sftvec pos = sft_state->lighting.point[l].pos;
            const f32 radius = sft_state->lighting.point[l].radius;

            // the light's linear w
            const f32 w = pos.x * vp->m[0][3] + pos.y * vp->m[1][3] + pos.z * vp->m[2][3] + vp->m[3][3];

            // linear bounds
            const f32 linear_min = w - radius;
            const f32 linear_max = w + radius;

            // if the light's volume clips behind the camera
            const f32 safe_linear_min = (linear_min > SFT_EPSILON) ? linear_min : SFT_EPSILON;

            // convert to 1/w space (min and max swap because of the inversion)
            const f32 light_inv_w_min = 1.f / linear_max;
            const f32 light_inv_w_max = 1.f / safe_linear_min;

            // Z cull
            if (light_inv_w_max < tile_min_z || light_inv_w_min > tile_max_z) {
                continue; 
            }

            // screen space 2D bounds check
            if (w > radius) { 
                const f32 tx = pos.x * vp->m[0][0] + pos.y * vp->m[1][0] + pos.z * vp->m[2][0] + vp->m[3][0];
                const f32 ty = pos.x * vp->m[0][1] + pos.y * vp->m[1][1] + pos.z * vp->m[2][1] + vp->m[3][1];
                
                const f32 sx = (tx / w + 1.f) * hw + vpx;
                const f32 sy = (-ty / w + 1.f) * hh + vpy;

                // extract the pure projection scales by getting the length of the VP matrix columns
                const f32 proj_scale_x = sqrtf(vp->m[0][0]*vp->m[0][0] + vp->m[1][0]*vp->m[1][0] + vp->m[2][0]*vp->m[2][0]);
                const f32 proj_scale_y = sqrtf(vp->m[0][1]*vp->m[0][1] + vp->m[1][1]*vp->m[1][1] + vp->m[2][1]*vp->m[2][1]);

                const f32 safe_w = SFT_MAX(w - radius, SFT_EPSILON);

                // scale the radius using the pure projection factors, not the view dependent ones
                const f32 rx_screen = fabsf((radius * proj_scale_x) / safe_w * hw);
                const f32 ry_screen = fabsf((radius * proj_scale_y) / safe_w * hh);

                const f32 l_min_x = sx - rx_screen;
                const f32 l_max_x = sx + rx_screen;
                const f32 l_min_y = sy - ry_screen;
                const f32 l_max_y = sy + ry_screen;

                const f32 t_min_x = (f32)tile_min_x;
                const f32 t_max_x = (f32)(tile_min_x + SFT_TILE_SIZE);
                const f32 t_min_y = (f32)tile_min_y;
                const f32 t_max_y = (f32)(tile_min_y + SFT_TILE_SIZE);

                if (l_max_x < t_min_x || l_min_x > t_max_x || l_max_y < t_min_y || l_min_y > t_max_y) {
                    continue;
                }
            }

            tile_lights[tile_light_count] = (u16)l;
            tile_light_count += 1;

            if (SFT_ARRLEN(tile_lights) == tile_light_count) {
                break;
            }

        }
    }
    
    // ---  proxy to light matching and constant caching  ---
    struct sft_light_proxy_cache proxy_cache[16][16]; // [light_index][proxy_index]
    
    u8 l_proxy_count[16] = {0};
    const i32 total_proxies = sft_state->shadows.proxy_count.count;

    if (sft_state->shadows.enabled && total_proxies > 0 && tile_light_count > 0) {
        for (i32 i = 0; i < tile_light_count; i += 1) {
            const sft_point_light* l = &sft_state->lighting.point[tile_lights[i]];
            u8 p_count = 0;
            
            for (i32 p = 0; p < total_proxies && p_count < 16; p += 1) {
                const struct sft_world_shadow_proxy* px = &sft_state->shadows.proxies[p];
                
                // get gradient score
                const u8 vis_score = px->light_vis[tile_lights[i]];
                if (0 == vis_score) {
                    // fully occluded
                    continue;
                }

                const sftvec cap_center = SFT_V(
                    (px->posa.x + px->posb.x) * 0.5f,
                    (px->posa.y + px->posb.y) * 0.5f,
                    (px->posa.z + px->posb.z) * 0.5f);
                const sftvec ab_scalar = sft_v_sub(px->posb, px->posa);
                const f32 cap_len = sft_v_length(ab_scalar);
                const f32 bound_radius = px->radius + (cap_len * 0.5f);
                const f32 combined_r = bound_radius + l->radius;
                
                const sftvec W_scalar = sft_v_sub(cap_center, l->pos); 
                const f32 dist2 = sft_v_length2(W_scalar);
                
                if (dist2 <= (combined_r * combined_r)) {
                    struct sft_light_proxy_cache* c = &proxy_cache[i][p_count];
                    
                    c->pa = (sftvvec){ sft_vf_set1(px->posa.x), sft_vf_set1(px->posa.y), sft_vf_set1(px->posa.z) };
                    c->pb = (sftvvec){ sft_vf_set1(px->posb.x), sft_vf_set1(px->posb.y), sft_vf_set1(px->posb.z) };
                    c->ab = (sftvvec){ sft_vf_set1(ab_scalar.x), sft_vf_set1(ab_scalar.y), sft_vf_set1(ab_scalar.z) };
                    c->W  = (sftvvec){ sft_vf_set1(W_scalar.x), sft_vf_set1(W_scalar.y), sft_vf_set1(W_scalar.z) };
                    
                    c->abdab = sft_vf_set1(sft_v_dot(ab_scalar, ab_scalar));
                    c->wdw   = sft_vf_set1(dist2);
                    c->rb2   = sft_vf_set1(bound_radius * bound_radius);
                    c->pr    = sft_vf_set1(px->radius);
                    c->pr2   = sft_vf_set1(px->radius * px->radius);

                    // cache the multiplier (1.0, 0.66, or 0.33)
                    c->vis_mult  = sft_vf_set1((f32)vis_score * 0.3333333f);
                    p_count += 1;
                }
            }
            l_proxy_count[i] = p_count;
        }
    }

    const vf32 v_zero = sft_vf_setzero();
    const vf32 v_one = sft_vf_set1(1.f);
    const vi32 v_empty_vid = sft_vi_set1(0xFFFFFFFF);
    const vi32 v_screen_w = sft_vi_set1(screen_w);
    const vi32 v_tile_size = sft_vi_set1(SFT_TILE_SIZE);

    // ---  2D block offsets  ---
    #if SFT_SIMD_WIDTH == 8
        alignas(32) const f32 x_offsets_arr[8]  = {0.f, 1.f, 2.f, 3.f, 0.f, 1.f, 2.f, 3.f};
        alignas(32) const f32 y_offsets_arr[8]  = {0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f};
        alignas(32) const i32 ix_offsets_arr[8] = {0, 1, 2, 3, 0, 1, 2, 3};
        alignas(32) const i32 iy_offsets_arr[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    #elif SFT_SIMD_WIDTH == 4
        alignas(16) const f32 x_offsets_arr[4]  = {0.f, 1.f, 0.f, 1.f};
        alignas(16) const f32 y_offsets_arr[4]  = {0.f, 0.f, 1.f, 1.f};
        alignas(16) const i32 ix_offsets_arr[4] = {0, 1, 0, 1};
        alignas(16) const i32 iy_offsets_arr[4] = {0, 0, 1, 1};
    #else
        const f32 x_offsets_arr[1]  = {0.f};
        const f32 y_offsets_arr[1]  = {0.f};
        const i32 ix_offsets_arr[1] = {0};
        const i32 iy_offsets_arr[1] = {0};
    #endif

    const vf32 v_x_offsets = sft_vf_load(x_offsets_arr);
    const vf32 v_y_offsets = sft_vf_load(y_offsets_arr);
    const vi32 v_ix_offsets = sft_vi_load(ix_offsets_arr);
    const vi32 v_iy_offsets = sft_vi_load(iy_offsets_arr);

    for (i32 y = tile_min_y; y < limit_y; y += SFT_BLOCK_Y) {
        for (i32 x = tile_min_x; x < limit_x; x += SFT_BLOCK_X) {
            
            // ---  2D block coord generation  ---
            const vi32 v_global_x = sft_vi_add(sft_vi_set1(x), v_ix_offsets);
            const vi32 v_global_y = sft_vi_add(sft_vi_set1(y), v_iy_offsets);
            const vf32 v_px = sft_vf_add(sft_vf_set1((f32)x + 0.5f), v_x_offsets);
            const vf32 v_py = sft_vf_add(sft_vf_set1((f32)y + 0.5f), v_y_offsets);

            // bounds check handles pixels that bleed off the bottom/right edges of the screen
            const vi32 v_in_bounds = sft_vi_and(
                sft_vi_cmp_gt(v_screen_w, v_global_x),
                sft_vi_cmp_gt(sft_vi_set1(screen_h), v_global_y)
            );

            // ---  gather 2D visibility buffer  ---
            const vi32 v_local_x = sft_vi_sub(v_global_x, sft_vi_set1(tile_min_x));
            const vi32 v_local_y = sft_vi_sub(v_global_y, sft_vi_set1(tile_min_y));
            const vi32 v_p_ind = sft_vi_add(sft_vi_mullo(v_local_y, v_tile_size), v_local_x);
            
            const vi32 v_vids = sft_vi_gather((const int*)vid_buf, v_p_ind);
            
            const vi32 v_is_empty = sft_vi_cmp_eq(v_vids, v_empty_vid);
            const vi32 v_is_valid = sft_vi_andnot(v_is_empty, v_in_bounds);
            
            i32 active_pixels = sft_vi_movemask(v_is_valid) & ((1 << SFT_SIMD_WIDTH) - 1);

            // clear background
            if (sft_state->display.should_clear) {
                i32 empty_mask = sft_vi_movemask(sft_vi_and(v_is_empty, v_in_bounds)) & ((1 << SFT_SIMD_WIDTH) - 1);
                while (empty_mask) {
                    const i32 lane = sft__tzcnt(empty_mask);
                    empty_mask &= (empty_mask - 1);
                    const i32 px = x + ix_offsets_arr[lane];
                    const i32 py = y + iy_offsets_arr[lane];
                    sft_state->display.color_buffer[py * pitch + px] = sft_state->display.clear_color;
                }
            }

            if (0 == active_pixels) {
                continue;
            }

            while (active_pixels) {
                const i32 first_lane = sft__tzcnt(active_pixels);
                
                // extract target vid from the scalar array representation of the lane
                // must use a scalar array load here because the vids array is contiguous 1D memory, 
                // but the 2D block's actual visibility data was gathered into the vector register
                alignas(32) u32 s_vids[SFT_SIMD_WIDTH];
                sft_vi_store((vi32*)s_vids, v_vids);
                const u32 target_vid = s_vids[first_lane];

                const vi32 v_target_vid = sft_vi_set1(target_vid);
                const vi32 v_tri_mask = sft_vi_and(sft_vi_cmp_eq(v_vids, v_target_vid), v_in_bounds);
                
                const i32 tri_lanes = sft_vi_movemask(v_tri_mask) & active_pixels;
                active_pixels &= ~tri_lanes;

                u32 inst_ind, m_ind;
                vf32 sx0, sy0, inv_w0, u0, v0, lu0, lv0;
                vf32 sx1, sy1, inv_w1, u1, v1, lu1, lv1;
                vf32 sx2, sy2, inv_w2, u2, v2, lu2, lv2;

                // world space vectors here so theyre available to the interpolator
                sftvvec wp0, wp1, wp2;
                sftvvec wn0, wn1, wn2;

                const u32 is_clipped = target_vid & (1U << 31);
                
                if (is_clipped) {
                    const u32 clip_slot = target_vid & ~((u32)1 << 31);
                    const struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[clip_slot];
                    
                    inst_ind = ct->inst_ind;
                    m_ind = ct->m_ind;
                    
                    sx0 = sft_vf_set1(ct->sx[0]), sy0 = sft_vf_set1(ct->sy[0]), inv_w0 = sft_vf_set1(ct->inv_w[0]);
                    u0 = sft_vf_set1(ct->u[0]), v0 = sft_vf_set1(ct->v[0]);
                    lu0 = sft_vf_set1(ct->lu[0]), lv0 = sft_vf_set1(ct->lv[0]);
                    
                    wp0 = (sftvvec){ sft_vf_set1(ct->px[0]), sft_vf_set1(ct->py[0]), sft_vf_set1(ct->pz[0]) };
                    wn0 = (sftvvec){ sft_vf_set1(ct->nx[0]), sft_vf_set1(ct->ny[0]), sft_vf_set1(ct->nz[0]) };
                    
                    sx1 = sft_vf_set1(ct->sx[1]), sy1 = sft_vf_set1(ct->sy[1]), inv_w1 = sft_vf_set1(ct->inv_w[1]);
                    u1 = sft_vf_set1(ct->u[1]), v1 = sft_vf_set1(ct->v[1]);
                    lu1 = sft_vf_set1(ct->lu[1]), lv1 = sft_vf_set1(ct->lv[1]);
                    
                    wp1 = (sftvvec){ sft_vf_set1(ct->px[1]), sft_vf_set1(ct->py[1]), sft_vf_set1(ct->pz[1]) };
                    wn1 = (sftvvec){ sft_vf_set1(ct->nx[1]), sft_vf_set1(ct->ny[1]), sft_vf_set1(ct->nz[1]) };
                    
                    sx2 = sft_vf_set1(ct->sx[2]), sy2 = sft_vf_set1(ct->sy[2]), inv_w2 = sft_vf_set1(ct->inv_w[2]);
                    u2 = sft_vf_set1(ct->u[2]), v2 = sft_vf_set1(ct->v[2]);
                    lu2 = sft_vf_set1(ct->lu[2]), lv2 = sft_vf_set1(ct->lv[2]);
                    
                    wp2 = (sftvvec){ sft_vf_set1(ct->px[2]), sft_vf_set1(ct->py[2]), sft_vf_set1(ct->pz[2]) };
                    wn2 = (sftvvec){ sft_vf_set1(ct->nx[2]), sft_vf_set1(ct->ny[2]), sft_vf_set1(ct->nz[2]) };
                }
                else {
                    inst_ind  = (target_vid >> (SFT_TRIANGLE_BITS + SFT_MESHLET_BITS)) & ((1 << SFT_INSTANCE_BITS) - 1);
                    m_ind     = (target_vid >> SFT_TRIANGLE_BITS) & ((1 << SFT_MESHLET_BITS) - 1); // global_m_ind
                    const u32 local_tri = target_vid & ((1 << SFT_TRIANGLE_BITS) - 1);

                    const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
                    
                    const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
                    const sft_mesh* mesh = &inst->model->meshes[mesh_ind];
                    const u32 local_m_ind = m_ind - mesh->model_meshlet_offset;

                    const struct sft_meshlet_inds* m_inds = &mesh->inds[local_m_ind];
                    const struct sft_meshlet_attribs* m_attribs = &mesh->meshlet_attribs[local_m_ind];
                    const u32 ss_m_ind = inst->ss_meshlet_offset + m_ind;

                    const u32 batch = local_tri / SFT_SIMD_WIDTH;
                    const u32 lane  = local_tri % SFT_SIMD_WIDTH;
                    const u32 batch_offset = batch * (SFT_SIMD_WIDTH * 4);

                    const u8 i0 = m_inds->inds[batch_offset + (0 * SFT_SIMD_WIDTH) + lane];
                    const u8 i1 = m_inds->inds[batch_offset + (1 * SFT_SIMD_WIDTH) + lane];
                    const u8 i2 = m_inds->inds[batch_offset + (2 * SFT_SIMD_WIDTH) + lane];

                    sx0 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sx[i0]);
                    sy0 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sy[i0]);
                    inv_w0 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].inv_w[i0]);
                    u0 = sft_vf_set1(m_attribs->u[i0]), v0 = sft_vf_set1(m_attribs->v[i0]);
                    lu0 = sft_vf_set1(m_attribs->lu[i0]), lv0 = sft_vf_set1(m_attribs->lv[i0]);

                    wp0 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wx[i0]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wy[i0]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wz[i0]) };
                    wn0 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnx[i0]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wny[i0]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnz[i0]) };

                    sx1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sx[i1]);
                    sy1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sy[i1]);
                    inv_w1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].inv_w[i1]);
                    u1 = sft_vf_set1(m_attribs->u[i1]), v1 = sft_vf_set1(m_attribs->v[i1]);
                    lu1 = sft_vf_set1(m_attribs->lu[i1]), lv1 = sft_vf_set1(m_attribs->lv[i1]);
                    
                    wp1 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wx[i1]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wy[i1]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wz[i1]) };
                    wn1 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnx[i1]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wny[i1]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnz[i1]) };
                    
                    sx2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sx[i2]);
                    sy2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sy[i2]);
                    inv_w2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].inv_w[i2]);
                    u2 = sft_vf_set1(m_attribs->u[i2]), v2 = sft_vf_set1(m_attribs->v[i2]);
                    lu2 = sft_vf_set1(m_attribs->lu[i2]), lv2 = sft_vf_set1(m_attribs->lv[i2]);

                    wp2 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wx[i2]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wy[i2]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wz[i2]) };
                    wn2 = (sftvvec){ sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnx[i2]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wny[i2]), sft_vf_set1(sft_state->ss_cache[ss_m_ind].wnz[i2]) };
                }

                const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];

                // ---  unconditional barycentric setup  ---
                // evaluate the plane equations for all lanes in the block, even if they aren't part 
                // of the current triangle so the DDX/DDY math stays stable across the entire 2D stamp
                const vf32 v_dx1 = sft_vf_sub(sx1, sx0), v_dx2 = sft_vf_sub(sx2, sx0);
                const vf32 v_dy1 = sft_vf_sub(sy1, sy0), v_dy2 = sft_vf_sub(sy2, sy0);
                const vf32 v_inv_det = sft_vf_rcp_refined(sft_vf_sub(sft_vf_mul(v_dx1, v_dy2), sft_vf_mul(v_dx2, v_dy1)));

                const vf32 v_px_sub_sx0 = sft_vf_sub(v_px, sx0);
                const vf32 v_py_sub_sy0 = sft_vf_sub(v_py, sy0);

                const vf32 bu = sft_vf_mul(sft_vf_sub(sft_vf_mul(v_px_sub_sx0, v_dy2), sft_vf_mul(v_py_sub_sy0, v_dx2)), v_inv_det);
                const vf32 bv = sft_vf_mul(sft_vf_sub(sft_vf_mul(v_py_sub_sy0, v_dx1), sft_vf_mul(v_px_sub_sx0, v_dy1)), v_inv_det);
                const vf32 bw = sft_vf_sub(sft_vf_sub(v_one, bu), bv);

                const vf32 v_inv_z = sft_vf_fmadd(bw, inv_w0, sft_vf_fmadd(bu, inv_w1, sft_vf_mul(bv, inv_w2)));
                const vf32 v_z = sft_vf_rcp_refined(v_inv_z);

                const vf32 pW0 = sft_vf_mul(sft_vf_mul(bw, inv_w0), v_z);
                const vf32 pW1 = sft_vf_mul(sft_vf_mul(bu, inv_w1), v_z);
                const vf32 pW2 = sft_vf_mul(sft_vf_mul(bv, inv_w2), v_z);

                const vf32 cu = sft_vf_fmadd(pW0, u0, sft_vf_fmadd(pW1, u1, sft_vf_mul(pW2, u2)));
                const vf32 cv = sft_vf_fmadd(pW0, v0, sft_vf_fmadd(pW1, v1, sft_vf_mul(pW2, v2)));

                const vf32 cu_lm = sft_vf_fmadd(pW0, lu0, sft_vf_fmadd(pW1, lu1, sft_vf_mul(pW2, lu2)));
                const vf32 cv_lm = sft_vf_fmadd(pW0, lv0, sft_vf_fmadd(pW1, lv1, sft_vf_mul(pW2, lv2)));

                // ---  perspective correct interpolation  ---
                sftvvec interp_pos;
                interp_pos.x = sft_vf_fmadd(pW0, wp0.x, sft_vf_fmadd(pW1, wp1.x, sft_vf_mul(pW2, wp2.x)));
                interp_pos.y = sft_vf_fmadd(pW0, wp0.y, sft_vf_fmadd(pW1, wp1.y, sft_vf_mul(pW2, wp2.y)));
                interp_pos.z = sft_vf_fmadd(pW0, wp0.z, sft_vf_fmadd(pW1, wp1.z, sft_vf_mul(pW2, wp2.z)));

                sftvvec interp_norm;
                interp_norm.x = sft_vf_fmadd(pW0, wn0.x, sft_vf_fmadd(pW1, wn1.x, sft_vf_mul(pW2, wn2.x)));
                interp_norm.y = sft_vf_fmadd(pW0, wn0.y, sft_vf_fmadd(pW1, wn1.y, sft_vf_mul(pW2, wn2.y)));
                interp_norm.z = sft_vf_fmadd(pW0, wn0.z, sft_vf_fmadd(pW1, wn1.z, sft_vf_mul(pW2, wn2.z)));

                // ---  normalization and vector setup  ---
                const sftvvec N = sft_vv_norm_fast(interp_norm);
                
                // broadcast the camera position
                const sftvvec v_cam_pos = {
                    sft_vf_set1(sft_state->camera_pos.x),
                    sft_vf_set1(sft_state->camera_pos.y),
                    sft_vf_set1(sft_state->camera_pos.z)
                };
                
                const sftvvec v_cam_to_pos = sft_vv_sub(v_cam_pos, interp_pos);
                const sftvvec V = sft_vv_norm_fast(v_cam_to_pos);

                // ---  material lookup  ---
                const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
                const sft_mesh* render_mesh = &inst->model->meshes[mesh_ind];
                const sft_mat_handle mat_handle = render_mesh->material;
                const u32 lm_page = render_mesh->lightmap_page;
                
                u32 base_color_int = 0xFFFFFFFF;
                sft_tex_handle tex_handle = SFT_INVALID_HANDLE;
                f32 mat_shininess = 4.f;
                
                if (SFT_INVALID_HANDLE != mat_handle) {
                    const u32 m_mat_ind = SFT_MAT_INDEX(mat_handle);
                    if (SFT_MAT_IS_TRANSIENT(mat_handle)) {
                        base_color_int = sft_state->mat_pool_transient.base_color[m_mat_ind];
                        tex_handle = sft_state->mat_pool_transient.albedo_tex[m_mat_ind];
                        mat_shininess = sft_state->mat_pool_transient.shininess[m_mat_ind];
                    }
                    else {
                        base_color_int = sft_state->mat_pool.base_color[m_mat_ind];
                        tex_handle = sft_state->mat_pool.albedo_tex[m_mat_ind];
                        mat_shininess = sft_state->mat_pool.shininess[m_mat_ind];
                    }
                }

                if (SFT_DEBUG_CHECKERBOARD == sft_state->display.debug_mode) {
                    tex_handle = sft_state->mat_pool.albedo_tex[SFT_MAT_INDEX(sft_state->base_mat)];
                    base_color_int = 0xFFFFFFFF;
                }

                // decode base color
                const vf32 inv255 = sft_vf_set1(1.f / 255.f);
                const vf32 base_r = sft_vf_mul(sft_vf_set1((f32)((base_color_int >> 16) & 0xFF)), inv255);
                const vf32 base_g = sft_vf_mul(sft_vf_set1((f32)((base_color_int >> 8) & 0xFF)), inv255);
                const vf32 base_b = sft_vf_mul(sft_vf_set1((f32)((base_color_int >> 0) & 0xFF)), inv255);

                // ---  texture sampling  ---
                const vf32 ddx_u = sft_vf_ddx(cu);
                const vf32 ddy_u = sft_vf_ddy(cu);
                const vf32 ddx_v = sft_vf_ddx(cv);
                const vf32 ddy_v = sft_vf_ddy(cv);

                const i32 lod = sft__calculate_lod(tex_handle, ddx_u, ddy_u, ddx_v, ddy_v, v_tri_mask, x, y);
                vf32 tex_r, tex_g, tex_b;
                
                // texels are so small that bilinear isn't noticeable
                if (lod > 1) {
                    sft__sample_tex_nearest(tex_handle, lod, cu, cv, &tex_r, &tex_g, &tex_b);
                }
                else {
                    sft__sample_tex_bilinear(tex_handle, lod, cu, cv, &tex_r, &tex_g, &tex_b);
                }
                
                const vf32 albedo_r = sft_vf_mul(tex_r, base_r);
                const vf32 albedo_g = sft_vf_mul(tex_g, base_g);
                const vf32 albedo_b = sft_vf_mul(tex_b, base_b);

                vf32 final_r, final_g, final_b;
                vf32 diffuse_r, diffuse_g, diffuse_b;
                vf32 spec_r = v_zero, spec_g = v_zero, spec_b = v_zero;
                
                const vf32 v_one = sft_vf_set1(1.f);
                vf32 v_lightmap_shadow = v_one; // accumulates capsule shadow occlusion for lightmaps

                const vf32 v_n = sft_vf_set1(mat_shininess);
                const vf32 v_specular_scale = sft_vf_set1(((mat_shininess + 8.f) / 8.f) * 0.04f);
                const u8 is_lightmapped = (0xFFFFFFFF != lm_page);

                // ---  1, base lighting  ---
                if (is_lightmapped) {
                    const sft_tex_handle lm_handle = inst->model->lightmaps[lm_page];
                    sft__sample_tex_bilinear(lm_handle, 0, cu_lm, cv_lm, &diffuse_r, &diffuse_g, &diffuse_b);

                    // overbrightening
                    const vf32 v_mult = sft_vf_set1(sft_state->lighting.lightmap_brightness);
                    diffuse_r = sft_vf_mul(diffuse_r, v_mult);
                    diffuse_g = sft_vf_mul(diffuse_g, v_mult);
                    diffuse_b = sft_vf_mul(diffuse_b, v_mult);
                } 
                else {
                    diffuse_r = sft_vf_set1(sft_state->ambient_light.x);
                    diffuse_g = sft_vf_set1(sft_state->ambient_light.y);
                    diffuse_b = sft_vf_set1(sft_state->ambient_light.z);

                    // ---  directional lights accumulation loop  ---
                    const i32 dir_count = sft_state->lighting.directional_count.count;
                    for (i32 l = 0; l < dir_count; l += 1) {
                        const sftvec l_dir = sft_state->lighting.directional[l].dir;
                        const sftvec l_col = sft_state->lighting.directional[l].color;
                        
                        const sftvvec L = { sft_vf_set1(l_dir.x), sft_vf_set1(l_dir.y), sft_vf_set1(l_dir.z) };
                        const vf32 lr = sft_vf_set1(l_col.x), lg = sft_vf_set1(l_col.y), lb = sft_vf_set1(l_col.z);

                        const vf32 n_dot_l = sft_vf_max(sft_vv_dot(N, L), v_zero);
                        const vf32 lit_mask = sft_vf_cmp_gt(n_dot_l, v_zero);
                        
                        diffuse_r = sft_vf_fmadd(n_dot_l, lr, diffuse_r);
                        diffuse_g = sft_vf_fmadd(n_dot_l, lg, diffuse_g);
                        diffuse_b = sft_vf_fmadd(n_dot_l, lb, diffuse_b);

                        if (0 == sft_vf_movemask(lit_mask)) {
                            continue;
                        }

                        // specular
                        const sftvvec H = sft_vv_norm_fast(sft_vv_add(L, V));
                        const vf32 n_dot_h = sft_vf_max(sft_vv_dot(N, H), v_zero);
                        const vf32 denom = sft_vf_add(sft_vf_sub(v_n, sft_vf_mul(v_n, n_dot_h)), n_dot_h);
                        const vf32 spec = sft_vf_mul(n_dot_h, sft_vf_rcp_refined(denom));
                        
                        const vf32 specular_term = sft_vf_and(sft_vf_mul(sft_vf_mul(spec, v_specular_scale), n_dot_l), lit_mask);
                        spec_r = sft_vf_fmadd(specular_term, lr, spec_r);
                        spec_g = sft_vf_fmadd(specular_term, lg, spec_g);
                        spec_b = sft_vf_fmadd(specular_term, lb, spec_b);
                    }
                }

                // ---  2, point lights and capsule shadows  ---
                for (i32 i = 0; i < tile_light_count; i += 1) {
                    const u16 l = tile_lights[i];
                    const sft_point_light* light = &sft_state->lighting.point[l];
                    
                    const sftvvec v_light_pos = { sft_vf_set1(light->pos.x), sft_vf_set1(light->pos.y), sft_vf_set1(light->pos.z) };
                    const vf32 lr = sft_vf_set1(light->color.x), lg = sft_vf_set1(light->color.y), lb = sft_vf_set1(light->color.z);
                    const vf32 v_inv_r2 = sft_vf_set1(light->inv_radius_sqr);

                    const sftvvec L_raw = sft_vv_sub(v_light_pos, interp_pos);
                    const vf32 d2 = sft_vv_dot(L_raw, L_raw);

                    // more intuitive falloff: max(0, 1 - (d^2 / r^2))^2
                    // creates a smooth bell curve that hits exactly 0.0 at the radius
                    vf32 attenuation = sft_vf_max(v_zero, sft_vf_fnmadd(d2, v_inv_r2, v_one));
                    attenuation = sft_vf_mul(attenuation, attenuation);

                    // skip lighting math if attenuation is zero
                    const vf32 active_light_mask = sft_vf_cmp_gt(attenuation, v_zero);
                    if (0 == sft_vf_movemask(active_light_mask)) {
                        continue;
                    }

                    // normalize L
                    const sftvvec L = sft_vv_norm_fast(L_raw);
                    
                    const vf32 v_lane_mask = sft_vf_cast_vi32(v_tri_mask);

                    // diffuse
                    const vf32 n_dot_l = sft_vf_max(sft_vv_dot(N, L), v_zero);
                    vf32 lit_mask = sft_vf_and(sft_vf_cmp_gt(n_dot_l, v_zero), active_light_mask);

                        // restrict the lit mask strictly to valid pixels
                    lit_mask = sft_vf_and(lit_mask, v_lane_mask);

                    vf32 v_max_shadow = v_zero;
                    const u8 active_proxy_count = l_proxy_count[i];
                    
                    if (active_proxy_count > 0) {
                        const vf32 l_raw_len = sft_vf_sqrt(d2); 
                        
                        const vf32 v_blend_k = sft_vf_set1(0.2f); // increase to make joints blobbier
                        const vf32 v_blend_inv4k = sft_vf_set1(0.25f / 0.2f); // always 0.25 / v_blend_k
                        const vi32 v_abs_mask = sft_vi_set1(0x7FFFFFFF);

                        for (u32 p_ind = 0; p_ind < active_proxy_count; p_ind += 1) {
                            const struct sft_light_proxy_cache* c = &proxy_cache[i][p_ind];
                            
                            const vf32 W_dot_V = sft_vf_sub(v_zero, sft_vv_dot(c->W, L_raw)); 
                            const vf32 is_between = sft_vf_and(sft_vf_cmp_gt(W_dot_V, v_zero), sft_vf_cmp_lt(W_dot_V, d2));
                            
                            const vf32 t_frac = sft_vf_mul(W_dot_V, sft_vf_rcp_refined(d2));
                            const vf32 closest_dist2 = sft_vf_fnmadd(W_dot_V, t_frac, c->wdw);
                            
                            vf32 active_ray_mask = sft_vf_and(is_between, sft_vf_cmp_lt(closest_dist2, c->rb2));
                            active_ray_mask = sft_vf_and(active_ray_mask, is_lightmapped ? v_lane_mask : lit_mask); 
                            
                            if (0 == sft_vf_movemask(active_ray_mask)) {
                                continue; 
                            }

                            const sftvvec ao = sft_vv_sub(interp_pos, c->pa);
                            const vf32 ab_dot_l  = sft_vv_dot(c->ab, L);
                            const vf32 ab_dot_ao = sft_vv_dot(c->ab, ao);
                            const vf32 l_dot_ao  = sft_vv_dot(L, ao);
                            
                            const vf32 denom = sft_vf_fnmadd(ab_dot_l, ab_dot_l, c->abdab); 
                            const vf32 safe_denom = sft_vf_max(denom, sft_vf_set1(SFT_EPSILON));
                            
                            vf32 t2 = sft_vf_mul(sft_vf_fnmadd(ab_dot_l, l_dot_ao, ab_dot_ao), sft_vf_rcp_refined(safe_denom));
                            t2 = sft_vf_max(v_zero, sft_vf_min(t2, v_one)); 
                            
                            const vf32 t1 = sft_vf_fmsub(t2, ab_dot_l, l_dot_ao); 
                            const vf32 is_valid_caster = sft_vf_and(sft_vf_cmp_gt(t1, sft_vf_set1(SFT_EPSILON)), sft_vf_cmp_lt(t1, l_raw_len));
                            
                            const sftvvec pt_ray = sft_vv_add(interp_pos, sft_vv_scale(L, t1));
                            const sftvvec pt_cap = sft_vv_add(c->pa, sft_vv_scale(c->ab, t2));
                            const vf32 dist2 = sft_vv_length2(sft_vv_sub(pt_ray, pt_cap));
                            
                            vf32 shadow_val = sft_vf_max(v_zero, sft_vf_sub(v_one, sft_vf_div(dist2, c->pr2)));
                            shadow_val = sft_vf_and(shadow_val, active_ray_mask);
                            shadow_val = sft_vf_and(shadow_val, is_valid_caster);
                            shadow_val = sft_vf_mul(shadow_val, c->vis_mult);

                            const vf32 diff = sft_vf_sub(v_max_shadow, shadow_val);
                            const vf32 abs_diff = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(diff), v_abs_mask));

                            // the smoothing curve
                            const vf32 h = sft_vf_max(v_zero, sft_vf_sub(v_blend_k, abs_diff));
                            const vf32 blend_add = sft_vf_mul(sft_vf_mul(h, h), v_blend_inv4k);

                            // standard max plus the smooth metaball bridge
                            v_max_shadow = sft_vf_add(sft_vf_max(v_max_shadow, shadow_val), blend_add);
                        }
                    }

                    // convert the maximum shadow back into light intensity
                    const vf32 v_shadow_intensity = sft_vf_max(v_zero, sft_vf_sub(v_one, v_max_shadow));

                    // ---  3, route output  ---
                    if (is_lightmapped) {
                        // shadow lightmaps: map the darkness to the light's geometric attenuation so
                        // a distant light casts fainly but a nearby light casts a harsh shadow
                        const vf32 shadow_drop = sft_vf_mul(sft_vf_sub(v_one, v_shadow_intensity), attenuation);
                        v_lightmap_shadow = sft_vf_mul(v_lightmap_shadow, sft_vf_sub(v_one, shadow_drop));
                    }
                    else {
                        // standard dynamic lighting
                        const vf32 diff_intensity = sft_vf_mul(n_dot_l, attenuation);
                        // apply the interpolated shadow float directly to the diffuse output
                        const vf32 masked_diff_intensity = sft_vf_and(sft_vf_mul(diff_intensity, v_shadow_intensity), lit_mask);
                        
                        diffuse_r = sft_vf_fmadd(masked_diff_intensity, lr, diffuse_r);
                        diffuse_g = sft_vf_fmadd(masked_diff_intensity, lg, diffuse_g);
                        diffuse_b = sft_vf_fmadd(masked_diff_intensity, lb, diffuse_b);
                        
                        // skip lighting math if attenuation is zero or completely shadowed
                        if (0 == sft_vf_movemask(lit_mask)) {
                            continue;
                        }

                        // specular
                        const sftvvec H = sft_vv_norm_fast(sft_vv_add(L, V));
                        const vf32 n_dot_h = sft_vf_max(sft_vv_dot(N, H), v_zero);
                        
                        const vf32 denom = sft_vf_add(sft_vf_sub(v_n, sft_vf_mul(v_n, n_dot_h)), n_dot_h);
                        const vf32 spec = sft_vf_mul(n_dot_h, sft_vf_rcp_refined(denom));
                        
                        const vf32 specular_term = sft_vf_and(sft_vf_mul(sft_vf_mul(sft_vf_mul(spec, v_specular_scale), diff_intensity), v_shadow_intensity), lit_mask);
                        spec_r = sft_vf_fmadd(specular_term, lr, spec_r);
                        spec_g = sft_vf_fmadd(specular_term, lg, spec_g);
                        spec_b = sft_vf_fmadd(specular_term, lb, spec_b);
                    }
                }

                // ---  4, final color combination  ---
                if (is_lightmapped) {
                    // calculate the luminance of the baked lightmap
                    const vf32 v_lum_r = sft_vf_mul(diffuse_r, sft_vf_set1(0.299f));
                    const vf32 v_lum_g = sft_vf_mul(diffuse_g, sft_vf_set1(0.587f));
                    const vf32 v_lum_b = sft_vf_mul(diffuse_b, sft_vf_set1(0.114f));
                    vf32 v_lum = sft_vf_add(v_lum_r, sft_vf_add(v_lum_g, v_lum_b));
                    
                    // normalize luminance to [0, 1] range (accounting for the overbrightening)
                    v_lum = sft_vf_min(v_one, sft_vf_mul(v_lum, sft_vf_set1(1.f / sft_state->lighting.lightmap_brightness)));

                    // Scale the shadow strength by the baked luminance. 
                    // In dark areas, the dynamic shadow fades out, masking the hard occlusion pop.
                    // const vf32 masked_shadow = sft_vf_add(v_one, sft_vf_mul(sft_vf_sub(v_lightmap_shadow, v_one), v_lum));
                    const vf32 masked_shadow = v_lightmap_shadow;

                    const vf32 v_ambient_floor = sft_vf_set1(0.4f);
                    vf32 v_final_lm_mod = sft_vf_fmadd(masked_shadow, sft_vf_sub(v_one, v_ambient_floor), v_ambient_floor);

                    diffuse_r = sft_vf_mul(diffuse_r, v_final_lm_mod);
                    diffuse_g = sft_vf_mul(diffuse_g, v_final_lm_mod);
                    diffuse_b = sft_vf_mul(diffuse_b, v_final_lm_mod);

                    final_r = sft_vf_mul(albedo_r, diffuse_r);
                    final_g = sft_vf_mul(albedo_g, diffuse_g);
                    final_b = sft_vf_mul(albedo_b, diffuse_b);
                }
                else {
                    final_r = sft_vf_fmadd(albedo_r, diffuse_r, spec_r);
                    final_g = sft_vf_fmadd(albedo_g, diffuse_g, spec_g);
                    final_b = sft_vf_fmadd(albedo_b, diffuse_b, spec_b);
                }

                // ---  wireframe debug  ---
                if (SFT_DEBUG_WIREFRAME == sft_state->display.debug_mode) {
                    const vi32 abs_mask = sft_vi_set1(0x7FFFFFFF);
                    
                    // analytical gradients of the barycentric coordinates
                    const vf32 dbu_dx = sft_vf_mul(sft_vf_sub(v_zero, v_dy2), v_inv_det);
                    const vf32 dbu_dy = sft_vf_mul(v_dx2, v_inv_det);
                    const vf32 abs_dbu_dx = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbu_dx), abs_mask));
                    const vf32 abs_dbu_dy = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbu_dy), abs_mask));
                    const vf32 fw_u = sft_vf_add(abs_dbu_dx, abs_dbu_dy); // fast fwidth approx
                    
                    const vf32 dbv_dx = sft_vf_mul(v_dy1, v_inv_det);
                    const vf32 dbv_dy = sft_vf_mul(sft_vf_sub(v_zero, v_dx1), v_inv_det);
                    const vf32 abs_dbv_dx = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbv_dx), abs_mask));
                    const vf32 abs_dbv_dy = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbv_dy), abs_mask));
                    const vf32 fw_v = sft_vf_add(abs_dbv_dx, abs_dbv_dy);
                    
                    const vf32 dbw_dx = sft_vf_sub(v_zero, sft_vf_add(dbu_dx, dbv_dx));
                    const vf32 dbw_dy = sft_vf_sub(v_zero, sft_vf_add(dbu_dy, dbv_dy));
                    const vf32 abs_dbw_dx = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbw_dx), abs_mask));
                    const vf32 abs_dbw_dy = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(dbw_dy), abs_mask));
                    const vf32 fw_w = sft_vf_add(abs_dbw_dx, abs_dbw_dy);
                    
                    // an edge is where the barycentric coordinate is less than ~1 pixel wide
                    const vf32 is_edge = sft_vf_or(sft_vf_cmp_lt(bu, fw_u), 
                        sft_vf_or(sft_vf_cmp_lt(bv, fw_v), sft_vf_cmp_lt(bw, fw_w)));
                    
                    // blend bright green over the shaded geometry
                    final_r = sft_vf_blendv(v_zero, final_r, is_edge);
                    final_g = sft_vf_blendv(v_one, final_g, is_edge);
                    final_b = sft_vf_blendv(v_zero, final_b, is_edge);
                }

                // re pack to 8 bit integer
                const vf32 v_255 = sft_vf_set1(255.f);
                final_r = sft_vf_mul(sft_vf_max(v_zero, sft_vf_min(final_r, v_one)), v_255);
                final_g = sft_vf_mul(sft_vf_max(v_zero, sft_vf_min(final_g, v_one)), v_255);
                final_b = sft_vf_mul(sft_vf_max(v_zero, sft_vf_min(final_b, v_one)), v_255);

                const vi32 v_color = sft_vi_or(sft_vi_set1(0xFF000000),
                    sft_vi_or(sft_vi_slli(sft_vi_cvttps_epi32(final_r), 16),
                    sft_vi_or(sft_vi_slli(sft_vi_cvttps_epi32(final_g), 8), sft_vi_cvttps_epi32(final_b))));

                // masked write to screen
                i32 store_mask = tri_lanes;
                while (store_mask) {
                    const i32 lane = sft__tzcnt(store_mask);
                    store_mask &= (store_mask - 1);
                    const i32 px = x + ix_offsets_arr[lane];
                    const i32 py = y + iy_offsets_arr[lane];
                    sft_state->display.color_buffer[py * pitch + px] = sft_vi_extract(v_color, lane);
                }
            }
        }
    }

    sft__debug_draw_tile_overlay(
        tile_min_x, tile_min_y,
        limit_x, limit_y,
        screen_w, tile_min_z, tile_max_z,
        tile_light_count,
        depth_buf);
}

// main rasterization pipeline
static SFT_INLINE void sft__worker_phase_rasterize(void) {
    const i32 cols = sft_state->display.tile_cols;
    const i32 rows = sft_state->display.tile_rows;
    const i32 total_tiles = cols * rows;

    alignas(32) const f32 x_offsets_arr[8] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f};
    const vf32 v_x_step = sft_vf_load(x_offsets_arr);
    const vf32 v_zero = sft_vf_setzero();

    alignas(64) f32 depth_buf[SFT_TILE_SIZE * SFT_TILE_SIZE];
    alignas(64) u32 vid_buf[SFT_TILE_SIZE * SFT_TILE_SIZE];

    while (1) {
        const i32 queue_ind = sft_atomic_add(&sft_state->next_tile_ind.count, 1);
        if (queue_ind >= total_tiles) {
            break;
        }

        const i32 tile_ind = sft_state->display.sorted_tiles[queue_ind];
        
        const i32 tile_x = tile_ind % cols;
        const i32 tile_y = tile_ind / cols;

        const i32 tile_min_x = tile_x * SFT_TILE_SIZE;
        const i32 tile_min_y = tile_y * SFT_TILE_SIZE;
        const i32 tile_max_x = tile_min_x + SFT_TILE_SIZE;
        const i32 tile_max_y = tile_min_y + SFT_TILE_SIZE;

        memset(depth_buf, 0, SFT_TILE_SIZE * SFT_TILE_SIZE * sizeof(f32));
        memset(vid_buf, 0xFF, SFT_TILE_SIZE * SFT_TILE_SIZE * sizeof(u32));

        i32 current_chunk_ind = sft_state->display.tile_heads[tile_ind];

        const vf32 v_tile_min_x = sft_vf_set1((f32)tile_min_x);
        const vf32 v_tile_max_x = sft_vf_set1((f32)(tile_max_x - 1));
        const vf32 v_tile_min_y = sft_vf_set1((f32)tile_min_y);
        const vf32 v_tile_max_y = sft_vf_set1((f32)(tile_max_y - 1));

        while (-1 != current_chunk_ind) {
            const struct sft_payload_chunk* chunk = &sft_state->display.chunk_pool[current_chunk_ind];

            for (i32 i = 0; i < 31; i += 1) {
                const u64 payload = chunk->payloads[i];
                if (0xFFFFFFFFFFFFFFFFull == payload) {
                    break;
                }

                sft__rast_simd_batch(
                    payload, depth_buf, vid_buf,
                    tile_min_x, tile_min_y,
                    v_x_step, v_zero, v_tile_min_x, v_tile_max_x, v_tile_min_y, v_tile_max_y);
            }
            current_chunk_ind = chunk->next_ind;
        }

        sft__resolve_tile(depth_buf, vid_buf, tile_min_x, tile_min_y);
    }
}

static int sft__cmp_tile_loads(const void* a, const void* b) {
    const i32 ta = *(const i32*)a;
    const i32 tb = *(const i32*)b;
    // sort descending (heaviest first)
    return sft_state->display.tile_payload_counts[tb] - sft_state->display.tile_payload_counts[ta];
}

static void sft__worker_thread(void* user_data) {
    (void)user_data;

    while (1) {
        sft_sem_wait(&sft_state->sem_start);

        // ---  shadow pass  ---
        if (sft_state->shadows.enabled) {
            // TODO add analytical point light shadow
            sft__barrier_wait(&sft_state->shadow_barrier);
        }

        const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;
        
        // grab this thread's private slice of the pre allocated array
        struct sft_thread_bin_state* local_bins = &sft_state->thread_bin_states[sft_thread_id * total_tiles];

        // reset all local bins for the new frame
        for (i32 i = 0; i < total_tiles; i += 1) {
            local_bins[i].chunk_ind = -1;
            local_bins[i].write_head = NULL;
            local_bins[i].write_end = NULL;
            local_bins[i].payload_count = 0;
        }

        // ---  geometry and binning pass  ---
        sft__worker_phase_geom(local_bins);
        sft__barrier_wait(&sft_state->geom_barrier);

        if (0 == sft_thread_id) {            
            const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;
            for (i32 i = 0; i < total_tiles; i += 1) {
                sft_state->display.sorted_tiles[i] = i;
            }
            qsort(sft_state->display.sorted_tiles, total_tiles, sizeof(i32), sft__cmp_tile_loads);
        }

        sft__barrier_wait(&sft_state->prefix_barrier);

        // ---  rasterization pass  ---
        sft__worker_phase_rasterize();
        sft__barrier_wait(&sft_state->raster_barrier); 
        
        // final synchronize and cleanup
        sft__barrier_wait(&sft_state->raster_barrier); 
        
        sft_sem_post(&sft_state->sem_done, 1);
    }
}


/*================================================================
:: OPTIONAL LIBRARIES (CGLTF, STB_IMAGE)
================================================================*/

#ifdef SFT_USE_CGLTF

#ifndef SFT_CGLTF_PATH
    #define SFT_CGLTF_PATH "optional/cgltf.h"
#endif

// user should define
// #define CGLTF_IMPLEMENTATION
#include SFT_CGLTF_PATH

static void* sft__cgltf_alloc(void* user, cgltf_size size) {
    (void)user;
    return malloc((size_t)size);
}
static void sft__cgltf_free(void* user, void* ptr) {
    (void)user;
    free(ptr);
}

#endif // SFT_USE_CGLTF

#ifdef SFT_USE_STB_IMAGE

#ifndef SFT_STB_IMAGE_PATH
    #define SFT_STB_IMAGE_PATH "optional/stb_image.h"
#endif

#define STBI_MALLOC(sz)     malloc(sz)
#define STBI_FREE(p)        free(p)
#define STBI_REALLOC(p, sz) realloc(p, sz)
#define STBI_ASSERT(x)

// user should define
// #define STB_IMAGE_IMPLEMENTATION
#include SFT_STB_IMAGE_PATH

#endif // SFT_USE_STB_IMAGE


/*================================================================
:: FUNCTION DEFINITIONS (CGLTF)
================================================================*/

// finalizer that packs normals and UVs into SoA format
static void sft__builder_finalize_attribs(
    const struct sft_meshlet_builder* builder, 
    const struct sft_raw_geometry* raw,
    u32 skeleton_bone_count, 
    i32* scratch_bone_weights,
    const struct sft_meshlet_output* out
) {
    const u32 batch_count = 128 / SFT_SIMD_WIDTH;
    const u32 batch_bytes = SFT_SIMD_WIDTH * 4;
    for (u32 batch = 0; batch < batch_count; batch += 1) {
        const u32 batch_offset = batch * batch_bytes;
        for (u32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
            const u32 tri_ind = batch * SFT_SIMD_WIDTH + lane;
            u8 v0 = 0, v1 = 0, v2 = 0;
            if (tri_ind < builder->tri_count) {
                v0 = builder->local_inds[tri_ind * 3 + 0];
                v1 = builder->local_inds[tri_ind * 3 + 1];
                v2 = builder->local_inds[tri_ind * 3 + 2];
            }
            out->inds->inds[batch_offset + (0 * SFT_SIMD_WIDTH) + lane] = v0;
            out->inds->inds[batch_offset + (1 * SFT_SIMD_WIDTH) + lane] = v1;
            out->inds->inds[batch_offset + (2 * SFT_SIMD_WIDTH) + lane] = v2;
            out->inds->inds[batch_offset + (3 * SFT_SIMD_WIDTH) + lane] = 0;
        }
    }

    sftvec temp_verts[SFT_VERTS_PER_MESHLET];
    struct sft_aabb aabb = {
        .min = { 1e30f,  1e30f,  1e30f},
        .max = {-1e30f, -1e30f, -1e30f}
    };

    if (out->skin && scratch_bone_weights) {
        memset(scratch_bone_weights, 0, skeleton_bone_count * sizeof(u32));
    }
    
    i32 max_bone = 0, max_bone_weight = 0;

    for (u32 i = 0; i < SFT_VERTS_PER_MESHLET; i += 1) {
        if (i < builder->vert_count) {
            const i32 gi = builder->global_vert_inds[i];
            
            out->verts->x[i] = raw->vx[gi]; 
            out->verts->y[i] = raw->vy[gi]; 
            out->verts->z[i] = raw->vz[gi];
            
            out->attribs->nx[i] = raw->nx[gi]; 
            out->attribs->ny[i] = raw->ny[gi]; 
            out->attribs->nz[i] = raw->nz[gi];
            
            out->attribs->u[i] = raw->u[gi]; 
            out->attribs->v[i] = raw->v[gi];

            if (out->skin) {
                out->skin->j0[i] = raw->j0[gi]; out->skin->w0[i] = raw->w0[gi];
                out->skin->j1[i] = raw->j1[gi]; out->skin->w1[i] = raw->w1[gi];
                out->skin->j2[i] = raw->j2[gi]; out->skin->w2[i] = raw->w2[gi];
                out->skin->j3[i] = raw->j3[gi]; out->skin->w3[i] = raw->w3[gi];

                const u16 j0 = raw->j0[gi], j1 = raw->j1[gi], j2 = raw->j2[gi], j3 = raw->j3[gi];
                
                // track bone weights to find the primary culling bone for the meshlet
                if (j0 < skeleton_bone_count) {
                    scratch_bone_weights[j0] += raw->w0[gi];
                    if (scratch_bone_weights[j0] > max_bone_weight) {
                        max_bone_weight = scratch_bone_weights[j0];
                        max_bone = j0;
                    }
                }
                if (j1 < skeleton_bone_count) {
                    scratch_bone_weights[j1] += raw->w1[gi];
                    if (scratch_bone_weights[j1] > max_bone_weight) {
                        max_bone_weight = scratch_bone_weights[j1];
                        max_bone = j1;
                    }
                }
                if (j2 < skeleton_bone_count) {
                    scratch_bone_weights[j2] += raw->w2[gi];
                    if (scratch_bone_weights[j2] > max_bone_weight) {
                        max_bone_weight = scratch_bone_weights[j2];
                        max_bone = j2;
                    }
                }
                if (j3 < skeleton_bone_count) {
                    scratch_bone_weights[j3] += raw->w3[gi];
                    if (scratch_bone_weights[j3] > max_bone_weight) {
                        max_bone_weight = scratch_bone_weights[j3];
                        max_bone = j3;
                    }
                }
            }

            temp_verts[i].x = raw->vx[gi]; 
            temp_verts[i].y = raw->vy[gi]; 
            temp_verts[i].z = raw->vz[gi];
            
            if (raw->vx[gi] < aabb.min.x) aabb.min.x = raw->vx[gi]; 
            if (raw->vx[gi] > aabb.max.x) aabb.max.x = raw->vx[gi];
            if (raw->vy[gi] < aabb.min.y) aabb.min.y = raw->vy[gi]; 
            if (raw->vy[gi] > aabb.max.y) aabb.max.y = raw->vy[gi];
            if (raw->vz[gi] < aabb.min.z) aabb.min.z = raw->vz[gi]; 
            if (raw->vz[gi] > aabb.max.z) aabb.max.z = raw->vz[gi];
        }
        else {
            out->verts->x[i] = out->verts->y[i] = out->verts->z[i] = 0.f;
            out->attribs->nx[i] = out->attribs->ny[i] = out->attribs->nz[i] = 0.f;
            out->attribs->u[i] = out->attribs->v[i] = 0.f;
            
            if (out->skin) {
                out->skin->j0[i] = out->skin->j1[i] = out->skin->j2[i] = out->skin->j3[i] = 0;
                out->skin->w0[i] = out->skin->w1[i] = out->skin->w2[i] = out->skin->w3[i] = 0;
            }
        }
        
        out->attribs->lu[i] = 0.f; 
        out->attribs->lv[i] = 0.f;
    }

    if (out->culling_bone) {
        *out->culling_bone = out->skin ? max_bone : -1;
    }
    if (out->sphere && builder->vert_count > 0) {
        *out->sphere = sft__meshlet_bounding_sphere(temp_verts, builder->vert_count);
    }
    if (out->aabb && builder->vert_count > 0) {
        *out->aabb = aabb;
    }
}

#ifdef SFT_USE_CGLTF

SFT_FUNC sft_model* sft_model_load_glb(const char* filename, sft_arena* arena, i32 uv_channel, i32 shadow_proxy_budget) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        SFT_ERR_RET(NULL, "failed to open '%s'\n", filename);
    }

    fseek(file, 0, SEEK_END);
    const i64 file_size = ftell(file);
    rewind(file);

    u8* file_content = (u8*)malloc(file_size);
    if (fread(file_content, 1, file_size, file) != (u64)file_size) {
        fclose(file);
        free(file_content);
        SFT_ERR_RET(NULL, "failed to read '%s'\n", filename);
    }
    fclose(file);

    cgltf_options options = {0};
    options.memory.alloc_func = sft__cgltf_alloc;
    options.memory.free_func  = sft__cgltf_free;

    cgltf_data* data = NULL;
    if (cgltf_parse(&options, file_content, file_size, &data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, filename) != cgltf_result_success
    ) {
        if (data) {
            cgltf_free(data);
        }
        free(file_content);
        SFT_ERR_RET(NULL, "failed to parse or load buffers\n");
    }

    // ---  pre load materials  ---
    sft_mat_handle* mat_handles = (sft_mat_handle*)malloc(data->materials_count * sizeof(sft_mat_handle));

    #ifdef SFT_USE_STB_IMAGE
        for (cgltf_size i = 0; i < data->materials_count; i += 1) {
            const cgltf_material* mat = &data->materials[i];
            u32 base_color = 0xFFFFFFFF;
            sft_tex_handle tex = SFT_INVALID_HANDLE;

            if (mat->has_pbr_metallic_roughness) {
                const f32* c = mat->pbr_metallic_roughness.base_color_factor;
                base_color = ((u32)(c[3] * 255.f) << 24) | ((u32)(c[0] * 255.f) << 16) | 
                            ((u32)(c[1] * 255.f) << 8)  | ((u32)(c[2] * 255.f) << 0);

                const cgltf_texture_view* tv = &mat->pbr_metallic_roughness.base_color_texture;
                if (tv->texture && tv->texture->image) {
                    const cgltf_image* img = tv->texture->image;
                    if (img->buffer_view && img->buffer_view->buffer->data) {
                        const u8* img_data = (u8*)img->buffer_view->buffer->data + img->buffer_view->offset;
                        tex = sft_texture_load_from_memory(img_data, img->buffer_view->size);
                    } 
                }
            }
            mat_handles[i] = sft_material_create(tex, base_color, 2.f);
        }
    #else
        #warning "SFT WARNING: using 'cgltf' but not 'stb_image' will cause GLB/GLTF model textures to use fallback checkerboard"
        for (cgltf_size i = 0; i < data->materials_count; i += 1) {
            mat_handles[i] = sft_state->base_mat;
        }
    #endif

    // ---  allocate model  ---
    sft_model* model = (sft_model*)sft_arena_alloc(arena, sizeof(sft_model));
    memset(model, 0, sizeof(sft_model));
    // maximum possible meshes is materials_count + 1 (for primitives with no material)
    model->meshes = (sft_mesh*)sft_arena_alloc(arena, (data->materials_count + 1) * sizeof(sft_mesh));
    u32 active_meshes = 0;

    // ---  extract skeleton and inverse bind matrices  ---
    model->skeleton.bone_count = (i32)data->nodes_count;
    model->skeleton.bones = sft_arena_alloc(arena, data->nodes_count * sizeof(struct sft_bone));
    
    for (cgltf_size i = 0; i < data->nodes_count; i += 1) {
        model->skeleton.bones[i].parent_ind = data->nodes[i].parent ? (i32)(data->nodes[i].parent - data->nodes) : -1;
        model->skeleton.bones[i].inverse_bind_matrix = sft_m_identity();

        model->skeleton.bones[i].base_t = data->nodes[i].has_translation ? 
            SFT_V(data->nodes[i].translation[0], data->nodes[i].translation[1], -data->nodes[i].translation[2]) : SFT_V(0.f, 0.f, 0.f);
        
        model->skeleton.bones[i].base_r = data->nodes[i].has_rotation ? 
            (sftquat){-data->nodes[i].rotation[0], -data->nodes[i].rotation[1], data->nodes[i].rotation[2], data->nodes[i].rotation[3]} : (sftquat){0.f, 0.f, 0.f, 1.f};
        
        model->skeleton.bones[i].base_s = data->nodes[i].has_scale ? 
            SFT_V(data->nodes[i].scale[0], data->nodes[i].scale[1], data->nodes[i].scale[2]) : SFT_V(1.f, 1.f, 1.f);

        cgltf_float world_mat[16];
        cgltf_node_transform_world(&data->nodes[i], world_mat);
        model->skeleton.bones[i].bind_pos = SFT_V(world_mat[12], world_mat[13], -world_mat[14]); 
        
        cgltf_float local_mat[16];
        cgltf_node_transform_local(&data->nodes[i], local_mat);

        sftmat m_raw;
        m_raw.m[0][0] = local_mat[0];  m_raw.m[0][1] = local_mat[1];  m_raw.m[0][2] = local_mat[2];  m_raw.m[0][3] = local_mat[3];
        m_raw.m[1][0] = local_mat[4];  m_raw.m[1][1] = local_mat[5];  m_raw.m[1][2] = local_mat[6];  m_raw.m[1][3] = local_mat[7];
        m_raw.m[2][0] = local_mat[8];  m_raw.m[2][1] = local_mat[9];  m_raw.m[2][2] = local_mat[10]; m_raw.m[2][3] = local_mat[11];
        m_raw.m[3][0] = local_mat[12]; m_raw.m[3][1] = local_mat[13]; m_raw.m[3][2] = local_mat[14]; m_raw.m[3][3] = local_mat[15];

        const sftmat flip_z = sft_m_scale(1.f, 1.f, -1.f);
        model->skeleton.bones[i].local_bind_matrix = sft_m_mul(flip_z, sft_m_mul(m_raw, flip_z));
        
        // safely decompose the fixed matrix for animation fallbacks
        const sftmat m = model->skeleton.bones[i].local_bind_matrix;
        model->skeleton.bones[i].base_t = SFT_V(m.m[3][0], m.m[3][1], m.m[3][2]);
        
        const f32 sx = sqrtf(m.m[0][0]*m.m[0][0] + m.m[0][1]*m.m[0][1] + m.m[0][2]*m.m[0][2]);
        const f32 sy = sqrtf(m.m[1][0]*m.m[1][0] + m.m[1][1]*m.m[1][1] + m.m[1][2]*m.m[1][2]);
        const f32 sz = sqrtf(m.m[2][0]*m.m[2][0] + m.m[2][1]*m.m[2][1] + m.m[2][2]*m.m[2][2]);
        model->skeleton.bones[i].base_s = SFT_V(sx, sy, sz);

        const f32 isx = sx > 0.0001f ? 1.f / sx : 1.f;
        const f32 isy = sy > 0.0001f ? 1.f / sy : 1.f;
        const f32 isz = sz > 0.0001f ? 1.f / sz : 1.f;
        
        const f32 m00 = m.m[0][0] * isx, m01 = m.m[0][1] * isx, m02 = m.m[0][2] * isx;
        const f32 m10 = m.m[1][0] * isy, m11 = m.m[1][1] * isy, m12 = m.m[1][2] * isy;
        const f32 m20 = m.m[2][0] * isz, m21 = m.m[2][1] * isz, m22 = m.m[2][2] * isz;

        const f32 tr = m00 + m11 + m22;
        sftquat q;
        if (tr > 0.f) {
            const f32 S = sqrtf(tr + 1.f) * 2.f;
            q.w = 0.25f * S;
            q.x = (m12 - m21) / S;
            q.y = (m20 - m02) / S;
            q.z = (m01 - m10) / S;
        }
        else if ((m00 > m11) && (m00 > m22)) {
            const f32 S = sqrtf(1.f + m00 - m11 - m22) * 2.f;
            q.w = (m12 - m21) / S;
            q.x = 0.25f * S;
            q.y = (m01 + m10) / S;
            q.z = (m02 + m20) / S;
        }
        else if (m11 > m22) {
            const f32 S = sqrtf(1.f + m11 - m00 - m22) * 2.f;
            q.w = (m20 - m02) / S;
            q.x = (m01 + m10) / S;
            q.y = 0.25f * S;
            q.z = (m12 + m21) / S;
        }
        else {
            const f32 S = sqrtf(1.f + m22 - m00 - m11) * 2.f;
            q.w = (m01 - m10) / S;
            q.x = (m02 + m20) / S;
            q.y = (m12 + m21) / S;
            q.z = 0.25f * S;
        }
        model->skeleton.bones[i].base_r = sft_q_norm(q);
    }
    
    for (cgltf_size i = 0; i < data->skins_count; i += 1) {
        cgltf_skin* skin = &data->skins[i];
        if (!skin->inverse_bind_matrices) {
            continue;
        }
        
        for (cgltf_size j = 0; j < skin->joints_count; j += 1) {
            f32 mat[16];
            cgltf_accessor_read_float(skin->inverse_bind_matrices, j, mat, 16);
            
            sftmat m_raw;
            m_raw.m[0][0] = mat[0];
            m_raw.m[0][1] = mat[1];
            m_raw.m[0][2] = mat[2];
            m_raw.m[0][3] = mat[3];
            m_raw.m[1][0] = mat[4];
            m_raw.m[1][1] = mat[5];
            m_raw.m[1][2] = mat[6];
            m_raw.m[1][3] = mat[7];
            m_raw.m[2][0] = mat[8];
            m_raw.m[2][1] = mat[9];
            m_raw.m[2][2] = mat[10];
            m_raw.m[2][3] = mat[11];
            m_raw.m[3][0] = mat[12];
            m_raw.m[3][1] = mat[13];
            m_raw.m[3][2] = mat[14];
            m_raw.m[3][3] = mat[15];
            
            const sftmat flip_z = sft_m_scale(1.f, 1.f, -1.f);
            const u32 node_ind = (u32)(skin->joints[j] - data->nodes);
            model->skeleton.bones[node_ind].inverse_bind_matrix = sft_m_mul(flip_z, sft_m_mul(m_raw, flip_z));
        }
    }

    // ---  group geometry by material  ---
    // iterate through all materials, plus one final pass (m_ind == materials_count) for NULL materials
    for (cgltf_size m_ind = 0; m_ind <= data->materials_count; m_ind += 1) {
        const cgltf_material* target_mat = (m_ind < data->materials_count) ? &data->materials[m_ind] : NULL;
        sft_mat_handle curr_handle = (m_ind < data->materials_count) ? mat_handles[m_ind] : sft_state->base_mat;

        u64 total_verts = 0;
        u64 total_tris = 0;

        // 1, count required geometry sizes for this material
        for (cgltf_size i = 0; i < data->nodes_count; i += 1) {
            const cgltf_node* node = &data->nodes[i];
            if (!node->mesh) {
                continue;
            }
            
            for (cgltf_size p = 0; p < node->mesh->primitives_count; p += 1) {
                const cgltf_primitive* prim = &node->mesh->primitives[p];
                if (cgltf_primitive_type_triangles != prim->type || prim->material != target_mat) {
                    continue;
                }

                cgltf_accessor* pos_a = NULL;
                for (cgltf_size a = 0; a < prim->attributes_count; a += 1) {
                    if (cgltf_attribute_type_position == prim->attributes[a].type) {
                        pos_a = prim->attributes[a].data;
                        break;
                    }
                }
                if (!pos_a) {
                    continue;
                }

                total_tris += prim->indices ? (i32)prim->indices->count / 3 : (i32)pos_a->count / 3;
                total_verts += (i32)pos_a->count;
            }
        }

        // if no geometry uses this material, skip it
        if (0 == total_verts || 0 == total_tris) {
            continue;
        }

        // allocate unified geometry buffers
        f32* uni_vx = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_vy = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_vz = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_nx = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_ny = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_nz = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_u  = (f32*)malloc(total_verts * sizeof(f32));
        f32* uni_v  = (f32*)malloc(total_verts * sizeof(f32));
        u16 *uni_j0 = malloc(total_verts * sizeof(u16)), *uni_j1 = malloc(total_verts * sizeof(u16));
        u16 *uni_j2 = malloc(total_verts * sizeof(u16)), *uni_j3 = malloc(total_verts * sizeof(u16));
        u8  *uni_w0 = calloc(total_verts, 1), *uni_w1 = calloc(total_verts, 1);
        u8  *uni_w2 = calloc(total_verts, 1), *uni_w3 = calloc(total_verts, 1);
        i32* uni_inds = (i32*)malloc(total_tris * 3 * sizeof(i32));
        i32 v_offset = 0;
        i32 i_offset = 0;
        u8 material_has_skin = 0;

        // 2, extract and transform matching primitives
        for (cgltf_size i = 0; i < data->nodes_count; i += 1) {
            cgltf_node* node = &data->nodes[i];
            if (!node->mesh) {
                continue;
            }

            cgltf_float world_mat[16];
            cgltf_node_transform_world(node, world_mat);

            for (cgltf_size p = 0; p < node->mesh->primitives_count; p += 1) {
                const cgltf_primitive* prim = &node->mesh->primitives[p];
                if (cgltf_primitive_type_triangles != prim->type || prim->material != target_mat) {
                    continue;
                }

                cgltf_accessor *pos_a = NULL, *norm_a = NULL, *uv_a = NULL, *joints_a = NULL, *weights_a = NULL;
                for (cgltf_size a = 0; a < prim->attributes_count; a += 1) {
                    if (cgltf_attribute_type_position == prim->attributes[a].type) {
                        pos_a = prim->attributes[a].data;
                    }
                    if (cgltf_attribute_type_normal   == prim->attributes[a].type) {
                        norm_a = prim->attributes[a].data;
                    }
                    if (cgltf_attribute_type_texcoord == prim->attributes[a].type && uv_channel == prim->attributes[a].index) {
                        uv_a = prim->attributes[a].data;
                    }
                    if (cgltf_attribute_type_joints   == prim->attributes[a].type && 0 == prim->attributes[a].index) {
                        joints_a = prim->attributes[a].data;
                    }
                    if (cgltf_attribute_type_weights  == prim->attributes[a].type && 0 == prim->attributes[a].index) {
                        weights_a = prim->attributes[a].data;
                    }
                }

                u8 prim_has_skin = (joints_a && weights_a && node->skin);
                if (prim_has_skin) {
                    material_has_skin = 1;
                }

                const i32 prim_v_count = (i32)pos_a->count;
                for (i32 v = 0; v < prim_v_count; v += 1) {
                    f32 buf[3] = {0};
                    cgltf_accessor_read_float(pos_a, v, buf, 3);
                    
                    if (prim_has_skin) {
                        uni_vx[v_offset + v] = buf[0];
                        uni_vy[v_offset + v] = buf[1];
                        uni_vz[v_offset + v] = -buf[2]; // Z flip
                    }
                    else {
                        uni_vx[v_offset + v] = buf[0] * world_mat[0] + buf[1] * world_mat[4] + buf[2] * world_mat[8] + world_mat[12];
                        uni_vy[v_offset + v] = buf[0] * world_mat[1] + buf[1] * world_mat[5] + buf[2] * world_mat[9] + world_mat[13];
                        uni_vz[v_offset + v] = -(buf[0] * world_mat[2] + buf[1] * world_mat[6] + buf[2] * world_mat[10] + world_mat[14]);
                    }

                    if (norm_a) {
                        cgltf_accessor_read_float(norm_a, v, buf, 3);
                        if (prim_has_skin) {
                            uni_nx[v_offset + v] = buf[0];
                            uni_ny[v_offset + v] = buf[1];
                            uni_nz[v_offset + v] = -buf[2];
                        }
                        else {
                            f32 nx = buf[0] * world_mat[0] + buf[1] * world_mat[4] + buf[2] * world_mat[8];
                            f32 ny = buf[0] * world_mat[1] + buf[1] * world_mat[5] + buf[2] * world_mat[9];
                            f32 nz = buf[0] * world_mat[2] + buf[1] * world_mat[6] + buf[2] * world_mat[10];
                        
                            const f32 len = sqrtf(nx*nx + ny*ny + nz*nz);
                            if (len > SFT_EPSILON) {
                                nx /= len;
                                ny /= len;
                                nz /= len;
                            }
                            uni_nx[v_offset + v] = nx;
                            uni_ny[v_offset + v] = ny;
                            uni_nz[v_offset + v] = -nz;
                        }
                    }
                    else {
                        uni_nx[v_offset + v] = 0.f;
                        uni_ny[v_offset + v] = 1.f;
                        uni_nz[v_offset + v] = 0.f;
                    }

                    if (uv_a) {
                        cgltf_accessor_read_float(uv_a, v, buf, 2);
                        uni_u[v_offset + v] = buf[0];
                        uni_v[v_offset + v] = buf[1];
                    }
                    else {
                        uni_u[v_offset + v] = 0.f;
                        uni_v[v_offset + v] = 0.f;
                    }
                    
                    if (prim_has_skin) {
                        cgltf_uint j_buf[4] = {0};
                        cgltf_accessor_read_uint(joints_a, v, j_buf, 4);
                        f32 w_buf[4] = {0.f};
                        cgltf_accessor_read_float(weights_a, v, w_buf, 4);
                        
                        // map local skin joints to global skeleton inds
                        uni_j0[v_offset + v] = (u16)(node->skin->joints[j_buf[0]] - data->nodes);
                        uni_j1[v_offset + v] = (u16)(node->skin->joints[j_buf[1]] - data->nodes);
                        uni_j2[v_offset + v] = (u16)(node->skin->joints[j_buf[2]] - data->nodes);
                        uni_j3[v_offset + v] = (u16)(node->skin->joints[j_buf[3]] - data->nodes);
                        
                        // quantize weights
                        uni_w0[v_offset + v] = (u8)(w_buf[0] * 255.f + 0.5f);
                        uni_w1[v_offset + v] = (u8)(w_buf[1] * 255.f + 0.5f);
                        uni_w2[v_offset + v] = (u8)(w_buf[2] * 255.f + 0.5f);
                        uni_w3[v_offset + v] = (u8)(w_buf[3] * 255.f + 0.5f);
                    }
                }

                const i32 prim_tri_count = prim->indices ? (i32)prim->indices->count / 3 : prim_v_count / 3;
                for (i32 k = 0; k < prim_tri_count * 3; k += 1) {
                    const i32 kr = k - (k % 3) + (i32[3]){0, 2, 1}[k % 3];
                    const i32 ind = prim->indices ? (i32)cgltf_accessor_read_index(prim->indices, kr) : kr;
                    uni_inds[i_offset + k] = v_offset + ind;
                }

                v_offset += prim_v_count;
                i_offset += prim_tri_count * 3;
            }
        }

        // ---  build meshlets  ---
        sft_mesh* out_mesh = &model->meshes[active_meshes];
        out_mesh->material = curr_handle;
        out_mesh->lightmap_page = 0xFFFFFFFF;

        i32 *vertex_offsets, *adjacency, *write_heads;
        SFT_CALLOC(vertex_offsets, total_verts + 1, i32);
        SFT_CALLOC(adjacency, total_tris * 3, i32);
        SFT_CALLOC(write_heads, total_verts, i32);

        for (i32 k = 0; k < (i32)total_tris * 3; k += 1) {
            vertex_offsets[uni_inds[k]] += 1;
        }
        i32 prefix = 0;
        for (i32 k = 0; k < (i32)total_verts; k += 1) {
            const i32 count = vertex_offsets[k];
            vertex_offsets[k] = prefix;
            prefix += count;
        }
        vertex_offsets[total_verts] = prefix;
        memcpy(write_heads, vertex_offsets, total_verts * sizeof(i32));

        for (i32 k = 0; k < (i32)total_tris; k += 1) {
            for (i32 v = 0; v < 3; v += 1) {
                const i32 vert = uni_inds[k * 3 + v];
                const i32 pos = write_heads[vert];
                write_heads[vert] += 1;
                adjacency[pos] = k;
            }
        }

        const i32 max_meshlets = (total_tris + SFT_SIMD_WIDTH - 1) & ~(SFT_SIMD_WIDTH - 1);
        out_mesh->meshlets = (struct sft_meshlet*)sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet));
        out_mesh->meshlet_attribs = (struct sft_meshlet_attribs*)sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet_attribs)); 
        out_mesh->inds = (struct sft_meshlet_inds*)sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet_inds));
        
        out_mesh->bounds_center_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_center_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_center_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_radius   = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

        out_mesh->bounds_min_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_min_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_min_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_max_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_max_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->bounds_max_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

        out_mesh->cone_axis_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_axis_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_axis_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_cutoff = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

        if (material_has_skin) {
            out_mesh->meshlet_skins = sft_arena_alloc(arena, max_meshlets * sizeof(struct sft_meshlet_skin));
            out_mesh->culling_bone_inds = sft_arena_alloc(arena, max_meshlets * sizeof(i32));
        }
        else {
            out_mesh->meshlet_skins = NULL;
            out_mesh->culling_bone_inds = NULL;
        }

        i32* scratch_weights = (i32*)malloc(model->skeleton.bone_count * sizeof(i32));

        struct sft_raw_geometry raw_data = {
            .vx = uni_vx, .vy = uni_vy, .vz = uni_vz,
            .nx = uni_nx, .ny = uni_ny, .nz = uni_nz,
            .u  = uni_u,  .v  = uni_v,
            .j0 = uni_j0, .j1 = uni_j1, .j2 = uni_j2, .j3 = uni_j3,
            .w0 = uni_w0, .w1 = uni_w1, .w2 = uni_w2, .w3 = uni_w3
        };

        u8 *used_tris, *in_frontier;
        i32* frontier;
        SFT_CALLOC(used_tris, total_tris, u8);
        SFT_CALLOC(in_frontier, total_tris, u8);
        SFT_CALLOC(frontier, total_tris, i32);

        i32 unmapped_tris = total_tris;
        u32 meshlet_count = 0;
        struct sft_meshlet_builder builder;
        i32 linear_search_head = 0;

        while (unmapped_tris > 0) {
            sft__builder_reset(&builder);
            i32 frontier_head = 0, frontier_tail = 0;

            while (builder.vert_count < SFT_VERTS_PER_MESHLET && builder.tri_count < 128 && unmapped_tris > 0) {
                if (frontier_head == frontier_tail) {
                    i32 jump_seed = -1;
                    for (i32 k = linear_search_head; k < (i32)total_tris; k += 1) {
                        if (!used_tris[k]) {
                            jump_seed = linear_search_head = k;
                            break;
                        }
                    }
                    if (-1 == jump_seed) {
                        break;
                    }

                    const i32 v0 = uni_inds[jump_seed * 3 + 0];
                    const i32 v1 = uni_inds[jump_seed * 3 + 1];
                    const i32 v2 = uni_inds[jump_seed * 3 + 2];
                    const sftvec normal = sft_v_face_normal(
                        SFT_V(uni_vx[v0], uni_vy[v0], uni_vz[v0]), 
                        SFT_V(uni_vx[v1], uni_vy[v1], uni_vz[v1]), 
                        SFT_V(uni_vx[v2], uni_vy[v2], uni_vz[v2]));

                    if (builder.tri_count > 0 && sft__builder_score_candidate(&builder, v0, v1, v2, normal) < 0.f) {
                        break;
                    }

                    sft__builder_add_triangle(&builder, v0, v1, v2, normal);
                    used_tris[jump_seed] = 1;
                    unmapped_tris -= 1;

                    const i32 new_verts[3] = {v0, v1, v2};
                    for (i32 v = 0; v < 3; v += 1) {
                        for (i32 adj_ind = vertex_offsets[new_verts[v]]; adj_ind < vertex_offsets[new_verts[v] + 1]; adj_ind += 1) {
                            const i32 adj_tri = adjacency[adj_ind];
                            if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                                frontier[frontier_tail++] = adj_tri;
                                in_frontier[adj_tri] = 1;
                            }
                        }
                    }
                    continue;
                }

                f32 best_score = -1.f;
                i32 best_tri_ind = -1;
                sftvec best_normal = {0};

                for (i32 k = frontier_head; k < frontier_tail; k += 1) {
                    const i32 candidate_tri = frontier[k];
                    if (used_tris[candidate_tri]) {
                        continue;
                    }

                    const i32 cv0 = uni_inds[candidate_tri * 3 + 0];
                    const i32 cv1 = uni_inds[candidate_tri * 3 + 1];
                    const i32 cv2 = uni_inds[candidate_tri * 3 + 2];
                    const sftvec c_normal = sft_v_face_normal(
                        SFT_V(uni_vx[cv0], uni_vy[cv0], uni_vz[cv0]), 
                        SFT_V(uni_vx[cv1], uni_vy[cv1], uni_vz[cv1]), 
                        SFT_V(uni_vx[cv2], uni_vy[cv2], uni_vz[cv2]));

                    const f32 score = sft__builder_score_candidate(&builder, cv0, cv1, cv2, c_normal);
                    if (score > best_score) {
                        best_score = score;
                        best_tri_ind = k;
                        best_normal = c_normal;
                    }
                }

                if (-1 == best_tri_ind) {
                    frontier_head = frontier_tail = 0;
                    continue;
                }

                const i32 best_tri = frontier[best_tri_ind];
                const i32 bv0 = uni_inds[best_tri * 3 + 0];
                const i32 bv1 = uni_inds[best_tri * 3 + 1];
                const i32 bv2 = uni_inds[best_tri * 3 + 2];

                sft__builder_add_triangle(&builder, bv0, bv1, bv2, best_normal);
                used_tris[best_tri] = 1;
                unmapped_tris -= 1;
                frontier[best_tri_ind] = frontier[frontier_head++];
                
                const i32 new_verts[3] = {bv0, bv1, bv2};
                for (i32 v = 0; v < 3; v += 1) {
                    for (i32 adj_ind = vertex_offsets[new_verts[v]]; adj_ind < vertex_offsets[new_verts[v] + 1]; adj_ind += 1) {
                        const i32 adj_tri = adjacency[adj_ind];
                        if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                            frontier[frontier_tail++] = adj_tri; in_frontier[adj_tri] = 1;
                        }
                    }
                }
            }

            for (i32 k = frontier_head; k < frontier_tail; k += 1) {
                in_frontier[frontier[k]] = 0;
            }

            struct sft_sphere sphere = {0};
            struct sft_aabb aabb = {0};
            struct sft_meshlet_output out_ptrs = {
                .verts = &out_mesh->meshlets[meshlet_count],
                .attribs = &out_mesh->meshlet_attribs[meshlet_count],
                .inds = &out_mesh->inds[meshlet_count],
                .skin = material_has_skin ? &out_mesh->meshlet_skins[meshlet_count] : NULL,
                .culling_bone = material_has_skin ? &out_mesh->culling_bone_inds[meshlet_count] : NULL,
                .sphere = &sphere,
                .aabb = &aabb
            };

            sft__builder_finalize_attribs(&builder, &raw_data, model->skeleton.bone_count, scratch_weights, &out_ptrs);

            out_mesh->bounds_center_x[meshlet_count] = sphere.center.x;
            out_mesh->bounds_center_y[meshlet_count] = sphere.center.y;
            out_mesh->bounds_center_z[meshlet_count] = sphere.center.z;
            out_mesh->bounds_radius[meshlet_count]   = sphere.radius;

            out_mesh->bounds_min_x[meshlet_count] = aabb.min.x;
            out_mesh->bounds_min_y[meshlet_count] = aabb.min.y;
            out_mesh->bounds_min_z[meshlet_count] = aabb.min.z;
            out_mesh->bounds_max_x[meshlet_count] = aabb.max.x;
            out_mesh->bounds_max_y[meshlet_count] = aabb.max.y;
            out_mesh->bounds_max_z[meshlet_count] = aabb.max.z;

            const sftvec true_axis = sft_v_norm(builder.cone_axis);
            f32 min_dot = 1.f;
            for (u32 t = 0; t < builder.tri_count; t += 1) {
                const u8 v0 = builder.local_inds[t * 3 + 0];
                const u8 v1 = builder.local_inds[t * 3 + 1];
                const u8 v2 = builder.local_inds[t * 3 + 2];
                const sftvec n = sft_v_face_normal(
                    SFT_V(out_mesh->meshlets[meshlet_count].x[v0], out_mesh->meshlets[meshlet_count].y[v0], out_mesh->meshlets[meshlet_count].z[v0]),
                    SFT_V(out_mesh->meshlets[meshlet_count].x[v1], out_mesh->meshlets[meshlet_count].y[v1], out_mesh->meshlets[meshlet_count].z[v1]),
                    SFT_V(out_mesh->meshlets[meshlet_count].x[v2], out_mesh->meshlets[meshlet_count].y[v2], out_mesh->meshlets[meshlet_count].z[v2]));
                const f32 dot_val = sft_v_dot(n, true_axis);
                if (dot_val < min_dot) {
                    min_dot = dot_val;
                }
            }
            
            const f32 sin_alpha = (min_dot < 0.f) ? 1.f : sqrtf(1.f - min_dot * min_dot);
            out_mesh->cone_axis_x[meshlet_count] = true_axis.x;
            out_mesh->cone_axis_y[meshlet_count] = true_axis.y;
            out_mesh->cone_axis_z[meshlet_count] = true_axis.z;
            out_mesh->cone_cutoff[meshlet_count] = sin_alpha + 0.02f;

            meshlet_count += 1;
        }

        out_mesh->meshlet_count = meshlet_count;
        active_meshes += 1;

        free(scratch_weights);
        free(vertex_offsets);
        free(adjacency);
        free(write_heads);
        free(used_tris);
        free(frontier);
        free(in_frontier);
        free(uni_inds);
        free(uni_vx);
        free(uni_vy);
        free(uni_vz);
        free(uni_nx);
        free(uni_ny);
        free(uni_nz);
        free(uni_u);
        free(uni_v);
    }

    model->mesh_count = active_meshes;

    // ---  generate model relative offsets and lookups  ---
    model->total_meshlets = 0;
    for (u32 i = 0; i < active_meshes; i += 1) {
        model->meshes[i].model_meshlet_offset = model->total_meshlets;
        model->total_meshlets += model->meshes[i].meshlet_count;
    }

    if (model->total_meshlets > 0) {
        model->meshlet_mesh_inds = (u16*)sft_arena_alloc(arena, model->total_meshlets * sizeof(u16));
        for (u32 i = 0; i < active_meshes; i += 1) {
            for (u32 j = 0; j < model->meshes[i].meshlet_count; j += 1) {
                // map the global ind back to mesh ind 'i'
                model->meshlet_mesh_inds[model->meshes[i].model_meshlet_offset + j] = (u16)i;
            }
        }

        sft__blas_build_model(model, arena);
    }

    sft__generate_shadow_proxies(model, arena, shadow_proxy_budget);

    // ---  parse animations  ---
    model->anim_count = (i32)data->animations_count;
    if (model->anim_count > 0) {
        model->animations = sft_arena_alloc(arena, model->anim_count * sizeof(struct sft_animation));

        for (i32 i = 0; i < model->anim_count; i += 1) {
            const cgltf_animation* const ca = &data->animations[i];
            struct sft_animation* const sa = &model->animations[i];

            sa->duration = 0.f;
            sa->sampler_count = (i32)ca->samplers_count;
            sa->channel_count = (i32)ca->channels_count;
            sa->samplers = sft_arena_alloc(arena, sa->sampler_count * sizeof(struct sft_anim_sampler));
            sa->channels = sft_arena_alloc(arena, sa->channel_count * sizeof(struct sft_anim_channel));

            for (i32 j = 0; j < sa->sampler_count; j += 1) {
                const cgltf_animation_sampler* const cs = &ca->samplers[j];
                struct sft_anim_sampler* const ss = &sa->samplers[j];

                ss->count = (i32)cs->input->count;
                ss->inputs = sft_arena_alloc(arena, ss->count * sizeof(f32));

                ss->is_cubic = (cgltf_interpolation_type_cubic_spline == cs->interpolation);
                ss->is_step = (cgltf_interpolation_type_step == cs->interpolation);

                // cubic splines require 3x the output data (in-tangent, value, out-tangent)
                const i32 output_count = ss->is_cubic ? (ss->count * 3) : ss->count;
                ss->outputs = sft_arena_alloc(arena, output_count * 4 * sizeof(f32));

                for (i32 k = 0; k < ss->count; k += 1) {
                    cgltf_accessor_read_float(cs->input, k, &ss->inputs[k], 1);
                }
                for (i32 k = 0; k < output_count; k += 1) {
                    cgltf_accessor_read_float(cs->output, k, &ss->outputs[k * 4], 4);
                }

                if (ss->count > 0 && ss->inputs[ss->count - 1] > sa->duration) {
                    sa->duration = ss->inputs[ss->count - 1];
                }
            }

            for (i32 j = 0; j < sa->channel_count; j += 1) {
                const cgltf_animation_channel* const cc = &ca->channels[j];
                struct sft_anim_channel* const sc = &sa->channels[j];

                // target node maps 1 to 1 with skeleton bone inds
                sc->node_ind = (i32)(cc->target_node - data->nodes);
                sc->sampler_ind = (i32)(cc->sampler - ca->samplers);

                switch (cc->target_path) {
                    case cgltf_animation_path_type_translation: sc->path = SFT_ANIM_PATH_TRANSLATION; break;
                    case cgltf_animation_path_type_rotation:    sc->path = SFT_ANIM_PATH_ROTATION; break;
                    case cgltf_animation_path_type_scale:       sc->path = SFT_ANIM_PATH_SCALE; break;
                    default: break;
                }
            }

            // clean coords for left handed Z flipped space
            u8* patched = (u8*)calloc(sa->sampler_count, 1);
            for (i32 j = 0; j < sa->channel_count; j += 1) {
                struct sft_anim_channel* const sc = &sa->channels[j];
                if (patched[sc->sampler_ind]) {
                    continue;
                }
                patched[sc->sampler_ind] = 1;

                struct sft_anim_sampler* const ss = &sa->samplers[sc->sampler_ind];
                const i32 num_outputs = ss->is_cubic ? ss->count * 3 : ss->count;

                if (SFT_ANIM_PATH_TRANSLATION == sc->path) {
                    for (i32 k = 0; k < num_outputs; k += 1) {
                        ss->outputs[k * 4 + 2] = -ss->outputs[k * 4 + 2]; // negate Z translation
                    }
                }
                else if (SFT_ANIM_PATH_ROTATION == sc->path) {
                    for (i32 k = 0; k < num_outputs; k += 1) {
                        ss->outputs[k * 4 + 0] = -ss->outputs[k * 4 + 0]; // negate X rotation
                        ss->outputs[k * 4 + 1] = -ss->outputs[k * 4 + 1]; // negate Y rotation
                        
                        // normalize actual values to prevent matrix shearing
                        if (!ss->is_cubic || (k % 3 == 1)) {
                            const f32 qx = ss->outputs[k * 4 + 0], qy = ss->outputs[k * 4 + 1];
                            const f32 qz = ss->outputs[k * 4 + 2], qw = ss->outputs[k * 4 + 3];
                            const f32 len = sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
                            if (len > SFT_EPSILON) {
                                ss->outputs[k * 4 + 0] /= len;
                                ss->outputs[k * 4 + 1] /= len;
                                ss->outputs[k * 4 + 2] /= len;
                                ss->outputs[k * 4 + 3] /= len;
                            }
                        }
                    }
                }
            }
            free(patched);
        }
    }
    else {
        model->animations = NULL;
    }

    cgltf_free(data);
    free(file_content);
    free(mat_handles);

    return model;
}

#endif // SFT_USE_CGLTF

SFT_FUNC sft_model* sft_model_load_qsf(const char* path, sft_arena* arena) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        SFT_ERR_RET(NULL, "failed to open '%s'\n", path);
    }

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "QSF2", 4) != 0) {
        fclose(f);
        SFT_ERR_RET(NULL, "invalid QSF2 magic in '%s'\n", path);
    }

    u32 version;
    fread(&version, 4, 1, f);
    if (2 != version) {
        fclose(f);
        SFT_ERR_RET(NULL, "unsupported QSF2 version\n");
    }

    // skip the game data offset
    u64 game_data_offset;
    fread(&game_data_offset, 8, 1, f);

    // ---  textures and materials  ---
    u32 tex_count;
    fread(&tex_count, 4, 1, f);
    
    sft_mat_handle* mat_handles = sft_arena_alloc(arena, tex_count * sizeof(sft_mat_handle));
    
    for (u32 i = 0; i < tex_count; i += 1) {
        u32 w, h;
        fread(&w, 4, 1, f);
        fread(&h, 4, 1, f);
        
        u32* pixels = malloc(w * h * sizeof(u32));
        fread(pixels, 4, w * h, f);
        
        const sft_tex_handle tex = sft_texture_create(w, h, pixels);
        mat_handles[i] = sft_material_create(tex, 0xFFFFFFFF, 2.f);
        free(pixels);
    }

    sft_model* model = (sft_model*)sft_arena_alloc(arena, sizeof(sft_model));
    
    // ---  lightmap atlases  ---
    fread(&model->lightmap_count, 4, 1, f);
    if (model->lightmap_count > 0) {
        model->lightmaps = sft_arena_alloc(arena, model->lightmap_count * sizeof(sft_tex_handle));
        for (u32 i = 0; i < model->lightmap_count; i += 1) {
            u32 w, h;
            fread(&w, 4, 1, f);
            fread(&h, 4, 1, f);
            u32* pixels = malloc(w * h * sizeof(u32));
            fread(pixels, 4, w * h, f);
            model->lightmaps[i] = sft_texture_create(w, h, pixels);
            free(pixels);
        }
    }

    // ---  model header  ---
    fread(&model->mesh_count, 4, 1, f);
    fread(&model->total_meshlets, 4, 1, f);
    fread(&model->blas_node_count, 4, 1, f);
    fread(&model->blas_root_ind, 4, 1, f);

    // ---  meshes  ---
    model->meshes = (sft_mesh*)sft_arena_alloc(arena, model->mesh_count * sizeof(sft_mesh));
    for (u32 i = 0; i < model->mesh_count; i += 1) {
        sft_mesh* mesh = &model->meshes[i];
        
        fread(&mesh->meshlet_count, 4, 1, f);
        fread(&mesh->model_meshlet_offset, 4, 1, f);
        
        u32 tex_ind;
        fread(&tex_ind, 4, 1, f);
        mesh->material = (tex_ind < tex_count) ? mat_handles[tex_ind] : sft_state->base_mat;
        fread(&mesh->lightmap_page, 4, 1, f);

        const u32 mc = mesh->meshlet_count;
        
        mesh->meshlets = (struct sft_meshlet*)sft_arena_alloc(arena, mc * sizeof(struct sft_meshlet));
        fread(mesh->meshlets, sizeof(struct sft_meshlet), mc, f);

        mesh->meshlet_attribs = (struct sft_meshlet_attribs*)sft_arena_alloc(arena, mc * sizeof(struct sft_meshlet_attribs));
        fread(mesh->meshlet_attribs, sizeof(struct sft_meshlet_attribs), mc, f);

        mesh->inds = (struct sft_meshlet_inds*)sft_arena_alloc(arena, mc * sizeof(struct sft_meshlet_inds));

        u8 flat_inds[384];
        for (u32 m = 0; m < mc; m += 1) {
            fread(flat_inds, 1, 384, f);
            struct sft_meshlet_inds* m_inds = &mesh->inds[m];
            memset(m_inds, 0, sizeof(struct sft_meshlet_inds));
            for (u32 tri = 0; tri < 128; tri += 1) {
                const u32 batch = tri / SFT_SIMD_WIDTH;
                const u32 lane = tri % SFT_SIMD_WIDTH;
                const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
                m_inds->inds[offset + (0 * SFT_SIMD_WIDTH) + lane] = flat_inds[tri * 3 + 0];
                m_inds->inds[offset + (1 * SFT_SIMD_WIDTH) + lane] = flat_inds[tri * 3 + 1];
                m_inds->inds[offset + (2 * SFT_SIMD_WIDTH) + lane] = flat_inds[tri * 3 + 2];
            }
        }

        // bounding spheres
        mesh->bounds_center_x = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_center_y = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_center_z = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_radius   = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        fread(mesh->bounds_center_x, sizeof(f32), mc, f);
        fread(mesh->bounds_center_y, sizeof(f32), mc, f);
        fread(mesh->bounds_center_z, sizeof(f32), mc, f);
        fread(mesh->bounds_radius, sizeof(f32), mc, f);
        
        // AABBs
        mesh->bounds_min_x = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_min_y = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_min_z = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_max_x = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_max_y = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->bounds_max_z = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        fread(mesh->bounds_min_x, sizeof(f32), mc, f);
        fread(mesh->bounds_min_y, sizeof(f32), mc, f);
        fread(mesh->bounds_min_z, sizeof(f32), mc, f);
        fread(mesh->bounds_max_x, sizeof(f32), mc, f);
        fread(mesh->bounds_max_y, sizeof(f32), mc, f);
        fread(mesh->bounds_max_z, sizeof(f32), mc, f);

        // cone culling
        mesh->cone_axis_x = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->cone_axis_y = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->cone_axis_z = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        mesh->cone_cutoff = (f32*)sft_arena_alloc(arena, mc * sizeof(f32));
        fread(mesh->cone_axis_x, sizeof(f32), mc, f);
        fread(mesh->cone_axis_y, sizeof(f32), mc, f);
        fread(mesh->cone_axis_z, sizeof(f32), mc, f);
        fread(mesh->cone_cutoff, sizeof(f32), mc, f);
    }

    // ---  global topology and BVH  ---
    if (model->total_meshlets > 0) {
        model->meshlet_mesh_inds = (u16*)sft_arena_alloc(arena, model->total_meshlets * sizeof(u16));
        fread(model->meshlet_mesh_inds, sizeof(u16), model->total_meshlets, f);
    }
    
    if (model->blas_node_count > 0) {
        model->blas_nodes = (struct sft_bvh_node*)sft_arena_alloc(arena, model->blas_node_count * sizeof(struct sft_bvh_node));
        fread(model->blas_nodes, sizeof(struct sft_bvh_node), model->blas_node_count, f);
    }

    fclose(f);
    return model;
}


/*================================================================
:: FUNCTION DEFINITIONS (OTHER)
================================================================*/

SFT_FUNC void sft_init(i32 width, i32 height, i32 requested_threads) {
    // SFT_CALLOC(sft_state, 1, struct sft_state);
    sft_state = (struct sft_state*)SFT_ALIGNED_ALLOC(sizeof(struct sft_state), 64);
    if (!sft_state) {
        SFT_ERR_EXIT("failed to allocate aligned sft_state\n");
    }
    memset(sft_state, 0, sizeof(struct sft_state));

    { // ---  initialize multithreading  ---
        if (requested_threads > 0) {
            sft_state->thread_count = requested_threads;
        }
        else {
            sft_state->thread_count = sft__get_physical_core_count();
        }
    
        // initialize barriers and semaphores
        sft__barrier_init(&sft_state->geom_barrier, sft_state->thread_count);
        sft__barrier_init(&sft_state->shadow_barrier, sft_state->thread_count);
        sft__barrier_init(&sft_state->raster_barrier, sft_state->thread_count);
        sft__barrier_init(&sft_state->prefix_barrier, sft_state->thread_count);
        
        sft_sem_init(&sft_state->sem_start, 0);
        sft_sem_init(&sft_state->sem_done, 0);

        // spawn workers
        for (i32 i = 0; i < sft_state->thread_count; i += 1) {
            struct sft_thread_payload* payload = malloc(sizeof(struct sft_thread_payload));
            payload->thread_id = i;
            payload->func = sft__worker_thread;
            payload->user_data = NULL;
            sft_thread_create(payload);
        }
    
        SFT_PRINT("initialized %d worker threads and %d SIMD %s\n",
            sft_state->thread_count, SFT_SIMD_WIDTH, (SFT_SIMD_WIDTH > 1) ? "lanes" : "lane (scalar)");
    }

    { // ---  initialize arenas  ---
        sft_arena_init(&sft_state->instance_arena, 32ULL * 1024 * 1024);
        sft_state->cmd_queue.instances = (sft_instance*)sft_state->instance_arena.base;
        
        sft_arena_init(&sft_state->job_arena, 64ULL * 1024 * 1024);
        sft_state->cmd_queue.jobs = (struct sft_geom_job*)sft_state->job_arena.base;

        sft_arena_init(&sft_state->chunk_arena, 512ULL * 1024 * 1024);
        sft_state->display.chunk_pool = (struct sft_payload_chunk*)sft_state->chunk_arena.base;
        
        sft_arena_init(&sft_state->scratch_arena, 2ULL * 1024 * 1024 * 1024);
        // sft_state->ss_cache = (struct sft_ss_meshlet*)sft_state->scratch_arena.base;

        sft_arena_init(&sft_state->clip_arena, 32ULL * 1024 * 1024);
        sft_state->clip_cache.pool = (struct sft_clipped_triangle*)sft_state->clip_arena.base;

        sft_arena_init(&sft_state->tex_heap_arena, 4ULL * 1024 * 1024 * 1024);
        sft_state->mat_pool.free_head = -1;
        sft_state->tex_pool.free_head = -1;

        sft_arena_init(&sft_state->light_arena, 16ULL * 1024 * 1024);
        // directional at the start and point offset by 8MB
        sft_state->lighting.directional = (sft_directional_light*)sft_state->light_arena.base;
        sft_state->lighting.point = (sft_point_light*)(sft_state->light_arena.base + (8ULL * 1024 * 1024));

        sft_arena_init(&sft_state->shadow_arena, 128ULL * 1024 * 1024);
        sft_state->shadows.proxies = (struct sft_world_shadow_proxy*)sft_state->shadow_arena.base;
    }

    { // ---  initialize display  ---
        sft_state->display.half_width = (f32)width * 0.5f;
        sft_state->display.half_height = (f32)height * 0.5f;
        sft_state->display.vp_x = 0.f;
        sft_state->display.vp_y = 0.f;

        // calculate tile grid
        sft_state->display.tile_cols = (width + SFT_TILE_SIZE - 1) / SFT_TILE_SIZE;
        sft_state->display.tile_rows = (height + SFT_TILE_SIZE - 1) / SFT_TILE_SIZE;
        
        const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;

        SFT_CALLOC(sft_state->thread_bin_states, sft_state->thread_count * total_tiles, struct sft_thread_bin_state);
        SFT_CALLOC(sft_state->display.tile_heads, total_tiles, i32);
        SFT_CALLOC(sft_state->display.tile_tails, total_tiles, volatile i32);
        SFT_CALLOC(sft_state->display.tile_payload_counts, total_tiles, i32);
        SFT_CALLOC(sft_state->display.sorted_tiles, total_tiles, i32);
    }

    { // ---  initialize default material and other defaults  ---
        // 256x256 checkerboard
        const i32 checker_w = 256;
        const i32 checker_h = 256;
        u32* checker_pixels = (u32*)malloc(checker_w * checker_h * sizeof(u32));
        
        const i32 check_size = 32; // 32x32 squares
        for (i32 y = 0, i = 0; y < checker_h; y += 1) {
            for (i32 x = 0; x < checker_w; x += 1, i += 1) {
                const i32 cx = (x / check_size) % 2;
                const i32 cy = (y / check_size) % 2;
                checker_pixels[i] = (cx ^ cy) ? 0xFFAFFFAF : 0xFF555555;
            }
        }

        // push into the texture heap
        const sft_tex_handle checker_tex = sft_texture_create(checker_w, checker_h, checker_pixels);
        free(checker_pixels);

        // create the material
        sft_state->base_mat = sft_material_create(checker_tex, 0xFFFFFFFF, 2.f);

        // default overbrightening
        sft_state->lighting.lightmap_brightness = 1.5f;

        // default background clear color
        sft_state->display.clear_color = 0xFF222222;
    }

    { // ---  initialize dmesh templates  ---
        memset(&sft_state->dmesh.quad_inds, 0, sizeof(struct sft_meshlet_inds));
        memset(&sft_state->dmesh.tri_inds, 0, sizeof(struct sft_meshlet_inds));

        for (u32 tri = 0; tri < 128; tri += 1) {
            const u32 batch = tri / SFT_SIMD_WIDTH;
            const u32 lane  = tri % SFT_SIMD_WIDTH;
            const u32 offset = batch * (SFT_SIMD_WIDTH * 4);

            // quad template, 16 quads => 32 tris
            if (tri < 32) {
                const u32 q = tri / 2;
                const u8 bv = (u8)(q * 4);

                // tri 0: (0, 1, 2), tri 1: (0, 2, 3)
                const u8 v0 = bv;
                const u8 v1 = (0 == tri % 2) ? (bv + 1) : (bv + 2);
                const u8 v2 = (0 == tri % 2) ? (bv + 2) : (bv + 3);

                sft_state->dmesh.quad_inds.inds[offset + (0 * SFT_SIMD_WIDTH) + lane] = v0;
                sft_state->dmesh.quad_inds.inds[offset + (1 * SFT_SIMD_WIDTH) + lane] = v1;
                sft_state->dmesh.quad_inds.inds[offset + (2 * SFT_SIMD_WIDTH) + lane] = v2;
            }

            // tri template, 21 tris
            if (tri < 21) {
                const u8 bv = (u8)(tri * 3);

                sft_state->dmesh.tri_inds.inds[offset + (0 * SFT_SIMD_WIDTH) + lane] = bv + 0;
                sft_state->dmesh.tri_inds.inds[offset + (1 * SFT_SIMD_WIDTH) + lane] = bv + 1;
                sft_state->dmesh.tri_inds.inds[offset + (2 * SFT_SIMD_WIDTH) + lane] = bv + 2;
            }
        }
    }
}

SFT_FUNC void sft_release(void) {
    SFT_PRINT("releasing resources...\n");
    SFT_PRINT("arena commited sizes:\n");
    SFT_PRINT("  chunk:    %u KB\n", (u32)sft_state->chunk_arena.committed_size / 1024);
    SFT_PRINT("  job:      %u KB\n", (u32)sft_state->job_arena.committed_size / 1024);
    SFT_PRINT("  scratch:  %u KB\n", (u32)sft_state->scratch_arena.committed_size / 1024);
    SFT_PRINT("  instance: %u KB\n", (u32)sft_state->instance_arena.committed_size / 1024);
    SFT_PRINT("  clip:     %u KB\n", (u32)sft_state->clip_arena.committed_size / 1024);
    SFT_PRINT("  tex heap: %u KB\n", (u32)sft_state->tex_heap_arena.committed_size / 1024);
    SFT_PRINT("  shadows:  %u KB\n", (u32)sft_state->shadow_arena.committed_size / 1024);

    SFT_ALIGNED_FREE(sft_state);
}

SFT_FUNC void sft_present(void) {
    // reset frame transient atomic counters
    sft_state->next_job_ind.count = 0;
    sft_state->next_tile_ind.count = 0;
    
    const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;

    // reset the lock free global chunk allocator
    sft_state->next_chunk_ind.count = 0;

    // set all tile heads and tails to -1
    memset(sft_state->display.tile_heads, 0xFF, total_tiles * sizeof(i32));
    memset((void*)sft_state->display.tile_tails, 0xFF, total_tiles * sizeof(i32));
    memset(sft_state->display.tile_payload_counts, 0, total_tiles * sizeof(i32));

    if (sft_state->cmd_queue.total_ss_meshlets.count > 0) {
        sft_state->ss_cache = (struct sft_ss_meshlet*)sft_arena_alloc(
            &sft_state->scratch_arena, 
            sft_state->cmd_queue.total_ss_meshlets.count * sizeof(struct sft_ss_meshlet));
    }

    // build the frame TLAS before workers start geometry processing
    sft__tlas_build();

    // ---  multi ray occlusion pass (TODO move this into its own phase?)  ---
    if (sft_state->shadows.enabled && sft_state->lighting.point_count.count > 0 && sft_state->shadows.proxy_count.count > 0) {
        const i32 p_count = sft_state->shadows.proxy_count.count;
        const i32 l_count = SFT_MIN(sft_state->lighting.point_count.count, 64);
        
        // init all proxies to fully visible
        for (i32 p = 0; p < p_count; p += 1) {
            for (i32 l = 0; l < l_count; l += 1) {
                sft_state->shadows.proxies[p].light_vis[l] = 3;
            }
        }

        for (i32 l = 0; l < l_count; l += 1) {
            const sft_point_light* light = &sft_state->lighting.point[l];
            
            for (i32 p_base = 0; p_base < p_count; p_base += SFT_SIMD_WIDTH) {
                // cluster center for this proxy batch to face the light disk toward the proxy
                sftvec batch_center = {0};
                i32 active_in_batch = 0;
                for (i32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                    if (p_base + lane < p_count) {
                        const struct sft_world_shadow_proxy* px = &sft_state->shadows.proxies[p_base + lane];
                        batch_center.x += (px->posa.x + px->posb.x) * 0.5f;
                        batch_center.y += (px->posa.y + px->posb.y) * 0.5f;
                        batch_center.z += (px->posa.z + px->posb.z) * 0.5f;
                        active_in_batch += 1;
                    }
                }
                if (active_in_batch > 0) {
                    batch_center = sft_v_mul(batch_center, 1.f / (f32)active_in_batch);
                }

                // 2D tangent disk on the light facing the dynamic geometry
                const sftvec to_light = sft_v_sub(light->pos, batch_center);
                const sftvec l_dir = sft_v_norm(to_light);
                const sftvec l_right = sft_v_norm(sft_v_perpendicular(l_dir));
                const sftvec l_up = sft_v_cross(l_right, l_dir);

                // 3 target points at 120 degree intervals on the light source disk
                const f32 r = SFT_SHADOW_LIGHT_SIZE;
                const sftvec targets[3] = {
                    // top
                    sft_v_add(light->pos, sft_v_mul(l_up, r)),
                    // bottom right
                    sft_v_add(light->pos, sft_v_add(sft_v_mul(l_right, r * 0.866025f), sft_v_mul(l_up, -r * 0.5f))),
                    // bottom left
                    sft_v_add(light->pos, sft_v_add(sft_v_mul(l_right, -r * 0.866025f), sft_v_mul(l_up, -r * 0.5f)))
                };

                for (i32 ray_ind = 0; ray_ind < 3; ray_ind += 1) {
                    alignas(32) f32 ox[SFT_SIMD_WIDTH], oy[SFT_SIMD_WIDTH], oz[SFT_SIMD_WIDTH];
                    alignas(32) f32 lx[SFT_SIMD_WIDTH], ly[SFT_SIMD_WIDTH], lz[SFT_SIMD_WIDTH];
                    alignas(32) i32 am[SFT_SIMD_WIDTH];
                    
                    const sftvec target = targets[ray_ind];

                    for (i32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                        if (p_base + lane < p_count) {
                            const struct sft_world_shadow_proxy* px = &sft_state->shadows.proxies[p_base + lane];
                            
                            f32 px_x, px_y, px_z;
                            if (0 == ray_ind) { 
                                px_x = px->posa.x;
                                px_y = px->posa.y;
                                px_z = px->posa.z;
                            }
                            else if (1 == ray_ind) { 
                                px_x = px->posb.x;
                                px_y = px->posb.y;
                                px_z = px->posb.z;

                            }
                            else { 
                                px_x = (px->posa.x + px->posb.x) * 0.5f;
                                px_y = (px->posa.y + px->posb.y) * 0.5f;
                                px_z = (px->posa.z + px->posb.z) * 0.5f;

                            }
                            
                            ox[lane] = px_x;
                            oy[lane] = px_y;
                            oz[lane] = px_z;
                            
                            // vector from proxy origin to the offset target on the light
                            lx[lane] = target.x - px_x;
                            ly[lane] = target.y - px_y;
                            lz[lane] = target.z - px_z;
                            am[lane] = 0xFFFFFFFF;
                        }
                        else {
                            ox[lane] = 0.f;
                            oy[lane] = 0.f;
                            oz[lane] = 0.f;

                            lx[lane] = 0.f;
                            ly[lane] = 0.f;
                            lz[lane] = 0.f;

                            am[lane] = 0;
                        }
                    }
                    
                    const sftvvec v_orig = { sft_vf_load(ox), sft_vf_load(oy), sft_vf_load(oz) };
                    const sftvvec v_L = { sft_vf_load(lx), sft_vf_load(ly), sft_vf_load(lz) };
                    const vf32 v_active = sft_vf_cast_vi32(sft_vi_load(am));
                    
                    const vf32 occlusion = sft__trace_shadow_packet(v_orig, v_L, v_active, light->pos, light->radius, 1);
                    
                    const i32 occ_mask = sft_vf_movemask(occlusion);
                    for (i32 lane = 0; lane < SFT_SIMD_WIDTH; lane += 1) {
                        if (occ_mask & (1 << lane)) {
                            sft_state->shadows.proxies[p_base + lane].light_vis[l] -= 1;
                        }
                    }
                }
            }
        }
    }

    // wake all worker threads to process the queue
    sft_sem_post(&sft_state->sem_start, sft_state->thread_count);

    // block the main thread until all workers report finished
    for (i32 i = 0; i < sft_state->thread_count; i += 1) {
        sft_sem_wait(&sft_state->sem_done);
    }
}

SFT_FUNC void sft_frame_begin(void) {
    sft_arena_frame_begin(&sft_state->instance_arena);
    sft_arena_frame_begin(&sft_state->job_arena);
    sft_arena_frame_begin(&sft_state->chunk_arena);
    sft_arena_frame_begin(&sft_state->scratch_arena);
    sft_arena_frame_begin(&sft_state->clip_arena);
    sft_arena_frame_begin(&sft_state->light_arena);
    sft_arena_frame_begin(&sft_state->shadow_arena);
}

SFT_FUNC void sft_frame_end(void) {
    sft_state->cmd_queue.instance_count.count = 0;
    sft_state->cmd_queue.job_count.count = 0;
    sft_state->next_job_ind.count = 0;
    sft_state->next_tile_ind.count = 0;
    sft_state->cmd_queue.total_ss_meshlets.count = 0;
    sft_state->clip_cache.count = 0;
    sft_state->mat_pool_transient.count = 0;
    sft_state->lighting.directional_count.count = 0;
    sft_state->lighting.point_count.count = 0;
    sft_state->shadows.proxy_count.count = 0;
}

SFT_FUNC void sft_resize(i32 width, i32 height) {
    if (!sft_state) {
        return;
    }

    // update screen dimensions
    sft_state->display.half_width = (f32)width * 0.5f;
    sft_state->display.half_height = (f32)height * 0.5f;

    // recalculate tile grid
    sft_state->display.tile_cols = (width + SFT_TILE_SIZE - 1) / SFT_TILE_SIZE;
    sft_state->display.tile_rows = (height + SFT_TILE_SIZE - 1) / SFT_TILE_SIZE;
    const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;

    // ---  reallocate screen dependent and tile dependent buffers  ---
    sft_state->thread_bin_states = (struct sft_thread_bin_state*)realloc(
        sft_state->thread_bin_states, sft_state->thread_count * total_tiles * sizeof(struct sft_thread_bin_state));
    
    sft_state->display.tile_heads = (i32*)realloc(
        sft_state->display.tile_heads, total_tiles * sizeof(i32));
        
    sft_state->display.tile_tails = (volatile i32*)realloc(
        (void*)sft_state->display.tile_tails, total_tiles * sizeof(volatile i32));
        
    sft_state->display.tile_payload_counts = (i32*)realloc(
        sft_state->display.tile_payload_counts, total_tiles * sizeof(i32));
        
    sft_state->display.sorted_tiles = (i32*)realloc(
        sft_state->display.sorted_tiles, total_tiles * sizeof(i32));
}

SFT_FUNC void sft_set_camera(sftvec cam_pos, sftmat view, sftmat proj) {
    sft_state->camera_pos = cam_pos;
    sft_state->view = view, sft_state->proj = proj;
    sft_state->view_proj = sft_m_mul(view, proj);

    // extract frustum planes
    sftmat* vp = &sft_state->view_proj;
    f32 planes[6][4];

    // left, right, bottom, top, near, far
    for (i32 i = 0; i < 4; i += 1) {
        planes[0][i] = vp->m[i][3] + vp->m[i][0];
        planes[1][i] = vp->m[i][3] - vp->m[i][0];
        planes[2][i] = vp->m[i][3] + vp->m[i][1];
        planes[3][i] = vp->m[i][3] - vp->m[i][1];
        planes[4][i] = vp->m[i][3] + vp->m[i][2];
        planes[5][i] = vp->m[i][3] - vp->m[i][2];
    }

    const vi32 abs_mask = sft_vi_set1(0x7FFFFFFF);
    for (i32 p = 0; p < 6; p += 1) {
        const f32 length = sqrtf(planes[p][0]*planes[p][0] + planes[p][1]*planes[p][1] + planes[p][2]*planes[p][2]);
        const f32 inv_len = (length < SFT_EPSILON) ? SFT_EPSILON : 1.f / length;
        sft_state->frustum.normal[p].x = sft_vf_set1(planes[p][0] * inv_len);
        sft_state->frustum.normal[p].y = sft_vf_set1(planes[p][1] * inv_len);
        sft_state->frustum.normal[p].z = sft_vf_set1(planes[p][2] * inv_len);
        sft_state->frustum.abs_normal[p].x = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(sft_state->frustum.normal[p].x), abs_mask));
        sft_state->frustum.abs_normal[p].y = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(sft_state->frustum.normal[p].y), abs_mask));
        sft_state->frustum.abs_normal[p].z = sft_vf_cast_vi32(sft_vi_and(sft_vi_cast_vf32(sft_state->frustum.normal[p].z), abs_mask));
        sft_state->frustum.d[p] = sft_vf_set1(planes[p][3] * inv_len);
    }
}

SFT_FUNC void sft_set_debug_mode(enum sft_debug_mode mode) {
    if (mode < 0 || mode >= SFT_DEBUG_COUNT) {
        SFT_ERR_RET(, "invalid debug mode '%d', expected [0, %d)\n", mode, SFT_DEBUG_COUNT);
    }
    sft_state->display.debug_mode = mode;
}

SFT_FUNC const char* sft_get_debug_name(enum sft_debug_mode mode) {
    switch (mode) {
        case SFT_DEBUG_NONE: return "None";
        case SFT_DEBUG_WIREFRAME: return "Wireframe";
        case SFT_DEBUG_TILES: return "Tiles";
        case SFT_DEBUG_DEPTH: return "Depth";
        case SFT_DEBUG_LIGHT_COUNT: return "Light Count";
        case SFT_DEBUG_CHECKERBOARD: return "Checkerboard";
        default: return "INVALID MODE";
    }
}

SFT_FUNC void sft_set_clear(u8 enabled) {
    sft_state->display.should_clear = enabled;
}

SFT_FUNC void sft_set_clear_color(u32 color) {
    sft_state->display.clear_color = color;
}

SFT_FUNC void sft_set_color_buffer(u32* buf, i32 pitch_in_bytes) {
    sft_state->display.color_buffer = buf;
    sft_state->display.color_pitch = pitch_in_bytes / sizeof(u32);
}

SFT_FUNC void sft_draw_instance(const sft_instance* inst) {
    // claim instance slot
    const i32 inst_slot = sft_atomic_add(&sft_state->cmd_queue.instance_count.count, 1);
    
    // strict bit packing validation guard
    if (inst_slot >= SFT_MAX_INSTANCES_PER_FRAME) {
        return; 
    }

    sft_arena_commit_if_needed(&sft_state->instance_arena, (inst_slot + 1) * sizeof(sft_instance));
    sft_state->cmd_queue.instances[inst_slot] = *inst;

    sft_state->cmd_queue.instances[inst_slot].ss_meshlet_offset =
        sft_atomic_add(&sft_state->cmd_queue.total_ss_meshlets.count, inst->model->total_meshlets);

    // ---  transform and store shadow proxies  ---
    if (sft_state->shadows.enabled && inst->model && inst->model->shadow_proxy_count > 0) {
        const u32 p_count = inst->model->shadow_proxy_count;
        const i32 base_slot = sft_atomic_add(&sft_state->shadows.proxy_count.count, p_count);
        
        sft_arena_commit_if_needed(&sft_state->shadow_arena, (base_slot + p_count) * sizeof(struct sft_world_shadow_proxy));

        for (u32 i = 0; i < p_count; i += 1) {
            const struct sft_shadow_proxy* local = &inst->model->shadow_proxies[i];
            
            // use the base slot
            struct sft_world_shadow_proxy* world = &sft_state->shadows.proxies[base_slot + i];
            
            sftmat m = inst->transform;
            if (local->bone_ind != 0xFFFFFFFF && inst->posed_bone_matrices) {
                m = sft_m_mul(inst->posed_bone_matrices[local->bone_ind], inst->transform);
            }
            
            // instead of: world->posa = sft_m_mul_vec(m, local->posa);
            world->posa.x = local->posa.x * m.m[0][0] + local->posa.y * m.m[1][0] + local->posa.z * m.m[2][0] + m.m[3][0];
            world->posa.y = local->posa.x * m.m[0][1] + local->posa.y * m.m[1][1] + local->posa.z * m.m[2][1] + m.m[3][1];
            world->posa.z = local->posa.x * m.m[0][2] + local->posa.y * m.m[1][2] + local->posa.z * m.m[2][2] + m.m[3][2];

            world->posb.x = local->posb.x * m.m[0][0] + local->posb.y * m.m[1][0] + local->posb.z * m.m[2][0] + m.m[3][0];
            world->posb.y = local->posb.x * m.m[0][1] + local->posb.y * m.m[1][1] + local->posb.z * m.m[2][1] + m.m[3][1];
            world->posb.z = local->posb.x * m.m[0][2] + local->posb.y * m.m[1][2] + local->posb.z * m.m[2][2] + m.m[3][2];
            
            const f32 sx = sqrtf(m.m[0][0]*m.m[0][0] + m.m[0][1]*m.m[0][1] + m.m[0][2]*m.m[0][2]);
            world->radius = local->radius * sx;
        }
    }

    // slice geometry into jobs
    for (u32 m = 0; m < inst->model->mesh_count; m += 1) {
        const struct sft_mesh* mesh = &inst->model->meshes[m];
        u32 meshlets_remaining = mesh->meshlet_count;
        u32 current_start = 0;

        while (meshlets_remaining > 0) {
            const u32 count = SFT_CLAMP((i32)meshlets_remaining, 0, SFT_MESHLETS_PER_JOB);
            const i32 job_slot = sft_atomic_add(&sft_state->cmd_queue.job_count.count, 1);
            
            sft_arena_commit_if_needed(&sft_state->job_arena, (job_slot + 1) * sizeof(struct sft_geom_job));
            
            sft_state->cmd_queue.jobs[job_slot].instance_ind  = (u16)inst_slot;
            sft_state->cmd_queue.jobs[job_slot].mesh_ind      = (u16)m;
            sft_state->cmd_queue.jobs[job_slot].meshlet_start = current_start;
            sft_state->cmd_queue.jobs[job_slot].meshlet_count = count;

            current_start += count;
            meshlets_remaining -= count;
        }
    }
}

SFT_FUNC void sft_draw_cube(sft_dmesh* dm) {
    // bottom left, top left, top right, bottom right
    // front face
    sft_dmesh_push_quad(dm,
        SFT_V(-0.5f,-0.5f,-0.5f), SFT_V(-0.5f, 0.5f,-0.5f), SFT_V( 0.5f, 0.5f,-0.5f), SFT_V( 0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 0.f, -1.f));
    // right face
    sft_dmesh_push_quad(dm,
        SFT_V( 0.5f,-0.5f,-0.5f), SFT_V( 0.5f, 0.5f,-0.5f), SFT_V( 0.5f, 0.5f, 0.5f), SFT_V( 0.5f,-0.5f, 0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(1.f, 0.f, 0.f));
    // back face
    sft_dmesh_push_quad(dm,
        SFT_V( 0.5f,-0.5f, 0.5f), SFT_V( 0.5f, 0.5f, 0.5f), SFT_V(-0.5f, 0.5f, 0.5f), SFT_V(-0.5f,-0.5f, 0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 0.f, 1.f));
    // left face
    sft_dmesh_push_quad(dm,
        SFT_V(-0.5f,-0.5f, 0.5f), SFT_V(-0.5f, 0.5f, 0.5f), SFT_V(-0.5f, 0.5f,-0.5f), SFT_V(-0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(-1.f, 0.f, 0.f));
    // top face
    sft_dmesh_push_quad(dm,
        SFT_V(-0.5f, 0.5f,-0.5f), SFT_V(-0.5f, 0.5f, 0.5f), SFT_V( 0.5f, 0.5f, 0.5f), SFT_V( 0.5f, 0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 1.f, 0.f));
    // bottom face
    sft_dmesh_push_quad(dm,
        SFT_V( 0.5f,-0.5f, 0.5f), SFT_V(-0.5f,-0.5f, 0.5f), SFT_V(-0.5f,-0.5f,-0.5f), SFT_V( 0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, -1.f, 0.f));
}

SFT_FUNC void sft_draw_cube_inv(sft_dmesh* dm) {
    // top right, top left, bottom left, bottom right
    sft_dmesh_push_quad(dm, 
        SFT_V( 0.5f, 0.5f,-0.5f), SFT_V(-0.5f, 0.5f,-0.5f), SFT_V(-0.5f,-0.5f,-0.5f), SFT_V( 0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 0.f, 1.f));
    sft_dmesh_push_quad(dm, 
        SFT_V( 0.5f, 0.5f, 0.5f), SFT_V( 0.5f, 0.5f,-0.5f), SFT_V( 0.5f,-0.5f,-0.5f), SFT_V( 0.5f,-0.5f, 0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(-1.f, 0.f, 0.f));
    sft_dmesh_push_quad(dm, 
        SFT_V(-0.5f, 0.5f, 0.5f), SFT_V( 0.5f, 0.5f, 0.5f), SFT_V( 0.5f,-0.5f, 0.5f), SFT_V(-0.5f,-0.5f, 0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 0.f, -1.f));
    sft_dmesh_push_quad(dm, 
        SFT_V(-0.5f, 0.5f,-0.5f), SFT_V(-0.5f, 0.5f, 0.5f), SFT_V(-0.5f,-0.5f, 0.5f), SFT_V(-0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(1.f, 0.f, 0.f));
    sft_dmesh_push_quad(dm, 
        SFT_V( 0.5f, 0.5f, 0.5f), SFT_V(-0.5f, 0.5f, 0.5f), SFT_V(-0.5f, 0.5f,-0.5f), SFT_V( 0.5f, 0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, -1.f, 0.f));
    sft_dmesh_push_quad(dm, 
        SFT_V(-0.5f,-0.5f,-0.5f), SFT_V(-0.5f,-0.5f, 0.5f), SFT_V( 0.5f,-0.5f, 0.5f), SFT_V( 0.5f,-0.5f,-0.5f), 
        0.f,0.f, 1.f,1.f, SFT_V(0.f, 1.f, 0.f));
}

// essentially copied from raylib source: rmodels.c, DrawSphereEx
SFT_FUNC void sft_draw_sphere(sft_dmesh* dm, i32 rings, i32 slices) {
    // angle between latitudinal parallels
    // const f32 ringAngle = DEG2RAD * (180.f / (rings + 1));
    const f32 ring_angle = SFT_PI / (rings + 1);

    // angle between longitudinal meridians
    // const f32 sliceAngle = DEG2RAD * (360.f / (slices);
    const f32 slice_angle = 2.f * SFT_PI / slices;

    const f32 cos_ring = cosf(ring_angle);
    const f32 sin_ring = sinf(ring_angle);
    const f32 cos_slice = cosf(slice_angle);
    const f32 sin_slice = sinf(slice_angle);

    // required to store face vertices
    sftvec verts[4] = {0};
    verts[2] = SFT_V(0.f, 1.f, 0.f);
    verts[3] = SFT_V(sin_ring, cos_ring, 0.f);

    for (i32 i = 0; i <= rings; i += 1) {
        for (i32 j = 0; j < slices; j += 1) {
            verts[0] = verts[2];
            verts[1] = verts[3];
            // rotation matrix around y axis
            verts[2] = SFT_V(cos_slice*verts[2].x - sin_slice*verts[2].z, verts[2].y, sin_slice*verts[2].x + cos_slice*verts[2].z);
            verts[3] = SFT_V(cos_slice*verts[3].x - sin_slice*verts[3].z, verts[3].y, sin_slice*verts[3].x + cos_slice*verts[3].z);

            // for a sphere centered at the origin the normalized position is the normal
            const sftvec n0 = sft_v_norm(verts[0]);
            const sftvec n1 = sft_v_norm(verts[2]);
            const sftvec n2 = sft_v_norm(verts[3]);
            const sftvec n3 = sft_v_norm(verts[1]);

            // the vertex ordering matches the position ordering
            sft_dmesh_push_quad_4n(dm, 
                verts[0], verts[2], verts[3], verts[1], 
                0.f, 0.f, 1.f, 1.f, 
                n0, n1, n2, n3);
        }

        // rotate around z axis to set up starting verts for next ring
        verts[2] = verts[3];
        // rotation matrix around z axis
        verts[3] = SFT_V(cos_ring*verts[3].x + sin_ring*verts[3].y, -sin_ring*verts[3].x + cos_ring*verts[3].y, verts[3].z);
    }
}

// essentially copied from raylib source: rmodels.c, DrawCapsule
SFT_FUNC void sft_draw_capsule(sft_dmesh* dm, sftvec start_pos, sftvec end_pos, f32 radius, i32 slices, i32 rings) {
    if (slices < 3) {
        slices = 3;
    }

    sftvec dir = sft_v_sub(end_pos, start_pos);

    // draw a sphere if start and end point are the same
    const u8 sphere_case = (0.f == dir.x && 0.f == dir.y && 0.f == dir.z);
    if (sphere_case) {
        dir = SFT_V(0.f, 1.f, 0.f);
    }

    // construct a basic of the base and the caps
    sftvec b0 = sft_v_norm(dir);
    const sftvec b1 = sft_v_norm(sft_v_perpendicular(dir));
    const sftvec b2 = sft_v_norm(sft_v_cross(b1, dir));
    sftvec cap_center = end_pos;

    const f32 base_slice_angle = 2.f * SFT_PI / slices;
    const f32 base_ring_angle = 0.5f * SFT_PI / rings;

    // render both caps
    for (i32 c = 0; c < 2; c += 1) {
        for (i32 i = 0; i < rings; i += 1) {
            for (i32 j = 0; j < slices; j += 1) {
                // building up the rings from capCenter in the direction of the 'dir' vector

                // compute the four vertices
                const f32 rS1 = sinf(base_slice_angle*(j + 0))*cosf(base_ring_angle*(i + 0));
                const f32 rC1 = cosf(base_slice_angle*(j + 0))*cosf(base_ring_angle*(i + 0));
                const sftvec w1 = SFT_V(
                    cap_center.x + (sinf(base_ring_angle*(i + 0))*b0.x + rS1*b1.x + rC1*b2.x)*radius,
                    cap_center.y + (sinf(base_ring_angle*(i + 0))*b0.y + rS1*b1.y + rC1*b2.y)*radius,
                    cap_center.z + (sinf(base_ring_angle*(i + 0))*b0.z + rS1*b1.z + rC1*b2.z)*radius);

                const f32 rS2 = sinf(base_slice_angle*(j + 1))*cosf(base_ring_angle*(i + 0));
                const f32 rC2 = cosf(base_slice_angle*(j + 1))*cosf(base_ring_angle*(i + 0));
                const sftvec w2 = SFT_V(
                    cap_center.x + (sinf(base_ring_angle*(i + 0))*b0.x + rS2*b1.x + rC2*b2.x)*radius,
                    cap_center.y + (sinf(base_ring_angle*(i + 0))*b0.y + rS2*b1.y + rC2*b2.y)*radius,
                    cap_center.z + (sinf(base_ring_angle*(i + 0))*b0.z + rS2*b1.z + rC2*b2.z)*radius);

                const f32 rS3 = sinf(base_slice_angle*(j + 0))*cosf(base_ring_angle*(i + 1));
                const f32 rC3 = cosf(base_slice_angle*(j + 0))*cosf(base_ring_angle*(i + 1));
                const sftvec w3 = SFT_V(
                    cap_center.x + (sinf(base_ring_angle*(i + 1))*b0.x + rS3*b1.x + rC3*b2.x)*radius,
                    cap_center.y + (sinf(base_ring_angle*(i + 1))*b0.y + rS3*b1.y + rC3*b2.y)*radius,
                    cap_center.z + (sinf(base_ring_angle*(i + 1))*b0.z + rS3*b1.z + rC3*b2.z)*radius);

                const f32 rS4 = sinf(base_slice_angle*(j + 1))*cosf(base_ring_angle*(i + 1));
                const f32 rC4 = cosf(base_slice_angle*(j + 1))*cosf(base_ring_angle*(i + 1));
                const sftvec w4 = SFT_V(
                    cap_center.x + (sinf(base_ring_angle*(i + 1))*b0.x + rS4*b1.x + rC4*b2.x)*radius,
                    cap_center.y + (sinf(base_ring_angle*(i + 1))*b0.y + rS4*b1.y + rC4*b2.y)*radius,
                    cap_center.z + (sinf(base_ring_angle*(i + 1))*b0.z + rS4*b1.z + rC4*b2.z)*radius);

                // average face position
                const sftvec face_center = sft_v_mul(sft_v_add(sft_v_add(w1, w2), sft_v_add(w3, w4)), 0.25f);
                
                // normal is the direction from the hemisphere's origin to the face
                const sftvec face_normal = sft_v_norm(sft_v_sub(face_center, cap_center));

                // make sure cap triangle normals are facing outwards
                if (0 == c) {
                    sft_dmesh_push_quad(dm, w3, w1, w2, w4, 0.f, 0.f, 1.f, 1.f, face_normal);
                }
                else {
                    // reversed for the bottom cap
                    sft_dmesh_push_quad(dm, w3, w4, w2, w1, 0.f, 0.f, 1.f, 1.f, face_normal);
                }
            }
        }

        cap_center = start_pos;
        b0 = sft_v_mul(b0, -1.f);
    }

    // render middle
    if (!sphere_case) {
        for (int j = 0; j < slices; j += 1) {
            // compute the four vertices
            const f32 rS1 = sinf(base_slice_angle*(j + 0))*radius, rC1 = cosf(base_slice_angle*(j + 0))*radius;
            const sftvec w1 = SFT_V(start_pos.x + rS1*b1.x + rC1*b2.x, start_pos.y + rS1*b1.y + rC1*b2.y, start_pos.z + rS1*b1.z + rC1*b2.z);

            const f32 rS2 = sinf(base_slice_angle*(j + 1))*radius, rC2 = cosf(base_slice_angle*(j + 1))*radius;
            const sftvec w2 = SFT_V(start_pos.x + rS2*b1.x + rC2*b2.x, start_pos.y + rS2*b1.y + rC2*b2.y, start_pos.z + rS2*b1.z + rC2*b2.z);

            const f32 rS3 = sinf(base_slice_angle*(j + 0))*radius, rC3 = cosf(base_slice_angle*(j + 0))*radius;
            const sftvec w3 = SFT_V(end_pos.x + rS3*b1.x + rC3*b2.x, end_pos.y + rS3*b1.y + rC3*b2.y, end_pos.z + rS3*b1.z + rC3*b2.z);

            const f32 rS4 = sinf(base_slice_angle*(j + 1))*radius, rC4 = cosf(base_slice_angle*(j + 1))*radius;
            const sftvec w4 = SFT_V(end_pos.x + rS4*b1.x + rC4*b2.x, end_pos.y + rS4*b1.y + rC4*b2.y, end_pos.z + rS4*b1.z + rC4*b2.z);

            sft_dmesh_push_quad(dm, w3, w1, w2, w4, 0.f, 0.f, 1.f, 1.f, sft_v_face_normal(w3, w1, w2));
        }
    }
}

// essentially copied from raylib source: rmodels.c, DrawCylinderEx
SFT_FUNC void sft_draw_cylinder(sft_dmesh* dm, sftvec start_pos, sftvec end_pos, f32 start_radius, f32 end_radius, i32 sides) {
    if (sides < 3) {
        sides = 3;
    }

    const sftvec dir = sft_v_sub(end_pos, start_pos);
    if (0 == dir.x && 0 == dir.y && 0 == dir.z) {
        return; // security check
    }

    // construct a basis of the base and the top faces
    const sftvec b1 = sft_v_norm(sft_v_perpendicular(dir));
    const sftvec b2 = sft_v_norm(sft_v_cross(b1, dir));

    const f32 base_angle = (2.f * SFT_PI) / sides;

    for (i32 i = 0; i < sides; i += 1) {
        const f32 s1 = sinf(base_angle * (i + 0)) * start_radius, c1 = cosf(base_angle * (i + 0)) * start_radius;
        const sftvec w1 = SFT_V(start_pos.x + s1*b1.x + c1*b2.x, start_pos.y + s1*b1.y + c1*b2.y, start_pos.z + s1*b1.z + c1*b2.z);
            
        const f32 s2 = sinf(base_angle * (i + 1)) * start_radius, c2 = cosf(base_angle * (i + 1)) * start_radius;
        const sftvec w2 = SFT_V(start_pos.x + s2*b1.x + c2*b2.x, start_pos.y + s2*b1.y + c2*b2.y, start_pos.z + s2*b1.z + c2*b2.z);
            
        const f32 s3 = sinf(base_angle * (i + 0)) * end_radius, c3 = cosf(base_angle * (i + 0)) * end_radius;
        const sftvec w3 = SFT_V(end_pos.x + s3*b1.x + c3*b2.x, end_pos.y + s3*b1.y + c3*b2.y, end_pos.z + s3*b1.z + c3*b2.z);
            
        const f32 s4 = sinf(base_angle * (i + 1)) * end_radius, c4 = cosf(base_angle * (i + 1)) * end_radius;
        const sftvec w4 = SFT_V(end_pos.x + s4*b1.x + c4*b2.x, end_pos.y + s4*b1.y + c4*b2.y, end_pos.z + s4*b1.z + c4*b2.z);

        if (start_radius > 0) {
            sft_dmesh_push_quad(dm, start_pos, w2, w1, w1, 0.f, 0.f, 1.f, 1.f, sft_v_face_normal(start_pos, w2, w1));
        }
        
        sft_dmesh_push_quad(dm, w3, w1, w2, w4, 0.f, 0.f, 1.f, 1.f, sft_v_face_normal(w3, w1, w2));
        
        if (end_radius > 0) {
            sft_dmesh_push_quad(dm, end_pos, w3, w4, w4, 0.f, 0.f, 1.f, 1.f, sft_v_face_normal(end_pos, w3, w4));
        }
    }
}

SFT_FUNC sft_dmesh* sft_dmesh_begin(enum sft_dmesh_type type, sft_mat_handle material, u32 max_prims) {
    if (0 == max_prims) {
        return NULL;
    }

    const u32 max_verts_per_chunk = (SFT_DMESH_QUADS == type) ? 64 : 63;
    const u32 verts_per_prim = (SFT_DMESH_QUADS == type) ? 4 : 3;
    const u32 total_verts = max_prims * verts_per_prim;
    const u32 max_meshlets =
        (((total_verts + max_verts_per_chunk - 1) / max_verts_per_chunk) + SFT_SIMD_WIDTH - 1) & ~(SFT_SIMD_WIDTH - 1);

    sft_dmesh* dm = (sft_dmesh*)sft_arena_alloc(&sft_state->scratch_arena, sizeof(sft_dmesh));
    dm->type = type;
    dm->material = material;
    dm->max_meshlets = max_meshlets;
    dm->curr_meshlet = 0;
    dm->curr_vert = 0;
    dm->max_verts_per_chunk = max_verts_per_chunk;

    dm->meshlets = (struct sft_meshlet*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(struct sft_meshlet));
    dm->attribs = (struct sft_meshlet_attribs*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(struct sft_meshlet_attribs));
    dm->inds = (struct sft_meshlet_inds*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(struct sft_meshlet_inds));

    dm->bounds_center_x = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_center_y = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_center_z = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_radius   = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));

    dm->bounds_min_x = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_min_y = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_min_z = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_max_x = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_max_y = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->bounds_max_z = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));

    dm->cone_axis_x = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->cone_axis_y = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->cone_axis_z = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));
    dm->cone_cutoff = (f32*)sft_arena_alloc(&sft_state->scratch_arena, max_meshlets * sizeof(f32));

    return dm;
}

SFT_FUNC void sft_dmesh_push_quad_4n(
    sft_dmesh* dm,
    sftvec p0, sftvec p1, sftvec p2, sftvec p3,
    f32 u0, f32 v0, f32 u1, f32 v1,
    sftvec n0, sftvec n1, sftvec n2, sftvec n3
) {
    if (dm->curr_vert >= dm->max_verts_per_chunk) {
        sft__dmesh_finalize_chunk(dm);
    }
    if (dm->curr_meshlet >= dm->max_meshlets) {
        return; // prevent overflow
    }

    const u32 v = dm->curr_vert;
    struct sft_meshlet* m = &dm->meshlets[dm->curr_meshlet];
    struct sft_meshlet_attribs* attr = &dm->attribs[dm->curr_meshlet];

    // assign positions
    m->x[v + 0] = p0.x, m->y[v + 0] = p0.y, m->z[v + 0] = p0.z;
    m->x[v + 1] = p1.x, m->y[v + 1] = p1.y, m->z[v + 1] = p1.z;
    m->x[v + 2] = p2.x, m->y[v + 2] = p2.y, m->z[v + 2] = p2.z;
    m->x[v + 3] = p3.x, m->y[v + 3] = p3.y, m->z[v + 3] = p3.z;

    // assign UVs
    attr->u[v + 0] = u0, attr->v[v + 0] = v0;
    attr->u[v + 1] = u1, attr->v[v + 1] = v0;
    attr->u[v + 2] = u1, attr->v[v + 2] = v1;
    attr->u[v + 3] = u0, attr->v[v + 3] = v1;

    // assign unique vertex normals
    attr->nx[v + 0] = n0.x, attr->ny[v + 0] = n0.y, attr->nz[v + 0] = n0.z;
    attr->nx[v + 1] = n1.x, attr->ny[v + 1] = n1.y, attr->nz[v + 1] = n1.z;
    attr->nx[v + 2] = n2.x, attr->ny[v + 2] = n2.y, attr->nz[v + 2] = n2.z;
    attr->nx[v + 3] = n3.x, attr->ny[v + 3] = n3.y, attr->nz[v + 3] = n3.z;

    // zero out lightmap UVs
    attr->lu[v + 0] = 0.f, attr->lv[v + 0] = 0.f;
    attr->lu[v + 1] = 0.f, attr->lv[v + 1] = 0.f;
    attr->lu[v + 2] = 0.f, attr->lv[v + 2] = 0.f;
    attr->lu[v + 3] = 0.f, attr->lv[v + 3] = 0.f;

    dm->curr_vert += 4;
}

SFT_FUNC void sft_dmesh_push_quad(
    sft_dmesh* dm,
    sftvec p0, sftvec p1, sftvec p2, sftvec p3,
    f32 u0, f32 v0, f32 u1, f32 v1,
    sftvec normal
) {
    sft_dmesh_push_quad_4n(
        dm, 
        p0, p1, p2, p3, 
        u0, v0, u1, v1, 
        normal, normal, normal, normal
    );
}

SFT_FUNC void sft_dmesh_push_tri(
    sft_dmesh* dm,
    sftvec p0, sftvec p1, sftvec p2,
    f32 u0, f32 v0, f32 u1, f32 v1, f32 u2, f32 v2,
    sftvec normal
) {
    if (dm->curr_vert >= dm->max_verts_per_chunk) {
        sft__dmesh_finalize_chunk(dm);
    }
    if (dm->curr_meshlet >= dm->max_meshlets) {
        return; 
    }

    const u32 v = dm->curr_vert;
    struct sft_meshlet* m = &dm->meshlets[dm->curr_meshlet];
    struct sft_meshlet_attribs* attr = &dm->attribs[dm->curr_meshlet];

    m->x[v + 0] = p0.x, m->y[v + 0] = p0.y, m->z[v + 0] = p0.z;
    m->x[v + 1] = p1.x, m->y[v + 1] = p1.y, m->z[v + 1] = p1.z;
    m->x[v + 2] = p2.x, m->y[v + 2] = p2.y, m->z[v + 2] = p2.z;

    attr->u[v + 0] = u0, attr->v[v + 0] = v0;
    attr->u[v + 1] = u1, attr->v[v + 1] = v1;
    attr->u[v + 2] = u2, attr->v[v + 2] = v2;

    for (u32 i = 0; i < 3; i += 1) {
        attr->nx[v + i] = normal.x, attr->ny[v + i] = normal.y, attr->nz[v + i] = normal.z;
        attr->lu[v + i] = 0.f, attr->lv[v + i] = 0.f;
    }

    dm->curr_vert += 3;
}

SFT_FUNC u8 sft_dmesh_alloc_quads(sft_dmesh* dm, u32 count, sft_dmesh_qb* out_batch) {
    // max quads that fit in the active chunk
    u32 space_remaining = (dm->max_verts_per_chunk - dm->curr_vert) / 4;
    
    if (0 == space_remaining) {
        sft__dmesh_finalize_chunk(dm);
        space_remaining = dm->max_verts_per_chunk / 4;
    }
    if (dm->curr_meshlet >= dm->max_meshlets) {
        return 0;
    }

    const u32 allocation = (count < space_remaining) ? count : space_remaining;

    struct sft_meshlet* m = &dm->meshlets[dm->curr_meshlet];
    struct sft_meshlet_attribs* attr = &dm->attribs[dm->curr_meshlet];

    out_batch->x = m->x, out_batch->y = m->y, out_batch->z = m->z;
    out_batch->nx = attr->nx, out_batch->ny = attr->ny, out_batch->nz = attr->nz;
    out_batch->u = attr->u, out_batch->v = attr->v;
    out_batch->vert_offset = dm->curr_vert;
    out_batch->allocated_count = allocation;

    dm->curr_vert += allocation * 4;
    return 1;
}

SFT_FUNC void sft_dmesh_end(sft_dmesh* dm, const sftmat* opt_transform) {
    if (!dm) {
        return;
    }

    // flush any pending geometry
    if (dm->curr_vert > 0) {
        sft__dmesh_finalize_chunk(dm);
    }

    if (0 == dm->curr_meshlet) {
        return; // nothing drawn
    }

    sft_mesh* mesh = (sft_mesh*)sft_arena_alloc(&sft_state->scratch_arena, sizeof(sft_mesh));
    memset(mesh, 0, sizeof(sft_mesh));
    mesh->meshlets = dm->meshlets;
    mesh->meshlet_attribs = dm->attribs;
    mesh->inds = dm->inds;
    mesh->meshlet_count = dm->curr_meshlet;
    mesh->model_meshlet_offset = 0;
    mesh->material = dm->material;
    mesh->lightmap_page = 0xFFFFFFFF;
    
    mesh->bounds_center_x = dm->bounds_center_x;
    mesh->bounds_center_y = dm->bounds_center_y;
    mesh->bounds_center_z = dm->bounds_center_z;
    mesh->bounds_radius = dm->bounds_radius;
    mesh->bounds_min_x = dm->bounds_min_x;
    mesh->bounds_min_y = dm->bounds_min_y;
    mesh->bounds_min_z = dm->bounds_min_z;
    mesh->bounds_max_x = dm->bounds_max_x;
    mesh->bounds_max_y = dm->bounds_max_y;
    mesh->bounds_max_z = dm->bounds_max_z;
    
    mesh->cone_axis_x = dm->cone_axis_x;
    mesh->cone_axis_y = dm->cone_axis_y;
    mesh->cone_axis_z = dm->cone_axis_z;
    mesh->cone_cutoff = dm->cone_cutoff;

    sft_model* model = (sft_model*)sft_arena_alloc(&sft_state->scratch_arena, sizeof(sft_model));
    memset(model, 0, sizeof(sft_model));
    model->meshes = mesh;
    model->mesh_count = 1;
    model->total_meshlets = dm->curr_meshlet;
    model->blas_node_count = 0;
    
    model->meshlet_mesh_inds = (u16*)sft_arena_alloc(&sft_state->scratch_arena, dm->curr_meshlet * sizeof(u16));
    memset(model->meshlet_mesh_inds, 0, dm->curr_meshlet * sizeof(u16));

    sft_instance inst = {
        .model = model,
        .transform = opt_transform ? *opt_transform : SFT_MAT_IDENTITY,
        .ss_meshlet_offset = 0
    };

    sft_draw_instance(&inst);
}

sft_pose* sft_pose_create(const sft_model* model, sft_arena* arena) {
    if (!model || 0 == model->skeleton.bone_count) {
        return NULL;
    }

    sft_pose* pose = (sft_pose*)sft_arena_alloc(arena, sizeof(sft_pose));
    pose->model = model;
    pose->global_matrices = (sftmat*)sft_arena_alloc(arena, model->skeleton.bone_count * sizeof(sftmat));
    pose->posed_bone_matrices = (sftmat*)sft_arena_alloc(arena, model->skeleton.bone_count * sizeof(sftmat));
    return pose;
}

void sft_pose_update(sft_pose* pose, i32 anim_ind, f32 time) {
    if (!pose || !pose->model->animations || anim_ind < 0 || anim_ind >= pose->model->anim_count) {
        return;
    }

    const struct sft_model* model = pose->model;
    const struct sft_animation* anim = &model->animations[anim_ind];

    if (anim->duration > SFT_EPSILON) {
        time = fmodf(time, anim->duration);
    }

    // scratch buffer for the local TRS state of this specific frame
    // TODO remove this per frame (per call to update actually) allocation
    struct sft_trs* trs = (struct sft_trs*)calloc(model->skeleton.bone_count, sizeof(struct sft_trs));

    // evaluate the animation tracks
    for (i32 i = 0; i < anim->channel_count; i += 1) {
        const struct sft_anim_channel* ch = &anim->channels[i];
        const struct sft_anim_sampler* smp = &anim->samplers[ch->sampler_ind];
        if (ch->node_ind < 0 || ch->node_ind >= model->skeleton.bone_count) {
            continue;
        }

        i32 f0 = 0, f1 = 0; f32 t = 0.f;
        if (time <= smp->inputs[0]) {
            f0 = f1 = 0.f;
        }
        else if (time >= smp->inputs[smp->count - 1]) {
            f0 = f1 = smp->count - 1;
        }
        else {
            i32 low = 0, high = smp->count - 1;
            while (low <= high) {
                const i32 mid = low + (high - low) / 2;
                if (time < smp->inputs[mid]) {
                    high = mid - 1;
                }
                else if (time >= smp->inputs[mid + 1]) {
                    low = mid + 1;
                }
                else {
                    f0 = mid;
                    f1 = mid + 1;
                    break;
                }
            }
            const f32 dt = smp->inputs[f1] - smp->inputs[f0];
            t = (dt > 0.0001f) ? (time - smp->inputs[f0]) / dt : 0.f;
        }

        if (SFT_ANIM_PATH_ROTATION == ch->path) {
            sftquat val;
            if (smp->is_step) {
                val = (sftquat){
                    smp->outputs[f0 * 4 + 0],
                    smp->outputs[f0 * 4 + 1],
                    smp->outputs[f0 * 4 + 2],
                    smp->outputs[f0 * 4 + 3]
                };
            }
            else {
                const sftquat q0 = {
                    smp->outputs[f0 * 4 + 0],
                    smp->outputs[f0 * 4 + 1],
                    smp->outputs[f0 * 4 + 2],
                    smp->outputs[f0 * 4 + 3]
                };
                const sftquat q1 = {
                    smp->outputs[f1 * 4 + 0],
                    smp->outputs[f1 * 4 + 1],
                    smp->outputs[f1 * 4 + 2],
                    smp->outputs[f1 * 4 + 3]
                };
                val = sft_q_slerp(q0, q1, t); 
            }
            trs[ch->node_ind].r = val;
            trs[ch->node_ind].mask |= (1 << SFT_ANIM_PATH_ROTATION);
        }
        else {
            sftvec val;
            if (smp->is_step) {
                val = SFT_V(smp->outputs[f0 * 4 + 0], smp->outputs[f0 * 4 + 1], smp->outputs[f0 * 4 + 2]);
            }
            else {
                const sftvec v0 = SFT_V(smp->outputs[f0 * 4 + 0], smp->outputs[f0 * 4 + 1], smp->outputs[f0 * 4 + 2]);
                const sftvec v1 = SFT_V(smp->outputs[f1 * 4 + 0], smp->outputs[f1 * 4 + 1], smp->outputs[f1 * 4 + 2]);
                val = sft_v_add(v0, sft_v_mul(sft_v_sub(v1, v0), t));
            }
            
            if (SFT_ANIM_PATH_TRANSLATION == ch->path) {
                trs[ch->node_ind].p = val;
                trs[ch->node_ind].mask |= (1 << SFT_ANIM_PATH_TRANSLATION);
            }
            else if (SFT_ANIM_PATH_SCALE == ch->path) {
                trs[ch->node_ind].s = val;
                trs[ch->node_ind].mask |= (1 << SFT_ANIM_PATH_SCALE);
            }
        }
    }

    // reconstruct local mats and flatten the hierarchy
    // TODO remove this as well
    u8* computed = (u8*)calloc(model->skeleton.bone_count, 1);
    
    for (i32 i = 0; i < (i32)model->skeleton.bone_count; i += 1) {
        if (trs[i].mask) {
            const struct sft_bone* b = &model->skeleton.bones[i];

            const sftmat T = (trs[i].mask & (1 << SFT_ANIM_PATH_TRANSLATION)) ?
                sft_m_translate(trs[i].p.x, trs[i].p.y, trs[i].p.z) :
                sft_m_translate(b->base_t.x, b->base_t.y, b->base_t.z);
            
            const sftmat R = (trs[i].mask & (1 << SFT_ANIM_PATH_ROTATION)) ?
                sft_m_from_quat(trs[i].r) :
                sft_m_from_quat(b->base_r);
            
            const sftmat S = (trs[i].mask & (1 << SFT_ANIM_PATH_SCALE)) ?
                sft_m_scale(trs[i].s.x, trs[i].s.y, trs[i].s.z) :
                sft_m_scale(b->base_s.x, b->base_s.y, b->base_s.z);
            
            pose->global_matrices[i] = sft_m_mul(S, sft_m_mul(R, T));
        }
        else {
            pose->global_matrices[i] = model->skeleton.bones[i].local_bind_matrix;
        }
    }

    // flatten hierarchy
    i32 computed_count = 0;
    while (computed_count < model->skeleton.bone_count) {
        for (i32 i = 0; i < model->skeleton.bone_count; i += 1) {
            if (computed[i]) {
                continue;
            }
            
            const i32 parent = model->skeleton.bones[i].parent_ind;
            if (parent < 0) {
                computed[i] = 1;
                computed_count += 1;
            }
            else if (computed[parent]) {
                pose->global_matrices[i] = sft_m_mul(pose->global_matrices[i], pose->global_matrices[parent]);
                computed[i] = 1;
                computed_count += 1;
            }
        }
    }
    free(computed);

    for (i32 i = 0; i < model->skeleton.bone_count; i += 1) {
        pose->posed_bone_matrices[i] = sft_m_mul(model->skeleton.bones[i].inverse_bind_matrix, pose->global_matrices[i]);
    }

    free(trs);
}

SFT_FUNC void sft_set_ambient_light(f32 r, f32 g, f32 b) {
    sft_state->ambient_light.x = r;
    sft_state->ambient_light.y = g;
    sft_state->ambient_light.z = b;
}

SFT_FUNC void sft_draw_directional_light(sftvec dir, f32 r, f32 g, f32 b, f32 intensity) {
    const i32 slot = sft_atomic_add(&sft_state->lighting.directional_count.count, 1);
    
    sft_arena_commit_if_needed(&sft_state->light_arena, (slot + 1) * sizeof(sft_directional_light));

    sft_state->lighting.directional[slot].dir = sft_v_norm(dir);
    sft_state->lighting.directional[slot].color.x = r * intensity;
    sft_state->lighting.directional[slot].color.y = g * intensity;
    sft_state->lighting.directional[slot].color.z = b * intensity;
}

SFT_FUNC void sft_draw_point_light(sftvec pos, f32 r, f32 g, f32 b, f32 intensity, f32 radius, u32 light_id) {
    (void)light_id; // unused for now

    // store the data in the array for the lighting pass
    const i32 slot = sft_atomic_add(&sft_state->lighting.point_count.count, 1);
    
    const u64 required_bytes = (8ULL * 1024 * 1024) + ((slot + 1) * sizeof(sft_point_light));
    sft_arena_commit_if_needed(&sft_state->light_arena, required_bytes);

    sft_state->lighting.point[slot].pos = pos;
    sft_state->lighting.point[slot].color.x = r * intensity;
    sft_state->lighting.point[slot].color.y = g * intensity;
    sft_state->lighting.point[slot].color.z = b * intensity;
    sft_state->lighting.point[slot].radius = radius;
    sft_state->lighting.point[slot].inv_radius_sqr = 1.f / (radius * radius);
}

SFT_FUNC void sft_set_shadows_enabled(u8 enabled) {
    sft_state->shadows.enabled = enabled;
}

SFT_FUNC void sft_set_lightmap_brightness(f32 b) {
    sft_state->lighting.lightmap_brightness = b;
}

#ifdef SFT_USE_STB_IMAGE

SFT_FUNC sft_tex_handle sft_texture_load(const char* path) {
    i32 w, h, channels;
    stbi_uc* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) {
        SFT_ERR_RET(SFT_INVALID_HANDLE, "failed to load texture '%s'\n", path);
    }

    sft__swizzle_rgba_to_argb((u32*)pixels, w * h);
    const sft_tex_handle handle = sft_texture_create(w, h, (const u32*)pixels);
    stbi_image_free(pixels);
    return handle;
}

SFT_FUNC sft_tex_handle sft_texture_load_from_memory(const u8* buffer, i32 length) {
    i32 w, h, channels;
    stbi_uc* pixels = stbi_load_from_memory(buffer, length, &w, &h, &channels, 4);
    if (!pixels) {
        SFT_ERR_RET(SFT_INVALID_HANDLE, "failed to load texture from memory\n");
    }
    
    sft__swizzle_rgba_to_argb((u32*)pixels, w * h);
    const sft_tex_handle handle = sft_texture_create(w, h, (const u32*)pixels);
    stbi_image_free(pixels);
    return handle;
}

#endif // SFT_USE_STB_IMAGE

SFT_FUNC sft_tex_handle sft_texture_create(i32 width, i32 height, const u32* pixels) {
    if (width <= 0 || height <= 0 || !pixels) {
        return SFT_INVALID_HANDLE;
    }

    const u8 is_pot = (0 == (width & (width - 1))) && (0 == (height & (height - 1)));
    
    // ---  total memory needed for all mip levels padded to 4x4 blocks  ---
    i32 cw = width, ch = height;
    u64 total_padded_pixels = 0;
    i32 mip_levels = 0;
    
    while (mip_levels < 14) {
        const i32 pw = (cw + 3) & ~3;
        const i32 ph = (ch + 3) & ~3;
        total_padded_pixels += (u64)(pw * ph);
        
        mip_levels += 1;
        if (1 == cw && 1 == ch) {
            break;
        }
        
        cw = (cw > 1) ? cw / 2 : 1;
        ch = (ch > 1) ? ch / 2 : 1;
    }
    
    // ---  allocate contiguous block from virtual texture heap  ---
    const u64 total_bytes = total_padded_pixels * sizeof(u32);
    const u64 heap_offset = sft__tex_heap_alloc(total_bytes);
    
    // pointer to directly write into the mapped virtual arena
    u32* curr_swizzle_dest = (u32*)(sft_state->tex_heap_arena.base + heap_offset);

    // ---  claim a slot in the texture pool  ---
    if (-1 == sft_state->tex_pool.free_head) {
        sft__tex_pool_grow();
    }
    const i32 t_ind = sft_state->tex_pool.free_head;
    sft_state->tex_pool.free_head = sft_state->tex_pool.next_free[t_ind];
    
    sft_state->tex_pool.heap_offset[t_ind] = heap_offset;
    sft_state->tex_pool.w[t_ind] = width;
    sft_state->tex_pool.h[t_ind] = height;
    sft_state->tex_pool.mip_levels[t_ind] = mip_levels;
    sft_state->tex_pool.is_pot[t_ind] = is_pot;

    const i32 mip_array_base = t_ind * 14;

    // to swizzle linear data into the 4x4 block layout
    #define DO_SWIZZLE(lvl, src_w, src_h, linear_src) { \
        const i32 pw = (src_w + 3) & ~3; \
        const i32 ph = (src_h + 3) & ~3; \
        const i32 bw = pw / 4; \
        sft_state->tex_pool.mip_offsets[mip_array_base + lvl] = (u32)((u8*)curr_swizzle_dest - sft_state->tex_heap_arena.base); \
        sft_state->tex_pool.mip_blocks_w[mip_array_base + lvl] = bw; \
        sft_state->tex_pool.mip_stride_shift[mip_array_base + lvl] = is_pot ? sft__fast_log2(bw) : 0; \
        for (i32 y = 0; y < src_h; y += 1) { \
            for (i32 x = 0; x < src_w; x += 1) { \
                const i32 bx = x / 4, by = y / 4, in_x = x % 4, in_y = y % 4; \
                const i32 d_ind = (by * bw + bx) * 16 + (in_y * 4) + in_x; \
                curr_swizzle_dest[d_ind] = linear_src[y * src_w + x]; \
            } \
        } \
        curr_swizzle_dest += (pw * ph); \
    }

    // swizzle level 0
    DO_SWIZZLE(0, width, height, pixels);

    // ---  generate subsequent mips using linear temp buffers  ---
    cw = width; ch = height;
    const u32* src_linear = pixels;
    u32* temp_linear = NULL;

    for (i32 curr_level = 1; curr_level < mip_levels; curr_level += 1) {
        const i32 nw = (cw > 1) ? cw / 2 : 1;
        const i32 nh = (ch > 1) ? ch / 2 : 1;

        u32* dest_linear = malloc(sizeof(u32) * nw * nh);

        // tent filter
        for (i32 y = 0; y < nh; y += 1) {
            for (i32 x = 0; x < nw; x += 1) {
                const i32 cx = x * 2;
                const i32 cy = y * 2;

                u32 r = 0, g = 0, b = 0, a = 0;
                u32 wsum = 0;

                for (i32 ky = -1; ky <= 2; ky += 1) {
                    const i32 sy = SFT_CLAMP(cy + ky, 0, ch - 1);
                    const u32 wy = (-1 == ky || 2 == ky) ? 1 : 2;

                    for (i32 kx = -1; kx <= 2; kx += 1) {
                        const i32 sx = SFT_CLAMP(cx + kx, 0, cw - 1);
                        const u32 wx = (-1 == kx || 2 == kx) ? 1 : 2;

                        const u32 w = wx * wy;
                        const u32 c = src_linear[sy * cw + sx];

                        r += ((c >> 16) & 255) * w;
                        g += ((c >> 8)  & 255) * w;
                        b += ((c >> 0)  & 255) * w;
                        a += ((c >> 24) & 255) * w;
                        wsum += w;
                    }
                }
                dest_linear[y * nw + x] = ((a / wsum) << 24) | ((r / wsum) << 16) | ((g / wsum) << 8) | (b / wsum);
            }
        }

        DO_SWIZZLE(curr_level, nw, nh, dest_linear);

        if (temp_linear) {
            free(temp_linear);
        }
        temp_linear = dest_linear;
        src_linear = temp_linear;
        cw = nw;
        ch = nh;
    }
    
    if (temp_linear) {
        free(temp_linear);
    }
    #undef DO_SWIZZLE

    const u32 gen = sft_state->tex_pool.generation[t_ind];
    return (gen << SFT_HANDLE_GEN_SHIFT) | t_ind;
}

SFT_FUNC sft_tex_handle sft_texture_create_solid(u32 color) {
    return sft_texture_create(1, 1, &color);
}

SFT_FUNC i32 sft_texture_get_width(sft_tex_handle tex) {
    if (SFT_INVALID_HANDLE == tex) {
        return 0;
    }
    return sft_state->tex_pool.w[SFT_HANDLE_INDEX(tex)];
}

SFT_FUNC i32 sft_texture_get_height(sft_tex_handle tex) {
    if (SFT_INVALID_HANDLE == tex) {
        return 0;
    }
    return sft_state->tex_pool.h[SFT_HANDLE_INDEX(tex)];
}

SFT_FUNC u32* sft_texture_get_linear_pixels(sft_tex_handle tex) {
    if (SFT_INVALID_HANDLE == tex) {
        return NULL;
    }
    
    const u32 ind = SFT_HANDLE_INDEX(tex);
    const i32 w = sft_state->tex_pool.w[ind];
    const i32 h = sft_state->tex_pool.h[ind];
    
    // linear buffer to return
    u32* linear = (u32*)malloc(w * h * sizeof(u32));
    
    // the swizzled level 0 pixels in the virtual heap
    u32 mip0_offset = sft_state->tex_pool.mip_offsets[ind * 14];
    const u32* swizzled = (const u32*)(sft_state->tex_heap_arena.base + mip0_offset);
    
    const i32 bw = sft_state->tex_pool.mip_blocks_w[ind * 14];
    
    // un swizzle back into standard linear layout
    for (i32 y = 0; y < h; y += 1) {
        for (i32 x = 0; x < w; x += 1) {
            const i32 bx = x / 4, by = y / 4, in_x = x % 4, in_y = y % 4;
            const i32 s_ind = (by * bw + bx) * 16 + (in_y * 4) + in_x;
            linear[y * w + x] = swizzled[s_ind];
        }
    }
    
    return linear;
}

SFT_FUNC void sft_texture_release(sft_tex_handle tex) {
    if (tex == SFT_INVALID_HANDLE) {
        return;
    }

    const u32 ind = SFT_HANDLE_INDEX(tex);
    const u32 gen = tex >> SFT_HANDLE_GEN_SHIFT;

    if (ind >= (u32)sft_state->tex_pool.capacity || sft_state->tex_pool.generation[ind] != gen) {
        return;
    }

    // the total size to release back to the heap
    const u64 heap_offset = sft_state->tex_pool.heap_offset[ind];
    i32 cw = sft_state->tex_pool.w[ind];
    i32 ch = sft_state->tex_pool.h[ind];
    u64 total_padded_pixels = 0;
    
    for (i32 lvl = 0; lvl < sft_state->tex_pool.mip_levels[ind]; lvl += 1) {
        total_padded_pixels += (u64)(((cw + 3) & ~3) * ((ch + 3) & ~3));
        cw = (cw > 1) ? cw / 2 : 1;
        ch = (ch > 1) ? ch / 2 : 1;
    }

    // free the block in the virtual heap
    sft__tex_heap_free(heap_offset, total_padded_pixels * sizeof(u32));

    // invalidate handle and reclaim slot
    sft_state->tex_pool.generation[ind] += 1;
    sft_state->tex_pool.next_free[ind] = sft_state->tex_pool.free_head;
    sft_state->tex_pool.free_head = ind;
}

SFT_FUNC sft_mat_handle sft_material_create(sft_tex_handle albedo_tex, u32 base_color, f32 shininess) {
    if (-1 == sft_state->mat_pool.free_head) {
        sft__mat_pool_grow_static();
    }

    const i32 ind = sft_state->mat_pool.free_head;
    sft_state->mat_pool.free_head = sft_state->mat_pool.next_free[ind];

    sft_state->mat_pool.albedo_tex[ind] = albedo_tex;
    sft_state->mat_pool.base_color[ind] = base_color;
    sft_state->mat_pool.shininess[ind] = shininess;

    const u32 gen = sft_state->mat_pool.generation[ind];
    return (gen << SFT_MAT_GEN_SHIFT) | ind;
}

SFT_FUNC void sft_material_release(sft_mat_handle mat) {
    if (SFT_INVALID_HANDLE == mat || SFT_MAT_IS_TRANSIENT(mat)) {
        return;
    }

    const u32 ind = SFT_MAT_INDEX(mat);
    const u32 gen = (mat & SFT_MAT_GEN_MASK) >> SFT_MAT_GEN_SHIFT;

    if (ind >= (u32)sft_state->mat_pool.capacity || sft_state->mat_pool.generation[ind] != gen) {
        return;
    }

    sft_state->mat_pool.generation[ind] += 1; 
    sft_state->mat_pool.next_free[ind] = sft_state->mat_pool.free_head;
    sft_state->mat_pool.free_head = ind;
}

SFT_FUNC sft_mat_handle sft_material_clone(sft_mat_handle base_mat) {
    if (sft_state->mat_pool_transient.count >= sft_state->mat_pool_transient.capacity) {
        sft__mat_pool_grow_transient();
    }

    const i32 new_ind = sft_state->mat_pool_transient.count;
    sft_state->mat_pool_transient.count += 1;

    if (SFT_INVALID_HANDLE != base_mat) {
        const u32 base_ind = SFT_MAT_INDEX(base_mat);
        if (SFT_MAT_IS_TRANSIENT(base_mat)) {
            sft_state->mat_pool_transient.albedo_tex[new_ind] = sft_state->mat_pool_transient.albedo_tex[base_ind];
            sft_state->mat_pool_transient.base_color[new_ind] = sft_state->mat_pool_transient.base_color[base_ind];
            sft_state->mat_pool_transient.shininess[new_ind]  = sft_state->mat_pool_transient.shininess[base_ind];
        }
        else {
            sft_state->mat_pool_transient.albedo_tex[new_ind] = sft_state->mat_pool.albedo_tex[base_ind];
            sft_state->mat_pool_transient.base_color[new_ind] = sft_state->mat_pool.base_color[base_ind];
            sft_state->mat_pool_transient.shininess[new_ind]  = sft_state->mat_pool.shininess[base_ind];
        }
    }
    else {
        sft_state->mat_pool_transient.albedo_tex[new_ind] = SFT_INVALID_HANDLE;
        sft_state->mat_pool_transient.base_color[new_ind] = 0xFFFFFFFF;
        sft_state->mat_pool_transient.shininess[new_ind]  = 0.f;
    }

    return SFT_MAT_TRANSIENT_BIT | new_ind;
}

SFT_FUNC void sft_arena_init(sft_arena* arena, u64 reserve_size) {
    arena->max_size = reserve_size;
    arena->committed_size = 0;
    arena->offset = 0;
    arena->commit_lock = 0;

    #if defined(_WIN32)
        // reserve address space
        arena->base = (u8*)VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_NOACCESS);
        if (!arena->base) {
            SFT_ERR_EXIT("VirtualAlloc reserve failed for size %llu\n", reserve_size);
        }

    #else
        // reserves and handles commit automatically via page faults
        arena->base = (u8*)mmap(NULL, reserve_size, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (MAP_FAILED == arena->base) {
            SFT_ERR_EXIT("mmap reserve failed for size %zu\n", reserve_size);
        }

        // pretend it's all committed
        arena->committed_size = reserve_size; 
    #endif

    SFT_PRINT("arena initialized, reserved %zu KB at %p\n", (size_t)(reserve_size / 1024ULL), arena->base);
}

SFT_FUNC void* sft_arena_alloc(sft_arena* arena, u64 size) {
    if (!arena) {
        SFT_ERR_RET(NULL, "arena pointer is NULL\n");
    }

    // enforce 64 byte alignment.
    const u64 aligned_offset = (arena->offset + 63ULL) & ~63ULL;
    const u64 next_offset = aligned_offset + size;

    if (next_offset > arena->max_size) {
        SFT_ERR_EXIT("arena out of memory, requested %zu KB, max %zu, offset %zu\n",
            size / 1024, arena->max_size / 1024, aligned_offset);
    }

    // windows requires explicit committing if committed boundary is crossed
    #if defined(_WIN32)
        if (next_offset > arena->committed_size) {
            // how many chunks to commit to satisfy the request
            const u64 commit_needed = next_offset - arena->committed_size;
            u64 chunks_to_commit = ((commit_needed + SFT_ARENA_COMMIT_CHUNK - 1) / SFT_ARENA_COMMIT_CHUNK) * SFT_ARENA_COMMIT_CHUNK;
            
            // clamp the commit size so it never exceeds the total reserved address space
            if (arena->committed_size + chunks_to_commit > arena->max_size) {
                chunks_to_commit = arena->max_size - arena->committed_size;
            }
            
            void* commit_target = arena->base + arena->committed_size;
            const void* res = VirtualAlloc(commit_target, chunks_to_commit, MEM_COMMIT, PAGE_READWRITE);
            if (!res) {
                SFT_ERR_EXIT("VirtualAlloc commit failed for %zu KB at %p\n", chunks_to_commit / 1024, commit_target);
            }

            arena->committed_size += chunks_to_commit;
            SFT_PRINT("arena auto committed %zu KB\n", chunks_to_commit / 1024);
        }
    #endif

    // advance the arena state
    void* ptr = arena->base + aligned_offset;
    arena->offset = next_offset;

    return ptr;
}

SFT_INLINE void sft_arena_commit_if_needed(sft_arena* arena, u64 required_bytes) {
#if defined(_WIN32)
    if (required_bytes > arena->committed_size) {
        while (sft_atomic_cas(&arena->commit_lock, 0, 1) != 0) {
            sft_cpu_pause();
        }

        // double check after acquiring lock
        if (required_bytes > arena->committed_size) {
            const u64 commit_needed = required_bytes - arena->committed_size;
            u64 chunks_to_commit = ((commit_needed + SFT_ARENA_COMMIT_CHUNK - 1) / SFT_ARENA_COMMIT_CHUNK) * SFT_ARENA_COMMIT_CHUNK;
            
            if (arena->committed_size + chunks_to_commit > arena->max_size) {
                chunks_to_commit = arena->max_size - arena->committed_size;
            }
            
            void* commit_target = arena->base + arena->committed_size;
            void* res = VirtualAlloc(commit_target, chunks_to_commit, MEM_COMMIT, PAGE_READWRITE);
            if (!res) {
                SFT_ERR_EXIT("VirtualAlloc commit failed for %zu KB\n", chunks_to_commit);
            }

            arena->committed_size += chunks_to_commit;
        }

        sft_atomic_exchange(&arena->commit_lock, 0);
    }
#endif

    // function not needed if not on Windows so suppress warnings
    (void)arena;
    (void)required_bytes;
}

SFT_FUNC void sft_arena_frame_begin(sft_arena* arena) {
    arena->offset = 0;
}


#endif // SFT_IMPL

#ifdef SFT_PREFIXED_TYPES
    #undef i8
    #undef u8
    #undef i16
    #undef u16
    #undef i32
    #undef u32
    #undef i64
    #undef u64
    #undef f32
    #undef f64
    #undef vf32
    #undef vi32
#endif

#ifdef __cplusplus
}
#endif

#endif // SFT_H
