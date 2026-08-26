/*
 * Lista de Exercícios 1 - Exercício 2
 *
 * Baseado no "Hello Triangle" de Rossana Baptista Queiroz:
 *   - https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Disciplina: Processamento Gráfico: Fundamentos (Unisinos)
 *
 * Descrição:
 *   Desenho de um círculo na tela utilizando a EQUAÇÃO PARAMÉTRICA do círculo
 *   para gerar os vértices:
 *
 *       x(t) = cx + raio * cos(t)
 *       y(t) = cy + raio * sen(t)         com t variando de 0 a 2*PI
 *
 *   Os vértices são organizados como um "leque de triângulos" (GL_TRIANGLE_FAN):
 *   o primeiro vértice é o centro e os demais percorrem o arco, de modo que cada
 *   par de pontos consecutivos forma um triângulo com o centro.
 *
 *   Variando apenas o NÚMERO DE PONTOS e o INTERVALO ANGULAR do mesmo gerador,
 *   obtemos todas as formas pedidas no exercício:
 *
 *     [1] Círculo             - muitos pontos, volta completa
 *     [2] a) Octógono         - 8 pontos, volta completa
 *     [3] b) Pentágono        - 5 pontos, volta completa
 *     [4] c) Pac-man          - volta incompleta: o arco que falta é a boca
 *     [5] d) Fatia de pizza   - um arco pequeno
 *     [6] e) DESAFIO 1: estrela - raios alternados (externo/interno)
 *     [7] f) DESAFIO 2: espiral - o raio cresce junto com o ângulo
 *
 *   A forma inicial também pode ser passada por linha de comando, ex.: ./2 6
 */

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
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

// A janela não é quadrada (800x600), mas o espaço normalizado (NDC) vai de -1 a 1
// nos dois eixos. Sem corrigir, um "círculo" apareceria achatado como uma elipse.
// Multiplicamos o deslocamento em x por altura/largura para compensar.
const float ASPECTO = (float)HEIGHT / (float)WIDTH;

// Converte graus para radianos (as funções cos/sin trabalham em radianos)
inline float rad(float graus)
{
	return graus * PI / 180.0f;
}

// Quantidade de formas e forma exibida no momento (0 a NUM_FORMAS-1).
// Precisa ser de escopo de arquivo: a key_callback é um callback da GLFW (C puro)
// e não recebe o estado da aplicação.
const int NUM_FORMAS = 7;
int formaAtual = 0;

// Cada forma guarda seu próprio VAO, sua contagem de vértices e a primitiva usada
// para desenhá-la - o que permite uma única chamada de desenho genérica no loop.
struct Forma
{
	GLuint VAO;
	GLsizei nVertices;
	GLenum primitiva;
	const char *nome;
	float r, g, b;
};

Forma formas[NUM_FORMAS];

// ---------------------------------------------------------------------------
// Geradores de geometria
// ---------------------------------------------------------------------------

// Gera os vértices de um arco de círculo como um leque de triângulos.
// v0 é o centro; em seguida vêm nPontos+1 pontos sobre o arco (o ponto final é
// repetido para fechar a forma quando o arco é uma volta completa).
// Retorna as coordenadas x, y, z já intercaladas, prontas para o VBO.
vector<GLfloat> gerarArco(float cx, float cy, float raio, int nPontos,
						  float angIni, float angFim)
{
	vector<GLfloat> vertices;
	vertices.reserve((nPontos + 2) * 3);

	// Primeiro vértice do leque: o centro
	vertices.push_back(cx);
	vertices.push_back(cy);
	vertices.push_back(0.0f);

	for (int i = 0; i <= nPontos; i++)
	{
		// Interpola o ângulo entre o início e o fim do arco
		float t = angIni + (angFim - angIni) * ((float)i / (float)nPontos);

		// Equação paramétrica do círculo
		vertices.push_back(cx + raio * cos(t) * ASPECTO);
		vertices.push_back(cy + raio * sin(t));
		vertices.push_back(0.0f);
	}

	return vertices;
}

// DESAFIO 1: a estrela é o mesmo leque de triângulos, mas alternando dois raios.
// Os pontos de índice par ficam no raio externo (as pontas) e os de índice ímpar
// no raio interno (os "vales" entre as pontas).
vector<GLfloat> gerarEstrela(float cx, float cy, float raioExterno,
							 float raioInterno, int nPontas)
{
	vector<GLfloat> vertices;
	int nPontos = nPontas * 2; // uma ponta e um vale para cada "bico"
	vertices.reserve((nPontos + 2) * 3);

	vertices.push_back(cx);
	vertices.push_back(cy);
	vertices.push_back(0.0f);

	for (int i = 0; i <= nPontos; i++)
	{
		float t = rad(90.0f) + 2.0f * PI * ((float)i / (float)nPontos);
		float raio = (i % 2 == 0) ? raioExterno : raioInterno;

		vertices.push_back(cx + raio * cos(t) * ASPECTO);
		vertices.push_back(cy + raio * sin(t));
		vertices.push_back(0.0f);
	}

	return vertices;
}

