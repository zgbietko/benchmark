/*
l182
 * Shader.cpp
 *
 *  Created on: 5 sie 2014
 *      Author: dwg
 */
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include"defs.h"
#include"uth_log.h"
#include "fv_txt_utls.h"
#include "fv_dictstr.h"
#include"Light.h"
#include"Shader.h"
#include"Legend.h"
#include"ViewManager.h"


namespace FemViewer {


typedef struct {
	const char * vert_src;
	const char * geom_src;
	const char * frag_src;
} shader_srcs;

shader_srcs g_shaders[SH_ALL] = {
	// Wireframe rendering
	{
	"shEdge.vert",
	nullptr,
	"shEdge.frag"
	},
	// Linear rendering
	{
	"shTriVGF.vert",
	"shTriVGF.geom",
	"shTriVGF.frag"
	},
	// High order rendering
	{
	"shTriVGF.vert",
	"shTriStripVGF.geom",
	"shTriVGF.frag"
	}
};

const int Shader::ProjectionBlkIdx = 0;
const int Shader::IsoValuesBlkIdx = 1;

Shader::Shader(eRenderType type)
: _type(type)
, _programId(0)
, _UnifBlockIndexlocation(-1)
, _IsoValuesUnifBlk(-1)
{
}

Shader::~Shader()
{
	//mfp_log_debug("Shader::Dtr\n");
	for (vShaders::iterator it = _shaders.begin(); it != _shaders.end(); ++it) {
		glDeleteShader(*it);
	}

	if (_programId) {
		glDeleteProgram(_programId);
		_programId = 0;
	}
}

bool Shader::Init()
{
	//mfp_log_debug("Shader::Init\n");
	bool result(initGLEW());

	if (result) _programId = glCreateProgram();
	result = (_programId != 0) && (g_shaders[this->_type].vert_src != nullptr);

	if (result) {
		result = Attach(GL_VERTEX_SHADER,g_shaders[_type].vert_src);
	}
	if (result && g_shaders[this->_type].geom_src != NULL) {
		//mfp_debug("geometry shader creatin\n");
		result = Attach(GL_GEOMETRY_SHADER,g_shaders[_type].geom_src);
	}
	if (result && g_shaders[this->_type].frag_src != NULL) {
		result = Attach(GL_FRAGMENT_SHADER,g_shaders[_type].frag_src);
	}
	if (result) result = Complete();

	return result;
}

void Shader::Enable()
{
	assert(_programId > 0);
	glUseProgram(_programId);
}

//void Shader::SetMatrixMP(const Matrixf& MP)
//{
//	glUniformMatrix4fv(_MPlocation, 1, GL_FALSE, MP.matrix.data());
//}
//
//void Shader::SetMatrixMV(const Matrixf& MV)
//{
//	glUniformMatrix4fv(_MVlocation, 1, GL_FALSE, MV.matrix.data());
//}

bool Shader::Attach(GLenum Type,const char* FilePath)
{
	assert(_programId != 0);
	//mfp_debug("Loading file: %s\n",FilePath);
	std::string source;
	if (! loadFile(FilePath,source)) {
		mfp_log_err("Error while reading shader file %s\n",FilePath);
		return false;
	}
	//mfp_debug("Before create shader\n");
	GLuint shaderId = glCreateShader(Type);
	if (shaderId == 0) {
		mfp_log_err("Error while creating shader type %d\n",Type);
		return false;
	}
	_shaders.push_back(shaderId);

	//mfp_debug("before compiling shader\n");
	char const * srcPtr = source.c_str();
	glShaderSource(shaderId, 1, &srcPtr, NULL);
	glCompileShader(shaderId);

	GLint Result = GL_FALSE;

	// Check shader
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &Result);

	if (Result == GL_FALSE) {
		GLint length = 0;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
		std::vector<char> ShaderErrorMessage(length+1);
		glGetShaderInfoLog(shaderId, length, NULL, &ShaderErrorMessage[0]);
		fprintf(stderr,"%s\n", &ShaderErrorMessage[0]);
	}

	glAttachShader(_programId, shaderId);
	return (glGetError() == GL_NO_ERROR);
}

bool Shader::Complete()
{
	assert(_programId != 0);
	GLint Result = GL_FALSE;

	// Link program
	glLinkProgram(_programId);

	// Check the program
	glGetProgramiv(_programId, GL_LINK_STATUS, &Result);

	if (Result == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(_programId, GL_INFO_LOG_LENGTH, &length);
		std::vector<char> LinkerErrorMessage(length+1);
		glGetProgramInfoLog(_programId, length, NULL, &LinkerErrorMessage[0]);
		mfp_log_err("%s\n", &LinkerErrorMessage[0]);
		return false;
	}

	//mfp_debug("Albo tu program id = %u\n",_programId);

	for (vShaders::iterator it = _shaders.begin(); it != _shaders.end(); ++it) {
		glDeleteShader(*it);
	}

	_UnifBlockIndexlocation = GetUniformBlockIndex("Projection");
	if (_UnifBlockIndexlocation == INVALID_LOCATION) {
		return false;
	}
	glUniformBlockBinding(_programId, _UnifBlockIndexlocation, ProjectionBlkIdx);

	_IsoValuesUnifBlk = GetUniformBlockIndex("IsoValues");
	if (_IsoValuesUnifBlk == INVALID_LOCATION) {
		return false;
	}
	glUniformBlockBinding(_programId, _IsoValuesUnifBlk, IsoValuesBlkIdx);
	FV_CHECK_ERROR_GL();

	//mfp_debug("After binding uniform block pf Params --- complete shader %u %u\n",_IsoValuesUnifBlk,IsoValuesBlkIdx);
	_shaders.clear();
	return (glGetError() == GL_NO_ERROR);
}

