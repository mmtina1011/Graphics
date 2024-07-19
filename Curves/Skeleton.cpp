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

const char* vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

	void main() {
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

const char* fragmentSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";


struct Camera {
	vec2 wCenter;
	vec2 wSize;
public:
	Camera() {
		Animate(0);
	}

	mat4 V() { return TranslateMatrix(-wCenter); }
	mat4 P() { 
		return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y));
	}
	mat4 Vinv() {
		return TranslateMatrix(wCenter);
	}
	mat4 Pinv() {
		return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2));
	}
	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }

	void Animate(float t) {
		wCenter.x = 0;
		wCenter.y = 0;
		wSize.x = 30;
		wSize.y = 30;
	}
};
// a weboldalon talált https://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/geometriai%20modellez%C3%A9s/bezier.cpp adott kiindulási alapot
Camera camera;
bool animate = false;
float tCurrent = 0;
GPUProgram gpuProgram;
const int nTesselatedVertices = 100;

class Curve {
	unsigned int vaoCurve, vboCurve;
	unsigned int vaoCtrlPoints, vboCtrlPoints;
	unsigned int vaoAnimatedObject, vboAnimatedObject;
protected:
	std::vector<vec4> wCtrlPoints;
public:
	Curve() {
		glGenVertexArrays(1, &vaoCurve);
		glBindVertexArray(vaoCurve);

		glGenBuffers(1, &vboCurve); 
		glBindBuffer(GL_ARRAY_BUFFER, vboCurve);
		
		glEnableVertexAttribArray(0); 
		
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);

		glGenVertexArrays(1, &vaoCtrlPoints);
		glBindVertexArray(vaoCtrlPoints);

		glGenBuffers(1, &vboCtrlPoints);
		glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);

		glEnableVertexAttribArray(0); 

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);

		glGenVertexArrays(1, &vaoAnimatedObject);
		glBindVertexArray(vaoAnimatedObject);

		glGenBuffers(1, &vboAnimatedObject); 
		glBindBuffer(GL_ARRAY_BUFFER, vboAnimatedObject);

		glEnableVertexAttribArray(0);  

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL); 

	}

	virtual vec4 r(float t) { return wCtrlPoints[0]; }
	virtual float tStart() { return 0; }
	virtual float tEnd() { return 1; }

	virtual void AddControlPoint(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		wCtrlPoints.push_back(wVertex);
	}

	int PickControlPoint(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		for (unsigned int p = 0; p < wCtrlPoints.size(); p++) {
			if (dot(wCtrlPoints[p] - wVertex, wCtrlPoints[p] - wVertex) < 0.1) return p;
		}
		return -1;
	}

	void MoveControlPoint(int p, float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		wCtrlPoints[p] = wVertex;
	}

	void Draw() {
		mat4 VPTransform = camera.V() * camera.P();

		gpuProgram.setUniform(VPTransform, "MVP");

		int colorLocation = glGetUniformLocation(gpuProgram.getId(), "color");

		if (wCtrlPoints.size() >= 2) {
			std::vector<float> vertexData;
			for (int i = 0; i < nTesselatedVertices; i++) {
				float tNormalized = (float)i / (nTesselatedVertices - 1);
				float t = tStart() + (tEnd() - tStart()) * tNormalized;
				vec4 wVertex = r(t);
				vertexData.push_back(wVertex.x);
				vertexData.push_back(wVertex.y);
			}

			glBindVertexArray(vaoCurve);
			glBindBuffer(GL_ARRAY_BUFFER, vboCurve);
			glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);
			if (colorLocation >= 0) glUniform3f(colorLocation, 1, 1, 0);
			glDrawArrays(GL_LINE_STRIP, 0, nTesselatedVertices);

			if (animate) {
			
				float t = tCurrent;
				while (t > tEnd()) t -= tEnd();
				vec4 currentLocation = r(t);
	
				glBindVertexArray(vaoAnimatedObject);
				glBindBuffer(GL_ARRAY_BUFFER, vboAnimatedObject);
				glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), &currentLocation, GL_DYNAMIC_DRAW);
				if (colorLocation >= 0) glUniform3f(colorLocation, 1, 1, 1);
				glPointSize(10);
				glDrawArrays(GL_POINTS, 0, 1);
			}
		}

		if (wCtrlPoints.size() > 0) {
			glBindVertexArray(vaoCtrlPoints);
			glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);
			glBufferData(GL_ARRAY_BUFFER, wCtrlPoints.size() * 4 * sizeof(float), &wCtrlPoints[0], GL_DYNAMIC_DRAW);
			if (colorLocation >= 0) glUniform3f(colorLocation, 1, 0, 0);
			glPointSize(10);
			glDrawArrays(GL_POINTS, 0, wCtrlPoints.size());
		}

	}
};


