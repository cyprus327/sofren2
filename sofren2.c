// IF YOU CAN SEE THESE THREE LINES THEN I JUST FORGOT TO REMOVE THEM
#define SFT_IMPL
#define SFT_USE_CGLTF
#define SFT_USE_STB_IMAGE

#ifndef SFT_H
#define SFT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>
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

// public struct prototypes
typedef struct sftvec   sftvec;
typedef struct sftvvec  sftvvec;
typedef struct sftquat  sftquat;
typedef struct sftvquat sftvquat;
typedef union  sftmat   sftmat;
typedef union  sftvmat  sftvmat;

typedef struct sft_arena sft_arena;

typedef struct sft_mesh     sft_mesh;
typedef struct sft_model    sft_model;
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
#define SFT_SWAP(type, _a, _b) { const type _swapTemp = (_a); (_a) = (_b); (_b) = _swapTemp; }

// prime numbers for spatial hashing
#define SFT_HASH_PRIME_X 73856093
#define SFT_HASH_PRIME_Y 19349663
#define SFT_HASH_PRIME_Z 83492791

// cosine of 100 degrees, if the dot product of two normals is less than this,
// the angle between them is strictly greater than 100 degrees
#define SFT_CONE_LIMIT (-0.173648f)

// must total 31 to reserve bit 31 for clipping flag
// 18 meshlet bits required for huge scenes like Bistro, TODO maybe balance meshlets better somehow (preprocess models)
#define SFT_INSTANCE_BITS 6
#define SFT_MESHLET_BITS 18
#define SFT_TRIANGLE_BITS 7
#if 31 != (SFT_INSTANCE_BITS + SFT_MESHLET_BITS + SFT_TRIANGLE_BITS)
    #error "SFT ERROR: SFT_INSTANCE_BITS + SFT_MESHLET_BITS + SFT_TRIANGLE_BITS must equal 31"
#endif
#define SFT_MAX_INSTANCES_PER_FRAME (1 << SFT_INSTANCE_BITS)

#define SFT_VERTS_PER_MESHLET 64

#define SFT_MESHLETS_PER_JOB 2

#define SFT_CHUNK_CAPACITY 63

#define SFT_TILE_SHIFT 6
#define SFT_TILE_SIZE (1 << SFT_TILE_SHIFT)

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
    volatile i32 count, generation;
    i32 thread_count;
};

#if defined(_WIN32)
    // windows.h included in [MEMORY WRAPPERS]
    typedef HANDLE sft_thread;
    typedef HANDLE sft_sem;
#else
    #include <pthread.h>
    #include <semaphore.h>
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
SFT_INLINE sftvec sft_m_mul_quat(sftmat m, sftquat v);
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

// misc
SFT_FUNC void sft_set_camera(sftvec cam_pos, sftmat view, sftmat proj);

// drawing functions
SFT_FUNC void sft_draw_instance(const sft_instance* instance);

// light functions
SFT_FUNC void sft_set_ambient_light(f32 r, f32 g, f32 b);
SFT_FUNC void sft_draw_directional_light(sftvec dir, f32 r, f32 g, f32 b, f32 intensity);
SFT_FUNC void sft_draw_point_light(sftvec pos, f32 r, f32 g, f32 b, f32 intensity, f32 radius);

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
SFT_FUNC sft_model* sft_model_load_glb(const char* path, sft_arena* arena, i32 uv_channel);
#endif
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

typedef struct sft_arena {
    u8* base;                   // pointer to the start of the virtual address
    u64 max_size;               // reservation limit
    u64 committed_size;         // how much physical ram has been given
    u64 offset;                 // current bump pointer offset
    volatile i32 commit_lock;   // spin lock for OS page commits
} sft_arena;

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
};

struct sft_meshlet_inds {
    // 16 batches of 8 triangles,
    // each batch: 8x v0, 8x v1, 8x v2, 8x pad = 32 bytes.
    alignas(32) u8 inds[512]; // 16 * 32 = 512 bytes
};

typedef struct sft_mesh {
    // stored separately from culling data on purpose
    struct sft_meshlet* meshlets;
    struct sft_meshlet_attribs* meshlet_attribs;
    u32 meshlet_count;
    u32 model_meshlet_offset; // starting ind of this mesh's meshlets within entire model

    // pre swizzled inds for 128 triangles per meshlet
    struct sft_meshlet_inds* inds;
    
    // bounding spheres
    f32* bounds_center_x;
    f32* bounds_center_y;
    f32* bounds_center_z;
    f32* bounds_radius;

    // normal cones
    f32* cone_axis_x;
    f32* cone_axis_y;
    f32* cone_axis_z;
    f32* cone_cutoff; 

    sft_mat_handle material;
} sft_mesh;

typedef struct sft_model {
    // flat array of immutable meshes, multiple instances point to this single model
    struct sft_mesh* meshes;
    u32 mesh_count;

    u32 total_meshlets;
    u16* meshlet_mesh_inds; // maps a model relative meshlet ind to its parent mesh ind
} sft_model;

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
    f32 px[3], py[3], pz[3];
    f32 nx[3], ny[3], nz[3];
    u32 inst_ind;
    u32 m_ind;
};

struct sft_clip_vert {
    f32 tx, ty, tz, w;
    f32 u, v;
    f32 px, py, pz;
    f32 nx, ny, nz;
};

