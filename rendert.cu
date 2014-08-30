
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <algorithm>

#include "GL/glee.h"
#include "GL/glui.h"
#include "GL/glut.h"

#include "global.h"
#include "consts.h"
#include "particle.h"
#include "math_util.h"
#include "shader.h"

///
///		Render SkyBox
	
float fSkyFaceDim = 600;

void renderSkyBox()
{
	glEnable(GL_TEXTURE_2D);

	//	Ground
	glBindTexture(GL_TEXTURE_2D, skyTex[4]);
	float groundCtr[3] = {CamPos[0], CamPos[1] - fSkyFaceDim / 2.f, CamPos[2]};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 0);	glVertex3f(groundCtr[0] - fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 1);	glVertex3f(groundCtr[0] - fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 1);	glVertex3f(groundCtr[0] + fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(groundCtr[0] + fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] - fSkyFaceDim / 2.f);
	glEnd();

	//	X-
	glBindTexture(GL_TEXTURE_2D, skyTex[0]);
	float x_m_ctr[3] = {CamPos[0] - fSkyFaceDim / 2.f, CamPos[1], CamPos[2]};
	glBegin(GL_QUADS);
		glTexCoord2d(1, 1);	glVertex3f(x_m_ctr[0], x_m_ctr[1] - fSkyFaceDim / 2.f, x_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(x_m_ctr[0], x_m_ctr[1] + fSkyFaceDim / 2.f, x_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 0);	glVertex3f(x_m_ctr[0], x_m_ctr[1] + fSkyFaceDim / 2.f, x_m_ctr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(0, 1);	glVertex3f(x_m_ctr[0], x_m_ctr[1] - fSkyFaceDim / 2.f, x_m_ctr[2] + fSkyFaceDim / 2.f);
	glEnd();

	//	-Z
	glBindTexture(GL_TEXTURE_2D, skyTex[1]);
	float z_m_ctr[3] = { CamPos[0], CamPos[1], CamPos[2] - fSkyFaceDim / 2.f};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);	glVertex3f(z_m_ctr[0] - fSkyFaceDim / 2.f, z_m_ctr[1] - fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(1, 1);	glVertex3f(z_m_ctr[0] + fSkyFaceDim / 2.f, z_m_ctr[1] - fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(1, 0);	glVertex3f(z_m_ctr[0] + fSkyFaceDim / 2.f, z_m_ctr[1] + fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(0, 0);	glVertex3f(z_m_ctr[0] - fSkyFaceDim / 2.f, z_m_ctr[1] + fSkyFaceDim / 2.f, z_m_ctr[2]);
	glEnd();

}

///

void resize(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}

static 
void destroy()
{	
	global_destroy();

	math_destroy();
	//
	unlinkShaders();
	unloadCubemapTex();

	unloadSkyBoxTex();

	//	DevIL finalization
	ilDeleteImages(1, &nCurrImg);

	exit(EXIT_SUCCESS);
}

static void renderBitmapString(float x, float y, void *font, char *string) 
{  
	char *c;
	glRasterPos2f(x,y);

	for (c = string; *c != '\0'; c++) 
	{
		glutBitmapCharacter(font, *c);
	}
}

static void printFPS()
{
	static clock_t nLastTick = 0;

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0, 1, 0, 1);

	glColor3f(1, 1, 0);

	glUseProgram(0);

	//	Calculate FPS
	clock_t nCurrTick = clock();

	char buf[20] = {0};
	sprintf(buf, "%.2f fps", 1000.f / (nCurrTick - nLastTick));

	glColor3f(1, 1, 0);
	renderBitmapString(0, 0.95, GLUT_BITMAP_TIMES_ROMAN_24, buf);
	
	glPopMatrix();

	nLastTick = nCurrTick;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glUseProgram(program);
}

void display()
{

	clock_t t0 = clock();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, 0);

	{
		//	Pass CamPos as eyePos into V-Shader
		GLint eyePos;
		eyePos = glGetUniformLocation(program, "eyepos");
		glUniform3f(eyePos, CamPos[0], CamPos[1], CamPos[2]);

		GLint fluidColorInx = glGetUniformLocation(program, "fluidColor");
		glUniform4f(fluidColorInx, 0.1, 0.3, 0.6, 1);

		///
		///		Set View
		///
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
	    	
		float fScope = 0.5;
		glFrustum(-fScope, fScope, -fScope, fScope, 1, 1000);
		gluLookAt( CamPos[0], CamPos[1], CamPos[2],
				   CamTarget[3], CamTarget[1], CamTarget[2],
				   CamUp[0], CamUp[1], CamUp[2]);

		glTranslatef(InitDist, InitDist * 0.8, InitDist);
	
		{
			glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv( GL_VIEWPORT, viewport );
		}

		///
		///		Render
		///
		resetGrid();

		//	Render Particles
		for(int i = 0; i< particles.size(); i ++)
		{
			particles[i].render();
		}

clock_t t1 = clock();

		//	get depth buffer of Particles
		//
		glFlush();
		glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer);

		///
		///		render SkyBox
		///
		{
			glUseProgram(0);
				
			/* Lighting Variables */
			GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 1.0 }; 
			GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
			GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
			GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

			glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);

			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
				renderSkyBox();				
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
			
			glUseProgram(program);
		}

		//	get depth buffer of the whole scene
		//
		glFlush();
		glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer1);

		glPopMatrix();
		
		//
		//	Processing Z-Buffer data		
		//
		genDepthPic(depth_buffer, depth_buffer1, depth_buffer, width, height);

