/*
 * Lista de Exercícios 1 - Exercício 3
 *
 * Baseado no "Hello Triangle" de Rossana Baptista Queiroz:
 *   - https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Disciplina: Processamento Gráfico: Fundamentos (Unisinos)
 *
 * Descrição:
 *   Triângulo formado pelos vértices P1, P2 e P3, respectivamente nas cores
 *   VERMELHO, VERDE e AZUL. Diferente dos exercícios 1 e 2, a cor aqui não vem
 *   de uma variável uniform: ela é um ATRIBUTO DE VÉRTICE, armazenado no buffer
 *   junto com a posição.
 *
 * ---------------------------------------------------------------------------
 * a) Uma possível configuração dos buffers (VBO, VAO)
 * ---------------------------------------------------------------------------
 *
 *   Usamos UM ÚNICO VBO com os atributos INTERCALADOS (interleaved): para cada
 *   vértice, guardamos primeiro as 3 coordenadas e logo em seguida os 3 canais
 *   de cor, totalizando 6 floats (24 bytes) por vértice:
 *
 *       |   P1                  |   P2                  |   P3                  |
 *       | x  y  z | r  g  b     | x  y  z | r  g  b     | x  y  z | r  g  b     |
 *       | 0  1  2 | 3  4  5     | 6  7  8 | 9 10 11     |12 13 14 |15 16 17     |
 *       ^         ^
 *       offset 0  offset 12 bytes (3 * sizeof(GLfloat))
 *
 *       stride (distância entre dois vértices) = 6 * sizeof(GLfloat) = 24 bytes
 *
 *   O VAO guarda a "receita" de leitura desse VBO, com dois ponteiros de atributo:
 *
 *       glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)0);
 *       glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
 *
 *   Uma configuração alternativa igualmente válida seria usar DOIS VBOs
 *   separados - um só com as posições e outro só com as cores. Nesse caso o
 *   stride de cada um seria 3*sizeof(GLfloat) e o offset de ambos seria 0; o VAO
 *   continuaria com dois ponteiros de atributo, cada um lendo de um VBO diferente.
 *   A versão intercalada costuma ser preferida por localidade de memória: os
 *   dados de um mesmo vértice ficam juntos, e a GPU os lê de uma vez só.
 *
 * ---------------------------------------------------------------------------
 * b) Como estes atributos seriam identificados no vertex shader
 * ---------------------------------------------------------------------------
 *
 *   Pelo qualificador "layout (location = N)", onde N é exatamente o mesmo
 *   número passado como primeiro argumento de glVertexAttribPointer e de
 *   glEnableVertexAttribArray:
 *
 *       layout (location = 0) in vec3 position;   <-> atributo 0 (posição)
 *       layout (location = 1) in vec3 color;      <-> atributo 1 (cor)
 *
 *   É esse número que amarra o dado do buffer à variável de entrada do shader.
 *   O vertex shader repassa a cor para o fragment shader através de uma variável
 *   de saída (out vec3 vertexColor), recebida lá como entrada (in vec3 vertexColor).
 *   Entre uma etapa e outra o rasterizador INTERPOLA o valor ao longo da face do
 *   triângulo - por isso o resultado é um degradê entre vermelho, verde e azul,
 *   e não três regiões de cor chapada.
 */

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader: recebe DOIS atributos de vértice (posição e cor) e repassa a
// cor adiante, para que o rasterizador a interpole ao longo do triângulo.
const GLchar *vertexShaderSource = R"glsl(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec3 color;
 out vec3 vertexColor;
 void main()
 {
	 gl_Position = vec4(position.x, position.y, position.z, 1.0);
	 vertexColor = color;
 }
 )glsl";

// Fragment Shader: recebe a cor já interpolada para este fragmento.
// Note que aqui NÃO existe mais a uniform inputColor dos exercícios 1 e 2.
const GLchar *fragmentShaderSource = R"glsl(
 #version 400
 in vec3 vertexColor;
 out vec4 color;
 void main()
 {
	 color = vec4(vertexColor, 1.0);
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
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Exercicio 3 - Cor por Vertice", nullptr, nullptr);
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

	cout << "\n--- Exercicio 3: triangulo com cor por vertice ---" << endl;
	cout << "  P1 (topo)          -> vermelho" << endl;
	cout << "  P2 (base esquerda) -> verde" << endl;
	cout << "  P3 (base direita)  -> azul" << endl;
	cout << "  As cores sao interpoladas pelo rasterizador." << endl;
	cout << "  [ESC] Sair\n"
		 << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando o buffer com a geometria e as cores do triângulo
	GLuint VAO = setupGeometry();

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
				sprintf(tmp, "Exercicio 3 - Cor por Vertice\tFPS %.2lf", fps);
				glfwSetWindowTitle(window, tmp);

				title_countdown_s = 0.1;
			}
		}

		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO);

		// Uma única chamada de desenho: as cores vêm do próprio buffer,
		// não é preciso enviar nenhuma uniform antes de desenhar.
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}

	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
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

// Cria o VBO e o VAO do triângulo, com DOIS atributos por vértice.
// Ver a explicação detalhada da configuração no cabeçalho deste arquivo (item a).
// A função retorna o identificador do VAO.
int setupGeometry()
{
	// Um único VBO com os atributos intercalados: 6 floats por vértice.
	// As coordenadas foram estimadas a partir da figura do enunciado, em NDC.
	GLfloat vertices[] = {
		// x      y     z        r     g     b
		 0.0f,  0.8f, 0.0f,   1.0f, 0.0f, 0.0f, // P1 - topo,          vermelho
		-0.8f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, // P2 - base esquerda, verde
		 0.9f, -0.6f, 0.0f,   0.0f, 0.0f, 1.0f  // P3 - base direita,  azul
	};

	GLuint VBO, VAO;

	// Geração do identificador do VBO e envio dos dados para a GPU
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Atributo 0 - posição: 3 floats, começando no byte 0 de cada vértice.
	// O stride (6 floats) faz a OpenGL "pular" a cor para chegar no próximo vértice.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Atributo 1 - cor: 3 floats, começando após as 3 coordenadas (offset de 12 bytes)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Desvincula o buffer e o VAO (boa prática para evitar bugs medonhos)
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}