class BezierCurve : public Curve {
	float B(int i, float t) {
		int n = wCtrlPoints.size() - 1;
		float choose = 1;
		for (int j = 1; j <= i; j++) choose *= (float)(n - j + 1) / j;
		return choose * pow(t, i) * pow(1 - t, n - i);
	}
public:
	virtual vec4 r(float t) {
		vec4 wPoint = vec4(0, 0, 0, 0);
		for (unsigned int n = 0; n < wCtrlPoints.size(); n++) wPoint += wCtrlPoints[n] * B(n, t);
		return wPoint;
	}
};

//https://stackoverflow.com/questions/66559693/finding-equally-spaced-points-given-a-set-of-points-with-line-segments-in-betwee?fbclid=IwAR0YfW3XXK3ezAgAlU3TRLRHKpO4SC5UPyN0R5C_AEfdpR0u4Bk3PnxZw6w hogyan kéne csinálni
class LagrangeCurve : public Curve {
	std::vector<float> ts; 
	float L(int i, float t) {
		float Li = 1.0f;
		for (unsigned int j = 0; j < wCtrlPoints.size(); j++)
			if (j != i) Li *= (t - ts[j]) / (ts[i] - ts[j]);
		return Li;
	}
public:
	void AddControlPoint(float cX, float cY) {
		Curve::AddControlPoint(cX, cY);
		ts.clear();

		float totalLength = 0;
		ts.push_back(totalLength);

		for (int i = 1; i < wCtrlPoints.size(); ++i) {
			float segmLength = length(vec2(wCtrlPoints[i].x - wCtrlPoints[i - 1].x, wCtrlPoints[i].y - wCtrlPoints[i - 1].y));
			totalLength += segmLength;
			ts.push_back(totalLength);
		}

		for (int i = 0; i < ts.size(); ++i) {
			ts[i] /= totalLength;
		}


	}

	float tStart() { return ts[0]; }
	float tEnd() { return ts[wCtrlPoints.size() - 1]; }

	virtual vec4 r(float t) {
		vec4 wPoint = vec4(0, 0, 0, 0);
		for (unsigned int n = 0; n < wCtrlPoints.size(); n++) wPoint += wCtrlPoints[n] * L(n, t);
		return wPoint;
	}
};

float static tension = 0;



//moodle oldalon feltüntetett YouTube csatorna egyik videója alapján: https://www.youtube.com/watch?v=XPuOS39qadE
class CatmullRomSpline : public Curve {
	std::vector<float> ts; 

