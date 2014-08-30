#ifndef SHADER_H
#define SHADER_H

#include "GL/glee.h"

enum ShaderType {VERTEX, FRAGMENT};

extern GLenum  cube[6];

extern GLuint nCubemapTexId;
extern GLint cubeMapInx;

extern GLuint program;
extern GLuint v_shader;
extern GLuint f_shader;

extern unsigned char *face;
extern int nFaceDim;

bool loadCubemapTex(char *texPath);
void unloadCubemapTex();

bool linkShaders();
void unlinkShaders();
bool loadShader(char *shaderPath, ShaderType eType);

extern int thickAttribInx;
extern char *thickAttribStr;

///	Skybox
//GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
//GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
//GL_TEXTURE_CUBE_MAP_POSITIVE_X,
//GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,
//GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
//GL_TEXTURE_CUBE_MAP_POSITIVE_Y
extern GLuint skyTex[6];	
extern unsigned char *skyBoxData[6];
void unloadSkyBoxTex();
void loadSkyBoxTex(char *texPath, GLuint *);

#endif