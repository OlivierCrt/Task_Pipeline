#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "tasks.h"

// Fonction pour libérer la mémoire allouée dynamiquement
void libe(struct Body (*tabBodies)[N_BODIES], struct Image **img1, struct Image **img2, struct ImageStats* stats, int nb_steps){
  for (int i=0; i<nb_steps; ++i){
    free_img(img1[i]);  // Libérer l'image 1
    img1[i] = NULL;
    free_img(img2[i]);  // Libérer l'image 2
    img2[i] = NULL;
  }
  free(tabBodies);  // Libérer le tableau de corps
  free(img1);       // Libérer le tableau d'images 1
  free(img2);       // Libérer le tableau d'images 2
  free(stats);      // Libérer les statistiques d'images
}

// Structure pour les arguments de la fonction simulate_bodies
struct args_simulate_bodies{
     struct Body (*bodies)[N_BODIES];
    int nb_steps;
};

// Structure pour les arguments de la fonction generate_image_from_bodies
struct args_generate_image_from_bodies{
   struct Body (*bodies)[N_BODIES];
  struct Image **img;
  int nb_steps;
};

// Structure pour les arguments des fonctions nécessitant deux images
struct args_2_images{
  struct Image **img1;
  struct Image **img2;
  int nb_steps;
};

// Structure pour les arguments de la fonction save_img_as_png
struct args_save_img_as_png{
  struct Image **img;
  const char* png_file_format;
  int nb_steps;
};

// Structure pour les arguments de la fonction compute_image_statistics
struct args_compute_image_statistics{
  struct Image **img;
  struct ImageStats *stats;
  int nb_steps;
};

// Structure pour les arguments de la fonction save_stats
struct args_save_stats{
  struct ImageStats *stats;
  const char * stats_filename;
  int nb_steps;
};

// Fonction pour simuler les corps
void* func_simulate_bodies(void* p){
 struct  args_simulate_bodies* args=(struct  args_simulate_bodies*) p;
 simulate_n_bodies(args->bodies[0], N_BODIES, 1.0);

  for (int current_step_simulate = 1; current_step_simulate < args->nb_steps; ++current_step_simulate) {
    for (int j = 0; j < N_BODIES; ++j) {
            args->bodies[current_step_simulate][j] = args->bodies[current_step_simulate - 1][j]; // Copier element par element
        }
    simulate_n_bodies(args->bodies[current_step_simulate], N_BODIES, 1.0);
  }
  return NULL;
}

// Fonction pour générer des images à partir des corps
void* func_generate_image_from_bodies(void* p){
 struct  args_generate_image_from_bodies* args=(struct  args_generate_image_from_bodies*) p;

  for (int current_step_generate = 0; current_step_generate < args->nb_steps; ++current_step_generate) {
    generate_image_from_bodies(args->bodies[current_step_generate], N_BODIES, args->img[current_step_generate]);
  }
  return NULL;
}

// Fonction pour appliquer un flou gaussien aux images
void* func_apply_gaussian_blur(void* p){
  struct  args_2_images* args=(struct  args_2_images*) p;

  for (int current_step_blur = 0; current_step_blur < args->nb_steps; ++current_step_blur) {
    apply_gaussian_blur(args->img1[current_step_blur], args->img2[current_step_blur]);
  }
  return NULL;
}

// Fonction pour sauvegarder les images au format PNG
void* func_save_img_as_png(void* p){
  struct  args_save_img_as_png* args=(struct  args_save_img_as_png*) p;
  for (int current_step_save = 0; current_step_save < args->nb_steps; ++current_step_save) {
    save_img_as_png(args->img[current_step_save], args->png_file_format, current_step_save);
  }
  return NULL;
}

// Fonction pour convertir les images en niveaux de gris
void* func_convert_to_grayscale(void* p){
  struct  args_2_images* args=(struct  args_2_images*) p;
  for (int current_step_grayscale = 0; current_step_grayscale < args->nb_steps; ++current_step_grayscale) {
    convert_to_grayscale(args->img2[current_step_grayscale], args->img1[current_step_grayscale]);
  }
  return NULL;
}

// Fonction pour calculer les statistiques des images
void* func_compute_image_statistics(void* p){
  struct  args_compute_image_statistics* args=(struct  args_compute_image_statistics*) p;
  for (int current_step_compute_stats = 0; current_step_compute_stats < args->nb_steps; ++current_step_compute_stats) {
    compute_image_statistics(args->img[current_step_compute_stats], &args->stats[current_step_compute_stats]);
  }
  return NULL;
}

