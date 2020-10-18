/* $Id: types.h 4704 2014-01-16 05:30:46Z ming $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJ_TYPES_H__
#define __PJ_TYPES_H__


/**
 * @file types.h
 * @brief Declaration of basic types and utility.
 */
/**
 * @defgroup PJ_BASIC Basic Data Types and Library Functionality.
 * @ingroup PJ_DS
 * @{
 */
//#include <pj/config.h>
#include <stddef.h>


#ifndef PJ_IMPORT_DECL_SPECIFIER
#   define PJ_IMPORT_DECL_SPECIFIER
#endif

#	define PJ_DECL(type)	    PJ_IMPORT_DECL_SPECIFIER type

#define PJ_INLINE_SPECIFIER	inline
#define PJ_INLINE(type)	  PJ_INLINE_SPECIFIER type

#define PJ_END_DECL


#ifndef PJ_DEF
#define PJ_DEF(type)		    type
#endif

#define PJ_UNUSED_ARG(arg) (void)arg

#define HYT_HEVC 1

typedef enum CODEC_TYPE {
    H264_HARD_CODEC = 1,
    H265_HARD_CODEC = 2,
}CODEC_TYPE;

#define GET_H265_NAL_TYPE(c)  ( ((c)>>1) & 0x3F)

/**
 * Max packet size for transmitting direction.
 */
#ifndef PJMEDIA_MAX_MTU			
#  define PJMEDIA_MAX_MTU			1500
#endif

/**
 * Maximum video payload size. Note that this must not be greater than
 * PJMEDIA_MAX_MTU.
 *
 * Default: (PJMEDIA_MAX_MTU - 100)
 */
#ifndef PJMEDIA_MAX_VID_PAYLOAD_SIZE			
#  define PJMEDIA_MAX_VID_PAYLOAD_SIZE		(PJMEDIA_MAX_MTU - 200)
#endif


//PJ_BEGIN_DECL

/* ************************************************************************* */

/** Signed 32bit integer. */
typedef int		pj_int32_t;

/** Unsigned 32bit integer. */
typedef unsigned int	pj_uint32_t;

/** Signed 16bit integer. */
typedef short		pj_int16_t;

/** Unsigned 16bit integer. */
typedef unsigned short	pj_uint16_t;

/** Signed 8bit integer. */
typedef signed char	pj_int8_t;

/** Unsigned 8bit integer. */
typedef unsigned char	pj_uint8_t;

/** Large unsigned integer. */
typedef size_t		pj_size_t;

/** Large signed integer. */
#if defined(PJ_WIN64) && PJ_WIN64!=0
    typedef pj_int64_t     pj_ssize_t;
#else
    typedef long           pj_ssize_t;
#endif

/** Status code. */
typedef int		pj_status_t;

/** Boolean. */
typedef int		pj_bool_t;

/** Native char type, which will be equal to wchar_t for Unicode
 * and char for ANSI. */
#if defined(PJ_NATIVE_STRING_IS_UNICODE) && PJ_NATIVE_STRING_IS_UNICODE!=0
    typedef wchar_t pj_char_t;
#else
    typedef char pj_char_t;
#endif

/** This macro creates Unicode or ANSI literal string depending whether
 *  native platform string is Unicode or ANSI. */
#if defined(PJ_NATIVE_STRING_IS_UNICODE) && PJ_NATIVE_STRING_IS_UNICODE!=0
#   define PJ_T(literal_str)	L##literal_str
#else
#   define PJ_T(literal_str)	literal_str
#endif

/** Some constants */
enum pj_constants_
{
    /** Status is OK. */
    PJ_SUCCESS=0,

    /** True value. */
    PJ_TRUE=1,

    /** False value. */
    PJ_FALSE=0
};

/**
 * File offset type.
 */
#if defined(PJ_HAS_INT64) && PJ_HAS_INT64!=0
typedef pj_int64_t pj_off_t;
#else
typedef pj_ssize_t pj_off_t;
#endif

