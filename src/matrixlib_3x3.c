//////////////////////////////////////////////////////////
// QCADesigner                                          //
// Copyright 2002 Konrad Walus                          //
// All Rights Reserved                                  //
// Author: Konrad Walus                                 //
// Email: qcadesigner@gmail.com                         //
// WEB: http://qcadesigner.ca/                          //
//////////////////////////////////////////////////////////
//******************************************************//
//*********** PLEASE DO NOT REFORMAT THIS CODE *********//
//******************************************************//
// If your editor wraps long lines disable it or don't  //
// save the core files that way. Any independent files  //
// you generate format as you wish.                     //
//////////////////////////////////////////////////////////
// Please use complete names in variables and fucntions //
// This will reduce ramp up time for new people trying  //
// to contribute to the project.                        //
//////////////////////////////////////////////////////////
// Contents:                                            //
//                                                      //
// Complex 3x3 Matrix Library														//
//                                                      //
//////////////////////////////////////////////////////////

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>

#include		"matrixlib_3x3.h"

inline void complexMatrixMultiplication(complex A[3][3], complex B[3][3], complex result[3][3]){

	result[0][0] = complexAdd(complexAdd(complexMultiply(A[0][0], B[0][0]), complexMultiply(A[0][1], B[1][0])), complexMultiply(A[0][2], B[2][0]));
	result[0][1] = complexAdd(complexAdd(complexMultiply(A[0][0], B[0][1]), complexMultiply(A[0][1], B[1][1])), complexMultiply(A[0][2], B[2][1]));
	result[0][2] = complexAdd(complexAdd(complexMultiply(A[0][0], B[0][2]), complexMultiply(A[0][1], B[1][2])), complexMultiply(A[0][2], B[2][2]));

	result[1][0] = complexAdd(complexAdd(complexMultiply(A[1][0], B[0][0]), complexMultiply(A[1][1], B[1][0])), complexMultiply(A[1][2], B[2][0]));
	result[1][1] = complexAdd(complexAdd(complexMultiply(A[1][0], B[0][1]), complexMultiply(A[1][1], B[1][1])), complexMultiply(A[1][2], B[2][1]));
	result[1][2] = complexAdd(complexAdd(complexMultiply(A[1][0], B[0][2]), complexMultiply(A[1][1], B[1][2])), complexMultiply(A[1][2], B[2][2]));

	result[2][0] = complexAdd(complexAdd(complexMultiply(A[2][0], B[0][0]), complexMultiply(A[2][1], B[1][0])), complexMultiply(A[2][2], B[2][0]));
	result[2][1] = complexAdd(complexAdd(complexMultiply(A[2][0], B[0][1]), complexMultiply(A[2][1], B[1][1])), complexMultiply(A[2][2], B[2][1]));
	result[2][2] = complexAdd(complexAdd(complexMultiply(A[2][0], B[0][2]), complexMultiply(A[2][1], B[1][2])), complexMultiply(A[2][2], B[2][2]));

}

inline void complexMatrixAddition(complex A[3][3], complex B[3][3], complex result[3][3]){

	result[0][0] = complexAdd(A[0][0], B[0][0]);
	result[0][1] = complexAdd(A[0][1], B[0][1]);
	result[0][2] = complexAdd(A[0][2], B[0][2]);
	result[1][0] = complexAdd(A[1][0], B[1][0]);
	result[1][1] = complexAdd(A[1][1], B[1][1]);
	result[1][2] = complexAdd(A[1][2], B[1][2]);
	result[2][0] = complexAdd(A[2][0], B[2][0]);
	result[2][1] = complexAdd(A[2][1], B[2][1]);
	result[2][2] = complexAdd(A[2][2], B[2][2]);

}

inline void complexMatrixSubtraction(complex A[3][3], complex B[3][3], complex result[3][3]){

	result[0][0] = complexSub(A[0][0], B[0][0]);
	result[0][1] = complexSub(A[0][1], B[0][1]);
	result[0][2] = complexSub(A[0][2], B[0][2]);
	result[1][0] = complexSub(A[1][0], B[1][0]);
	result[1][1] = complexSub(A[1][1], B[1][1]);
	result[1][2] = complexSub(A[1][2], B[1][2]);
	result[2][0] = complexSub(A[2][0], B[2][0]);
	result[2][1] = complexSub(A[2][1], B[2][1]);
	result[2][2] = complexSub(A[2][2], B[2][2]);

}

