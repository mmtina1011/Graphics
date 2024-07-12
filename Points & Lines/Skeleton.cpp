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

	layout(location = 0) in vec3 vp;	

	void main() {
		gl_Position = vec4(vp.x, vp.y, vp.z, 1);
	}
)";

const char * const fragmentSource = R"(
	#version 330			
	precision highp float;
	
	uniform vec3 color;
	out vec4 outColor;

	void main() {
		outColor = vec4(color, 1);
	}
)";

enum State {
	startState,
	p_draw,
	l_define_first,
	l_define_second,
	l_move,
	l_move_select,
	l_intersect_first,
	l_intersect_second,
};

GPUProgram gpuProgram;
State pState;
int idx;
vec3 p1, p2;
float startPointX, startPointY;
vec3 startP1, startP2;

class Object {
	unsigned int vao, vbo;
	std::vector<vec3> vtx;
public:
	void create() {
		glGenVertexArrays(1, &vao);  glBindVertexArray(vao);
		glGenBuffers(1, &vbo);  glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}
	std::vector<vec3>& Vtx() { return vtx; }

	void updateGPU() {
		glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_STATIC_DRAW);
	}

	virtual void draw(int type, vec3 color) {
		updateGPU();

		gpuProgram.setUniform(color, "color");
		glDrawArrays(type, 0, vtx.size());
	}

};

float pointPointDist(vec2 p1, vec2 p2) {
	return length(vec2(p2.x - p1.x, p2.y - p1.y));
}

vec3 dirVec(vec3 p1, vec3 p2) {
	return vec3(p2.x - p1.x, p2.y - p1.y, 0);
}

vec3 getImplicit(vec3 p1, vec3 p2) {
	float a = p2.y - p1.y;
	float b = p1.x - p2.x;
	float c = (-1)*a * p1.x + (-1)*b * p1.y;
	return vec3(a, b, c);
}


class PointCollection : public Object{
public:
	vec3* getNearPoint(vec2 point, float limit = 0.02f) {
		for (int i = 0; i < Vtx().size(); i++) {
			vec2 p = vec2(Vtx()[i].x, Vtx()[i].y);
			if (pointPointDist(point, p) <= limit) return &(Vtx()[i]);
		}
		return nullptr;
	}

	void draw() {
		Object::draw(GL_POINTS, vec3(1,0,0));
	}
	void addPoint(vec3 p) {
		Vtx().push_back(p);
	}
};



class Line {
	vec3 start, end;
public:

	Line(vec3 p1, vec3 p2) {
		start = p1;
		end = p2;
		vec3 i = getImplicit(p1, p2);

		float vx = dirVec(p1, p2).x;
		float vy = dirVec(p1, p2).y;

		printf("\tImplicit: %3.2f x + %3.2f y + %3.2f = 0\n", i.x, i.y, i.z);
		printf("\tParametric: r(t) = (%3.2f, %3.2f) + (%3.2f, %3.2f)t\n", p1.x, p1.y, vx, vy);
	}

	vec3 intersection(Line l) {
		return vec3(-2, -2, -2);
	}
};




class LineCollection : public Object {
public:
	void addLine(vec3 p1, vec3 p2) {

		vec3 vector = dirVec(p1, p2);

		vec3 outOfBox = 4*normalize(vector);

		Vtx().push_back(p1 + outOfBox);
		Vtx().push_back(p2 - outOfBox);
	}


	int getNearLine(vec3 point, float limit = 0.01f) {
		for (int idx = 0; idx < Vtx().size(); idx += 2) 
		{
			vec3 p1 = vec3(Vtx()[idx].x, Vtx()[idx].y, 1);
			vec3 p2 = vec3(Vtx()[idx + 1].x, Vtx()[idx + 1].y, 1);

			float a = p2.y - p1.y;
			float b = p1.x - p2.x;
			float c = (-1) * a * p1.x + (-1) * b * p1.y;

			float help = a * point.x + b * point.y + c;

			float distance = fabsf(help) / sqrtf(a * a + b * b);

			if (distance <= limit) return idx;

		}
		return -1;
	}

