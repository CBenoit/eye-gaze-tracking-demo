#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ow/shader_program.hpp>
#include <ow/camera_fps.hpp>
#include <ow/vertex.hpp>
#include <ow/lights_set.hpp>
#include <ow/directional_light.hpp>
#include <ow/point_light.hpp>
#include <ow/spotlight.hpp>
#include <ow/mesh.hpp>
#include <ow/texture.hpp>
#include <ow/model.hpp>
#include <ow/utils.hpp>
#include <gui/window.hpp>

void process_input(gui::window& window, float dt);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// initialize camera system
// ------------------------
ow::camera_fps camera{glm::vec3(0, 0, 3)};
float last_x = static_cast<float>(SCR_WIDTH) / 2.f;
float last_y = static_cast<float>(SCR_HEIGHT) / 2.f;
bool first_mouse = true;

int main() {
	gui::window window{ "IN55", SCR_WIDTH, SCR_HEIGHT, nullptr, nullptr };
	if (window.invalid()) {
		ow::logger << "Failed to create window" << std::endl;
		return EXIT_FAILURE;
	}

	window.make_context_current();
	window.set_framebuffer_size_callback(framebuffer_size_callback);
	window.set_cursor_pos_callback(mouse_callback);
	window.set_scroll_callback(scroll_callback);

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

	// load the texture
	// ----------------
	auto white_texture = std::make_shared<ow::texture>("resources/textures/white.jpg", ow::texture_type::emission);

	// load shaders
	// ------------
	ow::shader_program prog{
			{{GL_VERTEX_SHADER, "phong_vertex.glsl"}
			,{GL_FRAGMENT_SHADER, "phong_frag.glsl"}
	}};

	// set up mesh
	// -----------
	std::vector<ow::vertex> vertices = {
		// front
		{glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
		{glm::vec3(-0.5,  0.5, 0.5), glm::vec3(0, 0, 1), glm::vec2(0, 1)},
		{glm::vec3(0.5,  -0.5, 0.5), glm::vec3(0, 0, 1), glm::vec2(1, 0)},
		{glm::vec3(0.5,   0.5, 0.5), glm::vec3(0, 0, 1), glm::vec2(1, 1)},

		// back
		{glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0, 0, -1), glm::vec2(0, 0)},
		{glm::vec3(-0.5,  0.5, -0.5), glm::vec3(0, 0, -1), glm::vec2(0, 1)},
		{glm::vec3(0.5,  -0.5, -0.5), glm::vec3(0, 0, -1), glm::vec2(1, 0)},
		{glm::vec3(0.5,   0.5, -0.5), glm::vec3(0, 0, -1), glm::vec2(1, 1)},

		// left
		{glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-1, 0, 0), glm::vec2(0, 0)},
		{glm::vec3(-0.5,  0.5, -0.5), glm::vec3(-1, 0, 0), glm::vec2(0, 1)},
		{glm::vec3(-0.5, -0.5,  0.5), glm::vec3(-1, 0, 0), glm::vec2(1, 0)},
		{glm::vec3(-0.5,  0.5,  0.5), glm::vec3(-1, 0, 0), glm::vec2(1, 1)},

		// right
		{glm::vec3(0.5, -0.5, -0.5), glm::vec3(1, 0, 0), glm::vec2(0, 0)},
		{glm::vec3(0.5,  0.5, -0.5), glm::vec3(1, 0, 0), glm::vec2(0, 1)},
		{glm::vec3(0.5, -0.5,  0.5), glm::vec3(1, 0, 0), glm::vec2(1, 0)},
		{glm::vec3(0.5,  0.5,  0.5), glm::vec3(1, 0, 0), glm::vec2(1, 1)},

		// top
		{glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0, 1, 0), glm::vec2(0, 0)},
		{glm::vec3(-0.5, 0.5,  0.5), glm::vec3(0, 1, 0), glm::vec2(0, 1)},
		{glm::vec3( 0.5, 0.5, -0.5), glm::vec3(0, 1, 0), glm::vec2(1, 0)},
		{glm::vec3( 0.5, 0.5,  0.5), glm::vec3(0, 1, 0), glm::vec2(1, 1)},

		// bottom
		{glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0, -1, 0), glm::vec2(0, 0)},
		{glm::vec3(-0.5, -0.5,  0.5), glm::vec3(0, -1, 0), glm::vec2(0, 1)},
		{glm::vec3( 0.5, -0.5, -0.5), glm::vec3(0, -1, 0), glm::vec2(1, 0)},
		{glm::vec3( 0.5, -0.5,  0.5), glm::vec3(0, -1, 0), glm::vec2(1, 1)},
	};

	std::vector<GLuint> indices = {
		// front face
		0, 1, 2, // first triangle
		1, 2, 3,  // second triangle
		// back face
		4, 5, 6,
		5, 6, 7,
		// left face
		8, 9, 10,
		9, 10, 11,
		// right face
		12, 13, 14,
		13, 14, 15,
		// top face
		16, 17, 18,
		17, 18, 19,
		// bottom face
		20, 21, 22,
		21, 22, 23,
	};

	ow::mesh lamp_mesh{std::move(vertices), std::move(indices)};
	lamp_mesh.add_texture(white_texture);

	// lights
	// ------
	ow::lights_set lights;
	lights.add_directional_light(std::make_shared<ow::directional_light>(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(.3f)));

	std::vector<std::shared_ptr<ow::point_light>> point_lights;
	{
		glm::vec3 positions[] = {
			glm::vec3( 0.7f,  0.2f,  2.0f),
			glm::vec3( 2.3f, -3.3f, -4.0f),
			glm::vec3(-4.0f,  2.0f, -12.0f),
			glm::vec3( 0.0f,  0.0f, -3.0f)
		};

		for (auto& pos : positions) {
			auto light = std::make_shared<ow::point_light>(pos, 1.0, 0.045, 0.0075);
			lights.add_point_light(light);
			point_lights.push_back(light);
		}
	}

	prog.use();
	prog.set("materials_shininess", 32.f);
	lights.update_all(prog, glm::mat4());

	// load models
	// -----------
	ow::model nanosuit{"resources/models/nanosuit/nanosuit.obj"};

	// game loop
	// -----------
	float delta_time;	// time between current frame and last frame
	float last_frame = 0.0f; // time of last frame
	while (!window.should_close()) {
		auto current_frame = static_cast<float>(glfwGetTime());
		delta_time = current_frame - last_frame;
		last_frame = current_frame;

		// input
		// -----
		process_input(window, delta_time);

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		ow::check_errors("Failed to set clear color.");
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ow::check_errors("Failed to clear scr.");

		// activate shader program
		prog.use();

		// create transformations
		glm::mat4 view = camera.get_view_matrix();
		glm::mat4 proj = camera.get_proj_matrix(static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT));
		prog.set("view", view);
		prog.set("proj", proj);

		// update lights
		lights.update_position_and_direction(prog, view);

		{ // nanosuit
			glm::mat4 model{1.0f};
			model = glm::translate(model, glm::vec3(-2.5));
			model = glm::scale(model, glm::vec3(0.5));
			prog.set("model", model);

			glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(view * model)));
			prog.set("normal_matrix", normal_matrix);

			nanosuit.draw(prog);
		}

		// draw lamps
		for (auto&& pt_light : point_lights) {
			glm::mat4 model{1.0f};
			model = glm::translate(model, pt_light->get_pos());
			model = glm::scale(model, glm::vec3(.2f));
			prog.set("model", model);

			glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(view * model)));
			prog.set("normal_matrix", normal_matrix);

			lamp_mesh.draw(prog);
		}

		window.render();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	glfwTerminate();

	return EXIT_SUCCESS;
}

