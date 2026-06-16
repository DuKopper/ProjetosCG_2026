#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "stb_image.h"

using namespace std;
namespace fs = std::filesystem;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
	glm::vec3 position;

	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	float yaw;
	float pitch;

	Camera()
	{
		position = glm::vec3(0.0f, 0.0f, 3.0f);

		yaw = -90.0f;
		pitch = 0.0f;

		front = glm::vec3(0.0f, 0.0f, -1.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);

		updateVectors();
	}

	void moveForward(float speed)
	{
		position += front * speed;
	}

	void moveBackward(float speed)
	{
		position -= front * speed;
	}

	void moveLeft(float speed)
	{
		position -= right * speed;
	}

	void moveRight(float speed)
	{
		position += right * speed;
	}

	void rotate(float yawOffset, float pitchOffset)
	{
		yaw += yawOffset;
		pitch += pitchOffset;

		if (pitch > 89.0f)
			pitch = 89.0f;

		if (pitch < -89.0f)
			pitch = -89.0f;

		updateVectors();
	}

	void updateVectors()
	{
		glm::vec3 direction;

		direction.x =
			cos(glm::radians(yaw)) *
			cos(glm::radians(pitch));

		direction.y =
			sin(glm::radians(pitch));

		direction.z =
			sin(glm::radians(yaw)) *
			cos(glm::radians(pitch));

		front = glm::normalize(direction);

		right =
			glm::normalize(
				glm::cross(front,
						   glm::vec3(0.0f, 1.0f, 0.0f)));

		up =
			glm::normalize(
				glm::cross(right,
						   front));
	}

	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(
			position,
			position + front,
			up);
	}
};

Camera camera;

int loadSimpleOBJ(string filePATH, int &nVertices)
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;
	std::vector<GLfloat> vBuffer;

	std::ifstream arqEntrada(filePATH.c_str());
	if (!arqEntrada.is_open())
	{
		std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
		return -1;
	}

	std::string line;
	while (std::getline(arqEntrada, line))
	{
		std::istringstream ssline(line);
		std::string word;
		ssline >> word;

		if (word == "v")
		{
			glm::vec3 vertice;
			ssline >> vertice.x >> vertice.y >> vertice.z;
			vertices.push_back(vertice);
		}
		else if (word == "vt")
		{
			glm::vec2 vt;
			ssline >> vt.s >> vt.t;
			texCoords.push_back(vt);
		}
		else if (word == "vn")
		{
			glm::vec3 normal;
			ssline >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (word == "f")
		{
			while (ssline >> word)
			{
				int vi = 0, ti = 0, ni = 0;
				std::istringstream ss(word);
				std::string index;

				if (std::getline(ss, index, '/'))
					vi = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index, '/'))
					ti = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index))
					ni = !index.empty() ? std::stoi(index) - 1 : 0;

				vBuffer.push_back(vertices[vi].x);
				vBuffer.push_back(vertices[vi].y);
				vBuffer.push_back(vertices[vi].z);

				vBuffer.push_back(texCoords[ti].x);
				vBuffer.push_back(texCoords[ti].y);

				vBuffer.push_back(normals[ni].x);
				vBuffer.push_back(normals[ni].y);
				vBuffer.push_back(normals[ni].z);
			}
		}
	}

	arqEntrada.close();

	std::cout << "Gerando o buffer de geometria..." << std::endl;
	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
						  8 * sizeof(GLfloat),
						  (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		8 * sizeof(GLfloat),
		(GLvoid *)0);

	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
		2,
		3,
		GL_FLOAT,
		GL_FALSE,
		8 * sizeof(GLfloat),
		(GLvoid *)(5 * sizeof(GLfloat)));

	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 8;

	return VAO;
}

