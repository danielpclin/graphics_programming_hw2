#include "../Externals/Include/Common.h"
#include <unordered_map>
#include <cmath>

enum Menu
{
	TEXTURE, NORMAL, IMAGE_ABSTRACTION, WATER_COLOR, MAGNIFIER, BLOOM, PIXELIZATION, SINE_DISTORTION
};


unsigned int timer_speed = 16;

using namespace glm;
using namespace std;
class Model;


// Global variables
int WIDTH = 1000, HEIGHT = 800;
GLuint program, blurProgram, screenProgram; // Shader program id
GLuint fbo;
GLuint textureColorBuffer[2];
GLuint rbo;
GLuint frameVAO;
GLuint frameVBO;
GLuint pingpongFBO[2];
GLuint pingpongColorBuffers[3];

vec3 pos = vec3(14.0, 2.0, 0.0);
vec3 frontVec = vec3(-1.0, 0.0, 0.0);
vec3 rightVec = normalize(cross(frontVec, vec3(0.0, 1.0, 0.0)));
float yawF = 180.0;
float pitchF = 0.0;
vec3 upVec = vec3(0.0, 1.0, 0.0);
mat4 mv;
mat4 p;
Model* sponza;

GLint um4mv;				// M matrix uniform id
GLint um4p;			// View and Perspective matrix uniform id
GLint utex;        // Texture uniform ids
GLint uMode, uTime, uMouse;

Menu mode = TEXTURE;
float barPos = 0.5;
bool moveBar = false;
int oldTimeSinceStart;
int deltaTime;
float mouse_last_x = 0;
float mouse_last_y = 0;
bool firstMouse = true;

