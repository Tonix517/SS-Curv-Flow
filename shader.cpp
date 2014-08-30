#include "shader.h"

#include "IL/ilu.h"
#include "IL/ilut.h"

#include <stdlib.h>
#include <stdio.h>

unsigned char *face = NULL;

GLint cubeMapInx = 0;
GLuint nCubemapTexId;
int nFaceDim = 0;

GLuint program = 0;
GLuint v_shader = 0;
GLuint f_shader = 0;

int thickAttribInx = 1;
char *thickAttribStr = "fThickA";

GLenum  cube[6] = 
{
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y
};

///

bool linkShaders()
{
	GLint linked = 0;

	program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, f_shader);

	glBindAttribLocation(program, thickAttribInx, thickAttribStr);

	glLinkProgram(program);
	    
	glGetProgramiv(program, GL_LINK_STATUS, &linked);	    
	if(linked)
	{
		glUseProgram(program);   
		return true;
	}

	GLint length;
	GLchar * log;
        
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	log = (GLchar *)malloc(length);

	glGetProgramInfoLog(program, length, &length, log);
	fprintf(stderr, "link log = %s \n", log);

	free(log);   

	return false;
}

void unlinkShaders()
{
	glDeleteShader(v_shader);
	glDeleteShader(f_shader);
	glDeleteProgram(program);
}

bool loadCubemapTex(char *texPath)
{
	if(face)
	{
		free(face);
		face = NULL;
	}
	
	//	Get Image Info
	ILuint nCurrTexImg = 0;
	ilGenImages(1, &nCurrTexImg);
	ilBindImage(nCurrTexImg);	

	if(ilLoadImage(texPath))
	{
		int nWidth  = ilGetInteger(IL_IMAGE_WIDTH);
		int nHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		
		nFaceDim = abs(nWidth - nHeight);
		int nDataOffset = nFaceDim * nFaceDim * 3;

		face = (unsigned char *)malloc(sizeof(unsigned char) * 6 * nFaceDim * nFaceDim  * 3);

		//GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		//GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,
		//GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_Y

		ilCopyPixels(           0, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face);		
		ilCopyPixels(nFaceDim    , nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face + nDataOffset);		
		ilCopyPixels(nFaceDim * 2, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face + nDataOffset * 2);	
		ilCopyPixels(nFaceDim * 3, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face + nDataOffset * 3);				
		ilCopyPixels(nFaceDim    , nFaceDim * 2    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face + nDataOffset * 4);				
		ilCopyPixels(nFaceDim    , 0, 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, face + nDataOffset * 5);				
		
		ilDeleteImages(1, &nCurrTexImg);

		//
		glEnable(GL_TEXTURE_2D);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
		glGenTextures(1, &nCubemapTexId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);		

		glBindTexture(GL_TEXTURE_CUBE_MAP, nCubemapTexId);	
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Set far filtering mode
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   

		for (int i = 0; i < 6; i ++) 
		{ 
			glTexImage2D
			(	
				cube[i], 
				0,                  //level 
				3,					//internal format 
				nFaceDim,           //width 
				nFaceDim,           //height 
				0,                  //border 
				GL_RGB,             //format 
				GL_UNSIGNED_BYTE,   //type 
				face + i * nFaceDim * nFaceDim * 3
			); // pixel data 
		}

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP); //自动生成纹理坐标
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

		return true;
	}

	ILenum ilErr = ilGetError();
	//const char* sErrMsg = iluErrorString(IL_INVALID_ENUM);
	printf("Error in LoadImage: %d [%s]\n", ilErr, texPath);

	ilDeleteImages(1, &nCurrTexImg);
	return false;
}

void unloadCubemapTex()
{
	if(face)
	{
		free(face);
		face = NULL;
	}

	glDeleteTextures(1, &nCubemapTexId);
}

