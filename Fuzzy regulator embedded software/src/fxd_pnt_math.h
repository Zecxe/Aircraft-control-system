#ifndef __FXD_PNT_MATH_H
#define __FXD_PNT_MATH_H

#include "math.h"

// Redefinition of standard data types to highlighy its Q form
#define __q32_t		int32_t
#define __qu32_t	uint32_t
#define __q16_t		int16_t
#define	__qu16_t	uint16_t
#define __q64_t		int64_t
#define __qu64_t	uint64_t

/* The basic operations perfomed on two numbers a and b of fixed
 point q format returning the answer in q format */
#define FADD(a,b) ((a)+(b))
#define FSUB(a,b) ((a)-(b))
#define FMUL(a,b,q) (((a)*(b))>>(q))
#define FDIV(a,b,q) ((__q32_t)((a)<<(q))/(b))

/* The basic operations where a is of fixed point q format and b is
 an integer */
#define FADDI(a,b,q) ((a)+((b)<<(q)))
#define FSUBI(a,b,q) ((a)-((b)<<(q)))
#define FMULI(a,b) ((a)*(b))
#define FDIVI(a,b) ((a)/(b))

/* convert a from q1 format to q2 format */
#define FCONV(a, q1, q2) (((q2)>(q1)) ? (a)<<((q2)-(q1)) : (a)>>((q1)-(q2)))
          
/* the general operation between a in q1 format and b in q2 format
 returning the result in q3 format */
#define FADDG(a,b,q1,q2,q3) (FCONV(a,q1,q3)+FCONV(b,q2,q3))
#define FSUBG(a,b,q1,q2,q3) (FCONV(a,q1,q3)-FCONV(b,q2,q3))
#define FMULG(a,b,q1,q2,q3) FCONV((a)*(b), (q1)+(q2), q3)
#define FDIVG(a,b,q1,q2,q3) (FCONV(a, q1, (q2)+(q3))/(b))

#define FTOINT(a, q) ( (uint32_t)((a)>>(q)) )
#define FLTOF(a, q) ( (__qu32_t)((a) * (1 << (q))) )

#endif // _FXD_PNT_MATH_H