// DESAFIO 2: espiral de Arquimedes - o raio cresce linearmente com o ângulo:
//     r(t) = a * t
// Como é uma curva aberta (e não uma área preenchida), não há vértice central:
// os pontos são ligados em sequência com GL_LINE_STRIP.
vector<GLfloat> gerarEspiral(float cx, float cy, float raioMax, int nVoltas,
							 int pontosPorVolta)
{
	vector<GLfloat> vertices;
	int nPontos = nVoltas * pontosPorVolta;
	vertices.reserve((nPontos + 1) * 3);

	float tMax = nVoltas * 2.0f * PI;
	float a = raioMax / tMax; // fator que faz a espiral terminar em raioMax

	for (int i = 0; i <= nPontos; i++)
	{
		float t = tMax * ((float)i / (float)nPontos);
		float raio = a * t;

		vertices.push_back(cx + raio * cos(t) * ASPECTO);
		vertices.push_back(cy + raio * sin(t));
		vertices.push_back(0.0f);
	}

	return vertices;
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

	// Atributo 0 = posição: 3 floats, sem espaçamento extra entre os vértices
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

// Monta uma Forma a partir de uma lista de vértices já gerada
Forma criarForma(const vector<GLfloat> &vertices, GLenum primitiva,
				 const char *nome, float r, float g, float b)
{
	Forma f;
	f.VAO = criarVAO(vertices);
	f.nVertices = (GLsizei)(vertices.size() / 3); // 3 floats por vértice
	f.primitiva = primitiva;
	f.nome = nome;
	f.r = r;
	f.g = g;
	f.b = b;
	return f;
}

// Cria todas as formas do exercício
void setupFormas()
{
	// [1] Círculo: muitos pontos para a borda parecer lisa
	formas[0] = criarForma(gerarArco(0.0f, 0.0f, 0.7f, 100, 0.0f, 2.0f * PI),
						   GL_TRIANGLE_FAN, "[1] Circulo", 1.0f, 0.5f, 0.0f);

	// [2] a) Octógono: o mesmo círculo com apenas 8 pontos
	formas[1] = criarForma(gerarArco(0.0f, 0.0f, 0.7f, 8, 0.0f, 2.0f * PI),
						   GL_TRIANGLE_FAN, "[2] Octagono", 0.2f, 0.7f, 1.0f);

	// [3] b) Pentágono: 5 pontos, começando em 90 graus para a ponta ficar para cima
	formas[2] = criarForma(gerarArco(0.0f, 0.0f, 0.7f, 5, rad(90.0f), rad(90.0f) + 2.0f * PI),
						   GL_TRIANGLE_FAN, "[3] Pentagono", 0.4f, 1.0f, 0.4f);

	// [4] c) Pac-man: volta incompleta (35 a 325 graus). O arco que NÃO é desenhado,
	// entre 325 e 395 graus, é justamente a boca aberta.
	formas[3] = criarForma(gerarArco(0.0f, 0.0f, 0.7f, 64, rad(35.0f), rad(325.0f)),
						   GL_TRIANGLE_FAN, "[4] Pac-man", 1.0f, 1.0f, 0.0f);

	// [5] d) Fatia de pizza: um arco estreito, de 20 a 70 graus
	formas[4] = criarForma(gerarArco(0.0f, 0.0f, 0.8f, 32, rad(20.0f), rad(70.0f)),
						   GL_TRIANGLE_FAN, "[5] Fatia de pizza", 1.0f, 0.8f, 0.3f);

	// [6] e) DESAFIO 1: estrela de 5 pontas. A razão 0.382 entre os raios é a que
	// deixa os lados das pontas alinhados, como numa estrela desenhada à mão.
	formas[5] = criarForma(gerarEstrela(0.0f, 0.0f, 0.8f, 0.8f * 0.382f, 5),
						   GL_TRIANGLE_FAN, "[6] Estrela (DESAFIO 1)", 1.0f, 0.9f, 0.1f);

	// [7] f) DESAFIO 2: espiral de 4 voltas, desenhada como uma linha contínua
	formas[6] = criarForma(gerarEspiral(0.0f, 0.0f, 0.9f, 4, 100),
						   GL_LINE_STRIP, "[7] Espiral (DESAFIO 2)", 0.6f, 0.9f, 1.0f);
}

// ---------------------------------------------------------------------------
// Shaders (mesmos do Hello Triangle: posição no vertex, cor via uniform)
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
int main(int argc, char *argv[])
{
	// Forma inicial opcional via linha de comando (ex.: ./2 6 abre na estrela)
	if (argc > 1)
	{
		int f = atoi(argv[1]);
		if (f >= 1 && f <= NUM_FORMAS)
			formaAtual = f - 1;
	}

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
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Exercicio 2 - Circulo Parametrico", nullptr, nullptr);
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

	// Gerando um VAO para cada forma do exercício
	setupFormas();

	cout << "\n--- Exercicio 2: formas geradas a partir do circulo ---" << endl;
	for (int i = 0; i < NUM_FORMAS; i++)
		cout << "  " << formas[i].nome << endl;
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
				sprintf(tmp, "Exercicio 2 - %s\tFPS %.2lf", formas[formaAtual].nome, fps);
				glfwSetWindowTitle(window, tmp);

				title_countdown_s = 0.1;
			}
		}

		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(3);
		glPointSize(8);

		// Desenha apenas a forma selecionada. Como cada Forma carrega o seu VAO,
		// a sua primitiva e a sua contagem de vértices, a chamada é sempre a mesma.
		Forma &f = formas[formaAtual];

		glBindVertexArray(f.VAO);
		glUniform4f(colorLoc, f.r, f.g, f.b, 1.0f);
		glDrawArrays(f.primitiva, 0, f.nVertices);
		glBindVertexArray(0);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}

	// Pede pra OpenGL desalocar os buffers
	for (int i = 0; i < NUM_FORMAS; i++)
		glDeleteVertexArrays(1, &formas[i].VAO);

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

	// Teclas 1 a 7 trocam a forma exibida
	if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_7)
	{
		formaAtual = key - GLFW_KEY_1;
		cout << "Forma: " << formas[formaAtual].nome << endl;
	}
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
