#include <iostream>
#include <tuple>
#include <queue>
#include <cmath>
#include <matrix.hpp>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <SFML/Audio.hpp>

#include <ow/shader_program.hpp>
#include <ow/camera_fps.hpp>
#include <ow/model.hpp>
#include <ow/utils.hpp>
#include <gui/window.hpp>
#include <eye_finder.hpp>

void process_input(gui::window& window, float dt);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// settings
constexpr unsigned int SCREEN_WIDTH = 800;
constexpr unsigned int SCREEN_HEIGHT = 600;

// initialize camera system
// ------------------------
ow::camera_fps camera{glm::vec3(0, 5, 15)};
float last_x = static_cast<float>(SCREEN_WIDTH) / 2.f;
float last_y = static_cast<float>(SCREEN_HEIGHT) / 2.f;
bool first_mouse = true;

std::optional<bool> process(const matrix<unsigned char>& pic, face f);

int main() {
	gui::window window{ "IN55", SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, nullptr };
	if (window.invalid()) {
		ow::logger << "Failed to create window" << std::endl;
		return EXIT_FAILURE;
	}

	window.make_context_current();
	window.set_framebuffer_size_callback(framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		ow::logger << "Failed to initialize GLAD" << std::endl;
		return EXIT_FAILURE;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	ow::check_errors("Failed to set GL_DEPTH_TEST.");

	// Init ImGui
	// ==========
	window.init_imgui();

	// load shaders
	// ------------
	ow::shader_program colorify_prog{{
        {GL_VERTEX_SHADER, "lamp_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "lamp_frag.glsl"}
    }};

	// load models
	// -----------
	ow::model nanosuit{"resources/models/nanosuit/nanosuit.obj"};

	// for each nanosuit, matrix model and color.
	std::vector<std::tuple<glm::mat4, glm::vec3>> nanosuit_models;
	nanosuit_models.reserve(2);
	{ // nanosuit 1
		glm::mat4 model{1.0f};
		model = glm::translate(model, glm::vec3(-5, 0, 0));
		model = glm::rotate(model, 0.25f, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(0.5));
		nanosuit_models.emplace_back(model, glm::vec3(1, 0, 0));
	} { // nanosuit 2
		glm::mat4 model{1.0f};
		model = glm::translate(model, glm::vec3(5, 0, 0));
		model = glm::rotate(model, -0.25f, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(0.5));
		nanosuit_models.emplace_back(model, glm::vec3(0, 1, 0));
	}

	// eye tracking stuff
	matrix<unsigned char> frame;
	eye_finder ef("../eye-tracking-lib/res/haarcascade_frontalface_alt.xml");
	cv::VideoCapture capture{0};
	if (!capture.isOpened()) {
		return -1;
	}
	capture.read(frame);
	cv::flip(frame, frame, 1);
	std::optional<face> f = ef.find_eyes(frame);

	// sounds
	sf::SoundBuffer red_buffer;
	sf::SoundBuffer green_buffer;
	if (!red_buffer.loadFromFile("../rouge.wav") || !green_buffer.loadFromFile("../vert.wav")) {
		return -1;
	}

	sf::Sound red_sound, green_sound;
	red_sound.setBuffer(red_buffer);
	green_sound.setBuffer(green_buffer);

	// game loop
	// -----------
	float delta_time;	// time between current frame and last frame
	float last_frame = 0.0f; // time of last frame
	float sound_time_accumulator = 0.0f;
	while (!window.should_close()) {
		auto current_frame = static_cast<float>(glfwGetTime());
		delta_time = current_frame - last_frame;
		last_frame = current_frame;

		// input
		// -----
		process_input(window, delta_time);

		// eye tracking
		// ------------
		sound_time_accumulator += delta_time;

		capture.read(frame);

		// mirror it
		cv::flip(frame, frame, 1);

		// Apply the classifier to the frame
		std::optional<bool> is_left;
		if (!frame.empty()) {
			f = ef.find_eyes(frame, f);
			if (f) {
				is_left = process(frame, f.value());
			}
		} else {
			printf(" --(!) No captured frame -- Break!");
			break;
		}

		if (sound_time_accumulator > 1) {
			if (is_left) {
				sound_time_accumulator = 0;
				if (*is_left) {
					red_sound.play();
				} else {
					green_sound.play();
				}
			}
		}

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		ow::check_errors("Failed to set clear color.");
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ow::check_errors("Failed to clear scr.");

		// create transformations
		glm::mat4 view = camera.get_view_matrix();
		glm::mat4 proj = camera.get_proj_matrix(static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT));
		glm::mat4 VP = proj * view;

		// activate shader program
		colorify_prog.use();

		// draw nanosuits
		for (auto model : nanosuit_models) {
			colorify_prog.set("MVP", VP * std::get<0>(model));
			colorify_prog.set("color", std::get<1>(model));
			nanosuit.draw(colorify_prog);
		}

		window.render();
	}

	return EXIT_SUCCESS;
}

// process all input: query GLFW whether relevant keys are pressed/released
// this frame and react accordingly
void process_input(gui::window& window, float dt) {
	if (window.get_key(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		window.set_should_close(true);
	}
}

// glfw: whenever the window size changed (by OS or user resize)
// this callback function executes
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

std::optional<bool> process(const matrix<unsigned char>& pic, face f) {
	static std::pair<cv::Point,cv::Point> corners[4];
	static std::optional<uint> to_draw;
	static unsigned int count = 0;

	cv::Rect rects[] =
			{
					{0,0, pic.width() / 8, pic.height() / 8},
					{0,pic.height() *7/ 8, pic.width() / 8, pic.height() / 8},
					{pic.width() *7/ 8, pic.height() *7/ 8, pic.width() / 8, pic.height() / 8},
					{pic.width() *7/ 8, 0, pic.width() / 8, pic.height() / 8},
			};

	char t = static_cast<char>(cv::waitKey(1));
	if (t == ' ') {
		if (count < 4) {
			corners[count++] = {f.eyes.first.eye_position, f.eyes.second.eye_position};
		}
	} else if (t == 27) {
		exit(0);
	}

	if (count >= 4) {
		double min = std::numeric_limits<double>::infinity();
		for (auto i = 0u ; i < 4 ; ++i) {
			double dist = std::hypot(f.eyes.first.eye_position.x - corners[i].first.x,
			                         f.eyes.first.eye_position.y - corners[i].first.y);
			dist += std::hypot(f.eyes.second.eye_position.x - corners[i].second.x,
			                   f.eyes.second.eye_position.y - corners[i].second.y);

			if (dist < min) {
				min = dist;
				to_draw = i;
			}
		}
	}


	f.eyes.first.eye_position.x += f.face_region.x;
	f.eyes.first.eye_position.y += f.face_region.y;
	f.eyes.second.eye_position.x += f.face_region.x;
	f.eyes.second.eye_position.y += f.face_region.y;

	matrix<unsigned char> display(pic.clone());
	cv::rectangle(display, f.face_region, CV_RGB(40,40,200));

	if (count < 4) {
		cv::rectangle(display, rects[count], cv::Scalar(40,220,40,0.3), -1);
	}

	cv::rectangle(display, f.face_region, CV_RGB(200, 0, 200));
	cv::circle(display, f.eyes.first.eye_position, 2, CV_RGB(40,40,200), 2);
	cv::circle(display, f.eyes.second.eye_position, 2, CV_RGB(200,40,40), 2);

	cv::imshow("aiue", display);

	if (to_draw) {
		return !(to_draw.value() >= 2);
	} else {
		return {};
	}
}
