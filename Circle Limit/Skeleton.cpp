//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//=============================================================================================
#include "framework.h"

const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform mat4 MVP;

	layout(location = 0) in vec2 vp;
	layout(location = 1) in vec2 uv;

	out vec2 texCoord;

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;
		texCoord = uv;
}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	in vec2 texCoord;
	uniform sampler2D samplerUnit;

	out vec4 outColor;

	void main() {
		outColor = texture(samplerUnit, texCoord);
	}
)";

GPUProgram gpuProgram;


float lorentz(vec3 v1, vec3 v2) {
	return v1.x * v2.x + v1.y * v2.y - v1.z * v2.z;
}


vec3 pontToHiperboloid(vec2 pont) {
		
	vec3 lineStart = vec3(0, 0, -1);
	vec3 point = vec3(pont.x, pont.y, 0);
	vec3 lineDirection = point - lineStart;

	float a = lorentz(lineDirection, lineDirection);
	float b = 2 * lorentz(lineDirection, lineStart);
	float c = lorentz(lineStart, lineStart) + 1;

	if (b * b - 4 * a * c < 0) {
		return vec3(0, 0, -1);
	}

	float t1 = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
	float t2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

	if(vec3(lineStart + t1 * lineDirection).z < 0){
		return lineStart + t2 * lineDirection;
	} else {
		return lineStart + t1 * lineDirection;
	}
}

vec2 hiperboloidToPont(vec3 hiperboloid) {
	vec3 lineStart = vec3(0, 0, -1);
	vec3 lineDirection = hiperboloid - lineStart;

	float t = (0 - lineStart.z) / lineDirection.z;

	vec2 point = vec2(lineStart.x + t * lineDirection.x, lineStart.y + t * lineDirection.y);

	return point;
}


vec3 pontToCircle(vec2 pont) {
	float r1 = length(pont);
	float r2 = 1 / r1;

	vec2 p2 = (pont / r1) * r2;
	vec2 o = (pont / 2) + (p2 / 2);

	float sugar = length(p2 - o);

	return vec3(o.x, o.y, sugar);
}

int circleCount(std::vector<vec3> circles, vec2 p) {
	int count = 0;
	
	for (vec3 circle : circles) {
		vec2 o = vec2(circle.x, circle.y);
		float sugar = circle.z;

		if (length(p - o) < sugar) {
			count++;
		}
	}
	return count;
}


std::vector<vec3> generateCircles() {
	std::vector<vec3> circles;
	float pi = 3.14159265359;

	for (int i = 0; i < 9; i++) {
		float alpha = i * 2 * pi / 9;
		vec3 v0 = vec3(cosf(alpha), sinf(alpha), 0);

		for (int j = 0; j < 6; j++) {
			float dist = 0.5 + j;
			
			vec3 p = vec3(0,0,1) * coshf(dist) + v0 * sinhf(dist);
			circles.push_back(pontToCircle(hiperboloidToPont(p)));
		}
	}
	return circles;
}


vec4 getColor(vec2 p, std::vector<vec3> circles) {
	if (circleCount(circles ,p) % 2 == 0) {
		return vec4(1, 1, 0, 1);
	}
	else {
		return vec4(0, 0, 1, 1);
	}
}

vec4 howToColor(int felbontas, int x, int y, std::vector<vec3> circles) {
	
	float pixel = 2.0 / felbontas;
	vec2 p = vec2(-1 + pixel * x + pixel / 2, -1 + pixel * y + pixel / 2);

	if (length(p) > 1) {
		return vec4(0, 0, 0, 0);
	}

	return getColor(p, circles);
}

std::vector<vec4> generateTexture(int felbontas, std::vector<vec3> circles) {
	std::vector<vec4> texture;
	for (int x = 0; x < felbontas; x++) {
		for (int y = 0; y < felbontas; y++) {
			vec4 color = howToColor(felbontas, x, y, circles);
			texture.push_back(color);
		}
	}
	return texture;
}


int res = 300;
class Poincare : Texture {
	unsigned int texture;
	std::vector<vec3> circles = generateCircles();
public:

