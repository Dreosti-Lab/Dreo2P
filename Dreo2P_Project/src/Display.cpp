// Dreo2P Display Class (source)
#include "Display.h"

// Constructor
Display::Display(int frame_width, int frame_height)
{
	// Set frame texture size parameters
	frame_width_ = frame_width;
	frame_height_ = frame_height;
	use_A_ = true;

	// Make space for frame texture data
	frame_data_A_.resize(frame_width_ * frame_height_);
	frame_data_B_.resize(frame_width_ * frame_height_);
	
	// Start the display thread
	active_ = true;
	display_thread_ = std::thread(&Display::Display_Thread_Function, this);
}


// Destructor
Display::~Display()
{
}


// Display thread function
void Display::Display_Thread_Function()
{
	// Configure GLFW display (must be done in same thread as the rendering)
	Display::Initialize_Window(1024, 1024);
	Display::Initialize_Render();

	// Run Render Loop
	while (Display::active_)
	{
		// Update frame texture
		Display::Update_Frame();

		// Draw stuff
		Display::Render();
	}
}


// Initialize GLFW winodw
void Display::Initialize_Window(int width, int height)
{
	// Set GLFW error callback function
	glfwSetErrorCallback(Error_Handler);

	// Set window size members
	window_width_ = width;
	window_height_ = height;

	// Initialize the GLFW library
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	// Specify OpenGL version (4.1)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For older MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a windowed mode window and its OpenGL context
	window_ = glfwCreateWindow(window_width_, window_height_, "Dreo2P - Live", NULL, NULL);
	if (!window_)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Make the window's context current
	glfwMakeContextCurrent(window_);

	// Start OpenGL extensions loader library (GLAD)
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Modify cursor when in window	
	// glfwSetInputMode(window_, GLFW_CURSOR, GLFW_HAND_CURSOR);

	// Report graphics hardware and OpenGL version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("OpenGL:\n");
	printf("-------\n");
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported: %s\n", version);

	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	// Set swap interval (vsync = 1)
	glfwSwapInterval(1);
}


// Initialize rendering: compile/attach shaders and set vertex attribute arrays
void Display::Initialize_Render()
{
	// Create texture object for display frame, bind texture for updating
	glGenTextures(1, &frame_texture_);
	glBindTexture(GL_TEXTURE_2D, frame_texture_);

	// Set texture properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Load default texture from file
	Display::Update_Frame();

	// Generate and bind vertex buffer (single full viewport quad with uv coordinates)
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
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, uvs, GL_STATIC_DRAW);

	// Generate vertex attribute array to store all VA calls
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	// Specify layout of vertex attributes (0 = vertex positions)
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Specify layout of vertex attributes (1 = uv coordinates)
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Define vertex shader
	static const char* vertex_shader_text =
		"#version 400\n"
		"layout(location = 0) in vec3 vp;\n"
		"layout(location = 1) in vec2 uv;\n"
		"out vec2 tex_coord;\n"
		"void main() {\n"
		"	tex_coord = uv;\n"
		"	gl_Position = vec4(vp, 1.0);\n"
		"}\n";

	// Define fragment shader
	static const char* fragment_shader_text =
		"#version 400\n"
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"in vec2 tex_coord;\n"
		"out vec4 frag_color;\n"
		"float val;\n"
		"void main() {\n"
		"	val = texture(tex, tex_coord).r;\n"
		" 	val = (val - min)/(max-min);\n"
		"	frag_color.rgba = vec4(val);\n"
		"}\n";

	// Load and compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);
	
	// Report shader and log (errors?)
	std::cout << "\nVertex Shader:\n";
	printf(vertex_shader_text);
	std::cout << "Log:\n";
	int log_length;
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
	char* buffer = (char*)malloc(sizeof(char) * log_length);
	glGetShaderInfoLog(vertex_shader, log_length, &log_length, buffer);
	printf(buffer);
	std::cout << "\n--------------\n";

	// Load and compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);

	// Report shader and log (errors?)
	std::cout << "\nFragment Shader:\n";
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
	min_location = glGetUniformLocation(program, "min");
	max_location = glGetUniformLocation(program, "max");

	return;
}


// Update display frame texture (must be called on the Display (OpenGL) thread)
void Display::Update_Frame()
{
	// Bind texture and update data
	glBindTexture(GL_TEXTURE_2D, frame_texture_);
	// - Double buffered
	if (use_A_) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, frame_width_, frame_height_, 0, GL_RED, GL_FLOAT, frame_data_A_.data());
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, frame_width_, frame_height_, 0, GL_RED, GL_FLOAT, frame_data_B_.data());
	}
}


// Render (draw calls, rescaling, user view updates, etc.)
void Display::Render()
{
	// Local variables
	float aspect_ratio;
	double time = glfwGetTime();

	// Get framebuffer size and compute aspect ratio
	glfwGetFramebufferSize(window_, &window_width_, &window_height_);
	aspect_ratio = window_width_ / (float)window_height_;

	// Set viewport size
	glViewport(0, 0, window_width_, window_height_);

	// Set clear color and clear buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Get user input (mouse cursor position)
	double xpos, ypos;
	glfwGetCursorPos(window_, &xpos, &ypos);

	// DRAWING

	// Run shaders (draw points)
	glUseProgram(program);

	// Specify where texture uniform is located
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_texture_);
	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	// Update uniform
	glUniform1f(min_location, min_);
	glUniform1f(max_location, max_);

	// Specify which vertex attribute array object to use
	glBindVertexArray(vertex_array_object);
	
	// Draw Quad (as two triangles)
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Check for user input (or other events, e.g. window close)
	glfwPollEvents();

	// Swap buffers (v-synced)
	glfwSwapBuffers(window_);
}


// Stop thread, close window (and terminate GLFW)
void Display::Close()
{
	// End scanning thread (if active)
	if (active_)
	{
		active_ = false;
		display_thread_.join();
	}

	// Close GLFW window and terminate
	glfwTerminate();
}


// Display error handler
void Display::Error_Handler(int error, const char* description)
{
	fprintf(stderr, "Display Error: %s\n", description);
}
