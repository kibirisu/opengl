/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0 / 2.0;
float fov   =  45.0f;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

GLuint tex0;
GLuint tex1;
GLuint wall;



void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  fov -= (float)yoffset;
  if (fov < 1.0f)
    fov = 1.0f;
  if (fov > 45.0f)
    fov = 45.0f;
}

//Error processing callback procedure
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	//Read into computers memory
	std::vector<unsigned char> image;   //Allocate memory 
	unsigned width, height;   //Variables for image size
	//Read the image
	unsigned error = lodepng::decode(image, width, height, filename);

	//Import to graphics card memory
	glGenTextures(1, &tex); //Initialize one handle
	glBindTexture(GL_TEXTURE_2D, tex); //Activate handle
	//Copy image to graphics cards memory reprezented by the active handle
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}

void texCube(glm::mat4 P, glm::mat4 V, glm::mat4 M, GLuint tex) {

	spTextured->use();

	glUniformMatrix4fv(spTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spTextured->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spTextured->u("M"), 1, false, glm::value_ptr(M));


	glEnableVertexAttribArray(spTextured->a("vertex"));
	glVertexAttribPointer(spTextured->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

	glEnableVertexAttribArray(spTextured->a("texCoord"));
	glVertexAttribPointer(spTextured->a("texCoord"), 2, GL_FLOAT, false, 0, myCubeTexCoords);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(spTextured->u("tex"), 0);

	glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

	glDisableVertexAttribArray(spTextured->a("vertex"));
	glDisableVertexAttribArray(spTextured->a("color"));
}

//Initialization code procedure
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	//************Place any code here that needs to be executed once, at the program start************
	glClearColor(0, 0, 0, 1); //Set color buffer clear color
	glEnable(GL_DEPTH_TEST); //Turn on pixel depth test based on depth buffer
	tex0=readTexture("stone-wall.png");
	tex1 = readTexture("bricks.png");
	wall = readTexture("bluu.png");
}

//Release resources allocated by the program
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	glDeleteTextures(1, &tex0);
	glDeleteTextures(1, &tex1);
	glDeleteTextures(1, &wall);


	//************Place any code here that needs to be executed once, after the main loop ends************
}

