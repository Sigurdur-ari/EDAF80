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
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
	auto camera_position = mCamera.mWorld.GetTranslation();

	// Create the shader programs
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
	
	GLuint cube_shader = 0u;
	program_manager.CreateAndRegisterProgram("Cube",
											 { { ShaderType::vertex, "EDAF80/cube.vert" },
											   { ShaderType::fragment, "EDAF80/cube.frag" } },
											 cube_shader);
	if (cube_shader == 0u)
		LogError("Failed to load cube shader");

	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//
	
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
	
		GLuint skybox_texture = bonobo::loadTextureCubeMap(
			config::resources_path("cubemaps/EveningSky/right.png"),
			config::resources_path("cubemaps/EveningSky/left.png"),
			config::resources_path("cubemaps/EveningSky/up.png"),
			config::resources_path("cubemaps/EveningSky/down.png"),
			config::resources_path("cubemaps/EveningSky/front.png"),
			config::resources_path("cubemaps/EveningSky/back.png")
		);

	auto const set_diffuse_uniforms = [&camera_position](GLuint program){
		glUniform3f(glGetUniformLocation(program, "light_position"), 0.0f, 15.0f, 10.0f);
		glUniform3f(glGetUniformLocation(program, "camera_position"), camera_position.x, camera_position.y, camera_position.z);
	};
	
	float elapsed_time_s = 0.0f;
	glm::vec3 current_color = glm::vec3( 1.0f );
	
	// pass time and color to cube shader
	auto const set_cube_uniforms = [&current_color, &elapsed_time_s](GLuint program){
		glUniform3f(glGetUniformLocation(program, "random_color"), current_color.r, current_color.g, current_color.b);
		glUniform1f(glGetUniformLocation(program, "t"), elapsed_time_s);
	};
	
	//Phong uniforms
	bool use_normal_mapping = true;
	auto const phong_set_uniforms = [&use_normal_mapping,&camera_position](GLuint program){
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3f(glGetUniformLocation(program, "light_position"), 0.0f, 15.0f, 10.0f);
		glUniform3f(glGetUniformLocation(program, "camera_position"), camera_position.x, camera_position.y, camera_position.z);
	};
	
	
	//Upload sphere textures
	
	GLuint diffuse_texture = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_coll1_2k.jpg"));
	
	GLuint specular_map = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_rough_2k.jpg"));
	
	GLuint normal_map = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_nor_2k.jpg"));
	
	// Upload normal mapping texture for water
	GLuint water_texture = bonobo::loadTexture2D(config::resources_path("textures/waves.png"));
	
	
	// Phong shaders
	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong",
											 {	{ShaderType::vertex, "EDAF80/phong.vert"},
												{ShaderType::fragment, "EDAF80/phong.frag"}},
											 phong_shader);
	if(phong_shader == 0u)
		LogError("Failed to load phong shader");
	
	
	// End screen shaders
	GLuint end_screen_shader = 0u;
	program_manager.CreateAndRegisterProgram("End Screen",
											 {	{ShaderType::vertex, "EDAF80/end_screen.vert"},
												{ShaderType::fragment, "EDAF80/end_screen.frag"}},
											 end_screen_shader);
	if(end_screen_shader == 0u)
		LogError("Failed to load phong shader");
	
	
	// Upload texture for end_screen
	GLuint end_screen_texture = bonobo::loadTexture2D(config::resources_path("textures/end_screen.png"));
	
	
	// Water shaders
	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water",
											 {	{ShaderType::vertex, "EDAF80/game_water.vert"},
												{ShaderType::fragment, "EDAF80/game_water.frag"}},
											 water_shader);
	if(water_shader == 0u)
		LogError("Failed to load water shader");

	
	//Water uniforms
	bool use_reflection = true;
	bool use_refraction = true;
	auto const water_set_uniforms = [&elapsed_time_s, &camera_position, &use_normal_mapping, &use_reflection, &use_refraction](GLuint program){
		glUniform1f(glGetUniformLocation(program, "t"), elapsed_time_s);
		glUniform3f(glGetUniformLocation(program, "camera_position"), camera_position.x, camera_position.y, camera_position.z);
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform1i(glGetUniformLocation(program, "use_reflection"), use_reflection ? 1 : 0);
		glUniform1i(glGetUniformLocation(program, "use_refraction"), use_refraction ? 1 : 0);
	};
	
	// Salmon shaders
	GLuint salmon_shader = 0u;
	program_manager.CreateAndRegisterProgram("Salmon",
											 { { ShaderType::vertex, "EDAF80/salmon.vert" },
											   { ShaderType::fragment, "EDAF80/salmon.frag" } },
											 salmon_shader);
	if (salmon_shader == 0u) {
		LogError("Failed to load salmon shader");
		return;
	}

	//
	// Todo: Load your geometry
	//
	
	// create cube geometry for obstacles, road, and rails
	auto const cube_shape = parametric_shapes::createCube(1.0f);
	
	if (cube_shape.vao == 0u)
		return;
	
	// Create quad geometry for water
	// Create quad for waves
	auto const water_shape = parametric_shapes::createQuad(10.0f, 80.0f ,1000, 1000);
	
	if (water_shape.vao == 0u)
		return;
	
	
	
	// Create sphere geometry for player
	auto const sphere_shape = parametric_shapes::createSphere(0.4f, 24, 24);
	
	if (sphere_shape.vao == 0u)
		return;
	
	// create skybox geometry and node
	auto const skybox_shape = parametric_shapes::createSphere(100.0f, 40u, 40u);
	if (skybox_shape.vao == 0u)
		return;
	
	// create end screen geometry
	auto const end_screen_shape = parametric_shapes::createQuad(9.0f, 16.0f, 1u, 1u);
	if (end_screen_shape.vao == 0u)
		return;

	
	// SALMON
	//
	// Load the salmon geometry
	//
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects(config::resources_path("scenes/Salmon.obj"));
	if (objects.empty()) {
		LogError("Failed to load the salmon geometry: exiting.");

		bonobo::deinit();

		return;
	}
	bonobo::mesh_data const& salmon = objects.front();
	
	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&skybox_shader);
	skybox.add_texture("skybox_texture", skybox_texture_teide, GL_TEXTURE_CUBE_MAP);

	auto cube = Node();
	cube.set_geometry(cube_shape);
	cube.set_program(&cube_shader, set_cube_uniforms);
	
	// Set the material constants for the sphere/player
	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.12f, 0.08f, 0.07f);
	demo_material.diffuse = glm::vec3(0.85f, 0.58f, 0.48f);
	demo_material.specular = glm::vec3(0.75f, 0.65f, 0.55f);
	demo_material.shininess = 24.0f;

	auto salmon_player = Node();
	salmon_player.set_geometry(salmon);
	salmon_player.set_program(&salmon_shader, set_diffuse_uniforms);
	salmon_player.set_material_constants(demo_material);
	salmon_player.add_texture("diffuse_texture", diffuse_texture, GL_TEXTURE_2D);
	salmon_player.add_texture("specular_map", specular_map, GL_TEXTURE_2D);
	salmon_player.add_texture("skybox_texture", skybox_texture_teide, GL_TEXTURE_CUBE_MAP);
	
	/**
	sphere.set_geometry(sphere_shape);
	sphere.set_material_constants(demo_material);
	sphere.set_program(&phong_shader, phong_set_uniforms);
	//Add textures to the sphere
	sphere.add_texture("diffuse_texture", diffuse_texture, GL_TEXTURE_2D);
	sphere.add_texture("specular_map", specular_map, GL_TEXTURE_2D);
	sphere.add_texture("normal_map", normal_map, GL_TEXTURE_2D);
	*/
	
	
	// end screen node
	auto end_screen = Node();
	end_screen.set_geometry(end_screen_shape);
	end_screen.get_transform().SetRotateX(-glm::half_pi<float>() - 0.25f);
	end_screen.get_transform().SetScale(glm::vec3(0.1f));
	end_screen.set_program(&end_screen_shader);
	end_screen.add_texture("screen_texture", end_screen_texture, GL_TEXTURE_2D);

	// road track and side rails
	auto road = Node();
	road.set_geometry(water_shape);
	road.set_program(&water_shader, water_set_uniforms);
	road.get_transform().SetTranslate(glm::vec3(5.0f, -0.5f, -75.0f));
	road.get_transform().SetRotateY(-glm::half_pi<float>());
	//road.get_transform().SetScale(glm::vec3(0.1f, 1.0f, 0.8f));
	road.add_texture("skybox_texture", skybox_texture_teide, GL_TEXTURE_CUBE_MAP);
	road.add_texture("wave_texture", water_texture, GL_TEXTURE_2D);

	auto left_rail = Node();
	left_rail.set_geometry(cube_shape);
	left_rail.set_program(&diffuse_shader, set_diffuse_uniforms);
	left_rail.get_transform().SetTranslate(glm::vec3(-5.1f, -0.4f, -25.0f));
	left_rail.get_transform().SetScale(glm::vec3(0.15f, 0.4f, 80.0f));

	auto right_rail = Node();
	right_rail.set_geometry(cube_shape);
	right_rail.set_program(&diffuse_shader, set_diffuse_uniforms);
	right_rail.get_transform().SetTranslate(glm::vec3(5.1f, -0.4f, -25.0f));
	right_rail.get_transform().SetScale(glm::vec3(0.15f, 0.4f, 80.0f));

	// obstacle management based on standard object pooling tutorial
	struct Obstacle {
		glm::vec3 position{ 0.0f };
		glm::vec3 scale{ 1.0f };
		glm::vec3 color{ 1.0f };
		bool active{ false }; // hide and unhide state
		bool moving{ false };
		float move_base_x{ 0.0f };
		float move_phase{ 0.0f };
		float move_speed{ 2.5f };
	};

	struct ObstacleManager {
		std::vector<Obstacle> pool;
		float lanes[5] = { -4.0f, -2.0f, 0.0f, 2.0f, 4.0f };
		float spawn_z = -50.0f;
		float cube_speed = 20.0f;
		float spawn_timer = 0.0f;
		float spawn_interval = 1.0f;

		ObstacleManager(size_t pool_size = 45) {
			pool.resize(pool_size);
		}

		// Spawn cubes, reuse cubes from pool by looping over inactive cubes.
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
					// Set the color of this obstacle to a random color
					obs.color = glm::vec3(
										  static_cast<float>(rand()) / RAND_MAX,
										  static_cast<float>(rand()) / RAND_MAX,
										  static_cast<float>(rand()) / RAND_MAX
										  );
					return;
				}
			}
		}

		// Pattern for 5 lanes
		void spawn_wave(float current_time) {
			int const pattern = std::rand() % 12;
			switch (pattern) {
				case 0: // single cube on random lane
					spawn_cube(lanes[std::rand() % 5], spawn_z);
					break;
				case 1: // two cubes on outer lanes
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[4], spawn_z);
					break;
				case 2: // three cubes in middle
					spawn_cube(lanes[1], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					spawn_cube(lanes[3], spawn_z);
					break;
				case 3: // wall with center lane open
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[1], spawn_z);
					spawn_cube(lanes[3], spawn_z);
					spawn_cube(lanes[4], spawn_z);
					break;
				case 4: // wall with left lane open
					spawn_cube(lanes[1], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					spawn_cube(lanes[3], spawn_z);
					spawn_cube(lanes[4], spawn_z);
					break;
				case 5: // wall with right lane open
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[1], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					spawn_cube(lanes[3], spawn_z);
					break;
				case 6: // checkerboard pattern
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[2], spawn_z);
					spawn_cube(lanes[4], spawn_z);
					break;
				case 7: // block left side
					spawn_cube(lanes[0], spawn_z);
					spawn_cube(lanes[1], spawn_z);
					break;
				case 8: // block right side
					spawn_cube(lanes[3], spawn_z);
					spawn_cube(lanes[4], spawn_z);
					break;
				case 9: // moving cube in center
					spawn_cube(lanes[2], spawn_z, true, current_time);
					break;
				case 10: // moving cube on left
					spawn_cube(lanes[1], spawn_z, true, current_time);
					break;
				case 11: // two moving cubes
					spawn_cube(lanes[1], spawn_z, true, current_time);
					spawn_cube(lanes[3], spawn_z, true, current_time + 1.57f);
					break;
			}
		}

		// move cubes towards player and hide when passed
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
					obs.position.x = glm::clamp(obs.position.x, -4.5f, 4.5f);
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
	float player_speed = 10.f;
	float const player_radius = 0.4f;

	// game state
	ObstacleManager obstacle_mgr(45);
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
		
		//Update time variable
		elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
		
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
			player_x = glm::clamp(player_x, -4.0f, 4.0f);

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


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		//
		// Todo: If you need to handle inputs, you can do it here
		//
		
		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		
		camera_position = mCamera.mWorld.GetTranslation();
		
		// make sphere static based on camera
		salmon_player.get_transform().SetTranslate(camera_position + glm::vec3(0.0f, -0.1f, -1.0f));
		salmon_player.get_transform().SetRotateY(glm::pi<float>());
		salmon_player.get_transform().SetScale(glm::vec3(0.3f));

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
				// Set the current random color to this cube
				current_color = obs.color;
				cube.render(view_proj);
			}
			// render player sphere
			salmon_player.get_transform().SetTranslate(glm::vec3(player_x, player_y, player_z));
			salmon_player.render(view_proj);
			//RENDER END SCREEN TEMP
			if(game_over){
				end_screen.get_transform().SetTranslate(camera_position + glm::vec3(-0.85f, -0.75f, -1.3f));

				glDisable(GL_DEPTH_TEST);

				end_screen.render(view_proj);

				glEnable(GL_DEPTH_TEST);
			}
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		
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
			const char* const skybox_options[] = { "Teide (Mountains)", "Nissi Beach", "Evening Sky" };
			if (ImGui::Combo("Skybox", &skybox_index, skybox_options, 3)) {
				skybox.add_texture("skybox_texture", (skybox_index == 0) ? skybox_texture_teide : (skybox_index == 1) ? skybox_texture_beach : skybox_texture, GL_TEXTURE_CUBE_MAP);
				road.add_texture("skybox_texture", (skybox_index == 0) ? skybox_texture_teide : (skybox_index == 1) ? skybox_texture_beach : skybox_texture, GL_TEXTURE_CUBE_MAP);
				salmon_player.add_texture("skybox_texture", (skybox_index == 0) ? skybox_texture_teide : (skybox_index == 1) ? skybox_texture_beach : skybox_texture, GL_TEXTURE_CUBE_MAP);
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
		
		// ImGui to show score at game over
		if(game_over){
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_AlwaysAutoResize;
			
			ImGui::SetNextWindowPos(ImVec2(870.0f, 300.0f));
			
			if(ImGui::Begin("Score", nullptr, flags)){
				ImGui::SetWindowFontScale(4.0f);
				ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%d", score);
			}
			ImGui::End();
			
			ImGui::SetNextWindowPos(ImVec2(930.0f, 460.0f));
			if(ImGui::Begin("High-score", nullptr, flags)){
				ImGui::SetWindowFontScale(4.0f);
				ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%d", high_score);
			}
			ImGui::End();
		}

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
