/*
 * tiny_ekf_struct.h: common data structure for TinyEKF
 *
 * You should #include this file after using #define for N (states) and M
*  (observations)
 *
 * Copyright (C) 2016 Simon D. Levy
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
 * along with this code.  If not, see <http:#www.gnu.org/licenses/>.
 */

typedef struct {
    int n;          /* number of state values */
    int m;          /* number of observables */

    float x[Nsta];    /* state vector */

    float P[Nsta][Nsta];  /* prediction error covariance */
    float Q[Nsta][Nsta];  /* process noise covariance */
    float R[Mobs][Mobs];  /* measurement error covariance */

    float G[Nsta][Mobs];  /* Kalman gain; a.k.a. K */

    float F[Nsta][Nsta];  /* Jacobian of process model */
    float H[Mobs][Nsta];  /* Jacobian of measurement model */

    float Ht[Nsta][Mobs]; /* transpose of measurement Jacobian */
    float Ft[Nsta][Nsta]; /* transpose of process Jacobian */
    float Pp[Nsta][Nsta]; /* P, post-prediction, pre-update */

    float fx[Nsta];   /* output of user defined f() state-transition function */
    float hx[Mobs];   /* output of user defined h() measurement function */

    /* temporary storage */
    float tmp0[Nsta][Nsta];
    float tmp1[Nsta][Mobs];
    float tmp2[Mobs][Nsta];
    float tmp3[Mobs][Mobs];
    float tmp4[Mobs][Mobs];
    float tmp5[Mobs];
} ekf_t;