void room1exit(glm::mat4 Ms, glm::mat4 P, glm::mat4 V) {

	glm::mat4 Mf1 = glm::scale(Ms, glm::vec3(2.0f, 0.025f, 2.0f));
	/*Mp = glm::translate(Mp, glm::vec3(0.0f, -0.0f, 0.0f));*/
	texCube(P, V, Mf1, tex0);

	glm::mat4 Mf2 = glm::scale(Ms, glm::vec3(2.0f, 0.025f, 2.0f));
	Mf2 = glm::translate(Mf2, glm::vec3(0.0f, 30.0f, 0.0f));
	texCube(P, V, Mf2, tex0);


	glm::mat4 Mw1 = glm::scale(Ms, glm::vec3(2.0f, 0.375f, 0.025f));
	Mw1 = glm::translate(Mw1, glm::vec3(0.0f, 1.0f, 80.0f));
	texCube(P, V, Mw1, wall);


	glm::mat4 Mw2 = glm::scale(Ms, glm::vec3(2.0f, 0.375f, 0.025f));
	Mw2 = glm::translate(Mw2, glm::vec3(0.0f, 1.0f, -80.0f));
	texCube(P, V, Mw2, wall);


	glm::mat4 Mw3 = glm::scale(Ms, glm::vec3(0.025f, 0.375f, 2.0f));
	Mw3 = glm::translate(Mw3, glm::vec3(80.0f, 1.0f, 0.0f));
	texCube(P, V, Mw3, wall);


	glm::mat4 Mw4 = glm::scale(Ms, glm::vec3(0.025f, 0.375f, 2.0f));
	Mw4 = glm::translate(Mw4, glm::vec3(-80.0f, 1.0f, 0.0f));

	
	glm::mat4 Mk1 = glm::scale(Mw4, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk1 = glm::translate(Mk1, glm::vec3(0.0f, 0.0f, 1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk1));
	Models::cube.drawSolid();

	glm::mat4 Mk2 = glm::scale(Mw4, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk2 = glm::translate(Mk2, glm::vec3(0.0f, 0.0f, -1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk2));
	Models::cube.drawSolid();
}

void room2exit(glm::mat4 Ms) {

	glm::mat4 Mf1 = glm::scale(Ms, glm::vec3(2.0f, 0.025f, 2.0f));
	/*Mp = glm::translate(Mp, glm::vec3(0.0f, -0.0f, 0.0f));*/
	glUniform4f(spLambert->u("color"), 1, 1, 1, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf1));
	Models::cube.drawSolid();

	glm::mat4 Mf2 = glm::scale(Ms, glm::vec3(2.0f, 0.025f, 2.0f));
	Mf2 = glm::translate(Mf2, glm::vec3(0.0f, 30.0f, 0.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf2));
	Models::cube.drawSolid();

	glm::mat4 Mw1 = glm::scale(Ms, glm::vec3(2.0f, 0.375f, 0.025f));
	Mw1 = glm::translate(Mw1, glm::vec3(0.0f, 1.0f, 80.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mw1));
	Models::cube.drawSolid();

	glm::mat4 Mw2 = glm::scale(Ms, glm::vec3(2.0f, 0.375f, 0.025f));
	Mw2 = glm::translate(Mw2, glm::vec3(0.0f, 1.0f, -80.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mw2));
	Models::cube.drawSolid();

	glm::mat4 Mw4 = glm::scale(Ms, glm::vec3(0.025f, 0.375f, 2.0f));
	Mw4 = glm::translate(Mw4, glm::vec3(-80.0f, 1.0f, 0.0f));

	glm::mat4 Mk1 = glm::scale(Mw4, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk1 = glm::translate(Mk1, glm::vec3(0.0f, 0.0f, 1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk1));
	Models::cube.drawSolid();

	glm::mat4 Mk2 = glm::scale(Mw4, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk2 = glm::translate(Mk2, glm::vec3(0.0f, 0.0f, -1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk2));
	Models::cube.drawSolid();

	glm::mat4 Mw5 = glm::scale(Ms, glm::vec3(0.025f, 0.375f, 2.0f));
	Mw5 = glm::translate(Mw5, glm::vec3(80.0f, 1.0f, 0.0f));

	glm::mat4 Mk4 = glm::scale(Mw5, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk4 = glm::translate(Mk4, glm::vec3(0.0f, 0.0f, 1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk4));
	Models::cube.drawSolid();

	glm::mat4 Mk5 = glm::scale(Mw5, glm::vec3(1.0f, 1.0f, 0.4f));
	Mk5 = glm::translate(Mk5, glm::vec3(0.0f, 0.0f, -1.5f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mk5));
	Models::cube.drawSolid();
}

void corridor(glm::mat4 Ms)
{
	glm::mat4 Mf1 = glm::scale(Ms, glm::vec3(1.0f, 0.025f, 0.45f));
	/*Mp = glm::translate(Mp, glm::vec3(0.0f, -0.0f, 0.0f));*/
	glUniform4f(spLambert->u("color"), 1, 0, 1, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf1));
	Models::cube.drawSolid();

	glm::mat4 Mf2 = glm::scale(Ms, glm::vec3(1.0f, 0.025f, 0.45f));
	Mf2 = glm::translate(Mf2, glm::vec3(0.0f, 30.0f, 0.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf2));
	Models::cube.drawSolid();

	glm::mat4 Mf3 = glm::scale(Ms, glm::vec3(1.0f, 0.375f, 0.025f));
	Mf3 = glm::translate(Mf3, glm::vec3(0.0f, 1.0f, 17.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf3));
	Models::cube.drawSolid();

	glm::mat4 Mf4 = glm::scale(Ms, glm::vec3(1.0f, 0.375f, 0.025f));
	Mf4 = glm::translate(Mf4, glm::vec3(0.0f, 1.0f, -17.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mf4));
	Models::cube.drawSolid();
}

void paintings(glm::mat4 Ms, glm::mat4 P, glm::mat4 V)
{
	/*spTextured->use();*/
	glm::mat4 Mp1 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp1 = glm::translate(Mp1, glm::vec3(-8.0f, 2.0f, -99.0f));
	texCube(P, V, Mp1, tex0);


	glm::mat4 Mp2 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp2 = glm::translate(Mp2, glm::vec3(-4.0f, 2.0f, -99.0f));
	texCube(P, V, Mp2, tex0);


	glm::mat4 Mp3 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp3 = glm::translate(Mp3, glm::vec3(4.0f, 2.0f, -99.0f));
	texCube(P, V, Mp3, tex0);


	glm::mat4 Mp4 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp4 = glm::translate(Mp4, glm::vec3(8.0f, 2.0f, -99.0f));
	texCube(P, V, Mp4, tex0);


	glm::mat4 Mp0 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp0 = glm::translate(Mp0, glm::vec3(0.0f, 2.0f, -99.0f));
	texCube(P, V, Mp0, tex0);


	glm::mat4 Mp5 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp5 = glm::translate(Mp5, glm::vec3(-8.0f, 2.0f, 99.0f));
	texCube(P, V, Mp5, tex0);


	glm::mat4 Mp6 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp6 = glm::translate(Mp6, glm::vec3(-4.0f, 2.0f, 99.0f));
	texCube(P, V, Mp6, tex0);


	glm::mat4 Mp7 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp7 = glm::translate(Mp7, glm::vec3(4.0f, 2.0f, 99.0f));
	texCube(P, V, Mp7, tex0);


	glm::mat4 Mp8 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp8 = glm::translate(Mp8, glm::vec3(8.0f, 2.0f, 99.0f));
	texCube(P, V, Mp8, tex0);


	glm::mat4 Mp9 = glm::scale(Ms, glm::vec3(0.18f, 0.18f, 0.02f));
	Mp9 = glm::translate(Mp9, glm::vec3(0.0f, 2.0f, 99.0f));
	texCube(P, V, Mp9, tex1);

}

void endPaintings(glm::mat4 Ms, glm::mat4 P, glm::mat4 V)
{

	glm::mat4 Mp2 = glm::scale(Ms, glm::vec3(0.02f, 0.18f, 0.18f));
	Mp2 = glm::translate(Mp2, glm::vec3(-99.0f, 2.0f, 6.5f));
	texCube(P, V, Mp2, tex0);

	glm::mat4 Mp3 = glm::scale(Ms, glm::vec3(0.02f, 0.18f, 0.18f));
	Mp3 = glm::translate(Mp3, glm::vec3(-99.0f, 2.0f, -6.5f));
	texCube(P, V, Mp3, tex1);

	glm::mat4 Mp4 = glm::scale(Ms, glm::vec3(0.02f, 0.18f, 0.18f));
	Mp4 = glm::translate(Mp4, glm::vec3(99.0f, 2.0f, 6.5f));
	texCube(P, V, Mp4, tex0);

	glm::mat4 Mp5 = glm::scale(Ms, glm::vec3(0.02f, 0.18f, 0.18f));
	Mp5 = glm::translate(Mp5, glm::vec3(99.0f, 2.0f, -6.5f));
	texCube(P, V, Mp5, tex1);
}

void midPainting(glm::mat4 Ms, glm::mat4 P, glm::mat4 V)
{
	glm::mat4 Mp1 = glm::scale(Ms, glm::vec3(0.02f, 0.18f, 0.18f));
	Mp1 = glm::translate(Mp1, glm::vec3(-99.0f, -2.0f, 0.0f));
	texCube(P, V, Mp1, tex1);
}

void character() {
	glm::mat4 Ms = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.65f, 0.0f));

	// Corpus
	glm::mat4 Mp = glm::scale(Ms, 0.1f * glm::vec3(0.125f, 0.5f, 0.5f));
	Mp = glm::translate(Mp, glm::vec3(0.0f, 2.0f, 0.0f));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mp));
	Models::cube.drawSolid();

	/* // Legs*/
	glm::mat4 Ml = glm::scale(Ms, 0.1f * glm::vec3(0.125f, 0.5f, 0.20f));
	Ml = glm::translate(Ml, glm::vec3(0.0f, 0.0f, -1.5f));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Ml));
	Models::cube.drawSolid();
	glm::mat4 Ml2 = glm::scale(Ms, 0.1f * glm::vec3(0.125f, 0.5f, 0.20f));
	Ml2 = glm::translate(Ml2, glm::vec3(0.0f, 0.0f, 1.5));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Ml2));
	Models::cube.drawSolid();

  // Hands
	glm::mat4 Mr = glm::scale(Ms, 0.1f * glm::vec3(0.125f, 0.5f, 0.20f));
	Mr = glm::translate(Mr, glm::vec3(0.0f, 2.0f, 3.5f));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mr));
	Models::cube.drawSolid();
	glm::mat4 Mr2 = glm::scale(Ms, 0.1f * glm::vec3(0.125f, 0.5f, 0.20f));
	Mr2 = glm::translate(Mr2, glm::vec3(0.0f, 2.0f, -3.5f));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mr2));
	Models::cube.drawSolid();

  // Head
  glm::mat4 Mh = glm::scale(Ms, 0.1f * glm::vec3(0.5, 0.5, 0.5));
  Mh = glm::translate(Mh, glm::vec3(0.0f, 4.0f, 0.0f));
	glUniform4f(spLambert->u("color"), 1, 0, 0, 1);
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Mh));
  Models::sphere.drawSolid();
}