#define PJMEDIA_FOURCC(C1, C2, C3, C4) ( C4<<24 | C3<<16 | C2<<8 | C1 )
#define PJMEDIA_FORMAT_PACK(C1, C2, C3, C4) PJMEDIA_FOURCC(C1, C2, C3, C4)

/**
 * This enumeration uniquely identify audio sample and/or video pixel formats.
 * Some well known formats are listed here. The format ids are built by
 * combining four character codes, similar to FOURCC. The format id is
 * extensible, as application may define and use format ids not declared
 * on this enumeration.
 *
 * This format id along with other information will fully describe the media
 * in #pjmedia_format structure.
 */
typedef enum pjmedia_format_id
{
    /*
     * Audio formats
     */

    /** 16bit signed integer linear PCM audio */
    PJMEDIA_FORMAT_L16	    = 0,

    /** Alias for PJMEDIA_FORMAT_L16 */
    PJMEDIA_FORMAT_PCM	    = PJMEDIA_FORMAT_L16,

    /** G.711 ALAW */
    PJMEDIA_FORMAT_PCMA	    = PJMEDIA_FORMAT_PACK('A', 'L', 'A', 'W'),

    /** Alias for PJMEDIA_FORMAT_PCMA */
    PJMEDIA_FORMAT_ALAW	    = PJMEDIA_FORMAT_PCMA,

    /** G.711 ULAW */
    PJMEDIA_FORMAT_PCMU	    = PJMEDIA_FORMAT_PACK('u', 'L', 'A', 'W'),

    /** Aliaw for PJMEDIA_FORMAT_PCMU */
    PJMEDIA_FORMAT_ULAW	    = PJMEDIA_FORMAT_PCMU,

    /** AMR narrowband */
    PJMEDIA_FORMAT_AMR	    = PJMEDIA_FORMAT_PACK(' ', 'A', 'M', 'R'),

    /** ITU G.729 */
    PJMEDIA_FORMAT_G729	    = PJMEDIA_FORMAT_PACK('G', '7', '2', '9'),

    /** Internet Low Bit-Rate Codec (ILBC) */
    PJMEDIA_FORMAT_ILBC	    = PJMEDIA_FORMAT_PACK('I', 'L', 'B', 'C'),


    /*
     * Video formats.
     */
    /**
     * 24bit RGB
     */
    PJMEDIA_FORMAT_RGB24    = PJMEDIA_FORMAT_PACK('R', 'G', 'B', '3'),

    /**
     * 32bit RGB with alpha channel
     */
    PJMEDIA_FORMAT_RGBA     = PJMEDIA_FORMAT_PACK('R', 'G', 'B', 'A'),
    PJMEDIA_FORMAT_BGRA     = PJMEDIA_FORMAT_PACK('B', 'G', 'R', 'A'),

    /**
     * Alias for PJMEDIA_FORMAT_RGBA
     */
    PJMEDIA_FORMAT_RGB32    = PJMEDIA_FORMAT_RGBA,

    /**
     * Device Independent Bitmap, alias for 24 bit RGB
     */
    PJMEDIA_FORMAT_DIB      = PJMEDIA_FORMAT_PACK('D', 'I', 'B', ' '),

    /**
     * This is planar 4:4:4/24bpp RGB format, the data can be treated as
     * three planes of color components, where the first plane contains
     * only the G samples, the second plane contains only the B samples,
     * and the third plane contains only the R samples.
     */
    PJMEDIA_FORMAT_GBRP    = PJMEDIA_FORMAT_PACK('G', 'B', 'R', 'P'),

    /**
     * This is a packed 4:4:4/32bpp format, where each pixel is encoded as
     * four consecutive bytes, arranged in the following sequence: V0, U0,
     * Y0, A0. Source:
     * http://msdn.microsoft.com/en-us/library/dd206750%28v=VS.85%29.aspx#ayuv
     */
    PJMEDIA_FORMAT_AYUV	    = PJMEDIA_FORMAT_PACK('A', 'Y', 'U', 'V'),

    /**
     * This is packed 4:2:2/16bpp YUV format, the data can be treated as
     * an array of unsigned char values, where the first byte contains
     * the first Y sample, the second byte contains the first U (Cb) sample,
     * the third byte contains the second Y sample, and the fourth byte
     * contains the first V (Cr) sample, and so forth. Source:
     * http://msdn.microsoft.com/en-us/library/dd206750%28v=VS.85%29.aspx#yuy2
     */
    PJMEDIA_FORMAT_YUY2	    = PJMEDIA_FORMAT_PACK('Y', 'U', 'Y', '2'),

    /**
     * This format is the same as the YUY2 format except the byte order is
     * reversed -- that is, the chroma and luma bytes are flipped. If the
     * image is addressed as an array of two little-endian WORD values, the
     * first WORD contains U in the LSBs and Y0 in the MSBs, and the second
     * WORD contains V in the LSBs and Y1 in the MSBs. Source:
     * http://msdn.microsoft.com/en-us/library/dd206750%28v=VS.85%29.aspx#uyvy
     */
    PJMEDIA_FORMAT_UYVY	    = PJMEDIA_FORMAT_PACK('U', 'Y', 'V', 'Y'),

    /**
     * This format is the same as the YUY2 and UYVY format except the byte
     * order is reversed -- that is, the chroma and luma bytes are flipped.
     * If the image is addressed as an array of two little-endian WORD values,
     * the first WORD contains Y0 in the LSBs and V in the MSBs, and the second
     * WORD contains Y1 in the LSBs and U in the MSBs.
     */
    PJMEDIA_FORMAT_YVYU	    = PJMEDIA_FORMAT_PACK('Y', 'V', 'Y', 'U'),

    /**
     * This is planar 4:2:0/12bpp YUV format, the data can be treated as
     * three planes of color components, where the first plane contains
     * only the Y samples, the second plane contains only the U (Cb) samples,
     * and the third plane contains only the V (Cr) sample.
     */
    PJMEDIA_FORMAT_I420	    = PJMEDIA_FORMAT_PACK('I', '4', '2', '0'),

    /**
     * IYUV is alias for I420.
     */
    PJMEDIA_FORMAT_IYUV	    = PJMEDIA_FORMAT_I420,

    /**
     * This is planar 4:2:0/12bpp YUV format, similar to I420 or IYUV but
     * the U (Cb) and V (Cr) planes order is switched, i.e: the second plane
     * contains the V (Cb) samples and the third plane contains the V (Cr)
     * samples.
     */
    PJMEDIA_FORMAT_YV12	    = PJMEDIA_FORMAT_PACK('Y', 'V', '1', '2'),

    /**
     * This is planar 4:2:2/16bpp YUV format, the data can be treated as
     * three planes of color components, where the first plane contains
     * only the Y samples, the second plane contains only the U (Cb) samples,
     * and the third plane contains only the V (Cr) sample.
     */
    PJMEDIA_FORMAT_I422	    = PJMEDIA_FORMAT_PACK('I', '4', '2', '2'),

    /**
     * The JPEG version of planar 4:2:0/12bpp YUV format.
     */
    PJMEDIA_FORMAT_I420JPEG = PJMEDIA_FORMAT_PACK('J', '4', '2', '0'),

    /**
     * The JPEG version of planar 4:2:2/16bpp YUV format.
     */
    PJMEDIA_FORMAT_I422JPEG = PJMEDIA_FORMAT_PACK('J', '4', '2', '2'),

    /**
     * Encoded video formats
     */

    PJMEDIA_FORMAT_H261     = PJMEDIA_FORMAT_PACK('H', '2', '6', '1'),
    PJMEDIA_FORMAT_H263     = PJMEDIA_FORMAT_PACK('H', '2', '6', '3'),
    PJMEDIA_FORMAT_H263P    = PJMEDIA_FORMAT_PACK('P', '2', '6', '3'),
    PJMEDIA_FORMAT_H264     = PJMEDIA_FORMAT_PACK('H', '2', '6', '4'),
    
#ifdef HYT_HEVC
	PJMEDIA_FORMAT_H265 	= PJMEDIA_FORMAT_PACK('H', '2', '6', '5'), 
#endif 

    PJMEDIA_FORMAT_MJPEG    = PJMEDIA_FORMAT_PACK('M', 'J', 'P', 'G'),
    PJMEDIA_FORMAT_MPEG1VIDEO = PJMEDIA_FORMAT_PACK('M', 'P', '1', 'V'),
    PJMEDIA_FORMAT_MPEG2VIDEO = PJMEDIA_FORMAT_PACK('M', 'P', '2', 'V'),
    PJMEDIA_FORMAT_MPEG4    = PJMEDIA_FORMAT_PACK('M', 'P', 'G', '4'),

} pjmedia_format_id;

