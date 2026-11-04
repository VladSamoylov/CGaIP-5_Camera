#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <iostream>

const GLint WIDTH = 800, HEIGHT = 800; // Константи початкової ширини та висоти вікна
GLuint shader, uniformModel, uniformProjection, uniformView; // Глобальні змінні для об'єктів у пам'яті відеокарти shader - ID скомпільованої шейдерної програми, а uniformModel - ID для передачі матриці 4х4 у вершинний шейдер...
const float toRadians = 3.14159265f / 180.0f; // Коефіцієнт для переведення кутів із градусів у радіани
float curAngle = 0.0f; // Поточний кут повороту фігури
// Структура, що об'єднує дані одного об'єкта
struct Figure {
	GLuint VAO; // Ідентифікатор  масиву налаштувань буферів
	GLuint VBO; // Ідентифікатор буфера з чистими координатами у відеопам'яті
	GLuint IBO; // Ідентифікатор буфера індексів 
	int verteciesAmount; // Загальна кількість вершин у фігурі
};

// Створюємо текст вершиного шейдера, який використовується для перетворення вершин у відповідні координати на екрані
static const char* vShader = R"(
	#version 330 // Вказуємо відеокарті, що код написаний на мові GLSL версії 3.3
	// in (це вхідні дані, які приходять у шейдер ззовні)
	// vec3 (координати у форматі вектора 3-х чисел типу float для кожної точки)
	// layout (location = 0) номер порту, який зв'язує C++ масив із цим шейдером
	layout(location = 0) in vec3 pos;
	out vec4 vCol; // Вихідна змінна: передає колір із вершинного шейдера далі
	uniform mat4 model; // Створюємо Uniform-змінну для передачі матриці моделі 4х4
	uniform mat4 projection; // Створюємо Uniform-змінну для передачі матриці проєкції 4х4
	uniform mat4 view; // Створюємо Uniform-змінну для передачі матриці виду 4х4
	void main()	{ // Головна функція шейдера, яка викликається окремо для кожної вершини
			gl_Position = projection * view * model * vec4(pos, 1.0); // Обчислюємо фінальну позицію точки на екрані монітора шляхом послідовного перемноження матриць
			vCol = vec4(clamp(pos, 0.0f, 1.0f), 1.0); // Перетворюємо просторові координати точки на колір
	}
)";

// Створюємо текст фрагметного шейдера, який буде керувати кольором пікселей
static const char* fShader = R"(
	#version 330
	in vec4 vCol; // Вхідна змінна: приймає вже автоматично проінтерполований колір для певного пікселя
	out vec4 colour; // out (вихідні дані, які шейдер відає назовні для подальшого відображення пікселю на екран
		void main()	{ // Головна функція шейдера, яка викликається окремо для кожного пікселя
			colour = vCol; // Присвоюємо поточному пікселю його унікальний, отриманий після інтерполяції колір
		}
)";

// Клас для реалізації камери виду
class Camera {
	public:
		Camera();
		glm::mat4 getViewMatrix(); // Метод розрахунку та повернення фінальної матриці виду
		void processInput(GLFWwindow* window); // Метод опитування натискання клавіш клавіатури для керування камери
	private:
		glm::vec3 cameraPos; // Вектор поточної позиції камери 
		glm::vec3 cameraFront; // Вектор напрямку погляду камери
		glm::vec3 cameraUp; // Вектор верху камери
		float deltaTime; // Час, який знадобився відеокарті на рендеринг одного минулого кадру
		float lastFrame; // Системний час моменту рендерингу попереднього кадру
		float rollAngle; // Кут обертання камери навколо осі Z
		float pitchAngle; // Кут нахилу камери вгору-вниз навколо осі X 
		float jawAngle; // Кут повороту камери вліво-вправо навколо осі Y
};

// Конструктор класу, де ініціалізуються  
Camera::Camera()
	: cameraPos(glm::vec3(0.0f, 0.0f, 3.0f)),
	cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
	cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	deltaTime(0.0f),
	lastFrame(0.0f), rollAngle(0.0f), pitchAngle(0.0f), jawAngle(0.0f) {}

