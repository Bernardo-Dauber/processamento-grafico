/*
 * Lista de Exercícios 2 - Exercício 6
 *
 * Descrição:
 *   Criar triângulos a partir do clique do mouse, com os seguintes requisitos:
 *     - Ao clicar na tela, cria-se apenas 1 VÉRTICE
 *     - A cada 3 vértices criados, cria-se um TRIÂNGULO
 *     - Para cada novo triângulo criado, usa-se uma COR NOVA
 *
 *   Mapeamento: usa-se projeção paralela ortográfica com a janela do mundo
 *   com as mesmas dimensões da tela (800 x 600 unidades), para que cada
 *   unidade do mundo corresponda a 1 pixel. A posição do cursor
 *   (glfwGetCursorPos, que tem origem no CANTO SUPERIOR ESQUERDO da janela,
 *   com Y crescendo para baixo) é usada DIRETAMENTE como vértice do mundo —
 *   por isso a janela do mundo é ortho(0, 800, 600, 0): o Y do mundo também
 *   cresce para baixo.
 *
 *   Buffers: um VAO único com um VBO dinâmico (interleaved: posição XYZ +
 *   cor RGB por vértice). A cada clique o buffer é reenviado à GPU com
 *   glBufferData(..., GL_DYNAMIC_DRAW). Os triângulos completos são
 *   desenhados com GL_TRIANGLES; os vértices "pendentes" (1 ou 2 cliques
 *   ainda sem triângulo) aparecem como pontos.
 *
 *   Cores: cada triângulo recebe uma cor nova gerada pela rotação de matiz
 *   (golden angle ~137,5 graus), que garante cores bem distintas entre
 *   triângulos consecutivos.
 *
 * Histórico:
 *   - Versão inicial: 02/09/2026
 */

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
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

// Protótipos das funções de callback
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

// Protótipos das funções
int setupShader();

// Dimensões da janela (iguais às dimensões do mundo, para 1 unidade = 1 pixel)
const GLuint WIDTH = 800, HEIGHT = 600;

// Estado da aplicação - variáveis de escopo de arquivo, pois os callbacks da
// GLFW (C puro) não recebem o estado da aplicação.
GLuint VAO, VBO;					  // buffers da geometria dinâmica
std::vector<float> vertices;		  // vértices acumulados: x, y, z, r, g, b (interleaved)
bool bufferDesatualizado = true;	  // precisa reenviar o VBO para a GPU

// Código fonte do Vertex Shader (em GLSL): hardcoded
// Agora com DOIS atributos por vértice: posição e cor (exercício 6 usa cor
// por vértice, vinda do buffer, em vez do uniform inputColor dos demais).
const GLchar *vertexShaderSource = R"glsl(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec3 color;
 uniform mat4 uProjection;
 out vec3 vertexColor;
 void main()
 {
	 gl_Position = uProjection * vec4(position, 1.0);
	 vertexColor = color;
 }
 )glsl";

// Código fonte do Fragment Shader (em GLSL): hardcoded
const GLchar *fragmentShaderSource = R"glsl(
 #version 400
 in vec3 vertexColor;
 out vec4 color;
 void main()
 {
	 color = vec4(vertexColor, 1.0);
 }
 )glsl";

