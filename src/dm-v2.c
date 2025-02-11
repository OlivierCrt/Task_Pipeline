#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "tasks.h"  

#define NUM_WORKERS 4
#define BUFFER_SIZE 100 



typedef enum {
    TASK_SIMULATE,
    TASK_GEN_IMAGE,
    TASK_GAUSS_BLUR,
    TASK_CONVERT_GRAY,
    TASK_COMPUTE_STATS,
    TASK_SAVE_STATS,
    TASK_SAVE_IMG,
    TASK_EXIT     
} task_e;

typedef struct {
    task_e type;
    int step;  
} task_t;

typedef struct {
    int nb_steps;
    int save_img;
    const char *png_filename_format;
    const char *stats_filename;
    struct Image **img1;               
    struct Image **img2;     
    struct Body (*tabBodies)[N_BODIES];            
    struct ImageStats *stats;           
} wargs_t;

typedef struct {
    task_t tasks[BUFFER_SIZE];
    int in;     
    int out;    
    int count;  
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} task_buffer_t;

task_buffer_t task_buffer;

void init_buffer(task_buffer_t *buf) {
    buf->in = buf->out = buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
}



void pop(task_buffer_t *buf, task_t *task) {
    pthread_mutex_lock(&buf->mutex);
    while (buf->count == 0)
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    *task = buf->tasks[buf->out];
    buf->out = (buf->out + 1) % BUFFER_SIZE;
    buf->count--;
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->mutex);
}

void task_p(task_buffer_t *buf, task_t task) {
    pthread_mutex_lock(&buf->mutex);
    while (buf->count == BUFFER_SIZE)
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    buf->tasks[buf->in] = task;
    buf->in = (buf->in + 1) % BUFFER_SIZE;
    buf->count++;
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->mutex);
}

int tasks_executed = 0; 


int expected_tasks = 0;   

pthread_mutex_t exec_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t exec_cond = PTHREAD_COND_INITIALIZER;


// taches
void task_executed() {
    pthread_mutex_lock(&exec_mutex);
    tasks_executed++;
    if (tasks_executed == expected_tasks)
        pthread_cond_signal(&exec_cond);
    pthread_mutex_unlock(&exec_mutex);
}


void execute_task(task_t t, wargs_t *w_args) {


    int nb_steps = w_args->nb_steps;
    switch (t.type) {
        case TASK_SIMULATE:
            if (t.step == 0) {
                simulate_n_bodies(w_args->tabBodies[0], N_BODIES, 1.0);
            } else {
                for (int j = 0; j < N_BODIES; j++) {
                    w_args->tabBodies[t.step][j] = w_args->tabBodies[t.step - 1][j];
                }
                simulate_n_bodies(w_args->tabBodies[t.step], N_BODIES, 1.0);
            }
            if (t.step < nb_steps - 1) {
                task_t next_sim;
                next_sim.type = TASK_SIMULATE;
                next_sim.step = t.step + 1;
                task_p(&task_buffer, next_sim);
            }
            {
                task_t gen;
                gen.type = TASK_GEN_IMAGE;
                gen.step = t.step;
                task_p(&task_buffer, gen);
            }
            break;
        case TASK_GEN_IMAGE:
            generate_image_from_bodies(w_args->tabBodies[t.step], N_BODIES, w_args->img1[t.step]);
            {
                task_t blur;
                blur.type = TASK_GAUSS_BLUR;
                blur.step = t.step;
                task_p(&task_buffer, blur);
            }
            break;
        case TASK_GAUSS_BLUR:
            apply_gaussian_blur(w_args->img1[t.step], w_args->img2[t.step]);
            if (w_args->save_img) {
                task_t save;
                save.type = TASK_SAVE_IMG;
                save.step = t.step;
                task_p(&task_buffer, save);
            }
            {
                task_t voo;
                voo.type = TASK_CONVERT_GRAY;
                voo.step = t.step;
                task_p(&task_buffer, voo);
            }
            break;
        case TASK_SAVE_IMG:
            save_img_as_png(w_args->img2[t.step], w_args->png_filename_format, t.step);
            break;
        case TASK_CONVERT_GRAY:
            convert_to_grayscale(w_args->img2[t.step], w_args->img1[t.step]);
            {
                task_t temp;
                temp.type = TASK_COMPUTE_STATS;
                temp.step = t.step;
                task_p(&task_buffer, temp);
            }
            break;
        case TASK_COMPUTE_STATS:
            compute_image_statistics(w_args->img1[t.step], &w_args->stats[t.step]);
            {
                task_t save_stats_task;
                save_stats_task.type = TASK_SAVE_STATS;
                save_stats_task.step = t.step;
                task_p(&task_buffer, save_stats_task);
            }
            break;
        case TASK_SAVE_STATS:
            save_stats(&w_args->stats[t.step], w_args->stats_filename, t.step);
            break;
        case TASK_EXIT:
            break;
        default:
            fprintf(stderr, "Tâche inconnue\n");
            exit(EXIT_FAILURE);
    }
    task_executed();
}


void *worker_func(void *arg) {
    wargs_t *w_args = (wargs_t *) arg;
    while (1) {
        task_t t;
        pop(&task_buffer, &t);
        if (t.type == TASK_EXIT)
            break;
        execute_task(t, w_args);
    }
    return NULL;
}



