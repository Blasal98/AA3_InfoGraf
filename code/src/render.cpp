#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include <iostream>
#include "GL_framework.h"
#include <vector>


#define  STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"



///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
void setupAxis();
void cleanupAxis();
void drawAxis();
}
////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 50.f;

	int width, height;
	bool cameraType = true;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;

		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;

	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	RV::width = width;
	RV::height = height;
	if(height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {

	if (RV::cameraType) {
		if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
			float diffx = ev.posx - RV::prevMouse.lastx;
			float diffy = ev.posy - RV::prevMouse.lasty;
			switch (ev.button) {
			case MouseEvent::Button::Left: // ROTATE
				RV::rota[0] += diffx * 0.005f;
				RV::rota[1] += diffy * 0.005f;
				break;
			case MouseEvent::Button::Right: // MOVE XY
				RV::panv[0] += diffx * 0.03f;
				RV::panv[1] -= diffy * 0.03f;
				break;
			case MouseEvent::Button::Middle: // MOVE Z
				RV::panv[2] += diffy * 0.05f;
				break;
			default: break;
			}
		}
		else {
			RV::prevMouse.button = ev.button;
			RV::prevMouse.waspressed = true;
		}
		RV::prevMouse.lastx = ev.posx;
		RV::prevMouse.lasty = ev.posy;
	}
}

//////////////////////////////////////////////////
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name="") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

////////////////////////////////////////////////// AXIS
namespace Axis {
GLuint AxisVao;
GLuint AxisVbo[3];
GLuint AxisShader[2];
GLuint AxisProgram;

float AxisVerts[] = {
	0.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 0.0, 1.0
};
float AxisColors[] = {
	1.0, 0.0, 0.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0
};
GLubyte AxisIdx[] = {
	0, 1,
	2, 3,
	4, 5
};
const char* Axis_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
const char* Axis_fragShader =
"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

void setupAxis() {
	glGenVertexArrays(1, &AxisVao);
	glBindVertexArray(AxisVao);
	glGenBuffers(3, AxisVbo);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
	AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

	AxisProgram = glCreateProgram();
	glAttachShader(AxisProgram, AxisShader[0]);
	glAttachShader(AxisProgram, AxisShader[1]);
	glBindAttribLocation(AxisProgram, 0, "in_Position");
	glBindAttribLocation(AxisProgram, 1, "in_Color");
	linkProgram(AxisProgram);
}
void cleanupAxis() {
	glDeleteBuffers(3, AxisVbo);
	glDeleteVertexArrays(1, &AxisVao);

	glDeleteProgram(AxisProgram);
	glDeleteShader(AxisShader[0]);
	glDeleteShader(AxisShader[1]);
}
void drawAxis() {
	glBindVertexArray(AxisVao);
	glUseProgram(AxisProgram);
	glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

	glUseProgram(0);
	glBindVertexArray(0);
}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
GLuint cubeVao;
GLuint cubeVbo[3];
GLuint cubeShaders[2];
GLuint cubeProgram;
glm::mat4 objMat = glm::mat4(1.f);

extern const float halfW = 0.5f;
int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

