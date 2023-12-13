#include <stdbool.h>
#include "display.h"
#include "vector.h"
#include "matrix.h"
#include "mesh.h"
#include "array.h"

#define PI 3.14159265358979323846264338327950288
// Array of triangles that should be renderer frame by frame
triangle_t *triangles_to_render = NULL;

// Global variables
bool is_running = false;
int previous_frame_rate = 0;

vec3_t camera_position = {.x = 0, .y = 0, .z = 0};
mat4_t proj_matrix;

void setup(void)
{
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

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

    // Initialize the perspective projection matrix
    float fov = PI / 3.0; // the same as 160/3 deg but in rad
    float aspect = (float)window_height / (float)window_width;
    float znear = 0.1;
    float zfar = 100.0;
    proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);

    // Load the mesh values in the data structure
    load_cube_mesh_data();
    // load_obj_file_data("./assets/cube.obj");
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
        if (event.key.keysym.sym == SDLK_1)
            render_method = RENDER_WIRE_VERTEX;
        if (event.key.keysym.sym == SDLK_2)
            render_method = RENDER_WIRE;
        if (event.key.keysym.sym == SDLK_3)
            render_method = RENDER_FILL_TRIANGLE;
        if (event.key.keysym.sym == SDLK_4)
            render_method = RENDER_FILL_TRIANGLE_WIRE;
        if (event.key.keysym.sym == SDLK_c)
            cull_method = CULL_BACKFACE;
        if (event.key.keysym.sym == SDLK_d)
            cull_method = CULL_NONE;

        break;
    }
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

float backface_culling(vec4_t *transformed_vertices)
{
    vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A  */
    vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \ */
    vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

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

    // Change the mesh scale/rotation values per animation frame
    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    mesh.rotation.z += 0.01;
    // mesh.scale.x += 0.002;
    // mesh.scale.y += 0.001;
    // mesh.translation.x += 0.01;
    mesh.translation.z = 5.0;

    // Create a scale matrix that will be used to multiply the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);

    // Create a translation matrix
    mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);

    // Create a rotation matrices
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

    // Create a world matrix combining scale, rotation, and translation matrices
    mat4_t world_matrix = mat4_identity();

    // Perform all matrices multiplications. Order matters ( FIrst scale, then rotate, then translate) [T]*[R]*[S]*v
    world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
    world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

    // Loop all triangle faces of our mesh
    for (int i = 0; i < array_length(mesh.faces); i++)
    {
        face_t mesh_face = mesh.faces[i];
        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1]; // -1 because arrays start in 0 in C
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        vec4_t transformed_vertices[3];

        // Loop all 3 vertices and apply transformations
        for (int j = 0; j < 3; j++)
        {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Multiply the world matrix by the original vector
            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            // Save transformed vertex
            transformed_vertices[j] = transformed_vertex;
        }

        // Check backface culling
        float camera_alignment = (cull_method == CULL_BACKFACE) ? backface_culling(transformed_vertices) : 1;

        if (camera_alignment > 0)
        {
            triangle_t projected_triangle;

            float sum_depth = 0;
            for (int j = 0; j < 3; j++)
            {
                // Projecting transformed vertex to 2D
                vec4_t projected_point = mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

                // Scale into the view
                projected_point.x *= window_width / 2;
                projected_point.y *= window_height / 2;

                // Translate the projected points to the middle of the screen
                projected_point.x += window_width / 2;
                projected_point.y += window_height / 2;

                // Add the projected point
                vec2_t point = {.x = projected_point.x, .y = projected_point.y};
                projected_triangle.points[j] = point;

                // Sum all z values
                sum_depth += transformed_vertices[j].z;
            }
            // Add the same color of the mesh to the projected triangle
            projected_triangle.color = mesh_face.color;

            // Calculate the average depth
            projected_triangle.avg_depth = sum_depth / 3.0f;

            // Save the projected triangle in the array of triangles to render
            array_push(triangles_to_render, projected_triangle);
        }

        // Sort triangles to render by their avg_depth
        int num_triangles = array_length(triangles_to_render);
        for (int i = 0; i < num_triangles; i++)
        {
            for (int j = i; j < num_triangles; j++)
            {
                if (triangles_to_render[i].avg_depth > triangles_to_render[j].avg_depth)
                {
                    // Swap the triangles positions in the array
                    triangle_t temp = triangles_to_render[i];
                    triangles_to_render[i] = triangles_to_render[j];
                    triangles_to_render[j] = temp;
                }
            }
        }
    }
}

void render(void)
{
    draw_grid(50);

    // Loop all projected triangles and render them
    for (int i = 0; i < array_length(triangles_to_render); i++)
    {
        triangle_t triangle = triangles_to_render[i];

        if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE)
        {
            // Draw filled triangle
            draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                 triangle.points[1].x, triangle.points[1].y,
                                 triangle.points[2].x, triangle.points[2].y,
                                 triangle.color);
        }

        if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE)
        {
            uint32_t line_color = 0XFF00FF00;

            // Draw unfilled triangle
            draw_triangle(
                triangle.points[0].x, triangle.points[0].y,
                triangle.points[1].x, triangle.points[1].y,
                triangle.points[2].x, triangle.points[2].y,
                line_color);
        }

        if (render_method == RENDER_WIRE_VERTEX)
        {
            uint32_t vertex_color = 0XFFFF0000;
            int vertex_size = 6;
            // Draw vertex points
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, vertex_size, vertex_size, vertex_color);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, vertex_size, vertex_size, vertex_color);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, vertex_size, vertex_size, vertex_color);
        }
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