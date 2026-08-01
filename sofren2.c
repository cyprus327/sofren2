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

// total of 32
#define SFT_INSTANCE_BITS 14
#define SFT_MESHLET_BITS 11
#define SFT_TRIANGLE_BITS 7
#define SFT_MAX_INSTANCES_PER_FRAME (1 << SFT_INSTANCE_BITS)

#define SFT_VERTS_PER_MESHLET 64

#define SFT_MESHLETS_PER_JOB 2

#define SFT_CHUNK_CAPACITY 63

#define SFT_TILE_SHIFT 5
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
        #define SFT_INLINE static __forceinline
    #elif defined(__GNUC__) || defined(__clang__)
        #define SFT_INLINE static inline __attribute__((always_inline))
    #else
        #define SFT_INLINE static inline
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
    typedef __m256  vf32;
    typedef __m256i vi32;

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

    // ---  vf32 bitwise and logic  ---
    #define sft_vf_and(a, b)            _mm256_and_ps((a), (b))
    #define sft_vf_or(a, b)             _mm256_or_ps((a), (b))
    #define sft_vf_andnot(a, b)         _mm256_andnot_ps((a), (b))
    
    #define sft_vf_cmp_lt(a, b)         _mm256_cmp_ps((a), (b), _CMP_LT_OQ)
    #define sft_vf_cmp_le(a, b)         _mm256_cmp_ps((a), (b), _CMP_LE_OQ)
    #define sft_vf_cmp_gt(a, b)         _mm256_cmp_ps((a), (b), _CMP_GT_OQ)
    #define sft_vf_cmp_ge(a, b)         _mm256_cmp_ps((a), (b), _CMP_GE_OQ)
    
    #define sft_vf_blendv(a, b, m)      _mm256_blendv_ps((b), (a), (m))
    #define sft_vf_movemask(a)          _mm256_movemask_ps(a)

    SFT_INLINE i32 sft_vi_hmin(vi32 v) {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i m1 = _mm_min_epi32(lo, hi);
        __m128i shuf1 = _mm_shuffle_epi32(m1, _MM_SHUFFLE(2,3,0,1));
        __m128i m2 = _mm_min_epi32(m1, shuf1);
        __m128i shuf2 = _mm_shuffle_epi32(m2, _MM_SHUFFLE(1,0,3,2));
        __m128i m3 = _mm_min_epi32(m2, shuf2);
        return _mm_cvtsi128_si32(m3);
    }
    SFT_INLINE i32 sft_vi_hmax(vi32 v) {
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
    #define sft_vi_loadu_8u_to_32i(ptr) _mm256_cvtepu8_epi32(_mm_loadl_epi64((const __m128i*)(ptr)))
    SFT_INLINE i32 sft_vi_extract(vi32 v, i32 lane) {
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

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            _mm256_add_epi32((a), (b))
    #define sft_vi_sub(a, b)            _mm256_sub_epi32((a), (b))
    #define sft_vi_max(a, b)            _mm256_max_epi32((a), (b))
    #define sft_vi_min(a, b)            _mm256_min_epi32((a), (b))
    #define sft_vi_and(a, b)            _mm256_and_si256((a), (b))
    #define sft_vi_or(a, b)             _mm256_or_si256((a), (b))
    #define sft_vi_srai(a, imm)         _mm256_srai_epi32((a), (imm))
    #define sft_vi_slli(a, imm)         _mm256_slli_epi32((a), (imm))
    #define sft_vi_movemask(a)          _mm256_movemask_ps(_mm256_castsi256_ps(a))

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         _mm256_cmpgt_epi32((a), _mm256_sub_epi32((b), _mm256_set1_epi32(1)))
    #define sft_vi_cmp_gt(a, b)         _mm256_cmpgt_epi32((a), (b))
    #define sft_vi_cmp_le(a, b)         _mm256_cmpgt_epi32(_mm256_add_epi32((b), _mm256_set1_epi32(1)), (a))

    // ---  casting  ---
    #define sft_vf_cast_vi32(a)         _mm256_castsi256_ps(a)
    #define sft_vi_cast_vf32(a)         _mm256_castps_si256(a)
    #define sft_vi_cvttps_epi32(v)      _mm256_cvttps_epi32(v)


#elif defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    
    #define SFT_SIMD_WIDTH 4
    typedef float32x4_t vf32;
    typedef int32x4_t   vi32;
    
    // ---  vf32 base and memory  ---
    #define sft_vf_setzero()            vdupq_n_f32(0.f)
    #define sft_vf_set1(v)              vdupq_n_f32(v)
    #define sft_vf_load(ptr)            vld1q_f32(ptr)
    #define sft_vf_loadu(ptr)           vld1q_f32(ptr)
    #define sft_vf_store(ptr, val)      vst1q_f32((ptr), (val))
    #define sft_vf_storeu(ptr, val)     vst1q_f32((ptr), (val))
    SFT_INLINE vf32 sft_vf_gather(const f32 *base, vi32 ind) {
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

    // ---  vf32 bitwise and logic  ---
    SFT_INLINE vf32 sft_vf_and(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }
    SFT_INLINE vf32 sft_vf_or(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }
    SFT_INLINE vf32 sft_vf_andnot(vf32 a, vf32 b) {
        return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(b), vreinterpretq_u32_f32(a)));
    }

    #define sft_vf_cmp_lt(a, b)         vreinterpretq_f32_u32(vcltq_f32((a), (b)))
    #define sft_vf_cmp_le(a, b)         vreinterpretq_f32_u32(vcleq_f32((a), (b)))
    #define sft_vf_cmp_gt(a, b)         vreinterpretq_f32_u32(vcgtq_f32((a), (b)))
    #define sft_vf_cmp_ge(a, b)         vreinterpretq_f32_u32(vcgeq_f32((a), (b)))

    SFT_INLINE vf32 sft_vf_blendv(vf32 a, vf32 b, vf32 mask) {
        return vbslq_f32(vreinterpretq_u32_f32(mask), a, b);
    }
    SFT_INLINE i32 sft_vf_movemask(vf32 a) {
        uint32x4_t sgn = vshrq_n_u32(vreinterpretq_u32_f32(a), 31);
        const uint32x4_t bit_weights = {1, 2, 4, 8};
        sgn = vmulq_u32(sgn, bit_weights);
        return vaddvq_u32(sgn);
    }

    SFT_INLINE i32 sft_vi_hmin(vi32 v) { return vminvq_s32(v); }
    SFT_INLINE i32 sft_vi_hmax(vi32 v) { return vmaxvq_s32(v); }

    // ---  vi32 base and memory  ---
    #define sft_vi_setzero()            vdupq_n_s32(0)
    #define sft_vi_set1(v)              vdupq_n_s32(v)
    #define sft_vi_load(ptr)            vld1q_s32((const i32*)(ptr))
    #define sft_vi_store(ptr, val)      vst1q_s32((i32*)(ptr), (val))
    SFT_INLINE vi32 sft_vi_loadu_8u_to_32i(const u8 *ptr) {
        return vreinterpretq_s32_u32(vmovl_u16(vmovl_u8(vld1_u8(ptr))));
    }
    SFT_INLINE i32 sft_vi_extract(vi32 v, i32 lane) {
        switch(lane) {
            case 0: return vgetq_lane_s32(v, 0);
            case 1: return vgetq_lane_s32(v, 1);
            case 2: return vgetq_lane_s32(v, 2);
            case 3: return vgetq_lane_s32(v, 3);
            default: return 0;
        }
    }

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            vaddq_s32((a), (b))
    #define sft_vi_sub(a, b)            vsubq_s32((a), (b))
    #define sft_vi_max(a, b)            vmaxq_s32((a), (b))
    #define sft_vi_min(a, b)            vminq_s32((a), (b))
    #define sft_vi_and(a, b)            vandq_s32((a), (b))
    #define sft_vi_or(a, b)             vorrq_s32((a), (b))
    #define sft_vi_srai(a, imm)         vshrq_n_s32((a), (imm))
    #define sft_vi_slli(a, imm)         vshlq_n_s32((a), (imm))
    SFT_INLINE i32 sft_vi_movemask(vi32 a) {
        const uint32x4_t sgn = vshrq_n_u32(vreinterpretq_u32_s32(a), 31);
        const uint32x4_t bit_weights = {1, 2, 4, 8};
        return vaddvq_u32(vmulq_u32(sgn, bit_weights));
    }

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         vreinterpretq_s32_u32(vcgeq_s32((a), (b)))
    #define sft_vi_cmp_gt(a, b)         vreinterpretq_s32_u32(vcgtq_s32((a), (b)))
    #define sft_vi_cmp_le(a, b)         vreinterpretq_s32_u32(vcleq_s32((a), (b)))

    // ---  casting  ---
    #define sft_vf_cast_vi32(a)         vreinterpretq_f32_s32(a)
    #define sft_vi_cast_vf32(a)         vreinterpretq_s32_f32(a)
    #define sft_vi_cvttps_epi32(v)      vcvtq_s32_f32(v)


#else
    #define SFT_SIMD_WIDTH 1
    typedef float vf32;
    typedef i32   vi32;
    
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

    // ---  vf32 bitwise and logic  ---
    SFT_INLINE vf32 sft_vf_and(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i &= ub.i; return ua.f;
    }
    SFT_INLINE vf32 sft_vf_or(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i |= ub.i; return ua.f;
    }
    SFT_INLINE vf32 sft_vf_andnot(vf32 a, vf32 b) {
        union { f32 f; u32 i; } ua, ub; ua.f = a; ub.f = b; ua.i = (~ua.i) & ub.i; return ua.f;
    }

    SFT_INLINE vf32 sft_vf_cmp_lt(vf32 a, vf32 b) { return (a <  b) ? -0.f : 0.f; }
    SFT_INLINE vf32 sft_vf_cmp_le(vf32 a, vf32 b) { return (a <= b) ? -0.f : 0.f; }
    SFT_INLINE vf32 sft_vf_cmp_gt(vf32 a, vf32 b) { return (a >  b) ? -0.f : 0.f; }
    SFT_INLINE vf32 sft_vf_cmp_ge(vf32 a, vf32 b) { return (a >= b) ? -0.f : 0.f; }

    SFT_INLINE vf32 sft_vf_blendv(vf32 a, vf32 b, vf32 mask) {
        union { f32 f; u32 i; } u; u.f = mask;
        return (u.i & 0x80000000) ? a : b; 
    }
    SFT_INLINE i32 sft_vf_movemask(vf32 a) {
        union { f32 f; u32 i; } u; u.f = a;
        return (u.i >> 31) & 1;
    }

    SFT_INLINE i32 sft_vi_hmin(vi32 v) { return v; }
    SFT_INLINE i32 sft_vi_hmax(vi32 v) { return v; }

    // ---  vi32 base and memory  ---
    #define sft_vi_setzero()            0
    #define sft_vi_set1(v)              (v)
    #define sft_vi_load(ptr)            (*(ptr))
    #define sft_vi_store(ptr, val)      (*(ptr) = (val))
    #define sft_vi_loadu_8u_to_32i(ptr) ((i32)*(const u8 *)(ptr))
    #define sft_vi_extract(v, lane)     (v)

    // ---  vi32 arithmetic and logic  ---
    #define sft_vi_add(a, b)            ((a) + (b))
    #define sft_vi_sub(a, b)            ((a) - (b))
    #define sft_vi_max(a, b)            ((a) > (b) ? (a) : (b))
    #define sft_vi_min(a, b)            ((a) < (b) ? (a) : (b))
    #define sft_vi_and(a, b)            ((a) & (b))
    #define sft_vi_or(a, b)             ((a) | (b))
    #define sft_vi_srai(a, imm)         ((a) >> (imm))
    #define sft_vi_slli(a, imm)         ((a) << (imm))
    #define sft_vi_movemask(a)          (((u32)(a) >> 31) & 1)

    // ---  vi32 comparisons  ---
    #define sft_vi_cmp_ge(a, b)         ((a) >= (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_gt(a, b)         ((a) >  (b) ? (i32)0x80000000 : 0)
    #define sft_vi_cmp_le(a, b)         ((a) <= (b) ? (i32)0x80000000 : 0)

    // ---  casting  ---
    SFT_INLINE vf32 sft_vf_cast_vi32(vi32 a) { 
        union { vi32 i; vf32 f; } u; 
        u.i = (-1 == a) ? 0x80000000 : a; 
        return u.f; 
    }
    SFT_INLINE vi32 sft_vi_cast_vf32(vf32 a) { union { vf32 f; vi32 i; } u; u.f = a; return u.i; }
    #define sft_vi_cvttps_epi32(v)      ((i32)(v))

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
    #define SFT_MEM_RESERVE MEM_RESERVE
    #define SFT_MEM_COMMIT MEM_COMMIT
#else
    #include <sys/mman.h>
    #ifndef MAP_ANONYMOUS
        #define MAP_ANONYMOUS 0x20
    #endif
#endif

// 32 MB
#define SFT_ARENA_COMMIT_CHUNK (32ULL * 1024 * 1024)


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
SFT_INLINE sftvvec sft_vv_transform(sftvvec v, sftvmat vm);
SFT_INLINE sftvvec sft_vv_transform_dir(sftvvec v, sftvmat vm);
SFT_INLINE sftvmat sft_vm_broadcast(sftmat m);

// core functions
SFT_FUNC void sft_init(i32 width, i32 height, i32 requested_threads); // requested_threads == 0 => use physical core count
SFT_FUNC void sft_release(void);
SFT_FUNC void sft_present(void);
SFT_FUNC void sft_frame_begin(void);
SFT_FUNC void sft_frame_end(void);

SFT_FUNC void sft_set_camera(sftvec cam_pos, sftmat view, sftmat proj);

// drawing functions
SFT_FUNC void sft_draw_instance(const sft_instance* instance);

// arena functions
SFT_FUNC void sft_arena_init(sft_arena* arena, u64 reserve_size);
SFT_FUNC void* sft_arena_alloc(sft_arena* arena, u64 size);
SFT_INLINE void sft_arena_commit_if_needed(sft_arena* arena, u64 required_bytes);
SFT_FUNC void sft_arena_frame_begin(sft_arena* arena);

// cgltf functions
#ifdef SFT_USE_CGLTF
SFT_FUNC sft_model* sft_load_gltf(const char* filename, sft_arena* arena, i32 uv_channel);
#endif


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
} sft_mesh;

