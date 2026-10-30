# Station météo — travail du groupe

Cette branche `group-work` est un exercice à compléter. La branche `main`
contient le prototype complet, conservé comme référence. Les noms dans les TODO
indiquent une proposition de répartition ; les rôles peuvent être échangés.

## Récupérer le projet sans compte GitHub

1. Télécharger [le ZIP de group-work](https://github.com/abysse8/station-meteo/archive/refs/heads/group-work.zip).
2. Décompresser le dossier, puis l'ouvrir dans VS Code.
3. Installer GTK 3, un compilateur C, `make` et `pkg-config` (voir README).
4. Ouvrir un terminal dans le dossier et lancer `make`, puis `make run`.

Le dépôt public se lit et se télécharge sans compte. Pour proposer une modification
directement sur GitHub, il faut un compte et les droits adaptés. Sans compte,
envoyer à Julien uniquement les fonctions modifiées, avec leur nom et le fichier,
ou les fichiers accompagnés de la liste des fonctions changées. Julien intègre
les fonctions séparément : Boubacar et Chahid partagent `meteo.c`, Diouma et Julien
partagent `interface.c`. Ne pas remplacer tout le fichier par une ancienne copie.

## Répartition et critères de réussite

| Personne | Fonctions à écrire | À apprendre | Vérification |
|---|---|---|---|
| Boubacar | B1 : `weather_day_add_reading`, et un helper `clamp` si utile | Tableaux, pointeurs, calcul, limites | `make test-simulation` |
| Chahid | C1 : `weather_day_export_txt` | Fichiers, formatage, gestion des erreurs | `make test-export` |
| Diouma | D1 : `update_value_labels` ; D2 : `update_clock_label` | Chaînes, temps, labels GTK | Cas vide, une mesure et journée terminée |
| Julien | J1 : `on_timer`, puis intégration des contributions | Callbacks, rafraîchissement, état des widgets | Démonstration de bout en bout |

La construction de la fenêtre, les cadres, les signaux, les dialogues d'export
et le rendu Cairo sont fournis. Chacun doit pouvoir expliquer sa fonction ;
le prototype peut servir à comparer le comportement après une première tentative.

### Boubacar — B1 : simulation (`meteo.c`)

- Garder `weather_day_init` fourni et utiliser les champs de `WeatherDay`.
- Si `count` vaut déjà 144, renvoyer 0 sans modifier la journée.
- Sinon, remplir `readings[count]` : minute = `count × 10` ; les instants vont
  de 00:00 à 23:50. Incrémenter `count` une seule fois et renvoyer 1.
- Cycle journalier : `cos(2 × PI × (heure - 15) / 24)`.
- Température = moyenne + amplitude × cycle + dérive.
- Humidité = humidité moyenne − 2,5 × amplitude × cycle + dérive.
- Faire varier les dérives doucement : pas aléatoire de −0,3 à +0,3 °C,
  dérive bornée à ±2 °C ; pas d'humidité de −1,5 à +1,5, dérive bornée à ±8.
- Borner les mesures aux constantes de `meteo.h`. Un helper `clamp` peut ramener
  une valeur dans un intervalle.

Les tests vérifient le nombre, les instants, les limites et le refus d'une 145e
mesure. Ils ne prouvent pas la forme du cycle : vérifier aussi la formule et
les courbes avec le groupe. Aucun capteur réel n'est utilisé.

### Chahid — C1 : export (`meteo.c`)

- Écrire dans `path` un fichier texte avec le titre `WEATHER STATION`, la date,
  les noms des colonnes et exactement `day->count` lignes de données.
- Format des données : `HH:MM`, température avec une décimale, humidité entière,
  avec des espaces entre les colonnes. Accepter aussi une journée partielle.
- Renvoyer 0 si tout réussit ; sinon −1 avec `errno` renseigné.
- Vérifier l'ouverture, les erreurs d'écriture et la fermeture. Fermer chaque
  fichier ouvert, y compris si une écriture échoue.

Le test utilise ses propres mesures : B1 n'a pas besoin d'être terminé. Le bouton
et les dialogues sont déjà fournis dans `interface.c` ; ne pas les réécrire.

### Diouma — D1 et D2 : affichage (`interface.c`)

- D1 : si `day.count == 0`, afficher `-- °C` et `-- %` sans accéder au tableau.
  Sinon, afficher `readings[count - 1]`, température avec une décimale et humidité
  entière. Utiliser les labels existants, sans en créer de nouveaux.
- D2 : afficher l'instant simulé de la dernière mesure, le compteur sur 144,
  l'heure réelle du PC et la vitesse (10 minutes simulées pour 0,2 seconde réelle).
- Prévoir le cas vide (`--:--`), la journée complète (`day complete`) et un échec
  de `localtime`. L'heure réelle doit continuer à évoluer après la simulation.
- Utiliser des buffers bornés (`g_snprintf`), `gtk_label_set_text` ou
  `gtk_label_set_markup` et les fonctions de `<time.h>`.

Pour tester sans B1/J1, dans une copie locale de travail, placer temporairement
une mesure connue et `day.count = 1` dans `interface_create`, juste avant les
appels aux fonctions d'affichage. Tester aussi 0 et 144 mesures valides. Retirer
ces données de test avant d'envoyer la contribution.

### Julien — J1 : timer et intégration (`interface.c`)

- Demander une nouvelle mesure à chaque tick.
- Après un ajout réussi, actualiser les valeurs et demander le redessin des
  deux graphiques avec `gtk_widget_queue_draw`.
- Activer l'export lorsque la 144e mesure est disponible.
- Actualiser l'horloge à chaque tick, même une fois la journée terminée.
- Conserver le timer avec `G_SOURCE_CONTINUE` ; ne pas utiliser `sleep` ou une
  boucle qui bloque GTK.
- Intégrer seulement les fonctions appartenant à chaque personne, compiler avec
  les avertissements, puis tester la chaîne complète.

## État initial et validation finale

La version d'exercice doit compiler. Au départ, les labels affichent des TODO,
aucune mesure n'est générée, le bouton reste désactivé et les tests échouent.
Le stub d'export renvoie `ENOSYS` et ne touche à aucun fichier. C'est intentionnel.

Après les contributions :

```sh
make -B
make test-simulation
make test-export
make run
```

Vérifier au démarrage les valeurs vides, puis les mises à jour et le tracé des
deux courbes. Après environ 29 secondes, vérifier 144 mesures, un export actif,
un fichier contenant toutes les lignes et l'horloge réelle toujours active.
Redimensionner et fermer la fenêtre. Faire aussi un export depuis un dossier
non accessible en écriture pour vérifier le dialogue d'erreur.

La démonstration graphique reste à faire sur un poste avec affichage ; une
compilation réussie ne remplace pas cette vérification.
