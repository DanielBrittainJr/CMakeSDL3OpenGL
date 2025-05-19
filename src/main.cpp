// src/main.cpp

#include <SDL3/SDL.h>
#include <filesystem>
#include <glad/glad.h>
// --- New Includes ---
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_video.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <chrono>
#include <iostream>
#include <vector>

#include <fstream>
#include <sstream>

//profiling
#include <tracy/Tracy.hpp>

/*
* Load Shader File. Located in src/shaders.
*/
std::string LoadShaderSource(const char* filepath);

static GLuint CompileShader(GLenum type, const char* src);
static GLuint LinkProgram(GLuint vs, GLuint fs);

void SetGLAttributes();
void InitSDL();

struct GLContext {
    SDL_Window* window;
    SDL_GLContext context;
};

GLContext InitSDLGL(const char* title, int width, int height);
void PrintGLInfo();
ImGuiIO& InitIMGUI(GLContext gl);
void CleanupImgui();
void CleanupSDL(GLContext gl);
void ConfigImgui(ImGuiIO& io, glm::vec4 shapeColor, glm::vec4 clearColor);
void SetupTriangle();

int main(int argc, char** argv) {
	ZoneScoped;
	ZoneName("Main Function", sizeof("Main Function"));

    //************************INIT PROGRAM*****************************
    InitSDL();
    SetGLAttributes();
    GLContext gl = InitSDLGL("Dematik", 1024, 768);
    PrintGLInfo();
    //Init IMGUI
    ImGuiIO& io = InitIMGUI(gl);

    //*************************SHADER STUFF******************************

    //Load shaders from file
    std::string vertexSource = LoadShaderSource("src/shaders/vertex.glsl");
    std::string fragmentSource = LoadShaderSource("src/shaders/fragment.glsl");

    //Compile Shaders
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSource.c_str());
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());

    //Link Shaders
    GLuint program = LinkProgram(vs, fs);
    glDeleteShader(vs); // Delete shaders after linking
    glDeleteShader(fs);
    

    //Setup triangle
    //void SetupTriangle();

    // --- Triangle Data ---
    float triVerts[] = {
        0.0f,  0.5f,  // Top
        -0.5f, -0.5f, // Left
        0.5f,  -0.5f  // Right
    };

    //Vertex array object, vertex buffer object
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void*)0);

    auto t0 = std::chrono::high_resolution_clock::now();
    glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.12f, 1.0f);
    glm::vec4 triangleColor = glm::vec4(1.0f, 0.5f, 0.1f, 1.0f); // Initial color

    //***************************GAMELOOP***************************
    bool running = true;
    while (running) {

        //*****************POLL EVENTS********************
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);

            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (!io.WantCaptureKeyboard) {
                        if (ev.key.key == SDLK_ESCAPE) running = false;
                    }
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    int w_px, h_px;
                    SDL_GetWindowSizeInPixels(gl.window, &w_px, &h_px);
                    glViewport(0, 0, w_px, h_px);
                    break;
                default:
                    break;
            }
        }
        //**********************GAME LOOP************************
        //Imgui config
	ZoneScoped;
	ZoneName("GameLoop", sizeof("Gameloop"));
        ConfigImgui(io, triangleColor, clearColor);


        auto t1 = std::chrono::high_resolution_clock::now();
        float s = std::chrono::duration<float>(t1 - t0).count();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y); // Use ImGui display size
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        GLint angleLoc = glGetUniformLocation(program, "uAngle");
        glUniform1f(angleLoc, s); // Update rotation angle

        GLint colorLoc = glGetUniformLocation(program, "uColor");
        glUniform4fv(colorLoc, 1, glm::value_ptr(triangleColor));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0); // Unbind VAO

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(gl.window);
    }

    //**********************CLEANUP PROGRAM******************
    //Cleanup IMGUI
    CleanupImgui();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);

    //Cleanup SDL
    CleanupSDL(gl);

    return 0;
}
//****************FUNCTION IMPLEMENTATIONS*************************
static GLuint LinkProgram(GLuint vs, GLuint fs) 
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLength = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> buf(logLength);
        glGetProgramInfoLog(p, logLength, nullptr, buf.data());
        std::cerr << "Program link error: " << buf.data() << "\n";
        glDeleteProgram(p);
        std::exit(-1);
    }
    glDetachShader(p, vs);
    glDetachShader(p, fs);
    return p;
}

std::string LoadShaderSource(const char* filepath) 
{
    std::ifstream file(filepath);
    if(!file) {
        std::cerr << "Failed to open shader file: " << filepath << "\n";
        std::exit(-1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
static GLuint CompileShader(GLenum type, const char* src) 
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLength = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> buf(logLength);
        glGetShaderInfoLog(s, logLength, nullptr, buf.data());
        std::cerr << "Shader compile error: " << buf.data() << "\n";
        glDeleteShader(s);
        std::exit(-1);
    }
    return s;
}
void SetGLAttributes() {

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
}
void InitSDL(){
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return;
    }
}
GLContext InitSDLGL(const char* title, int width, int height) {
    GLContext gl;

    gl.window = SDL_CreateWindow(
        title, width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!gl.window) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        std::exit(-1);
    }

    gl.context = SDL_GL_CreateContext(gl.window);
    if (!gl.context) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(gl.window);
        SDL_Quit();
        std::exit(-1);
    }

    SDL_GL_MakeCurrent(gl.window, gl.context);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to init GlAD: %s", SDL_GetError());
        SDL_GL_DestroyContext(gl.context);
        SDL_DestroyWindow(gl.window);
        SDL_Quit();
        std::exit(-1);
    }

    return gl;
}
void PrintGLInfo(){

    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
              << "\n";
}
ImGuiIO& InitIMGUI(GLContext gl){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    const char* glsl_version = "#version 330 core"; // Match shader version
    ImGui_ImplSDL3_InitForOpenGL(gl.window, gl.context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return io;
}
void CleanupImgui(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return;
}

void CleanupSDL(GLContext gl){

    SDL_GL_DestroyContext(gl.context);
    SDL_DestroyWindow(gl.window);
    SDL_Quit();

    return;
}
void ConfigImgui(ImGuiIO& io,glm::vec4 shapeColor,glm::vec4 clearColor) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");
    ImGui::ColorEdit4("Clear Color", glm::value_ptr(clearColor));
    ImGui::ColorEdit4("Triangle Color",
                      glm::value_ptr(shapeColor));
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();

    return;
}
void SetupTriangle(){

}
