/*
 * Shader.h
 *
 *  Created on: 5 sie 2014
 *      Author: dwg
 */

#ifndef SHADER_H_
#define SHADER_H_

#include<vector>
#include"fv_inc.h"

//#include"MathHelper.h"
//#include"Matrix.h"
//#include"Light.h"


namespace FemViewer {

class Light;

enum eRenderType {
	SH_UNKNOWN	= -1,
	SH_EDGE		= 0,
	SH_TRIANGLE,
	SH_TRIANGLE_STRIP,
	SH_ALL
};


class Shader
{
  public:
	static const int ProjectionBlkIdx;
	static const int IsoValuesBlkIdx;

  public:
	Shader(eRenderType type = SH_UNKNOWN);
	virtual ~Shader();
	virtual bool Init();
	void Enable();
	//static Matrixf MP;
	//static Matrixf MV;
	//bool SetProjection(const Matrixf& pPorj, const Matrixf& pModelView);
	//void SetMatrixMP(const Matrixf& MP);
	//void SetMatrixMV(const Matrixf& MV);
  protected:
	bool 	Attach(GLenum Type,const char* FilePath);
	bool 	Complete();
	GLuint 	GetUniformParam(const char* NameOfParam);
	GLuint 	GetUniformBlockIndex(const char* UnifBlockName);

	eRenderType _type;
	GLuint  _programId;
	GLuint  _UnifBlockIndexlocation;
	GLuint  _IsoValuesUnifBlk;
  private:
	typedef std::vector<GLuint> vShaders;
	vShaders _shaders;
};

class EdgeShader : public Shader
{
  public:
	EdgeShader();
	~EdgeShader() {}

  private:
	EdgeShader(const EdgeShader&);
	EdgeShader& operator=(const EdgeShader&);
};

class TriangleShader : public Shader
{
public:
	TriangleShader(eRenderType type = SH_TRIANGLE);
	virtual ~TriangleShader() {}
	virtual bool Init();

	void SetMatrixNM(const float martix[]);
	void SetNumOfTriangles(const unsigned int nTraingles);
	void SetLight(const Light& light);
	void EdgesOn(bool flag = true);
	void IsoLinesOn(bool flag = false);
	void ColoredIsoLines(bool flag = false);

protected:
	enum {
		NUM_OF_TRIFACES = 0,
		LIGHT_POSITION,
		LIGHT_INTENISTY,
		AMBIENT_INTENSITY,
		DRAW_EDGES,
		DRAW_ISOLINES,
		ISOLINES_COLORED,
		ALL_PARAMS
	};

	GLuint _shParams[ALL_PARAMS];
private:
	TriangleShader(const TriangleShader&);
	TriangleShader& operator=(const TriangleShader&);
};


class TriStripsVGFShader : public TriangleShader
{
public:
	TriStripsVGFShader() : TriangleShader(SH_TRIANGLE_STRIP)
	{}
private:
	TriStripsVGFShader(const TriStripsVGFShader&);
	TriStripsVGFShader& operator=(const TriStripsVGFShader&);
};

}// end namespace
#endif /* SHADER_H_ */
