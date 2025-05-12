#version 330 core
out vec4 FragColor;
uniform vec4 uColor; // Added color uniform
void main() {
    FragColor = uColor; // Use the uniform
}
