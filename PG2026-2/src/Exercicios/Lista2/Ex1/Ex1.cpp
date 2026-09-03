/*
 * Lista de Exercícios 2 - Exercício 1
 *
 * Descrição:
 *   Modificar a janela do mundo (window/ortho) para os limites
 *   xmin=-10, xmax=10, ymin=-10, ymax=10.
 *
 *   A "janela do mundo" é o retângulo do espaço 2D que a câmera ortográfica
 *   enxerga. Ele é definido pela matriz de projeção ortográfica
 *   glm::ortho(...), aplicada a todos os vértices no vertex shader:
 *
 *     world (window/ortho)      projection        NDC           viewport (pixels)
 *        -10..10           --------------->  -1..1   ------>      800 x 600
 *
 *   A janela do mundo é exatamente a pedida no enunciado: x e y em -10..10.
 *   Como a janela da aplicação é 800x600 (4:3) e a janela do mundo é
 *   quadrada, a geometria é pré-escalada em x pelo fator altura/largura
 *   (0,75) — o mesmo truque usado no exercício 2 da Lista 1 — para que 1
 *   unidade ocupe o mesmo número de pixels nos dois eixos. Sem isso, as
 *   figuras sairiam achatadas na horizontal (mais largas que altas).
 *
 *   A cena desenhada é a "gravata borboleta" (2 triângulos) do exercício 1
 *   da Lista 1, agora posicionada dentro da janela do mundo -10..10.
 *
 * Histórico:
 *   - Versão inicial: 02/09/2026
 */

#include <iostream>
#include <cstdlib>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM: matemática de vetores e matrizes (aqui, para glm::ortho)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela (podem ser alteradas em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): hardcoded
const GLchar *vertexShaderSource = R"glsl(
 #version 400
 layout (location = 0) in vec3 position;
 uniform mat4 uProjection;
 void main()
 {
	 gl_Position = uProjection * vec4(position, 1.0);
 }
 )glsl";

// Código fonte do Fragment Shader (em GLSL): hardcoded
const GLchar *fragmentShaderSource = R"glsl(
 #version 400
 uniform vec4 inputColor;
 out vec4 color;
 void main()
 {
	 color = inputColor;
 }
 )glsl";

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 2 - Ex1: ortho (-10,10)x(-10,10)", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros de funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	cout << "\n--- Lista 2, Ex1: janela do mundo -10..10 ---" << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer simples, com a geometria da cena
	GLuint VAO = setupGeometry();

	// Localização das variáveis uniform
	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
	GLint projLoc = glGetUniformLocation(shaderID, "uProjection");

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	// ----------------------------------------------------------------------
	// JANELA DO MUNDO (window/ortho): xmin=-10, xmax=10, ymin=-10, ymax=10
	// (exatamente como no enunciado)
	// ----------------------------------------------------------------------
	// A correção de proporção da janela 800x600 (4:3) é feita NA GEOMETRIA:
	// as coordenadas x dos vértices são multiplicadas por altura/largura
	// (0,75) em setupGeometry(), como no exercício 2 da Lista 1. Assim a
	// janela do mundo permanece exatamente -10..10 em ambos os eixos.
	glm::mat4 proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

	double prev_s = glfwGetTime();	 // Define o "tempo anterior" inicial.
	double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Calcula e mostra a contagem do FPS na barra de título
		{
			double curr_s = glfwGetTime();
			double elapsed_s = curr_s - prev_s;
			prev_s = curr_s;

			title_countdown_s -= elapsed_s;
			if (title_countdown_s <= 0.0 && elapsed_s > 0.0)
			{
				double fps = 1.0 / elapsed_s;
				char tmp[256];
				sprintf(tmp, "Lista 2 - Ex1: ortho (-10,10)x(-10,10)\tFPS %.2lf", fps);
				glfwSetWindowTitle(window, tmp);
				title_countdown_s = 0.1;
			}
		}

		// Checa se houveram eventos de input (key pressed, mouse moved etc.)
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO); // Conectando ao buffer de geometria

		glUniform4f(colorLoc, 0.0f, 0.8f, 1.0f, 1.0f); // ciano: borboleta preenchida
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

// Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
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

// Esta função está bastante hardcoded - objetivo é criar os buffers que armazenam a
// geometria da cena
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
int setupGeometry()
{
	// "Gravata borboleta": 2 triângulos que compartilham o vértice central (0,0),
	// dentro da janela do mundo -10..10 (portanto 4 a 5 unidades de lado).
	//
	// Correção de proporção: a janela da aplicação é 800x600 (4:3) e a janela
	// do mundo é quadrada. Para que 1 unidade ocupe o mesmo número de pixels
	// nos dois eixos, as coordenadas x são multiplicadas por altura/largura
	// (600/800 = 0,75) — o mesmo truque do exercício 2 da Lista 1. Sem isso,
	// a borboleta sairia mais larga que alta.
	const float ESCALA_X = (float)HEIGHT / (float)WIDTH; // 0,75

	GLfloat vertices[] = {
		// x                y    z
		// T0
		-5.0f * ESCALA_X, 5.0f, 0.0f, // v0
		-5.0f * ESCALA_X, -5.0f, 0.0f, // v1
		0.0f, 0.0f, 0.0f,			  // v2
		// T1
		0.0f, 0.0f, 0.0f,			   // v3
		5.0f * ESCALA_X, -5.0f, 0.0f, // v4
		5.0f * ESCALA_X, 5.0f, 0.0f	   // v5
	};

	GLuint VBO, VAO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGL
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Observe que isso é permitido: glVertexAttribPointer registrou o VBO como o objeto
	// de buffer de vértice atualmente vinculado - depois podemos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (boa prática desvincular qualquer buffer ou array)
	glBindVertexArray(0);

	return VAO;
}