//Drawing procedure
void drawScene(GLFWwindow* window, float angle, float wheelAngle) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear color and depth buffers

	glm::mat4 P = glm::perspective(glm::radians(fov), 1.0f, 0.1f, 100.0f);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	//spLambert->use();//Aktywacja programu cieniującego
    character();
	//glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P));
	//glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V));

	glm::mat4 Ms = glm::mat4(1.0f);


	spTextured->use();
	//glBindTexture(GL_TEXTURE_2D, tex1);


	midPainting(Ms,P,V);
	//spLambert->use();//Aktywacja programu cieniującego

	Ms = glm::rotate(Ms, PI, glm::vec3(0.0f, 0.0f, 1.0f));
	room1exit(Ms,P,V);
	endPaintings(Ms, P, V);
	paintings(Ms,P,V);

	//Ms = glm::rotate(Ms, PI, glm::vec3(0.0f, 1.0f, 0.0f));
	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//corridor(Ms);
	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//room2exit(Ms);
	//paintings(Ms);
	/*endPaintings(Ms,P,V);*/

	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//corridor(Ms);
	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//room2exit(Ms);
	//paintings(Ms);
	//endPaintings(Ms);

	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//corridor(Ms);
	//Ms = glm::translate(Ms, glm::vec3(3.0f, 0.0f, 0.0f));
	//room1exit(Ms);
	//paintings(Ms);
	//endPaintings(Ms);
	//Ms = glm::rotate(Ms, PI, glm::vec3(0.0f, 1.0f, 0.0f));
	//Ms = glm::translate(Ms, glm::vec3(0.0f, 0.72f, 0.0f));

	//midPainting(Ms);
	//Ms = glm::rotate(Ms, PI, glm::vec3(0.0f, 1.0f, 0.0f));



	glfwSwapBuffers(window); //Copy back buffer to the front buffer
}

