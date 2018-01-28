// Find the scene in the script
int scene_id = 0;
int start_time = 0;
while (start_time + (int)(script_duration_[scene_id]) * kSceneTic < itime) {
    start_time += (int)(script_duration_[scene_id]) * kSceneTic;
    scene_id++;
}
int sitime = itime - start_time;

glClear(GL_COLOR_BUFFER_BIT);

// Set the render matrix
float parameterMatrix[4][4];

// Set parameters to other locations, using seed stuff
unsigned int start_seed = script_seed_[scene_id];
unsigned int seed = start_seed;
for (int i = 1; i < 16; i++) {
    parameterMatrix[0][i] = jo_frand(&seed);
}

parameterMatrix[0][0] = sitime * (1.0f / 32768.0f);
parameterMatrix[2][2] += (sitime) * (1.0f / 1048576.0f) * script_move_[scene_id];

glUniformMatrix4fv(0, 1, GL_FALSE, &(parameterMatrix[0][0]));
// render
glEnable(GL_BLEND);
glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);