	vec4 Hermite(vec4 p0, vec4 v0, float t0, vec4 p1, vec4 v1, float t1, float t) {
		float delta = t1 - t0;
		t -= t0;
		float delta2 = delta * delta;
		float delta3 = delta * delta2;
		vec4 a0 = p0, a1 = v0;		
		vec4 a2 = (p1 - p0) * 3 / delta2 - (v1 + v0 * 2) / delta;
		vec4 a3 = (p0 - p1) * 2 / delta3 + (v1 + v0) / delta2;
		return ((a3 * t + a2) * t + a1) * t + a0;
	}
public:
	void AddControlPoint(float cx, float cy) {
		Curve::AddControlPoint(cx, cy);
		ts.clear();
		float totalLength = 0;
		ts.push_back(totalLength);

		for (int i = 1; i < wCtrlPoints.size(); ++i) {
			float segmLength = length(vec2(wCtrlPoints[i].x - wCtrlPoints[i - 1].x, wCtrlPoints[i].y - wCtrlPoints[i - 1].y));
			totalLength += segmLength;
			ts.push_back(totalLength);
		}
		for (int i = 0; i < ts.size(); ++i) {
			ts[i] /= totalLength;
		}

	}

	float tStart() { return ts[0]; }
	float tEnd() { return ts[wCtrlPoints.size() - 1]; }

	vec4 r(float t) {
		vec4 wPoint = vec4(0, 0, 0, 0);
		for (int i = 0; i < wCtrlPoints.size() - 1; ++i)	
			if (ts[i] <= t && t <= ts[i + 1]) {				

				vec4 vPrev = (i > 0) ? (wCtrlPoints[i] - wCtrlPoints[i - 1]) * (1.0f / (ts[i] - ts[i - 1])) : vec4(0, 0, 0, 0);

				vec4 vCur = (wCtrlPoints[i + 1] - wCtrlPoints[i]) / (ts[i + 1] - ts[i]);
				vec4 vNext = (i < wCtrlPoints.size() - 2) ? (wCtrlPoints[i + 2] - wCtrlPoints[i + 1]) / (ts[i + 2] - ts[i + 1]) : vec4(0, 0, 0, 0);	
		
				vec4 kezdoSebesseg = (vPrev + vCur) * ((1 - tension) / 2);
				vec4 vegSebesseg = (vNext + vCur) * ((1 - tension) / 2);

				return Hermite(wCtrlPoints[i], kezdoSebesseg, ts[i], wCtrlPoints[i + 1], vegSebesseg, ts[i + 1], t);
			}

		return wPoint;
	}
};

Curve* curve;

void onInitialization() {

	glViewport(0, 0, windowWidth, windowHeight);
	glLineWidth(2);
	
	curve = new Curve();

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


void onDisplay() {
	glClearColor(0, 0, 0, 0);							
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	curve->Draw();

	glutSwapBuffers();						
}

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'b':
		delete curve;
		curve = new BezierCurve();
		break;
	case 'l':
		delete curve;
		curve = new LagrangeCurve();
		break;
	case 'c':
		delete curve;
		curve = new CatmullRomSpline();
		break;
	case 'Z':
		camera.Zoom(1.1);
		break;
	case 'z':
		camera.Zoom(1 / 1.1);
		break;
	case 'P':
		camera.Pan(vec2(1, 0));
		break;
	case 'p':
		camera.Pan(vec2(-1, 0));
		break;
	case 'T':
		tension += 0.1;
		break;
	case 't':
		tension -= 0.1;
		break;
	}
	glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

int pickedControlPoint = -1;

void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  
		float cX = 2.0f * pX / windowWidth - 1;	
		float cY = 1.0f - 2.0f * pY / windowHeight;
		curve->AddControlPoint(cX, cY);
		glutPostRedisplay();     
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {  
		float cX = 2.0f * pX / windowWidth - 1;	
		float cY = 1.0f - 2.0f * pY / windowHeight;
		pickedControlPoint = curve->PickControlPoint(cX, cY);
		glutPostRedisplay(); 
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) { 
		pickedControlPoint = -1;
	}
}

void onMouseMotion(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	if (pickedControlPoint >= 0) curve->MoveControlPoint(pickedControlPoint, cX, cY);
	glutPostRedisplay();
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); 
	tCurrent = time / 1000.0f;		
	glutPostRedisplay();			
}