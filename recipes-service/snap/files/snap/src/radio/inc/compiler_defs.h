/** \file compiler_defs.h
 *
 * \brief Register/bit definitions for cross-compiler projects on the C8051F93x/2x family.
 *
 * \b COPYRIGHT
 * \n Portions of this file are copyright Maarten Brock
 * \n http://sdcc.sourceforge.net
 * \n Portions of this file are copyright 2010, Silicon Laboratories, Inc.
 * \n http://www.silabs.com
 *
 *
 */

//-----------------------------------------------------------------------------
// Header File Preprocessor Directive
//-----------------------------------------------------------------------------

#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define SEG_FAR
#define SEG_DATA
#define SEG_NEAR
#define SEG_IDATA
#define SEG_XDATA
#define SEG_PDATA
#define SEG_CODE
#define SEG_BDATA

#define SBIT(name, addr, bit)  volatile unsigned char  name
#define SFR(name, addr)        volatile unsigned char  name
#define SFRX(name, addr)       volatile unsigned char  name
#define SFR16(name, addr)      volatile unsigned short name
#define SFR16E(name, fulladdr) volatile unsigned short name
#define SFR32(name, fulladdr)  volatile unsigned long  name
#define SFR32E(name, fulladdr) volatile unsigned long  name
#define INTERRUPT(name, vector) void name (void)
#define INTERRUPT_USING(name, vector, regnum) void name (void)
#define INTERRUPT_PROTO(name, vector) void name (void)
#define INTERRUPT_PROTO_USING(name, vector, regnum) void name (void)
#define FUNCTION_USING(name, return_value, parameter, regnum) return_value name (parameter)
#define FUNCTION_PROTO_USING(name, return_value, parameter, regnum) return_value name (parameter)
// Note: Parameter must be either 'void' or include a variable type and name. (Ex: char temp_variable)

#define SEGMENT_VARIABLE(name, vartype, locsegment) vartype locsegment name
#define VARIABLE_SEGMENT_POINTER(name, vartype, targsegment) vartype targsegment * name
#define SEGMENT_VARIABLE_SEGMENT_POINTER(name, vartype, targsegment, locsegment) vartype targsegment * locsegment name
#define SEGMENT_POINTER(name, vartype, locsegment) vartype * locsegment name
#define LOCATED_VARIABLE(name, vartype, locsegment, addr, init) locsegment vartype name
#define LOCATED_VARIABLE_NO_INIT(name, vartype, locsegment, addr) locsegment vartype name


// used with UU16
#define LSB 0
#define MSB 1

// used with UU32 (b0 is least-significant byte)
#define b0 0
#define b1 1
#define b2 2
#define b3 3

typedef unsigned char U8;
typedef unsigned int U16;
typedef unsigned long U32;

typedef signed char S8;
typedef signed int S16;
typedef signed long S32;

typedef union UU16
{
    U16 U16;
    S16 S16;
    U8 U8[2];
    S8 S8[2];
} UU16;

typedef union UU32
{
    U32 U32;
    S32 S32;
    UU16 UU16[2];
    U16 U16[2];
    S16 S16[2];
    U8 U8[4];
    S8 S8[4];
} UU32;

// NOP () macro support
extern void _nop (void);
#define NOP() _nop()

typedef unsigned char bit;
typedef bit BIT;


#define BITS(bitArray, bitPos)  BIT bitArray ## bitPos
#define WRITE_TO_BIT_ARRAY(bitArray, byte)  bitArray ## 0 = byte & 0x01; \
                                            bitArray ## 1 = byte & 0x02; \
                                            bitArray ## 2 = byte & 0x04; \
                                            bitArray ## 3 = byte & 0x08; \
                                            bitArray ## 4 = byte & 0x10; \
                                            bitArray ## 5 = byte & 0x20; \
                                            bitArray ## 6 = byte & 0x40; \
                                            bitArray ## 7 = byte & 0x80;

#define READ_FROM_BIT_ARRAY(bitArray, byte) byte =  (bitArray ## 0) | \
                                                   ((bitArray ## 1) << 1) | \
                                                   ((bitArray ## 2) << 2) | \
                                                   ((bitArray ## 3) << 3) | \
                                                   ((bitArray ## 4) << 4) | \
                                                   ((bitArray ## 5) << 5) | \
                                                   ((bitArray ## 6) << 6) | \
                                                   ((bitArray ## 7) << 7);

//-----------------------------------------------------------------------------
// Compiler independent data type definitions
//-----------------------------------------------------------------------------
#ifndef   FALSE
#define   FALSE     0
#endif
#ifndef   TRUE
#define   TRUE      !FALSE
#endif

#ifndef   NULL
#define   NULL      ((void *) 0)
#endif

//-----------------------------------------------------------------------------
// Header File PreProcessor Directive
//-----------------------------------------------------------------------------

#endif                                 // #define COMPILER_DEFS_H

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