struct sft_sphere {
    sftvec center;
    f32 radius;
};

struct sft_frustum {
    sftvvec normal[6];
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

    struct {
        // pointers to virtual reservations
        sft_instance* instances;
        struct sft_geom_job* jobs;
        
        struct sft_atomic_counter instance_count;
        struct sft_atomic_counter job_count;
        struct sft_atomic_counter total_ss_meshlets; // allocated screen space meshlets
    } cmd_queue;

    struct sft_atomic_counter next_job_ind;
    struct sft_atomic_counter next_tile_ind;
    struct sft_atomic_counter next_chunk_ind;
    
    struct sft_barrier geom_barrier;
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
    } lighting;

    struct sft_frustum frustum;
    
    struct {
        f32 half_width, half_height;
        f32 vp_x, vp_y;

        i32 tile_cols, tile_rows;
        u32* color_buffer;

        struct sft_payload_chunk* chunk_pool;

        i32* tile_heads;
        volatile i32* tile_tails;
        i32* tile_payload_counts; // global heuristic counter
        i32* sorted_tiles;
    } display;

    struct sft_ss_meshlet {
        alignas(32) f32 sx[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 sy[SFT_VERTS_PER_MESHLET];
        alignas(32) f32 inv_w[SFT_VERTS_PER_MESHLET];
    }* ss_cache;

    struct {
        struct sft_clipped_triangle* pool;
        volatile i32 count;
    } clip_cache;

    sftvec camera_pos;
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
    if (fabsf(forward.x) < 0.001f && fabsf(forward.z) < 0.001f) {
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
    DWORD length = 0;
    GetLogicalProcessorInformation(NULL, &length);
    if (0 == length) {
        return 1;
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(length);
    GetLogicalProcessorInformation(buffer, &length);

    i32 physical_cores = 0;
    i32 count = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (i32 i = 0; i < count; i += 1) {
        if (RelationProcessorCore == buffer[i].Relationship) {
            physical_cores += 1;
        }
    }

    free(buffer);
    return physical_cores > 0 ? physical_cores : 1;

#elif defined(__linux__)
    i32 physical_cores = 0;
    
    // gives the number of processors the OS is configured for
    i32 max_cpus = sysconf(_SC_NPROCESSORS_CONF);
    if (max_cpus < 1) {
        max_cpus = 1;
    }

    // track seen physical cores based on their primary thread ID to avoid double counting
    i32* seen;
    SFT_CALLOC(seen, max_cpus, i32);
    for (i32 i = 0; i < max_cpus; i += 1) {
        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", i);
        
        FILE* f = fopen(path, "r");
        if (f) {
            i32 first_sibling = -1;
            // thread_siblings_list is usually formatted as "0,4" or "0-3". 
            // reading the first %d extracts the primary logical CPU ID 
            // associated with this physical core
            if (1 == fscanf(f, "%d", &first_sibling)) {
                if (first_sibling >= 0 && first_sibling < max_cpus && 0 == seen[first_sibling]) {
                    seen[first_sibling] = 1;
                    physical_cores += 1;
                }
            }
            fclose(f);
        }
    }
    free(seen);

    if (physical_cores > 0) {
        return physical_cores;
    }

    // fallback if sysfs is unreadable (e.g., inside restricted containers/chroots)
    const i32 logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (logical_cores > 1) ? logical_cores / 2 : 1;

#elif defined(__APPLE__)
    #include <sys/sysctl.h>
    i32 physical_cores = 1;
    size_t size = sizeof(physical_cores);
    sysctlbyname("hw.physicalcpu", &physical_cores, &size, NULL, 0);
    return (physical_cores > 0) ? physical_cores : 1;

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
    struct sft_sphere* out_sphere
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
}

static void sft__builder_reset(struct sft_meshlet_builder* builder) {
    builder->vert_count = 0;
    builder->tri_count = 0;
    builder->cone_axis = (sftvec){0};
}

static void sft__barrier_init(struct sft_barrier* b, i32 thread_count) {
    b->count = 0;
    b->generation = 0;
    b->thread_count = thread_count;
}

// TODO, IMPORTANT, update to have preprocessing for linux
static SFT_INLINE void sft__barrier_wait(struct sft_barrier* b) {
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
        sft__builder_finalize(
            &builder, raw_vx, raw_vy, raw_vz, 
            &out_mesh->meshlets[meshlet_count], 
            &out_mesh->inds[meshlet_count], 
            &sphere);

        out_mesh->bounds_center_x[meshlet_count] = sphere.center.x;
        out_mesh->bounds_center_y[meshlet_count] = sphere.center.y;
        out_mesh->bounds_center_z[meshlet_count] = sphere.center.z;
        out_mesh->bounds_radius[meshlet_count]   = sphere.radius;

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

static SFT_INLINE struct sft_clip_vert sft__lerp_clip_vert(struct sft_clip_vert a, struct sft_clip_vert b, f32 t) {
    return (struct sft_clip_vert){
        .tx = SFT_LERPF(a.tx, b.tx, t),
        .ty = SFT_LERPF(a.ty, b.ty, t),
        .tz = SFT_LERPF(a.tz, b.tz, t),
        .w  = SFT_LERPF(a.w,  b.w,  t),
        .u  = SFT_LERPF(a.u,  b.u,  t),
        .v  = SFT_LERPF(a.v,  b.v,  t),
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
            out[0][0] = in[0]; out[0][1] = in[1]; out[0][2] = in[2];
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
    sem_wait(s);
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
        const struct sft_meshlet* m_verts = &mesh->meshlets[local_m_ind];

        const struct sft_clip_vert in_tri[3] = { {
            .tx = clip_tx[i0], .ty = clip_ty[i0], .tz = clip_tz[i0], .w = clip_w[i0],
            .u = m_attribs->u[i0], .v = m_attribs->v[i0],
            .px = m_verts->x[i0], .py = m_verts->y[i0], .pz = m_verts->z[i0],
            .nx = m_attribs->nx[i0], .ny = m_attribs->ny[i0], .nz = m_attribs->nz[i0]
        }, {
            .tx = clip_tx[i1], .ty = clip_ty[i1], .tz = clip_tz[i1], .w = clip_w[i1],
            .u = m_attribs->u[i1], .v = m_attribs->v[i1],
            .px = m_verts->x[i1], .py = m_verts->y[i1], .pz = m_verts->z[i1],
            .nx = m_attribs->nx[i1], .ny = m_attribs->ny[i1], .nz = m_attribs->nz[i1]
        }, {
            .tx = clip_tx[i2], .ty = clip_ty[i2], .tz = clip_tz[i2], .w = clip_w[i2],
            .u = m_attribs->u[i2], .v = m_attribs->v[i2],
            .px = m_verts->x[i2], .py = m_verts->y[i2], .pz = m_verts->z[i2],
            .nx = m_attribs->nx[i2], .ny = m_attribs->ny[i2], .nz = m_attribs->nz[i2]
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

        sftmat mvp = sft_m_mul(inst->transform, sft_state->view_proj);
        sftvmat v_mvp = sft_vm_broadcast(mvp);
        const sftvmat v_transform = sft_vm_broadcast(inst->transform);

        const vf32 v_scale = sft_vf_set1(sqrtf(inst->transform.m[0][0]*inst->transform.m[0][0] +
                                               inst->transform.m[0][1]*inst->transform.m[0][1] +
                                               inst->transform.m[0][2]*inst->transform.m[0][2]));
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
            const vf32 radius = sft_vf_mul(sft_vf_load(&mesh->bounds_radius[m_ind]), v_scale);
            center = sft_vv_transform(center, v_transform);

            vf32 inside_mask = sft_vf_cast_vi32(sft_vi_set1(0x80000000)); 
            for (i32 p = 0; p < 6; p += 1) {
                vf32 dist = sft_vf_add(sft_vv_dot(center, sft_state->frustum.normal[p]), sft_state->frustum.d[p]);
                inside_mask = sft_vf_and(inside_mask, sft_vf_cmp_ge(sft_vf_add(dist, radius), v_zero));
            }

            sftvvec cone_axis;
            cone_axis.x = sft_vf_load(&mesh->cone_axis_x[m_ind]);
            cone_axis.y = sft_vf_load(&mesh->cone_axis_y[m_ind]);
            cone_axis.z = sft_vf_load(&mesh->cone_axis_z[m_ind]);
            cone_axis = sft_vv_transform_dir(cone_axis, v_transform);
            cone_axis = sft_vv_norm(cone_axis);

            const sftvvec v_raw = sft_vv_sub(center, v_camera_pos);
            const vf32 d = sft_vv_length(v_raw);
            const vf32 dot_val = sft_vv_dot(v_raw, cone_axis);
            const vf32 sin_alpha = sft_vf_load(&mesh->cone_cutoff[m_ind]);
            const vf32 threshold = sft_vf_fmadd(d, sin_alpha, radius);
            const vf32 cone_mask = sft_vf_cmp_le(dot_val, threshold);
            
            inside_mask = sft_vf_and(inside_mask, cone_mask);
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

                const u32 surviving_m_ind = m_ind + lane; // Local to the mesh
                const u32 global_m_ind = mesh->model_meshlet_offset + surviving_m_ind; // global to the model

                const struct sft_meshlet* m_verts = &mesh->meshlets[surviving_m_ind];
                const struct sft_meshlet_inds* m_inds = &mesh->inds[surviving_m_ind];
                
                // ss_cache tracks meshlets globally across the instance
                const u32 ss_m_ind = inst->ss_meshlet_offset + global_m_ind;

                // ---  vertex transforms  ---
                for (u32 v_chunk = 0; v_chunk < (SFT_VERTS_PER_MESHLET / SFT_SIMD_WIDTH); v_chunk += 1) {
                    const u32 v_ind = v_chunk * SFT_SIMD_WIDTH;
                    
                    sftvvec v;
                    v.x = sft_vf_load(&m_verts->x[v_ind]);
                    v.y = sft_vf_load(&m_verts->y[v_ind]);
                    v.z = sft_vf_load(&m_verts->z[v_ind]);

                    vf32 w = sft_vf_fmadd(v.z, v_mvp.m[2][3], v_mvp.m[3][3]);
                         w = sft_vf_fmadd(v.y, v_mvp.m[1][3], w); 
                         w = sft_vf_fmadd(v.x, v_mvp.m[0][3], w);

                    vf32 tx = sft_vf_fmadd(v.z, v_mvp.m[2][0], v_mvp.m[3][0]);
                         tx = sft_vf_fmadd(v.y, v_mvp.m[1][0], tx); 
                         tx = sft_vf_fmadd(v.x, v_mvp.m[0][0], tx);

                    vf32 ty = sft_vf_fmadd(v.z, v_mvp.m[2][1], v_mvp.m[3][1]);
                         ty = sft_vf_fmadd(v.y, v_mvp.m[1][1], ty); 
                         ty = sft_vf_fmadd(v.x, v_mvp.m[0][1], ty);

                    vf32 tz = sft_vf_fmadd(v.z, v_mvp.m[2][2], v_mvp.m[3][2]);
                         tz = sft_vf_fmadd(v.y, v_mvp.m[1][2], tz); 
                         tz = sft_vf_fmadd(v.x, v_mvp.m[0][2], tz);

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

static SFT_INLINE void sft__resolve_tile(
    const f32* depth_buf, const u32* vid_buf,
    i32 tile_min_x, i32 tile_min_y
) {
    const i32 screen_w = (i32)(sft_state->display.half_width * 2.f);
    const i32 screen_h = (i32)(sft_state->display.half_height * 2.f);

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
            const f32 radius = sft_state->lighting.point[l].radius * 100.f;
            // TODO remove the "* 100.f" hack                      ^^^^^^^

            const f32 w = pos.x * vp->m[0][3] + pos.y * vp->m[1][3] + pos.z * vp->m[2][3] + vp->m[3][3];
            
            // Z cull
            if ((w + radius) < tile_min_z || (w - radius) > tile_max_z) {
                continue; 
            }

            // screen space 2D bounds check
            if (w > radius) { 
                const f32 tx = pos.x * vp->m[0][0] + pos.y * vp->m[1][0] + pos.z * vp->m[2][0] + vp->m[3][0];
                const f32 ty = pos.x * vp->m[0][1] + pos.y * vp->m[1][1] + pos.z * vp->m[2][1] + vp->m[3][1];
                
                const f32 sx = (tx / w + 1.f) * hw + vpx;
                const f32 sy = (-ty / w + 1.f) * hh + vpy;

                const f32 safe_w = SFT_MAX(w - radius, 0.001f);
                const f32 rx_screen = fabsf((radius * vp->m[0][0]) / safe_w * hw);
                const f32 ry_screen = fabsf((radius * vp->m[1][1]) / safe_w * hh);

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

            // draw background, TODO maybe make optional
            // i32 empty_mask = sft_vi_movemask(sft_vi_and(v_is_empty, v_in_bounds)) & ((1 << SFT_SIMD_WIDTH) - 1);
            // while (empty_mask) {
            //     const i32 lane = sft__tzcnt(empty_mask);
            //     empty_mask &= (empty_mask - 1);
            //     const i32 px = x + ix_offsets_arr[lane];
            //     const i32 py = y + iy_offsets_arr[lane];
            //     sft_state->display.color_buffer[py * screen_w + px] = 0xFFFF2222;
            // }

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
                vf32 sx0, sy0, inv_w0, u0, v0;
                vf32 sx1, sy1, inv_w1, u1, v1;
                vf32 sx2, sy2, inv_w2, u2, v2;
                vf32 px0, py0, pz0, nx0, ny0, nz0;
                vf32 px1, py1, pz1, nx1, ny1, nz1;
                vf32 px2, py2, pz2, nx2, ny2, nz2;

                const u32 is_clipped = target_vid & (1U << 31);
                
                if (is_clipped) {
                    const u32 clip_slot = target_vid & ~((u32)1 << 31);
                    const struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[clip_slot];
                    
                    inst_ind = ct->inst_ind;
                    m_ind = ct->m_ind;
                    
                    sx0 = sft_vf_set1(ct->sx[0]), sy0 = sft_vf_set1(ct->sy[0]), inv_w0 = sft_vf_set1(ct->inv_w[0]);
                    u0 = sft_vf_set1(ct->u[0]), v0 = sft_vf_set1(ct->v[0]);
                    px0 = sft_vf_set1(ct->px[0]), py0 = sft_vf_set1(ct->py[0]), pz0 = sft_vf_set1(ct->pz[0]);
                    nx0 = sft_vf_set1(ct->nx[0]), ny0 = sft_vf_set1(ct->ny[0]), nz0 = sft_vf_set1(ct->nz[0]);
                    
                    sx1 = sft_vf_set1(ct->sx[1]), sy1 = sft_vf_set1(ct->sy[1]), inv_w1 = sft_vf_set1(ct->inv_w[1]);
                    u1 = sft_vf_set1(ct->u[1]), v1 = sft_vf_set1(ct->v[1]);
                    px1 = sft_vf_set1(ct->px[1]), py1 = sft_vf_set1(ct->py[1]), pz1 = sft_vf_set1(ct->pz[1]);
                    nx1 = sft_vf_set1(ct->nx[1]), ny1 = sft_vf_set1(ct->ny[1]), nz1 = sft_vf_set1(ct->nz[1]);
                    
                    sx2 = sft_vf_set1(ct->sx[2]), sy2 = sft_vf_set1(ct->sy[2]), inv_w2 = sft_vf_set1(ct->inv_w[2]);
                    u2 = sft_vf_set1(ct->u[2]), v2 = sft_vf_set1(ct->v[2]);
                    px2 = sft_vf_set1(ct->px[2]), py2 = sft_vf_set1(ct->py[2]), pz2 = sft_vf_set1(ct->pz[2]);
                    nx2 = sft_vf_set1(ct->nx[2]), ny2 = sft_vf_set1(ct->ny[2]), nz2 = sft_vf_set1(ct->nz[2]);
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
                    const struct sft_meshlet* m_verts = &mesh->meshlets[local_m_ind];
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
                    
                    sx1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sx[i1]);
                    sy1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sy[i1]);
                    inv_w1 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].inv_w[i1]);
                    u1 = sft_vf_set1(m_attribs->u[i1]), v1 = sft_vf_set1(m_attribs->v[i1]);
                    
                    sx2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sx[i2]);
                    sy2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].sy[i2]);
                    inv_w2 = sft_vf_set1(sft_state->ss_cache[ss_m_ind].inv_w[i2]);
                    u2 = sft_vf_set1(m_attribs->u[i2]), v2 = sft_vf_set1(m_attribs->v[i2]);

                    px0 = sft_vf_set1(m_verts->x[i0]), py0 = sft_vf_set1(m_verts->y[i0]), pz0 = sft_vf_set1(m_verts->z[i0]);
                    nx0 = sft_vf_set1(m_attribs->nx[i0]), ny0 = sft_vf_set1(m_attribs->ny[i0]), nz0 = sft_vf_set1(m_attribs->nz[i0]);
                    
                    px1 = sft_vf_set1(m_verts->x[i1]), py1 = sft_vf_set1(m_verts->y[i1]), pz1 = sft_vf_set1(m_verts->z[i1]);
                    nx1 = sft_vf_set1(m_attribs->nx[i1]), ny1 = sft_vf_set1(m_attribs->ny[i1]), nz1 = sft_vf_set1(m_attribs->nz[i1]);
                    
                    px2 = sft_vf_set1(m_verts->x[i2]), py2 = sft_vf_set1(m_verts->y[i2]), pz2 = sft_vf_set1(m_verts->z[i2]);
                    nx2 = sft_vf_set1(m_attribs->nx[i2]), ny2 = sft_vf_set1(m_attribs->ny[i2]), nz2 = sft_vf_set1(m_attribs->nz[i2]);
                }

                // ---  fetch and transform to world space  ---
                const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
                const sftvmat v_transform = sft_vm_broadcast(inst->transform);

                const sftvvec wp0 = sft_vv_transform((sftvvec){px0, py0, pz0}, v_transform);
                const sftvvec wp1 = sft_vv_transform((sftvvec){px1, py1, pz1}, v_transform);
                const sftvvec wp2 = sft_vv_transform((sftvvec){px2, py2, pz2}, v_transform);

                // use transform_dir for normals to ignore translation
                const sftvvec wn0 = sft_vv_transform_dir((sftvvec){nx0, ny0, nz0}, v_transform);
                const sftvvec wn1 = sft_vv_transform_dir((sftvvec){nx1, ny1, nz1}, v_transform);
                const sftvvec wn2 = sft_vv_transform_dir((sftvvec){nx2, ny2, nz2}, v_transform);

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
                
                // V = normalize(cam_pos - world_pos)
                const sftvvec V = sft_vv_norm_fast(sft_vv_sub(v_cam_pos, interp_pos));

                // ---  material lookup  ---
                const u16 mesh_ind = inst->model->meshlet_mesh_inds[m_ind];
                const sft_mat_handle mat_handle = inst->model->meshes[mesh_ind].material;
                
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

                // ---  lighting accumulation setup  ---
                const vf32 v_n = sft_vf_set1(mat_shininess);
                const vf32 v_specular_scale = sft_vf_set1(((mat_shininess + 8.f) / 8.f) * 0.04f);

                // initialize with ambient
                vf32 diffuse_r = sft_vf_set1(sft_state->ambient_light.x);
                vf32 diffuse_g = sft_vf_set1(sft_state->ambient_light.y);
                vf32 diffuse_b = sft_vf_set1(sft_state->ambient_light.z);
                
                vf32 spec_r = v_zero;
                vf32 spec_g = v_zero;
                vf32 spec_b = v_zero;

                // ---  directional lights accumulation loop  ---
                const i32 dir_count = sft_state->lighting.directional_count.count;
                for (i32 l = 0; l < dir_count; l += 1) {
                    const sftvec l_dir = sft_state->lighting.directional[l].dir;
                    const sftvec l_col = sft_state->lighting.directional[l].color;
                    
                    const sftvvec L = { sft_vf_set1(l_dir.x), sft_vf_set1(l_dir.y), sft_vf_set1(l_dir.z) };
                    const vf32 lr = sft_vf_set1(l_col.x);
                    const vf32 lg = sft_vf_set1(l_col.y);
                    const vf32 lb = sft_vf_set1(l_col.z);

                    // diffuse
                    const vf32 n_dot_l = sft_vf_max(sft_vv_dot(N, L), v_zero);
                    const vf32 lit_mask = sft_vf_cmp_gt(n_dot_l, v_zero);
                    
                    // diffuse += n_dot_l * light_color
                    diffuse_r = sft_vf_fmadd(n_dot_l, lr, diffuse_r);
                    diffuse_g = sft_vf_fmadd(n_dot_l, lg, diffuse_g);
                    diffuse_b = sft_vf_fmadd(n_dot_l, lb, diffuse_b);

                    // skip specular math if the block is entirely unlit
                    if (0 == sft_vf_movemask(lit_mask)) {
                        continue;
                    }

                    // specular
                    const sftvvec H = sft_vv_norm_fast(sft_vv_add(L, V));
                    const vf32 n_dot_h = sft_vf_max(sft_vv_dot(N, H), v_zero);
                    
                    const vf32 denom = sft_vf_add(sft_vf_sub(v_n, sft_vf_mul(v_n, n_dot_h)), n_dot_h);
                    const vf32 spec = sft_vf_mul(n_dot_h, sft_vf_rcp_refined(denom));
                    
                    vf32 specular_term = sft_vf_mul(sft_vf_mul(spec, v_specular_scale), n_dot_l);
                    specular_term = sft_vf_and(specular_term, lit_mask);

                    // spec += specular_term * light_color
                    spec_r = sft_vf_fmadd(specular_term, lr, spec_r);
                    spec_g = sft_vf_fmadd(specular_term, lg, spec_g);
                    spec_b = sft_vf_fmadd(specular_term, lb, spec_b);
                }

                // ---  point lights accumulation loop  ---
                for (i32 i = 0; i < tile_light_count; i += 1) {
                    const u16 l = tile_lights[i];
                    
                    const sftvec l_pos = sft_state->lighting.point[l].pos;
                    const sftvec l_col = sft_state->lighting.point[l].color;
                    const f32 l_inv_r2 = sft_state->lighting.point[l].inv_radius_sqr;
                    
                    const sftvvec v_light_pos = { sft_vf_set1(l_pos.x), sft_vf_set1(l_pos.y), sft_vf_set1(l_pos.z) };
                    const vf32 lr = sft_vf_set1(l_col.x);
                    const vf32 lg = sft_vf_set1(l_col.y);
                    const vf32 lb = sft_vf_set1(l_col.z);
                    const vf32 v_inv_r2 = sft_vf_set1(l_inv_r2);

                    // L_raw = light_pos - world_pos
                    const sftvvec L_raw = sft_vv_sub(v_light_pos, interp_pos);
                    
                    // distance squared
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

                    // diffuse
                    const vf32 n_dot_l = sft_vf_max(sft_vv_dot(N, L), v_zero);
                    vf32 lit_mask = sft_vf_and(sft_vf_cmp_gt(n_dot_l, v_zero), active_light_mask);
                    
                    // diffuse += n_dot_l * light_color * attenuation
                    const vf32 diff_intensity = sft_vf_mul(n_dot_l, attenuation);
                    diffuse_r = sft_vf_fmadd(diff_intensity, lr, diffuse_r);
                    diffuse_g = sft_vf_fmadd(diff_intensity, lg, diffuse_g);
                    diffuse_b = sft_vf_fmadd(diff_intensity, lb, diffuse_b);

                    // skip specular math if the block is entirely unlit
                    if (0 == sft_vf_movemask(lit_mask)) {
                        continue; 
                    }

                    // specular
                    const sftvvec H = sft_vv_norm_fast(sft_vv_add(L, V));
                    const vf32 n_dot_h = sft_vf_max(sft_vv_dot(N, H), v_zero);
                    
                    const vf32 denom = sft_vf_add(sft_vf_sub(v_n, sft_vf_mul(v_n, n_dot_h)), n_dot_h);
                    const vf32 spec = sft_vf_mul(n_dot_h, sft_vf_rcp_refined(denom));
                    
                    vf32 specular_term = sft_vf_mul(sft_vf_mul(spec, v_specular_scale), diff_intensity);
                    specular_term = sft_vf_and(specular_term, lit_mask);

                    // spec += specular_term * light_color
                    spec_r = sft_vf_fmadd(specular_term, lr, spec_r);
                    spec_g = sft_vf_fmadd(specular_term, lg, spec_g);
                    spec_b = sft_vf_fmadd(specular_term, lb, spec_b);
                }

                // ---  final color combination  ---
                // final = (albedo * diffuse_total) + spec_total
                vf32 final_r = sft_vf_fmadd(albedo_r, diffuse_r, spec_r);
                vf32 final_g = sft_vf_fmadd(albedo_g, diffuse_g, spec_g);
                vf32 final_b = sft_vf_fmadd(albedo_b, diffuse_b, spec_b);

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
                    sft_state->display.color_buffer[py * screen_w + px] = sft_vi_extract(v_color, lane);
                }
            }
        }
    }
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
        
        // TODO real resolve phase

        sft_sem_post(&sft_state->sem_done, 1);
    }
}


