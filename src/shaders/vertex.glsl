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
