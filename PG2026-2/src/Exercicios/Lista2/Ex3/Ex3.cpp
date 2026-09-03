/*
 * Lista de Exercícios 2 - Exercício 3
 *
 * Descrição:
 *   Utilizando a câmera 2D do exercício anterior (janela do mundo 0..800 x
 *   600..0, eixo Y invertido), desenhar algo na tela e observar o que
 *   acontece quando posicionamos os objetos.
 *
 *   O que acontece quando posicionamos os objetos?  Nesta configuração, a
 *   posição de cada objeto é dada DIRETAMENTE em pixels: (0,0) é o canto
 *   superior esquerdo da janela, X cresce para a direita e Y para baixo.
 *   Posicionar um objeto é dizer "em que pixel ele deve aparecer" — não há
 *   transformação, escala nem conversão para resolver.
 *
 *   Por que é útil essa configuração?  É o padrão de jogos 2D, UIs e
 *   plotters: (1) 1 unidade do mundo = 1 pixel, então o layout fica
 *   previsível e idêntico ao que se vê; (2) as coordenadas do mouse
 *   (glfwGetCursorPos) já vêm nesse sistema (Y para baixo), então mapear
 *   clique -> posição no mundo é a identidade; (3) dispensa conversões de
 *   sistema de coordenadas e facilita sobrepor elementos da interface.
 *
 *   (Respostas completas também no README da Lista 2.)
 *
 *   A cena desenhada é a mesma usada nos exercícios 4 e 5: um triângulo
 *   preenchido posicionado em coordenadas de pixel, com contorno.
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

// GLM
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
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 2 - Ex3: camera 2D (coords de pixel)", nullptr, nullptr);
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

	cout << "\n--- Lista 2, Ex3: desenhando com a camera 2D ---" << endl;
	cout << "  A cena e posicionada em coordenadas de pixel (1 unidade = 1 px)." << endl;
	cout << "  O vertice do triangulo esta em (400,150); a base em y=400." << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
	glViewport(0, 0, framebufferWidth, framebufferHeight);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer simples, com a geometria da cena
	GLuint VAO = setupGeometry();

	// Localização das variáveis uniform
	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
	GLint projLoc = glGetUniformLocation(shaderID, "uProjection");

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	// ----------------------------------------------------------------------
	// CAMERA 2D do exercício anterior: xmin=0, xmax=800, ymin=600, ymax=0
	// ----------------------------------------------------------------------
	glm::mat4 proj = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f);
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
				sprintf(tmp, "Lista 2 - Ex3: camera 2D (coords de pixel)\tFPS %.2lf", fps);
				glfwSetWindowTitle(window, tmp);
				title_countdown_s = 0.1;
			}
		}

		// Checa se houveram eventos de input
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO); // Conectando ao buffer de geometria

		// Triângulo preenchido
		glUniform4f(colorLoc, 0.2f, 0.8f, 0.3f, 1.0f); // verde
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// Contorno do triângulo
		glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f); // branco
		glLineWidth(3);
		glDrawArrays(GL_LINE_LOOP, 0, 3);

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

// Compila e "builda" um programa de shader simples e único
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

// Cria os buffers que armazenam a geometria da cena
int setupGeometry()
{
	// Um triângulo em coordenadas de PIXEL:
	//   vertice em (400, 150), base em y = 400 (entre x = 300 e x = 500).
	// Como Y cresce para baixo, o vertice em y=150 fica ACIMA da base (y=400).
	// Essa é a mesma cena usada nos exercícios 4 e 5.
	GLfloat vertices[] = {
		// x       y      z
		300.0f, 400.0f, 0.0f, // v0 - base, esquerda
		500.0f, 400.0f, 0.0f, // v1 - base, direita
		400.0f, 150.0f, 0.0f	// v2 - vértice (topo)
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
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (boa prática)
	glBindVertexArray(0);

	return VAO;
}