clock_t t2 = clock();	

		//	Thickness
		projectAllParticles(particles, particles2d);
		calcAllThickness(thick_buffer, depth_buffer, width, height, particles2d);

clock_t t3 = clock();		

		//	Curvature Flow
		memset(cf_buf, 0, sizeof(float) * WinWidth * WinHeight);
		curvature_flow(depth_buffer, fCFFactor, width, height, nIterCount);

clock_t t4 = clock();		

		////	Normals
		calcAllNormals(normals, depth_buffer, width, height, nNormalsLerpStep);

clock_t t5 = clock();

		///
		///		Render using Particles
		///
		glMatrixMode(GL_PROJECTION);	
		glPushMatrix();

		glLoadIdentity();
		gluOrtho2D(0, 1, 0, 1);

		//	Fluid vertices
		//
		glUseProgram(program);

		glColor3f(1, 0, 0);
		glBegin(GL_POINTS);
		for(int j = 0; j < height; j ++)
		for(int i = 0; i < width; i ++)
		{
			float fd = *(depth_buffer + (i + j * width));
			if(fd > 0)
			{					
				glVertexAttrib1f(thickAttribInx, *(thick_buffer + (i + j * width)));
				glNormal3fv(normals + (i + j * width) * 3);
				glVertex2f(i * 1.f/ width, j * 1.f/ height);				
			}
		}
		glEnd();

		glPopMatrix();

clock_t t6 = clock();

		for(int i = 0; i < particles.size(); i ++)
		{
			particles[i].update();
		}
		
		clock_t t7 = clock();	
		clock_t nTotal = t7 - t0;
		printf(" - IO : %.2f, Thick : %.2f, CF : %.2f, Norm : %.2f, Vertex : %.2f, Physics : %.2f \n", 
				(t2 - t1) * 1.f / nTotal, (t3 - t2) * 1.f / nTotal, (t4 - t3) * 1.f / nTotal, 
				(t5 - t4) * 1.f / nTotal, (t6 - t5) * 1.f / nTotal, (t7 - t6) * 1.f / nTotal  );

	}	

	printFPS();

	glutSwapBuffers();
}