/*================================================================
:: OPTIONAL LIBRARIES (CGLTF, STB_IMAGE)
================================================================*/

#ifdef SFT_USE_CGLTF

#ifndef SFT_CGLTF_PATH
    #define SFT_CGLTF_PATH "cgltf.h"
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
    #define SFT_STB_IMAGE_PATH "stb_image.h"
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

#ifdef SFT_USE_CGLTF

// finalizer that packs normals and UVs into SoA format
static void sft__builder_finalize_attribs(
    const struct sft_meshlet_builder* builder, 
    const f32* raw_vx, const f32* raw_vy, const f32* raw_vz,
    const f32* raw_nx, const f32* raw_ny, const f32* raw_nz,
    const f32* raw_u, const f32* raw_v,
    struct sft_meshlet* out_meshlet,
    struct sft_meshlet_attribs* out_attribs,
    struct sft_meshlet_inds* out_inds,
    struct sft_sphere* out_sphere
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
            out_inds->inds[batch_offset + (0 * SFT_SIMD_WIDTH) + lane] = v0;
            out_inds->inds[batch_offset + (1 * SFT_SIMD_WIDTH) + lane] = v1;
            out_inds->inds[batch_offset + (2 * SFT_SIMD_WIDTH) + lane] = v2;
            out_inds->inds[batch_offset + (3 * SFT_SIMD_WIDTH) + lane] = 0;
        }
    }

    sftvec temp_verts[SFT_VERTS_PER_MESHLET];

    for (u32 i = 0; i < SFT_VERTS_PER_MESHLET; i += 1) {
        if (i < builder->vert_count) {
            const i32 gi = builder->global_vert_inds[i];
            out_meshlet->x[i] = raw_vx[gi];
            out_meshlet->y[i] = raw_vy[gi];
            out_meshlet->z[i] = raw_vz[gi];
            out_attribs->nx[i] = raw_nx[gi];
            out_attribs->ny[i] = raw_ny[gi];
            out_attribs->nz[i] = raw_nz[gi];
            out_attribs->u[i] = raw_u[gi];
            out_attribs->v[i] = raw_v[gi];

            temp_verts[i].x = raw_vx[gi];
            temp_verts[i].y = raw_vy[gi];
            temp_verts[i].z = raw_vz[gi];
        }
        else {
            out_meshlet->x[i] = out_meshlet->y[i] = out_meshlet->z[i] = 0.f;
            out_attribs->nx[i] = out_attribs->ny[i] = out_attribs->nz[i] = 0.f;
            out_attribs->u[i] = out_attribs->v[i] = 0.f;
        }
    }

    if (out_sphere && builder->vert_count > 0) {
        *out_sphere = sft__meshlet_bounding_sphere(temp_verts, builder->vert_count);
    }
}