/* ************************************************************************* */
/*
 * Data structure types.
 */
/**
 * This type is used as replacement to legacy C string, and used throughout
 * the library. By convention, the string is NOT null terminated.
 */
struct pj_str_t
{
    /** Buffer pointer, which is by convention NOT null terminated. */
    char       *ptr;

    /** The length of the string. */
    pj_ssize_t  slen;
};

/**
 * This structure represents high resolution (64bit) time value. The time
 * values represent time in cycles, which is retrieved by calling
 * #pj_get_timestamp().
 */
typedef union pj_timestamp
{
    struct
    {
#if defined(PJ_IS_LITTLE_ENDIAN) && PJ_IS_LITTLE_ENDIAN!=0
	pj_uint32_t lo;     /**< Low 32-bit value of the 64-bit value. */
	pj_uint32_t hi;     /**< high 32-bit value of the 64-bit value. */
#else
	pj_uint32_t hi;     /**< high 32-bit value of the 64-bit value. */
	pj_uint32_t lo;     /**< Low 32-bit value of the 64-bit value. */
#endif
    } u32;                  /**< The 64-bit value as two 32-bit values. */

#if PJ_HAS_INT64
    pj_uint64_t u64;        /**< The whole 64-bit value, where available. */
#endif
} pj_timestamp;