// Converte HSV (h em graus, s e v em 0..1) para RGB (0..1)
void hsvToRgb(float h, float s, float v, float &r, float &g, float &b)
{
	int i = (int)(h / 60.0f) % 6;
	float f = h / 60.0f - (int)(h / 60.0f);
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);
	switch (i)
	{
	case 0: r = v, g = t, b = p; break;
	case 1: r = q, g = v, b = p; break;
	case 2: r = p, g = v, b = t; break;
	case 3: r = p, g = q, b = v; break;
	case 4: r = t, g = p, b = v; break;
	case 5: r = v, g = p, b = q; break;
	}
}

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 2 - Ex6: triangulos com o mouse", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Fazendo o registro das funções de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

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

	cout << "\n--- Lista 2, Ex6: triangulos com o mouse ---" << endl;
	cout << "  Clique com o botao esquerdo para criar 1 vertice." << endl;
	cout << "  A cada 3 vertices um triangulo e criado, com uma cor nova." << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
	glViewport(0, 0, framebufferWidth, framebufferHeight);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Localizações das variáveis uniform
	GLint projLoc = glGetUniformLocation(shaderID, "uProjection");

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	// ----------------------------------------------------------------------
	// CAMERA 2D: janela do mundo com as mesmas dimensões da tela (800 x 600).
	// Assim cada unidade do mundo corresponde a 1 pixel, e a posição do mouse
	// (origem no canto superior esquerdo, Y para baixo) vira o vértice.
	// ----------------------------------------------------------------------
	glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

	// ----------------------------------------------------------------------
	// VAO único + VBO dinâmico (interleaved: posição + cor).
	// O VBO começa vazio; cresce a cada clique. Como os ponteiros de atributo
	// são definidos uma única vez aqui, reenviar os dados com glBufferData
	// mantém a configuração do VAO intacta.
	// ----------------------------------------------------------------------
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	// Atributo 0: posição (x, y, z) - 3 floats, no início de cada vértice
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	// Atributo 1: cor (r, g, b) - 3 floats, logo após a posição
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

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
				sprintf(tmp, "Lista 2 - Ex6: %d vertices (%d triangulos)\tFPS %.2lf",
						(int)(vertices.size() / 6), (int)(vertices.size() / 6 / 3), fps);
				glfwSetWindowTitle(window, tmp);
				title_countdown_s = 0.1;
			}
		}

		// Checa se houveram eventos de input (aqui o clique do mouse é
		// processado nas funções de callback correspondentes)
		glfwPollEvents();

		// Se novos vértices foram adicionados pelo callback do mouse, reenvia
		// o buffer completo para a GPU
		if (bufferDesatualizado)
		{
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			bufferDesatualizado = false;
		}

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO); // Conectando ao buffer de geometria

		// Quantidade de vértices pertencentes a triângulos COMPLETOS
		// (múltiplo de 3). O resto são vértices "pendentes" do triângulo em
		// construção, desenhados como pontos.
		size_t nVerts = vertices.size() / 6;
		size_t nCompletos = (nVerts / 3) * 3;

		if (nCompletos > 0)
		{
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)nCompletos);
		}

		if (nVerts > nCompletos)
		{
			glPointSize(10.0f);
			glDrawArrays(GL_POINTS, (GLsizei)nCompletos, (GLsizei)(nVerts - nCompletos));
		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
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

	// Tecla C limpa todos os vértices/triângulos
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		vertices.clear();
		bufferDesatualizado = true;
		cout << "Cena limpa!" << endl;
	}
}

// Função de callback de mouse - chamada quando um botão do mouse é pressionado
// ou solto. Aqui, a cada clique ESQUERDO criamos 1 novo vértice.
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// Posição do cursor em coordenadas da janela (origem no canto
		// SUPERIOR ESQUERDO do conteúdo da janela, Y crescendo para baixo)
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Qual triângulo este vértice pertence (0-based)? A cada 3 vértices,
		// um novo triângulo começa -> uma cor nova.
		int triIdx = (int)(vertices.size() / 6) / 3;

		// Cor nova para o triângulo: rotação de matiz (golden angle), s e v
		// fixos. Todos os 3 vértices do mesmo triângulo recebem a mesma cor.
		float r, g, b;
		hsvToRgb(fmodf(triIdx * 137.507764f, 360.0f), 0.75f, 1.0f, r, g, b);

		// Acrescenta o vértice (posição + cor) ao vetor interleaved
		vertices.push_back((float)xpos);
		vertices.push_back((float)ypos);
		vertices.push_back(0.0f);
		vertices.push_back(r);
		vertices.push_back(g);
		vertices.push_back(b);

		bufferDesatualizado = true;

		cout << "  Vertice (" << (int)xpos << ", " << (int)ypos
			 << ") - " << (int)(vertices.size() / 6) << " vertice(s)"
			 << ", " << (int)(vertices.size() / 6 / 3) << " triangulo(s)" << endl;
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