					   //   4---------7
					   //  /|        /|
					   // / |       / |
					   //5---------6  |
					   //|  0------|--3
					   //| /       | /
					   //|/        |/
					   //1---------2
glm::vec3 verts[] = {
	glm::vec3(-halfW, -halfW, -halfW),
	glm::vec3(-halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW, -halfW),
	glm::vec3(-halfW,  halfW, -halfW),
	glm::vec3(-halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW, -halfW)
};
glm::vec3 norms[] = {
	glm::vec3(0.f, -1.f,  0.f),
	glm::vec3(0.f,  1.f,  0.f),
	glm::vec3(-1.f,  0.f,  0.f),
	glm::vec3(1.f,  0.f,  0.f),
	glm::vec3(0.f,  0.f, -1.f),
	glm::vec3(0.f,  0.f,  1.f)
};

glm::vec3 cubeVerts[] = {
	verts[1], verts[0], verts[2], verts[3],
	verts[5], verts[6], verts[4], verts[7],
	verts[1], verts[5], verts[0], verts[4],
	verts[2], verts[3], verts[6], verts[7],
	verts[0], verts[4], verts[3], verts[7],
	verts[1], verts[2], verts[5], verts[6]
};
glm::vec3 cubeNorms[] = {
	norms[0], norms[0], norms[0], norms[0],
	norms[1], norms[1], norms[1], norms[1],
	norms[2], norms[2], norms[2], norms[2],
	norms[3], norms[3], norms[3], norms[3],
	norms[4], norms[4], norms[4], norms[4],
	norms[5], norms[5], norms[5], norms[5]
};
GLubyte cubeIdx[] = {
	0, 1, 2, 3, UCHAR_MAX,
	4, 5, 6, 7, UCHAR_MAX,
	8, 9, 10, 11, UCHAR_MAX,
	12, 13, 14, 15, UCHAR_MAX,
	16, 17, 18, 19, UCHAR_MAX,
	20, 21, 22, 23, UCHAR_MAX
};

const char* cube_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
const char* cube_fragShader =
"#version 330\n\
in vec4 vert_Normal;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 color;\n\
void main() {\n\
	out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
}";
void setupCube() {
	glGenVertexArrays(1, &cubeVao);
	glBindVertexArray(cubeVao);
	glGenBuffers(3, cubeVbo);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glPrimitiveRestartIndex(UCHAR_MAX);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
	cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

	cubeProgram = glCreateProgram();
	glAttachShader(cubeProgram, cubeShaders[0]);
	glAttachShader(cubeProgram, cubeShaders[1]);
	glBindAttribLocation(cubeProgram, 0, "in_Position");
	glBindAttribLocation(cubeProgram, 1, "in_Normal");
	linkProgram(cubeProgram);
}
void cleanupCube() {
	glDeleteBuffers(3, cubeVbo);
	glDeleteVertexArrays(1, &cubeVao);

	glDeleteProgram(cubeProgram);
	glDeleteShader(cubeShaders[0]);
	glDeleteShader(cubeShaders[1]);
}
void updateCube(const glm::mat4& transform) {
	objMat = transform;
}
void drawCube() {
	glEnable(GL_PRIMITIVE_RESTART);
	glBindVertexArray(cubeVao);
	glUseProgram(cubeProgram);
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
	glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
	glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

	glUseProgram(0);
	glBindVertexArray(0);
	glDisable(GL_PRIMITIVE_RESTART);
}
}
//////////////////////////////////////////////////OBJ file (DRAGON)
extern bool loadOBJ(const char* path,
	std::vector <glm::vec3> & out_vertices,
	std::vector <glm::vec2> & out_uvs,
	std::vector <glm::vec3> & out_normals); //importamos la funci�n hecha en clase


class BezierCurve {
private:
	std::vector<glm::vec3> points;
	glm::vec3 p0 = glm::vec3(0, 0, 0);
	glm::vec3 p1 = glm::vec3(4, 0, -10);
	glm::vec3 p2 = glm::vec3(8, 0, 10);
	glm::vec3 p3 = glm::vec3(12, 0, -10);
	glm::vec3 p4 = glm::vec3(16, 0, 0);
public:
	std::vector<glm::vec3> getPoints() {
		return points;
	}
	float getX(float _t) {
		float totalX = 0;
		float tBuena = _t;
		if (tBuena > 1) tBuena = 1;

		for (int i = 0; i < points.size(); i++) {
			int iBuena = i + 1;
			totalX += points[i].x * combinatory(points.size(), iBuena) * glm::pow(tBuena, iBuena) * glm::pow(1 - tBuena, points.size() - iBuena);
		}
		return totalX;
	}
	float getZ(float _t) {
		float totalZ = 0;
		float tBuena = _t;
		if (tBuena > 1) tBuena = 1;

		for (int i = 0; i < points.size(); i++) {
			int iBuena = i + 1;
			totalZ += points[i].z * combinatory(points.size(), iBuena) * glm::pow(tBuena, iBuena) * glm::pow(1 - tBuena, points.size() - iBuena);
		}
		return totalZ;
	}
	/*float getYRot(float t) {

		return glm::atan(getX(t) / getZ(t));
	}*/

	unsigned long combinatory(int n, int i) {
		return factorial(n) / (factorial(i) * factorial(n - i));
	}

	unsigned long factorial(int f)
	{
		if (f == 0)
			return 1;
		return(f * factorial(f - 1));
	}