	void init() {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		int sampler = 0;
		gpuProgram.setUniform(sampler, "samplerUnit");
		glActiveTexture(GL_TEXTURE0 + sampler);
	}

	void RenderToTexture(int resolution, int sampling) {

		std::vector<vec4> t = generateTexture(resolution, circles);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution, resolution, 0, GL_RGBA, GL_FLOAT, &(t[0]));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling);
	}

};

unsigned int vao, vbo[2];
std::vector<vec2> vtx, uvs;

class Star {
public:
	void draw() {
		vtx.push_back(vec2(0, 0));  uvs.push_back(vec2(0.5, 0.5));
		vtx.push_back(vec2(-0.5, -0.5));  uvs.push_back(vec2(0, 0));
		vtx.push_back(vec2(0, -0.5)); uvs.push_back(vec2(0.5, 0));
		vtx.push_back(vec2(0.5, -0.5)); uvs.push_back(vec2(1, 0));
		vtx.push_back(vec2(0.5, 0)); uvs.push_back(vec2(1, 0.5));
		vtx.push_back(vec2(0.5, 0.5)); uvs.push_back(vec2(1, 1));
		vtx.push_back(vec2(0, 0.5)); uvs.push_back(vec2(0.5, 1));
		vtx.push_back(vec2(-0.5, 0.5)); uvs.push_back(vec2(0, 1));
		vtx.push_back(vec2(-0.5, 0)); uvs.push_back(vec2(0, 0.5));
		vtx.push_back(vec2(-0.5, -0.5)); uvs.push_back(vec2(0, 0));


		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(2, &vbo[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	}

	void decreaseBreakpoints() {
		for (int i = 0; i < vtx.size(); i += 2) {
			vec2 to_center = (vec2(0, 0) - vtx[i]) * 0.25;
			vtx[i] = vtx[i] + to_center;
		}
	}

	void animate(long time) {
		float pi = 3.14159265359;
		float t = (2 * pi) / 5000 * time;

		mat4 rotate = RotationMatrix(t, vec3(0,0,1));

		float alpha = (pi / 5000) * time;;


		mat4 scale = ScaleMatrix(vec3(16.0 / 15.0, 16.0 / 15.0, 0));

		mat4 move = TranslateMatrix(vec3(cosf(alpha)* 30.0 / 75.0, sinf(alpha)*30.0 / 75.0, 0));

		gpuProgram.setUniform(scale*rotate * move, "MVP");
		
		glutPostRedisplay();
	}

};


Star star;
Poincare texture;
int mode = GL_LINEAR;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	gpuProgram.create(vertexSource, fragmentSource, "outColor");



	texture.init();
	texture.RenderToTexture(res, mode);

	mat4 scale = ScaleMatrix(vec3(16.0/15.0, 16.0/15.0, 0));

	mat4 start = TranslateMatrix(vec3(30.0/75.0, 0, 0));

	gpuProgram.setUniform(scale*start, "MVP");

	star.draw();
}


void onDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);




	glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vtx), &vtx[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(uvs), &uvs[0], GL_STATIC_DRAW);

	glDrawArrays(GL_TRIANGLE_FAN, 0, vtx.size());

	glutSwapBuffers();
}


bool animStart = false;
long t0 = 0;

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'r':
		res -= 100;
		texture.RenderToTexture(res, mode);
		break;
	case 'R':
		res += 100;
		texture.RenderToTexture(res, mode);
		break;
	case 't':
		mode = GL_NEAREST;
		texture.RenderToTexture(res, mode);
		break;
	case 'T':
		mode = GL_LINEAR;
		texture.RenderToTexture(res, GL_LINEAR);
		break;
	case 'h':
		star.decreaseBreakpoints();
		break;
	case 'a':
		if (animStart) {
			break;
		}
		animStart = true;
		t0 = glutGet(GLUT_ELAPSED_TIME);
		break;
	}
	glutPostRedisplay();
}


void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(int pX, int pY) {
}

void onMouse(int button, int state, int pX, int pY) {
	
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
	if (animStart) {
		star.animate(time - t0);
	}
}
