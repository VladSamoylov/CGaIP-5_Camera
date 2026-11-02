#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const float toRadians = 3.14159265f / 180.0f;

class Camera {
public:
	Camera();
	glm::mat4 getViewMatrix();
	void processInput(GLFWwindow* window);
private:
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
	glm::vec3 tiltedUp;
	float deltaTime;
	float lastFrame;
	float rollAngle;
	float pitchAngle;
	float jawAngle;
};

Camera::Camera()
	: cameraPos(glm::vec3(0.0f, 0.0f, 3.0f)),
	cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
	cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)), 
	tiltedUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	deltaTime(0.0f),
	lastFrame(0.0f), rollAngle(0.0f), pitchAngle(0.0f), jawAngle(0.0f)  {}

glm::mat4 Camera::getViewMatrix() {
	glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), rollAngle * toRadians, cameraFront);
	tiltedUp = glm::vec3(rollMatrix * glm::vec4(cameraUp, 1.0f));
	glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), pitchAngle * toRadians, glm::cross(cameraFront, tiltedUp));
	glm::mat4 jawMatrix = glm::rotate(glm::mat4(1.0f), jawAngle * toRadians, tiltedUp);
	glm::mat4 finMatrix = pitchMatrix * jawMatrix;
	cameraFront = glm::vec3(finMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f));
	return glm::lookAt(cameraPos, cameraPos + cameraFront, tiltedUp);
}
void Camera::processInput(GLFWwindow* window) {
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	const float cameraSpeed = 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, tiltedUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, tiltedUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) rollAngle += cameraSpeed * 50;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) rollAngle -= cameraSpeed * 50;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) pitchAngle += cameraSpeed * 50; 
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) pitchAngle -= cameraSpeed * 50;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) jawAngle += cameraSpeed * 50;
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) jawAngle -= cameraSpeed * 50;
	if (pitchAngle > 89) pitchAngle = 89;
	if (pitchAngle < -89) pitchAngle = -89;
}

int main() {
	if (!glfwInit()) {
		fprintf(stderr, "GLFW initialization failed\n");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Camera Example", NULL, NULL);
		if (!window) {
			fprintf(stderr, "GLFW window creation failed\n");
			glfwTerminate();
			return -1;
		}
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "GLEW initialization failed\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 800, 600);

	// Shader source code
	const char* vertexShaderSource = R"(
		 #version 330 core
		 layout (location = 0) in vec3 aPos;
		 uniform mat4 model;
		 uniform mat4 view;
		 uniform mat4 projection;
		 void main() {
		gl_Position = projection * view * model *
		vec4(aPos, 1.0);
		 }
	 )";
	const char* fragmentShaderSource = R"(
 #version 330 core
 out vec4 FragColor;
 void main() {
 FragColor = vec4(1.0, 0.5, 0.2, 1.0);
 }
 )";

	// Compile and link shaders
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource,
		NULL);
	glCompileShader(vertexShader);
	GLuint fragmentShader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	// Vertex data
	GLfloat vertices[] = {
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	0.0f, 0.5f, 0.0f
	};
	// Vertex array and buffer objects
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 *	sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	// Uniform locations
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram,	"view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
	Camera camera;

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		camera.processInput(window);
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(90.0f), 800.0f / 600.0f, 0.1f, 100.0f);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
			
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