	BezierCurve() {
		points.push_back(p0);
		points.push_back(p1);
		points.push_back(p2);
		points.push_back(p3);
		points.push_back(p4);
	}

};
BezierCurve car_path;
float t = 0;
float ourDt = 0.01f;

namespace Object {

	const char* path = "cotxe.obj";
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
	glm::vec4 prueba;
	std::vector <glm::vec3> vertices;
	std::vector <glm::vec2> uvs;
	std::vector <glm::vec3> normals;

	GLuint vao;
	GLuint vbo[3];
	GLuint textureIDCar;

	GLuint shaders[2];
	GLuint program;

	glm::mat4 objMat;

	static const GLchar* vertex_shader_source[] = {
		"#version 330\n"
		"in vec3 in_Normal;\n"
		"in vec3 in_Position;\n"
		"in vec2 in_Uvs;\n"
		"uniform mat4 objMat;\n"
		"uniform mat4 mvpMat;\n"
		"out vec4 vert_Normal;\n"
		"out vec4 fragment_Position;\n"
		"out vec2 textCoords;\n"
		"void main() {\n"

			"gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n"

			"vert_Normal = objMat * vec4(in_Normal, 0.0);\n"
			"fragment_Position = objMat * vec4(in_Position, 1.0);\n"

			"textCoords =in_Uvs; \n"

		"}\n"
	};

	// A fragment shader that assigns a static color
	static const GLchar* fragment_shader_source[] = {
		"#version 330\n"
		"in vec4 vert_Normal;\n"
		"in vec4 fragment_Position;\n"
		"in vec2 textCoords;\n"

		"uniform sampler2D objTexture;\n"

		"out vec4 color;\n"


		"void main() {\n"

			"color = texture(objTexture,textCoords);\n"

		"}\n"
	};


	void setupObject() {
		bool res = loadOBJ(path, vertices, uvs, normals);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(3, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		shaders[0] = compileShader(vertex_shader_source[0], GL_VERTEX_SHADER, "objectVertexShader");
		shaders[1] = compileShader(fragment_shader_source[0], GL_FRAGMENT_SHADER, "objectFragmentShader");

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);
		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_Uvs");
		linkProgram(program);


		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);

		unsigned char* data = stbi_load(
			"colors.png",
			&width,
			&height,
			&channels,
			0);

		if (data == NULL) {

			fprintf(stderr, "No se lee nada %s\n", stbi_failure_reason());

		}
		else {
			//leemos la imagen
			glGenTextures(1, &textureIDCar);

			glBindTexture(GL_TEXTURE_2D, textureIDCar);
		
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGB,
				width,
				height,
				0,
				channels == 3 ? GL_RGB : GL_RGBA,
				GL_UNSIGNED_BYTE,
				data


			);

		}

		stbi_image_free(data);


		objMat = glm::mat4(1.f);
	}

	void updateObject(glm::mat4 matrix) {
		objMat = matrix;
	}

	void drawObject() {
		glBindVertexArray(vao);
		glUseProgram(program);

		glBindTexture(GL_TEXTURE_2D, textureIDCar);
		glm::vec4 colorObj = glm::vec4(1.53f, 0.28f, 0, 0.f);
		glm::vec4 ligth_color = glm::vec4(255.f, 255.f, 255.f, 0.f);

		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		/*glUniform4f(glGetUniformLocation(program, "object_color"), colorObj[0], colorObj[1], colorObj[2], colorObj[3]);
		glUniform4f(glGetUniformLocation(program, "light_color"), ligth_color[0], ligth_color[1], ligth_color[2], ligth_color[3]);
		glUniform4f(glGetUniformLocation(program, "camera_position"), RV::panv[0], RV::panv[1], RV::panv[2], 1.f);
		glUniform4f(glGetUniformLocation(program, "light_position"), Object::prueba[0], Object::prueba[1], Object::prueba[2], 1);
		glUniform1f(glGetUniformLocation(program, "ambient_light"), Object::ambientStrength*0.001);
		glUniform1f(glGetUniformLocation(program, "diffuse_light"), Object::diffuseStrength*0.001);
		glUniform1f(glGetUniformLocation(program, "specular_light"), Object::specularStrength*0.01);*/

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}

	void cleanupObject() {
		glDeleteBuffers(3, vbo);
		glDeleteVertexArrays(1, &vao);

		glDeleteProgram(program);
		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
	}
}

