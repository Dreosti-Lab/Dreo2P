// Dreo2P Display Class (header)
#pragma once
// Include OpenGL handler
#include <glad/glad.h>		// glad
#include <GLFW/glfw3.h>		// GLFW

// Include OpenGL Math library classes (GLM)
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Include STD headers
#include <iostream>

class Display
{
public:
	// Constructors
	Display();	// Create basic object reference

	// Destructors
	~Display();

	// Members
	GLFWwindow* window;

	// Methods
	void Initialize_Window(int width, int height);
	void Initialize_Render();
	void Render(int num_voxels);
	void Close();

private:
	// Members
	GLuint		vertex_buffer;
	GLuint		vertex_shader, fragment_shader, program;
	GLint		mvp_location, vpos_location, vcol_location;

	// Methods
	static void Error_Callback(int error, const char* description);	// GLFW error callback function
};