/**
 * The opaque data type for linked list, which is used as arguments throughout
 * the linked list operations.
 */
typedef void pj_list_type;

/** 
 * List.
 */
typedef struct pj_list pj_list;

/**
 * Opaque data type for hash tables.
 */
typedef struct pj_hash_table_t pj_hash_table_t;

/**
 * Opaque data type for hash entry (only used internally by hash table).
 */
typedef struct pj_hash_entry pj_hash_entry;

/**
 * Data type for hash search iterator.
 * This structure should be opaque, however applications need to declare
 * concrete variable of this type, that's why the declaration is visible here.
 */
typedef struct pj_hash_iterator_t
{
    pj_uint32_t	     index;     /**< Internal index.     */
    pj_hash_entry   *entry;     /**< Internal entry.     */
} pj_hash_iterator_t;


/**
 * Forward declaration for memory pool factory.
 */
typedef struct pj_pool_factory pj_pool_factory;

/**
 * Opaque data type for memory pool.
 */
typedef struct pj_pool_t pj_pool_t;

/**
 * Forward declaration for caching pool, a pool factory implementation.
 */
typedef struct pj_caching_pool pj_caching_pool;

/**
 * This type is used as replacement to legacy C string, and used throughout
 * the library.
 */
typedef struct pj_str_t pj_str_t;

/**
 * Opaque data type for I/O Queue structure.
 */
typedef struct pj_ioqueue_t pj_ioqueue_t;

/**
 * Opaque data type for key that identifies a handle registered to the
 * I/O queue framework.
 */
typedef struct pj_ioqueue_key_t pj_ioqueue_key_t;

/**
 * Opaque data to identify timer heap.
 */
typedef struct pj_timer_heap_t pj_timer_heap_t;

/** 
 * Opaque data type for atomic operations.
 */
typedef struct pj_atomic_t pj_atomic_t;

/**
 * Value type of an atomic variable.
 */
//typedef PJ_ATOMIC_VALUE_TYPE pj_atomic_value_t;
 
/* ************************************************************************* */

/** Thread handle. */
typedef struct pj_thread_t pj_thread_t;

