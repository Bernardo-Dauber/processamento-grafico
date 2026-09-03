/*
 * Lista de Exercícios 2 - Exercício 4
 *
 * Descrição:
 *   Modificar o VIEWPORT para desenhar a cena apenas no seguinte quadrante
 *   da janela da aplicação. O viewport é o retângulo da janela (em pixels)
 *   onde o framebuffer é desenhado; ele é definido por glViewport(x, y, w, h),
 *   onde (x, y) é o CANTO INFERIOR ESQUERDO do retângulo.
 *
 *   A cena (o mesmo triângulo do exercício 3) é desenhada dentro de apenas
 *   um quadrante; os demais ficam com a cor de fundo. As teclas 1 a 4 trocam
 *   o quadrante em tempo real:
 *
 *     [1] superior esquerdo  -> glViewport(0,      h/2, w/2, h/2)
 *     [2] superior direito   -> glViewport(w/2,    h/2, w/2, h/2)
 *     [3] inferior esquerdo  -> glViewport(0,      0,   w/2, h/2)
 *     [4] inferior direito   -> glViewport(w/2,    0,   w/2, h/2)
 *
 *   O padrão inicial é o quadrante superior direito (item [2]), conforme a
 *   figura do enunciado. Linhas guia discretas mostram a divisão da janela.
 *
 *   IMPORTANTE: o glViewport usa o tamanho do FRAMEBUFFER
 *   (glfwGetFramebufferSize), que em telas de alta densidade (HiDPI/retina)
 *   é maior que o tamanho da janela em pontos (glfwGetWindowSize).
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

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Quadrante atual (variável de escopo de arquivo: a key_callback é uma função
// de callback da GLFW em C puro e não recebe o estado da aplicação).
// Padrão: [2] superior direito, conforme a figura do enunciado.
int quadrante = 2;

// Nomes dos quadrantes, para exibir no título da janela / no terminal
const char *NOMES_QUADRANTES[] = {
	"",
	"[1] superior esquerdo",
	"[2] superior direito",
	"[3] inferior esquerdo",
	"[4] inferior direito"};

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
int main(int argc, char *argv[])
{
	// Quadrante inicial opcional via linha de comando (ex.: ./Ex4 3)
	if (argc > 1)
	{
		int q = atoi(argv[1]);
		if (q >= 1 && q <= 4)
			quadrante = q;
	}

	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 2 - Ex4: viewport em um quadrante", nullptr, nullptr);
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

	cout << "\n--- Lista 2, Ex4: viewport em um quadrante ---" << endl;
	for (int i = 1; i <= 4; i++)
		cout << "  " << NOMES_QUADRANTES[i] << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer simples, com a geometria da cena
	GLuint VAO = setupGeometry();

	// Localização das variáveis uniform
	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
	GLint projLoc = glGetUniformLocation(shaderID, "uProjection");

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	// Camera 2D do exercício 2: janela do mundo 0..800 x 600..0
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
				sprintf(tmp, "Lista 2 - Ex4: %s\tFPS %.2lf", NOMES_QUADRANTES[quadrante], fps);
				glfwSetWindowTitle(window, tmp);
				title_countdown_s = 0.1;
			}
		}

		// Checa se houveram eventos de input
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		// Tamanho do framebuffer (>= tamanho da janela em telas HiDPI)
		int fbW, fbH;
		glfwGetFramebufferSize(window, &fbW, &fbH);
		int vw = fbW / 2, vh = fbH / 2;

		// ------------------------------------------------------------------
		// VIEWPORT: define em que retângulo da janela (em pixels) a cena é
		// desenhada. A origem (x, y) é o CANTO INFERIOR ESQUERDO do retângulo.
		// ------------------------------------------------------------------
		switch (quadrante)
		{
		case 1: // superior esquerdo: canto superior esquerdo da janela
			glViewport(0, vh, vw, vh);
			break;
		case 2: // superior direito: canto superior direito da janela
			glViewport(vw, vh, vw, vh);
			break;
		case 3: // inferior esquerdo: canto inferior esquerdo da janela
			glViewport(0, 0, vw, vh);
			break;
		case 4: // inferior direito: canto inferior direito da janela
			glViewport(vw, 0, vw, vh);
			break;
		}

		glBindVertexArray(VAO); // Conectando ao buffer de geometria

		// A cena: triângulo preenchido + contorno
		glUniform4f(colorLoc, 0.2f, 0.8f, 0.3f, 1.0f); // verde
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f); // branco
		glLineWidth(3);
		glDrawArrays(GL_LINE_LOOP, 0, 3);

		// Linhas guia discretas mostrando a divisão da janela em quadrantes
		// (desenhadas sobre toda a janela, com o viewport voltando ao tamanho
		// completo - não são parte da cena).
		glViewport(0, 0, fbW, fbH);
		glUniform4f(colorLoc, 0.15f, 0.15f, 0.15f, 1.0f); // cinza bem escuro
		glLineWidth(1);
		glDrawArrays(GL_LINES, 3, 4);

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

	// Teclas 1 a 4 trocam o quadrante
	if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_4)
	{
		quadrante = key - GLFW_KEY_1 + 1;
		cout << "Quadrante: " << NOMES_QUADRANTES[quadrante] << endl;
	}
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
// Índices:
//   0..2  -> triângulo da cena
//   3..6  -> linhas guia (divisão da janela em quadrantes)
int setupGeometry()
{
	// A cena: o mesmo triângulo do exercício 3, em coordenadas de pixel
	// (vértice em (400,150), base em y=400).
	GLfloat vertices[] = {
		// cena
		300.0f, 400.0f, 0.0f, // v0 - base, esquerda
		500.0f, 400.0f, 0.0f, // v1 - base, direita
		400.0f, 150.0f, 0.0f, // v2 - vértice (topo)
		// linhas guia (GL_LINES: cada par de vértices é um segmento):
		// segmento horizontal (base do ponto médio esquerdo ao direito)
		0.0f, 300.0f, 0.0f,	 // v3
		800.0f, 300.0f, 0.0f, // v4
		// segmento vertical (base do ponto médio superior ao inferior)
		400.0f, 0.0f, 0.0f,	 // v5
		400.0f, 600.0f, 0.0f	// v6
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