#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define t      mat[i][j]
#define u_char unsigned char
// GLOBALS

u_char **mat;
int      h, w;
u_char   mod;

void addToECells(int a, int b) {
  mod = (mat[a][b] & 3);
  mod <<= mod << 1;
  for (int i = MAX(a - 1, 0); i <= MIN(a + 1, h); i++)
    for (int j = MAX(b - 1, 0); j <= MIN(b + 1, w + 1); j++)
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
  for (int j = 1; j <= w; j++) {
    mat[0][j] = mat[h][j];
    mat[h + 1][j] = mat[1][j];
    addToECells(0, j);
    addToECells(h + 1, j);
  }

  for (int i = 1; i < h; i++) {
    mat[i][0] = mat[i][w];
    mat[i][w + 1] = mat[i][1];
    addToECells(i, 0);
    addToECells(i, w + 1);
  }

  mat[0][0] = mat[h][w];
  mat[0][w + 1] = mat[h][1];
  mat[h + 1][0] = mat[1][w];
  mat[h + 1][w + 1] = mat[1][1];

  addToECells(0, 0);
  addToECells(0, w + 1);
  addToECells(h + 1, 0);
  addToECells(h + 1, w + 1);

  /* Normal AddToCells */
  for (int i = 1; i <= h; i++)
    for (int j = 1; j <= w; j++)
      addToCells(i, j);
}

void evolveNormal(void) {
  doAdditions();
  /* Rules */
  for (int i = 1; i <= h; i++)
    for (int j = 1; j <= w; j++)
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
  unsigned char s1, s2;
  doAdditions();
  for (int i = 1; i <= h; i++) {
    for (int j = 1; j <= w; j++) {
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
  unsigned char s1, s2;
  doAdditions();
  for (int i = 1; i <= h; i++) {
    for (int j = 1; j <= w; j++) {
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
  unsigned char s1, s2;
  doAdditions();
  for (int i = 1; i <= h; i++) {
    for (int j = 1; j <= w; j++) {
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
  unsigned char s1, s2;
  doAdditions();
  for (int i = 1; i <= h; i++) {
    for (int j = 1; j <= w; j++) {
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

void do_evolution(int steps, void (*evolution)(void)) {
  while (steps--) {
    evolution();
    print_matrix();
    usleep(500000);
  }
}

void print_matrix(void) {
  printf("\n");
  for (int i = 1; i <= h; i++) {
    for (int j = 1; j <= w; j++) {
      printf("%d ", mat[i][j] & 3);
    }
    printf("\n");
  }
}

int evolve_main(void) {
  int mode;

  srand(time(NULL));
  /*   printf("Enter Size (W H): ");
    scanf("%d %d", &h, &w);

    mat = malloc((h + 2) * sizeof(u_char *));
    for (int i = 0; i <= h + 1; i++)
      mat[i] = calloc((w + 2), sizeof(u_char));

    printf("Enter Matrix:\n");
    for (int i = 1; i <= h; i++)
      for (int j = 1; j <= w; j++) {
        scanf("%d", &mat[i][j]);
      }
   */
  int steps = 3;
  void (*evolution_modes[])() = {evolveNormal, evolveCoExist, evolvePredator,
                                 evolveVirus, evolveUnknown};

  /* print_matrix();
  printf("\nEnter Mode: ");
  scanf("%d", &mode); */

  do_evolution(steps, evolution_modes[mode]);

  for (int i = 0; i <= h + 1; i++)
    free(mat[i]);
  free(mat);

  return 0;
}