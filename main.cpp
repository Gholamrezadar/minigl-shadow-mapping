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
#include <vector>

// Globals // 
int WIDTH = 1280;
int HEIGHT = 720;


// Types //
enum MSAA_SAMPLES {
    MSAA_SAMPLES_4  = 4,
    MSAA_SAMPLES_8  = 8,
    MSAA_SAMPLES_16 = 16
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    GLsizei indexCount = 0;
};

struct Primitive {
    Mesh mesh;
    glm::vec3 position;
    glm::vec3 scale;
    bool visible = true;
};

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    float fov;
};

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambient;
    float intensity;
    float size;
};

struct State {
    glm::vec3 clear_color;

    Camera camera;

    DirectionalLight light;

    float a;

    int frame_count;
    float sum_fps;
    float avg_fps;

    bool msaa;
    GLuint msFBO;
    MSAA_SAMPLES msaa_samples;

    bool wireframe;

    Primitive cube;
    Primitive plane;
    Primitive sphere;
    Primitive cylinder;

    GLuint default_shader;
    GLint uModelLoc;
    GLint uViewLoc;
    GLint uProjectionLoc;
    
    GLuint shadow_shader;
};

/// UTILS ///
std::string read_file(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream ss;

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    ss << file.rdbuf();
    return ss.str();
}

GLuint compile_shader(GLenum type, const std::string& source, const std::string& shaderPath) {
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

GLuint create_program(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = read_file(vertexPath);
    std::string fragmentCode = read_file(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;
    }

    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexCode, vertexPath);
    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentCode, fragmentPath);

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
GLuint create_MSAA_FBO(int, int, int);

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto* state = static_cast<State*>(glfwGetWindowUserPointer(window));
    WIDTH = width;
    HEIGHT = height;
    state->msFBO = create_MSAA_FBO(WIDTH, HEIGHT, state->msaa_samples);
    glViewport(0, 0, width, height);
}

GLFWwindow* init_window(int width, int height, State &state) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "MiniGL - Shadow Mapping", nullptr, nullptr);
    glfwSetWindowUserPointer(window, &state); // for resizing callback to work

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

void cleanup(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}