// process all input: query GLFW whether relevant keys are pressed/released
// this frame and react accordingly
void process_input(gui::window& window, float dt) {
	if (window.get_key(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		window.set_should_close(true);
	}

	if (window.get_key(GLFW_KEY_W) == GLFW_PRESS) {
		camera.process_movement(ow::FORWARD, dt);
	} else if (window.get_key(GLFW_KEY_S) == GLFW_PRESS) {
		camera.process_movement(ow::BACKWARD, dt);
	}

	if (window.get_key(GLFW_KEY_A) == GLFW_PRESS) {
		camera.process_movement(ow::LEFT, dt);
	} else if (window.get_key(GLFW_KEY_D) == GLFW_PRESS) {
		camera.process_movement(ow::RIGHT, dt);
	}

	if (window.get_key(GLFW_KEY_SPACE) == GLFW_PRESS) {
		camera.process_movement(ow::UP, dt);
	} else if (window.get_key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		camera.process_movement(ow::DOWN, dt);
	}
}

// glfw: whenever the window size changed (by OS or user resize)
// this callback function executes
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
	auto xposf = static_cast<float>(xpos);
	auto yposf = static_cast<float>(ypos);

	if (first_mouse) { // this bool variable is initially set to true
		last_x = xposf;
		last_y = yposf;
		first_mouse = false;
	}

	float xoffset = xposf - last_x;
	float yoffset = last_y - yposf; // y is reversed
	last_x = xposf;
	last_y = yposf;

	camera.process_mouse_movement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow*, double /*xoffset*/, double yoffset) {
	camera.process_mouse_scroll(static_cast<float>(yoffset));
}
