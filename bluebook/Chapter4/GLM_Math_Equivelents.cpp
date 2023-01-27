#if 0

#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace glm;

//Vectors
vec4 test(1.0, 2.0, 3.0, 4.0);
vec4 test2(2.0, 3.0, 4.0, 5.0);
	//Dot
f32 dot_result = dot(test, test2);
	//Cross
vec3 cross_result = cross(vec3(test), vec3(test2));
	//Magnitude/Length
f32 magnitude_result = length(test);
	//Distance
f32 distance_result = distance(test, test2);
	//Reflection/Refraction
vec4 reflection_result = reflect(test, test2);
vec4 refract_result = refract(test, test2, 1.0f);

//Matrices
mat4 test3(test, test, test, test);
	//Identity
mat4 identity_result(1.0f);
mat4 ident;
	//Translation
mat4 translate_result = translate(identity_result, vec3(1.0f, 2.0f, 3.0f));
	//Rotation/Axis Rotation
mat4 rotate_result = rotate(identity_result, radians(45.0f), vec3(0.0f, 0.0f, 1.0f));
	//Scaling
mat4 scaling_result = scale(identity_result, vec3(1.0, 2.0, 0.0));
	//LookAt
mat4 look_at = lookAt(vec3(0.0f, 0.0f, -5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	//Perspective/Orthographic Projection
mat4 persp_proj = perspective(90.0f, 16.0f/9.0f, 0.1f, 100.0f);
mat4 ortho_proj = ortho(0.0f, 1980.0f, 0.0f, 1080.0f);

//Quaternions

//Interpolation
	//Bezier Curvers
	//Splines
	
#endif //0