/// MSAA ///
GLuint create_MSAA_FBO(int width, int height, int samples) {
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

void resolve_MSAA(GLuint msFBO, int width, int height) {
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
void build_UI(State &state) {
    // Imgui stuff
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Settings");

    // FPS counter
    ImGui::Text("%.1f fps (%.0f avg)", ImGui::GetIO().Framerate, state.avg_fps);
    state.sum_fps += ImGui::GetIO().Framerate;
    state.avg_fps = state.sum_fps / state.frame_count;

    if(ImGui::BeginTabBar("Settings")) {
        if(ImGui::BeginTabItem("Camera")) {
            // Sliders
            ImGui::SliderFloat("fov", &state.camera.fov, 0.0f, 360.0f);
            ImGui::DragFloat3("Position", &state.camera.position.x, 0.1f);
            ImGui::DragFloat3("Target", &state.camera.target.x, 0.1f);
            ImGui::EndTabItem();
        }

        if(ImGui::BeginTabItem("Scene")) {

            if(ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Z Rotation", &state.a, 0.0f, 360.0f);
                ImGui::TreePop();
            }

            // Plane
            if (ImGui::TreeNodeEx("Plane", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible##Plane", &state.plane.visible);
                ImGui::DragFloat3("Position##Plane", &state.plane.position.x, 0.1f);
                ImGui::DragFloat3("Scale##Plane", &state.plane.scale.x, 0.1f);
                ImGui::TreePop();
            }

            // Cube
            if (ImGui::TreeNodeEx("Cube", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible##Cube", &state.cube.visible);
                ImGui::DragFloat3("Position##Cube", &state.cube.position.x, 0.1f);
                ImGui::DragFloat3("Scale##Cube", &state.cube.scale.x, 0.1f);
                ImGui::TreePop();
            }

            // Sphere
            if (ImGui::TreeNodeEx("Sphere", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible##Sphere", &state.sphere.visible);
                ImGui::DragFloat3("Position##Sphere", &state.sphere.position.x, 0.1f);
                ImGui::DragFloat3("Scale##Sphere", &state.sphere.scale.x, 0.1f);
                ImGui::TreePop();
            }

            // Cylinder
            if (ImGui::TreeNodeEx("Cylinder", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible##Cylinder", &state.cylinder.visible);
                ImGui::DragFloat3("Position##Cylinder", &state.cylinder.position.x, 0.1f);
                ImGui::DragFloat3("Scale##Cylinder", &state.cylinder.scale.x, 0.1f);
                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }

        if(ImGui::BeginTabItem("Lighting")) {
            if(ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Clear Color", &state.clear_color.x);
                ImGui::TreePop();
            }
            if(ImGui::TreeNodeEx("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Direction", &state.light.direction.x, 0.1f);
                ImGui::ColorEdit3("Color", &state.light.color.x);
                ImGui::ColorEdit3("Ambient", &state.light.ambient.x);
                ImGui::DragFloat("Intensity", &state.light.intensity, 0.1f);
                ImGui::DragFloat("Size", &state.light.size, 0.01f);
                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Rendering")) {

            // Wireframe
            ImGui::Checkbox("Wireframe", &state.wireframe);

            // MSAA
            ImGui::Checkbox("MSAA", &state.msaa);
            
            // MSAA samples
            static const char* msaa_samples_names[] = {
                "4",
                "8",
                "16"
            };

            // Convert enum to combo index
            int current_msaa_index = 0;
            switch (state.msaa_samples) {
                case MSAA_SAMPLES_4:  current_msaa_index = 0; break;
                case MSAA_SAMPLES_8:  current_msaa_index = 1; break;
                case MSAA_SAMPLES_16: current_msaa_index = 2; break;
            }

            if (ImGui::Combo(
                "MSAA Samples",
                &current_msaa_index,
                msaa_samples_names,
                IM_ARRAYSIZE(msaa_samples_names)))
            {
                // Convert combo index -> enum
                switch (current_msaa_index) {
                case 0: state.msaa_samples = MSAA_SAMPLES_4;  break;
                case 1: state.msaa_samples = MSAA_SAMPLES_8;  break;
                case 2: state.msaa_samples = MSAA_SAMPLES_16; break;
                }

                // Recreate MSAA FBO
                if (state.msaa) {
                    state.msFBO = create_MSAA_FBO(WIDTH, HEIGHT, state.msaa_samples);
                }
            }
            
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::Render();
}

void render_UI() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


/// MESH ///
Mesh create_mesh(const float* vertices, size_t vertexBytes, const unsigned int* indices, size_t indexBytes) {
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

void draw_mesh(const Mesh& mesh) {
    glBindVertexArray(mesh.vao);

    glDrawElements(
        GL_TRIANGLES,
        mesh.indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );
}


/// Primitives ///
Mesh create_cube() {
    static float vertices[] = {
    // pos           // normal         // uv
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

    return create_mesh(
        vertices,
        sizeof(vertices),
        indices,
        sizeof(indices)
    );
}

Mesh create_plane()
{
    static float vertices[] = {
        // pos                 normal          uv
        -0.5f, 0.0f, -0.5f,    0,1,0,          0,0,
         0.5f, 0.0f, -0.5f,    0,1,0,          1,0,
         0.5f, 0.0f,  0.5f,    0,1,0,          1,1,
        -0.5f, 0.0f,  0.5f,    0,1,0,          0,1,
    };

    static unsigned int indices[] = {
        0,1,2,
        2,3,0
    };

    return create_mesh(
        vertices,
        sizeof(vertices),
        indices,
        sizeof(indices)
    );
}

Mesh create_sphere(int rows, int cols)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float radius = 0.5f;

    for (int y = 0; y <= rows; y++)
    {
        float v = (float)y / rows;
        float phi = v * glm::pi<float>();

        for (int x = 0; x <= cols; x++)
        {
            float u = (float)x / cols;
            float theta = u * glm::two_pi<float>();

            float sx = sin(phi) * cos(theta);
            float sy = cos(phi);
            float sz = sin(phi) * sin(theta);

            // Position
            vertices.push_back(radius * sx);
            vertices.push_back(radius * sy);
            vertices.push_back(radius * sz);

            // Normal
            vertices.push_back(sx);
            vertices.push_back(sy);
            vertices.push_back(sz);

            // UV
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            int a = y * (cols + 1) + x;
            int b = a + cols + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);

            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }

    return create_mesh(
        vertices.data(),
        vertices.size() * sizeof(float),
        indices.data(),
        indices.size() * sizeof(unsigned int)
    );
}

Mesh create_cylinder(int rows, int cols)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float radius = 0.5f;
    const float halfHeight = 0.5f;

    // Side vertices
    for (int y = 0; y <= rows; y++)
    {
        float v = (float)y / rows;
        float py = glm::mix(-halfHeight, halfHeight, v);

        for (int x = 0; x <= cols; x++)
        {
            float u = (float)x / cols;
            float theta = u * glm::two_pi<float>();

            float nx = cos(theta);
            float nz = sin(theta);

            vertices.push_back(radius * nx);
            vertices.push_back(py);
            vertices.push_back(radius * nz);

            vertices.push_back(nx);
            vertices.push_back(0.0f);
            vertices.push_back(nz);

            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Side indices
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            int a = y * (cols + 1) + x;
            int b = a + cols + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);

            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }

    unsigned int topCenter =
        (unsigned int)(vertices.size() / 8);

    // Top center
    vertices.insert(vertices.end(), {
        0, halfHeight, 0,
        0,1,0,
        0.5f,0.5f
    });

    for (int x = 0; x <= cols; x++)
    {
        float u = (float)x / cols;
        float theta = u * glm::two_pi<float>();

        float px = radius * cos(theta);
        float pz = radius * sin(theta);

        vertices.insert(vertices.end(), {
            px, halfHeight, pz,
            0,1,0,
            px + 0.5f, pz + 0.5f
        });
    }

    for (int x = 0; x < cols; x++)
    {
        indices.push_back(topCenter);
        indices.push_back(topCenter + x + 1);
        indices.push_back(topCenter + x + 2);
    }

    unsigned int bottomCenter =
        (unsigned int)(vertices.size() / 8);

    vertices.insert(vertices.end(), {
        0,-halfHeight,0,
        0,-1,0,
        0.5f,0.5f
    });

    for (int x = 0; x <= cols; x++)
    {
        float u = (float)x / cols;
        float theta = u * glm::two_pi<float>();

        float px = radius * cos(theta);
        float pz = radius * sin(theta);

        vertices.insert(vertices.end(), {
            px,-halfHeight,pz,
            0,-1,0,
            px + 0.5f,pz + 0.5f
        });
    }

    for (int x = 0; x < cols; x++)
    {
        indices.push_back(bottomCenter);
        indices.push_back(bottomCenter + x + 2);
        indices.push_back(bottomCenter + x + 1);
    }

    return create_mesh(
        vertices.data(),
        vertices.size() * sizeof(float),
        indices.data(),
        indices.size() * sizeof(unsigned int)
    );
}

void draw_primitive(
    State& state,
    const Primitive& primitive,
    GLuint shaderProgram,
    const glm::mat4& projection,
    const glm::mat4& view,
    float angle)
{
    if (!primitive.visible)
        return;

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::translate(model, primitive.position);
    model = glm::scale(model, primitive.scale);

    glUseProgram(shaderProgram);
        glUniformMatrix4fv(state.uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(state.uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(state.uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    draw_mesh(primitive.mesh);
}

/// Shaders ///  
GLuint create_default_shader() {
    unsigned int shaderProgram = create_program("shaders/simple.vert", "shaders/simple.frag");
    glUseProgram(shaderProgram);
    return shaderProgram;
}

void send_light_camera_data_to_shader(GLuint shaderProgram, const State& state) {
    // light
    glUniform3fv(
        glGetUniformLocation(shaderProgram, "uLightDirection"),
        1,
        glm::value_ptr(state.light.direction)
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "uLightColor"),
        1,
        glm::value_ptr(state.light.color)
    );

    glUniform1f(
        glGetUniformLocation(shaderProgram, "uLightIntensity"),
        state.light.intensity
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "uLightAmbient"),
        1,
        glm::value_ptr(state.light.ambient)
    );

    glUniform1f(
        glGetUniformLocation(shaderProgram, "uLightSize"),
        state.light.size
    );

    // camera
    glUniform3fv(
        glGetUniformLocation(shaderProgram, "uCameraPos"),
        1,
        glm::value_ptr(state.camera.position)
    );
}

void shadow_pass(
    State& state
) {
    return;
}

void main_pass(
    State& state
) {
    if(state.msaa) 
        glBindFramebuffer(GL_FRAMEBUFFER, state.msFBO);
    else 
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Clear the screen
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(state.clear_color.x, state.clear_color.y, state.clear_color.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // UI
    build_UI(state);
    
    if (state.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // PROJECTION
    glm::mat4 projection = glm::perspective(
        glm::radians(state.camera.fov),
        static_cast<float>(WIDTH) / static_cast<float>(HEIGHT),
        0.1f,
        100.0f
    );
    
    // VIEW
    glm::mat4 view = glm::lookAt(
        state.camera.position, // camera position
        state.camera.target,   // look at
        glm::vec3(0.0f, 1.0f, 0.0f)  // up
    );
    
    send_light_camera_data_to_shader(state.default_shader, state);
    
    draw_primitive(state, state.cube,     state.default_shader, projection, view, state.a);
    draw_primitive(state, state.plane,    state.default_shader, projection, view, state.a);
    draw_primitive(state, state.sphere,   state.default_shader, projection, view, state.a);
    draw_primitive(state, state.cylinder, state.default_shader, projection, view, state.a);
    
    if(state.msaa) {
        resolve_MSAA(state.msFBO, WIDTH, HEIGHT);
    }
}

void init_state(State &state) {
    state = {
        .clear_color = glm::vec3(0.28f, 0.35f, 0.4f),
        .camera = {
            .position = glm::vec3(-1.5f, 1.1f, 3.3f),
            .target   = glm::vec3(0.176f, -0.8f, -0.1f),
            .fov = 41.0f
        },
        .light = {
            .direction = glm::vec3(15.0f, -8.0f, 10.0f),
            .color = glm::vec3(1.0f, 1.0f, 1.0f),
            .ambient = glm::vec3(0.075f, 0.161f, 0.235f),
            .intensity = 0.8f,
            .size = 0.001f
        },
        .a = 0.0f,
        .frame_count = 1,
        .sum_fps = 0.0f,
        .avg_fps = 0.0f,
        .msaa = true,
        .msaa_samples = MSAA_SAMPLES_4 ,
        .wireframe = false,
        .cube = {
            .position = glm::vec3(0.0f, -0.25f, 0.0f),
            .scale = glm::vec3(0.5f),
            .visible = true
        },
        .plane = {
            .position = glm::vec3(0.0f, -0.5f, 0.0f),
            .scale = glm::vec3(3.0f),
            .visible = true
        },
        .sphere = {
            .position = glm::vec3(-0.8f, -0.25f, 0.0f),
            .scale = glm::vec3(0.5f),
            .visible = true
        },
        .cylinder = {
            .position = glm::vec3(0.7f, 0.0f, 0.0f),
            .scale = glm::vec3(0.1f, 1.0f, 0.1f),
            .visible = true
        }
    };

    // Default Shader
    state.default_shader = create_default_shader();
        state.uModelLoc      = glGetUniformLocation(state.default_shader, "uModel");
        state.uViewLoc       = glGetUniformLocation(state.default_shader, "uView");
        state.uProjectionLoc = glGetUniformLocation(state.default_shader, "uProjection");
        glm::mat4 uModel      = glm::mat4(1.0f);
        glm::mat4 uView       = glm::mat4(1.0f);
        glm::mat4 uProjection = glm::mat4(1.0f);
        glUniformMatrix4fv(state.uModelLoc, 1, GL_FALSE, glm::value_ptr(uModel));
        glUniformMatrix4fv(state.uViewLoc, 1, GL_FALSE, glm::value_ptr(uView));
        glUniformMatrix4fv(state.uProjectionLoc, 1, GL_FALSE, glm::value_ptr(uProjection));
        send_light_camera_data_to_shader(state.default_shader, state);

    // Scene
    state.cube.mesh     = create_cube();
    state.plane.mesh    = create_plane();
    state.sphere.mesh   = create_sphere(32, 32);
    state.cylinder.mesh = create_cylinder(8, 32);
}

int main() {
    State state;
    GLFWwindow* window = init_window(WIDTH, HEIGHT, state);

    // Create world, shaders, ...
    init_state(state);

    // Enable Depth
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if(state.msaa) {
        state.msFBO = create_MSAA_FBO(WIDTH, HEIGHT, state.msaa_samples);
    }
    
    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        shadow_pass(
            state
        );

        main_pass(
            state
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        render_UI();

        glfwSwapBuffers(window);
        state.frame_count++;
    }

    cleanup(window);

    return 0;
}