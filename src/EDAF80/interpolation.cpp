#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::vec3 p = (1-x)*p0 + x*p1;
	return p;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	/**
	glm::mat4 M = glm::mat4(glm::vec4(0, -t, 2*t, -t),
							glm::vec4(1, 0, t-3, 2-t),
							glm::vec4(0, t, 3-2*t, t-2),
							glm::vec4(0, 0, -t, t));
	 */
	//Calculate the coefficients in the formula q(x) = C_0*p0 + C_1 * p1.... by putting the value of x into each vector in the matrix. 
	float const coefficient_0 = -t * x + 2.0f*t*glm::pow(x, 2) + -t*glm::pow(x, 3);
	float const coefficient_1 = 1.0f + (t-3.0f)*glm::pow(x, 2) + (2.0f-t)*glm::pow(x, 3);
	float const coefficient_2 = t*x + (3.0f-2.0f*t)*glm::pow(x, 2) + (t-2.0f)*glm::pow(x, 3);
	float const coefficient_3 = -t*glm::pow(x, 2) + t*glm::pow(x, 3);
	
	glm::vec3 q = coefficient_0*p0 + coefficient_1*p1 + coefficient_2*p2 + coefficient_3*p3;
	
	return q;
}