// Метод розрахунку та повернення фінальної матриці виду
glm::mat4 Camera::getViewMatrix() {
	glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), rollAngle * toRadians, glm::vec3(0.0f, 0.0f, -1.0f)); // Ініціалізуємо матрицю обертання камери
	glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), pitchAngle * toRadians, glm::vec3(1.0f, 0.0f, 0.0f)); // Ініціалізуємо матрицю нахилу камери
	glm::mat4 jawMatrix = glm::rotate(glm::mat4(1.0f), jawAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // Ініціалізуємо матрицю повороту камери
	glm::mat4 finMatrix = jawMatrix * pitchMatrix * rollMatrix; // Об'єднуємо три матриці у фінальну
	cameraFront = glm::vec3(finMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)); // Перераховуємо куди тепер спрямований погляд камери
	cameraUp = glm::vec3(finMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Перераховуємо верх камери
	return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // Передаємо готову матрицю виду: передаємо позицію камери, точку фокусу та де вершина камери 
}

// Метод опитування натискання клавіш клавіатури для керування камери
void Camera::processInput(GLFWwindow* window) {
	// Синхронізація часу: рахуємо чисту тривалість рендерингу попереднього кадру в секундах
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	const float cameraSpeed = 2.5f * deltaTime; // Обчислюємо швидкість камери, для того щоб вона не залежала від частоти кадрів
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront; // Камера летить вперед за вектором погляду
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront; // Камера летить назад
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; // Рух ліворуч: вироховуємо перпендикуляр "вбік" через векторний добуток погляду і нахиленого верху
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; // Рух праворуч
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) rollAngle += cameraSpeed * 50; // Обератання камери ліворуч
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) rollAngle -= cameraSpeed * 50; // Обертання камери праворуч
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) pitchAngle += cameraSpeed * 50; // Нахил камери вгору
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) pitchAngle -= cameraSpeed * 50; // Нахил камери вниз
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) jawAngle -= cameraSpeed * 50; // Поворот камери праворуч
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) jawAngle += cameraSpeed * 50; // Поворот камери ліворуч
	
}

// Функція створення фігури
Figure CreateFigure(GLfloat* vertices, int size, unsigned short* indices, int sizeIndices) {
	Figure newFig;
	newFig.verteciesAmount = size / (3 * sizeof(GLfloat)); // Рахуємо кількість вершин
	glGenVertexArrays(1, &newFig.VAO); // Створюємо VAO, яка буде запам'ятовувати всі налаштування буферів 
	glBindVertexArray(newFig.VAO); // Активуємо VAO

	glGenBuffers(1, &newFig.IBO); // Створюємо та ініціціалізуємо буфер індексів для оптимізації роботи з вершинами та підвищення ефективності малювання складних геометричних об'єктів
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newFig.IBO); // Активуємо індексний буфер
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIndices, indices, GL_STATIC_DRAW); // Копіюємо масив індексів з ОЗП до VRAM

	glGenBuffers(1, &newFig.VBO); // Створюємо VBO буфер, куди фізично завантажимо координати фігури
	glBindBuffer(GL_ARRAY_BUFFER, newFig.VBO); // Активуємо VBO
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW); // Копіюємо масив точок з ОЗП у VRAM
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Створюємо декодер-пояснення як читати байти (слот №0, 3 числа, типу флоат, без нормалізації, крок 0, зміщення 0)
	glEnableVertexAttribArray(0); // Вмикаємо декодер для слоту 0
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Відв'язуємо буфери, щоб випадково не стерти дані в іншому місці програми
	glBindVertexArray(0);
	return newFig;
}

