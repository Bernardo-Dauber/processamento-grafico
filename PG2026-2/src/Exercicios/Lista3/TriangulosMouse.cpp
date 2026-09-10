/*
 * Lista de Exercícios 3 - Processamento Gráfico: Fundamentos (Unisinos)
 * "Criando Triângulos a partir do Clique do Mouse"
 *
 * Descrição:
 *   Revisão de buffers de geometria e envio de dados para o programa de
 *   shader. Requisitos do enunciado:
 *     (1) Cada clique com o mouse cria UM ÚNICO vértice;
 *     (2) A cada 3 vértices criados forma-se UM TRIÂNGULO;
 *     (3) Cada novo triângulo recebe uma COR NOVA.
 *
 *   Mapeamento (dica do enunciado):
 *     Usa-se projeção paralela ortográfica com a JANELA DO MUNDO com as
 *     mesmas dimensões da tela (800 x 600 unidades), de modo que 1 unidade
 *     do mundo = 1 pixel. Assim, a posição do cursor devolvida por
 *     glfwGetCursorPos (origem no canto SUPERIOR ESQUERDO da janela, com Y
 *     crescendo para baixo) é usada DIRETAMENTE como coordenada de vértice.
 *     Para isso a projeção é glm::ortho(0, 800, 600, 0): o Y do mundo também
 *     cresce para baixo, espelhando o sistema de coordenadas da tela.
 *
 *   Buffers:
 *     Um VAO único + um VBO dinâmico intercalado (posição xyz + cor rgb por
 *     vértice). A cada clique o buffer completo é reenviado à GPU com
 *     glBufferData(..., GL_DYNAMIC_DRAW). Os triângulos completos são
 *     desenhados com uma única chamada glDrawArrays(GL_TRIANGLES, 0, 3*n);
 *     os vértices "pendentes" (1 ou 2 cliques que ainda não fecharam um
 *     triângulo) aparecem como pontos, para dar retorno visual ao usuário.
 *
 *   Cores:
 *     Cada triângulo recebe uma cor nova por rotação de matiz pelo ângulo
 *     áureo (~137,5°), que distribui as matizes de modo que triângulos
 *     consecutivos fiquem bem distintos. Os 3 vértices de um triângulo
 *     compartilham a mesma cor.
 *
 * Controles:
 *   - Clique com o botão ESQUERDO do mouse: adiciona 1 vértice.
 *   - Tecla C: limpa todos os vértices/triângulos.
 *   - Tecla ESC: fecha a janela.
 *
 * Histórico:
 *   - Versão inicial (Lista 3): 2026
 */

#include <iostream>
#include <vector>
#include <cstdio>
#include <cmath>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// ---------------------------------------------------------------------------
// Protótipos (declarados antes do uso)
// ---------------------------------------------------------------------------
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
int setupShader();
void appendVertex(double xpos, double ypos);
void uploadGeometry();
void hsvToRgb(float h, float s, float v, float &r, float &g, float &b);

// ---------------------------------------------------------------------------
// Constantes
// ---------------------------------------------------------------------------

// Dimensões da janela. São usadas TAMBÉM como dimensões da janela do mundo
// (glm::ortho), garantindo 1 unidade do mundo = 1 pixel.
const GLuint WIDTH = 800, HEIGHT = 600;

// Quantidade de floats por vértice no buffer intercalado: x, y, z, r, g, b.
const GLuint FLOATS_PER_VERTEX = 6;

// ---------------------------------------------------------------------------
// Estado da aplicação
//
// Os callbacks da GLFW são funções de C puro e não recebem o estado da
// aplicação, por isso o estado compartilhado fica no escopo de arquivo.
// ---------------------------------------------------------------------------
GLuint VAO = 0, VBO = 0;              // buffers da geometria dinâmica
vector<float> vertices;               // x, y, z, r, g, b por vértice (interleaved)
bool geometryDirty = false;           // há vértices novos para reenviar à GPU