void freeAll_resources(wargs_t *w_args) {
    for (int i = 0; i < w_args->nb_steps; i++) {
        free_img(w_args->img1[i]);
        free_img(w_args->img2[i]);
    }
    free(w_args->tabBodies);
    free(w_args->img1);
    free(w_args->img2);
    free(w_args->stats);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <nb-steps> <img-width> <img-height> <save-img>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int nb_steps = atoi(argv[1]);
    int width    = atoi(argv[2]);
    int height   = atoi(argv[3]);
    int save_img = atoi(argv[4]);
    
    const char *stats_filename = "./img-stats_v2.csv";
    const char *png_filename_format = "./img%03d_v2.png";
    
    // Suppression
    remove(stats_filename);
    if (save_img) {
        char filename[256];
        for (int i = 0; i < nb_steps; i++) {
            snprintf(filename, sizeof(filename), png_filename_format, i);
            remove(filename);
        }
    }
    
    wargs_t w_args;
    w_args.nb_steps = nb_steps;
    w_args.save_img = save_img;
    w_args.png_filename_format = png_filename_format;
    w_args.stats_filename = stats_filename;
   
    w_args.tabBodies = malloc(nb_steps * sizeof(struct Body[N_BODIES]));
    if (w_args.tabBodies == NULL) {
        fprintf(stderr, "Erreur lors de l'allocation de tabBodies\n");
        exit(EXIT_FAILURE);
    }
    
    w_args.img1 = malloc(nb_steps * sizeof(struct Image *));
    w_args.img2 = malloc(nb_steps * sizeof(struct Image *));
    
    for (int i = 0; i < nb_steps; i++) {
        w_args.img1[i] = alloc_img(width, height);
        if (w_args.img1[i] == NULL) {
            fprintf(stderr, "Erreur allocation de img1[%d]\n", i);
            exit(EXIT_FAILURE);
        }
        w_args.img2[i] = alloc_img(width, height);
        if (w_args.img2[i] == NULL) {
            fprintf(stderr, "Erreur allocation de img2[%d]\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    w_args.stats = malloc(nb_steps * sizeof(struct ImageStats));
    if (w_args.stats == NULL) {
        fprintf(stderr, "Erreur allocation de stats\n");
        exit(EXIT_FAILURE);
    }
    
    struct Body bodies[N_BODIES] = {
        { 0.00, 0.0,  0.000, 0.0,      1.0, 0.00465047,  5.0e2, 255, 204,  0},
        { 0.39, 0.0,  0.323, 0.0, 1.65e-7,  1.765e-5, 10.0e3, 169, 169,169},
        { 0.72, 0.0,  0.218, 0.0, 2.45e-6,  4.552e-5, 10.0e3, 255, 204,153},
        { 1.00, 0.0,  0.170, 0.0, 3.00e-6,  4.258e-5, 10.0e3,   0, 102,204},
        { 1.52, 0.0,  0.128, 0.0, 3.21e-7,  2.279e-5, 10.0e3, 255, 102,  0},
        { 5.20, 0.0,  0.060, 0.0, 9.55e-4,  4.7789e-4,  5.0e3, 204, 153,102},
        { 9.58, 0.0,  0.043, 0.0, 2.86e-4,  4.0072e-4,  5.0e3, 210, 180,140},
        {19.22, 0.0,  0.030, 0.0, 4.36e-5,  1.6938e-4,  5.0e3, 173, 216,230},
        {30.05, 0.0,  0.024, 0.0, 5.17e-5,  1.6418e-4,  5.0e3,   0,   0,128},
    };
    srand(1);
    for (int i = 1; i < N_BODIES; i++) {
        double dist = bodies[i].x;
        double angle = 2 * M_PI * (double)rand() / RAND_MAX;
        bodies[i].x = dist * cos(angle);
        bodies[i].y = dist * sin(angle);
        double rot_speed = bodies[i].vx;
        bodies[i].vx = -rot_speed * bodies[i].y;
        bodies[i].vy =  rot_speed * bodies[i].x;
    }
    for (int i = 0; i < N_BODIES; i++) {
        w_args.tabBodies[0][i] = bodies[i];
    }
    
    if (w_args.save_img)
        expected_tasks = nb_steps * 7;
    else
        expected_tasks = nb_steps * 6;
    
    init_buffer(&task_buffer);
    
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pthread_create(&workers[i], NULL, worker_func, (void *)&w_args) != 0) {
            fprintf(stderr, "Erreur lors de la création du thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    struct timespec t0_time, t1_time;
    if (clock_gettime(CLOCK_BOOTTIME, &t0_time) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    
    task_t init_task;
    init_task.type = TASK_SIMULATE;
    init_task.step = 0;
    task_p(&task_buffer, init_task);
    
    pthread_mutex_lock(&exec_mutex);
    while (tasks_executed < expected_tasks)
        pthread_cond_wait(&exec_cond, &exec_mutex);
    pthread_mutex_unlock(&exec_mutex);
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        task_t exit_task;
        exit_task.type = TASK_EXIT;
        exit_task.step = 0; 
        task_p(&task_buffer, exit_task);
    }
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }
    
    if (clock_gettime(CLOCK_BOOTTIME, &t1_time) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    int64_t total_ns = ns_diff(&t0_time, &t1_time);
    print_elapsed_time_stats(total_ns);
    
    freeAll_resources(&w_args);
    
    return 0;
}