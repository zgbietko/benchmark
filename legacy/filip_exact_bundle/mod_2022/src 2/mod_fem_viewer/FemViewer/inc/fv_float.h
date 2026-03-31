/*
 * fv_float.h
 *
 *  Created on: 22 maj 2014
 *      Author: dwg
 */

#ifndef FV_FLOAT_H_
#define FV_FLOAT_H_


#ifdef USE_DOUBLE_PREC
typedef double mfvFloat_t;
#else
typedef float mfvFloat_t;
#endif



typedef mfvFloat_t CoordType;
typedef double     ScalarValueType;


#endif /* FV_FLOAT_H_ */
