#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <deque>

GLUquadric* quad = gluNewQuadric();
bool forward = true;

struct Vec3 {
	double v[3];
	Vec3() { v[0] = v[1] = v[2] = 0; }
	Vec3(double x, double y, double z) { v[0] = x; v[1] = y; v[2] = z; }
	double operator[](int i) const { return v[i]; }
	double& operator[](int i) { return v[i]; }
	Vec3 operator+(const Vec3& other) const {
		return Vec3(v[0] + other[0], v[1] + other[1], v[2] + other[2]);
	}
	Vec3& operator+=(const Vec3& other) {
		v[0] += other[0]; v[1] += other[1]; v[2] += other[2];
		return *this;
	}
};

struct Comet {
	Vec3 startPos;
	Vec3 endPos;
	double t;
	double speed;
	double size;
	Vec3 color;
	std::deque<Vec3> trail;

	Vec3 getPosition() const {
		return Vec3(
			startPos[0] + t * (endPos[0] - startPos[0]),
			startPos[1] + t * (endPos[1] - startPos[1]),
			startPos[2] + t * (endPos[2] - startPos[2])
		);
	}
};

#ifdef _DEBUG
#include <Debugapi.h> 
struct debug_print {
	template<class C>
	debug_print& operator<<(const C& a) {
		OutputDebugStringA((std::stringstream() << a).str().c_str());
		return *this;
	}
} debout;
#else
struct debug_print {
	template<class C>
	debug_print& operator<<(const C& a) { return *this; }
} debout;
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

bool texturing = true;
bool lightning = true;
bool alpha = false;

void switchModes(OpenGL* sender, KeyEventArg arg) {
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
	switch (key) {
	case 'L': lightning = !lightning; break;
	case 'T': texturing = !texturing; break;
	case 'A': alpha = !alpha; break;
	}
}

GuiTextRectangle text;
GLuint texId;
GLuint textureID;

void initRender() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);

	int x, y, n;
	unsigned char* data = stbi_load("planet-normals.png", &x, &y, &n, 4);

	unsigned char* _tmp = new unsigned char[x * 4];
	for (int i = 0; i < y / 2; ++i) {
		std::memcpy(_tmp, data + i * x * 4, x * 4);
		std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4);
		std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4);
	}
	delete[] _tmp;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	int width, height, nrChannels;
	unsigned char* dat = stbi_load("kamen.png", &width, &height, &nrChannels, 0);

	if (dat) {
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, dat);
		gluBuild2DMipmaps(GL_TEXTURE_2D, format == GL_RGBA ? 4 : 3, width, height, format, GL_UNSIGNED_BYTE, dat);


		stbi_image_free(dat);
	}
	else {
		std::cerr << "Не удалось загрузить текстуру kamen.png" << std::endl;
	}

	camera.caclulateCameraPos();
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);

	camera.setPosition(7, 1.5, 1.5);
}

// === Перемещение и границы управляемого метеорита ===
Vec3 controlledMeteorPos(7.0, 7.0, 0.0);
Vec3 controlledMeteorVelocity(0.0, 0.0, 0.0);
const double controlledMeteorSpeed = 10.0;

const double minBound = -20.0;
const double maxBound = 20.0;

void processInput(double delta_time) {

	Vec3 direction(0, 0, 0);
	if (gl.isKeyPressed('W')) direction[1] += 1;
	if (gl.isKeyPressed('S')) direction[1] -= 1;
	if (gl.isKeyPressed('A')) direction[0] -= 1;
	if (gl.isKeyPressed('D')) direction[0] += 1;
	if (gl.isKeyPressed('Q')) direction[2] += 1;
	if (gl.isKeyPressed('E')) direction[2] -= 1;

	double len = sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
	if (len > 0.001) {
		direction[0] /= len;
		direction[1] /= len;
		direction[2] /= len;
	}

	controlledMeteorVelocity = Vec3(
		direction[0] * controlledMeteorSpeed,
		direction[1] * controlledMeteorSpeed,
		direction[2] * controlledMeteorSpeed
	);

	controlledMeteorPos += Vec3(
		controlledMeteorVelocity[0] * delta_time,
		controlledMeteorVelocity[1] * delta_time,
		controlledMeteorVelocity[2] * delta_time
	);

	for (int i = 0; i < 3; ++i) {
		if (controlledMeteorPos[i] < minBound)
			controlledMeteorPos[i] = minBound;
		else if (controlledMeteorPos[i] > maxBound)
			controlledMeteorPos[i] = maxBound;
	}

	double dist = sqrt(
		controlledMeteorPos[0] * controlledMeteorPos[0] +
		controlledMeteorPos[1] * controlledMeteorPos[1] +
		controlledMeteorPos[2] * controlledMeteorPos[2]
	);

	const double moonRadius = 5.0;

	if (dist < moonRadius) {
		for (int i = 0; i < 3; ++i) {
			controlledMeteorPos[i] = controlledMeteorPos[i] * (moonRadius / dist);
		}
	}
}

