/*This source code copyrighted by Lazy Foo' Productions (2004-2022)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <iostream>
#include <vector>
#include <limits>
#include <chrono>
#include <thread>
#include <SDL_thread.h>

//Screen dimension constants
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

// The renderer tied to the window
SDL_Renderer* gRenderer = nullptr;

//The image we will load and show on the screen
SDL_Surface* gHelloWorld = NULL;

//Main loop flag
bool quit = false;

//Event handler
SDL_Event e;

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Create window
		//gWindow = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
		//gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
		SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &gWindow, &gRenderer);

		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface(gWindow);
		}
	}

	return success;
}


void UpdateWindow() {
	SDL_UpdateWindowSurface(gWindow);
}

void loadBMPToWindow(const char* bmpPath) {
	SDL_Surface* surface = SDL_LoadBMP(bmpPath);
	
	SDL_BlitSurface(surface, NULL, gScreenSurface, NULL);
	UpdateWindow();

	SDL_FreeSurface(surface);
}

void close()
{
	//Deallocate surface
	SDL_FreeSurface(gHelloWorld);
	gHelloWorld = NULL;

	//Destroy window
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}


void changeWindowBG(Uint8 r, Uint8 g, Uint8 b) {
	//Fill the surface white
	SDL_FillRect(gScreenSurface, NULL, SDL_MapRGB(gScreenSurface->format, r, g, b));

	//Update the surface
	UpdateWindow();
}

struct RGBA {
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t A;

	RGBA() {
		R = 0; G = 0; B = 0; A = 0;
	}
	RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		R = r; G = g; B = b; A = a;
	}

	RGBA operator+(const RGBA &color) const {
		return RGBA(this->R + color.R, this->G + color.G, this->B + color.B, this->A);
	}
	RGBA operator*(double i) {
		return RGBA(this->R * i, this->G * i, this->B * i, this->A);
	}
};


struct Vec3 {
	double x;
	double y;
	double z;

	Vec3 operator+(const Vec3& vec1) const {
		double x = this->x + vec1.x;
		double y = this->y + vec1.y;
		double z = this->z + vec1.z;
		return Vec3{ x, y, z };
	}

	Vec3 operator-(const Vec3& vec1) const {
		double x = this->x - vec1.x;
		double y = this->y - vec1.y;
		double z = this->z - vec1.z;
		return Vec3{ x, y, z };
	}

	Vec3 operator*(double i) const {
		return Vec3{ this->x * i, this->y * i, this->z * i };
	}

	Vec3 operator/(double i) const {
		return Vec3{ this->x / i, this->y / i, this->z / i };
	}

	static double Dot(const Vec3 &vec1, const Vec3 &vec2) {
		return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
	}
	static double Length(const Vec3& vec) {
		return sqrt(Vec3::Dot(vec, vec));
	}
};

void loadTextureToRenderer(const char* bmpPath) {
	SDL_Surface* tempSurface = SDL_LoadBMP(bmpPath);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);

	SDL_RenderCopy(gRenderer, texture, nullptr, nullptr);

	SDL_FreeSurface(tempSurface);
	SDL_DestroyTexture(texture);
}

double distance(Vec3 from, Vec3 to) {
	return sqrt(pow(from.y - to.y, 2) + pow(from.x - to.x, 2));
}

void drawPoint(const Vec3& pt, const RGBA &color) {
	SDL_SetRenderDrawColor(gRenderer, color.R, color.G, color.B, 255);
	SDL_RenderDrawPoint(gRenderer, pt.x, pt.y);
}

Vec3 ScreenToCanvas(const Vec3 &pt) {
	double x = SCREEN_WIDTH / 2 + pt.x;
	double y = SCREEN_HEIGHT / 2 - pt.y;
	double z = pt.z;
	return Vec3{x, y, z};
}

// convert computer coordinates:
// x: 0 -> screen_width (left - right)
// y: 0 -> screen_height (top - bottom)
// to cartesian plane
Vec3 ScreenToCanvas2(const Vec3& pt) {
	double x = pt.x - SCREEN_WIDTH / 2;
	double y = -(pt.y - (SCREEN_HEIGHT / 2 + 1));
	double z = pt.z;
	return Vec3{ x, y, z };
}

void drawLine(const Vec3&from, const Vec3&to, const RGBA &color) {
	Vec3 currPt{};
	for (float t = 0.; t < 1.; t += .01) {
		currPt.x = from.x + (to.x - from.x) * t;
		currPt.y = from.y + (to.y - from.y) * t;

		drawPoint(ScreenToCanvas(currPt), color);
	}

	//loadTextureToRenderer("hello_world.bmp");
	SDL_RenderPresent(gRenderer);
}

struct Viewport {
	double width=1;
	double height=1;
	double distance=1;
};
Viewport viewport{};


struct Sphere {
	Vec3 center;
	double radius;
	RGBA color;
	double specular;
	double reflectiveness;

	Sphere(Vec3 cPt, double r, RGBA c, double s, double reflective)
		: center(cPt), radius(r), color(c), specular(s), reflectiveness(reflective)
	{

	}
};

class Light {
public:
	double intensity;
	virtual double Illuminate(const Vec3& intersectPt, const Vec3& normal, const Vec3& viewDir, double specular) = 0;
protected:
	Light(double i) :
		intensity(i) {}
};


struct Camera {
	Vec3 Position{ 0, 0, -5 };
};
Camera camera{};

struct Scene {
	std::vector<Sphere*> Spheres;
	std::vector<Light*> Lights;
};
Scene scene{};


Vec3 CanvasToViewport(const Vec3& cvs) {
	return Vec3{ 
		cvs.x * viewport.width / SCREEN_WIDTH,
		cvs.y * viewport.height / SCREEN_HEIGHT,
		viewport.distance};
}

Vec3 ReflectRay(const Vec3 &ray, const Vec3 &normal) {
	return normal * Vec3::Dot(normal, ray) * 2 - ray;
}

double ComputeLighting(const Vec3& intersectPt, const Vec3& normal, const Vec3& viewDir, double specular) {
	double i = 0.0;
	for (Light* light : scene.Lights)
		i += light->Illuminate(intersectPt, normal, viewDir, specular);
	return i;
}

std::pair<double, double> IntersectRaySphere(const Vec3& O, const Vec3& D, const Sphere* sphere) {
	double r = sphere->radius;
	Vec3 CO = O - sphere->center;

	double a = Vec3::Dot(D, D);
	double b = 2 * Vec3::Dot(CO, D);
	double c = Vec3::Dot(CO, CO) - r * r;

	double discriminant = b * b - 4 * a * c;
	if (discriminant < 0) {
		return std::pair<double, double>(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
	}

	double t1 = (-b + sqrt(discriminant)) / (2 * a);
	double t2 = (-b - sqrt(discriminant)) / (2 * a);
	return std::pair<double, double>(t1, t2);
}

std::pair<Sphere*, double> ClosestIntersection(const Vec3& O, const Vec3& D, const double& t_min, const double& t_max) {
	double closest_t = std::numeric_limits<double>::infinity();
	Sphere* closest_sphere = nullptr;
	for (Sphere*& sphere : scene.Spheres) {
		std::pair<double, double> t = IntersectRaySphere(O, D, sphere);
		if (t.first >= t_min && t.first < t_max && t.first < closest_t) {
			closest_t = t.first;
			closest_sphere = sphere;
		}
		if (t.second >= t_min && t.second < t_max && t.second < closest_t) {
			closest_t = t.second;
			closest_sphere = sphere;
		}
	}

	return std::pair<Sphere*, double>{ closest_sphere, closest_t };
}


class Point : public Light {
public:
	Vec3 Position;

	Point(double i, Vec3 pos) :
		Light(i), Position(pos) {}

	double Illuminate(const Vec3& intersectPt, const Vec3& normal, const Vec3& viewDir, double specular) override {
		Vec3 lightRay = Position - intersectPt;
		double total_intensity = 0;

		// Shadow check
		std::pair<Sphere*, double> closest = ClosestIntersection(intersectPt, lightRay, 0.001, 1);
		Sphere* shadow_sphere = closest.first;
		double shadow_t = closest.second;
		if (shadow_sphere != nullptr)
			return 0;

		// diffuse reflection
		double n_dot_l = Vec3::Dot(normal, lightRay);
		if (n_dot_l > 0)
			total_intensity += intensity * n_dot_l / (Vec3::Length(normal) * Vec3::Length(intersectPt));

		// specular reflection
		if (specular != -1) {
			Vec3 R(ReflectRay(lightRay, normal));
			double r_dot_v = Vec3::Dot(R, viewDir);
			if (r_dot_v > 0) {
				total_intensity += intensity * pow(r_dot_v / (Vec3::Length(R) * Vec3::Length(viewDir)), specular);
			}
		}

		return total_intensity;
	}
};

class Directional : public Light {
public:
	Vec3 Direction;

	Directional(double i, Vec3 dir) :
		Light(i), Direction(dir) {}

	double Illuminate(const Vec3& intersectPt, const Vec3& normal, const Vec3& viewDir, double specular) override {
		double n_dot_l = Vec3::Dot(normal, Direction);
		double total_intensity = 0;

		// Shadow check
		std::pair<Sphere*, double> closest = ClosestIntersection(intersectPt, Direction, 0.001, std::numeric_limits<double>::infinity());
		Sphere* shadow_sphere = closest.first;
		double shadow_t = closest.second;
		if (shadow_sphere != nullptr)
			return 0;

		// diffuse reflection
		if (n_dot_l > 0)
			total_intensity += intensity * n_dot_l / (Vec3::Length(normal) * Vec3::Length(intersectPt));

		// specular reflection
		if (specular != -1) {
			Vec3 R(ReflectRay(Direction, normal));
			double r_dot_v = Vec3::Dot(R, viewDir);
			if (r_dot_v > 0) {
				total_intensity += intensity * pow(r_dot_v / (Vec3::Length(R) * Vec3::Length(viewDir)), specular);
			}
		}

		return 0;
	}
};

class Ambient : public Light {
public:

	Ambient(double i) :
		Light(i) {}

	double Illuminate(const Vec3& intersectPt, const Vec3& normal, const Vec3& viewDir, double specular) override {
		return intensity;
	}
};


RGBA TraceRay(const Vec3 &O, const Vec3 &D, const double &t_min, const double &t_max, double reflection_limit=3) {
	std::pair<Sphere*, double> closest = ClosestIntersection(O, D, t_min, t_max);
	Sphere* closest_sphere = closest.first;
	double closest_t = closest.second;

	if (closest_sphere == nullptr)
		return RGBA(0, 0, 0, 0);

	Vec3 intersectPt = O + D * closest_t;
	Vec3 normal = intersectPt - closest_sphere->center;
	normal = normal / Vec3::Length(normal);
	double lightValue = ComputeLighting(intersectPt, normal, D * -1, closest_sphere->specular);
	if (lightValue > 1)
		lightValue = 1;
	
	RGBA resultingColor = closest_sphere->color * lightValue;

	// If we hit the recursion limit or the object is not reflective, we're done
	double r = closest_sphere->reflectiveness;
	if (reflection_limit <= 0 or r <= 0 )
		return resultingColor;

	// Compute the reflected color
	Vec3 reflectedRay = ReflectRay(D * -1, normal);
	RGBA reflected_color = TraceRay(intersectPt, reflectedRay, 0.001, std::numeric_limits<double>::infinity(), reflection_limit - 1);

	return resultingColor * (1 - r) + reflected_color * r;

	return closest_sphere->color * lightValue;
}

static std::vector<std::vector<RGBA*>> frame;

static double xRad;
static double yRad;
void renderPortion(int startX, int endX, int startY, int endY) {
	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			Vec3 screenPt = ScreenToCanvas2({ (double)x, (double)y });
			Vec3 canvasPt = ScreenToCanvas(screenPt);
			Vec3 O = camera.Position;
			Vec3 D = CanvasToViewport(screenPt);


			Vec3 rotatedX{
				D.x,
				D.y * cos(yRad) - sin(yRad) * D.z,
				sin(yRad) * D.y + cos(yRad) * D.z
			};

			Vec3 rotatedY{
				rotatedX.x * cos(xRad) + rotatedX.z * sin(xRad),
				rotatedX.y,
				-sin(xRad) * rotatedX.x + cos(xRad) * rotatedX.z
			};

			RGBA color = TraceRay(O, rotatedY, 0, std::numeric_limits<double>::infinity());


			RGBA* newColor = new RGBA{ color.R, color.G, color.B, color.A };
			frame[x][y] = newColor;

		}
	}
}

void renderSceneMultithreaded() {
	auto start = std::chrono::high_resolution_clock::now();

	int num_cpus = std::thread::hardware_concurrency();
	//num_cpus = 2;
	std::vector<std::thread> threads;


	for (int i = 0; i < num_cpus; i++) {
		int xBlock = SCREEN_WIDTH / num_cpus;
		int yBlock = SCREEN_HEIGHT / num_cpus;

		if (i == num_cpus - 1)
			threads.push_back(std::thread(renderPortion, 0, SCREEN_WIDTH, i * yBlock, i * yBlock + yBlock + SCREEN_HEIGHT % yBlock));
		else
			threads.push_back(std::thread(renderPortion, 0, SCREEN_WIDTH, i * yBlock, i * yBlock + yBlock));
	}
	for (int i = 0; i < num_cpus; i++) {
		threads[i].join();
	}

	SDL_RenderClear(gRenderer);

	for (int x = 0; x < SCREEN_WIDTH; x++) {
		for (int y = 0; y < SCREEN_HEIGHT; y++) {
			SDL_SetRenderDrawColor(gRenderer, frame[x][y]->R, frame[x][y]->G, frame[x][y]->B, 255);
			SDL_RenderDrawPoint(gRenderer, x, y);
			delete frame[x][y];
			//frame[x][y] = nullptr;
		}
	}
	SDL_RenderPresent(gRenderer);

	auto end = std::chrono::high_resolution_clock::now();


	std::chrono::duration<float> duration = end - start;
	float ms = duration.count() * 1000.0f;
	std::cout << "Rendering Took " << ms << " ms" << " |   " << 1000 / ms << " FPS" << std::endl;
}


void renderScene() {
	renderSceneMultithreaded();
	return;
	auto start = std::chrono::high_resolution_clock::now();
	 
	SDL_RenderClear(gRenderer);
	for (int x = -SCREEN_WIDTH / 2; x < SCREEN_WIDTH / 2; x++) {
		for (int y = -SCREEN_HEIGHT; y < SCREEN_HEIGHT / 2; y++) {
			Vec3 screenPt = Vec3{ (double)x, (double)y, 0 };
			Vec3 canvasPt = ScreenToCanvas(screenPt);
			Vec3 O = camera.Position;
			Vec3 D = CanvasToViewport(screenPt);


			RGBA color = TraceRay(O, D, 0, std::numeric_limits<double>::infinity());

			drawPoint(canvasPt, color);

			//std::cout << "Location: " << D.x << ", " << D.y << ", " << D.z << " | Color: " << color.r << ", " << color.g << ", " << color.b << "\n";
		}
	}
	SDL_RenderPresent(gRenderer);

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float> duration = end - start;
	float ms = duration.count() * 1000.0f;
	std::cout << "Rendering Took " << ms << " ms" << " |   " << 1000 / ms << " FPS" << std::endl;
}

int main(int argc, char* args[]) {
	init();
	frame.resize(SCREEN_WIDTH, std::vector<RGBA*>(SCREEN_HEIGHT, nullptr));
	//loadBMPToWindow("hello_world.bmp");

	//drawLine(Vec3{ 0, 0 }, Vec3{ 100, 100 }, RGBA{ 255, 0, 0, 0 });

	Vec3 test{ 3, 4 };
	std::cout << Vec3::Length(test) << std::endl;

	// define scene
	Sphere sphere1 = Sphere(Vec3{ 0, -1, 3 }, 1, RGBA(255, 0, 0, 0), -1, .13); // red
	Sphere sphere2 = Sphere(Vec3{ 2, 0, 4 }, 1, RGBA( 0, 0, 255, 0), 10, .1); // blue
	Sphere sphere3 = Sphere(Vec3{ -2, 0, 4 }, 1, RGBA(0, 255, 0, 0), -1, .1); // green
	Sphere sphere4 = Sphere(Vec3{ 2, 2, 7 }, 1.5, RGBA(178, 155, 100, 0), 50, 1); // brown
	Sphere sphere5 = Sphere(Vec3{ -1, 1, 4 }, 1, RGBA(33, 233, 128, 0), 300, .2); // cyan
	Sphere sphere6 = Sphere(Vec3{ -2, 0, 0 }, 1, RGBA(230, 230, 250, 0 ), 100, .1); // lavender
	
	// walls (actually spheres but very big)
	Sphere sphere7 = Sphere(Vec3{ 0, -5001, 0 }, 5000, RGBA(255, 255, 0, 0), -1, .5); // yellow

	scene.Spheres.push_back(&sphere1);
	scene.Spheres.push_back(&sphere2);
	scene.Spheres.push_back(&sphere3);
	scene.Spheres.push_back(&sphere4);
	scene.Spheres.push_back(&sphere5);
	scene.Spheres.push_back(&sphere6);

	scene.Spheres.push_back(&sphere7);
	// add lights
	Point* pLight = new Point{ 0.6, {2, 1, 0} }; // original code
	//Point* pLight = new Point{ .9, {3, 1, 0} };
	Directional dLight{ 0.35, {1, 4, 4} };
	Ambient aLight{ 0.05 };
	scene.Lights.push_back(pLight);
	scene.Lights.push_back(&dLight);
	scene.Lights.push_back(&aLight);

	// initial render
	renderScene();
	std::cout << "Done rendering!\n";


	// states
	bool panning = false;
	bool movingLeft = false;
	bool movingRight = false;
	bool movingForward = false;
	bool movingBackward = false;
	bool movingUp = false;
	bool movingDown = false;
	int mouseX{};
	int mouseY{};

	SDL_SetRelativeMouseMode(SDL_TRUE);

	while (!quit) {
		bool needsRender = false;
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			//User requests quit
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_MOUSEWHEEL) {
				//if (e.wheel.y > 0) // scroll up
				//if (e.wheel.y < 0) // scroll down

				//renderScene();
			}
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				SDL_GetMouseState(&mouseX, &mouseY);
				panning = true;

				std::cout << mouseX << ", " << mouseY << '\n';
			}
			if (e.type == SDL_MOUSEMOTION) {
				Vec3 screenRelOffset{ e.motion.xrel, e.motion.yrel };

				xRad += screenRelOffset.x / SCREEN_WIDTH * M_PI / 3;
				yRad += screenRelOffset.y / SCREEN_HEIGHT * M_PI / 3;


				needsRender = true;
			}
			if (e.type == SDL_MOUSEBUTTONUP) {

				panning = false;
			}
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_a)
					movingLeft = true;
				if (e.key.keysym.sym == SDLK_d)
					movingRight = true;
				if (e.key.keysym.sym == SDLK_w)
					movingForward = true;
				if (e.key.keysym.sym == SDLK_s)
					movingBackward = true;
				if (e.key.keysym.sym == SDLK_SPACE)
					movingUp = true;
				if (e.key.keysym.sym == SDLK_LSHIFT)
					movingDown = true;
			}

			if (e.type == SDL_KEYUP) {
				if (e.key.keysym.sym == SDLK_a)
					movingLeft = false;
				if (e.key.keysym.sym == SDLK_d)
					movingRight = false;
				if (e.key.keysym.sym == SDLK_w)
					movingForward = false;
				if (e.key.keysym.sym == SDLK_s)
					movingBackward = false;
				if (e.key.keysym.sym == SDLK_SPACE)
					movingUp = false;
				if (e.key.keysym.sym == SDLK_LSHIFT)
					movingDown = false;
			}
		}

		if (movingLeft) {
			Vec3 newPos{
				0, 0, 1
			};
			// rotated by Y-axis
			newPos = {
				newPos.x * cos(xRad - M_PI / 2) + newPos.z * sin(xRad - M_PI / 2),
				newPos.y,
				-sin(xRad - M_PI / 2) * newPos.x + cos(xRad - M_PI / 2) * newPos.z
			};


			camera.Position = camera.Position + newPos;
			needsRender = true;
		}
		if (movingRight) {
			Vec3 newPos{
				0, 0, 1
			};
			// rotated by Y-axis
			newPos = {
				newPos.x * cos(xRad + M_PI / 2) + newPos.z * sin(xRad + M_PI / 2),
				newPos.y,
				-sin(xRad + M_PI / 2) * newPos.x + cos(xRad + M_PI / 2) * newPos.z
			};


			camera.Position = camera.Position + newPos;
			needsRender = true;
		}
		if (movingForward) {
			Vec3 newPos{
				0, 0, 1
			};
			// rotated by Y-axis
			newPos = {
				newPos.x * cos(xRad) + newPos.z * sin(xRad),
				newPos.y,
				-sin(xRad) * newPos.x + cos(xRad) * newPos.z
			};
			camera.Position = camera.Position + newPos;

			needsRender = true;
		}
		if (movingBackward) {
			Vec3 newPos{
				0, 0, 1
			};
			// rotated by Y-axis
			newPos = {
				newPos.x * cos(xRad - M_PI) + newPos.z * sin(xRad - M_PI),
				newPos.y,
				-sin(xRad - M_PI) * newPos.x + cos(xRad - M_PI) * newPos.z
			};
			camera.Position = camera.Position + newPos;

			needsRender = true;
		}

		if (movingUp) {
			Vec3 newPos{
				0, 1, 0
			};
			camera.Position = camera.Position + newPos;

			needsRender = true;
		}
		if (movingDown) {
			Vec3 newPos{
				0, -1, 0
			};
			camera.Position = camera.Position + newPos;
			if (camera.Position.y < 0)
				camera.Position.y = 0;

			needsRender = true;
		}



		if (needsRender)
			renderScene();

	}
	
	close();

	return 0;
}