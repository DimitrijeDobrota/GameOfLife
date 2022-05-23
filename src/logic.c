#include <stdio.h>
#include <stdlib.h>

#include "logic.h"
#include "utils.h"

cell **mat;
char **evolution_names[] = {"Normal", "CoExsistance", "Predator", "Virus",
                            "Unknown"};

static void (*evolution_modes[])() = {
    evolveNormal, evolveCoExist, evolvePredator, evolveVirus, evolveUnknown};
static void (*evolve)(void);
static int height, width;
static int mod;

void addToECells(int a, int b) {
  mod = (mat[a][b] & 3);
  mod <<= mod << 1;
  for (int i = MAX(a - 1, 0); i <= MIN(a + 1, height); i++)
    for (int j = MAX(b - 1, 0); j <= MIN(b + 1, width + 1); j++)
      if (i != a || j != b)
        mat[i][j] += mod;
}
void addToCells(int i, int j) {
  mod = (mat[i][j] & 3);
  mod <<= mod << 1;
  for (int k = i - 1; k <= i + 1; k++)
    for (int l = j - 1; l <= j + 1; l++)
      if (k != i || l != j)
        mat[k][l] += mod;
}

void doAdditions(void) {
  for (int j = 1; j <= width; j++) {
    mat[0][j] = mat[height][j];
    mat[height + 1][j] = mat[1][j];
    addToECells(0, j);
    addToECells(height + 1, j);
  }

  for (int i = 1; i < height; i++) {
    mat[i][0] = mat[i][width];
    mat[i][width + 1] = mat[i][1];
    addToECells(i, 0);
    addToECells(i, width + 1);
  }

  mat[0][0] = mat[height][width];
  mat[0][width + 1] = mat[height][1];
  mat[height + 1][0] = mat[1][width];
  mat[height + 1][width + 1] = mat[1][1];

  addToECells(0, 0);
  addToECells(0, width + 1);
  addToECells(height + 1, 0);
  addToECells(height + 1, width + 1);

  /* Normal AddToCells */
  for (int i = 1; i <= height; i++)
    for (int j = 1; j <= width; j++)
      addToCells(i, j);
}

void evolveNormal(void) {
  doAdditions();
  /* Rules */
  for (int i = 1; i <= height; i++)
    for (int j = 1; j <= width; j++)
      switch (mat[i][j]) {
      case 9:
      case 12:
      case 13:
        mat[i][j] = 1;
        break;
      default:
        mat[i][j] = 0;
      }
}

void evolveCoExist(void) {
  int s1, s2;
  doAdditions();
  for (int i = 1; i <= height; i++) {
    for (int j = 1; j <= width; j++) {
      s2 = mat[i][j] >> 5;
      s1 = (mat[i][j] & 31) >> 2;
      if ((mat[i][j] & 3) == 0) {
        if ((s1 + s2) == 3) {
          if (mat[i][j] >= 64)
            mat[i][j] = 2;
          else
            mat[i][j] = 1;
        }
        continue;
      }
      if ((s1 + s2) < 2 || (s1 + s2) > 3) {
        mat[i][j] = 0;
        continue;
      }
      mat[i][j] = mod;
    }
  }
}

void evolvePredator(void) {
  int s1, s2;
  doAdditions();
  for (int i = 1; i <= height; i++) {
    for (int j = 1; j <= width; j++) {
      s2 = mat[i][j] >> 5;
      s1 = (mat[i][j] & 31) >> 2;
      mod = mat[i][j] & 3;
      if ((s1 + s2) < 2 || (s1 + s2) > 3) {
        mat[i][j] = 0;
        continue;
      }
      switch (mod) {
      case 0:
        if ((s1 + s2) == 3) {
          if (mat[i][j] >= 64)
            mat[i][j] = 2;
          else
            mat[i][j] = 1;
        }
        break;
      case 1:
        if (s2 > 0)
          mat[i][j] = 0;
        break;
      }
      mat[i][j] = mod;
    }
  }
}

void evolveVirus(void) {
  int s1, s2;
  doAdditions();
  for (int i = 1; i <= height; i++) {
    for (int j = 1; j <= width; j++) {
      s2 = mat[i][j] >> 5;
      s1 = (mat[i][j] & 31) >> 2;
      mod = mat[i][j] & 3;
      if ((s1 + s2) < 2 || (s1 + s2) > 3) {
        mat[i][j] = 0;
        continue;
      }
      switch (mod) {
      case 0:
        if ((s1 + s2) == 3) {
          if (mat[i][j] >= 64)
            mat[i][j] = 2;
          else
            mat[i][j] = 1;
        }
        break;
      case 1:
        if (s2 > 0)
          mat[i][j] = 2;
        break;
      }
      mat[i][j] = mod;
    }
  }
}

void evolveUnknown(void) { // Assumption 3 ones and 3 twos result in 50/50
                           // chanse of 0 becoming one of them:
  int s1, s2;
  doAdditions();
  for (int i = 1; i <= height; i++) {
    for (int j = 1; j <= width; j++) {
      s2 = mat[i][j] >> 5;
      s1 = (mat[i][j] & 31) >> 2;
      mod = mat[i][j] & 3;
      switch (mod) {
      case 0:
        if (s1 == 3 && s2 == 3) {
          mat[i][j] = rand() % 2 + 1;
          continue;
        }
        if (s1 == 3) {
          mat[i][j] = 1;
          continue;
        }
        if (s2 == 3) {
          mat[i][j] = 2;
          continue;
        }
        break;
      case 1:
        if (s1 < 2 || s1 > 3) {
          mat[i][j] = 0;
          continue;
        }
        break;
      case 2:
        if (s2 < 2 || s2 > 3) {
          mat[i][j] = 0;
          continue;
        }
        break;
      }

      mat[i][j] = mod;
    }
  }
}

void do_evolution(int steps) {
  while (steps--) {
    evolve();
  }
}

int logic_init(int w, int h) {
  width = w;
  height = h;

  mat = malloc((h + 2) * sizeof(cell *));
  for (int i = 0; i <= h + 1; i++)
    mat[i] = calloc((w + 2), sizeof(cell));

  return 1;
}

int evolution_init(int index) {
  evolve = evolution_modes[index];
  return 1;
}

int logic_free(void) {
  for (int i = 0; i <= height + 1; i++)
    free(mat[i]);
  free(mat);
  return 1;
}