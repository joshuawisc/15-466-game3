#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space, z, tab;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	// Scene::Transform *hip = nullptr;
	// Scene::Transform *upper_leg = nullptr;
	// Scene::Transform *lower_leg = nullptr;
	// glm::quat hip_base_rotation;
	// glm::quat upper_leg_base_rotation;
	// glm::quat lower_leg_base_rotation;
	// float wobble = 0.0f;

	//scan line:
	Scene::Transform *scanline = nullptr;
	Scene::Transform *slider = nullptr;
	Scene::Transform *sonar_knob = nullptr;
	Scene::Transform *net_knob = nullptr;
	Scene::Transform *net_light= nullptr;
	Scene::Transform *sonar_shoot= nullptr;
	Scene::Transform *net_shoot= nullptr;
	Scene::Transform *net_right= nullptr;
	Scene::Transform *net_left= nullptr;

	glm::vec3 scan_orig_pos;
	glm::vec3 net_light_orig;
	glm::quat sonar_knob_orig;
	glm::quat net_knob_orig;
	glm::vec3 slider_final_pos;
	glm::vec3 net_pos;
	glm::vec3 item_pos;

	uint8_t sonar_axis = 0; // Axis for sonar ping, 0,1,2 = x,y,z
	uint8_t net_axis = 0;

	enum Device {
		SONAR,
		NET
	} device;

	// glm::vec3 get_leg_tip_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;

	//camera:
	Scene::Camera *camera = nullptr;

};
