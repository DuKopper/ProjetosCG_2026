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

// Armazena indices de um vertice de uma face do Obj.
struct FaceVertex
{
	int vi;
	int ti;
	int ni;
};

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
			std::vector<FaceVertex> face;

			while (ssline >> word)
			{
				FaceVertex fv;

				fv.vi = 0;
				fv.ti = 0;
				fv.ni = 0;

				std::istringstream ss(word);
				std::string index;

				if (std::getline(ss, index, '/'))
					fv.vi = !index.empty() ? std::stoi(index) - 1 : 0;

				if (std::getline(ss, index, '/'))
					fv.ti = !index.empty() ? std::stoi(index) - 1 : 0;

				if (std::getline(ss, index))
					fv.ni = !index.empty() ? std::stoi(index) - 1 : 0;

				face.push_back(fv);
			}

			for (size_t i = 1; i < face.size() - 1; i++)
			{
				FaceVertex tri[3] =
					{
						face[0],
						face[i],
						face[i + 1]};

				for (int j = 0; j < 3; j++)
				{
					int vi = tri[j].vi;
					int ti = tri[j].ti;
					int ni = tri[j].ni;

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
	}

	arqEntrada.close();

	std::cout << "Gerando o buffer de geometria..." << std::endl;
	GLuint VBO, VAO;
	// Construindo buffers (VBO e VAO)
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

	std::cout << "Vertices carregados: "
			  << nVertices
			  << std::endl;

	return VAO;
}

struct Material
{
	glm::vec3 Ka = glm::vec3(0.2f);
	glm::vec3 Kd = glm::vec3(1.0f);
	glm::vec3 Ks = glm::vec3(1.0f);
	float Ns = 32.0f;
	GLuint textureID = 0;
};

struct Light
{
	glm::vec3 position;
	glm::vec3 color;
};

struct OBJ
{
	GLuint VAO;
	GLuint VBO;

	int nVertices;
	Material material;

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

	if (!data)
	{
		cout << "Falha ao carregar textura!" << endl;
	}

	stbi_image_free(data);

	cout << "Largura: " << width
		 << " Altura: " << height
		 << " Canais: " << nrChannels
		 << endl;

	return textureID;
}

Material loadMaterialFromMTL(std::string mtlPath)
{
	std::ifstream file(mtlPath);

	Material mat;

	if (!file.is_open())
	{
		std::cout << "Erro ao abrir MTL\n";
		return mat;
	}

	fs::path baseDir = fs::path(mtlPath).parent_path();
	std::string line;

	while (std::getline(file, line))
	{
		std::istringstream ss(line);

		std::string word;
		ss >> word;

		if (word == "Ka")
		{
			ss >> mat.Ka.x >> mat.Ka.y >> mat.Ka.z;
		}
		else if (word == "Kd")
		{
			ss >> mat.Kd.x >> mat.Kd.y >> mat.Kd.z;
		}
		else if (word == "Ks")
		{
			ss >> mat.Ks.x >> mat.Ks.y >> mat.Ks.z;
		}
		else if (word == "Ns")
		{
			ss >> mat.Ns;
		}
		else if (word == "map_Kd")
		{
			std::string textureFile;
			ss >> textureFile;

			fs::path fullPath = baseDir / textureFile;

			std::cout << "Textura encontrada: " << fullPath << std::endl;

			mat.textureID = loadTexture(fullPath.string().c_str());
		}
	}

	return mat;
}

std::vector<OBJ> objects;
std::vector<Light> lights;

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
									 "uniform vec3 lightPos[3];\n"	   // Vetor que contém as 3 luzes
									 "uniform vec3 lightColor[3];\n"   // Cor de cada uma das luzes
									 "uniform bool lightEnabled[3];\n" // Define se cada luz está ligada ou não
									 "uniform vec3 viewPos;\n"
									 "uniform vec3 Ka;\n"  // Reflexão ambiente no material
									 "uniform vec3 Kd;\n"  // Reflexão difusa no material
									 "uniform vec3 Ks;\n"  // Reflexão especular no material
									 "uniform float Ns;\n" // Shininess (brilho)
									 "void main()\n"
									 "{\n"
									 "vec3 texColor = texture(texture1, TexCoord).rgb;\n"
									 "vec3 norm = normalize(Normal);\n"
									 "vec3 viewDir = normalize(viewPos - FragPos);\n"
									 "vec3 totalLight = vec3(0.0);\n"
									 "for(int i = 0; i < 3; i++) {\n"
									 "if(!lightEnabled[i]) continue;\n"
									 "vec3 lightDir = normalize(lightPos[i] - FragPos);\n"
									 "float diff = max(dot(norm, lightDir), 0.0);\n"									  // Intensidade da iluminação difusa
									 "float distance = length(lightPos[i] - FragPos);\n"								  // Distância entre a luz e o fragmento
									 "float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);\n" // Atenuação da luz
									 "vec3 ambient = Ka * lightColor[i];\n"
									 "vec3 diffuse = Kd * diff * lightColor[i] * attenuation;\n"
									 "vec3 reflectDir = reflect(-lightDir, norm);\n"			   // Direção em que a luz é refletida
									 "float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);\n" // Calcula Phong
									 "vec3 specular = Ks * spec * lightColor[i] * attenuation; totalLight += ambient + diffuse + specular;}\n"
									 "vec3 result = totalLight * texColor;\n"
									 "color = vec4(result, 1.0);\n"
									 "}\n\0";