typedef struct sft_model {
    // flat array of immutable meshes, multiple instances point to this single model
    struct sft_mesh* meshes;
    u32 mesh_count;
} sft_model;

typedef struct sft_instance {
    const struct sft_model* model;
    sftmat transform;
    u32 ss_meshlet_offset;  // cache tracking
} sft_instance;

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
    f32 sx[3];
    f32 sy[3];
    f32 inv_w[3];
    u32 original_vid; // store identity of the parent triangle for consistent coloring, TODO remove once resolve exists
};

struct sft_clip_vert {
    f32 tx, ty, tz, w;
};

struct sft_sphere {
    sftvec center;
    f32 radius;
};

struct sft_frustum {
    sftvvec normal[6];
    vf32 d[6];
};


/*================================================================
:: PRIVATE VARIABLES
================================================================*/

static SFT_THREAD_LOCAL i32 sft_thread_id = 0;

// diagnostic trackers for up to 32 threads
static f64 dbg_geom_work[32] = {0};
static f64 dbg_geom_wait[32] = {0};
static i32 dbg_job_count[32] = {0};
static f64 dbg_rast_work[32] = {0};
static f64 dbg_rast_wait[32] = {0};
static i32 dbg_tile_count[32] = {0};
static f64 dbg_qsort_time = 0.;

static struct sft_state {
    i32 thread_count;
    struct sft_thread_bin_state* thread_bin_states;

    // four sub arenas
    sft_arena instance_arena;
    sft_arena job_arena;
    sft_arena chunk_arena;
    sft_arena scratch_arena;
    sft_arena clip_arena;

    struct {
        // pointers to virtual reservations
        sft_instance* instances;
        struct sft_geom_job* jobs;
        
        volatile i32 instance_count;
        volatile i32 job_count;
        volatile i32 total_ss_meshlets; // allocated screen space meshlets
    } cmd_queue;

    struct sft_atomic_counter next_job_ind;
    struct sft_atomic_counter next_tile_ind;
    struct sft_atomic_counter next_chunk_ind;
    
    struct sft_barrier geom_barrier;
    struct sft_barrier raster_barrier;
    struct sft_barrier prefix_barrier;
    sft_sem sem_start;
    sft_sem sem_done;

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
    const vf32 epsilon = sft_vf_set1(0.000001f);
    vf32 len = sft_vv_length(v);
    len = sft_vf_max(len, epsilon); 
    
