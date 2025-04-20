// src/main.cpp

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <chrono>
#include <iostream>

static const char* kVertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform float uAngle;
void main() {
    mat2 rot = mat2(
        cos(uAngle), -sin(uAngle),
        sin(uAngle),  cos(uAngle)
    );
    gl_Position = vec4(rot * aPos, 0.0, 1.0);
}
)glsl";

static const char* kFragmentShaderSrc = R"glsl(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.5, 0.1, 1.0);
}
)glsl";

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, 512, nullptr, buf);
        std::cerr << "Shader compile error: " << buf << "\n";
        std::exit(-1);
    }
    return s;
}

static GLuint LinkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(p, 512, nullptr, buf);
        std::cerr << "Program link error: " << buf << "\n";
        std::exit(-1);
    }
    return p;
}

int main(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    SDL_Window* window =
        SDL_CreateWindow("SDL3 + GLAD Test",                       // title
                         800, 600,                                 // width, height
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE  // flags
        );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    GLuint program = LinkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float triVerts[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    bool running = true;
    auto t0 = std::chrono::high_resolution_clock::now();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    glViewport(0, 0, ev.window.data1, ev.window.data2);
                    break;
                default:
                    break;
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        float s = std::chrono::duration<float>(t1 - t0).count();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        GLint loc = glGetUniformLocation(program, "uAngle");
        glUniform1f(loc, s);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(ctx);  // renamed in SDL3
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