/** Lock object. */
typedef struct pj_lock_t pj_lock_t;

/** Group lock */
typedef struct pj_grp_lock_t pj_grp_lock_t;

/** Mutex handle. */
typedef struct pj_mutex_t pj_mutex_t;

/** Semaphore handle. */
typedef struct pj_sem_t pj_sem_t;

/** Event object. */
typedef struct pj_event_t pj_event_t;

/** Unidirectional stream pipe object. */
typedef struct pj_pipe_t pj_pipe_t;

/** Operating system handle. */
typedef void *pj_oshandle_t;

/** Socket handle. */
#if defined(PJ_WIN64) && PJ_WIN64!=0
    typedef pj_int64_t pj_sock_t;
#else
    typedef long pj_sock_t;
#endif

/** Generic socket address. */
typedef void pj_sockaddr_t;

/** Forward declaration. */
typedef struct pj_sockaddr_in pj_sockaddr_in;

/** Color type. */
typedef unsigned int pj_color_t;

/** Exception id. */
typedef int pj_exception_id_t;

/* ************************************************************************* */

/** Utility macro to compute the number of elements in static array. */
#define PJ_ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

/** Maximum value for signed 32-bit integer. */
#define PJ_MAXINT32  0x7FFFFFFFL

/**
 * Length of object names.
 */
#define PJ_MAX_OBJ_NAME	32

/* ************************************************************************* */
/*
 * General.
 */
/**
 * Initialize the PJ Library.
 * This function must be called before using the library. The purpose of this
 * function is to initialize static library data, such as character table used
 * in random string generation, and to initialize operating system dependent
 * functionality (such as WSAStartup() in Windows).
 *
 * Apart from calling pj_init(), application typically should also initialize
 * the random seed by calling pj_srand().
 *
 * @return PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_init(void);


/**
 * Shutdown PJLIB.
 */
PJ_DECL(void) pj_shutdown(void);

/**
 * Type of callback to register to pj_atexit().
 */
typedef void (*pj_exit_callback)(void);

/**
 * Register cleanup function to be called by PJLIB when pj_shutdown() is 
 * called.
 *
 * @param func	    The function to be registered.
 *
 * @return PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_atexit(pj_exit_callback func);



/**
 * Swap the byte order of an 16bit data.
 *
 * @param val16	    The 16bit data.
 *
 * @return	    An 16bit data with swapped byte order.
 */
// PJ_INLINE(pj_int16_t) pj_swap16(pj_int16_t val16)
// {
//     pj_uint8_t *p = (pj_uint8_t*)&val16;
//     pj_uint8_t tmp = *p;
//     *p = *(p+1);
//     *(p+1) = tmp;
//     return val16;
// }

// /**
//  * Swap the byte order of an 32bit data.
//  *
//  * @param val32	    The 32bit data.
//  *
//  * @return	    An 32bit data with swapped byte order.
//  */
// PJ_INLINE(pj_int32_t) pj_swap32(pj_int32_t val32)
// {
//     pj_uint8_t *p = (pj_uint8_t*)&val32;
//     pj_uint8_t tmp = *p;
//     *p = *(p+3);
//     *(p+3) = tmp;
//     tmp = *(p+1);
//     *(p+1) = *(p+2);
//     *(p+2) = tmp;
//     return val32;
// }


/**
 * @}
 */
/**
 * @addtogroup PJ_TIME Time Data Type and Manipulation.
 * @ingroup PJ_MISC
 * @{
 */

/**
 * Representation of time value in this library.
 * This type can be used to represent either an interval or a specific time
 * or date. 
 */
typedef struct pj_time_val
{
    /** The seconds part of the time. */
    long    sec;

    /** The miliseconds fraction of the time. */
    long    msec;

} pj_time_val;

/**
 * Normalize the value in time value.
 * @param t     Time value to be normalized.
 */
PJ_DECL(void) pj_time_val_normalize(pj_time_val *t);