    sftvvec r;
    r.x = sft_vf_div(v.x, len);
    r.y = sft_vf_div(v.y, len);
    r.z = sft_vf_div(v.z, len);
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

#if defined(_WIN32)
    // windows.h is already included in memory wrappers
    SFT_INLINE f64 sft__get_ms(void) {
        LARGE_INTEGER freq, counter;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);
        return (f64)counter.QuadPart / (f64)freq.QuadPart * 1000.0;
    }
#else
    #include <time.h>
    SFT_INLINE f64 sft__get_ms(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
    }
#endif

SFT_INLINE i32 sft__tzcnt(i32 mask) {
    #if defined(_MSC_VER)
        unsigned long r;
        _BitScanForward(&r, mask);
        return (i32)r;
    #else
        return __builtin_ctz(mask);
    #endif
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

SFT_INLINE void sft__barrier_wait(struct sft_barrier* b) {
    const i32 local_gen = b->generation;
    const i32 arrived = sft_atomic_add(&b->count, 1) + 1;
    
    if (arrived == b->thread_count) {
        // last thread to arrive resets the count and flips the generation
        b->count = 0;
        sft_atomic_add(&b->generation, 1);
    }
    else {
        // spin and yield CPU resources until the generation flips
        i32 backoff = 1;
        while (local_gen == b->generation) {
            for (i32 i = 0; i < backoff; i += 1) {
                sft_cpu_pause();
            }
            // exponential backoff
            if (backoff < 64) {
                backoff *= 2; 
            }
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

SFT_INLINE struct sft_clip_vert sft__lerp_clip_vert(struct sft_clip_vert a, struct sft_clip_vert b, f32 t) {
    return (struct sft_clip_vert){
        .tx = SFT_LERPF(a.tx, b.tx, t),
        .ty = SFT_LERPF(a.ty, b.ty, t),
        .tz = SFT_LERPF(a.tz, b.tz, t),
        .w  = SFT_LERPF(a.w,  b.w,  t)
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
    u32 original_vid, 
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
    
    out_tri->original_vid = original_vid;
    return slot;
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

SFT_INLINE void sft__push_new_chunk(struct sft_thread_bin_state* bin, i32 tile_ind, i32* t_chunk_base, i32* t_chunk_count) {
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
SFT_INLINE void sft__geom_bin_tiles(
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
SFT_INLINE void sft__geom_clip_and_bin(
    i32 active_intersecting,
    vi32 ind_v0, vi32 ind_v1, vi32 ind_v2,
    const f32* clip_tx, const f32* clip_ty, const f32* clip_tz, const f32* clip_w,
    u32 inst_ind, u32 m_ind, u32 batch,
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

        const struct sft_clip_vert in_tri[3] = {
            {clip_tx[i0], clip_ty[i0], clip_tz[i0], clip_w[i0]},
            {clip_tx[i1], clip_ty[i1], clip_tz[i1], clip_w[i1]},
            {clip_tx[i2], clip_ty[i2], clip_tz[i2], clip_w[i2]}
        };

        struct sft_clip_vert out_tris[2][3];
        const i32 num_clipped = sft__clip_triangle_near_plane(out_tris, in_tri);

        const u32 original_vid = (inst_ind << (SFT_TRIANGLE_BITS + SFT_MESHLET_BITS)) | 
                                 (m_ind << SFT_TRIANGLE_BITS) | 
                                 ((batch * SFT_SIMD_WIDTH) + lane);

        for (i32 c = 0; c < num_clipped; c += 1) {
            const i32 slot = sft__project_and_store_clipped_triangle(out_tris[c], original_vid, t_clip_base, t_clip_count);
            const struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[slot];

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
SFT_INLINE void sft__geom_setup_and_bin_batch(
    u32 batch, u32 inst_ind, u32 m_ind, const struct sft_meshlet_inds* m_inds,
    const f32* clip_tx, const f32* clip_ty, const f32* clip_tz, const f32* clip_w,
    const f32* scratch_sx, const f32* scratch_sy,
    struct sft_thread_bin_state* local_bins,
    i32* t_chunk_base, i32* t_chunk_count, i32* t_clip_base, i32* t_clip_count,
    f64* out_setup_time, f64* out_bin_time
) {
    f64 t_start = 0;
    if (0 == sft_thread_id) {
        t_start = sft__get_ms();
    }

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
            clip_tx, clip_ty, clip_tz, clip_w, inst_ind, m_ind, batch,
            local_bins, t_chunk_base, t_chunk_count, t_clip_base, t_clip_count);
    }

    if (0 == sft_vf_movemask(all_inside)) {
        if (0 == sft_thread_id) {
            *out_setup_time += (sft__get_ms() - t_start);
        }
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
        if (0 == sft_thread_id) {
            *out_setup_time += (sft__get_ms() - t_start);
        }
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
        if (0 == sft_thread_id) {
            *out_setup_time += (sft__get_ms() - t_start);
        }
        return;
    }

    if (0 == sft_thread_id) {
        const f64 t_now = sft__get_ms();
        *out_setup_time += (t_now - t_start);
        t_start = t_now;
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

    if (0 == sft_thread_id) {
        *out_bin_time += (sft__get_ms() - t_start);
    }
}

// geometry binning pipeline
static void sft__worker_phase_geom(struct sft_thread_bin_state* local_bins) {
    static f64 time_cull = 0.0;
    static f64 time_transform = 0.0;
    static f64 time_setup = 0.0;
    static f64 time_bin = 0.0;
    static i32 frames = 0;

    i32 t_chunk_base = 0, t_chunk_count = 0;
    i32 t_clip_base = 0, t_clip_count = 0;

    f64 l_cull = 0.0, l_trans = 0.0, l_setup = 0.0, l_bin = 0.0;

    alignas(32) f32 clip_tx[64];
    alignas(32) f32 clip_ty[64];
    alignas(32) f32 clip_tz[64];
    alignas(32) f32 clip_w[64];
    alignas(32) f32 scratch_sx[64];
    alignas(32) f32 scratch_sy[64];

    while (1) {
        const i32 job_ind = sft_atomic_add(&sft_state->next_job_ind.count, 1);
        if (job_ind >= sft_state->cmd_queue.job_count) {
            break;
        }

        dbg_job_count[sft_thread_id] += 1;

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

            f64 t_start = 0;
            if (0 == sft_thread_id) {
                t_start = sft__get_ms();
            }

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

            if (0 == sft_thread_id) {
                l_cull += (sft__get_ms() - t_start);
            }
            
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

                const u32 surviving_m_ind = m_ind + lane;
                const struct sft_meshlet* m_verts = &mesh->meshlets[surviving_m_ind];
                const struct sft_meshlet_inds* m_inds = &mesh->inds[surviving_m_ind];
                const u32 ss_m_ind = inst->ss_meshlet_offset + surviving_m_ind;

                if (0 == sft_thread_id) {
                    t_start = sft__get_ms();
                }

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

                    const vf32 approx_w = sft_vf_rcp(w);
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

                if (0 == sft_thread_id) {
                    l_trans += (sft__get_ms() - t_start);
                }

                const u32 batch_count = 128 / SFT_SIMD_WIDTH;
                for (u32 batch = 0; batch < batch_count; batch += 1) {
                    sft__geom_setup_and_bin_batch(
                        batch, job->instance_ind, surviving_m_ind, m_inds,
                        clip_tx, clip_ty, clip_tz, clip_w,
                        scratch_sx, scratch_sy,
                        local_bins, &t_chunk_base, &t_chunk_count, &t_clip_base, &t_clip_count,
                        &l_setup, &l_bin);
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

    if (0 == sft_thread_id) {
        time_cull += l_cull;
        time_transform += l_trans;
        time_setup += l_setup;
        time_bin += l_bin;
        
        frames += 1;
        if (frames >= 200) {
            SFT_PRINT("--- Geometry Sub-Phases (Thread 0 Avg) ---\n");
            SFT_PRINT("  Meshlet Culling: %6.3f ms\n", time_cull / 200.0);
            SFT_PRINT("  Vertex Trans:    %6.3f ms\n", time_transform / 200.0);
            SFT_PRINT("  Triangle Setup:  %6.3f ms\n", time_setup / 200.0);
            SFT_PRINT("  SIMD Binning:    %6.3f ms\n", time_bin / 200.0);
            time_cull = time_transform = time_setup = time_bin = 0.0;
            frames = 0;
        }
    }
}

// rasterizes a single clipped triangle
SFT_INLINE void sft__rast_clipped_triangle(
    u64 payload, f32* depth_buf, u32* vid_buf,
    i32 tile_min_x, i32 tile_min_y, i32 tile_max_x, i32 tile_max_y,
    f64* out_setup_time, f64* out_rast_time
) {
    f64 t_start = 0;
    if (0 == sft_thread_id) {
        t_start = sft__get_ms();
    }

    const u32 slot = (u32)(payload & 0xFFFFFFFF);
    const struct sft_clipped_triangle* ct = &sft_state->clip_cache.pool[slot];

    const f32 sx0 = ct->sx[0], sy0 = ct->sy[0], inv_w0 = ct->inv_w[0];
    const f32 sx1 = ct->sx[1], sy1 = ct->sy[1], inv_w1 = ct->inv_w[1];
    const f32 sx2 = ct->sx[2], sy2 = ct->sy[2], inv_w2 = ct->inv_w[2];

    // localize vertices to the bounding box min to prevent C0 precision loss
    const f32 fmin_x = floorf(fminf(sx0, fminf(sx1, sx2)));
    const f32 fmin_y = floorf(fminf(sy0, fminf(sy1, sy2)));

    const f32 lx0 = sx0 - fmin_x, ly0 = sy0 - fmin_y;
    const f32 lx1 = sx1 - fmin_x, ly1 = sy1 - fmin_y;
    const f32 lx2 = sx2 - fmin_x, ly2 = sy2 - fmin_y;

    // evaluate edge equations using localized coordinates
    const f32 A0 = ly0 - ly1, B0 = lx1 - lx0, C0 = lx0 * ly1 - ly0 * lx1;
    const f32 A1 = ly1 - ly2, B1 = lx2 - lx1, C1 = lx1 * ly2 - ly1 * lx2;
    const f32 A2 = ly2 - ly0, B2 = lx0 - lx2, C2 = lx2 * ly0 - ly2 * lx0;

    const f32 dx1 = lx1 - lx0, dx2 = lx2 - lx0;
    const f32 dy1 = ly1 - ly0, dy2 = ly2 - ly0;

    // strict valid area check
    const f32 det = dx1 * dy2 - dx2 * dy1;
    if (det <= SFT_EPSILON) {
        if (0 == sft_thread_id) {
            *out_setup_time += (sft__get_ms() - t_start);
        }
        return;
    }
    const f32 inv_det = 1.f / det;

    const f32 dzdx = ((inv_w1 - inv_w0) * dy2 - (inv_w2 - inv_w0) * dy1) * inv_det;
    const f32 dzdy = ((inv_w2 - inv_w0) * dx1 - (inv_w1 - inv_w0) * dx2) * inv_det;
    
    const f32 z_base = inv_w0 - (lx0 * dzdx + ly0 * dzdy);

    // set up exact pixel bounds
    i32 min_x = (i32)fmin_x;
    i32 max_x = (i32)ceilf(fmaxf(sx0, fmaxf(sx1, sx2)));
    i32 min_y = (i32)fmin_y;
    i32 max_y = (i32)ceilf(fmaxf(sy0, fmaxf(sy1, sy2)));
    min_x = SFT_MAX(min_x, tile_min_x);
    max_x = SFT_MIN(max_x, tile_max_x - 1);
    min_y = SFT_MAX(min_y, tile_min_y);
    max_y = SFT_MIN(max_y, tile_max_y - 1);

    const u32 out_vid = ct->original_vid;

    // evaluate edge equations relative to the bounds
    f32 row_e0 = ((f32)min_x + 0.5f - sx0) * A0 + ((f32)min_y + 0.5f - sy0) * B0;
    f32 row_e1 = ((f32)min_x + 0.5f - sx1) * A1 + ((f32)min_y + 0.5f - sy1) * B1;
    f32 row_e2 = ((f32)min_x + 0.5f - sx2) * A2 + ((f32)min_y + 0.5f - sy2) * B2;
    f32 row_z  = inv_w0 + dzdx * ((f32)min_x + 0.5f - sx0) + dzdy * ((f32)min_y + 0.5f - sy0);

    if (0 == sft_thread_id) {
        f64 t_now = sft__get_ms();
        *out_setup_time += (t_now - t_start);
        t_start = t_now;
    }

    // evaluate pixel centers relative to the localized origin
    for (i32 y = min_y; y <= max_y; y += 1) {
        f32 e0 = row_e0;
        f32 e1 = row_e1;
        f32 e2 = row_e2;
        f32 z  = row_z;

        i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (min_x - tile_min_x);

        for (i32 x = min_x; x <= max_x; x += 1, e0 += A0, e1 += A1, e2 += A2, z += dzdx, p_ind += 1) {
            if (e0 >= 0.f && e1 >= 0.f && e2 >= 0.f && z > depth_buf[p_ind]) {
                depth_buf[p_ind] = z;
                vid_buf[p_ind] = out_vid;
            }
        }
        
        row_e0 += B0;
        row_e1 += B1;
        row_e2 += B2;
        row_z  += dzdy;
    }

    if (0 == sft_thread_id) {
        *out_rast_time += (sft__get_ms() - t_start);
    }
}

// rasterizes a SIMD batch
SFT_INLINE void sft__rast_simd_batch(
    u64 payload, f32* depth_buf, u32* vid_buf,
    i32 tile_min_x, i32 tile_min_y, i32 tile_max_x, i32 tile_max_y,
    vf32 v_x_step, vf32 v_zero, vf32 v_tile_min_x, vf32 v_tile_max_x, vf32 v_tile_min_y, vf32 v_tile_max_y,
    f64* out_setup_time, f64* out_rast_time
) {
    f64 t_start = 0;
    if (0 == sft_thread_id) {
        t_start = sft__get_ms();
    }

    const u32 inst_ind = (u32)(payload & 0xFFFFFF);
    const u32 m_ind    = (u32)((payload >> 24) & 0xFFFFFF);
    const u32 batch    = (u32)((payload >> 48) & 0xFF);
    u32 active_tris    = (u32)((payload >> 56) & 0xFF);

    const sft_instance* inst = &sft_state->cmd_queue.instances[inst_ind];
    const struct sft_meshlet_inds* m_inds = &inst->model->meshes[0].inds[m_ind];
    const u32 offset = batch * (SFT_SIMD_WIDTH * 4);
    
    const vi32 v_i0 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (0 * SFT_SIMD_WIDTH)]);
    const vi32 v_i1 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (1 * SFT_SIMD_WIDTH)]);
    const vi32 v_i2 = sft_vi_loadu_8u_to_32i(&m_inds->inds[offset + (2 * SFT_SIMD_WIDTH)]);

    const u32 ss_m_ind = inst->ss_meshlet_offset + m_ind;
    
    const vf32 sx0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i0);
    const vf32 sy0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i0);
    const vf32 inv_w0 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i0);

    const vf32 sx1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i1);
    const vf32 sy1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i1);
    const vf32 inv_w1 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i1);

    const vf32 sx2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sx, v_i2);
    const vf32 sy2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].sy, v_i2);
    const vf32 inv_w2 = sft_vf_gather(sft_state->ss_cache[ss_m_ind].inv_w, v_i2);

    const vf32 A0 = sft_vf_sub(sy0, sy1), B0 = sft_vf_sub(sx1, sx0), C0 = sft_vf_sub(sft_vf_mul(sx0, sy1), sft_vf_mul(sy0, sx1));
    const vf32 A1 = sft_vf_sub(sy1, sy2), B1 = sft_vf_sub(sx2, sx1), C1 = sft_vf_sub(sft_vf_mul(sx1, sy2), sft_vf_mul(sy1, sx2));
    const vf32 A2 = sft_vf_sub(sy2, sy0), B2 = sft_vf_sub(sx0, sx2), C2 = sft_vf_sub(sft_vf_mul(sx2, sy0), sft_vf_mul(sy2, sx0));

    const vf32 dx1 = sft_vf_sub(sx1, sx0), dx2 = sft_vf_sub(sx2, sx0);
    const vf32 dy1 = sft_vf_sub(sy1, sy0), dy2 = sft_vf_sub(sy2, sy0);
    const vf32 inv_det = sft_vf_rcp(sft_vf_sub(sft_vf_mul(dx1, dy2), sft_vf_mul(dx2, dy1)));

    const vf32 dzdx = sft_vf_mul(sft_vf_sub(sft_vf_mul(sft_vf_sub(inv_w1, inv_w0), dy2), sft_vf_mul(sft_vf_sub(inv_w2, inv_w0), dy1)), inv_det);
    const vf32 dzdy = sft_vf_mul(sft_vf_sub(sft_vf_mul(sft_vf_sub(inv_w2, inv_w0), dx1), sft_vf_mul(sft_vf_sub(inv_w1, inv_w0), dx2)), inv_det);

    const vf32 half = sft_vf_set1(0.5f);
    const vf32 c0_abs = sft_vf_fmadd(B0, half, sft_vf_fmadd(A0, half, C0));
    const vf32 c1_abs = sft_vf_fmadd(B1, half, sft_vf_fmadd(A1, half, C1));
    const vf32 c2_abs = sft_vf_fmadd(B2, half, sft_vf_fmadd(A2, half, C2));
    vf32 z_base_abs = sft_vf_fmadd(sft_vf_sub(half, sy0), dzdy, sft_vf_fmadd(sft_vf_sub(half, sx0), dzdx, inv_w0));

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

    if (0 == sft_thread_id) {
        f64 t_now = sft__get_ms();
        *out_setup_time += (t_now - t_start);
        t_start = t_now;
    }

    while (active_tris) {
        const u32 lane = sft__tzcnt(active_tris);
        active_tris &= (active_tris - 1);
        
        const u32 vid_payload = (inst_ind << (SFT_TRIANGLE_BITS + SFT_MESHLET_BITS)) | 
                                (m_ind << SFT_TRIANGLE_BITS) | 
                                ((batch * SFT_SIMD_WIDTH) + lane);
        const vf32 v_payload = sft_vf_cast_vi32(sft_vi_set1(vid_payload));

        const i32 min_x = s_min_x[lane];
        const i32 max_x = s_max_x[lane];
        const i32 min_y = s_min_y[lane];
        const i32 max_y = s_max_y[lane];

        const f32 lane_A0 = s_A0[lane], lane_B0 = s_B0[lane], lane_c0 = s_c0_abs[lane];
        const f32 lane_A1 = s_A1[lane], lane_B1 = s_B1[lane], lane_c1 = s_c1_abs[lane];
        const f32 lane_A2 = s_A2[lane], lane_B2 = s_B2[lane], lane_c2 = s_c2_abs[lane];
        const f32 lane_dzdx = s_dzdx[lane], lane_dzdy = s_dzdy[lane], lane_z_base = s_z_base_abs[lane];

        const i32 aligned_min_x = min_x & ~(SFT_SIMD_WIDTH - 1);
        const i32 aligned_max_x = (max_x & ~(SFT_SIMD_WIDTH - 1)) + SFT_SIMD_WIDTH;

        const vf32 v_A0_step = sft_vf_mul(sft_vf_set1(lane_A0), v_x_step);
        const vf32 v_A1_step = sft_vf_mul(sft_vf_set1(lane_A1), v_x_step);
        const vf32 v_A2_step = sft_vf_mul(sft_vf_set1(lane_A2), v_x_step);
        const vf32 v_dzdx_step = sft_vf_mul(sft_vf_set1(lane_dzdx), v_x_step);
        
        const vf32 v_B0 = sft_vf_set1(lane_B0);
        const vf32 v_B1 = sft_vf_set1(lane_B1);
        const vf32 v_B2 = sft_vf_set1(lane_B2);
        const vf32 v_dzdy = sft_vf_set1(lane_dzdy);

        if (aligned_max_x - aligned_min_x == SFT_SIMD_WIDTH) {
            const f32 base_e0 = lane_c0 + lane_A0 * (f32)aligned_min_x + lane_B0 * (f32)min_y;
            const f32 base_e1 = lane_c1 + lane_A1 * (f32)aligned_min_x + lane_B1 * (f32)min_y;
            const f32 base_e2 = lane_c2 + lane_A2 * (f32)aligned_min_x + lane_B2 * (f32)min_y;
            
            vf32 v_e0 = sft_vf_add(sft_vf_set1(base_e0), v_A0_step);
            vf32 v_e1 = sft_vf_add(sft_vf_set1(base_e1), v_A1_step);
            vf32 v_e2 = sft_vf_add(sft_vf_set1(base_e2), v_A2_step);
            vf32 v_z  = sft_vf_add(sft_vf_set1(lane_z_base + lane_dzdx * (f32)aligned_min_x + lane_dzdy * (f32)min_y), v_dzdx_step);

            for (i32 y = min_y; y <= max_y; y += 1) {
                const i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (aligned_min_x - tile_min_x);
                const vf32 geom_mask = sft_vf_and(sft_vf_cmp_ge(v_e0, v_zero), 
                                 sft_vf_and(sft_vf_cmp_ge(v_e1, v_zero), sft_vf_cmp_ge(v_e2, v_zero)));
                
                if (sft_vf_movemask(geom_mask)) {
                    const vf32 old_z = sft_vf_load(&depth_buf[p_ind]);
                    const vf32 final_mask = sft_vf_and(geom_mask, sft_vf_cmp_gt(v_z, old_z));
                    
                    if (sft_vf_movemask(final_mask)) {
                        sft_vf_store(&depth_buf[p_ind], sft_vf_blendv(v_z, old_z, final_mask));
                        const vf32 old_p = sft_vf_load((f32*)&vid_buf[p_ind]);
                        sft_vf_store((f32*)&vid_buf[p_ind], sft_vf_blendv(v_payload, old_p, final_mask));
                    }
                }
                v_e0 = sft_vf_add(v_e0, v_B0);
                v_e1 = sft_vf_add(v_e1, v_B1);
                v_e2 = sft_vf_add(v_e2, v_B2);
                v_z  = sft_vf_add(v_z, v_dzdy);
            }
            continue; 
        }

        const f32 lane_max_offset = (f32)(SFT_SIMD_WIDTH - 1);

        const f32 e0_max_offset = SFT_MAX(0.f, lane_A0 * lane_max_offset) + SFT_MAX(0.f, lane_B0 * lane_max_offset);
        const f32 e1_max_offset = SFT_MAX(0.f, lane_A1 * lane_max_offset) + SFT_MAX(0.f, lane_B1 * lane_max_offset);
        const f32 e2_max_offset = SFT_MAX(0.f, lane_A2 * lane_max_offset) + SFT_MAX(0.f, lane_B2 * lane_max_offset);

        const f32 e0_min_offset = SFT_MIN(0.f, lane_A0 * lane_max_offset) + SFT_MIN(0.f, lane_B0 * lane_max_offset);
        const f32 e1_min_offset = SFT_MIN(0.f, lane_A1 * lane_max_offset) + SFT_MIN(0.f, lane_B1 * lane_max_offset);
        const f32 e2_min_offset = SFT_MIN(0.f, lane_A2 * lane_max_offset) + SFT_MIN(0.f, lane_B2 * lane_max_offset);

        for (i32 y_block = (min_y & ~(SFT_SIMD_WIDTH - 1)); y_block <= max_y; y_block += SFT_SIMD_WIDTH) {
            const i32 start_y = SFT_MAX(min_y, y_block);
            const i32 end_y = SFT_MIN(max_y, y_block + SFT_SIMD_WIDTH - 1);

            for (i32 x_block = aligned_min_x; x_block < aligned_max_x; x_block += SFT_SIMD_WIDTH) {
                const f32 base_e0 = lane_c0 + lane_A0 * (f32)x_block + lane_B0 * (f32)y_block;
                const f32 base_e1 = lane_c1 + lane_A1 * (f32)x_block + lane_B1 * (f32)y_block;
                const f32 base_e2 = lane_c2 + lane_A2 * (f32)x_block + lane_B2 * (f32)y_block;
                
                if (base_e0 + e0_max_offset < 0.f || 
                    base_e1 + e1_max_offset < 0.f || 
                    base_e2 + e2_max_offset < 0.f
                ) {
                    continue;
                }

                u8 is_full = (base_e0 + e0_min_offset >= 0.f) && 
                             (base_e1 + e1_min_offset >= 0.f) && 
                             (base_e2 + e2_min_offset >= 0.f);

                const f32 row_e0 = lane_c0 + lane_A0 * (f32)x_block + lane_B0 * (f32)start_y;
                const f32 row_e1 = lane_c1 + lane_A1 * (f32)x_block + lane_B1 * (f32)start_y;
                const f32 row_e2 = lane_c2 + lane_A2 * (f32)x_block + lane_B2 * (f32)start_y;
                const f32 row_z  = lane_z_base + lane_dzdx * (f32)x_block + lane_dzdy * (f32)start_y;

                vf32 v_e0 = sft_vf_add(sft_vf_set1(row_e0), v_A0_step);
                vf32 v_e1 = sft_vf_add(sft_vf_set1(row_e1), v_A1_step);
                vf32 v_e2 = sft_vf_add(sft_vf_set1(row_e2), v_A2_step);
                vf32 v_z  = sft_vf_add(sft_vf_set1(row_z), v_dzdx_step);

                if (is_full) {
                    for (i32 y = start_y; y <= end_y; y += 1) {
                        const i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (x_block - tile_min_x);
                        
                        const vf32 old_z = sft_vf_load(&depth_buf[p_ind]);
                        const vf32 depth_mask = sft_vf_cmp_gt(v_z, old_z);
                        
                        const i32 m_int = sft_vf_movemask(depth_mask);
                        if (((1 << SFT_SIMD_WIDTH) - 1) == m_int) { 
                            sft_vf_store(&depth_buf[p_ind], v_z);
                            sft_vf_store((f32*)&vid_buf[p_ind], v_payload);
                        }
                        else if (m_int) {
                            sft_vf_store(&depth_buf[p_ind], sft_vf_blendv(v_z, old_z, depth_mask));
                            const vf32 old_p = sft_vf_load((f32*)&vid_buf[p_ind]);
                            sft_vf_store((f32*)&vid_buf[p_ind], sft_vf_blendv(v_payload, old_p, depth_mask));
                        }
                        v_z = sft_vf_add(v_z, v_dzdy);
                    }
                }
                else {
                    for (i32 y = start_y; y <= end_y; y += 1) {
                        const vf32 geom_mask = sft_vf_and(sft_vf_cmp_ge(v_e0, v_zero), 
                                         sft_vf_and(sft_vf_cmp_ge(v_e1, v_zero), sft_vf_cmp_ge(v_e2, v_zero)));
                        
                        if (sft_vf_movemask(geom_mask)) {
                            const i32 p_ind = (y - tile_min_y) * SFT_TILE_SIZE + (x_block - tile_min_x);
                            
                            const vf32 old_z = sft_vf_load(&depth_buf[p_ind]);
                            const vf32 final_mask = sft_vf_and(geom_mask, sft_vf_cmp_gt(v_z, old_z));
                            
                            if (sft_vf_movemask(final_mask)) {
                                sft_vf_store(&depth_buf[p_ind], sft_vf_blendv(v_z, old_z, final_mask));
                                const vf32 old_p = sft_vf_load((f32*)&vid_buf[p_ind]);
                                sft_vf_store((f32*)&vid_buf[p_ind], sft_vf_blendv(v_payload, old_p, final_mask));
                            }
                        }
                        v_e0 = sft_vf_add(v_e0, v_B0);
                        v_e1 = sft_vf_add(v_e1, v_B1);
                        v_e2 = sft_vf_add(v_e2, v_B2);
                        v_z  = sft_vf_add(v_z, v_dzdy);
                    }
                }
            }
        }
    }
    if (0 == sft_thread_id) {
        *out_rast_time += (sft__get_ms() - t_start);
    }
}

