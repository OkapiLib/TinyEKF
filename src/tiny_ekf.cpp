/*
 * TinyEKF: Extended Kalman Filter for embedded processors
 *
 * Copyright (C) 2015 Simon D. Levy
 *
 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with ekf-> code.  If not, see <http:#www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdlib.h>
#include "tiny_ekf.h"

/* Cholesky-decomposition matrix-inversion code, adapated from
   http://jean-pierre.moreau.pagesperso-orange.fr/Cplus/choles_cpp.txt */

static int choldc1(float *a, float *p, const int n) {
    int i,j,k;
    float sum;

    for (i = 0; i < n; i++) {
        for (j = i; j < n; j++) {
            sum = a[i * n + j];
            for (k = i - 1; k >= 0; k--) {
                sum -= a[i * n + k] * a[j * n + k];
            }
            if (i == j) {
                if (sum <= 0) {
                    return 1; /* error */
                }
                p[i] = sqrt(sum);
            }
            else {
                a[j * n + i] = sum / p[i];
            }
        }
    }

    return 0; /* success */
}

static int choldcsl(float * A, float * a, float * p, int n) {
    int i,j,k;
    float sum;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            a[i * n + j] = A[i * n + j];
    if (choldc1(a, p, n))
      return 1;

    for (i = 0; i < n; i++) {
        a[i * n + i] = 1 / p[i];
        for (j = i + 1; j < n; j++) {
            sum = 0;
            for (k = i; k < j; k++) {
                sum -= a[j * n + k] * a[k * n + i];
            }
            a[j * n + i] = sum / p[j];
        }
    }

    return 0; /* success */
}


static int cholsl(float *A, float *a, float *p, const int n) {
    int i,j,k;

    if (choldcsl(A,a,p,n))
      return 1;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            a[i * n + j] = 0.0;
        }
    }

    for (i = 0; i < n; i++) {
        a[i*n+i] *= a[i*n+i];
        for (k = i + 1; k < n; k++) {
            a[i * n + i] += a[k * n + i] * a[k * n + i];
        }
        for (j = i + 1; j < n; j++) {
            for (k = j; k < n; k++) {
                a[i * n + j] += a[k * n + i] * a[k * n + j];
            }
        }
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            a[i * n + j] = a[j * n + i];
        }
    }

    return 0; /* success */
}

static void zeros(float *a, const int m, const int n) {
    int j;
    for (j = 0; j < m * n; ++j)
        a[j] = 0;
}

/* C <- A * B */
static void mulmat(const float *a, const float *b, float *c, const int arows, const int acols, const int bcols) {
    int i, j,l;
    for (i = 0; i < arows; ++i)
        for (j = 0; j < bcols; ++j) {
            c[i * bcols + j] = 0;
            for (l = 0; l < acols; ++l)
                c[i * bcols + j] += a[i * acols + l] * b[l * bcols + j];
        }
}

static void mulvec(const float *a, const float *x, float *y, const int m, const int n) {
    int i, j;
    for (i = 0; i < m; ++i) {
        y[i] = 0;
        for (j = 0; j < n; ++j)
            y[i] += x[j] * a[i * n + j];
    }
}

static void transpose(float *a, float *at, const int m, const int n) {
    int i,j;
    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j) {
            at[j * m + i] = a[i * n + j];
        }
}

/* A <- A + B */
static void accum(float *a, const float *b, const int m, const int n) {
    int i,j;
    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j)
            a[i * n + j] += b[i * n + j];
}

/* C <- A + B */
static void add(const float *a, const float *b, float *c, const int n) {
    int j;
    for (j = 0; j < n; ++j)
        c[j] = a[j] + b[j];
}


/* C <- A - B */
static void sub(float *a, float *b, float *c, const int n) {
    int j;
    for (j = 0; j < n; ++j)
        c[j] = a[j] - b[j];
}

static void negate(float *a, const int m, const int n) {
    int i, j;
    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j)
            a[i * n + j] = -a[i * n + j];
}

static void mat_addeye(float *a, const int n) {
  int i;
    for (i = 0; i < n; ++i)
        a[i * n + i] += 1;
}

/* TinyEKF code ------------------------------------------------------------------- */

