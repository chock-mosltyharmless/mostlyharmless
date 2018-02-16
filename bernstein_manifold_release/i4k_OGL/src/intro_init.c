// Load the script
// Create and link shader and stuff:

GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
// compile sources:
const char *pt = vertexMainParticle;
glShaderSource(vMainParticle, 1, &pt, NULL);
glCompileShader(vMainParticle);
pt = geometryMainParticle;
glShaderSource(gMainParticle, 1, &pt, NULL);
glCompileShader(gMainParticle);
pt = fragmentMainParticle;
glShaderSource(fMainParticle, 1, &pt, NULL);
glCompileShader(fMainParticle);

#ifdef SHADER_DEBUG
// Check programs
int tmp, tmp2;
glGetShaderiv(vMainParticle, GL_COMPILE_STATUS, &tmp);
if (!tmp)
{
    glGetShaderInfoLog(vMainParticle, 4096, &tmp2, err);
    err[tmp2] = 0;
    MessageBox(hWnd, err, "vMainParticle shader error", MB_OK);
    return;
}
glGetShaderiv(vHandParticle, GL_COMPILE_STATUS, &tmp);
if (!tmp)
{
    glGetShaderInfoLog(vHandParticle, 4096, &tmp2, err);
    err[tmp2] = 0;
    MessageBox(hWnd, err, "vHandParticle shader error", MB_OK);
    return;
}
glGetShaderiv(gMainParticle, GL_COMPILE_STATUS, &tmp);
if (!tmp)
{
    glGetShaderInfoLog(gMainParticle, 4096, &tmp2, err);
    err[tmp2] = 0;
    MessageBox(hWnd, err, "gMainParticle shader error", MB_OK);
    return;
}
glGetShaderiv(fMainParticle, GL_COMPILE_STATUS, &tmp);
if (!tmp)
{
    glGetShaderInfoLog(fMainParticle, 4096, &tmp2, err);
    err[tmp2] = 0;
    MessageBox(hWnd, err, "fMainParticle shader error", MB_OK);
    return;
}
#endif

// link shaders:
GLuint shaderProgram;
shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vMainParticle);
glAttachShader(shaderProgram, gMainParticle);
glAttachShader(shaderProgram, fMainParticle);
glLinkProgram(shaderProgram);
glUseProgram(shaderProgram);

#ifdef SHADER_DEBUG
int programStatus;
glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
if (programStatus == GL_FALSE)
{
    MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
    return;
}
glGetProgramiv(shaderPrograms[1], GL_LINK_STATUS, &programStatus);
if (programStatus == GL_FALSE)
{
    MessageBox(hWnd, "Could not link program", "Shader 1 error", MB_OK);
    return;
}
#endif

// Set vertex location
int vertex_id = 0;
float pz = 1.0f;
for (int z = 0; z < NUM_PARTICLES_PER_DIM; z++) {
    float py = 1.0f;
    for (int y = 0; y < NUM_PARTICLES_PER_DIM; y++) {
        float px = 1.0f;
        for (int x = 0; x < NUM_PARTICLES_PER_DIM; x++) {
            vertices_[vertex_id++] = px;
            vertices_[vertex_id++] = py;
            vertices_[vertex_id++] = pz;
            //colors_[color_id] = jo_frand(&seed);
            vertices_[vertex_id] = 0.5f + 0.5f * sinf((px * py * pz + pz) * (1 << 24));
            vertices_[vertex_id] = 1.0f - vertices_[vertex_id] * vertices_[vertex_id];
            vertex_id++;
            px -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
        }
        py -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
    }
    pz -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
}

//unsigned int seed = 23690984;
//jo_frand_seed_ = 0;
for (int z = 0; z < NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * 4; z++) {
    vertices_[z] += JoFrand() * (1.0f / 32.0f);
}

// Set up vertex buffer and stuff
int vboID;
glGenBuffers(1, &vboID); // Generate our Vertex Buffer Object  

glEnableVertexAttribArray(0);  // Vertex array position data
glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind our Vertex Buffer Object  
glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 4 * sizeof(GLfloat),
    vertices_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
glVertexAttribPointer(0, // attribute
    4, // size
    GL_FLOAT, // type
    GL_FALSE, // normalized?
    0, // stride
    (void*)0); // array buffer offset

#ifdef SHADER_DEBUG
               // Get all the errors:
GLenum errorValue = glGetError();
if (errorValue != GL_NO_ERROR)
{
    char *errorString = (char *)gluErrorString(errorValue);
    MessageBox(hWnd, errorString, "Init error", MB_OK);
    return;
}
#endif
