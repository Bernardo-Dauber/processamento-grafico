/*
 * Lista de Exercícios 2 - Exercício 2
 *
 * Descrição:
 *   Modificar a janela do mundo (window/ortho) para os limites
 *   xmin=0, xmax=800, ymin=600, ymax=0.
 *
 *   Note a ordem estranha do eixo Y: ymin=600 e ymax=0. Isso inverte o eixo Y
 *   da projeção — o mundo agora tem origem (0,0) no CANTO SUPERIOR ESQUERDO
 *   da janela e o Y cresce PARA BAIXO (como o sistema de coordenadas da tela
 *   e do mouse). Cada unidade do mundo corresponde a 1 pixel da janela.
 *
 *   A matriz glm::ortho(left, right, bottom, top) mapeia:
 *     left   -> NDC x = -1   (borda esquerda da janela)
 *     right  -> NDC x = +1   (borda direita da janela)
 *     bottom -> NDC y = -1   (borda de baixo da janela)
 *     top    -> NDC y = +1   (borda de cima da janela)
 *
 *   Com bottom=600, top=0: o mundo y=600 cai na borda inferior (NDC -1) e o
 *   mundo y=0 na borda superior (NDC +1) -> eixo Y invertido.
 *
 *   A mesma cena da Lista 1 (gravata borboleta) é desenhada em coordenadas
 *   de pixel: v0..v5 posicionadas como se fossem coordenadas da tela.
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
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 2 - Ex2: ortho (0,800)x(600,0)", nullptr, nullptr);
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

	cout << "\n--- Lista 2, Ex2: janela do mundo 0..800 x 0..600 (Y invertido) ---" << endl;
	cout << "  A origem (0,0) fica no canto superior esquerdo; cada unidade = 1 pixel." << endl;
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
	// JANELA DO MUNDO (window/ortho): xmin=0, xmax=800, ymin=600, ymax=0
	// ----------------------------------------------------------------------
	// glm::ortho(left, right, bottom, top): com bottom=600 e top=0 o eixo Y é
	// invertido (Y cresce para baixo, origem no canto superior esquerdo).
	// Como o mundo tem exatamente as dimensões da janela (800x600), nenhuma
	// correção de proporção é necessária: 1 unidade = 1 pixel em ambos os eixos.
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
				sprintf(tmp, "Lista 2 - Ex2: ortho (0,800)x(600,0)\tFPS %.2lf", fps);
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

		glUniform4f(colorLoc, 0.0f, 0.8f, 1.0f, 1.0f); // ciano
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Contorno da janela do mundo (0..800 x 0..600): coincide com a própria
		// janela da aplicação, então quase não deve aparecer.
		glUniform4f(colorLoc, 0.3f, 0.3f, 0.3f, 1.0f); // cinza escuro
		glDrawArrays(GL_LINE_LOOP, 6, 4);

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
	// Mesma "gravata borboleta" da Lista 1, agora posicionada em coordenadas
	// de PIXEL dentro da janela do mundo 0..800 x 0..600 (Y cresce para baixo).
	// Por ex.: v0 = (350, 150) quer dizer "350 pixels para a direita,
	// 150 pixels para baixo" do canto superior esquerdo.
	GLfloat vertices[] = {
		// x       y      z
		// T0
		350.0f, 150.0f, 0.0f, // v0
		350.0f, 450.0f, 0.0f, // v1
		450.0f, 300.0f, 0.0f, // v2
		// T1
		450.0f, 300.0f, 0.0f, // v3
		450.0f, 150.0f, 0.0f, // v4
		350.0f, 300.0f, 0.0f, // v5
		// Contorno da janela do mundo (borda interna, 1 px de recuo p/ enxergar)
		1.0f, 1.0f, 0.0f,		 // v6
		799.0f, 1.0f, 0.0f,		 // v7
		799.0f, 599.0f, 0.0f,	 // v8
		1.0f, 599.0f, 0.0f		 // v9
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
	// atualmente vinculado - depois podemos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (boa prática)
	glBindVertexArray(0);

	return VAO;
}