float frameVertices[] = {
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

class Model {
public:
	struct Mesh {
		GLuint vao;
		unsigned int indicesCount;
		unsigned int materialID;
	};
	struct Texture {
		GLuint textureID;
	};
	unordered_map<int, Texture> diffuse;
private:

	string directory;
	Mesh processMesh(const aiMesh *mesh, const aiScene *scene) {
		GLuint vao, vbo, ebo;
		// create buffers/arrays
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);

		// load data into vertex buffers
		int vertices_size = sizeof(float) * 8 * mesh->mNumVertices;
		float *vertices = (float*)malloc(vertices_size);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D vert = mesh->mVertices[i];
			aiVector3D norm = mesh->mNormals[i];
			aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0);

			//verts
			vertices[i * 8] = vert.x;
			vertices[i * 8 + 1] = vert.y;
			vertices[i * 8 + 2] = vert.z;

			//normals
			vertices[i * 8 + 3] = norm.x;
			vertices[i * 8 + 4] = norm.y;
			vertices[i * 8 + 5] = norm.z;

			//uvs
			vertices[i * 8 + 6] = uv.x;
			vertices[i * 8 + 7] = uv.y;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
		free(vertices);

		int indices_size = sizeof(int) * 3 * mesh->mNumFaces;
		unsigned int *indices = (unsigned int*)malloc(indices_size);

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			assert(face.mNumIndices == 3);
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices[i * 3 + j] = face.mIndices[j];
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);
		free(indices);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
		// vertex texture coords
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));
		// vertex normals
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));

		glBindVertexArray(0);
		cout << "mesh loaded: " << mesh->mName.C_Str() << endl;
		return Mesh{ vao, 3 * mesh->mNumFaces, mesh->mMaterialIndex };
	}
	void processNode(aiNode *node, const aiScene *scene) {
		// process all the node's meshes (if any)
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

			meshes.push_back(processMesh(mesh, scene));
		}
		// then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}
	void processMaterial(const aiScene *scene) {
		cout << "Material count: " << scene->mNumMaterials << endl;
		for (int i = 0; i < scene->mNumMaterials; i++)
		{

			cout << "Material " << i << " Diffuse " << scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) << " height " << scene->mMaterials[i]->GetTextureCount(aiTextureType_HEIGHT) << endl;
			if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString str;
				scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &str);
				diffuse[i] = loadTexture(str.C_Str());
			}
		}
	}
	Texture loadTexture(string const &pFile) {
		GLuint textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = stbi_load((directory + '/' + pFile).c_str(), &width, &height, &nrComponents, 4);
		if (data)
		{
			glActiveTexture(GL_TEXTURE0 + textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			cout << "Texture loaded: " << pFile << endl;
			stbi_image_free(data);
		}
		else
		{
			cout << "Texture failed to load at path: " << pFile << endl;
			stbi_image_free(data);
		}
		return Texture{ textureID };
	}

public:
	vector<Mesh> meshes;
	Model(string const &pFile) {
		const struct aiScene* scene = aiImportFile(pFile.c_str(),
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GenNormals | 
			aiProcess_FlipUVs |
			aiProcess_GenUVCoords|
			aiProcess_SortByPType
		);

		// If the import failed, report it
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			//DoTheErrorLogging(aiGetErrorString());
			cout << "ERROR ASSIMP " << aiGetErrorString() << endl;
		}
		
		directory = pFile.substr(0, pFile.find_last_of('/'));
		
		// Now we can access the file's contents
		//DoTheSceneProcessing(scene);
		processNode(scene->mRootNode, scene);

		processMaterial(scene);

		// We're done. Release all resources associated with this import
		aiReleaseImport(scene);
	}
};
char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}
GLint loadShader(string const &vertexPath, string const &fragmentPath) {
	// Create Shader Program
	GLint program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource(vertexPath.c_str());
	char** fragmentShaderSource = loadShaderSource(fragmentPath.c_str());

	// Assign content of these shader files to those shaders we created before
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
	shaderLog(fragmentShader);

	// Assign the program we created before with these shaders
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	return program;
}
void My_Init()
{
	glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = loadShader("vertex.vs.glsl", "fragment.fs.glsl");
	blurProgram = loadShader("blur_vertex.vs.glsl", "blur_fragment.fs.glsl");
	screenProgram = loadShader("screen_vertex.vs.glsl", "screen_fragment.fs.glsl");

	sponza = new Model("sponza/sponza.obj");

	// setup frame buffer vao & vbo
	glGenVertexArrays(1, &frameVAO);
	glGenBuffers(1, &frameVBO);
	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), &frameVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

	// setup frame buffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	
	glGenTextures(2, textureColorBuffer);
	for (unsigned int i = 0; i < 2; i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureColorBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, textureColorBuffer[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, textureColorBuffer[i], 0);
	}

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR FRAMEBUFFER is not complete!" << endl;

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	// setup program
	glUseProgram(program);
	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	utex = glGetUniformLocation(program, "utex");

	// ping-pong-framebuffer for blurring
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(3, pingpongColorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glActiveTexture(GL_TEXTURE0 + pingpongColorBuffers[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
		// also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}

	glActiveTexture(GL_TEXTURE0 + pingpongColorBuffers[2]);
	glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// setup blur program
	glUseProgram(blurProgram);
	
	
	// setup screen program
	glUseProgram(screenProgram);
	glUniform1i(glGetUniformLocation(screenProgram, "colorTexture"), textureColorBuffer[0]);
	glUniform1i(glGetUniformLocation(screenProgram, "normalTexture"), textureColorBuffer[1]);
	glUniform1i(glGetUniformLocation(screenProgram, "weakBlurTexture"), pingpongColorBuffers[2]);
	glUniform1f(glGetUniformLocation(screenProgram, "ubarPos"), barPos);
	uTime = glGetUniformLocation(screenProgram, "uTime");
	uMode = glGetUniformLocation(screenProgram, "uMode"); 
	uMouse = glGetUniformLocation(screenProgram, "uMouse");

	glUseProgram(0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void My_Display()
{

	// Draw to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glEnable(GL_DEPTH_TEST);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	
	mv = lookAt(pos, pos + frontVec, vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(mv));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(p));

	// Draw meshes
	for (int i = 0; i < sponza->meshes.size(); i++)
	{
		auto diffuseTexIter = sponza->diffuse.find(sponza->meshes[i].materialID);
		if (diffuseTexIter != sponza->diffuse.end()) {
			glUniform1i(utex, diffuseTexIter->second.textureID);
		} else {
			glUniform1i(utex, 0);
		}
		glBindVertexArray(sponza->meshes[i].vao);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(sponza->meshes[i].indicesCount), GL_UNSIGNED_INT, 0);
	}

	// calculate bloom
	bool horizontal = true, first_iteration = true;
	unsigned int amount = 16;
	glUseProgram(blurProgram);
	for (unsigned int i = 0; i < amount; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
		glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal);
		glUniform1i(glGetUniformLocation(blurProgram, "image"), first_iteration ? textureColorBuffer[0] : pingpongColorBuffers[!horizontal]);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
		if (i == 3) {
			glCopyTextureSubImage2D(pingpongColorBuffers[2], 0, 0, 0, 0, 0, WIDTH, HEIGHT);
		}
	}

	// draw to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST); 
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(screenProgram);
	glUniform1i(uTime, glutGet(GLUT_ELAPSED_TIME));
	glUniform1i(glGetUniformLocation(screenProgram, "blurTexture"), pingpongColorBuffers[!horizontal]);
	glBindVertexArray(frameVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUseProgram(0);
	glBindVertexArray(0);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	WIDTH = width;
	HEIGHT = height;
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	//vp = perspective(radians(80.0f), viewportAspect, 0.1f, 1000.0f) * lookAt(vec3(5.0f, 20.0f, 0.0f), vec3(-0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	p = perspective(radians(45.0f), viewportAspect, 0.1f, 1000.0f);

	glUseProgram(screenProgram);
	glUniform2i(glGetUniformLocation(screenProgram, "uResolution"), WIDTH, HEIGHT);

	glActiveTexture(GL_TEXTURE0 + textureColorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, textureColorBuffer[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glActiveTexture(GL_TEXTURE0 + textureColorBuffer[1]);
	glBindTexture(GL_TEXTURE_2D, textureColorBuffer[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glActiveTexture(GL_TEXTURE0 + pingpongColorBuffers[0]);
	glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glActiveTexture(GL_TEXTURE0 + pingpongColorBuffers[1]);
	glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glActiveTexture(GL_TEXTURE0 + pingpongColorBuffers[2]);
	glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void My_Timer(int val)
{
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = timeSinceStart - oldTimeSinceStart;
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		firstMouse = true;
		if (mode != 0 && abs(WIDTH * barPos - x) < 3) {
			moveBar = true;
		}
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if (state == GLUT_UP)
	{
		moveBar = false;
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	float delta = float(4e-1);
	float oldY = pos.y;
	if (key == 'w') {
		pos.y = oldY;
		pos += frontVec * delta;
	}
	if (key == 's') {
		pos.y = oldY;
		pos -= frontVec * delta;
	}
	if (key == 'a') {
		pos.y = oldY;
		pos -= rightVec * delta;
	}
	if (key == 'd') {
		pos += rightVec * delta;
		pos.y = oldY;
		}
	if (key == 'z')
		pos += upVec * delta;
	if (key == 'x')
		pos -= upVec * delta;
	
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}
void updateCameraVectors() {
	glm::vec3 _front;
	_front.x = cos(radians(yawF)) * cos(radians(pitchF));
	_front.y = sin(radians(pitchF));
	_front.z = sin(radians(yawF)) * cos(radians(pitchF));
	frontVec = normalize(_front);

	rightVec = normalize(cross(frontVec, upVec));
	upVec = normalize(cross(rightVec, frontVec));
}
void My_MotionMouse(int x, int y) {
	if (mode != 0 && abs(WIDTH * barPos - x) < 3) {
		glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
	}
	else {
		glutSetCursor(GLUT_CURSOR_INHERIT);
	}
	if (moveBar) {
		barPos = float(x) / WIDTH;
		glUseProgram(screenProgram);
		glUniform1f(glGetUniformLocation(screenProgram, "ubarPos"), barPos);
		glUseProgram(0);
	}
	else {
		if (firstMouse)
		{
			mouse_last_x = x;
			mouse_last_y = y;
			firstMouse = false;
		}

		float x_offset = x - mouse_last_x;
		float y_offset = mouse_last_y - y;

		mouse_last_x = x;
		mouse_last_y = y;


		float mouseSensitivity = 0.1;
		yawF += x_offset * mouseSensitivity;
		pitchF += y_offset * mouseSensitivity;

		if (pitchF > 89.0f)
			pitchF = 89.0f;
		if (pitchF < -89.0f)
			pitchF = -89.0f;

		updateCameraVectors();
	}
	glUseProgram(screenProgram);
	glUniform2f(uMouse, float(x) / WIDTH, 1.0 - float(y) / HEIGHT);
	glUseProgram(0);
}
void My_PassiveMouse(int x, int y) {
	if (mode != 0 && abs(WIDTH * barPos - x) < 3) {
		glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
	} else {
		glutSetCursor(GLUT_CURSOR_INHERIT);
	}
	glUseProgram(screenProgram);
	glUniform2f(uMouse, float(x)/WIDTH, 1.0 - float(y)/HEIGHT);
	glUseProgram(0);
}

void My_Menu(int id)
{
	cout << "Menu: " << id << endl;
	mode = static_cast<Menu>(id);
	glUseProgram(screenProgram);
	glUniform1i(uMode, mode);
	glUseProgram(0);
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddMenuEntry("Diffuse Texture", Menu::TEXTURE);
	glutAddMenuEntry("Normal Color", Menu::NORMAL);
	glutAddMenuEntry("Image abstraction", Menu::IMAGE_ABSTRACTION);
	glutAddMenuEntry("Watercolor", Menu::WATER_COLOR);
	glutAddMenuEntry("Magnifier", Menu::MAGNIFIER);
	glutAddMenuEntry("Bloom effect", Menu::BLOOM);
	glutAddMenuEntry("Pixelization", Menu::PIXELIZATION);
	glutAddMenuEntry("Sine wave distortion", Menu::SINE_DISTORTION);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);
	glutMotionFunc(My_MotionMouse);
	glutPassiveMotionFunc(My_PassiveMouse);

	// Enter main event loop.
	glutMainLoop();

	delete sponza;
	return 0;
}
