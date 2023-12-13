#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mesh.h"
#include "array.h"

mesh_t mesh = {
    .vertices = NULL,
    .faces = NULL,
    .rotation = {.x = 0, .y = 0, .z = 0},
    .scale = {.x = 1.0, .y = 1.0, .z = 1.0},
    .translation = {.x = 0, .y = 0, .z = 0},
};

vec3_t cube_vertices[N_CUBE_VERTICES] = {
    {.x = -1, .y = -1, .z = -1}, // 1
    {.x = -1, .y = 1, .z = -1},  // 2
    {.x = 1, .y = 1, .z = -1},   // 3
    {.x = 1, .y = -1, .z = -1},  // 4
    {.x = 1, .y = 1, .z = 1},    // 5
    {.x = 1, .y = -1, .z = 1},   // 6
    {.x = -1, .y = 1, .z = 1},   // 7
    {.x = -1, .y = -1, .z = 1},  // 8
};

face_t cube_faces[N_CUBE_FACES] = {
    // front
    {.a = 1, .b = 2, .c = 3, .color = 0xFFFF0000},
    {.a = 1, .b = 3, .c = 4, .color = 0xFFFF0000},
    // right
    {.a = 4, .b = 3, .c = 5, .color = 0xFF00FF00},
    {.a = 4, .b = 5, .c = 6, .color = 0xFF00FF00},
    // back
    {.a = 6, .b = 5, .c = 7, .color = 0xFF0000FF},
    {.a = 6, .b = 7, .c = 8, .color = 0xFF0000FF},
    // left
    {.a = 8, .b = 7, .c = 2, .color = 0xFFFFFF00},
    {.a = 8, .b = 2, .c = 1, .color = 0xFFFFFF00},
    // top
    {.a = 2, .b = 7, .c = 5, .color = 0xFFFF00FF},
    {.a = 2, .b = 5, .c = 3, .color = 0xFFFF00FF},
    // bottom
    {.a = 6, .b = 8, .c = 1, .color = 0xFF00FFFF},
    {.a = 6, .b = 1, .c = 4, .color = 0xFF00FFFF},
};

void load_cube_mesh_data(void)
{
    for (int i = 0; i < N_CUBE_VERTICES; i++)
    {
        vec3_t cube_vertex = cube_vertices[i];
        array_push(mesh.vertices, cube_vertex);
    }

    for (int i = 0; i < N_CUBE_FACES; i++)
    {
        face_t cube_face = cube_faces[i];
        array_push(mesh.faces, cube_face);
    }
}

void load_obj_file_data(char *filename)
{
    FILE *file;
    char *line = NULL;
    size_t bufsize = 0; // Initial buffer size

    // Open the file in read mode
    file = fopen(filename, "r");

    // Check if the file was opened successfully
    if (file == NULL)
    {
        fprintf(stderr, "Error opening OBJ file.\n");
        return;
    }

    // Read the file line by line
    while (getline(&line, &bufsize, file) != -1)
    {
        // Process each line
        if (line[0] == 'v' && line[1] == ' ')
        {
            vec3_t vertex;
            sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
            array_push(mesh.vertices, vertex);
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            int vertex_indices[3];
            int texture_indices[3];
            int normal_indices[3];
            sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                   &vertex_indices[0], &texture_indices[0], &normal_indices[0],
                   &vertex_indices[1], &texture_indices[1], &normal_indices[1],
                   &vertex_indices[2], &texture_indices[2], &normal_indices[2]);

            face_t face = {.a = vertex_indices[0], .b = vertex_indices[1], .c = vertex_indices[2]};
            array_push(mesh.faces, face);
        }
    }

    // Close the file
    fclose(file);

    // Free the dynamically allocated memory
    free(line);
}