SFT_FUNC sft_model* sft_model_load_glb(const char* filename, sft_arena* arena, i32 uv_channel) {
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

    // ---  allocate model  ---
    sft_model* model = (sft_model*)sft_arena_alloc(arena, sizeof(sft_model));
    // maximum possible meshes is materials_count + 1 (for primitives with no material)
    model->meshes = (sft_mesh*)sft_arena_alloc(arena, (data->materials_count + 1) * sizeof(sft_mesh));
    u32 active_meshes = 0;

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
        i32* uni_inds = (i32*)malloc(total_tris * 3 * sizeof(i32));

        i32 v_offset = 0;
        i32 i_offset = 0;

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

                cgltf_accessor *pos_a = NULL, *norm_a = NULL, *uv_a = NULL;
                for (cgltf_size a = 0; a < prim->attributes_count; a += 1) {
                    switch (prim->attributes[a].type) {
                        case cgltf_attribute_type_position: pos_a  = prim->attributes[a].data; break;
                        case cgltf_attribute_type_normal:   norm_a = prim->attributes[a].data; break;
                        case cgltf_attribute_type_texcoord: {
                            if (uv_channel == prim->attributes[a].index) {
                                uv_a = prim->attributes[a].data;
                            }
                        } break;
                        default: break;
                    }
                }

                const i32 prim_v_count = (i32)pos_a->count;
                for (i32 v = 0; v < prim_v_count; v += 1) {
                    f32 buf[3] = {0};
                    cgltf_accessor_read_float(pos_a, v, buf, 3);
                    
                    const f32 tx = buf[0] * world_mat[0] + buf[1] * world_mat[4] + buf[2] * world_mat[8] + world_mat[12];
                    const f32 ty = buf[0] * world_mat[1] + buf[1] * world_mat[5] + buf[2] * world_mat[9] + world_mat[13];
                    const f32 tz = buf[0] * world_mat[2] + buf[1] * world_mat[6] + buf[2] * world_mat[10] + world_mat[14];

                    uni_vx[v_offset + v] = tx;
                    uni_vy[v_offset + v] = ty;
                    uni_vz[v_offset + v] = -tz;

                    if (norm_a) {
                        cgltf_accessor_read_float(norm_a, v, buf, 3);
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
        out_mesh->cone_axis_x = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_axis_y = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_axis_z = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));
        out_mesh->cone_cutoff = (f32*)sft_arena_alloc(arena, max_meshlets * sizeof(f32));

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
            sft__builder_finalize_attribs(
                &builder, uni_vx, uni_vy, uni_vz, uni_nx, uni_ny, uni_nz, uni_u, uni_v,
                &out_mesh->meshlets[meshlet_count], &out_mesh->meshlet_attribs[meshlet_count],
                &out_mesh->inds[meshlet_count], &sphere);

            out_mesh->bounds_center_x[meshlet_count] = sphere.center.x;
            out_mesh->bounds_center_y[meshlet_count] = sphere.center.y;
            out_mesh->bounds_center_z[meshlet_count] = sphere.center.z;
            out_mesh->bounds_radius[meshlet_count]   = sphere.radius;

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
    }

    cgltf_free(data);
    free(file_content);
    free(mat_handles);

    return model;
}