/**
 * Get the total time value in miliseconds. This is the same as
 * multiplying the second part with 1000 and then add the miliseconds
 * part to the result.
 *
 * @param t     The time value.
 * @return      Total time in miliseconds.
 * @hideinitializer
 */
#define PJ_TIME_VAL_MSEC(t)	((t).sec * 1000 + (t).msec)

/**
 * This macro will check if \a t1 is equal to \a t2.
 *
 * @param t1    The first time value to compare.
 * @param t2    The second time value to compare.
 * @return      Non-zero if both time values are equal.
 * @hideinitializer
 */
#define PJ_TIME_VAL_EQ(t1, t2)	((t1).sec==(t2).sec && (t1).msec==(t2).msec)

/**
 * This macro will check if \a t1 is greater than \a t2
 *
 * @param t1    The first time value to compare.
 * @param t2    The second time value to compare.
 * @return      Non-zero if t1 is greater than t2.
 * @hideinitializer
 */
#define PJ_TIME_VAL_GT(t1, t2)	((t1).sec>(t2).sec || \
                                ((t1).sec==(t2).sec && (t1).msec>(t2).msec))

/**
 * This macro will check if \a t1 is greater than or equal to \a t2
 *
 * @param t1    The first time value to compare.
 * @param t2    The second time value to compare.
 * @return      Non-zero if t1 is greater than or equal to t2.
 * @hideinitializer
 */
#define PJ_TIME_VAL_GTE(t1, t2)	(PJ_TIME_VAL_GT(t1,t2) || \
                                 PJ_TIME_VAL_EQ(t1,t2))

/**
 * This macro will check if \a t1 is less than \a t2
 *
 * @param t1    The first time value to compare.
 * @param t2    The second time value to compare.
 * @return      Non-zero if t1 is less than t2.
 * @hideinitializer
 */
#define PJ_TIME_VAL_LT(t1, t2)	(!(PJ_TIME_VAL_GTE(t1,t2)))

/**
 * This macro will check if \a t1 is less than or equal to \a t2.
 *
 * @param t1    The first time value to compare.
 * @param t2    The second time value to compare.
 * @return      Non-zero if t1 is less than or equal to t2.
 * @hideinitializer
 */
#define PJ_TIME_VAL_LTE(t1, t2)	(!PJ_TIME_VAL_GT(t1, t2))

/**
 * Add \a t2 to \a t1 and store the result in \a t1. Effectively
 *
 * this macro will expand as: (\a t1 += \a t2).
 * @param t1    The time value to add.
 * @param t2    The time value to be added to \a t1.
 * @hideinitializer
 */
#define PJ_TIME_VAL_ADD(t1, t2)	    do {			    \
					(t1).sec += (t2).sec;	    \
					(t1).msec += (t2).msec;	    \
					pj_time_val_normalize(&(t1)); \
				    } while (0)


/**
 * Substract \a t2 from \a t1 and store the result in \a t1. Effectively
 * this macro will expand as (\a t1 -= \a t2).
 *
 * @param t1    The time value to subsctract.
 * @param t2    The time value to be substracted from \a t1.
 * @hideinitializer
 */
#define PJ_TIME_VAL_SUB(t1, t2)	    do {			    \
					(t1).sec -= (t2).sec;	    \
					(t1).msec -= (t2).msec;	    \
					pj_time_val_normalize(&(t1)); \
				    } while (0)


/**
 * This structure represent the parsed representation of time.
 * It is acquired by calling #pj_time_decode().
 */
typedef struct pj_parsed_time
{
    /** This represents day of week where value zero means Sunday */
    int wday;

    /* This represents day of the year, 0-365, where zero means
     *  1st of January.
     */
    /*int yday; */

    /** This represents day of month: 1-31 */
    int day;

    /** This represents month, with the value is 0 - 11 (zero is January) */
    int mon;

    /** This represent the actual year (unlike in ANSI libc where
     *  the value must be added by 1900).
     */
    int year;

    /** This represents the second part, with the value is 0-59 */
    int sec;

    /** This represents the minute part, with the value is: 0-59 */
    int min;

    /** This represents the hour part, with the value is 0-23 */
    int hour;

    /** This represents the milisecond part, with the value is 0-999 */
    int msec;

} pj_parsed_time;