GLuint loadTexture(const char *path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);

	// parâmetros
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D,
					GL_TEXTURE_MIN_FILTER,
					GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D,
					GL_TEXTURE_MAG_FILTER,
					GL_LINEAR);

	int width, height, nrChannels;

	unsigned char *data =
		stbi_load(path, &width, &height,
				  &nrChannels, 0);

	if (data)
	{
		GLenum format;

		if (nrChannels == 3)
			format = GL_RGB;
		else
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 format,
					 width,
					 height,
					 0,
					 format,
					 GL_UNSIGNED_BYTE,
					 data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	stbi_image_free(data);

	return textureID;
}

GLuint loadTextureFromMTL(std::string mtlPath)
{
	std::ifstream file(mtlPath);

	if (!file.is_open())
	{
		std::cout << "Erro ao abrir MTL\n";
		return 0;
	}

	fs::path baseDir = fs::path(mtlPath).parent_path();

	std::string line;

	while (getline(file, line))
	{
		std::istringstream ss(line);

		std::string word;
		ss >> word;

		if (word == "map_Kd")
		{
			std::string textureFile;
			ss >> textureFile;

			fs::path fullPath = baseDir / textureFile;

			std::cout << "Textura encontrada: "
					  << fullPath << std::endl;

			return loadTexture(fullPath.string().c_str());
		}
	}

	return 0;
}

struct OBJ
{
	GLuint VAO;
	int nVertices;
	GLuint textureID;

	glm::vec3 position;
	glm::vec3 rotationAxis;
	float angle;
	glm::vec3 scale;
	bool rotating;
	std::vector<glm::vec3> trajectoryPoints;
	bool followingTrajectory = false;
	int currentTargetPoint = 0;
	float trajectorySpeed = 0.01f;
};

std::vector<OBJ> objects;

int selectedObject = 0;