inline void complexConstMatrixMultiplication(complex A, complex B[3][3], complex result[3][3]){

	result[0][0] = complexMultiply(A, B[0][0]);
	result[0][1] = complexMultiply(A, B[0][1]);
	result[0][2] = complexMultiply(A, B[0][2]);
	result[1][0] = complexMultiply(A, B[1][0]);
	result[1][1] = complexMultiply(A, B[1][1]);
	result[1][2] = complexMultiply(A, B[1][2]);
	result[2][0] = complexMultiply(A, B[2][0]);
	result[2][1] = complexMultiply(A, B[2][1]);
	result[2][2] = complexMultiply(A, B[2][2]);

}

inline void complexConstMatrixDivision(complex A, complex B[3][3], complex result[3][3]){

	result[0][0] = complexDivide(A, B[0][0]);
	result[0][1] = complexDivide(A, B[0][1]);
	result[0][2] = complexDivide(A, B[0][2]);
	result[1][0] = complexDivide(A, B[1][0]);
	result[1][1] = complexDivide(A, B[1][1]);
	result[1][2] = complexDivide(A, B[1][2]);
	result[2][0] = complexDivide(A, B[2][0]);
	result[2][1] = complexDivide(A, B[2][1]);
	result[2][2] = complexDivide(A, B[2][2]);

}

inline void complexMatrixRealExponential(complex result[3][3]){
	//takes the exponent of the real part
	//basically we assume that this will be used only on real matricies
	//that had to be made complex for other calculations
	//checks if a matrix is real
	//if(!IS_REAL_3X3(result)){
	//	printf("Critical Error: complex matrix was passed to complexMatrixRealExponential()\n");
	//	exit(1);
	//}

	result[0][0].re = exp(result[0][0].re);	result[0][1].re = exp(result[0][1].re);	result[0][2].re = exp(result[0][2].re);
	result[1][0].re = exp(result[1][0].re);	result[1][1].re = exp(result[1][1].re);	result[1][2].re = exp(result[1][2].re);
	result[2][0].re = exp(result[2][0].re);	result[2][1].re = exp(result[2][1].re);	result[2][2].re = exp(result[2][2].re);

}

// complex matrix trace
inline complex complexTr(complex A[3][3]){
	complex result;

	result.re = A[0][0].re + A[1][1].re + A[2][2].re;
	result.im = A[0][0].im + A[1][1].im + A[2][2].im;

	return(result);
}

//complex multiplication using only three real multiplications
inline complex complexMultiply(complex A, complex B){
	/*complex result;

	result.re = A.re*B.re - A.im*B.im;
	result.im = A.re*B.im + A.im*B.re;
	return result;
	*/
	complex result;
	double temp = A.re*B.re;
	double temp2 = A.im*B.im;

	result.re = temp - temp2;
	result.im = (A.re+A.im)*(B.re+B.im) - temp - temp2;
	return result;

}

inline complex complexDivide(complex denominator, complex numerator){
	complex result;
	long double nre = numerator.re;
	long double nim = numerator.im;
	long double dre = denominator.re;
	long double dim = denominator.im;
	long double denom;

	//if numbers are too large they could cause an overflow in the division
	//so instead I multiply the numerator and denominator by a very small number
	//the result should be the same. However .....
	if(nre > 1e150 || nim > 1e150 || dre > 1e150 || dim > 1e150)
		{
		nre*=1e-150;
		nim*=1e-150;
		dre*=1e-150;
		dim*=1e-150;
		}

	denom = dre*dre + dim*dim;

	result.re = (long double)((nre*dre+nim*dim)/denom);
	result.im = (long double)((nim*dre-nre*dim)/denom);
	return result;
}

inline complex complexAdd(complex A, complex B){
	complex result;

	result.re = A.re + B.re;
	result.im = A.im + B.im;
	return result;
}

inline complex complexSub(complex A, complex B){
	complex result;

	result.re = A.re - B.re;
	result.im = A.im - B.im;
	return result;
}

inline void complexIdentityMatrix(complex result[3][3]){

	result[0][0].re = 1;
	result[0][1].re = 0;
	result[0][2].re = 0;
	result[1][0].re = 0;
	result[1][1].re = 1;
	result[1][2].re = 0;
	result[2][0].re = 0;
	result[2][1].re = 0;
	result[2][2].re = 1;

	result[0][0].im = 0;
	result[0][1].im = 0;
	result[0][2].im = 0;
	result[1][0].im = 0;
	result[1][1].im = 0;
	result[1][2].im = 0;
	result[2][0].im = 0;
	result[2][1].im = 0;
	result[2][2].im = 0;

}

inline void complexExtractColumn(int column, complex A[3][3], complex vector[3]){
	vector[0] = A[0][column];
	vector[1] = A[1][column];
	vector[2] = A[2][column];
}

inline void complexExtractRow(int row, complex A[3][3], complex vector[3]){
	vector[0] = A[row][0];
	vector[1] = A[row][1];
	vector[2] = A[row][2];
}