// Fonction pour sauvegarder les statistiques des images
void* func_save_stats(void* p){
  struct  args_save_stats* args=(struct  args_save_stats*) p;
  for (int current_step_save_stats = 0; current_step_save_stats < args->nb_steps; ++current_step_save_stats) {
    save_stats(&args->stats[current_step_save_stats], args->stats_filename, current_step_save_stats);
  }
  return NULL;
}

// Fonction principale
int main(int argc, char *argv[]) {
  if (argc < 1)
    exit(1);
  if (argc != 5) {
    fprintf(stderr, "usage: %s <nb-steps> <img-width> <img-height> <save-img>\n", argv[0]);
    exit(1);
  }

  int nb_steps = atoi(argv[1]);  // Nombre d'étapes
  int width = atoi(argv[2]);     // Largeur de l'image
  int height = atoi(argv[3]);    // Hauteur de l'image
  int save_img = atoi(argv[4]);  // Indicateur pour sauvegarder les images

  // Initialisation des corps
  struct Body bodies[N_BODIES] = {
    { 0.00, 0.0,  0.000, 0.0,      1.0, 0.00465047,  5.0e2,  255, 204,   0},
    { 0.39, 0.0,  0.323, 0.0,  1.65e-7,   1.765e-5, 10.0e3,  169, 169, 169},
    { 0.72, 0.0,  0.218, 0.0,  2.45e-6,   4.552e-5, 10.0e3,  255, 204, 153},
    { 1.00, 0.0,  0.170, 0.0,  3.00e-6,   4.258e-5, 10.0e3,    0, 102, 204},
    { 1.52, 0.0,  0.128, 0.0,  3.21e-7,   2.279e-5, 10.0e3,  255, 102,   0},
    { 5.20, 0.0,  0.060, 0.0,  9.55e-4,  4.7789e-4,  5.0e3,   204, 153, 102},
    { 9.58, 0.0,  0.043, 0.0,  2.86e-4,  4.0072e-4,  5.0e3,   210, 180, 140},
    {19.22, 0.0,  0.030, 0.0,  4.36e-5,  1.6938e-4,  5.0e3,   173, 216, 230},
    {30.05, 0.0,  0.024, 0.0,  5.17e-5,  1.6418e-4,  5.0e3,     0,   0, 128},
  };

  srand(1);  // Initialisation de la graine aléatoire
  for (int i = 1; i < N_BODIES; ++i) {
    double dist = bodies[i].x;
    double angle = 2 * M_PI * (double)rand() / RAND_MAX;
    bodies[i].x = dist * cos(angle);
    bodies[i].y = dist * sin(angle);

    double rot_speed = bodies[i].vx;
    bodies[i].vx = -rot_speed * bodies[i].y;
    bodies[i].vy =  rot_speed * bodies[i].x;
  }

  const char * stats_filename = "./img-stats_v1.csv";  // Nom du fichier de statistiques
  const char * png_filename_format = "./img%03d_v1.png";  // Format du nom des fichiers PNG

  // Suppression des fichiers existants
  remove(stats_filename);
  char filename[256];
  if (save_img) {
    for (int i = 0; i < nb_steps; ++i) {
      snprintf(filename, 256, png_filename_format, i);
      remove(filename);
    }
  }

  // Allocation dynamique de la mémoire pour les corps et les images
  struct Body (*tabBodies)[N_BODIES] = malloc(nb_steps * sizeof(struct Body[N_BODIES]));
  if (tabBodies == NULL) {
      fprintf(stderr, "Erreur d'allocation de la mémoire pour tabBodies\n");
      exit(EXIT_FAILURE);
  }

  struct Image **img1=malloc(nb_steps*sizeof(struct Image *));
  if (img1 == NULL) {
      fprintf(stderr, "Erreur d'allocation de la mémoire pour img1\n");
      exit(EXIT_FAILURE);
  }
  struct Image **img2=malloc(nb_steps*sizeof(struct Image *));
  if (img2 == NULL) {
      fprintf(stderr, "Erreur d'allocation de la mémoire pour img2\n");
      exit(EXIT_FAILURE);
  }

  for (int i = 0; i < N_BODIES; ++i) {
    tabBodies[0][i] = bodies[i];
  }

  for (int i=0; i<nb_steps;++i){
    img1[i] = alloc_img(width, height);
    img2[i] = alloc_img(width, height);
  }
  struct ImageStats *stats=malloc(nb_steps*sizeof(struct ImageStats));

  struct timespec t0, t1;
  if (clock_gettime(CLOCK_BOOTTIME, &t0) == -1) {
    perror("clock_gettime");
    exit(1);
  }

  // Déclaration des threads
  pthread_t thread_simulate_bodies;
  pthread_t thread_generate_image_from_bodies;
  pthread_t thread_apply_gaussian_blur;
  pthread_t thread_save_img_as_png;
  pthread_t thread_convert_to_grayscale;
  pthread_t thread_compute_image_statistics;
  pthread_t thread_save_stats;
  int pthread_erreur=0;

  // Création et exécution des threads
  struct args_simulate_bodies asb={tabBodies, nb_steps};
  pthread_erreur = pthread_create(&thread_simulate_bodies, NULL, func_simulate_bodies, &asb);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_create pour simulate_bodies a échoué (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  pthread_erreur = pthread_join(thread_simulate_bodies, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join pour simulate_bodies a échoué (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  struct args_generate_image_from_bodies agifbs={tabBodies, img1, nb_steps};
  pthread_erreur = pthread_create(&thread_generate_image_from_bodies, NULL, func_generate_image_from_bodies, &agifbs);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_create pour generate_image_from_bodies (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  pthread_erreur = pthread_join(thread_generate_image_from_bodies, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join pour generate_image_from_bodies a échoué (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  struct args_2_images a_2_img={img1, img2, nb_steps};
  pthread_erreur = pthread_create(&thread_apply_gaussian_blur, NULL, func_apply_gaussian_blur, &a_2_img);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_create pour apply_gaussian_blur (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  pthread_erreur = pthread_join(thread_apply_gaussian_blur, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join pour apply_gaussian_blur a échoué (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  struct args_save_img_as_png asiap={img2, png_filename_format, nb_steps};
  if (save_img){
    pthread_erreur = pthread_create(&thread_save_img_as_png, NULL, func_save_img_as_png, &asiap);
    if (pthread_erreur != 0) {
      fprintf(stderr, "Erreur: pthread_create pour save_img_as_png (%s)\n", strerror(pthread_erreur));
      libe(tabBodies, img1, img2, stats, nb_steps);
      exit(EXIT_FAILURE);
    }
  }

  pthread_erreur = pthread_create(&thread_convert_to_grayscale, NULL, func_convert_to_grayscale, &a_2_img);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_create for convert_to_grayscale (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  pthread_erreur = pthread_join(thread_convert_to_grayscale, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join pour convert_to_grayscale failed (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  struct args_compute_image_statistics acis={img1, stats, nb_steps};
  pthread_erreur = pthread_create(&thread_compute_image_statistics, NULL, func_compute_image_statistics, &acis);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_create pour compute_image_statistics (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  pthread_erreur = pthread_join(thread_compute_image_statistics, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join pour compute_image_statistics failed (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  struct args_save_stats ass={stats, stats_filename, nb_steps};
  pthread_erreur = pthread_create(&thread_save_stats, NULL, func_save_stats, &ass);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Error: pthread_create for save_stats (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }

  if (save_img){
    pthread_erreur = pthread_join(thread_save_img_as_png, NULL);
    if (pthread_erreur != 0) {
      fprintf(stderr, "Erreur: pthread_join for save_img_as_png a échoué (%s)\n", strerror(pthread_erreur));
      libe(tabBodies, img1, img2, stats, nb_steps);
      exit(EXIT_FAILURE);
    }
  }

  pthread_erreur = pthread_join(thread_save_stats, NULL);
  if (pthread_erreur != 0) {
    fprintf(stderr, "Erreur: pthread_join for save_stats a échoué (%s)\n", strerror(pthread_erreur));
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(EXIT_FAILURE);
  }
  libe(tabBodies, img1, img2, stats, nb_steps);

  if (clock_gettime(CLOCK_BOOTTIME, &t1) == -1) {
    perror("clock_gettime");
    libe(tabBodies, img1, img2, stats, nb_steps);
    exit(1);
  }

  int64_t total_ns = ns_diff(&t0, &t1);
  print_elapsed_time_stats(total_ns);

  return 0;
}