bool lightEnabled[3] =
	{
		false,
		false,
		false};

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 330 core\n"
								   "layout (location = 0) in vec3 position;\n"
								   "layout (location = 1) in vec2 texCoord;\n"
								   "layout (location = 2) in vec3 normal;\n"
								   "out vec2 TexCoord;;\n"
								   "out vec3 Normal;\n"
								   "out vec3 FragPos;\n"
								   "uniform mat4 model;\n"
								   "uniform mat4 view;\n"
								   "uniform mat4 projection;\n"
								   "void main()\n"
								   "{\n"
								   "FragPos = vec3(model * vec4(position, 1.0));\n"
								   "Normal = mat3(transpose(inverse(model))) * normal;\n"
								   "TexCoord = texCoord;\n"
								   "gl_Position = projection * view * model * vec4(position, 1.0);\n"
								   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 330 core\n"
									 "in vec2 TexCoord;\n"
									 "in vec3 Normal;\n"
									 "in vec3 FragPos;\n"
									 "out vec4 color;\n"
									 "uniform sampler2D texture1;\n"
									 "uniform vec3 lightPos[3];\n"
									 "uniform vec3 lightColor[3];\n"
									 "uniform bool lightEnabled[3];\n"
									 "uniform vec3 viewPos;\n"
									 "uniform vec3 Ka;\n"
									 "uniform vec3 Kd;\n"
									 "uniform vec3 Ks;\n"
									 "uniform float Ns;\n"
									 "void main()\n"
									 "{\n"
									 "vec3 texColor = texture(texture1, TexCoord).rgb;\n"
									 "vec3 norm = normalize(Normal);\n"
									 "vec3 viewDir = normalize(viewPos - FragPos);\n"
									 "vec3 totalLight = vec3(0.0);\n"
									 "for(int i = 0; i < 3; i++) {\n"
									 "if(!lightEnabled[i]) continue;\n"
									 "vec3 lightDir = normalize(lightPos[i] - FragPos);\n"
									 "float diff = max(dot(norm, lightDir), 0.0);\n"
									 "float distance = length(lightPos[i] - FragPos);\n"
									 "float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);\n"
									 "vec3 ambient = Ka * lightColor[i];\n"
									 "vec3 diffuse = Kd * diff * lightColor[i] * attenuation;\n"
									 "vec3 reflectDir = reflect(-lightDir, norm);\n"
									 "float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);\n"
									 "vec3 specular = Ks * spec * lightColor[i] * attenuation; totalLight += ambient + diffuse + specular;}\n"
									 "vec3 result = totalLight * texColor;\n"
									 "color = vec4(result, 1.0);\n"
									 "}\n\0";

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Eduardo!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	glm::mat4 projection =
		glm::perspective(
			glm::radians(45.0f),
			(float)WIDTH / HEIGHT,
			0.1f,
			100.0f);

	GLint projectionLoc =
		glGetUniformLocation(
			shaderID,
			"projection");

	glUniformMatrix4fv(
		projectionLoc,
		1,
		GL_FALSE,
		glm::value_ptr(projection));

	// Gerando um buffer simples
	int nVertices;
	GLuint objVAO = loadSimpleOBJ("C:\\Users\\user\\Documents\\Objetos 3D\\Suzanne.obj", nVertices);
	GLuint texture = loadTextureFromMTL("C:\\Users\\user\\Documents\\Texturas\\Suzanne.mtl");

	OBJ obj1;
	obj1.textureID = texture;
	obj1.VAO = objVAO;
	obj1.nVertices = nVertices;
	obj1.position = glm::vec3(-0.5f, 0.0f, 0.0f);
	obj1.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
	obj1.rotating = false;
	obj1.angle = 0.0f;
	obj1.scale = glm::vec3(0.3f);

	OBJ obj2 = obj1;
	obj2.textureID = texture;
	obj2.position = glm::vec3(0.5f, 0.0f, 0.0f);
	obj2.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	obj2.rotating = false;

	objects.push_back(obj1);
	objects.push_back(obj2);

	glUseProgram(shaderID);

	glm::vec3 lightPos[3] =
		{
			glm::vec3(2.0f, 2.0f, 2.0f),  // principal
			glm::vec3(-2.0f, 1.0f, 1.0f), // preenchimento
			glm::vec3(0.0f, 2.0f, -2.0f)  // fundo
		};

	glm::vec3 lightColor[3] =
		{
			glm::vec3(1.0f, 1.0f, 1.0f), // principal
			glm::vec3(0.4f, 0.4f, 0.4f), // preenchimento
			glm::vec3(0.6f, 0.6f, 0.6f)	 // fundo
		};

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightPos[0]"),
		1,
		glm::value_ptr(lightPos[0]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightPos[1]"),
		1,
		glm::value_ptr(lightPos[1]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightPos[2]"),
		1,
		glm::value_ptr(lightPos[2]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightColor[0]"),
		1,
		glm::value_ptr(lightColor[0]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightColor[1]"),
		1,
		glm::value_ptr(lightColor[1]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "lightColor[2]"),
		1,
		glm::value_ptr(lightColor[2]));

	glUniform3fv(
		glGetUniformLocation(shaderID, "viewPos"),
		1,
		glm::value_ptr(camera.position));

	glUniform3f(
		glGetUniformLocation(shaderID, "Ka"),
		0.2f,
		0.2f,
		0.2f);

	glUniform3f(
		glGetUniformLocation(shaderID, "Kd"),
		1.0f,
		1.0f,
		1.0f);

	glUniform3f(
		glGetUniformLocation(shaderID, "Ks"),
		1.0f,
		1.0f,
		1.0f);

	glUniform1f(
		glGetUniformLocation(shaderID, "Ns"),
		32.0f);

	glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

	glm::mat4 model = glm::mat4(1); // matriz identidade;

	GLint modelLoc = glGetUniformLocation(shaderID, "model");

	GLint viewLoc =
		glGetUniformLocation(shaderID, "view");

	//
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/ glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // preto
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		glUniform1i(
			glGetUniformLocation(shaderID, "lightEnabled[0]"),
			lightEnabled[0]);

		glUniform1i(
			glGetUniformLocation(shaderID, "lightEnabled[1]"),
			lightEnabled[1]);

		glUniform1i(
			glGetUniformLocation(shaderID, "lightEnabled[2]"),
			lightEnabled[2]);

		glm::mat4 view =
			camera.getViewMatrix();

		glUniformMatrix4fv(
			viewLoc,
			1,
			GL_FALSE,
			glm::value_ptr(view));

		glUniformMatrix4fv(
			projectionLoc,
			1,
			GL_FALSE,
			glm::value_ptr(projection));

		for (auto &obj : objects)
		{
			if (obj.followingTrajectory &&
				obj.trajectoryPoints.size() > 1)
			{
				glm::vec3 target =
					obj.trajectoryPoints[obj.currentTargetPoint];

				glm::vec3 direction =
					target - obj.position;

				float distance =
					glm::length(direction);

				if (distance < 0.05f)
				{
					obj.currentTargetPoint++;

					if (obj.currentTargetPoint >=
						obj.trajectoryPoints.size())
					{
						obj.currentTargetPoint = 0;
					}
				}
				else
				{
					obj.position +=
						glm::normalize(direction) *
						obj.trajectorySpeed;
				}
			}

			if (obj.rotating)
				obj.angle += 0.01f;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, obj.position);
			model = glm::rotate(model, obj.angle, obj.rotationAxis);
			model = glm::scale(model, obj.scale);

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glBindVertexArray(obj.VAO);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, obj.textureID);

			glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &objVAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	OBJ &obj = objects[selectedObject];

	// MOVIMENTO
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.position.y += 0.1f;

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.position.y -= 0.1f;

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.position.x -= 0.1f;

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.position.x += 0.1f;

	// ESCALA
	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.scale -= glm::vec3(0.1f);

	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.scale += glm::vec3(0.1f);

	if (obj.scale.x < 0.1f || obj.scale.y < 0.1f || obj.scale.z < 0.1f)
		obj.scale = glm::vec3(0.1f);

	// ROTAÇÃO
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
		objects[selectedObject].rotationAxis = glm::vec3(1, 0, 0);

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
		objects[selectedObject].rotationAxis = glm::vec3(0, 1, 0);

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		objects[selectedObject].rotationAxis = glm::vec3(0, 0, 1);

	// Seleção de objeto em cena
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		selectedObject = 0;

	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		selectedObject = 1;

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		objects[selectedObject].rotating = !objects[selectedObject].rotating;
	}

	// Seleção de luz em cena
	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
		lightEnabled[0] = !lightEnabled[0];

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
		lightEnabled[1] = !lightEnabled[1];

	if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
		lightEnabled[2] = !lightEnabled[2];

	// Controle da câmera
	if (key == GLFW_KEY_UP &&
		(action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		camera.moveForward(0.1f);
	}
	if (key == GLFW_KEY_DOWN &&
		(action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		camera.moveBackward(0.1f);
	}
	if (key == GLFW_KEY_LEFT &&
		(action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		camera.rotate(-2.0f, 0.0f);
	}

	if (key == GLFW_KEY_RIGHT &&
		(action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		camera.rotate(2.0f, 0.0f);
	}

	// Salvando checkpoint de posição
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		objects[selectedObject]
			.trajectoryPoints
			.push_back(
				objects[selectedObject].position);
	}

	// Iniciando trajetória entre checkpoints
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		objects[selectedObject].followingTrajectory =
			!objects[selectedObject].followingTrajectory;
	}
}

	int setupShader()
	{
		// Vertex shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		// Checando erros de compilação (exibição via log no terminal)
		GLint success;
		GLchar infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		// Fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		// Checando erros de compilação (exibição via log no terminal)
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		// Linkando os shaders e criando o identificador do programa de shader
		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		// Checando por erros de linkagem
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
					  << infoLog << std::endl;
		}
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return shaderProgram;
	}