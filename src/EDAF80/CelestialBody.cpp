#include "CelestialBody.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "core/helpers.hpp"
#include "core/Log.h"

CelestialBody::CelestialBody(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id)
{
	_body.node.set_geometry(shape);
	_body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_body.node.set_program(program);
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const& view_projection,
                                glm::mat4 const& parent_transform,
                                bool show_basis)
{
	// Convert the duration from microseconds to seconds.
	auto const elapsed_time_s = std::chrono::duration<float>(elapsed_time).count();
	// If a different ratio was needed, for example a duration in
	// milliseconds, the following would have been used:
	// auto const elapsed_time_ms = std::chrono::duration<float, std::milli>(elapsed_time).count();

	
	//Calculate continuous spin based on previous angle
	_body.spin.rotation_angle += elapsed_time_s * _body.spin.speed;
	
	//Calculate orbit rotation angle
	_body.orbit.rotation_angle += elapsed_time_s * _body.orbit.speed;
	
	//scaling matrix
	glm::mat4 const mIdentity = glm::mat4(1.0f);
	
	glm::mat4 const S = glm::scale(mIdentity, _body.scale);
	
	
	
	//Rotation Matrices
	//-Spin
	glm::mat4 const R1_s = glm::rotate(mIdentity, _body.spin.rotation_angle, glm::vec3 (0.0f, 1.0f, 0.0f));
	
	glm::mat4 const R2_s = glm::rotate(mIdentity, _body.spin.axial_tilt, glm::vec3 (0.0f, 0.0f, 1.0f));
	
	//Spin matrix
	glm::mat4 const M_spin = R2_s * R1_s * S;
	
	
	
	//-Orbit
	glm::mat4 const R1_0 = glm::rotate(mIdentity, _body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	
	glm::mat4 const R2_0 = glm::rotate(mIdentity, _body.orbit.inclination, glm::vec3 (0.0f, 0.0f, 1.0f));
	
	//Translation matrix
	glm::mat4 const T_0 = glm::translate(mIdentity, glm::vec3(_body.orbit.radius, 0.0f, 0.0f));
	
	
	//ORBIT TILT FIX
	glm::mat4 const RN_0 = glm::rotate(mIdentity, -_body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	
	
	//Orbit matrix
	glm::mat4 const M_orbit = R2_0 * R1_0 * T_0 * RN_0;
	
	
	glm::mat4 world = parent_transform * M_orbit * M_spin;
	

	if (show_basis)
	{
		bonobo::renderBasis(1.0f, 2.0f, view_projection, world);
	}

	// Note: The second argument of `node::render()` is supposed to be the
	// parent transform of the node, not the whole world matrix, as the
	// node internally manages its local transforms. However in our case we
	// manage all the local transforms ourselves, so the internal transform
	// of the node is just the identity matrix and we can forward the whole
	// world matrix.
	_body.node.render(view_projection, world);
	
	
	//Matrix for child node transforms
	glm::mat4 const child_parent_transform = parent_transform * M_orbit * R2_s;
	
	
	//RINGS
	//Check if the body that is being rendered (parent) has rings.
	if(_ring.is_set){
		//A matrix that scales the rings by first converting the vec2 into a vec3, since it only scales in the xy-dimension, the z scaling is set as 1.0f.
		glm::mat4 rS = glm::scale(mIdentity, glm::vec3(_ring.scale, 1.0f));
		
		//A matrix rotating the rings by 90° around the x-axis
		glm::mat4 R_90 = glm::rotate(mIdentity, glm::half_pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		
		//Computing the scaling, rotation and finally transformations of the parent body in that order to make sure it looks correct.
		glm::mat4 const rings = child_parent_transform * R2_s * R_90 * rS;
		
		//Render the rings
		_ring.node.render(view_projection, rings);
	}
	

	return child_parent_transform;
}

void CelestialBody::add_child(CelestialBody* child)
{
	_children.push_back(child);
}

std::vector<CelestialBody*> const& CelestialBody::get_children() const
{
	return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const& configuration)
{
	_body.orbit.radius = configuration.radius;
	_body.orbit.inclination = configuration.inclination;
	_body.orbit.speed = configuration.speed;
	_body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const& scale)
{
	_body.scale = scale;
}

void CelestialBody::set_spin(SpinConfiguration const& configuration)
{
	_body.spin.axial_tilt = configuration.axial_tilt;
	_body.spin.speed = configuration.speed;
	_body.spin.rotation_angle = 0.0f;
}

void CelestialBody::set_ring(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id,
                             glm::vec2 const& scale)
{
	_ring.node.set_geometry(shape);
	_ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_ring.node.set_program(program);

	_ring.scale = scale;

	_ring.is_set = true;
}
