#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int WIDTH = 1280;
int HEIGHT = 720;

struct State {
    float a;
    float fov;
    float z;
    int frame_count;
    float sum_fps;
    float avg_fps;
    bool msaa;
};

GLuint msFBO;


/// UTILS ///
std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream ss;

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const std::string& source, const std::string& shaderPath) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, ' ');
        glGetShaderInfoLog(shader, logLength, nullptr, &log[0]);
        std::cerr << "Error compiling shader (" << shaderPath << "):\n" << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint createProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode, vertexPath);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode, fragmentPath);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint linkSuccess;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, ' ');
        glGetProgramInfoLog(program, logLength, nullptr, &log[0]);
        std::cerr << "Error linking shader program:\n" << log << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    // Clean up shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


/// SYSTEM ///
GLuint CreateMSAAFBO(int, int, int);
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    WIDTH = width;
    HEIGHT = height;
    msFBO = CreateMSAAFBO(WIDTH, HEIGHT, 4);
    glViewport(0, 0, width, height);

}

GLFWwindow* InitWindow(int width, int height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "MiniGL - Shadow Mapping", nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1); // Vsync on: 1, Vsync off: 0

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return nullptr;
    }

    // Imgui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    io.Fonts->AddFontFromFileTTF(
        "resources/FiraCode-Regular.ttf",
        16.0f
    );
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");

    return window;
}

void Cleanup(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}


/// MSAA ///
GLuint CreateMSAAFBO(int width, int height, int samples) {
    // Create multisampled FBO
    GLuint msFBO;
    glGenFramebuffers(1, &msFBO);
    int old_width = WIDTH;
    int old_height = HEIGHT;
    const int MSAA_SAMPLES = 4;
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
        // Color renderbuffer for MSAA FBO
        GLuint msColorBuffer;
        glGenRenderbuffers(1, &msColorBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, msColorBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLES, GL_RGBA8, WIDTH, HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msColorBuffer);

        // Depth renderbuffer for MSAA FBO
        GLuint msDepthBuffer;
        glGenRenderbuffers(1, &msDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, msDepthBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLES, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msDepthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return msFBO;
}

void ResolveMSAA(GLuint msFBO, int width, int height) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(
        0, 0, WIDTH, HEIGHT,
        0, 0, WIDTH, HEIGHT,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
}


/// UI ///
void BuildUI(State &state) {
    // Imgui stuff
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Settings");

    // FPS counter
    ImGui::Text("%.1f fps (%.0f avg)", ImGui::GetIO().Framerate, state.avg_fps);
    state.sum_fps += ImGui::GetIO().Framerate;
    state.avg_fps = state.sum_fps / state.frame_count;

    // Sliders
    ImGui::SliderFloat("a", &state.a, 0.0f, 360.0f);
    ImGui::SliderFloat("fov", &state.fov, 0.0f, 360.0f);
    ImGui::SliderFloat("z", &state.z, -10.0f, 10.0f);
    
    ImGui::End();
    ImGui::Render();
}

void RenderUI() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


/// MESH ///
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    GLsizei indexCount = 0;
};

Mesh CreateMesh(const float* vertices, size_t vertexBytes, const unsigned int* indices, size_t indexBytes) {
    Mesh mesh;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indexBytes / sizeof(unsigned int));

    return mesh;
}

Mesh CreateCube() {
    // Cube data position | normal | uv
    static float vertices[] = {
    // positions           // normals         // texture coords
    // Front face
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

    // Back face
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, -1.0f,  1.0f, 1.0f,

    // Left face
    -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

    // Right face
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,

    // Top face
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,

    // Bottom face
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
    };

    static unsigned int indices[] = {
        // front
        0, 1, 2,  2, 3, 0,
        // back
        6, 5, 4,  4, 7, 6,
        // left
        10, 9, 8, 8, 11, 10,
        // right
        12,13,14, 14,15,12,
        // top
        18, 17, 16, 16, 19, 18,
        // bottom
        20,21,22, 22,23,20
    };

    return CreateMesh(
        vertices,
        sizeof(vertices),
        indices,
        sizeof(indices)
    );
}

void DrawMesh(const Mesh& mesh) {
    glBindVertexArray(mesh.vao);

    glDrawElements(
        GL_TRIANGLES,
        mesh.indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );
}

/// Shaders ///  
GLuint CreateShader() {
    unsigned int shaderProgram = createProgram("shaders/simple.vert", "shaders/simple.frag");
    glUseProgram(shaderProgram);
    return shaderProgram;
}

int main() {
    GLFWwindow* window = InitWindow(WIDTH, HEIGHT);

    State state = {
        .a = 0.0f,
        .fov = 45.0f,
        .z = 3.0f,
        .frame_count = 0,
        .sum_fps = 0.0f,
        .avg_fps = 0.0f,
        .msaa = true 
    };
    
    // Enable Depth
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if(state.msaa) {
        msFBO = CreateMSAAFBO(WIDTH, HEIGHT, 4);
    }
    
    // Shader
    GLuint shaderProgram = CreateShader();
        GLint uMatrixLoc = glGetUniformLocation(shaderProgram, "uMatrix");
        glm::mat4 uMatrix = glm::mat4(1.0f);
        glUniformMatrix4fv(uMatrixLoc, 1, GL_FALSE, glm::value_ptr(uMatrix));

    // Scene
    Mesh cube = CreateCube();

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if(state.msaa) {
            glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Clear the screen
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.3f, 0.5f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // UI
        BuildUI(state);

        // PROJECTION
        glm::mat4 projection = glm::perspective(
            glm::radians(state.fov),
            static_cast<float>(WIDTH) / static_cast<float>(HEIGHT),
            0.1f,
            100.0f
        );

        // VIEW
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, state.z), // camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // look at
            glm::vec3(0.0f, 1.0f, 0.0f)  // up
        );

        // Cube
        {
            // MODEL
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(state.a), glm::vec3(0, 1, 0));

            // Set uniforms and draw the mesh
            glUseProgram(shaderProgram);
                uMatrix = projection * view * model;
                glUniformMatrix4fv(uMatrixLoc, 1, GL_FALSE, glm::value_ptr(uMatrix));
                DrawMesh(cube);
        }
        
        if(state.msaa) {
            ResolveMSAA(msFBO, WIDTH, HEIGHT);
        }
        RenderUI();
        glfwSwapBuffers(window);
        state.frame_count++;
    }

    Cleanup(window);

    return 0;
}