// outputs the tile buffer to the main screen, temporarily since the resolve phase doesn't exist
SFT_INLINE void sft__rast_output_tile(
    const u32* vid_buf, i32 tile_min_x, i32 tile_min_y
) {
    const i32 screen_w = (i32)(sft_state->display.half_width * 2.f);
    const i32 screen_h = (i32)(sft_state->display.half_height * 2.f);

    for (i32 y = 0; y < SFT_TILE_SIZE; y += 1) {
        const i32 global_y = tile_min_y + y;
        if (global_y >= screen_h) {
            break;
        }

        for (i32 x = 0; x < SFT_TILE_SIZE; x += 1) {
            const i32 global_x = tile_min_x + x;
            if (global_x >= screen_w) {
                break;
            }
            
            const u32 payload = vid_buf[y * SFT_TILE_SIZE + x];
            if (0xFFFFFFFF == payload) {
                continue;
            }

            const u32 color = ((payload + 1) * 0x2E3A1941) | 0xFF000000;
            sft_state->display.color_buffer[global_y * screen_w + global_x] = color;
        }
    }
}

// main rasterization pipeline
static void sft__worker_phase_rasterize(void) {
    static f64 time_clear = 0.0;
    static f64 time_setup = 0.0;
    static f64 time_raster = 0.0;
    static f64 time_output = 0.0;
    static i32 frames = 0;

    f64 l_clear = 0.0, l_setup = 0.0, l_raster = 0.0, l_output = 0.0;

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

        dbg_tile_count[sft_thread_id] += 1;

        const i32 tile_ind = sft_state->display.sorted_tiles[queue_ind];
        
        const i32 tile_x = tile_ind % cols;
        const i32 tile_y = tile_ind / cols;

        const i32 tile_min_x = tile_x * SFT_TILE_SIZE;
        const i32 tile_min_y = tile_y * SFT_TILE_SIZE;
        const i32 tile_max_x = tile_min_x + SFT_TILE_SIZE;
        const i32 tile_max_y = tile_min_y + SFT_TILE_SIZE;

        f64 t_start = 0;
        if (0 == sft_thread_id) {
            t_start = sft__get_ms();
        }

        memset(depth_buf, 0, SFT_TILE_SIZE * SFT_TILE_SIZE * sizeof(f32));
        memset(vid_buf, 0xFF, SFT_TILE_SIZE * SFT_TILE_SIZE * sizeof(u32));

        if (0 == sft_thread_id) {
            l_clear += (sft__get_ms() - t_start);
        }

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

                const u32 valid_tris_mask = (u32)((payload >> 56) & 0xFF);

                if (0 == valid_tris_mask) {
                    sft__rast_clipped_triangle(
                        payload, depth_buf, vid_buf,
                        tile_min_x, tile_min_y, tile_max_x, tile_max_y,
                        &l_setup, &l_raster);
                    continue;
                }

                sft__rast_simd_batch(
                    payload, depth_buf, vid_buf,
                    tile_min_x, tile_min_y, tile_max_x, tile_max_y,
                    v_x_step, v_zero, v_tile_min_x, v_tile_max_x, v_tile_min_y, v_tile_max_y,
                    &l_setup, &l_raster);
            }
            current_chunk_ind = chunk->next_ind;
        }

        if (0 == sft_thread_id) {
            t_start = sft__get_ms();
        }

        sft__rast_output_tile(vid_buf, tile_min_x, tile_min_y);
        
        if (0 == sft_thread_id) {
            l_output += (sft__get_ms() - t_start);
        }
    }
    
    if (0 == sft_thread_id) {
        time_clear += l_clear;
        time_setup += l_setup;
        time_raster += l_raster;
        time_output += l_output;

        frames += 1;
        if (frames >= 200) {
            SFT_PRINT("--- Rasterize Sub-Phases (Thread 0 Avg) ---\n");
            SFT_PRINT("  Buffer Clear:   %6.3f ms\n", time_clear / 200.0);
            SFT_PRINT("  Triangle Setup: %6.3f ms\n", time_setup / 200.0);
            SFT_PRINT("  Rasterization:  %6.3f ms\n", time_raster / 200.0);
            SFT_PRINT("  Tile Output:    %6.3f ms\n", time_output / 200.0);
            time_clear = time_setup = time_raster = time_output = 0.0;
            frames = 0;
        }
    }
}

