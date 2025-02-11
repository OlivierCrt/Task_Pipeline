# Rapport

## Composition du groupe
- 22116059 Olivier Crampette
- 22105648 Solal Danton Laloy
- 22205982 Cyprien Gay
- 22206700 Ismael Monge Rouchdi

## Comment Compiler
Utilisez la commande suivante pour compiler le programme :
gcc -o [nom executable] [dm-version.c] tasks.c -lpng -lpthread -lm

## Comment Exécuter
Exécutez le programme avec la commande suivante :
./dm-v1 <nb-steps> <img-width> <img-height> <save-img>

### Paramètres :
- `<nb-steps>` : Nombre de photos à extraire de la simulation.
- `<img-width>` : Largeur de l'image en pixels (influence la qualité des images obtenues).
- `<img-height>` : Hauteur de l'image en pixels (influence la qualité des images obtenues).
- `<save-img>` : "1" pour sauvegarder les images, "0" pour ne pas les sauvegarder.

## Résultats

### Version 1 :
(base) root@LAPTOP-69PTGC54:/home/python/Task_Pipeline/src# ./dm-v1 1 1080 1080 1
ok

Temps total :        344 ms, 303728 ns (100.00 % du total)

Détail par étape :
- nbodies_simulation: 3436 ns (0.00 % du total)
- image_generation: 386311 ns (0.11 % du total)
- image_gaussian_blur: 274 ms, 512002 ns (79.73 % du total)
- image_save_fs: 53 ms, 839303 ns (15.64 % du total)
- image_grayscale: 10 ms, 314266 ns (3.00 % du total)
- image_stats: 5 ms, 120631 ns (1.49 % du total)
- stats_save_fs: 122517 ns (0.04 % du total)

### Version 2 :
(base) root@LAPTOP-69PTGC54:/home/python/Task_Pipeline/src# ./dm-v2 1 1080 1080 1
Temps total :        328 ms, 701164 ns (100.00 % du total)

Détail par étape :
- nbodies_simulation: 2670 ns (0.00 % du total)
- image_generation: 291334 ns (0.09 % du total)
- image_gaussian_blur: 287 ms, 286426 ns (87.40 % du total)
- image_save_fs: 40 ms, 315043 ns (12.26 % du total)
- image_grayscale: 8 ms, 85769 ns (2.46 % du total)
- image_stats: 5 ms, 261065 ns (1.60 % du total)
- stats_save_fs: 195389 ns (0.06 % du total)

### Version 3 :
(base) root@LAPTOP-69PTGC54:/home/python/Task_Pipeline/src# ./dm-v3 1 1080 1080 1
ok

Temps total :        514 ms, 776343 ns (100.00 % du total)

Détail par étape :
- nbodies_simulation: 9107 ns (0.00 % du total)
- image_generation: 1 ms, 444346 ns (0.28 % du total)
- image_gaussian_blur: 1 s, 528 ms, 735070 ns (296.97 % du total)
- image_save_fs: 185 ms, 402355 ns (36.02 % du total)
- image_grayscale: 41 ms, 61137 ns (7.98 % du total)
- image_stats: 24 ms, 340447 ns (4.73 % du total)
- stats_save_fs: 358360 ns (0.07 % du total)

## Remarque


Nous avons remarqué que les temps d'exécution variaient beaucoup, notamment en fonction de la machine qui les effectuait.
Nous avons exécuté les fichiers avec une configuration assez grosse, ce qui peut rendre la différence de temps d'exécution moins évidente.

[CPU](https://www.intel.fr/content/www/fr/fr/products/sku/230580/intel-core-i513500-processor-24m-cache-up-to-4-80-ghz/specifications.html)
ram cadencée a 3200 MHz
