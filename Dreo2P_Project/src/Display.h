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
#include <thread>

class Display
{
public:
	// Constructors
	Display();	// Create basic object reference

	// Destructors
	~Display();

	// Public Members
	GLFWwindow* window_;
	float*		frame_data_;
	int			frame_width_;
	int			frame_height_;
	float		intensity_ = 0.0f;

	// Public Methods
	void Close();

private:
	// Private Members
	GLuint		vertex_shader, fragment_shader, program;
	GLuint		vertex_array_object;
	GLuint		frame_texture_;
	GLint		max_location, vpos_location;
	int			window_width_;
	int			window_height_;

	// Private Members (display thread)
	std::thread		display_thread_;
	bool volatile	active_ = false;

	// Thread Function
	void		Display_Thread_Function();

	// Private Methods
	void		Initialize_Window(int width, int height);
	void		Initialize_Render();
	void		Render();
	void		Update_Frame();
	static void Error_Handler(int error, const char* description);	//  Display error handler
};