// ---------------------------------------------------------------------------
// Código-fonte dos shaders (GLSL), embutido como raw string
// ---------------------------------------------------------------------------

// Vertex shader: dois atributos de vértice — posição e cor — e a matriz de
// projeção ortográfica como uniform.
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

// Fragment shader: apenas repassa a cor interpolada do vértice.
const GLchar *fragmentShaderSource = R"glsl(
 #version 400
 in vec3 vertexColor;
 out vec4 color;
 void main()
 {
     color = vec4(vertexColor, 1.0);
 }
 )glsl";

// ---------------------------------------------------------------------------
// Função MAIN
// ---------------------------------------------------------------------------
int main()
{
    // Inicialização da GLFW
    if (!glfwInit())
    {
        cerr << "Falha ao inicializar a GLFW" << endl;
        return -1;
    }

    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT,
                                         "Lista 3: triangulos com o clique do mouse",
                                         nullptr, nullptr);
    if (!window)
    {
        cerr << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Registro das funções de callback da janela
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // GLAD: carrega os ponteiros das funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar a GLAD" << endl;
        return -1;
    }

    // Informações de versão do contexto
    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version supported " << glGetString(GL_VERSION) << endl;

    cout << "\n--- Lista 3: triangulos com o clique do mouse ---" << endl;
    cout << "  Clique com o botao ESQUERDO para criar 1 vertice." << endl;
    cout << "  A cada 3 vertices um triangulo e formado, com uma cor nova." << endl;
    cout << "  [C] Limpar   [ESC] Sair\n"
         << endl;

    // Viewport com as dimensões REAIS do framebuffer (trata telas HiDPI/retina,
    // em que o framebuffer é maior que o tamanho da janela em pontos).
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Compila e linka o programa de shader
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // -----------------------------------------------------------------------
    // CÂMERA 2D: janela do mundo com as mesmas dimensões da tela (800 x 600).
    // glm::ortho(left, right, bottom, top) mapeia bottom -> NDC -1 e
    // top -> NDC +1. Com bottom=600 e top=0, o Y do mundo cresce para baixo,
    // igual ao sistema de coordenadas da tela (e do mouse).
    // -----------------------------------------------------------------------
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f);
    GLint projLoc = glGetUniformLocation(shaderID, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // -----------------------------------------------------------------------
    // VAO único + VBO dinâmico (interleaved: posição + cor).
    // O VBO começa vazio; cresce a cada clique. Os ponteiros de atributo são
    // definidos UMA vez aqui: como o VAO guarda essa "receita" (incluindo o
    // VBO associado), reenviar os dados depois com glBufferData não a altera.
    // -----------------------------------------------------------------------
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    const GLsizei stride = FLOATS_PER_VERTEX * sizeof(GLfloat);

    // Atributo 0: posição (x, y, z) — 3 floats no início de cada vértice
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)0);
    glEnableVertexAttribArray(0);
    // Atributo 1: cor (r, g, b) — 3 floats logo após a posição
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    double prevTime = glfwGetTime();  // referência para o cálculo do FPS
    double titleCountdown = 0.1;      // intervalo de atualização do título

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        // Atualiza o título da janela com a contagem e o FPS
        {
            double currTime = glfwGetTime();
            double elapsed = currTime - prevTime;
            prevTime = currTime;

            titleCountdown -= elapsed;
            if (titleCountdown <= 0.0 && elapsed > 0.0)
            {
                int nVerts = (int)(vertices.size() / FLOATS_PER_VERTEX);
                int nTris = nVerts / 3;
                char title[256];
                snprintf(title, sizeof(title),
                         "Lista 3: %d vertices (%d triangulos)\tFPS %.2lf",
                         nVerts, nTris, 1.0 / elapsed);
                glfwSetWindowTitle(window, title);
                titleCountdown = 0.1;
            }
        }

        // Processa os eventos de input (o clique do mouse é tratado no
        // respectivo callback, que pode adicionar vértices)
        glfwPollEvents();

        // Se o callback adicionou vértices, reenvia o buffer completo à GPU
        if (geometryDirty)
        {
            uploadGeometry();
            geometryDirty = false;
        }

        // Limpa o framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        // Vértices pertencentes a triângulos COMPLETOS (múltiplo de 3).
        // O restante são vértices "pendentes" do triângulo em construção.
        GLsizei nVerts = (GLsizei)(vertices.size() / FLOATS_PER_VERTEX);
        GLsizei nComplete = (nVerts / 3) * 3;

        if (nComplete > 0)
        {
            glDrawArrays(GL_TRIANGLES, 0, nComplete);
        }

        // Vértices pendentes (1 ou 2) como pontos, como retorno visual
        if (nVerts > nComplete)
        {
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, nComplete, nVerts - nComplete);
        }

        glBindVertexArray(0);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Desaloca os buffers da GPU e finaliza a GLFW
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

