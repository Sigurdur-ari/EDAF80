#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/ShaderProgramManager.hpp"
#include "core/node.hpp"

#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
	auto camera_position = mCamera.mWorld.GetTranslation();

	// create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	
	GLuint diffuse_shader = 0u;
	program_manager.CreateAndRegisterProgram("Diffuse",
											 { { ShaderType::vertex, "EDAF80/diffuse.vert" },
											   { ShaderType::fragment, "EDAF80/diffuse.frag" } },
											 diffuse_shader);
	if (diffuse_shader == 0u) {
		LogError("Failed to load diffuse shader");
		return;
	}

	// register Skybox shader
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
	                                         { { ShaderType::vertex, "EDAF80/skybox.vert" },
	                                           { ShaderType::fragment, "EDAF80/skybox.frag" } },
	                                         skybox_shader);
	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}

	// load cubemap textures for Skybox
	GLuint skybox_texture_teide = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/Teide/posx.jpg"),
		config::resources_path("cubemaps/Teide/negx.jpg"),
		config::resources_path("cubemaps/Teide/posy.jpg"),
		config::resources_path("cubemaps/Teide/negy.jpg"),
		config::resources_path("cubemaps/Teide/posz.jpg"),
		config::resources_path("cubemaps/Teide/negz.jpg")
	);

	GLuint skybox_texture_beach = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negz.jpg")
	);

	auto const set_diffuse_uniforms = [](GLuint program){
		glUniform3f(glGetUniformLocation(program, "light_position"), 0.0f, 15.0f, 10.0f);
	};

	// create cube geometry for obstacles, road, and rails
	auto const cube_shape = parametric_shapes::createCube(1.0f);
	if (cube_shape.vao == 0u)
		return;

	// create sphere geometry for player
	auto const sphere_shape = parametric_shapes::createSphere(0.4f, 24, 24);
	if (sphere_shape.vao == 0u)
		return;

	// create skybox geometry and node
	auto const skybox_shape = parametric_shapes::createSphere(100.0f, 40u, 40u);
	if (skybox_shape.vao == 0u)
		return;

	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&skybox_shader);
	skybox.add_texture("skybox_texture", skybox_texture_teide, GL_TEXTURE_CUBE_MAP);

	auto cube = Node();
	cube.set_geometry(cube_shape);
	cube.set_program(&diffuse_shader, set_diffuse_uniforms);

	auto sphere = Node();
	sphere.set_geometry(sphere_shape);
	sphere.set_program(&diffuse_shader, set_diffuse_uniforms);

	// road track and side rails
	auto road = Node();
	road.set_geometry(cube_shape);
	road.set_program(&diffuse_shader, set_diffuse_uniforms);
	road.get_transform().SetTranslate(glm::vec3(0.0f, -0.55f, -25.0f));
	road.get_transform().SetScale(glm::vec3(6.5f, 0.1f, 80.0f));

	auto left_rail = Node();
	left_rail.set_geometry(cube_shape);
	left_rail.set_program(&diffuse_shader, set_diffuse_uniforms);
	left_rail.get_transform().SetTranslate(glm::vec3(-3.3f, -0.4f, -25.0f));
	left_rail.get_transform().SetScale(glm::vec3(0.15f, 0.4f, 80.0f));

	auto right_rail = Node();
	right_rail.set_geometry(cube_shape);
	right_rail.set_program(&diffuse_shader, set_diffuse_uniforms);
	right_rail.get_transform().SetTranslate(glm::vec3(3.3f, -0.4f, -25.0f));
	right_rail.get_transform().SetScale(glm::vec3(0.15f, 0.4f, 80.0f));

	// obstacle management based on standard object pooling tutorial
	struct Obstacle {
		glm::vec3 position{ 0.0f };
		glm::vec3 scale{ 1.0f };
		bool active{ false }; // hide and unhide state
		bool moving{ false };
		float move_base_x{ 0.0f };
		float move_phase{ 0.0f };
		float move_speed{ 2.5f };
	};

	struct ObstacleManager {
		std::vector<Obstacle> pool;
		float lanes[3] = { -2.0f, 0.0f, 2.0f };
		float spawn_z = -50.0f;
		float cube_speed = 16.0f;
		float spawn_timer = 0.0f;
		float spawn_interval = 1.3f;

		ObstacleManager(size_t pool_size = 30) {
			pool.resize(pool_size);
		}

		// 4. (hide&unhide) regeneration randomly: reuse inactive cube from pool
		void spawn_cube(float x, float z, bool moving = false, float phase = 0.0f) {
			for (auto& obs : pool) {
				if (!obs.active) {
					obs.active = true; // unhide
					obs.position = glm::vec3(x, 0.0f, z);
					obs.scale = glm::vec3(1.0f);
					obs.moving = moving;
					obs.move_base_x = x;
					obs.move_phase = phase;
					obs.move_speed = 2.5f;
					return;
				}
			}
		}

		// 2. make a "pattern" for movement: procedural waves with safe passages
		void spawn_wave(float current_time) {
			int const pattern = std::rand() % 7;
			switch (pattern) {
				case 0: // single cube on left lane
					spawn_cube(lanes[0], spawn_z);
					break;
				case 1: // single cube on center lane
					spawn_cube(lanes[1], spawn_z);
					break;
				case 2: // single cube on right lane
					spawn_cube(lanes[2], spawn_z);
					break;
				case 3: // double cube: left and center blocked, right is free
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[1], spawn_z);
					break;
				case 4: // double cube: center and right blocked, left is free
					spawn_cube(lanes[1], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					break;
				case 5: // double cube: left and right blocked, center is free
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					break;
				case 6: // moving oscillating cube in center
					spawn_cube(lanes[1], spawn_z, true, current_time);
					break;
			}
		}

		// 1. motivate cube to move: move cubes towards player and hide when passed
		int update(float dt_s, float total_time) {
			int dodged_count = 0;

			// update timer and trigger wave patterns
			spawn_timer += dt_s;
			if (spawn_timer >= spawn_interval) {
				spawn_timer = 0.0f;
				spawn_wave(total_time);
			}

			// move active cubes forward towards player (+z)
			for (auto& obs : pool) {
				if (!obs.active) continue;

				obs.position.z += cube_speed * dt_s;

				// update side-to-side pattern movement
				if (obs.moving) {
					obs.position.x = obs.move_base_x + std::sin(total_time * obs.move_speed + obs.move_phase) * 1.5f;
					obs.position.x = glm::clamp(obs.position.x, -2.5f, 2.5f);
				}

				// hide when cube passes behind player
				if (obs.position.z > 4.0f) {
					obs.active = false; // hide
					dodged_count++;
				}
			}
			return dodged_count;
		}

		// reset all obstacles back to hidden state
		void reset() {
			spawn_timer = 0.0f;
			for (auto& obs : pool) {
				obs.active = false;
			}
		}
	};

	// player state
	float player_x = 0.0f;
	float const player_y = 0.0f;
	float const player_z = 0.0f;
	float player_speed = 7.5f;
	float const player_radius = 0.4f;

	// game state
	ObstacleManager obstacle_mgr(30);
	int score = 0;
	int high_score = 0;
	bool game_over = false;
	float total_time = 0.0f;

	auto reset_game = [&]() {
		game_over = false;
		score = 0;
		player_x = 0.0f;
		obstacle_mgr.reset();
	};

	glClearDepthf(1.0f);
	glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = false;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		float const dt_s = static_cast<float>(deltaTimeUs.count()) / 1000000.0f;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();

		// game update
		if (!game_over) {
			total_time += dt_s;

			// move player sphere left / right
			float move_dir = 0.0f;
			if ((inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) ||
			    (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED)) {
				move_dir -= 1.0f;
			}
			if ((inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) ||
			    (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED)) {
				move_dir += 1.0f;
			}
			player_x += move_dir * player_speed * dt_s;
			player_x = glm::clamp(player_x, -2.7f, 2.7f);

			// update obstacles
			int const dodged = obstacle_mgr.update(dt_s, total_time);
			score += dodged;
			if (score > high_score) {
				high_score = score;
			}

			// collision detection (aabb vs sphere)
			glm::vec3 const player_pos(player_x, player_y, player_z);
			float const cube_half = 0.5f;
			for (auto const& obs : obstacle_mgr.pool) {
				if (!obs.active) continue;

				float const dx = std::abs(player_pos.x - obs.position.x);
				float const dz = std::abs(player_pos.z - obs.position.z);
				if (dx < (cube_half + player_radius * 0.85f) &&
				    dz < (cube_half + player_radius * 0.85f)) {
					game_over = true;
					break;
				}
			}
		}

		// keys handling
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			if (game_over) {
				reset_game();
			} else {
				shader_reload_failed = !program_manager.ReloadAllPrograms();
				if (shader_reload_failed)
					tinyfd_notifyPopup("Shader Program Reload Error",
					                   "An error occurred while reloading shader programs; see the logs for details.\n"
					                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
					                   "error");
			}
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		// third-person camera following the player
		mCamera.mWorld.SetTranslate(glm::vec3(player_x * 0.35f, 2.3f, 5.5f));
		mCamera.mWorld.SetRotateX(-0.25f);
		mCamera.Update(deltaTimeUs, inputHandler, true, true);

		// framebuffer size
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		mWindowManager.NewImGuiFrame();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		if (!shader_reload_failed) {
			glm::mat4 const view_proj = mCamera.GetWorldToClipMatrix();

			// render skybox at infinite depth
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			skybox.get_transform().SetTranslate(mCamera.mWorld.GetTranslation());
			skybox.render(view_proj);
			glEnable(GL_DEPTH_TEST);

			// render track & rails
			road.render(view_proj);
			left_rail.render(view_proj);
			right_rail.render(view_proj);

			// render active (unhidden) obstacles
			for (auto const& obs : obstacle_mgr.pool) {
				if (!obs.active) continue;
				cube.get_transform().SetTranslate(obs.position);
				cube.get_transform().SetScale(obs.scale);
				cube.render(view_proj);
			}

			// render player sphere
			sphere.get_transform().SetTranslate(glm::vec3(player_x, player_y, player_z));
			sphere.render(view_proj);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// imgui hud & controls
		bool const opened = ImGui::Begin("Game HUD", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (opened) {
			if (game_over) {
				ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "=== GAME OVER! ===");
				ImGui::Text("Press 'R' or click button to restart");
				if (ImGui::Button("Restart Game")) {
					reset_game();
				}
			} else {
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Playing");
			}
			ImGui::Separator();
			ImGui::Text("Score (Dodged): %d", score);
			ImGui::Text("High Score:     %d", high_score);
			ImGui::Separator();
			ImGui::SliderFloat("Cube Speed", &obstacle_mgr.cube_speed, 8.0f, 35.0f);
			ImGui::SliderFloat("Player Speed", &player_speed, 4.0f, 16.0f);
			ImGui::SliderFloat("Spawn Interval", &obstacle_mgr.spawn_interval, 0.6f, 2.5f);
			ImGui::Separator();

			static int skybox_index = 0;
			const char* const skybox_options[] = { "Teide (Mountains)", "Nissi Beach" };
			if (ImGui::Combo("Skybox", &skybox_index, skybox_options, 2)) {
				skybox.add_texture("skybox_texture", (skybox_index == 0) ? skybox_texture_teide : skybox_texture_beach, GL_TEXTURE_CUBE_MAP);
			}

			ImGui::Separator();
			ImGui::Text("Controls:");
			ImGui::BulletText("[A] / [D] or Left / Right: Move");
			ImGui::BulletText("[R]: Restart / Reload shaders");
			ImGui::BulletText("[F2]: Toggle GUI HUD");
			if (ImGui::Button("Reset Game")) {
				reset_game();
			}
			ImGui::Checkbox("Show basis", &show_basis);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