namespace Retrovisor {

	const char* path = "untitled.obj";
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
	glm::vec4 prueba;
	std::vector <glm::vec3> vertices;
	std::vector <glm::vec2> uvs;
	std::vector <glm::vec3> normals;

	GLuint vao;
	GLuint vbo[3];
	GLuint textureID;

	GLuint shaders[2];
	GLuint program;

	glm::mat4 objMat;

	static const GLchar* vertex_shader_source[] = {
		"#version 330\n"
		"in vec3 in_Normal;\n"
		"in vec3 in_Position;\n"
		"in vec2 in_Uvs;\n"
		"uniform mat4 objMat;\n"
		"uniform mat4 mvpMat;\n"
		"out vec4 vert_Normal;\n"
		"out vec4 fragment_Position;\n"
		"out vec2 textCoords;\n"
		"void main() {\n"

			"gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n"

			"vert_Normal = objMat * vec4(in_Normal, 0.0);\n"
			"fragment_Position = objMat * vec4(in_Position, 1.0);\n"

			"textCoords =in_Uvs; \n"

		"}\n"
	};

	
	static const GLchar* fragment_shader_source[] = {
		"#version 330\n"
		"in vec4 vert_Normal;\n"
		"in vec4 fragment_Position;\n"
		"in vec2 textCoords;\n"

		"uniform sampler2D objTexture;\n"

		"out vec4 color;\n"


		"void main() {\n"

			"color = texture(objTexture,textCoords);\n"

		"}\n"
	};


	

	void setupRetrovisor() {
		bool res = loadOBJ(path, vertices, uvs, normals);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(3, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		shaders[0] = compileShader(vertex_shader_source[0], GL_VERTEX_SHADER, "objectVertexShader");
		shaders[1] = compileShader(fragment_shader_source[0], GL_FRAGMENT_SHADER, "objectFragmentShader");

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);
		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_Uvs");
		linkProgram(program);


		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);

		unsigned char* data = stbi_load(
			"Captura.png",
			&width,
			&height,
			&channels,
			0);

		if (data == NULL) {

			fprintf(stderr, "No se lee nada %s\n", stbi_failure_reason());

		}
		else {
			//leemos la imagen
			glGenTextures(1, &textureID);

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGB,
				width,
				height,
				0,
				channels == 3 ? GL_RGB : GL_RGBA,
				GL_UNSIGNED_BYTE,
				data


			);

		}

		stbi_image_free(data);


		objMat = glm::mat4(1.f);
	}

	void updateRetrovisor(glm::mat4 matrix) {
		objMat = matrix;
	}

	void drawRetrovisor(GLuint  tex = NULL) {

		glm::mat4 rotacion = glm::mat4(1.f);
		glm::mat4 traslacion = glm::translate(glm::mat4(1.f), glm::vec3(0, -0.5f, -6));
		glm::mat4 escalado = glm::scale(glm::mat4(1.f), glm::vec3(2.f, 2.0f, 2.f));
		Retrovisor::updateRetrovisor(traslacion * rotacion * escalado);

		glBindVertexArray(vao);
		glUseProgram(program);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glm::vec4 colorObj = glm::vec4(1.53f, 0.28f, 0, 0.f);
		glm::vec4 ligth_color = glm::vec4(255.f, 255.f, 255.f, 0.f);

		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		/*glUniform4f(glGetUniformLocation(program, "object_color"), colorObj[0], colorObj[1], colorObj[2], colorObj[3]);
		glUniform4f(glGetUniformLocation(program, "light_color"), ligth_color[0], ligth_color[1], ligth_color[2], ligth_color[3]);
		glUniform4f(glGetUniformLocation(program, "camera_position"), RV::panv[0], RV::panv[1], RV::panv[2], 1.f);
		glUniform4f(glGetUniformLocation(program, "light_position"), Object::prueba[0], Object::prueba[1], Object::prueba[2], 1);
		glUniform1f(glGetUniformLocation(program, "ambient_light"), Object::ambientStrength*0.001);
		glUniform1f(glGetUniformLocation(program, "diffuse_light"), Object::diffuseStrength*0.001);
		glUniform1f(glGetUniformLocation(program, "specular_light"), Object::specularStrength*0.01);*/
		

		//esteifesopcional

		if (tex == NULL) {
			glBindTexture(GL_TEXTURE_2D, textureID);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, tex);
		}
		
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());


	}



	void cleanupRetrovisor() {
		glDeleteBuffers(3, vbo);
		glDeleteVertexArrays(1, &vao);

		glDeleteProgram(program);
		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
	}
}