static 
void key(unsigned char key, int x, int y)
{		
    switch (key) 
    {
	case 'c':
	case 'C':
		//	Taking & Saving the screenshot				   
		if(ilutGLScreen())
		{					  
		  ilEnable(IL_FILE_OVERWRITE);
		  char buf[30] = {0};
		  sprintf(buf, "fl_%d.jpg", nCurrImgCount++);
		  if(ilSaveImage(buf))
		  {
			 printf("Screenshot saved successfully as \'%s\'!\n",buf);
		  }
		  else
		  {
			 printf("Sorry, DevIL cannot save your screenshot...\n");
		  }
		}
		else
		{
		  printf(" Sorry man, DevIL screenshot taking failed...\n");
		}
		break;

    case 27 : 
    case 'q':
        destroy();
        break;
    }

    glutPostRedisplay();
}

int iWinId;
void idle() 
{
	glutSetWindow(iWinId);
	glutPostRedisplay();
}

///
void callback_gpu_enable(int)
{

}

///	Reset the scene...
void callback_reset(int)
{
	global_destroy();
	global_init();

	particles.clear();
	Particle::genParticlesByRandom(ParCount, clock());
}

///
int main(int argc, char* argv[])
{
	//	Print Usage
	printf("-------------------\n");
	printf(" SPH by Tony Zhang\n");
	printf("-------------------\n");
	printf("Press 'C' to capture screenshot\n");
	printf("Press ESC to exit\n\n");

	//
	srand(clock());

	//	Window Setup
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);	
	
	glutInitWindowSize(WinWidth, WinHeight);
    glutInitWindowPosition(WinLeft, WinTop);
    iWinId = glutCreateWindow(WinTitle);
    
	glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
	glutIdleFunc(idle);

	//	Warning : these init calling has to be put
	//		      after window is created.

	//	Anti-Aliasing for Lines
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glClearColor(0.75, 0.75, 0.75, 0);

	//	Particle Init	
	Particle::genParticlesByRandom(ParCount, clock());	
	global_init();

	//	DevIL init
	//
	ilInit();
	ilutRenderer(ILUT_OPENGL);
	ilutEnable(ILUT_OPENGL_CONV);

	//	Cubemap Texture setup
	//
	math_init();

	loadCubemapTex("violentdays_large.jpg");

	loadSkyBoxTex("violentdays_large.jpg", skyTex);

	loadShader("shader/v_shader.txt", VERTEX);
	loadShader("shader/f_shader.txt", FRAGMENT);
	linkShaders();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, nCubemapTexId);
	cubeMapInx = glGetUniformLocation(program, "cubemapTex");
	glUniform1i(cubeMapInx, 0);

	ilGenImages(1, &nCurrImg);
	ilBindImage(nCurrImg);	

	///

	//	Put Barrier
	//float ctr[3] = {0, 15, 0};
	//Ball *pBall = new Ball(5, ctr);
	//barVec.push_back(pBall);

	//	GLUI
	GLUI *glui = GLUI_Master.create_glui( "Param Control", 0, WinWidth + 30, 20 );
	
	//	GPU setting
	//
	{
		GLUI_Panel *pPGpu = glui->add_panel("GPU Setup");
		GLUI_Checkbox *pPMGpuChk = glui->add_checkbox_to_panel(pPGpu, "GPU Enabled", &bGPUEnabled, -1, callback_gpu_enable);
	}

	{
		//	Particle Count
		GLUI_Spinner *pParCount = glui->add_spinner("Particle Count", GLUI_SPINNER_INT, &ParCount);
		pParCount->set_float_limits(10, 100000);
		pParCount->set_speed(10);

		//	Particle Radius	
		GLUI_Spinner *pPRad = glui->add_spinner("Particle Radius", GLUI_SPINNER_FLOAT, &ParRad);
		pPRad->set_int_limits(0.1, 9);
		pPRad->set_speed(0.5);

	}

	//	Rigid Body Part
	//
	{
		GLUI_Panel *pRBPal = glui->add_panel("Rigid Body Param");

		//	Velocity Factor
		GLUI_Spinner *pVelFac = glui->add_spinner_to_panel(pRBPal, "Velocity Factor", GLUI_SPINNER_FLOAT, &fVelFactor);
		pVelFac->set_int_limits(0.01, 2);
		pVelFac->set_speed(0.05);

		////	Gravity Factor
		//GLUI_Spinner *pGravFac = glui->add_spinner_to_panel(pRBPal, "Gravity Factor", GLUI_SPINNER_FLOAT, &fGravFactor);
		//pGravFac->set_int_limits(0.01, 2);
		//pGravFac->set_speed(0.05);

		//	Velocity Dissipation by Wall
		GLUI_Spinner *pVelDiss = glui->add_spinner_to_panel(pRBPal, "Vel-Diss by Wall", GLUI_SPINNER_FLOAT, &fVelDissByWall);
		pVelDiss->set_int_limits(0.01, 2);
		pVelDiss->set_speed(0.05);

		//	Rigid Spring Coefficient
		GLUI_Spinner *pRigSpring = glui->add_spinner_to_panel(pRBPal, "Rigid Spring Coefficient", GLUI_SPINNER_FLOAT, &fSpringCoe);
		pRigSpring->set_int_limits(0.01, 50);
		pRigSpring->set_speed(0.1);

		//	Rigid Damping Coefficient
		GLUI_Spinner *pRigDamping = glui->add_spinner_to_panel(pRBPal, "Rigid Damping Coefficient", GLUI_SPINNER_FLOAT, &fDampingCoe);
		pRigDamping->set_int_limits(0.01, 50);
		pRigDamping->set_speed(0.01);
	}
	
	//	Fluid Part
	//
	{
		GLUI_Panel *pFLPal = glui->add_panel("Fluid Param");

		//	Fluid Density Coefficient
		GLUI_Spinner *pDensity = glui->add_spinner_to_panel(pFLPal, "Fluid Density", GLUI_SPINNER_FLOAT, &fStdFluidDensity);
		pDensity->set_int_limits(0.01, 20);
		pDensity->set_speed(0.1);

		//	Fluid Density Factor
		GLUI_Spinner *pDenFactor = glui->add_spinner_to_panel(pFLPal, "Fluid Density Factor", GLUI_SPINNER_FLOAT, &fFluidDensityFactor);
		pDenFactor->set_int_limits(0.01, 20);
		pDenFactor->set_speed(0.1);

		//	Mass per Particle
		GLUI_Spinner *pMass = glui->add_spinner_to_panel(pFLPal, "Mass per Particle", GLUI_SPINNER_FLOAT, &fMassPerPar);
		pMass->set_int_limits(0.01, 20);
		pMass->set_speed(0.1);
		
		////	Grid Count Coefficient
		//GLUI_Spinner *pGridCount = glui->add_spinner_to_panel(pFLPal, "Grid Count Per Dim", GLUI_SPINNER_INT, &GridCountPerDim);
		//pGridCount->set_int_limits(1, 1000);
		//pGridCount->set_speed(2);
		
		//	kPress
		GLUI_Spinner *pPress = glui->add_spinner_to_panel(pFLPal, "Pressure Factor", GLUI_SPINNER_FLOAT, &kPress);
		pPress->set_int_limits(0.0001, 20);
		pPress->set_speed(0.05);

		//	kVisco
		GLUI_Spinner *pVisco = glui->add_spinner_to_panel(pFLPal, "Viscosity Factor", GLUI_SPINNER_FLOAT, &kVisco);
		pVisco->set_int_limits(0.0001, 20);
		pVisco->set_speed(0.05);
	}
	glui->add_button("Reset", 0, callback_reset);

	GLUI_Master.set_glutIdleFunc(idle);

	///
	atexit(destroy);

	//
	glutMainLoop();

	destroy();
	return EXIT_SUCCESS;
}

