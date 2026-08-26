/*
 * Lista de Exercícios 1 - Exercício 4
 *
 * Baseado no "Hello Triangle" de Rossana Baptista Queiroz:
 *   - https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Disciplina: Processamento Gráfico: Fundamentos (Unisinos)
 *
 * Descrição:
 *   Reprodução, com primitivas da OpenGL, de um desenho feito em papel
 *   quadriculado: uma casa com telhado, porta, janela, chão e sol.
 *
 *   Cada parte do desenho tem o SEU PRÓPRIO VAO e a SUA PRÓPRIA CHAMADA DE
 *   DESENHO, o que permite usar primitivas diferentes para cada elemento:
 *
 *     GL_TRIANGLE_FAN  -> áreas preenchidas (chão, sol, corpo, porta, janela)
 *     GL_TRIANGLES     -> o telhado, que é um triângulo simples
 *     GL_LINES         -> os caixilhos da janela (segmentos soltos)
 *     GL_LINE_LOOP     -> os contornos fechados do corpo, telhado e janela
 *
 *   O sistema de coordenadas é o normalizado (NDC), de -1 a 1 nos dois eixos.
 *   Como a janela é 800x600, medidas horizontais são multiplicadas por
 *   altura/largura sempre que precisam ser proporcionais (ver o sol).
 */

#include <iostream>
#include <string>
#include <cmath>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

const float PI = 3.14159265358979323846f;

// Correção da proporção da tela, para que o sol saia redondo e não oval
const float ASPECTO = (float)HEIGHT / (float)WIDTH;

// Cada parte do desenho: seu VAO, sua contagem de vértices, sua primitiva e sua cor
struct Parte
{
	GLuint VAO;
	GLsizei nVertices;
	GLenum primitiva;
	float r, g, b;
	const char *nome;
};

vector<Parte> partes;

// ---------------------------------------------------------------------------
// Geradores de geometria
// ---------------------------------------------------------------------------

// Retângulo definido por dois cantos opostos, em ordem anti-horária.
// Serve tanto para preencher (GL_TRIANGLE_FAN) quanto para contornar (GL_LINE_LOOP).
vector<GLfloat> retangulo(float x0, float y0, float x1, float y1)
{
	return {
		x0, y0, 0.0f,
		x1, y0, 0.0f,
		x1, y1, 0.0f,
		x0, y1, 0.0f};
}

// Círculo pela equação paramétrica, como leque de triângulos (igual ao exercício 2)
vector<GLfloat> circulo(float cx, float cy, float raio, int nPontos)
{
	vector<GLfloat> v;
	v.reserve((nPontos + 2) * 3);

	v.push_back(cx);
	v.push_back(cy);
	v.push_back(0.0f);

	for (int i = 0; i <= nPontos; i++)
	{
		float t = 2.0f * PI * ((float)i / (float)nPontos);
		v.push_back(cx + raio * cos(t) * ASPECTO);
		v.push_back(cy + raio * sin(t));
		v.push_back(0.0f);
	}

	return v;
}

