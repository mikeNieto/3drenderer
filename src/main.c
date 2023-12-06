#include <stdbool.h>
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "array.h"

// Array of triangles that should be renderer frame by frame
triangle_t *triangles_to_render = NULL;

// Global variables
vec3_t camera_position = {.x = 0, .y = 0, .z = 0};

float fov_factor = 640.0; // Field of view
int previous_frame_rate = 0;
bool is_running = false;

void setup(void)
{
    // Allocating the required memory in bytes to hold the color buffer
    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    // Check if the memory was allocated
    if (!color_buffer)
    {
        is_running = false;
        return;
    }

    // Creating a SDL texture that is used to display the color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    // Load the mesh values in the data structure
    load_obj_file_data("./assets/cube.obj");
}

void process_input(void)
{
    // Check if there is an input form the user
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type)
    {
    case SDL_QUIT:
        is_running = false;
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE)
            is_running = false;
        break;
    }
}

// Function to project 3D points to 2D
vec2_t project(vec3_t point)
{

    vec2_t projected_point = {
        .x = (fov_factor * point.x) / point.z,
        .y = (fov_factor * point.y) / point.z};
    return projected_point;
}

void fix_frame_rate()
{
    int time_to_wait = previous_frame_rate + FRAME_TARGET_TIME - SDL_GetTicks();
    // Only delay if we are running to fast
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }
    previous_frame_rate = SDL_GetTicks();
}

float backface_culling(vec3_t *transformed_vertices)
{
    vec3_t vector_a = transformed_vertices[0]; /*   A  */
    vec3_t vector_b = transformed_vertices[1]; /*  / \ */
    vec3_t vector_c = transformed_vertices[2]; /* C---B */

    // Get the vector substraction B-A and C-A
    vec3_t vect_ab = vec3_sub(vector_b, vector_a);
    vec3_t vect_ac = vec3_sub(vector_c, vector_a);

    // Normalize vect_ab and vect_ac
    vec3_normalize(&vect_ab);
    vec3_normalize(&vect_ac);

    // Compute the face norma using cross product (left hand coordinates)
    vec3_t normal = vec3_cross(vect_ab, vect_ac);

    // normalize the face normal vector
    vec3_normalize(&normal);

    // Find a vector between a point in the triangle and the camera origin
    vec3_t camera_ray = vec3_sub(camera_position, vector_a);

    // Calculate how aligned the camera ray is with the face normal
    return vec3_dot(normal, camera_ray);
}

void update(void)
{
    fix_frame_rate();

    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    mesh.rotation.z += 0.01;

    // Loop all triangle faces of our mesh
    for (int i = 0; i < array_length(mesh.faces); i++)
    {
        face_t mesh_face = mesh.faces[i];
        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1]; // -1 because arrays start in 0 in C
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        triangle_t projected_triangle;
        vec3_t transformed_vertices[3];

        // Loop all 3 vertices and apply transformations
        for (int j = 0; j < 3; j++)
        {
            vec3_t transformed_vertex = face_vertices[j];

            transformed_vertex = vec3_rotate_x(transformed_vertex, mesh.rotation.x);
            transformed_vertex = vec3_rotate_y(transformed_vertex, mesh.rotation.y);
            transformed_vertex = vec3_rotate_z(transformed_vertex, mesh.rotation.z);

            // Translate the vertex away from the camera
            transformed_vertex.z += -5; // TODO: Change this is future

            // Save transformed vertex
            transformed_vertices[j] = transformed_vertex;
        }

        // Check backface culling
        float camera_alignment = backface_culling(transformed_vertices);

        if (camera_alignment >= 0)
        {
            for (int j = 0; j < 3; j++)
            {
                // Projecting transformed vertex to 2D
                vec2_t projected_point = project(transformed_vertices[j]);

                // Scale and translate the projected points to the middle of the screen
                projected_point.x += window_width / 2;
                projected_point.y += window_height / 2;

                // Add the projected point
                projected_triangle.points[j] = projected_point;
            }

            // Save the projected triangle in the array of triangles to render
            array_push(triangles_to_render, projected_triangle);
        }
    }
}

void render(void)
{
    draw_grid(50);

    uint32_t vertex_color = 0XFF00FFFF;
    uint32_t line_color = 0XFF0000FF;
    int vertex_size = 2;

    // Loop all projected triangles and render them
    for (int i = 0; i < array_length(triangles_to_render); i++)
    {
        triangle_t triangle = triangles_to_render[i];

        // Draw vertex points
        draw_rect(triangle.points[0].x, triangle.points[0].y, vertex_size, vertex_size, vertex_color);
        draw_rect(triangle.points[1].x, triangle.points[1].y, vertex_size, vertex_size, vertex_color);
        draw_rect(triangle.points[2].x, triangle.points[2].y, vertex_size, vertex_size, vertex_color);

        // Draw unfilled triangle
        draw_triangle(
            triangle.points[0].x, triangle.points[0].y,
            triangle.points[1].x, triangle.points[1].y,
            triangle.points[2].x, triangle.points[2].y,
            line_color);
    }
    
    // Clear the array of triangles every frame
    array_free(triangles_to_render);
    triangles_to_render = NULL;

    render_color_buffer();
    clear_color_buffer(0xFF000000);
    SDL_RenderPresent(renderer);
}

void free_resources(void)
{
    free(color_buffer);
    color_buffer = NULL;

    array_free(mesh.vertices);
    mesh.vertices = NULL;

    array_free(mesh.faces);
    mesh.faces = NULL;
}

int main(void)
{
    is_running = initialize_window();
    setup();

    while (is_running)
    {
        process_input();
        update();
        render();
    }

    destroy_window();
    free_resources();

    return 0;
}