	void draw() {
		Object::draw(GL_LINES, vec3(0,1,1));
	}
};
vec3 intersectedWhere(vec3 p1, vec3 p2, vec3 p3, vec3 p4) {
	float x1 = p1.x,
		y1 = p1.y,
		x2 = p2.x,
		y2 = p2.y,
		x3 = p3.x,
		y3 = p3.y,
		x4 = p4.x,
		y4 = p4.y;


	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
//keplet a https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection alapjan
	float x_intersec = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / d;
	float y_intersec = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / d;

	return vec3(x_intersec, y_intersec, 1);
}



PointCollection pointColl;
LineCollection lineColl;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glPointSize(10.0f);
	glLineWidth(3.0f);

	pState = startState;
	pointColl = PointCollection();
	pointColl.create();
	lineColl = LineCollection();
	lineColl.create();

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


void onDisplay() {
	glClearColor(0.3f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	lineColl.draw();
	pointColl.draw();

	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'p':
		pState = p_draw;
		break;
	case 'l':
		pState = l_define_first;
		printf("Define lines\n");
		break;
	case 'm':
		pState = l_move_select;
		printf("Move\n");
		break;
	case 'i':
		pState = l_intersect_first;
		printf("Intersect\n");
		break;
	}
	glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (pState == l_move) {
		float deltaX = cX - startPointX;
		float deltaY = cY - startPointY;
		vec3 v = vec3(deltaX, deltaY);

		lineColl.Vtx()[idx] = startP1 + v;
		lineColl.Vtx()[idx + 1] = startP2 + v;
		
		glutPostRedisplay();
		}
}


void onMouse(int button, int state, int pX, int pY) {
	static vec3* first_click;
	vec3* second_click;
	static vec3 impl1;
	int where1, where2;

	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON && pState == l_move_select) {

		idx = lineColl.getNearLine(vec3(cX, cY, 1));

		if (idx > -1) {
			pState = l_move;

			startPointX = cX;
			startPointY = cY;
			startP1 = lineColl.Vtx()[idx];
			startP2 = lineColl.Vtx()[idx + 1];
		}
	}

	if (!(state == GLUT_UP && button == GLUT_LEFT_BUTTON)) 
		return;

	switch (pState) {
	case p_draw:
		pointColl.addPoint(vec3(cX, cY, 1));
		printf("Point %3.2f, %3.2f added\n", cX, cY);
		break;

	case l_define_first:
		first_click = pointColl.getNearPoint(vec2(cX, cY));

		if (first_click != nullptr) {
			pState = l_define_second;
		}
		break;

	case l_define_second:
		second_click = pointColl.getNearPoint(vec2(cX, cY));

		if (second_click != nullptr && second_click != first_click) {
			pState = l_define_first;
			lineColl.addLine(*first_click, *second_click);
			printf("Line added\n");
			Line(*first_click, *second_click);
			pState = l_define_first;
		}
		break;

	case l_move:

		pState = l_move_select;

		break;

	case l_intersect_first:

		where1 = lineColl.getNearLine(vec3(cX, cY, 1));

		if (where1 > -1) {
			p1 = lineColl.Vtx()[where1];
			p2 = lineColl.Vtx()[where1 + 1];

			impl1 = getImplicit(p1, p2);
			pState = l_intersect_second;
		}

		break;

	case l_intersect_second:
		where2 = lineColl.getNearLine(vec3(cX, cY, 1));
		if (where2 > -1) 
		{
			vec3 p3 = lineColl.Vtx()[where2];
			vec3 p4 = lineColl.Vtx()[where2 + 1];

			vec3 pp = intersectedWhere(p1,p2,p3,p4);

			pointColl.addPoint(pp);
			printf("Point %3.2f, %3.2f added\n", pp.x, pp.y);

			pState = l_intersect_first;
		}
		break;

	glutPostRedisplay();
	}
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
}
