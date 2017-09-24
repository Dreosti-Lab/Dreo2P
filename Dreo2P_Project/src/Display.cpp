// Dreo2P Display Class (source)
#include "Display.h"

// Constructor
Display::Display()
{
}

// Destructor
Display::~Display()
{
}

/*
// Load voxel data into a vertex buffer
void Output::Load_Voxels(int num_voxels, const void* data)
{
	// Generate and bind vertex buffer
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Voxel) * num_voxels, data, GL_STATIC_DRAW);
}
*/

// Initialize GLFW winodw
void Display::Initialize_Window(int width, int height)
{
	// Set GLFW error callback function
	glfwSetErrorCallback(Error_Callback);

	// Initialize the GLFW library
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	// Create a windowed mode window and its OpenGL context
	window = glfwCreateWindow(width, height, "Dreo2P - Live", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Start OpenGL extensions loader library (GLAD)
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Hide cursor when in window
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// Set swap interval (vsync = 1)
	glfwSwapInterval(1);
}


// Initialize rendering: load, compile and attach shaders and set vertex attribute arrays
void Display::Initialize_Render()
{
	// Store shader text here for now...
	static const char* vertex_shader_text =
		"uniform mat4 MVP;\n"
		"attribute vec3 vPos;\n"
		"attribute vec4 vCol;\n"
		"varying vec4 color;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = MVP * vec4(vPos, 1.0);\n"
		"    color = vCol;\n"
		"}\n";

	static const char* fragment_shader_text =
		"varying vec4 color;\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor = color;\n"
		"}\n";

	// Load and compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);

	// Load and compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);

	// Attach shaders to a "program"
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	// Gather addresses of uniforms (matrices, vertex positions and colors)
	mvp_location = glGetUniformLocation(program, "MVP");
	vpos_location = glGetAttribLocation(program, "vPos");
	vcol_location = glGetAttribLocation(program, "vCol");

	// Enable vertex attribute arrays
	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)0);
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)(sizeof(float) * 3));
}

// Render (draw calls, rescaling, user view updates, etc.)
void Display::Render(int num_voxels)
{
	// Local variables
	int width, height;
	float aspect_ratio, h_angle, v_angle;
	glm::mat4x4 m, v, p, mvp;
	double time = glfwGetTime();

	// Get framebuffer size and compute aspect ratio
	glfwGetFramebufferSize(window, &width, &height);
	aspect_ratio = width / (float)height;

	// Set viewport
	glViewport(0, 0, width, height);

	// Set clear color and clear buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create a MODEL matrix (identity)
	m = glm::mat4(1.0);

	// Create a VIEW matrix (LookAt target updated by mouse position)
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	h_angle = 2.0f*glm::pi<float>()*(float)(xpos / width);
	v_angle = -1.0f*(glm::pi<float>()*(float)(ypos / height) + glm::pi<float>()/2.0f);
	glm::vec3 eye = glm::vec3(0.0f, 1.5f, 0.0f);
	glm::vec3 target = glm::vec3(glm::sin(h_angle) * glm::cos(v_angle), glm::sin(v_angle), glm::cos(v_angle) * glm::cos(h_angle));
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	v = glm::lookAt(eye, eye + target, up);

	// Create a PROJECTION matrix (perspective)
	p = glm::perspective(glm::radians(90.0f), aspect_ratio, 0.1f, 100.f);
	
	// Compute MVP
	mvp = p * v * m;

	// Run shaders (draw points)
	glUseProgram(program);
	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

	// DRAW VOXELS (as points for now)
	glDrawArrays(GL_POINTS, 0, num_voxels);

	// Swap buffers
	glfwSwapBuffers(window);

	// Check for user input (or other events, e.g. window close)
	glfwPollEvents();
}

// Close window (and terminate GLFW)
void Display::Close()
{
	glfwTerminate();
}

// GLFW error callback function
void Display::Error_Callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
