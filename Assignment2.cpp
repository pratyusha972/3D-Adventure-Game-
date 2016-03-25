#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define DEG2RAD(p) p*(6.28/360)
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>


using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
	GLuint TexMatrixID;
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			2,                  // attribute 2. Textures
			2,                  // size (s,t)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}

/**************************
 * Customizable functions *
 **************************/


float x = 0,y=0,z=0,tilesy = 0,uptiles = 1,downtiles = 0,heliview = 0;
int upview = 0,towerview = 0,advenview = 0,followview=0,followangle = 0,advenangle=0;
int a[11][11],b[11][11],times = 0,timesa = 0,count =0;
int lifes = 0,win = 0,score = 0;
int levelchange = 1,rightjump = 0,leftjump=0,upjump=0,downjump=0,spaceflag=0;
int playerleft = 0,playerright=0,playerup=0,playerdown=0;
float eyex = (-100)*cos(75*M_PI/180.0f),eyey = 270,eyez = 400*sin(75*M_PI/180.0f),targetx = 300,targety = 0,targetz = 150;
float upy = 1,zoom = 1;
double start_time = 0,curr_time =0,starttime=0,currenttime = 0;
int speedfactor = 4,won=0,lost=0;
int countright = 0,countleft = 0,countup = 0,countdown = 0,countrightjump = 0,countleftjump = 0,countupjump = 0,countdownjump = 0;
int freflag = 0;
VAO *rect1,*rect2,*rect3,*rect4,*rect5,*rect6,*back;
int timesppp = 0,die = 0;
class player
{
	public:
		float x;
		float y;
		float z;
		int i;
		int j;