bool loadShader(char *shaderPath, ShaderType eType)
{

	if(GLEE_ARB_vertex_program)
	{
		GLenum eShaderType;
		switch(eType)
		{
		case VERTEX:
			eShaderType = GL_VERTEX_SHADER;
			break;

		case FRAGMENT:
			eShaderType = GL_FRAGMENT_SHADER;
			break;

		default:
			printf("Not supported shader type! \n");
			return false;
			break;
		}

		//	Load from file
		char *shaderSrc = NULL;
		FILE *fp = fopen(shaderPath, "r");
		if(fp)
		{
			fseek(fp, 0, SEEK_END);
			long int fLen = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			if(fLen > 0)
			{
				shaderSrc = (char *)malloc(sizeof(char) * fLen);
				memset(shaderSrc, 0, sizeof(char) * fLen);
				fread(shaderSrc, sizeof(char) * fLen, 1, fp);
			}
			else
			{
				printf("Empty shader file.\n");
				fclose(fp);
				return false;
			}

			fclose(fp);
		}
		else
		{
			printf("cannot open shader file! \n");
			return false;
		}
		

		GLuint shader;
		GLint compiled;
	    
		shader = glCreateShader(eShaderType);
		switch(eType)
		{
		case VERTEX:
			v_shader = shader;
			break;

		case FRAGMENT:
			f_shader = shader;
			break;

		default:
			printf("Not supported shader type! \n");
			return false;
			break;
		}
	    
		//
		glShaderSource(shader, 1, (const GLchar **)&shaderSrc, NULL);
		glCompileShader(shader);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	    
		if(shaderSrc)
		{
			free(shaderSrc);
			shaderSrc = NULL;
		}

		if(!compiled)
		{
			GLint length;
			GLchar * log;

			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			log = (GLchar *)malloc(length);
	        
			glGetShaderInfoLog(shader, GL_INFO_LOG_LENGTH, &length, log);
			fprintf(stderr, "compile log = %s \n", log);

			free(log);

			return false;
		}

		return true;
	}// ARB

	return false;
}

///
///		Skybox
///
GLuint skyTex[6] = {0};

unsigned char *skyBoxData[6] = {0};

void loadSkyBoxTex(char *texPath, GLuint *skyTex)
{

	//GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	//GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	//GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	//GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,
	//GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	//GL_TEXTURE_CUBE_MAP_POSITIVE_Y

	ILuint nCurrTexImg = 0;
	ilGenImages(1, &nCurrTexImg);
	ilBindImage(nCurrTexImg);	

	if(ilLoadImage(texPath))
	{
		int nWidth  = ilGetInteger(IL_IMAGE_WIDTH);
		int nHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		
		nFaceDim = abs(nWidth - nHeight);
		int nDataOffset = nFaceDim * nFaceDim * 3;
		
		//GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		//GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		//GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		//GL_TEXTURE_CUBE_MAP_POSITIVE_Y
		{
			skyBoxData[0] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(           0, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[0]);		

			skyBoxData[1] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(nFaceDim    , nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[1]);

			skyBoxData[2] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(nFaceDim * 2, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[2]);	

			skyBoxData[3] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(nFaceDim * 3, nFaceDim    , 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[3]);		

			skyBoxData[4] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(nFaceDim    , nFaceDim * 2, 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[4]);				

			skyBoxData[5] = (unsigned char *)malloc(sizeof(unsigned char) * nFaceDim * nFaceDim  * 3);
			ilCopyPixels(nFaceDim    , 0, 0, nFaceDim, nFaceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, skyBoxData[5]);				
		}

		{
			glEnable(GL_TEXTURE_2D);

			for(int i = 0; i < 6; i ++)
			{
				glGenTextures(1, &skyTex[i]);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);		
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

				glBindTexture(GL_TEXTURE_2D, skyTex[i]);	

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Set far filtering mode
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

				//glTexImage2D
				//(	
				//	GL_TEXTURE_2D, 
				//	0,                  //level 
				//	3,					//internal format 
				//	nFaceDim,           //width 
				//	nFaceDim,           //height 
				//	0,                  //border 
				//	GL_RGB,             //format 
				//	GL_UNSIGNED_BYTE,   //type 
				//	skyBoxData[i]
				//); // pixel data 
				GLint ret = gluBuild2DMipmaps(	GL_TEXTURE_2D, 3, nFaceDim, 
												nFaceDim, GL_RGB, GL_UNSIGNED_BYTE, skyBoxData[i]);

				if(ret != 0)
				{
					 printf("- %s -\n", gluErrorString(ret));
				}
			
			}

		}

		ilDeleteImages(1, &nCurrTexImg);
	}
}

void unloadSkyBoxTex()
{
	glDeleteTextures(6, skyTex);
	for(int i = 0; i < 6; i ++)
	{
		free(skyBoxData[i]);
	}
}