/////////////////////////////////////////////////


GLuint fbo, fbo_texture,rbo;
void setupFBO() {

	glGenFramebuffers(1, &fbo);
	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//OLLO AQUI
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1500, 1000, 0, GL_RGB, GL_UNSIGNED_INT, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1500, 1000);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	//por seguriddad

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//nos vamos pal init
}
void drawObjects(float dt) {

	glm::mat4 traslacion = glm::mat4(1.f);
	glm::mat4 rotacion = glm::mat4(1.f);
	glm::mat4 escalado = glm::mat4(1.f);

	if (t <= 1) {
		glm::vec3 movimiento = glm::vec3(car_path.getX(t), 0, car_path.getZ(t));
		glm::vec3 movimientoDt = glm::vec3(car_path.getX(t + dt), 0, car_path.getZ(t + dt));
		glm::vec3 vector_t_dt = movimientoDt - movimiento;
		float angle = glm::atan(vector_t_dt.x / vector_t_dt.z);

		rotacion = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0));
		traslacion = glm::translate(glm::mat4(1.f), movimiento);
	}
	else {
		t = 0;
	}
	t += ourDt / 5;

	//suelo


	escalado = glm::scale(glm::mat4(1.f), glm::vec3(0.3f, 0.3f, 0.3f));
	Object::updateObject(traslacion * rotacion * escalado);
	Object::drawObject();

	rotacion = glm::mat4(1.f);
	traslacion = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 0));
	escalado = glm::scale(glm::mat4(1.f), glm::vec3(50.f, 0.2f, 50.f));
	Cube::updateCube(traslacion * rotacion * escalado);
	Cube::drawCube();

	


}
void drawRetrovisorFBOTexture(float dt) {
	glm::mat4 t_mvp = RV::_MVP;
	glm::mat4 t_mv = RV::_modelView;


	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClearColor(1.f, 1.f, 1.f, 0.2f);
	glViewport(0, 0, 1500, 1000);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_modelView = glm::translate(glm::mat4(1.f), glm::vec3(0, 0,-6));

	RV::_MVP = glm::perspective(RV::FOV,(float)1500/(float)1000, RV::zNear, RV::zFar) *RV::_modelView;
	
	drawObjects(dt);

	RV::_modelView = t_mv;
	RV::_MVP = t_mvp;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0,RV::width, RV::height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);

	Retrovisor::drawRetrovisor(fbo_texture);
}


void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	RV::width = width;
	RV::height = height;
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	
	
	Axis::setupAxis();
	Cube::setupCube();
	Object::setupObject();
	Retrovisor::setupRetrovisor();
	setupFBO();


	


	/////////////////////////////////////////////////////TODO
	// Do your init code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

void GLcleanup() {
	Axis::cleanupAxis();
	Cube::cleanupCube();
	Object::cleanupObject();
	Retrovisor::cleanupRetrovisor();

	glDeleteRenderbuffers(1 ,&rbo);
	/////////////////////////////////////////////////////TODO
	// Do your cleanup code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

void GLrender(float dt) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (RV::cameraType) {
		RV::_modelView = glm::mat4(1.f);
		RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));


	}
	else {

		RV::_modelView = glm::mat4(1.f);
		RV::_modelView = glm::translate(glm::mat4(1.f), glm::vec3(0, -1.f, -2.f));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));
	}
	RV::_MVP = RV::_projection * RV::_modelView;



	drawRetrovisorFBOTexture(dt);
	drawObjects(dt);


	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		if (ImGui::Button("Camera")) //Si se aprieta Close-Up
		{
			if (RV::cameraType) {
				
				RV::cameraType = false;
				
			}
			else {
				
				RV::cameraType = true;
			}
		}
		// ...
		// ...
		// ...
		/////////////////////////////////////////////////////////
	}
	// .........................

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}