	public:
		void position()
		{
			if(die == 1)
			{
				die = 0;
				x = 7.5;
				z = 7.5;
				y = 105;
				i = 0;
				j = 0;
				lifes = lifes+1;
			}
			starttime = starttime+0.1;
			float t = starttime;
			//if(y >= 105)
			//{
			if(playerright == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0 ) || (a[i+1][j]==2 and a[i][j]==2) || (a[i+1][j]==1 and a[i][j]==2)) && (a[i+1][j]!=2 || (a[i+1][j]==2 && tilesy <=0) || (a[i+1][j]==2 and a[i][j]==2)))
			{
				if(a[i+1][j]==2 and tilesy <= -30 and a[i][j]!=2)
				{
						y = tilesy;
						i = i+1;
						z = z+30;
						playerright = 0;
						die = 1;						
				}
				else
				{
				z = z+speedfactor;
				countright++;

				//cout << countright << endl;
				if(countright*speedfactor >= 30)
				{
					playerright = 0;

					z = z-(((countright)*speedfactor)%30);
					countright = 0;
					y = 105;

					//cout << z << endl;
					i++;
				}
			}
				//	cout << "right" << i<< endl; 
			}

			else if(playerleft == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i-1][j]==2 and a[i][j]==2) || (a[i-1][j]==1 and a[i][j]==2)) && (a[i-1][j]!=2 || (a[i-1][j]==2 && tilesy <=0) || (a[i-1][j]==2 and a[i][j]==2)))
			{
				if(a[i-1][j]==2 and tilesy <= -30 and a[i][j]!=2)
				{
						y = tilesy;
						i = i-1;
						z = z-30;
						playerleft = 0;
						die = 1;						
				}
				else
				{
				z = z-speedfactor;
				countleft++;

				if(speedfactor*countleft >= 30)
				{

					playerleft = 0;
					z = z+(((countleft)*speedfactor)%30);
					i--;
					y = 105;
					countleft = 0;

				}
			}
			}
			else if(playerup == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i][j+1]==2 and a[i][j]==2) || (a[i][j+1]==1 and a[i][j]==2)) && ((a[i][j+1]==2 and a[i][j]==2) || (a[i][j+1]==2 && tilesy <=0) || a[i][j+1]!=2 ))// || (a[i][j]==2 and a[i][j+1]==2)))
			{
				if(a[i][j+1]==2 and tilesy <= -30 and a[i][j]!=2)
				{
						y = tilesy;
						j = j+1;
						x = x+30;
						playerup = 0;
						die = 1;						
				}
				else
				{
				//cout << "yaycudcuscfs" << endl;
				x=x+speedfactor;
				countup++;
				if(countup*speedfactor >= 30)
				{playerup = 0;
					x = x-(((countup)*speedfactor)%30);
					countup = 0;
					y = 105;
					j++;
				}
			}
			}
			else if(playerdown == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i][j-1]==2 and a[i][j]==2) || (a[i-1][j]==1 and a[i][j]==2)) && (a[i][j-1]!=2 || (a[i][j-1]==2 && tilesy <=0) || (a[i][j-1]==2 and a[i][j]==2))) 
			{
				if(a[i][j-1]==2 and tilesy <= -30 and a[i][j]!=2)
				{
						y = tilesy;
						j = j-1;
						x = x-30;
						playerdown = 0;
						die = 1;						
				}
				else
				{
				x = x-speedfactor;
				countdown++;
				if(countdown*speedfactor >= 30)
				{
					playerdown = 0;

					x = x+(((countdown)*speedfactor)%30);
					countdown = 0;
					y = 105;
					j--;

				}
			}
			}

			else if(rightjump == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i+2][j]==2 and a[i][j]==2) || (a[i+2][j]==1 and a[i][j]==2)))
			{
				if(a[i+1][j]==2)
				{
					//	cout << "yes" << endl;
					if(tilesy >= 0)
					{
						y = tilesy;
						i = i+1;
						z = z+30;
						rightjump = 0;
					}
					else if(tilesy < 0 && tilesy >= -50 || timesppp == 1)
					{
						timesppp = 1;
						z = z+2;
						countrightjump++;
						y = 105 + 20*(t) - 5 * (t) * (t);
						//cout << y << endl;

						if(countrightjump >= 30)
						{
							timesppp = 0;
							rightjump = 0;
							countrightjump = 0;
							y = 105;
							i = i+2;
						}
					}
				}
				else if(a[i+2][j]==2)
				{
					//cout << "yayyyy" << endl;
					if(tilesy >= 0)
					{
						y = 105;
						i = i+1;
						z = z+30;
						rightjump = 0;

					}
					else if(tilesy < 0 && tilesy > -30)
					{
						y = tilesy;
						i = i+2;
						z = z+60;
						rightjump = 0;
					}
					else if(tilesy <= -30)
					{
						y = tilesy;
						i = i+2;
						z = z+60;
						rightjump = 0;
						die = 1;						
					}
				}
				/*else if(a[i][j] == 2 and a[i+2][j]==2)
				{
					if(a[i+1][j] == 1)
					{
						//cout << "yaes" << endl;
						z = z+2;
						countrightjump++;
						y = tilesy + 105 + 20*t - 5 * t * t;

						if(countrightjump >= 30)
						{
							countrightjump = 0;
							rightjump = 0;
							i = i+2;

						}
					}
					else
					{
						rightjump = 0;
					}
				}*/
				else
				{
					z = z+2;
					countrightjump++;

					y = 105 + 20 * t - 5 * t * t;

					if(countrightjump >= 30)
					{
						countrightjump = 0;
						rightjump = 0;
						y = 105;
						i=i+2;

					}
				}
			}
			else if(leftjump == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i-2][j]==2 and a[i][j]==2) || (a[i-2][j]==1 and a[i][j]==2)))
			{
				if(a[i-1][j]==2)
				{
					//	cout << "yes" << endl;
					if(tilesy >= 0)
					{
						y = tilesy;
						i = i-1;
						z = z-30;
						leftjump = 0;
					}
					else if(tilesy < 0 && tilesy >= -50 || timesppp == 1)
					{
						timesppp = 1;
						z = z-2;
						countleftjump++;
						y = 105 + 20*(t) - 5 * (t) * (t);
						//cout << y << endl;

						if(countleftjump >= 30)
						{
							timesppp = 0;
							leftjump = 0;
							countleftjump = 0;
							y = 105;
							i = i-2;
						}
					}
				}
				else if(a[i-2][j]==2)
				{
					//cout << "yayyyy" << endl;
					if(tilesy >= 0)
					{
						y = 105;
						i = i-1;
						z = z-30;
						leftjump = 0;
					}
					if(tilesy < 0 && tilesy >= -30)
					{
						y = tilesy;
						i = i-2;
						z = z-60;
						leftjump = 0;
					}
					else if(tilesy <= -30)
					{
						y = tilesy;
						i = i-2;
						z = z-60;
						leftjump = 0;
						die = 1;

					}
				}
				/*else if(a[i][j] == 2 and a[i-2][j]==2)
				{
					if(a[i-1][j] == 1)
					{
						//cout << "yaes" << endl;
						z = z-2;
						countleftjump++;
						y = tilesy + 105 + 20*t - 5 * t * t;

						if(countleftjump >= 30)
						{
							countleftjump = 0;
							leftjump = 0;
							i = i-2;

						}
					}
					else
					{
						leftjump = 0;
					}
				}*/
				else{
					z = z-2;
					countleftjump++;
					y = 105 + 20*t - 5 * t * t;
					//cout << y << endl;

					if(countleftjump >= 30)
					{
						countleftjump = 0;
						leftjump = 0;
						y = 105;
						i=i-2;

					}
				}
			}
			else if(upjump == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i][j+2]==2 and a[i][j]==2) || (a[i][j+2]==1 and a[i][j]==2)))
			{
				if(a[i][j+1]==2)
				{
					//	cout << "yes" << endl;
					if(tilesy >= 0)
					{
						y = tilesy;
						j = j+1;
						x = x+30;
						upjump = 0;
					}
					else if(tilesy < 0 && tilesy >= -50 || timesppp == 1)
					{
						timesppp = 1;
						x = x+2;
						countupjump++;
						y = 105 + 20*(t) - 5 * (t) * (t);
						//cout << y << endl;

						if(countupjump >= 30)
						{
							timesppp = 0;
							upjump = 0;
							countupjump = 0;
							y = 105;
							j = j+2;
						}
					}
				}
				else if(a[i][j+2]==2)
				{
					//cout << "yayyyy" << endl;
					if(tilesy >= 0)
					{
						y = 105;
						j = j+1;
						x = x+30;
						upjump = 0;


					}
					else if(tilesy < 0 && tilesy > -30)
					{
						y = tilesy;
						j = j+2;
						x = x+60;
						upjump = 0;
					}
					else if(tilesy <= -30)
					{
						y = tilesy;
						j = j+2;
						x = x+60;
						upjump = 0;
						die = 1;

					}
				}
				/*else if(a[i][j] == 2 and a[i][j+2]==2)
				{
					if(a[i][j+1] == 1)
					{
						//cout << "yaes" << endl;
						x = x+2;
						countupjump++;
						y = tilesy + 105 + 20*t - 5 * t * t;

						if(countupjump >= 30)
						{
							countupjump = 0;
							upjump = 0;
							j = j+2;

						}
					}
					else
					{
						upjump = 0;
					}
				}*/
				else
				{
					x = x+2;
					countupjump++;
					y = 105 + 20*(t) - 5 * (t) * (t);
					//cout << y << endl;

					if(countupjump >= 30)
					{
						upjump = 0;
						countupjump = 0;
						y = 105;
						j = j+2;

					}

				}
			}

			else if(downjump == 1 && (a[i][j]!=2 || (a[i][j]==2 && tilesy >= 0) || (a[i][j-2]==2 and a[i][j]==2) || (a[i][j+2]==1 and a[i][j]==2)))
			{
				if(a[i][j-1]==2)
				{
					//	cout << "yes" << endl;
					if(tilesy >= 0)
					{
						y = tilesy;
						j = j-1;
						x = x-30;
						downjump = 0;
					}
					else if(tilesy < 0 && tilesy >= -50 || timesppp == 1)
					{
						timesppp = 1;
						x = x-2;
						countdownjump++;
						y = 105 + 20*(t) - 5 * (t) * (t);
						//cout << y << endl;

						if(countdownjump >= 30)
						{
							timesppp = 0;
							downjump = 0;
							countdownjump = 0;
							y = 105;
							j = j-2;
						}
					}
				}
				else if(a[i][j-2]==2)
				{
					//cout << "yayyyy" << endl;
					if(tilesy >= 0)
					{
						y = 105;
						j = j-1;
						x = x-30;
						downjump = 0;

					}
					else if(tilesy < 0 && tilesy > -30)
					{
						y = tilesy;
						j = j-2;
						x = x-60;
						downjump = 0;
					}
					else if(tilesy <= -30)
					{
						y = tilesy;
						j = j-2;
						x = x-60;
						downjump = 0;
						die = 1;

					}
				}
				/*else if(a[i][j] == 2 and a[i][j-2]==2)
				{
					if(a[i][j-1] == 1)
					{
						//cout << "yaes" << endl;
						x = x-2;
						countdownjump++;
						y = tilesy + 105 + 20*t - 5 * t * t;

						if(countdownjump >= 30)
						{
							countdownjump = 0;
							downjump = 0;
							j = j-2;

						}
					}
					else
					{
						downjump = 0;
					}
				}*/

				else
				{

					x = x-2;
					countdownjump++;
					y = 105 + 20*t - 5 * t * t;

					if(countdownjump >= 30)
					{
						countdownjump = 0;
						downjump = 0;
						j = j-2;

					}
				}

			}
			else
			{
				playerdown = 0;
				playerleft = 0;
				playerright = 0;
				playerup = 0;
				upjump = 0;
				downjump = 0;
				leftjump = 0;
				rightjump = 0;
			}
			if(advenview == 1)
			{
				if(advenangle == 90)
				{
					eyex = x+13;
					eyey = y+30;
					eyez = z+7.5;
					targetx = x+30;
					targety = y+20;
					targetz = z+7.5;
					//	cout << "yes0" << endl;
				}
				else if(advenangle == 180)
				{
					eyex = x+7.5;
					eyey = y+30;
					eyez = z+2;
					targetx = x+7.5;
					targety = y+20;
					targetz = z-14.5;	
					//	cout << "yes180" << endl;			
				}
				else if(advenangle==270)
				{
					eyex = x+2;
					eyey = y+30;
					eyez = z+7.5;
					targetx = x-14.5;
					targety = y+20;
					targetz = z+7.5;
					//	cout << "yes270" << endl;
				}					
				else if(advenangle == 360 || timesa == 1)
				{
					timesa = 1;
					advenangle = 0;
					eyex = x+7.5;
					eyey = y+30;
					eyez = z+13;
					targetx = x+7.5;
					targety = y+20;
					targetz = z+30;	
					//	cout << "yes360" << endl;				
				}

			}
			else if(followview == 1)
			{
				if(followangle == 90)
				{
					eyex = x-7.5;
					eyey = y+35;
					eyez = z+7.5;
					targetx = x;
					targety = y+25;
					targetz = z+7.5;
				}
				else if(followangle == 180)
				{
					eyex = x+7.5;
					eyey = y+35;
					eyez = z+22.5;
					//changed 22.5
					targetx = x+7.5;
					targety = y+25;
					targetz = z+15;					
				}
				else if(followangle == 270)
				{
					eyex = x+22.5;
					eyey = y+35;
					eyez = z+7.5;
					//changed 22.5
					targetx = x+15;
					targety = y+25;
					targetz = z+7.5;
				}					
				else if(followangle == 360 || times == 1)
				{
					times = 1;
					followangle = 0;
					//changed 22.5
					eyex = x+7.5;
					eyey = y+35;
					eyez = z-7.5;
					targetx = x+7.5;
					targety = y+25;
					targetz = z;					
				}
			}

			//cout << i << " i " << "j " << j << '\n';
		}
		void checkdown()
		{
			if(a[i][j]==1)
			{
				x = 7.5;
				z = 7.5;
				i = 0;
				j = 0;
				lifes = lifes+1;
			}
		}
		void checkcollision()
		{
			if(b[i][j]==1)
			{
				x = 7.5;
				z = 7.5;
				i = 0;
				j = 0;
				lifes = lifes+1;
			}
		}
		void checkboundary()
		{
			if(i<0 || i>=10 || j<0 || j>=10)
			{
				x = 7.5;
				z = 7.5;
				i = 0;
				j = 0;
				lifes = lifes+1;
			}				
		}
		void checkwin()
		{
			if(i==9 and j==9)
			{
				win = 1;
			}	
		}


		void checksliding()
		{
			if(a[i][j]==2)
			{
				//if(tilesy >= -25)
				y = tilesy+105;
				/*else
				  {
				  x = 7.5;
				  z = 7.5;
				  i = 0;
				  j = 0;
				  lifes = lifes+1;
				  }*/
			}
		}


			public:
		VAO* createCube(float x,float y,float z)
		{
			const  GLfloat vertex_buffer_data [] = {
				0,0,0,
				x,0,0,
				x,y,0,

				x,y,0,
				0,y,0,
				0,0,0,

				0,0,0,
				0,y,0,
				0,y,z,

				0,y,z,
				0,0,z,
				0,0,0,

				0,y,0,
				0,y,z,
				x,y,z,

				x,y,z,
				x,y,0,
				0,y,0,

				0,0,0,
				0,0,z,
				x,0,z,

				x,0,z,
				x,0,0,
				0,0,0,

				0,0,z,
				x,0,z,
				x,y,z,

				x,y,z,
				0,y,z,
				0,0,z,

				x,0,z,
				x,0,0,
				x,y,0,

				x,y,0,
				x,y,z,
				x,0,z
			};

			const GLfloat color_buffer_data [] = {

				0,0.6,0.34,
				/*0,1,1,
				  0,1,1,

				  0,1,1,
				  0,1,1,
				  0,1,1,


				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,
				  0,1,1,*/

			};
			return create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

		}

		}user;	

		class enemy
		{
			public:
				float rad;
				float color1;
			public:
				VAO* createCircle()
				{
					GLfloat* vertex_buffer_data = new GLfloat [3*360];
					GLfloat* color_buffer_data = new GLfloat [3*360];
					for(int i=0;i<360;i++)
					{
						vertex_buffer_data [3*i] = (rad * cos(DEG2RAD(i)));
						vertex_buffer_data [3*i + 1] = (rad * sin(DEG2RAD(i)));
						vertex_buffer_data [3*i + 2] = 1;
						if(i%2==0)
						{
							color_buffer_data [3*i] =0.239;	
							color_buffer_data [3*i + 1] = 0.239;
							color_buffer_data [3*i + 2] = 0.239;
						}
						else
						{
							color_buffer_data [3*i] =0.653;	
							color_buffer_data [3*i + 1] = 0.245;
							color_buffer_data [3*i + 2] = 0.587;
						}
					}
					return create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
				}
		}obstacle;

		/* Executed when a regular key is pressed/released/held-down */
		/* Prefered for Keyboard events */
		void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			// Function is called first on GLFW_PRESS.

			if (action == (GLFW_PRESS || GLFW_REPEAT)) {
				switch (key) {
					case GLFW_KEY_U:
						//u for upview
						//upview = 1;
						//towerview = 0;
						//playerview = 0;
						advenview = 0;
						followangle = 0;
						followview = 0;
						timesa = 0;
						times = 0;
						advenangle =0;
						eyex = 149.999;
						eyey = 300;
						eyez = 150;
						targetx = 150;
						targety = 0;
						targetz = 150;
						break;
					case GLFW_KEY_SPACE:
						//cout << "yayyyy" << '\n';
						starttime = 0;
						spaceflag = 1;
						break;
					case GLFW_KEY_T:
						//t for tower view
						//towerview= 1;
						//upview = 0;
						//playerview = 0;
						followangle = 0;
						advenview = 0;
						advenangle = 0;
						followview = 0;
						timesa = 0;
						times = 0;
						eyex = -25;
						eyey = 270;
						eyez = 386.3;
						targetx = 300;
						targety = 0;
						targetz = 150;
						break;
					case GLFW_KEY_A:
						//a for adventure view
						advenview=1;
						followview = 0;
						followangle = 0;
						advenangle += 90;
						timesa = 0;	
						times = 0;
						//towerview=0;
						//upview = 0;
						/*eyex = user.x+15;
						  eyey = user.y+15;
						  eyez = user.z+15;
						  targetx = user.x+25;
						  targety = user.y;
						  targetz = user.z+25;*/
						break;
					case GLFW_KEY_B:
						//f for followcamview
						followview = 1;
						advenview = 0;
						advenangle = 0;
						times = 0;
						timesa = 0;
						followangle += 90;
						/*eyex = user.x-7.5;
						  eyey = user.y+25;
						  eyez = user.z-7.5;
						  targetx = user.x;
						  targety = user.y+5;
						  targetz = user.z;*/
						break;
					case GLFW_KEY_F:
						if(speedfactor <= 10)
							speedfactor += 1;
						break;
					case GLFW_KEY_S:
						if(speedfactor >= 1)
							speedfactor -= 1;
						break;
					case GLFW_KEY_N:
						won = 0;
						lost = 0;
						user.x = 7.5;

						user.y = 105;
						user.z = 7.5;	
						user.i = 0;
						user.j = 0;
						for(int lll = 0;lll <= 10;lll++)
						{
							for(int kkk = 0;kkk <= 10;kkk++)
							{
								a[lll][kkk]=0;
								b[lll][kkk]=0;
							}
						}
						levelchange = 1;
						lifes = 0;
						score = 0;
						count = 0;
						break;
					case GLFW_KEY_UP:
						if(followangle == 180 || advenangle == 180)
						{
							if(spaceflag==1)
							{	
								leftjump=1;
								spaceflag=0;
							}
							else
								playerleft = 1;
						}
						else if(followangle == 270 || advenangle == 270)
						{
							if(spaceflag==1)
							{	
								downjump=1;
								spaceflag=0;
							}
							else
								playerdown = 1;
						}
						else if(followangle == 360 || times == 1 || advenangle == 360 || timesa == 1){
							if(spaceflag==1)
							{	
								rightjump=1;
								spaceflag=0;
							}
							else
								playerright = 1;	
						}
						else
						{
							if(spaceflag==1)
							{	
								upjump=1;
								spaceflag=0;
							}
							else
								playerup = 1;
						}//}

				break;
				case GLFW_KEY_DOWN:
				//if(user.y>= 105)
				//{
				if(followangle == 180 || advenangle == 180)
				{
					if(spaceflag==1)
					{	
						rightjump=1;
						spaceflag=0;
					}
					else
						playerright = 1;
				}
				else if(followangle == 270 || advenangle == 270)
				{
					if(spaceflag==1)
					{	
						upjump=1;
						spaceflag=0;
					}
					else
						playerup = 1;
				}
				else if(followangle == 360 || times == 1 || advenangle == 360 || timesa == 1){
					if(spaceflag==1)
					{	
						leftjump=1;
						spaceflag=0;
					}
					else
						playerleft = 1;	
				}
				else
				{
					if(spaceflag==1)
					{	
						downjump=1;
						spaceflag=0;
					}
					else
						playerdown = 1;
				}

				break;
				case GLFW_KEY_LEFT:
				//if(user.y >= 105)
				//{
				if(followangle == 180 || advenangle == 180)
				{
					if(spaceflag==1)
					{	
						downjump=1;
						spaceflag=0;
					}
					else
						playerdown = 1;
				}
				else if(followangle == 270 || advenangle == 270)
				{
					if(spaceflag==1)
					{	
						rightjump=1;
						spaceflag=0;
					}
					else
						playerright = 1;
				}
				else if(followangle == 360 || times == 1 || advenangle == 360 || timesa == 1){
					if(spaceflag==1)
					{	
						upjump=1;
						spaceflag=0;
					}
					else
						playerup = 1;	
				}
				else
				{
					if(spaceflag==1)
					{	
						leftjump=1;
						spaceflag=0;
					}
					else
						playerleft = 1;
				}

				//	}
				break;
				case GLFW_KEY_RIGHT:
				//	if(user.y >= 105)
				//{
				//	if(followview == 1)
				//	{
				if(followangle == 180 || advenangle == 180)
				{
					if(spaceflag==1)
					{	
						upjump=1;
						spaceflag=0;
					}
					else
						playerup = 1;
				}
				else if(followangle == 270 || advenangle == 270)
				{
					if(spaceflag==1)
					{	
						leftjump=1;
						spaceflag=0;
					}
					else
						playerleft = 1;
				}
				else if(followangle == 360 || times == 1 || advenangle == 360 || timesa == 1){
					if(spaceflag==1)
					{	
						downjump=1;
						spaceflag=0;
					}
					else
						playerdown = 1;	
				}
				else
				{
					if(spaceflag==1)
					{	
						rightjump=1;
						spaceflag=0;
					}
					else
						playerright = 1;
				}
				break;		
				default:
				break;
			}
			}
			else if (action == GLFW_RELEASE) {
				switch (key) {
					case GLFW_KEY_ESCAPE:
						quit(window);
						break;
					case GLFW_KEY_E:
						quit(window);
						break;
					default:
						break;
				}
			}
			}

			/* Executed for character input (like in text boxes) */
			void keyboardChar (GLFWwindow* window, unsigned int key)
			{
				switch (key) {
					case 'Q':
					case 'q':
						quit(window);
						break;
					default:
						break;
				}
			}

			/* Executed when a mouse button is pressed/released */
			void mouseButton (GLFWwindow* window, int button, int action, int mods)
			{
				switch (button) {
					case GLFW_MOUSE_BUTTON_LEFT:
						if (action == GLFW_PRESS || action == GLFW_REPEAT)
							heliview = 1;
						else if(action == GLFW_RELEASE)
							heliview = 0;
						break;
					case GLFW_MOUSE_BUTTON_RIGHT:
						if (action == GLFW_RELEASE) {

						}
						break;
					default:
						break;
				}
			}

			void scrollback(GLFWwindow* window,double x,double y)
			{
				float add = float(y)/10;
				zoom = zoom+add;
				if(zoom >= 0.9 && zoom < 1.5)
				{
					if(add > 0)
					{
						eyex = eyex/zoom;
						eyey = eyey/zoom;
						eyez = eyez/zoom;
						targetx = targetx/zoom;
						targety = targety/zoom;
						targetz = targetz/zoom;
						//cout << zoom << endl;
					}
					else if(add < 0)
					{
						eyex = eyex*zoom;
						eyey = eyey*zoom;
						eyez = eyez*zoom;
						targetx = targetx*zoom;
						targety = targety*zoom;
						targetz = targetz*zoom;
						//cout << zoom << endl;
					}
				}
				//cout << zoom << endl;		
			}

			void mouse(GLFWwindow* window,double x,double y)
			{
				if(heliview == 1)
				{
					eyex = y - 150;
					eyez = x - 150;
					targetx = eyex/1.5;
					targety = eyey/1.5;
					targetz = eyez/1.5;

				}
			}

			/* Executed when window is resized to 'width' and 'height' */
			/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
			void reshapeWindow (GLFWwindow* window, int width, int height)
			{
				int fbwidth=width, fbheight=height;
				/* With Retina display on Mac OS X, GLFW's FramebufferSize
				   is different from WindowSize */
				glfwGetFramebufferSize(window, &fbwidth, &fbheight);

				GLfloat fov = 90.0f;

				// sets the viewport of openGL renderer
				glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);
				/* glMatrixMode (GL_PROJECTION);
				   glLoadIdentity ();
				   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
				// Store the projection matrix in a variable for future use
				// Perspective projection for 3D views
				Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

				// Ortho projection for 2D views
				//Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
			}

			VAO *triangle, *rectangle;
			VAO *cube,*cubetest,*obstacleex;

			VAO* createCube()
			{
				x = 30;
				y = 100;
				z = 30;
				const  GLfloat vertex_buffer_data [] = {

					0,0,0,
					x,0,0,
					x,y,0,

					x,y,0,
					0,y,0,
					0,0,0,

					0,0,0,
					0,y,0,
					0,y,z,

					0,y,z,
					0,0,z,
					0,0,0,

					0,y,0,
					0,y,z,
					x,y,z,

					x,y,z,
					x,y,0,
					0,y,0,

					0,0,0,
					0,0,z,
					x,0,z,

					x,0,z,
					x,0,0,
					0,0,0,

					0,0,z,
					x,0,z,
					x,y,z,

					x,y,z,
					0,y,z,
					0,0,z,

					x,0,z,
					x,0,0,
					x,y,0,

					x,y,0,
					x,y,z,
					x,0,z
				};

				const GLfloat color_buffer_data [] = {
					0.700f,  0.f,  0.f,

					0.195f,  0.548f,  0.859f,

					0.014f,  0.184f,  0.576f,

					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,

					0.609f,  0.115f,  0.436f,

					0.327f,  0.483f,  0.844f,

					0.310f,  0.747f,  0.185f,

					/*0.876f,  0.177f,  0.433f,

					  0.971f,  0.572f,  0.833f,

					  0.140f,  0.616f,  0.489f,

					  0.983f,  0.996f,  0.189f,

					  0.771f,  0.828f,  0.570f,

					  0.706f,  0.315f,  0.916f,*/

					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,

					/*	0.997f,  0.513f,  0.064f,

						0.945f,  0.719f,  0.592f,

						0.543f,  0.021f,  0.978f,

						0.279f,  0.317f,  0.505f,

						0.167f,  0.620f,  0.077f,

						0.783f,  0.290f,  0.734f,
					 */
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,

					0.722f,  0.645f,  0.174f,

					0.302f,  0.455f,  0.848f,

					0.225f,  0.587f,  0.040f,

					0.517f,  0.713f,  0.338f,

					0.053f,  0.459f,  0.120f,

					0.347f,  0.257f,  0.137f,

					0.655f,  0.653f,  0.042f,

					0.714f,  0.505f,  0.345f,

					0.393f,  0.621f,  0.862f,

					0.673f,  0.211f,  0.457f,

					0.520f,  0.583f,  0.171f,

					0.982f,  0.099f,  0.879f



				};

				//	const GLfloat texture_buffer_data [] = {

				/*0,0,0,
				  x,0,0,
				  x,y,0,

				  x,y,0,
				  0,y,0,
				  0,0,0,

				  0,0,0,
				  0,y,0,
				  0,y,z,

				  0,y,z,
				  0,0,z,
				  0,0,0,

				  0,y,0,
				  0,y,z,
				  x,y,z,

				  x,y,z,
				  x,y,0,
				  0,y,0,

				  0,0,0,
				  0,0,z,
				  x,0,z,

				  x,0,z,
				  x,0,0,
				  0,0,0,

				  0,0,z,
				  x,0,z,
				  x,y,z,

				  x,y,z,
				  0,y,z,
				  0,0,z,

				  x,0,z,
				  x,0,0,
				  x,y,0,

				  x,y,0,
				  x,y,z,
				  x,0,z*/
				/*	0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1,  // TexCoord 1 - bot left


					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1,  // TexCoord 1 - bot left

					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1,  // TexCoord 1 - bot left

					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1,  // TexCoord 1 - bot left
				//


				0,1, // TexCoord 1 - bot left
				1,1, // TexCoord 2 - bot right
				1,0, // TexCoord 3 - top right

				1,0, // TexCoord 3 - top right
				0,0, // TexCoord 4 - top left
				0,1,  // TexCoord 1 - bot left

				0,1, // TexCoord 1 - bot left
				1,1, // TexCoord 2 - bot right
				1,0, // TexCoord 3 - top right

				1,0, // TexCoord 3 - top right
				0,0, // TexCoord 4 - top left
				0,1, // TexCoord 1 - bot left 

				};*/
				return create3DObject(GL_TRIANGLES, 36, vertex_buffer_data,color_buffer_data, GL_FILL);
			}
			// Creates the rectangle object used in this sample code


			VAO* createRectangleUP (GLuint textureID)
			{
				x = 30;
				y = 100;
				z = 30;
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,0,0,
					x,0,0,
					x,y,0,

					x,y,0,
					0,y,0,
					0,0,0,
				};

				static const GLfloat color_buffer_data [] = {
					0.700f,  0.f,  0.f,

					0.195f,  0.548f,  0.859f,

					0.014f,  0.184f,  0.576f,

					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,

				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangleDown (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,0,0,
					0,y,0,
					0,y,z,

					0,y,z,
					0,0,z,
					0,0,0,
				};

				static const GLfloat color_buffer_data [] = {
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,
					0.8039,0.5215,0.2470,

					0.609f,  0.115f,  0.436f,

					0.327f,  0.483f,  0.844f,

					0.310f,  0.747f,  0.185f,


				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangleLeft (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,y,0,
					0,y,z,
					x,y,z,

					x,y,z,
					x,y,0,
					0,y,0,
				};

				static const GLfloat color_buffer_data [] = {
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239,
					0.239,0.239,0.239

				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangleRight (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,0,0,
					0,0,z,
					x,0,z,

					x,0,z,
					x,0,0,
					0,0,0,
				};

				static const GLfloat color_buffer_data [] = {

					0.639,0.639,0.639,
					0.639,0.639,0.639,
					0.639,0.639,0.639,
					0.639,0.639,0.639,
					0.639,0.639,0.639,
					0.639,0.639,0.639

				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangleFront (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,0,z,
					x,0,z,
					x,y,z,

					x,y,z,
					0,y,z,
					0,0,z,


				};

				static const GLfloat color_buffer_data [] = {
					0.722f,  0.645f,  0.174f,

					0.302f,  0.455f,  0.848f,

					0.225f,  0.587f,  0.040f,

					0.517f,  0.713f,  0.338f,

					0.053f,  0.459f,  0.120f,

					0.347f,  0.257f,  0.137f,


				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangleBack (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					x,0,z,
					x,0,0,
					x,y,0,

					x,y,0,
					x,y,z,
					x,0,z
				};

				static const GLfloat color_buffer_data [] = {
					0.655f,  0.653f,  0.042f,

					0.714f,  0.505f,  0.345f,

					0.393f,  0.621f,  0.862f,

					0.673f,  0.211f,  0.457f,

					0.520f,  0.583f,  0.171f,

					0.982f,  0.099f,  0.879f
				};

				// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};

				// create3DTexturedObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
			}

			VAO* createRectangle (GLuint textureID)
			{
				// GL3 accepts only Triangles. Quads are not supported
				static const GLfloat vertex_buffer_data [] = {
					0,0,0, // vertex 1
					10000,0,0, // vertex 2
					10000,0,10000, // vertex 3

					10000,0,10000, // vertex 3
					0,0,10000, // vertex 4
					0,0,0 // vertex 1
				};

				static const GLfloat color_buffer_data [] = {
					0,1,1, // color 1
					0,1,1, // color 2
					0,1,1, // color 3

					0,1,1, // color 3
					0,1,1, // color 4
					0,1,1  // color 1
				};
				static const GLfloat texture_buffer_data [] = {
					0,1, // TexCoord 1 - bot left
					1,1, // TexCoord 2 - bot right
					1,0, // TexCoord 3 - top right

					1,0, // TexCoord 3 - top right
					0,0, // TexCoord 4 - top left
					0,1  // TexCoord 1 - bot left
				};
				// create3DObject creates and returns a handle to a VAO that can be used later
				return create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data ,textureID, GL_FILL);
			}

			float camera_rotation_angle = 75;
			float rectangle_rotation = 0;
			float obstacle_rotation = 0;

			/* Render the scene with openGL */
			/* Edit this function according to your assignment */
			void draw ()
			{
				user.checkwin();

				// clear the color and depth in the frame buffer
				glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// use the loaded shader program
				// Don't change unless you know what you are doing
				glUseProgram (programID);
				// Target - Where is the camera looking at.  Don't change unless you are sure!!
				// Eye - Location of camera. Don't change unless you are sure!!
				glm::vec3 eye (eyex, eyey, eyez);
				// Target - Where is the camera looking at.  Don't change unless you are sure!!
				glm::vec3 target (targetx,targety, targetz);
				// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
				glm::vec3 up (0, 1, 0);

				// Compute Camera matrix (view)
				Matrices.view = glm::lookAt(eye, target, up); // Rotating Camera for 3D
				//  Don't change unless you are sure!!
				//Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 			2D (ortho) in XY plane

				// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
				//  Don't change unless you are sure!!
				glm::mat4 VP = Matrices.projection * Matrices.view;

				// Send our transformation to the currently bound shader, in the "MVP" uniform
				// For each model you render, since the MVP will be different (at least the M part)
				//  Don't change unless you are sure!!

				glm::mat4 MVP;	
				static int fontScale = 0;
				float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
				glm::vec3 fontColor = getRGBfromHue (fontScale);

				if(won!=1 and lost!=1)
				{	// MVP = Projection * View * Model
					glUseProgram(textureProgramID);
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateback = glm::translate (glm::vec3(-4000,0,-4000));
					//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
					Matrices.model *= translateback ;//* rotatecube;
					MVP = VP * Matrices.model;
					glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
					draw3DTexturedObject(back);
					// Increment angles
					//  float increments = 1;
					// camera_rotation_angle++; // Simulating camera rotation
					if(uptiles==1)
					{
						tilesy++;
						downtiles = 0;
					}
					else if(downtiles ==1)
					{
						tilesy--;
						uptiles = 0;
					}
					if(tilesy > 50)
					{
						downtiles = 1;
						uptiles = 0;
					}
					else if(tilesy < -50)
					{
						downtiles = 0;
						uptiles = 1;
					}	

					int i=0,j=0,random,randomevil,angle,p,q;
					if(levelchange==1)
					{
						for(p=0;p<=10;p++)
							{
								for(q=0;q<=10;q++)
								{
									b[p][q] = 0;
								}
							}
						z=0;
						for(i = 0;i<10;i++)
						{	
							x=0;
							random = rand() % 10;
							randomevil = rand() % 10;
							if((i==0 and j==0) and random==0)
							{
								while(random==0)
								{
									random = rand() % 10;
								}
							}
							if(random == randomevil)
							{
								while(random == randomevil)
								{
									randomevil = rand()%10;
								}

							}
							if(((i==0 and j==0) and randomevil ==0) || (random == randomevil)){
								while(randomevil==0 or randomevil==random)
								{
									randomevil = rand() % 10;
								}
							}
							if(a[i][random]!=2)
								a[i][random] = 1;	
							b[i][randomevil] = 1;	
							for( j=0;j<10;j++)
							{
								if((i==9 and j==9) and random==9)
								{
									while(random==9)
									{
										random = rand() % 10;
									}
									a[9][9]=0;
									a[9][random]=1;
								}
								if(j!=random)
								{
									glUseProgram (programID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translatecube = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translatecube ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									//glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DObject(cube);
									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);



									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect1 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect1 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DTexturedObject(rect1);
									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect2 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect2 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect2);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect3 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect3 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect3);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect4 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect4 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect4);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect5 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect5 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect5);
									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect6 = glm::translate (glm::vec3(x, y, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect6 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect6);



								}
								if(j==randomevil)
								{
									if((i==9 and j==9) and randomevil==9)
									{
										while(randomevil==9)
										{
											randomevil = rand() % 10;
										}
										//cout << "yayy" << '\n';
										b[9][9]=0;
										b[9][randomevil]=1;
									}
									for(angle=0;angle<=360;angle++)
									{
										glUseProgram (programID);
										Matrices.model = glm::mat4(1.0f);
										glm::mat4 translateobstacle = glm::translate (glm::vec3(x+15, 160, z+15));
										glm::mat4 rotateobstacle = glm::rotate((float)(angle*M_PI/180.0f), glm::vec3(0,1,0)); 
										Matrices.model *= translateobstacle * rotateobstacle;
										MVP = VP * Matrices.model;
										glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
										draw3DObject(obstacleex);
									}	
								}
								x=x+30;
							}
							z = z+30;
						}
						levelchange = 0;
					}
					else if(levelchange == 0)
					{
						z = 0;
						curr_time = glfwGetTime();
						if(curr_time - start_time > 5)
						{

							for(p=0;p<=10;p++)
							{
								for(q=0;q<=10;q++)
								{
									b[p][q] = 0;
								}
							}							

							freflag = 1;
							//cout << "entering" << endl;	
						}
						for( i = 0;i<10;i++)
						{	
							x = 0;
							if(freflag == 1)
							{
								randomevil = rand() % 10;
								if(((i==0 and j==0) and randomevil ==0)){
									while(randomevil==0)
									{
										randomevil = rand() % 10;
									}
								}
								b[i][randomevil]=1;
							}
							for( j=0;j<10;j++)
							{

								if(a[i][j]!=1 && a[i][j]!=2){
									glUseProgram (programID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translatecube = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translatecube ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									//	glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DObject(cube);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect1 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect1 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DTexturedObject(rect1);
									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect2 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect2 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DTexturedObject(rect2);
									// draw3DObject draws the VAO given to it using current MVP matrix

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect3 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect3 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect3);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect4 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect4 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect4);


									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect5 = glm::translate (glm::vec3(x, 0, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect5 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect5);
									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect6 = glm::translate (glm::vec3(x, y+0.5, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect6 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect6);



									if(freflag == 1)
									{

										if(j==randomevil)
										{
											if(((i==9 and j==9) and randomevil==9))
											{
												while(randomevil==9)
												{
													randomevil = rand() % 10;
												}
												//cout << "yayy" << '\n';
												b[9][9]=0;
												b[9][randomevil]=1;
											}
											if(user.i == i)
											{
												while(randomevil == user.j || randomevil == user.j+1 || randomevil == user.j-1 || a[i][randomevil]==2)
												{
													randomevil = rand() % 10;
												}
												//cout << "yayy" << '\n';
												b[i][user.j] = 0;
												b[i][randomevil] = 1;
											}
											for(angle=0;angle<=360;angle++)
											{
												glUseProgram (programID);
												Matrices.model = glm::mat4(1.0f);
												glm::mat4 translateobstacle = glm::translate (glm::vec3(x+15, 160, z+15));
												glm::mat4 rotateobstacle = glm::rotate((float)(angle*M_PI/180.0f), glm::vec3(0,1,0)); 
												Matrices.model *= translateobstacle * rotateobstacle;
												MVP = VP * Matrices.model;
												glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
												draw3DObject(obstacleex);
											}
										}
										freflag = 1;
									}
									else
									{	
										if(b[i][j]==1)
										{
											for(angle=0;angle<=360;angle++)
											{
												glUseProgram (programID);
												Matrices.model = glm::mat4(1.0f);
												glm::mat4 translateobstacle = glm::translate (glm::vec3(x+15, 115, z+15));
												glm::mat4 rotateobstacle = glm::rotate((float)(angle*M_PI/180.0f), glm::vec3(0,1,0)); 
												Matrices.model *= translateobstacle * rotateobstacle;
												MVP = VP * Matrices.model;
												glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
												draw3DObject(obstacleex);
											}
										}
									}
								}
								if((a[i][j]==1 and i%2==0) || a[i][j] == 2)
								{
									glUseProgram (programID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translatecube = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translatecube ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									draw3DObject(cube);



									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect1 = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect1 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DTexturedObject(rect1);
									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect2 = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect2 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
									draw3DTexturedObject(rect2);
									// draw3DObject draws the VAO given to it using current MVP matrix


									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect3 = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect3 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect3);

									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect4 = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect4 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect4);


									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect5 = glm::translate (glm::vec3(x, tilesy, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect5 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect5);
									glUseProgram(textureProgramID);
									Matrices.model = glm::mat4(1.0f);
									glm::mat4 translaterect6 = glm::translate (glm::vec3(x, tilesy + 100.5, z));
									//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
									Matrices.model *= translaterect6 ;//* rotatecube;
									MVP = VP * Matrices.model;
									glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
									glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

									// draw3DObject draws the VAO given to it using current MVP matrix
									//draw3DTexturedObject(rectangle);
									draw3DTexturedObject(rect6);
									a[i][j]=2;
								}
								x=x+30;
							}
							z = z+30;
						}
					}
					if(freflag == 1)
					{
						freflag = 0;
						start_time = glfwGetTime();
					}
					user.position();
					user.checkdown();

					user.checkcollision();
					user.checksliding();
					user.checkboundary();
					glUseProgram (programID);
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translatecube = glm::translate (glm::vec3(user.x, user.y,user.z));
					//glm::mat4 rotatecube = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,1,0)); 
					Matrices.model *= translatecube ;//* rotatecube;
					MVP = VP * Matrices.model;
					glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
					draw3DObject(cubetest);






					// Use font Shaders for next part of code
					glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText = glm::translate(glm::vec3(4,4,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("LEVEL : ");

					//glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText2 = glm::translate(glm::vec3(6.5,4,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText2; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					char pri[10];
					sprintf(pri,"%d",count+1);
					GL3Font.font->Render(pri);



					//glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText4 = glm::translate(glm::vec3(4,3,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText4; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("LIFES : ");



					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText3 = glm::translate(glm::vec3(6.5,3,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText3; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font

					sprintf(pri,"%d",10-lifes);
					GL3Font.font->Render(pri);

					//glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText5 = glm::translate(glm::vec3(4,2,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText5; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("SCORE : ");



					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText6 = glm::translate(glm::vec3(6.5,2,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText6; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font

					sprintf(pri,"%d",score);
					GL3Font.font->Render(pri);

				}


				/*float lifesx = 300; 
				  for( int e=0; e<(10-lifes); e++)
				  {
				  glUseProgram (programID);
				  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
				  Matrices.model = glm::mat4(1.0f);
				  glm::mat4 rotatelifes = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,0,1)); 
				  glm::mat4 translatelifes = glm::translate (glm::vec3(lifesx-(e*50),250,0));        // glTranslatef
				// rotate about vector (-1,1,1)
				Matrices.model *= translatelifes*rotatelifes;
				MVP = VP * Matrices.model;
				glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				// draw3DObject draws the VAO given to it using current MVP matrix
				draw3DObject(obstacleex);
				}*/


				//camera_rotation_angle++; // Simulating camera rotation
				//triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
				//rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;

				// font size and color changes'
				//fontScale = (fontScale + 1) % 360;

				if(won == 1)
				{
					glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText15 = glm::translate(glm::vec3(-1,3,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText15; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("YOU WON!!!");

					glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText20 = glm::translate(glm::vec3(-2,0,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText20; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("FOR PLAYING AGAIN,PRESS N");



				}
				if(lost == 1)
				{
					glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText9 = glm::translate(glm::vec3(-1,3,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText9; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("GAME OVER");


					glUseProgram(fontProgramID);
					Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

					// Transform the tex
					Matrices.model = glm::mat4(1.0f);
					glm::mat4 translateText10 = glm::translate(glm::vec3(-2,0,0));
					//glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
					Matrices.model *= translateText10; //* scaleText);
					MVP = Matrices.projection * Matrices.view * Matrices.model;
					// send font's MVP and font color to fond shaders
					glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

					// Render font
					GL3Font.font->Render("FOR PLAYING AGAIN,PRESS N");



				}



			}

			/* Initialise glfw window, I/O callbacks and the renderer to use */
			/* Nothing to Edit here */
			GLFWwindow* initGLFW (int width, int height)
			{
				GLFWwindow* window; // window desciptor/handle

				glfwSetErrorCallback(error_callback);
				if (!glfwInit()) {
					exit(EXIT_FAILURE);
				}

				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

				window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

				if (!window) {
					glfwTerminate();
					exit(EXIT_FAILURE);
				}

				glfwMakeContextCurrent(window);
				gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
				glfwSwapInterval( 1 );

				/* --- register callbacks with GLFW --- */

				/* Register function to handle window resizes */
				/* With Retina display on Mac OS X GLFW's FramebufferSize
				   is different from WindowSize */
				glfwSetFramebufferSizeCallback(window, reshapeWindow);
				glfwSetWindowSizeCallback(window, reshapeWindow);

				/* Register function to handle window close */
				glfwSetWindowCloseCallback(window, quit);

				/* Register function to handle keyboard input */
				glfwSetKeyCallback(window, keyboard);      // general keyboard input
				glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
				glfwSetScrollCallback(window, scrollback);
				glfwSetCursorPosCallback(window,mouse);
				/* Register function to handle mouse click */
				glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

				return window;
			}

			/* Initialize the OpenGL rendering properties */
			/* Add all the models to be created here */
			void initGL (GLFWwindow* window, int width, int height)
			{


				/* Objects should be created before any other gl function and shaders */
				// Create the models
				//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
				//createRectangle ();
				//cube = createCube(30,100,30);

				glActiveTexture(GL_TEXTURE0);
				// load an image file directly as a new OpenGL texture
				// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
				GLuint textureID = createTexture("crate.jpg");
				// check for an error during the load process
				if(textureID == 0 )
					cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

				// Create and compile our GLSL program from the texture shaders
				textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
				// Get a handle for our "MVP" uniform
				Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");


				/* Objects should be created before any other gl function and shaders */
				// Create the models
				//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
				cube = createCube ();
				cubetest = user.createCube(15,15,15);
				obstacleex = obstacle.createCircle();
				rect1 = createRectangleBack(textureID);
				rect2 = createRectangleUP(textureID);
				rect3 = createRectangleFront(textureID);
				rect4 = createRectangleDown(textureID);
				rect5 = createRectangleDown(textureID);


				glActiveTexture(GL_TEXTURE0);
				// load an image file directly as a new OpenGL texture
				// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
				GLuint textureIDup = createTexture("texture.png");
				// check for an error during the load process
				if(textureIDup == 0 )
					cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

				// Create and compile our GLSL program from the texture shaders
				textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
				// Get a handle for our "MVP" uniform
				Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");
				rect6 = createRectangleRight(textureIDup);


				glActiveTexture(GL_TEXTURE0);
				// load an image file directly as a new OpenGL texture
				// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
				GLuint textureIDwater = createTexture("water2.jpg");
				// check for an error during the load process
				if(textureIDwater == 0 )
					cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

				// Create and compile our GLSL program from the texture shaders
				textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
				// Get a handle for our "MVP" uniform
				Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");

				back = createRectangle(textureIDwater);


				// Create and compile our GLSL program from the shaders
				programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
				// Get a handle for our "MVP" uniform
				Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


				reshapeWindow (window, width, height);
				//47 79 79 
				// Background color of the scene
				//72 61 139
				glClearColor (1,1,1,1); // R, G, B, A
				glClearDepth (1.0f);

				glEnable (GL_DEPTH_TEST);
				glDepthFunc (GL_LEQUAL);

				const char* fontfile = "arial.ttf";
				GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

				if(GL3Font.font->Error())
				{
					cout << "Error: Could not load font `" << fontfile << "'" << endl;
					glfwTerminate();
					exit(EXIT_FAILURE);
				}

				// Create and compile our GLSL program from the font shaders
				fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
				GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
				fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
				fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
				fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
				GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
				GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

				GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
				GL3Font.font->FaceSize(1);
				GL3Font.font->Depth(0);
				GL3Font.font->Outset(0, 0);
				GL3Font.font->CharMap(ft_encoding_unicode);
				cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
				cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
				cout << "VERSION: " << glGetString(GL_VERSION) << endl;
				cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
			}

			int main (int argc, char** argv)
			{
				int width = 1600;
				int height = 800;
				user.x = 7.5;
				user.y = 105;
				user.z = 7.5;	
				user.i = 0;
				user.j = 0;
				obstacle.rad=15;
				obstacle.color1 = 1;
				start_time = glfwGetTime();

				GLFWwindow* window = initGLFW(width, height);

				initGL (window, width, height);


				//double last_update_time = glfwGetTime(), current_time;


				/* Draw in loop */
				while (!glfwWindowShouldClose(window)) {

					// OpenGL Draw commands
					draw();

					// Swap Frame Buffer in double buffering
					glfwSwapBuffers(window);

					// Poll for Keyboard and mouse events
					glfwPollEvents();

					// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
					//   current_time = glfwGetTime(); // Time in seconds
					/*  if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
					// do something every 0.5 seconds ..
					last_update_time = current_time;*/
					//}
					if(lifes > 9 and win==0)
					{
						lost = 1;
						cout << "YOU LOST" << endl;
						cout << "SCORE" <<" " << score << endl;
					}
					else if(win == 1)
					{
						cout << "YOU WON THIS LEVEL" << endl;
						win = 0;
						lifes = 0;
						user.x = 7.5;
						levelchange = 1;
						count++;
						score = score+100;
						user.y = 105;
						user.z = 7.5;	
						user.i = 0;
						user.j = 0;
						start_time = glfwGetTime();

					}
					if(count == 6)
					{
						cout << "YOU WON" << endl;
						cout << "SCORE" << " " << score << endl;
						won = 1;
					}
				}

				glfwTerminate();
				exit(EXIT_SUCCESS);
			}
