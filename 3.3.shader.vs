
#version 330 core
layout (location = 1) in vec4 aColor;
layout (location = 0) in vec3 aPos;

out vec4 ourColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
				 //screen   //camera  //world  //local
   gl_Position = projection * view * model * vec4(aPos, 1.0);
   ourColor = aColor;
}