#endif // SFT_USE_CGLTF


/*================================================================
:: FUNCTION DEFINITIONS (OTHER)
================================================================*/

SFT_FUNC void sft_init(i32 width, i32 height, i32 requested_threads) {
    SFT_CALLOC(sft_state, 1, struct sft_state);

    { // ---  initialize multithreading  ---
        if (requested_threads > 0) {
            sft_state->thread_count = requested_threads;
        }
        else {
            sft_state->thread_count = sft__get_physical_core_count();
        }
    
        // initialize barriers and semaphores
        sft__barrier_init(&sft_state->geom_barrier, sft_state->thread_count);
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
            sft_state->thread_count, SFT_SIMD_WIDTH, (SFT_SIMD_WIDTH > 1) ? "lanes" : "lane");
    }

    { // ---  initialize arenas  ---
        sft_arena_init(&sft_state->instance_arena, 32ULL * 1024 * 1024);
        sft_state->cmd_queue.instances = (sft_instance*)sft_state->instance_arena.base;
        
        sft_arena_init(&sft_state->job_arena, 64ULL * 1024 * 1024);
        sft_state->cmd_queue.jobs = (struct sft_geom_job*)sft_state->job_arena.base;

        sft_arena_init(&sft_state->chunk_arena, 512ULL * 1024 * 1024);
        sft_state->display.chunk_pool = (struct sft_payload_chunk*)sft_state->chunk_arena.base;
        
        sft_arena_init(&sft_state->scratch_arena, 2ULL * 1024 * 1024 * 1024);
        sft_state->ss_cache = (struct sft_ss_meshlet*)sft_state->scratch_arena.base;

        sft_arena_init(&sft_state->clip_arena, 32ULL * 1024 * 1024);
        sft_state->clip_cache.pool = (struct sft_clipped_triangle*)sft_state->clip_arena.base;

        sft_arena_init(&sft_state->tex_heap_arena, 4ULL * 1024 * 1024 * 1024);
        sft_state->mat_pool.free_head = -1;
        sft_state->tex_pool.free_head = -1;

        sft_arena_init(&sft_state->light_arena, 16ULL * 1024 * 1024);
        // directional at the start and point offset by 8MB
        sft_state->lighting.directional = (sft_directional_light*)sft_state->light_arena.base;
        sft_state->lighting.point = (sft_point_light*)(sft_state->light_arena.base + (8ULL * 1024 * 1024));
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
        SFT_CALLOC(sft_state->display.color_buffer, width * height, u32);
        SFT_CALLOC(sft_state->display.tile_heads, total_tiles, i32);
        SFT_CALLOC(sft_state->display.tile_tails, total_tiles, volatile i32);
        SFT_CALLOC(sft_state->display.tile_payload_counts, total_tiles, i32);
        SFT_CALLOC(sft_state->display.sorted_tiles, total_tiles, i32);
    }

    { // ---  initialize default material  ---
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

    SFT_FREE(sft_state);
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

    // exact fit bulk commit for the screen space cache
    if (sft_state->cmd_queue.total_ss_meshlets.count > 0) {
        sft_arena_commit_if_needed(&sft_state->scratch_arena, 
            sft_state->cmd_queue.total_ss_meshlets.count * sizeof(struct sft_ss_meshlet));
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
}

SFT_FUNC void sft_set_camera(sftvec cam_pos, sftmat view, sftmat proj) {
    sft_state->camera_pos = cam_pos;
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

    for (i32 p = 0; p < 6; p += 1) {
        const f32 length = sqrtf(planes[p][0]*planes[p][0] + planes[p][1]*planes[p][1] + planes[p][2]*planes[p][2]);
        sft_state->frustum.normal[p].x = sft_vf_set1(planes[p][0] / length);
        sft_state->frustum.normal[p].y = sft_vf_set1(planes[p][1] / length);
        sft_state->frustum.normal[p].z = sft_vf_set1(planes[p][2] / length);
        sft_state->frustum.d[p] = sft_vf_set1(planes[p][3] / length);
    }
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

SFT_FUNC void sft_draw_point_light(sftvec pos, f32 r, f32 g, f32 b, f32 intensity, f32 radius) {
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
            SFT_ERR_EXIT("mmap reserve failed for size %llu\n", reserve_size);
        }

        // pretend it's all committed
        arena->committed_size = reserve_size; 
    #endif

    SFT_PRINT("arena initialized, reserved %llu bytes at %p\n", reserve_size, arena->base);
}

SFT_FUNC void* sft_arena_alloc(sft_arena* arena, u64 size) {
    if (!arena) {
        SFT_ERR_RET(NULL, "arena pointer is NULL\n");
    }

    // enforce 64 byte alignment.
    const u64 aligned_offset = (arena->offset + 63ULL) & ~63ULL;
    const u64 next_offset = aligned_offset + size;

    if (next_offset > arena->max_size) {
        SFT_ERR_EXIT("arena out of memory, requested %llu bytes, max %llu, offset %llu\n",
            size, arena->max_size, aligned_offset);
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
                SFT_ERR_EXIT("VirtualAlloc commit failed for %llu bytes at %p\n", chunks_to_commit, commit_target);
            }

            arena->committed_size += chunks_to_commit;
            SFT_PRINT("arena auto committed %llu bytes\n", chunks_to_commit);
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
                SFT_ERR_EXIT("VirtualAlloc commit failed for %llu bytes\n", chunks_to_commit);
            }

            arena->committed_size += chunks_to_commit;
        }

        sft_atomic_exchange(&arena->commit_lock, 0);
    }
#endif
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