// Cria um VBO + VAO para uma lista de vértices com apenas o atributo posição (x,y,z)
GLuint criarVAO(const vector<GLfloat> &vertices)
{
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Atributo 0 = posição: 3 floats por vértice, sem espaçamento extra
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

// Registra mais uma parte do desenho (um VAO + uma chamada de desenho)
void adicionarParte(const vector<GLfloat> &vertices, GLenum primitiva,
					float r, float g, float b, const char *nome)
{
	Parte p;
	p.VAO = criarVAO(vertices);
	p.nVertices = (GLsizei)(vertices.size() / 3); // 3 floats por vértice
	p.primitiva = primitiva;
	p.r = r;
	p.g = g;
	p.b = b;
	p.nome = nome;
	partes.push_back(p);
}

// Monta o desenho inteiro. A ordem importa: as partes são desenhadas na ordem
// em que são adicionadas, então o que vem depois aparece por cima.
void setupDesenho()
{
	// --- Cenário ---------------------------------------------------------
	// O céu é a própria cor de fundo (glClearColor), então não precisa de VAO.

	// Chão / grama, ocupando toda a largura da tela
	adicionarParte(retangulo(-1.0f, -1.0f, 1.0f, -0.55f),
				   GL_TRIANGLE_FAN, 0.35f, 0.70f, 0.30f, "chao");

	// Sol, no canto superior direito
	adicionarParte(circulo(0.72f, 0.75f, 0.16f, 40),
				   GL_TRIANGLE_FAN, 1.00f, 0.90f, 0.20f, "sol");

	// --- Casa ------------------------------------------------------------
	// Corpo da casa: um retângulo cinza claro
	adicionarParte(retangulo(-0.50f, -0.55f, 0.50f, 0.15f),
				   GL_TRIANGLE_FAN, 0.85f, 0.85f, 0.82f, "corpo");

	// Telhado: um único triângulo vermelho, mais largo que o corpo (beiral)
	adicionarParte({-0.60f, 0.15f, 0.0f,
					0.60f, 0.15f, 0.0f,
					0.00f, 0.70f, 0.0f},
				   GL_TRIANGLES, 0.80f, 0.15f, 0.15f, "telhado");

	// Porta, à direita do centro
	adicionarParte(retangulo(0.02f, -0.55f, 0.26f, -0.05f),
				   GL_TRIANGLE_FAN, 0.45f, 0.25f, 0.10f, "porta");

	// Maçaneta: um círculo bem pequeno
	adicionarParte(circulo(0.21f, -0.30f, 0.022f, 16),
				   GL_TRIANGLE_FAN, 0.20f, 0.15f, 0.05f, "macaneta");

	// Janela, à esquerda do centro
	adicionarParte(retangulo(-0.34f, -0.20f, -0.06f, 0.02f),
				   GL_TRIANGLE_FAN, 1.00f, 0.95f, 0.45f, "janela");

	// --- Detalhes em linha -----------------------------------------------
	// Caixilhos da janela: dois segmentos independentes (uma cruz), por isso
	// GL_LINES e não GL_LINE_STRIP - cada par de vértices é uma linha separada.
	adicionarParte({-0.20f, -0.20f, 0.0f, // vertical: de baixo
					-0.20f, 0.02f, 0.0f,  //           até em cima
					-0.34f, -0.09f, 0.0f, // horizontal: da esquerda
					-0.06f, -0.09f, 0.0f}, //            até a direita
				   GL_LINES, 0.30f, 0.20f, 0.10f, "caixilhos");

	// Contornos fechados. Reutilizam exatamente a mesma geometria das partes
	// preenchidas: o que muda é só a primitiva da chamada de desenho.
	adicionarParte(retangulo(-0.34f, -0.20f, -0.06f, 0.02f),
				   GL_LINE_LOOP, 0.30f, 0.20f, 0.10f, "contorno da janela");

	adicionarParte(retangulo(-0.50f, -0.55f, 0.50f, 0.15f),
				   GL_LINE_LOOP, 0.25f, 0.25f, 0.25f, "contorno do corpo");

	adicionarParte({-0.60f, 0.15f, 0.0f,
					0.60f, 0.15f, 0.0f,
					0.00f, 0.70f, 0.0f},
				   GL_LINE_LOOP, 0.25f, 0.25f, 0.25f, "contorno do telhado");
}

// ---------------------------------------------------------------------------
// Shaders (posição no vertex, cor via uniform - uma cor por parte do desenho)
// ---------------------------------------------------------------------------

const GLchar *vertexShaderSource = R"glsl(
 #version 400
 layout (location = 0) in vec3 position;
 void main()
 {
	 gl_Position = vec4(position.x, position.y, position.z, 1.0);
 }
 )glsl";

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

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Ativa a suavização de serrilhado (MSAA) com 8 amostras por pixel
	// glfwWindowHint(GLFW_SAMPLES, 8);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Exercicio 4 - Desenho com Primitivas", nullptr, nullptr);
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

	// Definindo as dimensões da viewport com as mesmas dimensões da janela
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Criando um VAO para cada parte do desenho
	setupDesenho();

	cout << "\n--- Exercicio 4: desenho com primitivas ---" << endl;
	cout << "  " << partes.size() << " VAOs / " << partes.size() << " chamadas de desenho:" << endl;
	for (size_t i = 0; i < partes.size(); i++)
		cout << "    - " << partes[i].nome << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

	glUseProgram(shaderID);

	double prev_s = glfwGetTime();
	double title_countdown_s = 0.1;

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
				sprintf(tmp, "Exercicio 4 - Desenho com Primitivas\tFPS %.2lf", fps);
				glfwSetWindowTitle(window, tmp);

				title_countdown_s = 0.1;
			}
		}

		glfwPollEvents();

		// Limpa o buffer de cor - o azul do fundo é o céu do desenho
		glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(3);

		// Uma chamada de desenho por parte, cada uma com o seu VAO e a sua primitiva
		for (size_t i = 0; i < partes.size(); i++)
		{
			Parte &p = partes[i];

			glBindVertexArray(p.VAO);
			glUniform4f(colorLoc, p.r, p.g, p.b, 1.0f);
			glDrawArrays(p.primitiva, 0, p.nVertices);
		}

		glBindVertexArray(0);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}

	// Pede pra OpenGL desalocar os buffers
	for (size_t i = 0; i < partes.size(); i++)
		glDeleteVertexArrays(1, &partes[i].VAO);

	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - é chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

// Compila e "builda" o programa de shader (vertex + fragment).
// A função retorna o identificador do programa de shader.
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
