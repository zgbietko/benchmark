#ifndef _ELEM_TABLES_H_
#define _ELEM_TABLES_H_

#include "CutPlane.h"
#include "GraphicElem.hpp"

/*   The vertices in tetrahedron are indexed as follows:
       
	      + 0
         /|\
        / | \
       /  |  \
      +------+ 1
     3 \  |  /
        \ | /
         \|/
          + 2
 
    The edges in the tetrahedron are indexed as follows:
	
	      + 
         /|\
        3 | 0
       /  |  \
      +-4-----+ 
       \  2  /
        5 | 1
         \|/
          +
*/

/* Tetrahedron Edge Flags Table*/


/*
This table lists the edges intersected by the surface according to 'vertex states'
Each vertex can be in one of two states: active (inside of the surface) not active (outside of the surface). This determines 2^4=16 possible sets of 'vertex states'
There are 6 edges of the tetrahedron. If the given edge is intersected the right bit is set to 1.
*/

//int iTetrahedronEdgeF[16]=
//  {
//    0x000, /*0000 0000 inactive*/
//    0x00d, /*0000 1101*/
//    0x013, /*0001 0011*/
//    0x01e, /*0001 1110*/
//    0x026, /*0010 0110*/
//    0x02b, /*0010 1011*/
//    0x035, /*0011 0101*/
//    0x038, /*0011 1000*/
//    0x038, /*0011 1000*/
//    0x035, /*0011 1001*/
//    0x02b, /*0010 1011*/
//    0x026, /*0010 0110*/
//    0x01e, /*0001 1110*/
//    0x013, /*0001 0011*/
//    0x00d, /*0000 1101*/
//    0x000, /*0000 0000 active*/
//  };
//
//
///*Tetrahedron Triangles Connection Table*/
//
///*
//This table lists all configurations for triangulation of edge intersection points for tetrahedon.For each of the case there is a specific triangulation in the form 
//of 0-5 egde tiples terminated by -1. There is maximum 2 trinagles per tetrahedon in 4
//topologies. There are no ambiguous cases. 
//*/
//
//int iTetrahedronTrianglesCT[16][7] =
//  {
//    {-1, -1, -1, -1, -1, -1, -1}, /* 0 */
//    { 0,  3,  2, -1, -1, -1, -1}, /* 1 */
//    { 0,  1,  4, -1, -1, -1, -1}, /* 2 */
//    { 1,  4,  2,  2,  4,  3, -1}, /* 3 */
//
//    { 1,  2,  5, -1, -1, -1, -1}, /* 4 */
//    { 0,  3,  5,  0,  5,  1, -1}, /* 5 */
//    { 0,  2,  5,  0,  5,  4, -1}, /* 6 */
//    { 5,  4,  3, -1, -1, -1, -1}, /* 7 */
//
//    { 3,  4,  5, -1, -1, -1, -1}, /* 8 */
//    { 4,  5,  0,  5,  2,  0, -1}, /* 9 */
//    { 1,  5,  0,  5,  3,  0, -1}, /* 10 */
//    { 5,  2,  1, -1, -1, -1, -1}, /* 11 */
//
//    { 3,  4,  2,  2,  4,  1, -1}, /* 12 */
//    { 4,  1,  0, -1, -1, -1, -1}, /* 13 */
//    { 2,  3,  0, -1, -1, -1, -1}, /* 14 */
//    {-1, -1, -1, -1, -1, -1, -1}, /* 15 */
//  };
//
//
///*Squares Triangles Connection Table*/
//
/*
This table lists all configurations for lines generation of edge intersection points for square.For each of the case there is a specific line in the form 
of 0-4 egde dubles terminated by -1. There are maximum 2 lines per square in 4
topologies. There are two ambiguous cases. 
*/
extern int iQuadEdgesIds[4][2];
extern int iQuadFlags[16];
extern int iQuadEdges[16][7];
extern const int iPrizmEdges[9][2];
///* Square Edge Flags Table*/


extern int  DoSliceTerahedron(const FemViewer::CutPlane* plane,const int faces[5],double tetra[16], std::vector<FemViewer::GraphElement2<double> >& grEls,FILE* fp);

extern int  DoSlicePrizm(const FemViewer::CutPlane* plane,const int faces[5],double tetra[16], std::vector<FemViewer::GraphElement2<double> >& grEls,FILE* fp);




#endif