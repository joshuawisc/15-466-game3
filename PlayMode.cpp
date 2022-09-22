#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random> // rand
#include <math.h> // abs
#include <iostream> // cout
#include <algorithm> // max, min



GLuint base_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > base_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("base.pnct"));
	base_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > base_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("base.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = base_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = base_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > high_sonar_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("high-sonar.wav"));
});

Load< Sound::Sample > knob_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("knob.wav"));
});

Load< Sound::Sample > button_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("button.wav"));
});

Load< Sound::Sample > hit_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("hit.wav"));
});

Load< Sound::Sample > lose_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("lose.wav"));
});

Load< Sound::Sample > ambient_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("ambient.wav"));
});


PlayMode::PlayMode() : scene(*base_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "ScanLine") scanline = &transform;
		else if (transform.name == "Slider") slider = &transform;
		else if (transform.name == "SonarKnob") sonar_knob = &transform;
		else if (transform.name == "NetKnob") net_knob = &transform;
		else if (transform.name == "NetLocLight") net_light = &transform;
		else if (transform.name == "SonarShoot") sonar_shoot = &transform;
		else if (transform.name == "NetShoot") net_shoot = &transform;
		else if (transform.name == "NetLeft") net_left = &transform;
		else if (transform.name == "NetRight") net_right = &transform;
	}
	if (scanline == nullptr) throw std::runtime_error("Scanline not found.");

	scan_orig_pos = scanline->position;
	slider_final_pos = slider->position;
	sonar_knob_orig = sonar_knob->rotation;
	net_knob_orig = net_knob->rotation;
	net_light_orig = net_light->position;


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// rand docs: https://cplusplus.com/reference/cstdlib/rand/
	srand(time(NULL));

	item_pos.x = 1 + rand() % 8;
	item_pos.y = 1 + rand() % 8;
	item_pos.z = 1 + rand() % 8;
	// item_pos.x = 1.0f;
	// item_pos.y = 1.0f;
	// item_pos.z = 1.0f;

	net_pos = glm::vec3(0.0f, 0.0f, 0.0f);

	device = SONAR;


	//start music loop playing:
	// (note: position will be over-ridden in update())
	Sound::loop(*ambient_sample, 1.0f, 0.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN && evt.key.repeat == 0) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a && evt.key.repeat == 0) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d && evt.key.repeat == 0) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_l && evt.key.repeat == 0) {
			// From https://stackoverflow.com/questions/44664331/sdl-keydown-triggering-twice
			z.downs += 1;
			z.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_TAB && evt.key.repeat == 0) {
			tab.downs += 1;
			tab.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_l) {
			z.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_TAB) {
			tab.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {


	static float sliderSpeed = 5.0f;
	static float pressed = 2.21f;
	static float unpressed = 2.27f;
	// static float sonarKnobSpeed = 3.0f;
	// Set tool and options
	{
		if (tab.downs > 0) {
			// From: https://stackoverflow.com/questions/40979513/changing-enum-to-next-value-c11
			device = static_cast<Device>((device+1)%(NET+1));
			sliderSpeed *= -1;
			Sound::play(*knob_sample, 1.0f, 0.0f);
		}

		if (device == SONAR) {
			if (z.downs > 0) {
				Sound::play(*knob_sample, 1.0f, 0.0f);
				sonar_axis += 1;
				sonar_axis %= 3;
				sonar_knob->rotation =  sonar_knob_orig * glm::angleAxis(
					glm::radians(-45.0f*sonar_axis),
					glm::vec3(0.0f, 0.0f, 1.0f)
				);
			}
		} else if (device == NET) {
			if (z.downs > 0) {
				Sound::play(*knob_sample, 1.0f, 0.0f);
				net_axis += 1;
				net_axis %= 3;
				net_knob->rotation =  net_knob_orig * glm::angleAxis(
					glm::radians(-45.0f*net_axis),
					glm::vec3(0.0f, 0.0f, 1.0f)
				);
			}
		}
	}

	// Animate
	{
		float newPos = slider->position.y + sliderSpeed * elapsed;
		newPos = std::min(newPos, slider_final_pos.y);
		newPos = std::max(newPos, -slider_final_pos.y);
		slider->position.y = newPos;

		if (device == SONAR) {
			if (space.pressed) sonar_shoot->position.x = pressed;
			else sonar_shoot->position.x = unpressed;
		}

		if (device == NET) {
			if (left.pressed) net_left->position.x = pressed;
			else net_left->position.x = unpressed;

			if (right.pressed) net_right->position.x = pressed;
			else net_right->position.x = unpressed;

			if (space.pressed) net_shoot->position.x = pressed;
			else net_shoot->position.x = unpressed;
		}

		if (left.downs > 0 || right.downs > 0 || space.downs > 0) Sound::play(*button_sample, 1.0f, 0.0f);

	}

	// update netpos and launch net
	{
		if (device == NET) {
			if (right.downs > 0) net_pos[net_axis] += 1;
			else if (left.downs > 0) net_pos[net_axis] -= 1;
			net_pos[net_axis] = std::max(net_pos[net_axis], 0.f);
			net_pos[net_axis] = float(int(net_pos[net_axis]) % 11);
			net_light->position.y = net_light_orig.y + (net_pos[net_axis]*(0.27f));

			if (space.pressed) {
				if (glm::distance(net_pos, item_pos) <= 1) {
					Sound::play(*hit_sample, 1.0f, 0.0f);
				} else {
					Sound::play(*lose_sample, 1.0f, 0.0f);
				}
			}
		}
	}

	// move scanline
	{
		float end_coord = 1.45;
		static float PlayerSpeed = 0;
		scanline->position.y += PlayerSpeed * elapsed;


		// Change world pos range of scanline to 0-10
		auto get_scaled_pos = [&](glm::vec3 world_pos) {
			return ((world_pos.y - scan_orig_pos.y)/(end_coord - scan_orig_pos.y)) * 10;
		};

		float scaled_pos = get_scaled_pos(scanline->position);

		if (fabs(scaled_pos - item_pos[sonar_axis]) <= 0.1) {
			Sound::play(*high_sonar_sample, 1.0f, (item_pos[sonar_axis] / 10) * 2 - 1);
		}
		if (scanline->position.y > end_coord) {
			scanline->position = scan_orig_pos;
			PlayerSpeed = 0;
		}
		if (space.pressed && device == SONAR) {
			PlayerSpeed = (end_coord - scan_orig_pos.y)/5;
			scanline->position = scan_orig_pos;
		}

	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
	z.downs = 0;
	tab.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("SPACE for the red buttons, L for the knobs, TAB for the slider, A/D for the two black buttons. Destroy the creature",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("SPACE for the red buttons, L for the knobs, TAB for the slider, A/D for the two black buttons. Destroy the creature",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