/**
 * @}	// Time Management
 */

/* ************************************************************************* */
/*
 * Terminal.
 */
/**
 * Color code combination.
 */
enum {
    PJ_TERM_COLOR_R	= 2,    /**< Red            */
    PJ_TERM_COLOR_G	= 4,    /**< Green          */
    PJ_TERM_COLOR_B	= 1,    /**< Blue.          */
    PJ_TERM_COLOR_BRIGHT = 8    /**< Bright mask.   */
};

//#ifdef HYT_AUDIO_ENCRYPT_E2EE
#define VOICE_ENCRYPT_TYPE_UNKNOW 0
#define VOICE_ENCRYPT_TYPE_HARD_ENCRYPT 1
#define VOICE_ENCRYPT_TYPE_DMR 3
#define VOICE_ENCRYPT_TYPE_HYTERA 2
#define VOICE_ENCRYPT_TYPE_NOENCRYPT 4

#define FEATURE_SET_ID_VOICE_ENCRYPT_TYPE_HYTERA 0x68
#define FEATURE_SET_ID_VOICE_ENCRYPT_TYPE_DMRA 	 0x10

// max(decryptedDataBufLength) = num*(max_keylen(256bits) + ,num(except 1-9); + 0x0D0A(\r\n)) + (zero fill in the tail) - (excepted 1-9)
//                             = 30 *(64 + 4 + 2) + 5 -9 = 2096 in theory.
#define DECRYPT_DATA_MAX_LENGTH 2096

#define KEY_BITLEN_40 40
#define KEY_BITLEN_128 128
#define KEY_BITLEN_256 256

//Voice encryption type
enum {
    ARC4	= 0,
    AES128,
    AES256,
    ARBITRARY
};

typedef struct pjsua_encrypt_key
{
    int index;
    char key[256];
    int real_bit_num;
    int key_valid;
}pjsua_encrypt_key;

typedef struct pjsua_encrypt_config_des
{
    int encrypt_type;
	int encrypt_algorithm;
	pj_bool_t is_full_duplex;
    pjsua_encrypt_key encrypt_keys[30];
    int key_nums;
}pjsua_encrypt_config_des;
//#endif

//FEC
#define RS_FEC_BIT_WIDTH_8 8
#define RS_FEC_BIT_WIDTH_16 16

typedef struct FEC_statistic{
	int first_data_seq;
	int last_data_seq;
	pj_uint16_t fec_pkt_received;
	pj_uint16_t data_pkt_received;
	pj_uint16_t data_pkt_recovered;

	//vid FEC
	pj_uint16_t accumulate_total_pkt;
} FEC_statistic;

typedef struct pjsua_RS_FEC_config
{
	int fec_enable;
	
	int data_num;
	int fec_num;
	int width_in_bit;				//width_in_bit: 8/pj_unit8_t, 16/pj_unit16_t
	pj_uint16_t *gflog;
	pj_uint16_t *gfilog;
	pj_uint16_t **matrix_A;

	int fec_buf_size;
	pj_uint16_t ** fec_buf;
	pj_uint16_t fec_pkt_len;

	//fec decode params
	pj_bool_t fec_encode_enable;
	pj_bool_t fec_decode_enable;
	pj_uint16_t dec_fec_min_seq;
	int dec_fec_SN;
	pj_uint16_t fec_next_group_num;
	pj_uint16_t max_next_group_num;
	pj_uint16_t **inverse_matrix_A;
	void * recovered_data;
	pj_uint16_t recovered_data_buf_len;
	void ** received_pack;
	pj_uint16_t *received_len;
	pj_uint16_t *pack_lost_index;

	int ts_seq_record;
	pj_uint32_t ssrc_record;
	pj_uint32_t ts_record;
	pj_uint8_t non_fec_TG;

	FEC_statistic FEC_stat;
}pjsua_RS_FEC_config;

PJ_END_DECL


#endif /* __PJ_TYPES_H__ */

