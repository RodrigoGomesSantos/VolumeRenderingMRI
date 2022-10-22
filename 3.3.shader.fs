#version 330 core
out vec4 FragColor;
in vec4 ourColor;
void main()
{
	if(ourColor.a == 0.0f){
		discard;
	}   
	FragColor = ourColor;

}