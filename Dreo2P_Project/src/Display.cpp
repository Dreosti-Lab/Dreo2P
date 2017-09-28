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

// Initialize GLFW winodw
void Display::Initialize_Window(int width, int height)
{
	// Set GLFW error callback function
	glfwSetErrorCallback(Error_Handler);

	// Set window size members
	width_ = width;
	height_ = height;

	// Initialize the GLFW library
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	// Specify OpenGL version (4.1)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a windowed mode window and its OpenGL context
	window_ = glfwCreateWindow(width, height, "Dreo2P - Live", NULL, NULL);
	if (!window_)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Make the window's context current
	glfwMakeContextCurrent(window_);

	// Start OpenGL extensions loader library (GLAD)
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Show "hand" cursor when in window	
	// glfwSetInputMode(window_, GLFW_CURSOR, GLFW_HAND_CURSOR);

	// Report graphics hardware and OpenGL version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n\n", version);

	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	// Set swap interval (vsync = 1)
	glfwSwapInterval(1);
}

// Is not necessary...
void Display::Set_Frame(float intensity)
{
	intensity_ = intensity;

	// Fill a test texture with float values
	float* test_texture = (float*)malloc(sizeof(float) * width_ * height_ * 4);
	for (int i = 0; i < (width_ * height_ * 4); i++) {
		test_texture[i] = intensity_;
	}
	glBindTexture(GL_TEXTURE_2D, frame_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_BGRA, GL_FLOAT, test_texture);
}


// Initialize rendering: load, compile and attach shaders and set vertex attribute arrays
void Display::Initialize_Render()
{
	// Create texture for image data
	glGenTextures(1, &frame_texture);
	glBindTexture(GL_TEXTURE_2D, frame_texture);

	// Fill a test texture with float values
	float* test_texture = (float*) malloc(sizeof(float) * width_ * height_ * 4);
	for (int i = 0; i < (width_ * height_ * 4); i++) {
		test_texture[i] = (float)i/ (width_ * height_ * 4.0f);
	}
	std::cout << "Texture made." << test_texture[1000] << "\n";

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_FLOAT, width_, height_, 0, GL_RED, GL_FLOAT, test_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_BGRA, GL_FLOAT, test_texture);
	std::cout << "Texture built.\n";

	// Generate and bind vertex buffer (single full viewport quad)
	GLuint	vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	// XYZ (Quad Triangle Fan)
	float vertices[12] = { 
		-1.0f, -1.0f, 0.0f, 
		-1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);

	GLuint	uv_buffer;
	glGenBuffers(1, &uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	// UV (Quad)
	float uvs[8] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, uvs, GL_STATIC_DRAW);

	// Generate vertex attribute array (why??)
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Specify layout of vertex attributes (0 = vertex positions)
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Specify layout of vertex attributes (1 = uv coordinates)
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Store shader text here for now...
	static const char* vertex_shader_text =
		"#version 400\n"
		"layout(location = 0) in vec3 vp;\n"
		"layout(location = 1) in vec2 uv;\n"
		"out vec2 tex_coord;\n"
		"void main() {\n"
		"	tex_coord = uv;\n"
		"	gl_Position = vec4(vp, 1.0);\n"
		"}\n";

	static const char* fragment_shader_text =
		"#version 400\n"
		"uniform sampler2D tex;\n"
		"in vec2 tex_coord;\n"
		"out vec4 frag_color;\n"
		"void main() {\n"
		"	frag_color = texture(tex, tex_coord);\n"
		"}\n";

	// Load and compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);
	
	std::cout << "\n\nVertex Shader:\n";
	printf(vertex_shader_text);
	std::cout << "Log:\n";
	int log_length;
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
	char* buffer = (char*)malloc(sizeof(char) * log_length);
	glGetShaderInfoLog(vertex_shader, log_length, &log_length, buffer);
	printf(buffer);

	// Load and compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);

	std::cout << "\n\nFragment Shader:\n";
	printf(fragment_shader_text);
	std::cout << "Log:\n";
	glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
	buffer = (char*)malloc(sizeof(char) * log_length);
	glGetShaderInfoLog(fragment_shader, log_length, &log_length, buffer);
	printf(buffer);
	std::cout << "\n===============\n";

	// Attach shaders to a "program"
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	// Gather addresses of uniforms (matrices, vertex positions, and colors)
	//max_location = glGetUniformLocation(program, "max");
}

// Render (draw calls, rescaling, user view updates, etc.)
void Display::Render()
{
	// Local variables
	int width, height;
	float aspect_ratio;
	double time = glfwGetTime();

	// Get framebuffer size and compute aspect ratio
	glfwGetFramebufferSize(window_, &width, &height);
	aspect_ratio = width / (float)height;

	// Set viewport
	glViewport(0, 0, width, height);

	// Set clear color and clear buffer
	glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Get user input
	double xpos, ypos;
	glfwGetCursorPos(window_, &xpos, &ypos);

	// Run shaders (draw points)
	glUseProgram(program);


	// Specify where texture uniform will be
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_texture);
	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	// Specify which vertex attribute array object to use
	glBindVertexArray(vao);

	// Update uniform?
	//glUniformMatrix4fv(max_location, 1, GL_FALSE, glm::value_ptr(m));
	

	
	// Draw Quad (as two triangles)
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Check for user input (or other events, e.g. window close)
	glfwPollEvents();

	// Swap buffers
	glfwSwapBuffers(window_);

	glGetError();
}

// Close window (and terminate GLFW)
void Display::Close()
{
	glfwTerminate();
}

// Display error handler
void Display::Error_Handler(int error, const char* description)
{
	fprintf(stderr, "Display Error: %s\n", description);
}