void Render(double delta_time) {

	processInput(delta_time);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gl.isKeyPressed('F')) {
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();
	light.SetUpLight();

	glDisable(GL_LIGHTING);
	glColor3d(1, 1, 1);
	gluQuadricNormals(quad, GLU_NONE);

	static std::vector<std::tuple<Vec3, double, Vec3>> stars;

	if (stars.empty()) {
		for (int i = 0; i < 1000; ++i) {
			double size = 0.01 + (rand() % 300) / 1000.0;

			// Генерация спектрального класса
			int spectralType = rand() % 100;
			Vec3 color;

			if (spectralType < 1)        color = Vec3(0.7, 0.8, 1.0);   // O — 1%
			else if (spectralType < 5)   color = Vec3(0.75, 0.85, 1.0); // B — 4%
			else if (spectralType < 15)  color = Vec3(0.85, 0.9, 1.0);  // A — 10%
			else if (spectralType < 30)  color = Vec3(1.0, 1.0, 1.0);   // F — 15%
			else if (spectralType < 60)  color = Vec3(1.0, 1.0, 0.8);   // G — 30%
			else if (spectralType < 85)  color = Vec3(1.0, 0.9, 0.6);   // K — 25%
			else                         color = Vec3(1.0, 0.7, 0.7);   // M — 15%

			stars.push_back({
				Vec3(
					(rand() % 2000 - 1000) / 10.0,
					(rand() % 2000 - 1000) / 10.0,
					(rand() % 2000 - 1000) / 10.0
				),
				size,
				color
				});
		}
	}

	for (const auto& s : stars) {
		glPushMatrix();
		glTranslated(std::get<0>(s)[0], std::get<0>(s)[1], std::get<0>(s)[2]);
		glColor3d(std::get<2>(s)[0], std::get<2>(s)[1], std::get<2>(s)[2]);
		gluSphere(quad, std::get<1>(s), 6, 6);
		glPopMatrix();
	}

	// === Скопление звёзд ===
	static std::vector<Vec3> milky;
	if (milky.empty()) {
		for (int i = 0; i < 2000; ++i) {
			milky.push_back(Vec3(
				(rand() % 2000 - 500) / 10.0,
				(rand() % 2000 - 500) / 10.0,
				(rand() % 40 - 20) / 10.0
			));
		}
	}
	for (const auto& s : milky) {
		glPushMatrix();
		glTranslated(s[0], s[1], s[2]);
		gluSphere(quad, 0.03, 6, 6);
		glPopMatrix();
	}

	// === Кометы ===
	static std::vector<Comet> comets;
	if (comets.empty()) {
		comets.push_back({ Vec3(-1000, 10, 16), Vec3(1000, -50, 0), 0.0, 0.1, 0.3, Vec3(1.0, 0.8, 0.6) });
		comets.push_back({ Vec3(-56, 560, 20), Vec3(-56, -560, 10), 0.0, 0.2, 0.25, Vec3(0.8, 1.0, 0.8) });
		comets.push_back({ Vec3(1000, 0, -12), Vec3(-100, 5, -12), 0.0, 0.2, 0.35, Vec3(0.9, 0.9, 1.0) });
		comets.push_back({ Vec3(-56, -560, 300), Vec3(-6, 56, -100), 0.0, 0.15, 0.25, Vec3(0.8, 1.0, 0.8) });
	}

	for (auto& comet : comets) {
		comet.t += comet.speed * delta_time;
		if (comet.t > 1.0) comet.t = 0.0;

		Vec3 pos = comet.getPosition();

		comet.trail.push_front(pos);
		if (comet.trail.size() > 100)
			comet.trail.pop_back();

		glPushMatrix();
		glTranslated(pos[0], pos[1], pos[2]);
		glColor3d(comet.color[0], comet.color[1], comet.color[2]);
		gluSphere(quad, comet.size, 10, 10);
		glPopMatrix();

		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		int i = 0;
		for (const Vec3& p : comet.trail) {
			double alpha = 1.0 - (double)i / comet.trail.size();
			double scale = comet.size * 0.5 * alpha;

			glColor4d(comet.color[0], comet.color[1], comet.color[2], alpha);
			glPushMatrix();
			glTranslated(p[0], p[1], p[2]);
			gluSphere(quad, scale, 6, 6);
			glPopMatrix();

			i++;
		}

		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
	}

	// === Управляемый метеорит ===
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLUquadric* quad = gluNewQuadric();
	gluQuadricTexture(quad, GL_TRUE);

	glPushMatrix();
	glTranslated(controlledMeteorPos[0], controlledMeteorPos[1], controlledMeteorPos[2]);
	glColor3d(1.0, 1.0, 1.0);
	gluSphere(quad, 0.4, 24, 24);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	gluDeleteQuadric(quad);

	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	if (lightning) glEnable(GL_LIGHTING);
	if (texturing) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (alpha) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	float amb[] = { 0.2, 0.2, 0.1, 1. };
	float dif[] = { 0.4, 0.65, 0.5, 1. };
	float spec[] = { 0.9, 0.8, 0.3, 1. };
	float sh = 0.2f * 256;
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, sh);
	glShadeModel(GL_SMOOTH);

	// === Анимация орбитального движения НЛО вокруг Луны ===
	static double angle = 0;
	angle += delta_time * 0.5;

	double R = 6.0;

	double lat = sin(angle * 0.7) * 1.0 + 1.0;
	double lon = angle;

	double x = R * cos(lon) * sin(lat);
	double y = R * sin(lon) * sin(lat);
	double z = R * cos(lat);

	const double zMax = 6.0;
	const double flashZone = 0.000001;
	bool showBeam = (z > 5.8) && (z < zMax - flashZone);

	// === Мигание вспышки ===
	static double flash_timer = 0.0;
	static int flash_blinks = 0;
	static bool hasFlashed = false;
	const int max_blinks = 3;
	const double blink_interval = 0.2;
	bool showFlash = false;

	if (!hasFlashed && z >= zMax - flashZone) {
		flash_timer = 0.0;
		flash_blinks = max_blinks * 2;
		hasFlashed = true;
	}
	if (z < zMax - 1.0) {
		hasFlashed = false;
	}
	if (flash_blinks > 0) {
		flash_timer += delta_time;
		if (flash_timer >= blink_interval) {
			flash_timer = 0.0;
			flash_blinks--;
		}
		showFlash = (flash_blinks % 2 == 1);
	}

	glPushMatrix();
	glTranslated(x, y, z);

	// === Вспышка ===
	if (showFlash) {
		glPushMatrix();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);

		glColor4d(1.0, 1.0, 1.0, 0.8);

		double fx = 0, fy = 0, fz = -1;
		double flash_len = 2.0;

		glRotated(180.0, 1.0, 0.0, 0.0);

		GLUquadric* flash = gluNewQuadric();
		gluQuadricNormals(flash, GLU_NONE);
		gluCylinder(flash, 0.0, 2.0, flash_len, 24, 1);

		gluDeleteQuadric(flash);

		glEnable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	// === Световой конус ===
	if (showBeam) {
		glPushMatrix();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);

		glColor4d(0.5, 0.8, 1.0, 0.4);

		double dx = -x;
		double dy = -y;
		double dz = -z;

		double len = sqrt(dx * dx + dy * dy + dz * dz);
		const double moon_radius = 4.5;
		double beam_len = len - moon_radius;

		if (beam_len > 0.0001) {
			double nx = dx / len;
			double ny = dy / len;
			double nz = dz / len;

			double rx = -ny;
			double ry = nx;
			double rz = 0;

			double dot = nz;
			double angle = acos(dot) * 180.0 / 3.14159265;

			if (rx != 0 || ry != 0 || rz != 0)
				glRotated(angle, rx, ry, rz);

			GLUquadric* beam = gluNewQuadric();
			gluQuadricNormals(beam, GLU_NONE);
			gluCylinder(beam, 0.0, 1.5, beam_len, 16, 1);

			gluDeleteQuadric(beam);
		}

		glEnable(GL_LIGHTING);
		glDisable(GL_BLEND);

		glPopMatrix();
	}

	glRotated(180, 1, 0, 0);

	// === НЛО ===
	glPushMatrix();
	glColor3d(0.5, 0.5, 0.8);
	gluQuadricNormals(quad, GLU_SMOOTH);
	glScaled(1.0, 1.0, 0.5);
	gluSphere(quad, 1.0, 30, 30);
	glPopMatrix();

	glPushMatrix();
	glColor3d(0.4, 0.4, 0.6);
	glTranslated(0, 0, -0.3);
	glRotated(180, 1, 0, 0);
	glScaled(0.7, 0.7, 0.35);
	gluSphere(quad, 1.0, 30, 30);
	glPopMatrix();

	glPushMatrix();
	glColor3d(0.3, 1.0, 0.3);
	glRotated(90, 0, 0, 1);
	glNormal3d(1.0, 0.0, 0.0);
	gluDisk(quad, 1, 1.5, 40, 1);
	glPopMatrix();

	glPopMatrix();

	// === Луна ===
	static double moonRotation = 0;
	moonRotation += delta_time * 2.0;

	glPushMatrix();
	glRotated(moonRotation, 0, 0, -1);
	glBindTexture(GL_TEXTURE_2D, texId);
	gluQuadricTexture(quad, GL_TRUE);
	gluQuadricNormals(quad, GLU_SMOOTH);
	gluSphere(quad, 4.5, 30, 30);
	glPopMatrix();

	light.DrawLightGizmo();

	// === GUI текст ===
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур\n";
	ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение\n";
	ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение\n";
	ss << L"F - Свет из камеры\n";
	ss << L"G - двигать свет по горизонтали\n";
	ss << L"G+ЛКМ двигать свет по вертикали\n";
	ss << L"Свет: (" << light.x() << ", " << light.y() << ", " << light.z() << ")\n";
	ss << L"Камера: (" << camera.x() << ", " << camera.y() << ", " << camera.z() << ")\n";
	ss << L"R=" << camera.distance() << ", fi1=" << camera.fi1() << ", fi2=" << camera.fi2() << "\n";
	ss << L"delta_time: " << std::setprecision(5) << delta_time << "\n";

	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());
	text.Draw();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}