//bool Shader::SetProjection(const Matrixf& pPorj, const Matrixf& pModelView)
//{
//	return true;
//}

GLuint Shader::GetUniformParam(const char* NameOfParam)
{
	GLuint location = glGetUniformLocation(_programId, NameOfParam);
	if (location == INVALID_LOCATION) {
		mfp_log_warn("Can't get location for parameter %s\n",NameOfParam);
		//exit(-1);
	}

	return location;
}

GLuint Shader::GetUniformBlockIndex(const char* UnifBlockName)
{
	GLuint location = glGetUniformBlockIndex(_programId, UnifBlockName);
	if (location == INVALID_LOCATION) {
		mfp_log_warn("Can't get location for uniform block index of a name: %s\n",UnifBlockName);
	}

	return location;
}

EdgeShader::EdgeShader()
: Shader(SH_EDGE)
{
}


TriangleShader::TriangleShader(eRenderType type)
: Shader(type)
{
	memset(_shParams,0xFF,sizeof(_shParams));
}

bool TriangleShader::Init()
{
	//mfp_log_debug("Init Triangle Shader\n");
	bool result = Shader::Init();
	if (!result) return result;

	// Read parameters location
	_shParams[NUM_OF_TRIFACES] = GetUniformParam("nTriangles");
	_shParams[LIGHT_POSITION]  = GetUniformParam("posLight");
	_shParams[LIGHT_INTENISTY] = GetUniformParam("lightIntensity");
	_shParams[AMBIENT_INTENSITY] = GetUniformParam("ambientIntensity");
	_shParams[DRAW_EDGES] = GetUniformParam("bDrawEdges");
	_shParams[DRAW_ISOLINES] = GetUniformParam("bDrawIsoLines");

	if (_shParams[NUM_OF_TRIFACES] == INVALID_LOCATION ||
		_shParams[LIGHT_POSITION] == INVALID_LOCATION ||
		_shParams[LIGHT_INTENISTY] == INVALID_LOCATION ||
		_shParams[AMBIENT_INTENSITY] == INVALID_LOCATION ||
		_shParams[DRAW_EDGES] == INVALID_LOCATION ||
		_shParams[DRAW_ISOLINES] == INVALID_LOCATION
		) {
		result = false;
	}

	return result;
}

//void TriangleShader::SetMatrixNM(const float normMatrix[])
//{
//	//assert(_NormalModelViewMatrixUnif != INVALID_LOCATION);
//	//glUniformMatrix3fv(	_NormalModelViewMatrixUnif, 1, GL_FALSE, normMatrix);
//}

void TriangleShader::SetNumOfTriangles(const unsigned int nTriangles)
{
	//mfp_debug("Number of trianglefaces: %u\n",nTriangles);
	assert(_shParams[NUM_OF_TRIFACES] != INVALID_LOCATION);
	glUniform1i(_shParams[NUM_OF_TRIFACES], static_cast<GLint>(nTriangles));
}

void TriangleShader::SetLight(const Light& light)
{
	assert(_shParams[LIGHT_POSITION] != INVALID_LOCATION);
	assert(_shParams[LIGHT_INTENISTY] != INVALID_LOCATION);
	assert(_shParams[AMBIENT_INTENSITY] != INVALID_LOCATION);
	//glUniform2f(_WinScale, WinScale[0], WinScale[1]);
	FV_CHECK_ERROR_GL();
	//mfp_debug("Setting light position: %f %f %f\n",light.Position().x,light.Position().y,light.Position().z);
	glUniform3fv(_shParams[LIGHT_POSITION], 1, light.Position().v);
	glUniform3fv(_shParams[LIGHT_INTENISTY], 1, light.DiffuseIntensity().v);
	glUniform3fv(_shParams[AMBIENT_INTENSITY], 1, light.AmbientIntensity().v);
	//glUniform3fv(_LDirlocation, 1, light.Direction().v);
}

void TriangleShader::EdgesOn(bool flag)
{
	//mfp_debug("Setting bDrawEdges %u to : %d\n",_DrawEdgesUnif,int(flag));
	assert(_shParams[DRAW_EDGES] != INVALID_LOCATION);
	glUniform1i(_shParams[DRAW_EDGES], flag ? 1 : 0);
}

void TriangleShader::IsoLinesOn(bool flag)
{
	//mfp_debug("Setting bDrawIsovalues %u to : %d\n",_DrawIsoLinesUnif,int(flag));
	assert(_shParams[DRAW_ISOLINES] != INVALID_LOCATION);
	glUniform1i(_shParams[DRAW_ISOLINES], flag ? 1 : 0);
}

void TriangleShader::ColoredIsoLines(bool flag)
{
	//mfp_debug("Setting bColoredIsoLines %u to : %d\n",_ColoredIsoLinesUnif,int(flag));
	assert(_shParams[ISOLINES_COLORED] != INVALID_LOCATION);
	glUniform1i(_shParams[ISOLINES_COLORED], flag ? 1 : 0);
}



}// end namespace