// Допоміжна функція для компіляції одного шейдера
void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
	GLuint theShader = glCreateShader(shaderType); // Створюємо порожній об'єкт шейдера потрібного типу (вершинний або фрагментний)
	const GLchar* theCode[1];
	theCode[0] = shaderCode; // Передаємо текст програми нашого шейдера
	GLint codeLength[1];
	codeLength[0] = strlen(shaderCode); // Рахуємо довжину тексту
	glShaderSource(theShader, 1, theCode, codeLength); // Передаємо текст шейдера у створений порожній об'єкт шейдера
	glCompileShader(theShader); // Компілюємо шейдер
	// Перевіряємо на помилки в коді GLSL шейдера
	GLint result = 0;
	GLchar eLog[1024] = { 0 };
	glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(theShader, 1024, NULL, eLog);
		fprintf(stderr, "Error compiling the %d shader:	'%s'\n", shaderType, eLog); // Якщо компіляція провалилася до виводимо інформацію
		return;
	}
	glAttachShader(theProgram, theShader); // Інакше прикріплюємо цей скомпільований шейдер до нашої програми
}

void CompileShaders() {
	shader = glCreateProgram(); // Створюємо програму-інструкцію для відеокарти
	if (!shader)
	{
		printf("Failed to create shader\n");
		return;
	}
	AddShader(shader, vShader, GL_VERTEX_SHADER); // Додаємо та компілюємо вершинний шейдер
	AddShader(shader, fShader, GL_FRAGMENT_SHADER); // Додаємо та компілюємо фрагментний шейдер
	GLint result = 0;
	GLchar eLog[1024] = { 0 };
	glLinkProgram(shader); // З'єднує шейдер в одну інструкцію
	// Перевіряємо чи успішно вони з'єдналися між собою
	glGetProgramiv(shader, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error linking program: '%s'\n", eLog);
		return;
	}
	// Перевіряємо, чи зможе взагалі цей шейдер запуститися на поточній відеокарті
	glValidateProgram(shader);
	glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error validating program: '%s'\n", eLog);
		return;
	}
	uniformModel = glGetUniformLocation(shader, "model"); // находимо адресу змінної "model" всередині шейдеру і записуємо її в глобальний ідентифікатор uniformModel
	uniformProjection = glGetUniformLocation(shader, "projection"); // находимо адресу змінної "projection" всередині шейдеру і записуємо її в глобальний ідентифікатор uniformProjection
	uniformView = glGetUniformLocation(shader, "view"); // находимо адресу змінної "view" всередині шейдеру і записуємо її в глобальний ідентифікатор uniformView
}