int main(void)
{
	GLFWwindow* window; //Pointer to object that represents the application window

	glfwSetErrorCallback(error_callback);//Register error processing callback procedure

	if (!glfwInit()) { //Initialize GLFW library
		fprintf(stderr, "Can't initialize GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1000, 1000, "OpenGL", NULL, NULL);  //Create a window 500pxx500px titled "OpenGL" and an OpenGL context associated with it. 

	if (!window) //If no window is opened then close the program
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Since this moment OpenGL context corresponding to the window is active and all OpenGL calls will refer to this context.
	glfwSwapInterval(1); //During vsync wait for the first refresh

	GLenum err;
	if ((err = glewInit()) != GLEW_OK) { //Initialize GLEW library
		fprintf(stderr, "Can't initialize GLEW: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Call initialization procedure

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, mouse_callback);

	//Main application loop
	float angle = 0; //declare variable for storing current rotation angle
	float wheelAngle = 0;
	glfwSetTime(0); //clear internal timer
	while (!glfwWindowShouldClose(window)) //As long as the window shouldnt be closed yet...
	{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    processInput(window);
		/*angle += speed * glfwGetTime(); //Compute an angle by which the object was rotated during the previous frame*/
		/*wheelAngle += PI / 6 * glfwGetTime(); //Compute an angle by which the object was rotated during the previous frame*/
		/*glfwSetTime(0); //clear internal timer*/
		drawScene(window, angle, wheelAngle); //Execute drawing procedure
		glfwPollEvents(); //Process callback procedures corresponding to the events that took place up to now
	}
	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Delete OpenGL context and the window.
	glfwTerminate(); //Free GLFW resources
	exit(EXIT_SUCCESS);
}