typedef struct {
    float *x;    /* state vector */

    float *P;  /* prediction error covariance */
    float *Q;  /* process noise covariance */
    float *R;  /* measurement error covariance */

    float *G;  /* Kalman gain; a.k.a. K */

    float *F;  /* Jacobian of process model */
    float *H;  /* Jacobian of measurement model */

    float *Ht; /* transpose of measurement Jacobian */
    float *Ft; /* transpose of process Jacobian */
    float *Pp; /* P, post-prediction, pre-update */

    float *fx;  /* output of user defined f() state-transition function */
    float *hx;  /* output of user defined h() measurement function */

    /* temporary storage */
    float *tmp0;
    float *tmp1;
    float *tmp2;
    float *tmp3;
    float *tmp4;
    float *tmp5;
} ekf_t;

static void unpack(void *v, ekf_t *ekf, const int n, const int m) {
    /* skip over n, m in data structure */
    char *cptr = (char*)v;
    cptr += 2 * sizeof(int);

    float *dptr = (float*)cptr;
    ekf->x = dptr;
    dptr += n;
    ekf->P = dptr;
    dptr += n * n;
    ekf->Q = dptr;
    dptr += n * n;
    ekf->R = dptr;
    dptr += m * m;
    ekf->G = dptr;
    dptr += n * m;
    ekf->F = dptr;
    dptr += n * n;
    ekf->H = dptr;
    dptr += m * n;
    ekf->Ht = dptr;
    dptr += n * m;
    ekf->Ft = dptr;
    dptr += n * n;
    ekf->Pp = dptr;
    dptr += n * n;
    ekf->fx = dptr;
    dptr += n;
    ekf->hx = dptr;
    dptr += m;
    ekf->tmp0 = dptr;
    dptr += n * n;
    ekf->tmp1 = dptr;
    dptr += n * m;
    ekf->tmp2 = dptr;
    dptr += m * n;
    ekf->tmp3 = dptr;
    dptr += m * m;
    ekf->tmp4 = dptr;
    dptr += m * m;
    ekf->tmp5 = dptr;
  }

void ekf_init(void *v, const int n, const int m) {
    /* retrieve n, m and set them in incoming data structure */
    int *ptr = (int*)v;
    *ptr = n;
    ptr++;
    *ptr = m;

    /* unpack rest of incoming structure for initlization */
    ekf_t ekf;
    unpack(v, &ekf, n, m);

    /* zero-out matrices */
    zeros(ekf.P, n, n);
    zeros(ekf.Q, n, n);
    zeros(ekf.R, m, m);
    zeros(ekf.G, n, m);
    zeros(ekf.F, n, n);
    zeros(ekf.H, m, n);
}

bool ekf_step(void *v, float *z) {
    /* unpack incoming structure */
    int *ptr = (int*)v;
    int n = *ptr;
    ptr++;
    int m = *ptr;

    ekf_t ekf;
    unpack(v, &ekf, n, m);

    /* P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1} */
    mulmat(ekf.F, ekf.P, ekf.tmp0, n, n, n);
    transpose(ekf.F, ekf.Ft, n, n);
    mulmat(ekf.tmp0, ekf.Ft, ekf.Pp, n, n, n);
    accum(ekf.Pp, ekf.Q, n, n);

    /* G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1} */
    transpose(ekf.H, ekf.Ht, m, n);
    mulmat(ekf.Pp, ekf.Ht, ekf.tmp1, n, n, m);
    mulmat(ekf.H, ekf.Pp, ekf.tmp2, m, n, n);
    mulmat(ekf.tmp2, ekf.Ht, ekf.tmp3, m, n, m);
    accum(ekf.tmp3, ekf.R, m, m);
    if (cholsl(ekf.tmp3, ekf.tmp4, ekf.tmp5, m))
      return false;
    mulmat(ekf.tmp1, ekf.tmp4, ekf.G, n, m, m);

    /* \hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k)) */
    sub(z, ekf.hx, ekf.tmp5, m);
    mulvec(ekf.G, ekf.tmp5, ekf.tmp2, n, m);
    add(ekf.fx, ekf.tmp2, ekf.x, n);

    /* P_k = (I - G_k H_k) P_k */
    mulmat(ekf.G, ekf.H, ekf.tmp0, n, m, n);
    negate(ekf.tmp0, n, n);
    mat_addeye(ekf.tmp0, n);
    mulmat(ekf.tmp0, ekf.Pp, ekf.P, n, n, n);

    /* success */
    return true;
}