static int sft__cmp_tile_loads(const void* a, const void* b) {
    const i32 ta = *(const i32*)a;
    const i32 tb = *(const i32*)b;
    // sort descending (heaviest first)
    return sft_state->display.tile_payload_counts[tb] - sft_state->display.tile_payload_counts[ta];
}

static void sft__worker_thread(void* user_data) {
    static f64 accum_geom = 0.0;
    static f64 accum_raster = 0.0;
    static i32 frame_counter = 0;
    
    while (1) {
        sft_sem_wait(&sft_state->sem_start);

        const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;
        
        // grab this thread's private slice of the pre allocated array
        struct sft_thread_bin_state* local_bins = &sft_state->thread_bin_states[sft_thread_id * total_tiles];

        f64 t_start = 0.0, t_geom = 0.0, t_raster = 0.0;
        if (0 == sft_thread_id) {
            t_start = sft__get_ms();
        }

        // reset all local bins for the new frame
        for (i32 i = 0; i < total_tiles; i += 1) {
            local_bins[i].chunk_ind = -1;
            local_bins[i].write_head = NULL;
            local_bins[i].write_end = NULL;
            local_bins[i].payload_count = 0;
        }

        // ---  geometry and binning pass  ---
        f64 work_start = sft__get_ms();
        sft__worker_phase_geom(local_bins);
        dbg_geom_work[sft_thread_id] += (sft__get_ms() - work_start);
        
        f64 wait_start = sft__get_ms();
        sft__barrier_wait(&sft_state->geom_barrier);
        dbg_geom_wait[sft_thread_id] += (sft__get_ms() - wait_start);

        if (0 == sft_thread_id) {
            t_geom = sft__get_ms();
            
            const f64 qsort_start = sft__get_ms();
            const i32 total_tiles = sft_state->display.tile_cols * sft_state->display.tile_rows;
            for (i32 i = 0; i < total_tiles; i += 1) {
                sft_state->display.sorted_tiles[i] = i;
            }
            qsort(sft_state->display.sorted_tiles, total_tiles, sizeof(i32), sft__cmp_tile_loads);
            dbg_qsort_time += (sft__get_ms() - qsort_start);
        }

        sft__barrier_wait(&sft_state->prefix_barrier);

        // ---  rasterization pass  ---
        work_start = sft__get_ms();
        sft__worker_phase_rasterize();
        dbg_rast_work[sft_thread_id] += (sft__get_ms() - work_start);
        
        wait_start = sft__get_ms();
        sft__barrier_wait(&sft_state->raster_barrier); 
        dbg_rast_wait[sft_thread_id] += (sft__get_ms() - wait_start);
        
        // final synchronize and cleanup
        sft__barrier_wait(&sft_state->raster_barrier); 
        
        if (0 == sft_thread_id) {
            frame_counter += 1;
            if (frame_counter >= 200) {
                f64 max_g_work = 0, min_g_work = 99999;
                f64 max_g_wait = 0, min_g_wait = 99999;
                i32 max_jobs = 0, min_jobs = 99999;
                
                f64 max_r_work = 0, min_r_work = 99999;
                f64 max_r_wait = 0, min_r_wait = 99999;
                i32 max_tiles = 0, min_tiles = 99999;

                for(i32 i = 0; i < sft_state->thread_count; i += 1) {
                    f64 gw = dbg_geom_work[i] / 200.0;
                    f64 g_wait = dbg_geom_wait[i] / 200.0;
                    i32 jc = dbg_job_count[i] / 200;
                    
                    if (gw > max_g_work) max_g_work = gw;
                    if (gw < min_g_work) min_g_work = gw;
                    if (g_wait > max_g_wait) max_g_wait = g_wait;
                    if (g_wait < min_g_wait) min_g_wait = g_wait;
                    if (jc > max_jobs) max_jobs = jc;
                    if (jc < min_jobs) min_jobs = jc;

                    f64 rw = dbg_rast_work[i] / 200.0;
                    f64 r_wait = dbg_rast_wait[i] / 200.0;
                    i32 tc = dbg_tile_count[i] / 200;
                    
                    if (rw > max_r_work) max_r_work = rw;
                    if (rw < min_r_work) min_r_work = rw;
                    if (r_wait > max_r_wait) max_r_wait = r_wait;
                    if (r_wait < min_r_wait) min_r_wait = r_wait;
                    if (tc > max_tiles) max_tiles = tc;
                    if (tc < min_tiles) min_tiles = tc;
                }

                SFT_PRINT("--- THREAD LOAD DISTRIBUTION (Avg over 200 frames) ---\n");
                SFT_PRINT("GEOM   | Work: %5.2fms to %5.2fms | Wait: %5.2fms to %5.2fms | Jobs: %d to %d\n", min_g_work, max_g_work, min_g_wait, max_g_wait, min_jobs, max_jobs);
                SFT_PRINT("SORT   | QSort overhead: %5.3fms\n", dbg_qsort_time / 200.0);
                SFT_PRINT("RAST   | Work: %5.2fms to %5.2fms | Wait: %5.2fms to %5.2fms | Tiles: %d to %d\n\n", min_r_work, max_r_work, min_r_wait, max_r_wait, min_tiles, max_tiles);
                
                memset(dbg_geom_work, 0, sizeof(dbg_geom_work));
                memset(dbg_geom_wait, 0, sizeof(dbg_geom_wait));
                memset(dbg_job_count, 0, sizeof(dbg_job_count));
                memset(dbg_rast_work, 0, sizeof(dbg_rast_work));
                memset(dbg_rast_wait, 0, sizeof(dbg_rast_wait));
                memset(dbg_tile_count, 0, sizeof(dbg_tile_count));
                dbg_qsort_time = 0.0;
                frame_counter = 0;
            }
        }

        // TODO resolve phase

        sft_sem_post(&sft_state->sem_done, 1);
    }
}


