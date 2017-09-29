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

	// Public Members
	GLFWwindow* window_;
	float intensity_ = 0.0f;

	// Public Methods
	void Initialize_Window(int width, int height);
	void Initialize_Render();
	void Render();
	void Close();
	void Update_Frame(float* frame);

private:
	// Private Members
	GLuint		vertex_shader, fragment_shader, program;
	GLuint		vertex_array_object;
	GLint		max_location, vpos_location;
	GLuint		frame_texture;
	int			window_width_;
	int			window_height_;

	// Private Methods
	static void Error_Handler(int error, const char* description);	//  Display error handler
};