// Teclado: ESC fecha a janela; C limpa a cena.
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key == GLFW_KEY_C)
    {
        vertices.clear();
        geometryDirty = true;
        cout << "Cena limpa!" << endl;
    }
}

// Mouse: cada clique ESQUERDO cria exatamente 1 vértice.
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    (void)mods;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    // Posição do cursor em coordenadas da janela: origem no canto superior
    // esquerdo, Y crescendo para baixo — idêntica ao sistema do mundo.
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    appendVertex(xpos, ypos);
}

// ---------------------------------------------------------------------------
// Funções auxiliares
// ---------------------------------------------------------------------------

// Acrescenta 1 vértice ao vetor interleaved. A cor depende de a qual triângulo
// o vértice pertence: a cada 3 vértices inicia-se um triângulo novo, que
// recebe uma cor nova. Os 3 vértices de um triângulo compartilham a cor.
void appendVertex(double xpos, double ypos)
{
    int nVerts = (int)(vertices.size() / FLOATS_PER_VERTEX);
    int triIndex = nVerts / 3;  // índice (0-based) do triângulo em construção

    // Cor nova por rotação de matiz pelo ângulo áureo (~137,5°), com
    // saturação e valor fixos. Mantém triângulos consecutivos bem distintos.
    const float GOLDEN_ANGLE = 137.507764f;
    float hue = fmodf((float)triIndex * GOLDEN_ANGLE, 360.0f);
    float r, g, b;
    hsvToRgb(hue, 0.75f, 1.0f, r, g, b);

    // Posição + cor do vértice (z = 0, cena 2D)
    vertices.push_back((float)xpos);
    vertices.push_back((float)ypos);
    vertices.push_back(0.0f);
    vertices.push_back(r);
    vertices.push_back(g);
    vertices.push_back(b);

    geometryDirty = true;

    cout << "  Vertice (" << (int)xpos << ", " << (int)ypos << ")"
         << " -> " << (nVerts + 1) << " vertice(s), "
         << ((nVerts + 1) / 3) << " triangulo(s)" << endl;
}

// Reenvia o vetor completo de vértices para o VBO dinâmico.
void uploadGeometry()
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(float)),
                 vertices.empty() ? nullptr : vertices.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Converte HSV (h em graus; s, v em 0..1) para RGB (0..1).
void hsvToRgb(float h, float s, float v, float &r, float &g, float &b)
{
    // Normaliza a matiz para [0, 360)
    h = fmodf(h, 360.0f);
    if (h < 0.0f)
        h += 360.0f;

    int i = (int)(h / 60.0f) % 6;
    float f = h / 60.0f - (float)(int)(h / 60.0f);
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
    default: r = v, g = p, b = q; break;  // caso 5
    }
}

// Compila e linka o programa de shader. Reporta erros no terminal.
int setupShader()
{
    GLint success;
    GLchar infoLog[512];

    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    // Linka o programa
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return (int)shaderProgram;
}