/*================================================================
:: FUNCTION DEFINITIONS (CGLTF)
================================================================*/

// Requires #define SFT_USE_CGLTF and cgltf.h to be included
#ifdef SFT_USE_CGLTF

#ifndef SFT_CGLTF_PATH
    #define SFT_CGLTF_PATH "cgltf.h"
#endif

#define CGLTF_IMPLEMENTATION
#include SFT_CGLTF_PATH

SFT_FUNC void* sft__cgltf_alloc(void* user, cgltf_size size) {
    (void)user;
    return malloc((size_t)size);
}
SFT_FUNC void sft__cgltf_free(void* user, void* ptr) {
    (void)user;
    free(ptr);
}

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

SFT_FUNC sft_model* sft_load_gltf(const char* filename, sft_arena* arena, i32 uv_channel) {
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

    // ---  calculate required sizes to merge everything into a single mesh  ---
    //      TODO update this later when materials exist
    u64 total_verts = 0;
    u64 total_tris = 0;

    for (cgltf_size i = 0; i < data->nodes_count; i += 1) {
        cgltf_node* node = &data->nodes[i];
        if (!node->mesh) {
            continue;
        }

        for (cgltf_size p = 0; p < node->mesh->primitives_count; p += 1) {
            cgltf_primitive* prim = &node->mesh->primitives[p];
            if (cgltf_primitive_type_triangles != prim->type) {
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

    if (0 == total_verts || 0 == total_tris) {
        cgltf_free(data);
        free(file_content);
        return NULL;
    }

    // unified geometry buffers
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

    // ---  extract, transform, and merge primitives  ---
    for (cgltf_size i = 0; i < data->nodes_count; i += 1) {
        cgltf_node* node = &data->nodes[i];
        if (!node->mesh) {
            continue;
        }

        cgltf_float world_mat[16];
        cgltf_node_transform_world(node, world_mat);

        for (cgltf_size p = 0; p < node->mesh->primitives_count; p += 1) {
            cgltf_primitive* prim = &node->mesh->primitives[p];
            if (cgltf_primitive_type_triangles != prim->type) {
                continue;
            }

            cgltf_accessor *pos_a = NULL, *norm_a = NULL, *uv_a = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a += 1) {
                switch (prim->attributes[a].type) {
                    case cgltf_attribute_type_position: pos_a = prim->attributes[a].data; break;
                    case cgltf_attribute_type_normal:   norm_a = prim->attributes[a].data; break;
                    case cgltf_attribute_type_texcoord: {
                        if (uv_channel == prim->attributes[a].index) {
                            uv_a = prim->attributes[a].data;
                        }
                    } break;
                }
            }
            if (!pos_a) {
                continue;
            }

            const i32 prim_v_count = (i32)pos_a->count;
            for (i32 v = 0; v < prim_v_count; v += 1) {
                f32 buf[3] = {0};
                cgltf_accessor_read_float(pos_a, v, buf, 3);
                
                // apply global node transform to position
                const f32 tx = buf[0] * world_mat[0] + buf[1] * world_mat[4] + buf[2] * world_mat[8] + world_mat[12];
                const f32 ty = buf[0] * world_mat[1] + buf[1] * world_mat[5] + buf[2] * world_mat[9] + world_mat[13];
                const f32 tz = buf[0] * world_mat[2] + buf[1] * world_mat[6] + buf[2] * world_mat[10] + world_mat[14];

                uni_vx[v_offset + v] = tx;
                uni_vy[v_offset + v] = ty;
                uni_vz[v_offset + v] = -tz; // negate Z for left handed

                // apply transform to normal and re normalize
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
                    uni_v[v_offset + v] = 1.f - buf[1];
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

    cgltf_free(data);
    free(file_content);

    // ---  build single unified mesh using adjacency and search methods  ---
    sft_model* model = (sft_model*)sft_arena_alloc(arena, sizeof(sft_model));
    model->mesh_count = 1;
    model->meshes = (sft_mesh*)sft_arena_alloc(arena, sizeof(sft_mesh));
    sft_mesh* out_mesh = &model->meshes[0];

    i32 *vertex_offsets, *adjacency;
    SFT_CALLOC(vertex_offsets, total_verts + 1, i32);
    SFT_CALLOC(adjacency, total_tris * 3, i32);

    for (i32 k = 0; k < total_tris * 3; k += 1) {
        vertex_offsets[uni_inds[k]] += 1;
    }
    
    i32 prefix = 0;
    for (i32 k = 0; k < total_verts; k += 1) {
        const i32 count = vertex_offsets[k];
        vertex_offsets[k] = prefix;
        prefix += count;
    }
    vertex_offsets[total_verts] = prefix;

    i32* write_heads;
    SFT_CALLOC(write_heads, total_verts, i32);
    memcpy(write_heads, vertex_offsets, total_verts * sizeof(i32));

    for (i32 k = 0; k < total_tris; k += 1) {
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

    while (unmapped_tris > 0) {
        sft__builder_reset(&builder);
        i32 frontier_head = 0;
        i32 frontier_tail = 0;

        i32 seed_tri = -1;
        for (i32 k = 0; k < total_tris; k += 1) {
            if (!used_tris[k]) {
                seed_tri = k;
                break;
            }
        }
        if (-1 == seed_tri) {
            break;
        }

        const i32 v0 = uni_inds[seed_tri * 3 + 0];
        const i32 v1 = uni_inds[seed_tri * 3 + 1];
        const i32 v2 = uni_inds[seed_tri * 3 + 2];
        const sftvec normal = sft_v_face_normal(SFT_V(uni_vx[v0], uni_vy[v0], uni_vz[v0]), 
                                                SFT_V(uni_vx[v1], uni_vy[v1], uni_vz[v1]), 
                                                SFT_V(uni_vx[v2], uni_vy[v2], uni_vz[v2]));

        sft__builder_add_triangle(&builder, v0, v1, v2, normal);
        used_tris[seed_tri] = 1;
        unmapped_tris -= 1;

        const i32 seed_verts[3] = {v0, v1, v2};
        for (i32 v = 0; v < 3; v += 1) {
            const i32 vert = seed_verts[v];
            for (i32 adj_ind = vertex_offsets[vert]; adj_ind < vertex_offsets[vert + 1]; adj_ind += 1) {
                const i32 adj_tri = adjacency[adj_ind];
                if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                    frontier[frontier_tail++] = adj_tri;
                    in_frontier[adj_tri] = 1;
                }
            }
        }

        while (builder.vert_count < SFT_VERTS_PER_MESHLET && builder.tri_count < 128 && frontier_head < frontier_tail) {
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
                const sftvec c_normal = sft_v_face_normal(SFT_V(uni_vx[cv0], uni_vy[cv0], uni_vz[cv0]), 
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
                break;
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
                const i32 vert = new_verts[v];
                for (i32 adj_ind = vertex_offsets[vert]; adj_ind < vertex_offsets[vert + 1]; adj_ind += 1) {
                    const i32 adj_tri = adjacency[adj_ind];
                    if (!used_tris[adj_tri] && !in_frontier[adj_tri]) {
                        frontier[frontier_tail++] = adj_tri;
                        in_frontier[adj_tri] = 1;
                    }
                }
            }
        }

        for (i32 k = frontier_head; k < frontier_tail; k += 1) {
            in_frontier[frontier[k]] = 0;
        }

        struct sft_sphere sphere;
        sft__builder_finalize_attribs(
            &builder, uni_vx, uni_vy, uni_vz, uni_nx, uni_ny, uni_nz, uni_u, uni_v,
            &out_mesh->meshlets[meshlet_count], &out_mesh->meshlet_attribs[meshlet_count],
            &out_mesh->inds[meshlet_count], &sphere);

        out_mesh->bounds_center_x[meshlet_count] = sphere.center.x;
        out_mesh->bounds_center_y[meshlet_count] = sphere.center.y;
        out_mesh->bounds_center_z[meshlet_count] = sphere.center.z;
        out_mesh->bounds_radius[meshlet_count]   = sphere.radius;

        sftvec true_axis = sft_v_norm(builder.cone_axis);
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
        
        f32 sin_alpha = (min_dot < 0.f) ? 1.f : sqrtf(1.f - min_dot * min_dot);
        out_mesh->cone_axis_x[meshlet_count] = true_axis.x;
        out_mesh->cone_axis_y[meshlet_count] = true_axis.y;
        out_mesh->cone_axis_z[meshlet_count] = true_axis.z;
        out_mesh->cone_cutoff[meshlet_count] = sin_alpha + 0.02f;

        meshlet_count += 1;
    }

    out_mesh->meshlet_count = meshlet_count;

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
        sft_arena_init(&sft_state->instance_arena, 256ULL * 1024 * 1024);
        sft_state->cmd_queue.instances = (sft_instance*)sft_state->instance_arena.base;
        
        sft_arena_init(&sft_state->job_arena, 512ULL * 1024 * 1024);
        sft_state->cmd_queue.jobs = (struct sft_geom_job*)sft_state->job_arena.base;

        sft_arena_init(&sft_state->chunk_arena, 1024ULL * 1024 * 1024);
        sft_state->display.chunk_pool = (struct sft_payload_chunk*)sft_state->chunk_arena.base;
        
        sft_arena_init(&sft_state->scratch_arena, 512ULL * 1024 * 1024);
        sft_state->ss_cache = (struct sft_ss_meshlet*)sft_state->scratch_arena.base;

        sft_arena_init(&sft_state->clip_arena, 32ULL * 1024 * 1024);
        sft_state->clip_cache.pool = (struct sft_clipped_triangle*)sft_state->clip_arena.base;
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
}

SFT_FUNC void sft_release(void) {
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
    if (sft_state->cmd_queue.total_ss_meshlets > 0) {
        sft_arena_commit_if_needed(&sft_state->scratch_arena, 
            sft_state->cmd_queue.total_ss_meshlets * sizeof(struct sft_ss_meshlet));
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
}

SFT_FUNC void sft_frame_end(void) {
    sft_state->cmd_queue.instance_count = 0;
    sft_state->cmd_queue.job_count = 0;
    sft_state->next_job_ind.count = 0;
    sft_state->next_tile_ind.count = 0;
    sft_state->cmd_queue.total_ss_meshlets = 0;
    sft_state->clip_cache.count = 0;
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
    const i32 inst_slot = sft_atomic_add(&sft_state->cmd_queue.instance_count, 1);
    
    // strict bit packing validation guard
    if (inst_slot >= SFT_MAX_INSTANCES_PER_FRAME) {
        return; 
    }

    sft_arena_commit_if_needed(&sft_state->instance_arena, (inst_slot + 1) * sizeof(sft_instance));
    sft_state->cmd_queue.instances[inst_slot] = *inst;

    sft_state->cmd_queue.instances[inst_slot].ss_meshlet_offset =
        sft_atomic_add(&sft_state->cmd_queue.total_ss_meshlets, inst->model->meshes[0].meshlet_count);

    // slice geometry into jobs
    for (u32 m = 0; m < inst->model->mesh_count; m += 1) {
        const struct sft_mesh* mesh = &inst->model->meshes[m];
        u32 meshlets_remaining = mesh->meshlet_count;
        u32 current_start = 0;

        while (meshlets_remaining > 0) {
            const u32 count = SFT_CLAMP(meshlets_remaining, 0, SFT_MESHLETS_PER_JOB);
            const i32 job_slot = sft_atomic_add(&sft_state->cmd_queue.job_count, 1);
            
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

SFT_FUNC void sft_arena_init(sft_arena* arena, u64 reserve_size) {
    arena->max_size = reserve_size;
    arena->committed_size = 0;
    arena->offset = 0;
    arena->commit_lock = 0;

    #if defined(_WIN32)
        // reserve address space
        arena->base = (u8*)VirtualAlloc(NULL, reserve_size, SFT_MEM_RESERVE, PAGE_NOACCESS);
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
            const void* res = VirtualAlloc(commit_target, chunks_to_commit, SFT_MEM_COMMIT, PAGE_READWRITE);
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
            void* res = VirtualAlloc(commit_target, chunks_to_commit, SFT_MEM_COMMIT, PAGE_READWRITE);
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