int main() {
	if (!glfwInit()) // Спроба ініціалізації бібліотеки GLFW, яка потрібна для створення віконих інтерфейсів та обробки подій в графічних програмах
	{
		printf("GLFW failed to start :(");
		glfwTerminate(); // У разі не ініціалізації бібліотеки GLFW виконується очищення ресурсів і завершення виконання програми
		return 1;
	}
	// Налаштовуємо OpenGL перед створенням вікна
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Встановлюємо головну версію OpenGL 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Встановлюємо додаткову версію OpenGL 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Використовуємо Core-профіль, профіль сучасного OpenGL без застарілих функцій
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Вмикаємо зворотну сумісність для коректної роботи з macOS та новими драйверами
	GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "by Vladislav Samoilov KNT-223", NULL, NULL); // Створюємо головне вікно програми із заданими розмірами та заколовком
	if (!mainWindow) // Перевіряємо ініціалізацію вікна
	{
		printf("GLFW window creation failed :(");
		glfwTerminate(); // Закриваємо GLFW, якщо вікно не вдалося створити та завершаємо виконання програми
		return 1;
	}
	int bufferWidth, bufferHeight; // Змінні для збереження реального розміру області рендерингу (в пікселях)
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight); // Отримуємо фактичний розмір буфера кадру вікна
	glfwMakeContextCurrent(mainWindow); // Робимо контекст OpenGL створеного вікна поточним для цього потоку програми
	glewExperimental = GL_TRUE; // Дозволяємо GLEW завантажувати експериментальні та нові розширення OpenGL
	if (glewInit() != GLEW_OK) // Перевіряємо ініціалізацію бібліотеки GLEW, який динамічно завантажує функції OpenGL для відеокарти
	{
		printf("GLEW failed to start >_<");
		glfwDestroyWindow(mainWindow); // Знищуємо створене вікно
		glfwTerminate(); // Завершуємо роботу GLFW
		return 1;
	}
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, bufferWidth, bufferHeight); // Налаштовуємо область перегляду OpenGL

	// Координати вершин куба
	GLfloat squareV[] = {
		-1.0f, 1.0f, 1.0f, // Точка 0: лівий верхній передній кут
		-1.0f, -1.0f, 1.0f, // Точка 1: лівий нижній передній кут
		-1.0f, -1.0f, -1.0f, // Точка 2: лівий нижній задній кут
		-1.0f, 1.0f, -1.0f, // Точка 3: лівий верхній задній кут

		1.0f, 1.0f, 1.0f, // Точка 4: правий верхній передній кут
		1.0f, -1.0f, 1.0f, // Точка 5: правий нижній передній кут
		1.0f, -1.0f, -1.0f, // Точка 6: правий нижній задній кут
		1.0f, 1.0f, -1.0f // Точка 7: правий верхній задній кут
	};

	// Масив індексів для збірки куба (6 граней по 2 трикутника, разом 36 індексів для 12 унікальних трикутників)
	unsigned short indices[] = {
		0, 1, 2, // Ліва грань (трикутник №1)
		0, 2, 3, // Ліва грань (трикутник №2)
		0, 1, 5, // Передня грань (трикутник №1)
		0, 4, 5, // Передня грань (трикутник №2)
		4, 5, 6, // Права грань (трикутник №1)
		4, 6, 7, // Права грань (трикутник №2)
		0, 4, 7, // Верхня грань (трикутник №1)
		0, 3, 7, // Верхня грань (трикутник №2)
		1, 2, 6, // Нижня грань (трикутник №1)
		1, 5, 6, // Нижня грань (трикутник №2)
		2, 3, 6, // Задня грань (трикутник №1) 
		3, 6, 7  // Задня грань (трикутник №1) 
	};

	Figure squre = CreateFigure(squareV, sizeof(squareV), indices, sizeof(indices)); // Створюємо геометрію куба
	CompileShaders(); // Компілюємо шейдери
	// Створюємо перспективну матрицю проєкції: кут 90 градусів, пропорції екрана, межі видимості від 0.1 до 100 метрів
	glm::mat4 projection = glm::perspective(glm::radians(90.0f), (GLfloat)bufferWidth / (GLfloat)bufferHeight, 0.1f, 100.0f);

	Camera cam; 

	while (!glfwWindowShouldClose(mainWindow)) // Головний цикл роботи програми, працює, поки користувач не закриє вікно
	{
		glfwPollEvents(); // Перевіряємо та обробляємо події
		cam.processInput(mainWindow);
		glClearColor(0.0f, 0.3f, 1.0f, 1.0f); // Встановлюємо колір для очищення екрану RGBA
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Очищаємо буфер кольору кадру за допомогою вказаного вище кольору
		glUseProgram(shader); // Вмикаємо шейдерну програму
		glm::mat4 model(1.0f); // Ініціалізація одиничної матриці моделі
		glm::mat4 view = cam.getViewMatrix(); // Створюємо актуальну матрицю виду

		model = glm::rotate(model, 30.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // Додаємо нахил фігури на 45 градусів по осі Х
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model)); // Передаємо готову фінальну матрицю 4х4 у вершинний шейдер без перевороту (GL_FALSE)
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection)); // Передаємо перспективну матрицю проєкції у вершинний шейдер
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view)); // Передаємо матрицю виду у вершинний шейдер
		glBindVertexArray(squre.VAO); // Вмикаємо налаштування фігури
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0); // Даємо команду намалювати фігуру куб

		// Очищення, для того, щоб інші об'єкти не малювалися цим шейдером
		glBindVertexArray(0);
		glUseProgram(0);
		glfwSwapBuffers(mainWindow); // Міняємо місцями задній буфер (де ми малювали) і передній буфер (який бачить користувач)
	}
	return 0;
}