void loadScene(const std::string &fileName)
{
	std::ifstream file(fileName);

	if (!file.is_open())
	{
		std::cout << "Erro ao abrir scene.txt\n";
		return;
	}

	std::string word;

	while (file >> word)
	{
		if (word == "#")
		{
			std::string lixo;
			std::getline(file, lixo);
		}

		else if (word == "CAMERA")
		{
			file >> word;
			file >>
				camera.position.x >>
				camera.position.y >>
				camera.position.z;

			file >> word;
			file >> camera.yaw;

			file >> word;
			file >> camera.pitch;

			float fov;

			file >> word;
			file >> fov;

			camera.updateVectors();
		}

		else if (word == "LIGHT")
		{
			Light light;

			file >> word;

			file >>
				light.position.x >>
				light.position.y >>
				light.position.z;

			file >> word;

			file >>
				light.color.r >>
				light.color.g >>
				light.color.b;

			lights.push_back(light);
		}

		else if (word == "OBJECT")
		{
			OBJ obj;

			std::string objFile;
			std::string mtlFile;

			file >> word;
			file >> objFile;

			file >> word;
			file >> mtlFile;

			file >> word;

			file >>
				obj.position.x >>
				obj.position.y >>
				obj.position.z;

			file >> word;

			file >>
				obj.scale.x >>
				obj.scale.y >>
				obj.scale.z;

			file >> word;

			file >>
				obj.rotationAxis.x >>
				obj.rotationAxis.y >>
				obj.rotationAxis.z;

			file >> word;

			float angleDeg;

			file >> angleDeg;

			obj.angle =
				glm::radians(angleDeg);

			file >> word;

			std::string rotating;

			file >> rotating;

			obj.rotating =
				(rotating == "true");

			obj.VAO =
				loadSimpleOBJ(
					objFile,
					obj.nVertices);

			obj.material =
				loadMaterialFromMTL(
					mtlFile);

			objects.push_back(obj);
		}
	}
	std::cout << "Quantidade de objetos: "
          << objects.size()
          << std::endl;
}

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

	// Carrega câmera, objetos e luzes
	loadScene("scene.txt");

	glUseProgram(shaderID);

	for (int i = 0;
		 i < lights.size() && i < 3;
		 i++)
	{
		std::string posName =
			"lightPos[" +
			std::to_string(i) +
			"]";

		std::string colorName =
			"lightColor[" +
			std::to_string(i) +
			"]";

		glUniform3fv(
			glGetUniformLocation(
				shaderID,
				posName.c_str()),
			1,
			glm::value_ptr(
				lights[i].position));

		glUniform3fv(
			glGetUniformLocation(
				shaderID,
				colorName.c_str()),
			1,
			glm::value_ptr(
				lights[i].color));
	}

	glUniform3fv(
		glGetUniformLocation(shaderID, "viewPos"),
		1,
		glm::value_ptr(camera.position));

	glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

	glm::mat4 model = glm::mat4(1); // matriz identidade;

	GLint modelLoc = glGetUniformLocation(shaderID, "model");

	GLint viewLoc =
		glGetUniformLocation(shaderID, "view");

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

		// Matriz da câmera
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

		// Imprimindo objetos 3D na cena, cada um com seus checkpoints, texturas e animações
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

			// Definindo posição, angulo e escala do objeto na cena
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, obj.position);
			model = glm::rotate(model, obj.angle, obj.rotationAxis);
			model = glm::scale(model, obj.scale);

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glBindVertexArray(obj.VAO);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, obj.material.textureID);

			glUniform3fv(glGetUniformLocation(shaderID, "Ka"), 1, glm::value_ptr(obj.material.Ka));
			glUniform3fv(glGetUniformLocation(shaderID, "Kd"), 1, glm::value_ptr(obj.material.Kd));
			glUniform3fv(glGetUniformLocation(shaderID, "Ks"), 1, glm::value_ptr(obj.material.Ks));
			glUniform1f(glGetUniformLocation(shaderID, "Ns"), obj.material.Ns);

			glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	for (auto &obj : objects)
	{
		glDeleteVertexArrays(1, &obj.VAO);
		glDeleteBuffers(1, &obj.VBO);
		glDeleteTextures(1, &obj.material.textureID);
	}
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	// Fecha janela e encerra aplicação
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (objects.empty())
		return;

	// Mostra o objeto 3D selecionado
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
		obj.scale *= 0.9f;

	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
		obj.scale *= 1.1f;

	if (obj.scale.x < 0.0001f ||
		obj.scale.y < 0.0001f ||
		obj.scale.z < 0.0001f)
	{
		obj.scale = glm::vec3(0.0001f);
	}

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

	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		selectedObject = 2;

	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
		selectedObject = 3;

	// Ativar rotação
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